#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void stop_si(bool condition, const char* message_perror) {
    if (condition) {
        perror(message_perror);
        exit(EXIT_FAILURE);
    }
}
