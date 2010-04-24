#include "common.h"
#include "speak.h"
#include "channel.h"

using namespace std;


Speak::Speak(void) {

}

void Speak::addChannel(const char *name, IChannel *c) {
	channels[name] = c;
}
void Speak::addSpecialChannel(IChannel *c, char *name) {
	uniqueName(name);
	
	//debug("HASH S (%p) on %s",c,name);
	channels[name] = c;
}

IChannel *Speak::getChannel(const char *name) {
	channel_map::iterator it = channels.find(name);	

	if (it != channels.end()) return (*it).second;
	else {
		IChannel *c;

		if (memcmp(name,"*md5:",5) == 0) {
			char md5hashed[64];
			strcpy(md5hashed,"*md5#");
			// Hash the name, store at the end of md5hashed
			md5(name+5,md5hashed+5);
			IChannel *hashedChannel = getChannel(md5hashed);
			
			c = hashedChannel->getChannel();
		}else{
			c = new Channel;

			// Its a hashed channel
			if (memcmp(name,"*md5#",5) == 0) {
				c = new WriteChannel((Channel *)c);
			}
		}
		//debug("HASH R/W (%p) on %s",c,name);
		channels[name] = (IChannel *)c;
		return (IChannel *)c;
	}
}

void Speak::uniqueName(char *name) {
	int i;
	name[0] = '*';
	for (i = 1; i < 16; i++) {
		name[i] = (rand()%10)+'0';
	}
	name[i] = 0;
}
