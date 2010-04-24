#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "common.h"


#include "speak.h"
#include "channel.h"
#include "message.h"
#include "json.h"
#include "http.h"


using namespace std;

void speak_close(int i);


HTTP::HTTP(int _sock) {
	buffer = NULL;
	sock = _sock;
	queued = false;
	connection = NULL;
	channel = NULL;
}
HTTP::~HTTP() {
	close(false);
	if (buffer) free(buffer);
}

// FIXME: What if there are multiple connections
void HTTP::close(bool terminated) {
	if (queued) {
		channel->remove_queue(this);
		queued = false;
		channel = NULL;
	}

	if (connection) {
		connection->removeActive(terminated);
		connection = NULL;
	}
}

void HTTP::timeout(void) {
	debug("HTTP::timeout for %p (socket %d)\n",this,socket);
	reply_result("timeout");
}

int HTTP::reply(const char *d, int l) {
	if (sock < 0) return 0;

	char response_buffer[256];
	sprintf(response_buffer,"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n",l);
  int rc = send(sock, response_buffer, strlen(response_buffer), 0);
  if (rc < 0)
  {
      printf("HTTP::reply failed");
	}

  rc = send(sock, d, l, 0);
  if (rc < 0)
  {
      printf("HTTP::reply failed");
	}
	speak_close(sock); // reply and close the socket
	sock = -1;
	return 0;
}
int HTTP::reply(const char *s) {
	return reply(s,strlen(s));
}
int HTTP::reply_result(const char *format, ...) {
  char buffer[9000];
  va_list args;
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);

	// If we're calling json function
	json.clear(json_callback);
	json.add("\"result\":");
	json.add_string(buffer);
	json.add(",\"aux\":"); json.add(aux);
	return reply(json.close());
}
int HTTP::reply_error(const char *code, const char *format, ...) {
  char buffer[9000];
  va_list args;
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);

	// If we're calling json function
	json.clear(json_callback);
	json.add("\"error_code\":");
	json.add_string(code);
	json.add(",\"error\":");
	json.add_string(buffer);
	json.add(",\"aux\":"); json.add(aux);
	return reply(json.close());
}

