#ifndef __CHANNEL_H
#define __CHANNEL_H

#include "http.h"
#include "message.h"
#include "connection.h"


using namespace std;

class HTTP;
typedef vector<Message *>::iterator message_iterator;
//typedef vector<Connection *>::iterator connection_iterator;
typedef Connection** connection_iterator;


class IChannel {
	public:
		virtual bool write(const char *d, int l, int from=-1, int type=MESSAGE_MESSAGE);
		virtual bool can_read(void) { return false; }

		virtual void remove_queue(HTTP *c) {}
		virtual void add_queue(HTTP *c) {}

		virtual message_iterator begin(int from_id);
		virtual message_iterator end(void);

		virtual connection_iterator begin_connections(void);
		virtual connection_iterator end_connections(void);
		virtual Connection *addConnection(const char *introduction) {return NULL;}
		virtual Connection *getConnection(int id, const char *auth) { return NULL; }
		virtual void killConnection(int id) {}

		virtual Channel *getChannel(void) = 0;
};

class Channel : public IChannel {
	public:
		Channel(void); 

		bool write(Message *m);
		bool write(const char *d, int l, int from=-1, int type=MESSAGE_MESSAGE);
		bool can_read(void);

		void remove_queue(HTTP *c);
		void add_queue(HTTP *c);

		message_iterator begin(int from_id);
		message_iterator end(void);


		connection_iterator begin_connections(void);
		connection_iterator end_connections(void);

		Connection *addConnection(const char *introduction);
		Connection *getConnection(int id, const char *auth);
		void killConnection(int id);

		Channel *getChannel(void);
	private:
		int last_id;
		vector<Message *> messages;
		vector<HTTP *> queue;
		Connection **connections;
		int max_connections;
};

class WriteChannel : public IChannel {
	public:
		WriteChannel(Channel *c);

		bool write(const char *d,int l, int from=-1, int type=MESSAGE_MESSAGE);

		connection_iterator begin_connections(void);
		connection_iterator end_connections(void);
		Connection *addConnection(const char *introduction);
		Connection *getConnection(int id, const char *auth);
		void killConnection(int id);

		Channel *getChannel(void);
	private:
		Channel *channel;
};
class ReadChannel : public IChannel {
	public:
		ReadChannel(Channel *c);

		bool can_read(void);

		void remove_queue(HTTP *c);
		void add_queue(HTTP *c);

		message_iterator begin(int from_id);
		message_iterator end(void);

		Channel *getChannel(void);
	private:
		Channel *channel;
};

#endif
