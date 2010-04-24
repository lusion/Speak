#include "common.h"
#include "timeout.h"
#include <queue>
#include <time.h>

using namespace std;

Timeout::Timeout(void) {
	_timeout = 0;
	_timeout_waiting = false;
	_timeout_kill = false;
}

// Return true if we managed to kill the timeout
bool Timeout::kill_timeout(void) {
	if (_timeout_waiting) {
		_timeout_kill =true; return false;
	}else return true;
}
/**************
 * Consider using a couple different queues at set timeout lengths
 *  5 second / 10 second / 15 second / 30 second / 1 minute
 */
typedef pair<int, Timeout *> timeout_pair;
priority_queue<timeout_pair, std::vector<timeout_pair>, std::greater<timeout_pair> > timeout_queue;

void set_timeout(Timeout *t, int _time) {
	if (_time <= 0) {
		t->_timeout = 0;
	}else{
		t->_timeout = frame_time+_time;
	}
	if (!t->_timeout_waiting) {
		t->_timeout_waiting = true;
		timeout_queue.push(make_pair(t->_timeout,t));
	}
}
int check_timeouts(void) {
	if (!timeout_queue.size()) return 0;

	Timeout *t;
	while (timeout_queue.size()) {
		if (timeout_queue.top().first <= frame_time) {
			t = timeout_queue.top().second;
			timeout_queue.pop();
			if (t->_timeout <= frame_time) {
				t->_timeout_waiting = false;
				if (t->_timeout_kill) {
					delete t;
				}else if (t->_timeout > 0) {
					// As long as we havnt terminated the timeout
					t->_timeout = 0;
					t->timeout();
				}
			}else{
				timeout_queue.push(make_pair(t->_timeout,t));
			}
		}else{
			return timeout_queue.top().first - frame_time;
		}
	}
	if (timeout_queue.size()) {
                debug("%d timeouts remaining, next %d, now %d",timeout_queue.size(), timeout_queue.top().first, frame_time);
        }else{  
                debug("0 timeouts remaining");
        }
	return 0;
}