int HTTP::process_channel(void) {
	int k;

	vector<Message *>::iterator it = channel->begin(from_id);
	if (from_id > 0 && it == channel->end()) {
		if (queued) {
			printf("http process was already queued\n");
		}else{
			//debug("add this(%p) to queue on channel %p",this,channel);
			channel->add_queue(this);
			queued = true;
		}
		return 1; // dont quit
	}

	// Will return 0, indicating not queued
	if (queued) queued = false;
 
	json.clear(json_callback);

	
	json.add("\"messages\": [");
	bool first = true;
	if (from_id <= 0) {
		connection_iterator cit;
		for (cit = channel->begin_connections(); cit != channel->end_connections(); cit++) {
			if (*cit) {
				if (!first) {
					json.add(',');
				}else{
					first = false;
				}
				json.add("{\"type\":\"introduction\",\"message\":");
				json.add_string((*cit)->introduction);
				json.add(",\"from\":");
				json.add((*cit)->id);
				json.add('}');
			}
		}
	}

	int last_id = from_id;
	if (last_id < 1) last_id = 1;
	for (; it != channel->end(); it++) {
		if (from_id < 0) {
			last_id = (*it)->id;
			continue;
		}
		if (!first) {
			json.add(',');
		}else{
			first	= false;
		}
		json.add("{\"message\":");
		json.add_string((*it)->buffer,(*it)->length);
		json.add(",\"time\":");
		json.add((*it)->added);
		if ((*it)->connection_id >= 0) {
			json.add(",\"from\":");
			json.add((*it)->connection_id);
		}
		if ((*it)->type == MESSAGE_INTRODUCTION) {
			json.add(",\"type\":\"introduction\"}");
		}else if ((*it)->type == MESSAGE_GOODBYE) {
			json.add(",\"type\":\"goodbye\"}");
		}else{
			json.add(",\"type\":\"message\"}");
		}
		last_id = (*it)->id;
	}
	json.add("],\"aux\":"); json.add(aux);
	json.add(",\"last_id\":"); json.add(last_id);

	reply(json.close());
	
	// If we're on a connection; remove us as an active client
	if (connection) {
		//debug("process read complete on %p; remove action %p",this,connection);
		connection->removeActive(false);
		connection = NULL;
	}
	return 0;
}
int HTTP::process(void) {
	int k,j;

	// Variables
	int connection_id = -1;
	char auth[33];
	char url[4097];

	// Holds a pointer to the message
	char *message = NULL;
	int message_length;

	json_callback[0] = 0;
	from_id = 1;
	aux = 0;
	connection = NULL;
	channel = NULL;

	// Simple line parser
	const int STATE_URL = 1;
	const int STATE_VARIABLE = 2;
	const int STATE_VALUE = 3;
	int state = STATE_URL;

	char var[256]; int varL = 0;

	char valueBuffer[4097]; int valL = 0;
	char *val = valueBuffer;

	//debug("Process HTTP Request\n%s\n--------\n",buffer);
	// Parse the first line, look for a get request
	if (memcmp(buffer,"GET ",4) == 0 || memcmp(buffer,"POST ",5) == 0) {
		// Get request
		for (k = 4; k < length; k++) {
			if ((state == STATE_VALUE && buffer[k] == '&') ||
					(buffer[k] == ' ' || buffer[k] == '\n' || buffer[k] == '\r'))
			{
				if (state == STATE_URL) {
					// If we quit on the URL, save the value
					val[valL] = 0; strcpy(url,val);
				}else{
					var[varL] = 0; val[valL] = 0;

					if (strcmp(var,"channel") == 0) {
						channel = speak.getChannel(val);
						//debug("Got channel %p for %s",channel,val);
					}else if (strcmp(var,"from_id") == 0) {
						from_id = atoi(val);
					}else if (strcmp(var,"connection") == 0) {
						connection_id = atoi(val);
					}else if (strcmp(var,"aux") == 0) {
						aux = atoi(val);
					}else if (strcmp(var,"timeout") == 0) {
						set_timeout(this, atoi(val));
					}else if (strcmp(var,"auth") == 0) {
						if (strlen(val) > 32) {
							return reply_result("Maximum 32 letter auth codes");
						}
						strcpy(auth,val);
					}else if (strcmp(var,"callback") == 0) {
						if (strlen(val) > 254) {
							return reply_result("Maximum 254 letter callbacks");
						}
						strcpy(json_callback,val);
					}else if (strcmp(var,"message") == 0) {
						if (val == valueBuffer) {
							message = strdup(val);
							message_length = valL;
						}else{
							message = val;
							message_length = valL;
							val = valueBuffer;
						}
					}
				}

				// If it was an & get the next variable, otherwise quitz0r
				if (buffer[k] == '&') {
					varL = valL = 0;
					state = STATE_VARIABLE;
				}else{
					break;
				}
			}else if (state == STATE_URL && buffer[k] == '?') {
				state = STATE_VARIABLE;	
				val[valL] = 0;
				strcpy(url,val);
				varL = valL = 0;
			}else if (state == STATE_VARIABLE && buffer[k] == '=') {
				state = STATE_VALUE;
				var[varL] = 0;
				// Scan for the length
				j = k+1;
				while (buffer[j] != ' ' && buffer[j] != '&') j++;

				// Length is j-k;; includes \0
				if (j-k > 4096) {
					if (strcmp(var,"message") == 0) {
						if (j-k > 16384) {
							return reply_result("Maximum field length of message field is 16386. %s was too long",var);
						}else{
							val= (char *)malloc(j-k);
						}
					}else{
						return reply_result("Maximum field length of non-message fields is 4096. %s was too long",var);
					}
				}



			}else if (state == STATE_URL) {
				if (valL == 4096) {
					return reply_result("Url too long");
				}
				val[valL++] = buffer[k];
			}else if (state == STATE_VARIABLE) {
				if (varL == 255) {
					return reply_result("Maximum 254 letter variables");
				}
				var[varL++] = buffer[k];
			}else if (state == STATE_VALUE) {
				/* LENGTH CHECKING DONE WHEN SWITCHING INTO STATE_VALUE if (valL == 4096) {
					return reply_result("Value for %s field too long",var);
				}*/
				if (buffer[k] == '%') {
					int c = 0;
					k++;
					if (buffer[k] >= 'A' && buffer[k] <= 'Z') {
						c += 16 * (10 + (buffer[k] - 'A'));
					}else if (buffer[k] >= 'a' && buffer[k] <= 'z') {
						c += 16 * (10 + (buffer[k] - 'a'));
					}else if (buffer[k] >= '0' && buffer[k] <= '9') {
						c += 16 * (buffer[k] - '0');
					}
					k++;
					if (buffer[k] >= 'A' && buffer[k] <= 'Z') {
						c += (10 + (buffer[k] - 'A'));
					}else if (buffer[k] >= 'a' && buffer[k] <= 'z') {
						c += (10 + (buffer[k] - 'a'));
					}else if (buffer[k] >= '0' && buffer[k] <= '9') {
						c += (buffer[k] - '0');
					}

					val[valL++] = (char) c;
				}else if (buffer[k] == '+') {
					val[valL++] = ' ';
				}else{
					val[valL++] = buffer[k];
				}
			}
		}
	}

	/*****************************************************************************
	 ** MESSAGE ONLY USED WITH url=/write and /introduce
	 ****************************************************************************/
	if (message && strcmp(url,"/write") != 0 && strcmp(url,"/introduce") != 0) {
		free(message);
		return reply_error("PARAM","Message param must only be included with /write or /introduce");
	}

	if (strcmp(url,"/quit-now") == 0) {
		speak_quit();
	}

	if (strcmp(url,"/policy") == 0 || strcmp(url,"/crossdomain.xml") == 0) {
		return reply("HTTP/1.1 200 OK\r\nContent-Type: text/x-cross-domain-policy\r\n\r\n<?xml version=\"1.0\"?>\n<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">\n<cross-domain-policy>\n <allow-access-from domain=\"*\" />\n <allow-http-request-headers-from domain=\"*\" headers=\"*\" />\n</cross-domain-policy>");
	}else if (strcmp(url,"/iframe-test") == 0) {
		return reply("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<script type='text/javascript'>window.location=window.location+'#ok';window.close();</script>");
	}else if (strcmp(url,"/image-test") == 0) {
		char buffer[2048]; int L;
		char gif_data[] = { 0x47, 0x49, 0x46, 
		0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x91, 
		0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 
		0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x21, 0xf9, 
		0x04, 0x01, 0x00, 0x00, 0x02, 0x00, 0x2c, 0x00, 
		0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 
		0x02, 0x02, 0x54, 0x01, 0x00, 0x3b };	
		strcpy(buffer,"HTTP/1.1 200 OK\r\nContent-Type: image/gif\r\nContent-Length: 49\r\n\r\n");
		L = strlen(buffer);
		memcpy(buffer+L, gif_data,49);
		return reply(buffer,L+49);
	}else if (strcmp(url,"/test") == 0) {
		return reply_result("ok");
	}else if (strcmp(url,"/create") == 0) {
		char name_r[17];
		char name_w[17];
		char name_rw[17];

		Channel *rw = new Channel;
		ReadChannel *r = new ReadChannel(rw);
		WriteChannel *w = new WriteChannel(rw);
		speak.addSpecialChannel((IChannel *)rw,name_rw);
		speak.addSpecialChannel((IChannel *)r,name_r);
		speak.addSpecialChannel((IChannel *)w,name_w);

		//debug("R/W (%p) on %s",rw,name_rw);
		//debug("R (%p) on %s",r,name_r);
		//debug("W (%p) on %s",w,name_w);

		json.clear(json_callback);
		json.add("\"result\":\"ok\",");
		json.add("\"r\":");
		json.add_string(name_r);
		json.add(",\"w\":");
		json.add_string(name_w);
		json.add(",\"rw\":");
		json.add_string(name_rw);
		json.add(",\"aux\":");
		json.add(aux);
		return reply(json.close());
	}
	if (!channel) {
		return reply_result("error - no channel given");
	}
	if (connection_id > 0) {
		connection = channel->getConnection(connection_id,auth);
		if (!connection) return reply_error("AUTH","authorization code incorrect");
		else connection->addActive();
	}

	if (strcmp(url,"/read") == 0) {
		// If we're on a connection, add us as an active client
		return process_channel();
	}else if (strcmp(url,"/introduce") == 0) {
		/************************
		 * Must free(message)
		 ***********************/
		if (!message) {
			return reply_result("Please include a message to introduce yourself with");
		}
		if (connection) {
			connection->replaceIntroduction(message);
		}else{
			//debug("channel->addConnection(%s)",message);
			connection = channel->addConnection(message);
			if (!connection) {
				debug("addConnection returned NULL!");
				free(message);
				return reply_result("Could not open a connection");
			}
		}
		// Write out the connection incase anyone missed it
		channel->write(message, message_length, connection->id, MESSAGE_INTRODUCTION);

		// Done with message
		free(message);

		json.clear(json_callback);
		json.add("\"result\":\"ok\",\"auth\":");
		json.add_string(connection->auth);
		json.add(",\"connection\":");
		json.add(connection->id);
		json.add(",\"aux\":"); json.add(aux);
		return reply(json.close());
	}else if (strcmp(url,"/write") == 0) {
		/************************
		 * Must free(message)
		 ***********************/
		if (!message) {
			return reply_result("Please include a message to write");
		}
		//debug("write %s to channel %p",message,channel);
		if (connection) {
			connection->ping(); // keeps the connection alive
			channel->write(message,message_length,connection_id);
		}else{
			channel->write(message,message_length);
		}
		// Done with message
		free(message);
		return reply_result("ok");
	}else{
		return reply_result("Correct commands on /read, /write, /introduce, /test and /create");
	}
}

int HTTP::read(char *d, int l) {
	int start = 4-1; // \r\n\r\n
	if (buffer) {
		buffer = (char *) realloc(buffer, length+l+1);
		memcpy(buffer+length,d,l);
		start = length;
		length += l;
	}else{
		buffer = (char *) malloc(l+1);
		memcpy(buffer,d,l);
		length = l;
	}

	buffer[length]=0;

	int k;
	for (k = start; k < length; k++) {
		if (buffer[k-3] == '\r' && buffer[k-2] == '\n' && buffer[k-1] == '\r' && buffer[k] == '\n') {
			// We have an http request
			return process();
		}
	}
	return 1;
}
