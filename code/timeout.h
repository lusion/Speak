#ifndef __TIMEOUT_H
#define __TIMEOUT_H


class Timeout {
public:
	Timeout(void);
	virtual void timeout(void) = 0;
	bool kill_timeout(void);

	bool _timeout_kill;
	bool _timeout_waiting;
	unsigned int _timeout; // when it needs to timeout
};

void set_timeout(Timeout *timeout, int _time);
int check_timeouts(void);

#endif
