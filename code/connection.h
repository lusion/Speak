#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "timeout.h"

class Channel;
class Connection : public Timeout {
	public:
		Connection(Channel *_channel, int id, const char *_intro);
		~Connection(void);
	
		void kill(void); // responsible for calling 'delete'

		void replaceIntroduction(char *_intro);

		// Connection / timeout management
		void ping(void);
		void addActive(void);
		void removeActive(bool terminated=false);


		void timeout(void);

		Channel *channel;		
		int active;
		char *introduction;
		int id;
		char auth[9];


};

#endif
