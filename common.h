#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#define PORT 55555
#define BUFFER_LEN 10000

#include <stdbool.h>
void stop_si(bool condition, const char* message_perror);

#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })

#endif //HTTP_COMMON_H
