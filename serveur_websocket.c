#include <stdlib.h>
#include <sys/socket.h>

int main(__attribute__((unused)) int argc, char * argv[]) {
    int sockfd_session = atoi(argv[1]);
    send(sockfd_session, "test", 5, 0);
}