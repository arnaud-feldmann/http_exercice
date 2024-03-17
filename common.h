#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#include <stdbool.h>

#define PORT 55555
#define BUFFER_LEN 10000
#define min(a,b) \
	({ __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b; })
void stop_si(bool condition, const char* message_perror);
void socket_timeout(int sockfd);

#endif //HTTP_COMMON_H
