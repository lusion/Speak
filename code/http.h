#ifndef __HTTP_H
#define __HTTP_H

#include "channel.h"
#include "connection.h"

class IChannel;
class HTTP : public Timeout {
	public:
		HTTP(int _sock);
		~HTTP();

		int read(char *d, int l);
		int process_channel(void);

		void close(bool terminated=false);

		void timeout(void);
	private:
		int process(void);
		int reply(const char *d, int l);
		int reply(const char *s);
		int reply_result(const char *format, ...);
		int reply_error(const char *code,const char *format, ...);

		char *buffer;
		int length;
		int sock;


		bool queued;
		int from_id;
		IChannel *channel;
		Connection *connection;
		char json_callback[255];
		int aux;
};

#endif
