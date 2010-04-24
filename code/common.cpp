#include "common.h"
#include "md5.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>

using namespace std;


void md5(const char *src, char *hex_output) {
	md5_state_t state;
	md5_byte_t digest[16];
	int di;

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)src, strlen(src));
	md5_finish(&state, digest);
	for (di = 0; di < 16; ++di)
	    sprintf(hex_output + di * 2, "%02x", digest[di]);
}
void debug(const char *format,...) {
#if defined(DEBUG)
  char buffer[9000];
  va_list args;
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);
	printf("DEBUG: %s\n",buffer);
#endif
}

void drop_privileges(void)
{
    int res;
    struct passwd *pw = getpwnam(NOBODY_USER);
    if (!pw) {
        fprintf(stderr, "%s: not found\n", NOBODY_USER);
        exit(1);
    }
    if (setgid(pw->pw_gid) == -1 || setuid(pw->pw_uid) == -1) {
			perror("Could not drop privileges"); exit(1);
		}
}


void generate_code(char *str, int length) {
	const char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int count_letters = strlen(letters);
	int i; 
	for (i = 0; i < length; i++) {
		str[i] = letters[rand()%count_letters];
	}
	str[i] = 0;
}
