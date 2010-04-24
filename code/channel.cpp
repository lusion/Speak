#include "common.h"


#include "channel.h"

using namespace std;

bool IChannel::write(const char *d, int l, int from, int type) {
	debug("no write");  return false;
}

message_iterator IChannel::begin(int from_id) {
	message_iterator it;
	return it;
}
message_iterator IChannel::end(void) {
	message_iterator it;
	return it;
}
connection_iterator IChannel::begin_connections(void) {
	/*connection_iterator it;
	return it;*/ return NULL;
}
connection_iterator IChannel::end_connections(void) {
	/*connection_iterator it;
	return it;*/ return NULL;
}

Channel *Channel::getChannel(void) { return this; }
Channel *WriteChannel::getChannel(void) { return channel; }
Channel *ReadChannel::getChannel(void) { return channel; }


Channel::Channel(void) {
	last_id = 1;
	max_connections = CHANNEL_MAX_CONNECTIONS;
	connections = (Connection **) malloc(sizeof(Connection *) * max_connections);
	for (int k = 0; k < max_connections; k++) {
		connections[k] = NULL;
	}
}

bool Channel::write(Message *m) {
	messages.push_back(m);
	//debug("wrote to channel %p",this);

	vector<HTTP *>::iterator it;
	for (it = queue.begin(); it != queue.end();) {
		if ((*it)->process_channel() == 0) {
			// Erase it (automatic increment)
			it = queue.erase(it);
		}else{
			it++; // manual increment
		}
	}
	return true;
}
bool Channel::write(const char *d, int l, int from, int type) {
	return write(new Message(++last_id, d, l, from, type));
}

Connection *Channel::addConnection(const char *introduction) {
	for (int k = 0; k < max_connections; k++) {
		if (!connections[k]) {
			// Add one to k; for nice 1 based indexing
			connections[k] = new Connection(this,k+1,introduction);
			//connections.push_back(c);
			debug("Created new connection %p",connections[k]);
			return connections[k];
		}
	}
	debug("Channel %p full (%d)",this,max_connections);
	return NULL;
}
void Channel::killConnection(int id) {
	//debug("channel(%p) kill connection %d (%p)",this,id,connections[id-1]);

	id--; // reduce one; for nice 1 based indexing
	if (!connections[id]) return;
	connections[id]->kill();
	connections[id] = NULL;
	// add one to id, for nice 1 based indexing
	write(NULL,0,id+1,MESSAGE_GOODBYE);
}
Connection *Channel::getConnection(int id, const char *auth) {
	id--; // reduce one; for nice 1 based indexing
	if (id >= 0 && id < max_connections &&
				connections[id] && (strcmp(connections[id]->auth,auth) == 0)) {
		return connections[id];
	}
	//debug("getConnection(%d,%s) = FAIL",id,auth);
	return NULL;
}
connection_iterator Channel::begin_connections(void) {
	return (Connection **) (connections);
}
connection_iterator Channel::end_connections(void) {
	return (Connection **) (connections+max_connections);
}


bool Channel::can_read(void) {
	return true;
}
void Channel::add_queue(HTTP *c) {
	//debug("add %p to queue on channel %p",c,this);
	queue.push_back(c);
}
void Channel::remove_queue(HTTP *c) {
	vector<HTTP *>::iterator it;
	for (it = queue.begin(); it != queue.end(); it++) {
		if ((*it) == c) {
			queue.erase(it);
			return;
		}
	}
	debug("COULD NOT FIND HTTP TO REMOVE FROM QUEUE\n");
}

message_iterator Channel::begin(int from_id) {
	message_iterator it;
	//debug("Reading messages for channel %p from %d",this,from_id);
	if (from_id < 0) {
		it = messages.end();
		if (it != messages.begin()) it--;
		return it;
	}
	for (it	= messages.begin(); it != end();) {
		if (frame_time > (*it)->added+MESSAGE_TIMEOUT) {
			//debug("Message %p sent at %u expired at %u",*it,(*it)->added,frame_time);
			delete (*it);
			it = messages.erase(it);
		}else{
			if ((*it)->id >= from_id) break;
			it++;
		}
	}
	return it;
}

message_iterator Channel::end(void) {
	return messages.end();
}


WriteChannel::WriteChannel(Channel *c) {
	channel = c;
}
bool WriteChannel::write(const char *d, int l, int from, int type) {
	return channel->write(d,l,from,type);
}

ReadChannel::ReadChannel(Channel *c) {
	channel = c;
}
bool ReadChannel::can_read(void) {
	return channel->can_read();
}

// Write channel handles introductions
connection_iterator WriteChannel::begin_connections(void) { return channel->begin_connections(); }
connection_iterator WriteChannel::end_connections(void) { return channel->end_connections(); }
Connection *WriteChannel::addConnection(const char *introduction) { return channel->addConnection(introduction); }
Connection *WriteChannel::getConnection(int id, const char *auth) { return channel->getConnection(id,auth); }
void WriteChannel::killConnection(int id) { return channel->killConnection(id); }

// Read channel handle queues
void ReadChannel::remove_queue(HTTP *c) { channel->remove_queue(c); }
void ReadChannel::add_queue(HTTP *c) { channel->add_queue(c); }
message_iterator ReadChannel::begin(int last) { return channel->begin(last); }
message_iterator ReadChannel::end(void) { return channel->end(); }
