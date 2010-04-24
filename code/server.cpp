#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>
#include "common.h"
#include "http.h"
#include "speak.h"
#include "timeout.h"

// How many connections can this handle ?

using namespace std;


#define TRUE             1
#define FALSE            0


#define MAX_CONNECTIONS		256

HTTP *connections[MAX_CONNECTIONS];
   
int    listen_sd, max_sd, new_sd;
fd_set        master_set, working_set;

unsigned int frame_time;
Speak speak;

bool end_server = false;

void speak_open(int i) {
	debug("speak_open(%d)",i);
	if (connections[i]) {
		debug("ERROR: %d was already open\n",i);
	}
	connections[i] = new HTTP(i);
	set_timeout(connections[i],HTTP_TIMEOUT);
	// printf("%d opened\n",i);
	fflush(stdout);
}
int speak_read(int i, char *data, int length) {
	return connections[i]->read(data,length);
	//
	// printf("%d read \"%s\"",i,data);
	fflush(stdout);
}
void speak_close(int i) {
	if (connections[i]->kill_timeout()) {
		delete connections[i];
	}else{
		connections[i]->close(); // just close it, let timeout delete
	}
	connections[i] = NULL;
	debug("speak_close(%d)",i);

	// Physically close it
	close(i);
	FD_CLR(i, &master_set);
	if (i == max_sd)
	{
		 while (FD_ISSET(max_sd, &master_set) == FALSE)
				max_sd -= 1;
	}

	// printf("%d closed\n",i);
	fflush(stdout);
}
void speak_terminate(int i) {
	// Connection was killed
	debug("speak_terminate(%d)",i);
	connections[i]->close(true); // close with terminate
	speak_close(i);
}
void speak_quit(void) {
	debug("speak_quit() called");
	end_server = true;
}

// Source from http://publib.boulder.ibm.com/infocenter/iseries/v5r3/topic/rzab6/rzab6cnonblock.htm with minor modifications

void fatal(const char *error) {
	perror(error);
	// close open sockets if we can here
	exit(-1);
}

