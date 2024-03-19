#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "sys/socket.h"

void stop_si(bool condition, const char* message_perror) {
    if (condition) {
        perror(message_perror);
        exit(EXIT_FAILURE);
    }
}

void socket_timeout(int sockfd, int secondes) {
    struct timeval tv;
    tv.tv_sec = secondes;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}
