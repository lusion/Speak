#include <time.h>
#include "common.h"
#include "message.h"

using namespace std;

Message::Message(int _id, const char *_buffer, int _length, int _from, int _type) {
	id = _id; length = _length; type = _type;
	if (_buffer) {
		buffer = (char *)malloc(length);
		memcpy(buffer,_buffer,length);
	}else buffer = NULL;
	connection_id = _from;

	added = frame_time;
}
Message::~Message(void) {
	if (buffer) free(buffer);
}
