#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <vector>
#include <unistd.h>
#include "config.h"
#ifdef HASH_MAP
#ifdef EXT_HASH_MAP
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#else
#include <unordered_map>
#endif
#include "debugalloc.h"
#include "connection.h"
#include "channel.h"

void debug(const char *s,...);
void generate_code(char *str, int length);
void drop_privileges(void);

void speak_quit(void);

void md5(const char *src, char *hex_output);

extern unsigned int frame_time;
