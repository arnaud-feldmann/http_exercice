#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#define TRUE 1
#define FALSE 0
#define PORT 55555
#define BUFFER_LEN 10000

typedef int bool;
void stop_si(bool condition, const char* message_perror);

#endif //HTTP_COMMON_H
