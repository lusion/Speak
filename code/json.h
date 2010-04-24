class JSON {
public:
	//JSON(const char *callback);
	JSON(void);
	~JSON(void);

	//void grow_buffer(int extra);

	void clear(void);
	void clear(const char *callback);
	void add(const char *s);
	void add_string(const char *s, int l);
	void add_string(const char *s);
	void add(int i);
	void add(unsigned int i);
	void add(char c);
	const char *close(void);

private:
	char *buffer;
	//int buffer_size;
	int pos;
	bool callback;
	bool overflow;

};

extern JSON json;
