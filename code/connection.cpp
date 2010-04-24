#include "common.h"

using namespace std;

Connection::Connection(Channel *_channel, int _id,const char *_intro) {
	debug("Created connection %p with intro: %s",this,_intro);
	introduction = strdup(_intro); id = _id;
	channel = _channel;
	active = 1;
	generate_code(auth,8);

	// Ping the connection; gives us a CONNECTION_TIMEOUT
	ping();
}

Connection::~Connection(void) {
	free(introduction);
}

void Connection::kill(void) {
	if (_timeout_waiting) {
		channel = NULL; // disconnect us from the channel, when we timeout we will kill ourselves
	}else{
		debug("Delete myself %p; with %d active (timeout: %d*%d)",this,active,_timeout,_timeout_waiting);
		delete this; // kill ourselves right here and now
	}
}

void Connection::timeout(void) {
	if (!channel) {
		debug("Connection %p timed out and disconnected from channel; suicide.",this);
		delete this;
	}else{
		debug("Connection %p timed out, telling channel %p to kill",this,channel);
		channel->killConnection(id);
	}
}

void Connection::replaceIntroduction(char *_intro) {
	free(introduction);
	introduction = strdup(_intro);
}


// Timeout management
//
void Connection::addActive(void) {
	if (active == 0) set_timeout(this,0);
	active++;
	debug("Connection %p has %d active connections (+1)",this,active);
}
void Connection::removeActive(bool terminated) {
	active--;
	if (active == 0) {
		if (terminated) { // terminated the final connection?
			channel->killConnection(id);
		}else{
			set_timeout(this,CONNECTION_TIMEOUT);
		}
	}
	//debug("Connection %p has %d active connections (-1)",this,active);
}
void Connection::ping(void) {
	if (active == 0) set_timeout(this,CONNECTION_TIMEOUT);
}
