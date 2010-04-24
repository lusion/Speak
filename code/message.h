#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <time.h>
#include "connection.h"

enum MESSAGE_TYPE {
	MESSAGE_MESSAGE,
	MESSAGE_INTRODUCTION,
	MESSAGE_GOODBYE
};

class Message {
public:
	Message(int id, const char *buffer, int length, int from, int type=MESSAGE_MESSAGE);
	~Message(void);

	int type;
	int id;
	char *buffer;
	int length;
	unsigned int added;
	int connection_id;
};

#endif