int listen_port(int port) {
	int listen_sd, on = 1;
   sockaddr_in   addr;

	// Create AF_INIT stream socket
  listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0) { fatal("socket() failed"); }

	// Reusable
  if (setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR,
                  (char *)&on, sizeof(on)) < 0) { fatal("setsockopt() failed"); }

	// Nonblocking
	if (ioctl(listen_sd, FIONBIO, (char *)&on) < 0) { fatal("ioctl() failed"); }

	// Bind it
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
	//addr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);
  addr.sin_port        = htons(port);

  if (bind(listen_sd,
           (struct sockaddr *)&addr, sizeof(addr)) < 0) { fatal("bind() failed"); }

	// Back log
  if (listen(listen_sd, 32) < 0) fatal("listen() failed");

	if (listen_sd > max_sd) max_sd = listen_sd;
  FD_SET(listen_sd, &master_set);

	return listen_sd;
}
int server(void)
{
	srand(time(NULL));
   int    i, len, rc, on = 1;
   int    desc_ready;
   int    close_conn;
   char   buffer[1025];
   timeval       timeout;
   sockaddr_in   addr;

	 for (i = 0; i < MAX_CONNECTIONS; i++) connections[i] = NULL;

   FD_ZERO(&master_set);

	 int listen_sd[2]; int count_listen = 0;

	 listen_sd[count_listen++] = listen_port(SERVER_PORT);
	 debug("Speak server started on %s:%d",SERVER_IP,SERVER_PORT);
#ifdef SERVER_ALTERNATE_PORT
	 listen_sd[count_listen++] = listen_port(SERVER_ALTERNATE_PORT);
	 debug("Speak server started on %s:%d",SERVER_IP,SERVER_ALTERNATE_PORT);
#endif

   /*************************************************************/
   /* Loop waiting for incoming connects or for incoming data   */
   /* on any of the connected sockets.                          */
   /*************************************************************/
	 debug("Dropping privileges");
	 drop_privileges();
   do
   {

		 for (int k = 0; k < MAX_CONNECTIONS; k++){
			if (connections[k]) {
				debug("HTTP connection %d is open",k);
			}
		 }

      /**********************************************************/
      /* Copy the master fd_set over to the working fd_set.     */
      /**********************************************************/
      memcpy(&working_set, &master_set, sizeof(master_set));


			int next_timeout = check_timeouts();
      /**********************************************************/
      /* Call select() and wait 5 minutes for it to complete.   */
      /**********************************************************/
			if (next_timeout <= 0) {
				debug("waiting for more data...");
	      rc = select(max_sd + 1, &working_set, NULL, NULL, NULL);
			}else
			{
				debug("waiting for more data... next timeout in %d seconds",next_timeout);
				timeout.tv_sec = next_timeout;
				timeout.tv_usec = 0;
	      rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
			}

			frame_time = time(NULL);

      /**********************************************************/
      /* Check to see if the select call failed.                */
      /**********************************************************/
      if (rc < 0)
      {
				if (errno == EBADF) {
					debug("got EBADF");
					FD_ZERO(&working_set);
					timeout.tv_sec = 0;
					timeout.tv_usec = 0;
					for (i=0; i <= max_sd; i++) {
						if (!FD_ISSET(i,&master_set)) continue;
						FD_SET(i,&working_set);
						rc = select(i+1,&working_set,NULL,NULL,&timeout) ;
						if (rc < 0) {
							if (errno == EBADF) {
								debug("Found bad file descriptor: %d",i);
								speak_close(i);
								break;
							}else{
								perror("  sub-select() failed");
							}
						}
						FD_CLR(i, &working_set);
					}
					continue; // made it out a live? continue


				}else{
	         perror("  select() failed");
	         break;
				}
      }

      if (rc == 0)
      {
				// select timeout out
				debug("select timed out");
				continue;
      }

      /**********************************************************/
      /* One or more descriptors are readable.  Need to         */
      /* determine which ones they are.                         */
      /**********************************************************/
      desc_ready = rc;
      for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
      {
         /*******************************************************/
         /* Check to see if this descriptor is ready            */
         /*******************************************************/
         if (FD_ISSET(i, &working_set))
         {
            /****************************************************/
            /* A descriptor was found that was readable - one   */
            /* less has to be looked for.  This is being done   */
            /* so that we can stop looking at the working set   */
            /* once we have found all of the descriptors that   */
            /* were ready.                                      */
            /****************************************************/
            desc_ready -= 1;

						bool was_listening = false;
            /****************************************************/
            /* Check to see if this is the listening socket     */
            /****************************************************/
						for (int k = 0; k < count_listen; k++) {
							if (i == listen_sd[k])
							{
								was_listening = true;
								 /*************************************************/
								 /* Accept all incoming connections that are      */
								 /* queued up on the listening socket before we   */
								 /* loop back and call select again.              */
								 /*************************************************/
								 do
								 {
										/**********************************************/
										/* Accept each incoming connection.  If       */
										/* accept fails with EWOULDBLOCK, then we     */
										/* have accepted all of them.  Any other      */
										/* failure on accept will cause us to end the */
										/* server.                                    */
										/**********************************************/
										new_sd = accept(listen_sd[k], NULL, NULL);
										if (new_sd < 0)
										{
											 if (errno != EWOULDBLOCK)
											 {
													perror("  accept() failed");
													end_server = TRUE;
											 }
											 break;
										}
										rc = ioctl(new_sd, FIONBIO, (char *)&on);
										if (rc < 0) {
											perror("  ioctl() failed");
											end_server = TRUE;
											break;
										}

										/**********************************************/
										/* Add the new incoming connection to the     */
										/* master read set                            */
										/**********************************************/
										speak_open(new_sd);
										FD_SET(new_sd, &master_set);
										if (new_sd > max_sd)
											 max_sd = new_sd;

										/**********************************************/
										/* Loop back up and accept another incoming   */
										/* connection                                 */
										/**********************************************/
								 } while (new_sd != -1);

								 // It was a listening socket; look at the next socket now
								 break;
							}
						}

            /****************************************************/
            /* This is not the listening socket, therefore an   */
            /* existing connection must be readable             */
            /****************************************************/
						if (!was_listening)
            {
               /*************************************************/
               /* Receive all incoming data on this socket      */
               /* before we loop back and call select again.    */
               /*************************************************/
               do
               {
                  /**********************************************/
                  /* Receive data on this connection until the  */
                  /* recv fails with EWOULDBLOCK.  If any other */
                  /* failure occurs, we will close the          */
                  /* connection.                                */
                  /**********************************************/
                  rc = recv(i, buffer, sizeof(buffer)-1, 0);
                  if (rc < 0)
                  {
                     if (errno != EWOULDBLOCK)
                     {
                        perror("  recv() failed");
												speak_terminate(i);
                     }
                     break;
                  }

                  /**********************************************/
                  /* Check to see if the connection has been    */
                  /* closed by the client                       */
                  /**********************************************/
                  if (rc == 0)
                  {
										speak_terminate(i);
                    break;
                  }

                  /**********************************************/
                  /* Data was recevied                          */
                  /**********************************************/
									if (speak_read(i, buffer, rc) == 0)
									{
										//debug("speak_read set connection %d was done after reading %d bytes",i,rc);
										close_conn = TRUE;
										break;
									}

                  /**********************************************/
                  /* DONT Echo the data back to the client        
                  rc = send(i, buffer, len, 0);
                  if (rc < 0)
                  {
                     perror("  send() failed");
                     close_conn = TRUE;
                     break;
                  }
                  ***********************************************/

               } while (TRUE);

            } /* End of existing connection is readable */
         } /* End of if (FD_ISSET(i, &working_set)) */
      } /* End of loop through selectable descriptors */

   } while (end_server == FALSE);

	 if (end_server) debug("end_server");
	 else debug("broke out of main loop");

   /*************************************************************/
   /* Cleanup all of the sockets that are open                  */
   /*************************************************************/
   for (i=0; i <= max_sd; ++i)
   {
      if (FD_ISSET(i, &master_set))
         close(i);
   }
	 return 0;
}
