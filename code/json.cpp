#include "common.h"
#include "json.h"

using namespace std;

JSON json;

JSON::JSON(void) {
	buffer = (char *)malloc(JSON_BUFFER_SIZE);
	pos = 0;
	//buffer_size = 4096;
	callback = false;
}
JSON::~JSON(void) {
	free(buffer);
}
/*
void JSON::grow_buffer(int extra) {
	if (buffer_size > extra) buffer_size *= 2;
	else buffer_size += extra;

	buffer = (char *) realloc(buffer, buffer_size);
}
*/

void JSON::clear(const char *_callback) {
	pos = 0;
	if (_callback && _callback[0]) {
		callback = true;
		add(_callback);
		add("({");
	}else{
		callback = false;
		add('{');
	}
}
void JSON::clear(void) {
	pos = 0;
	callback = false;
	add('{');
}

void JSON::add(char c) {
	if (pos + 1 >= JSON_BUFFER_SIZE) return;
	buffer[pos++] = c;
}
void JSON::add(const char *s) {
	int l = strlen(s);
	if (pos + l >= JSON_BUFFER_SIZE) return;

	strcpy(buffer+pos,s);
	pos += strlen(s);
}
void JSON::add(int i) {
	// 64bit number is 20 digits long + terminating \0
	if (pos+21 >= JSON_BUFFER_SIZE) return;

	sprintf(buffer+pos,"%d",i);
	pos += strlen(buffer+pos);
}
void JSON::add(unsigned int i) {
	// 64bit number is 20 digits long + terminating \0
	if (pos+21 >= JSON_BUFFER_SIZE) return;

	sprintf(buffer+pos,"%u",i);
	pos += strlen(buffer+pos);
}
void JSON::add_string(const char *s, int l) {
	int k;

	if (s == NULL) {
		add("null");
		return;
	}
	// Maximum is 2*l + 2 quotes ---- TODO fix to scan properly 
	if ((2+l*2) + pos >= JSON_BUFFER_SIZE) return;

	buffer[pos++] = '"';
	for (k = 0; k < l; k++) {
		if (s[k] == '"' || s[k] == '\\') {
			buffer[pos++] = '\\';
		}else if (s[k] == '\n') {
			buffer[pos++] = '\\';
			buffer[pos++] = 'n';
			continue;
		}
		buffer[pos++] = s[k];
	}
	buffer[pos++] = '"';
}
void JSON::add_string(const char *s) {
	add_string(s,strlen(s));
}

const char *JSON::close(void) {
	if (callback) {
		add("});");
	}else{
		add('}');
	}
	buffer[pos] = 0;
	return buffer;
}
