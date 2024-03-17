#ifndef HTTP_SERVEUR_WEBSOCKET_H
#define HTTP_SERVEUR_WEBSOCKET_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

struct pthreads_actifs {
    pthread_t recv;
    pthread_t recv_texte;
    pthread_t recv_binaire;
    pthread_t recv_fermeture;
    pthread_t recv_ping;
    pthread_t recv_pong;
};
typedef struct pthreads_actifs pthreads_actifs_t;

struct pipe_actifs {
    int recv_texte[2];
    int recv_binaire[2];
    int recv_fermeture[2];
    int recv_ping[2];
    int recv_pong[2];
};
typedef struct pipe_actifs pipe_actifs_t;

#define CONTINUATION 0x0
#define TEXTE 0x1
#define BINAIRE 0x2
#define FERMETURE 0x8
#define PING 0x9
#define PONG 0xA

typedef uint8_t opcode_t;

struct header_websocket_petit {
    bool fin : 1;
    uint8_t rsv : 3;
    opcode_t opcode: 4;
    bool mask: 1;
    uint8_t payload_length: 7;
};
typedef struct header_websocket_petit header_websocket_petit_t;

struct header_websocket_moyen {
    bool fin : 1;
    uint8_t rsv : 3;
    uint8_t opcode: 4;
    bool mask: 1;
    uint8_t payload_length_1: 7;
    uint16_t payload_length_2: 16;
};
typedef struct header_websocket_moyen header_websocket_moyen_t;

struct header_websocket_grand {
    bool fin : 1;
    uint8_t rsv : 3;
    uint8_t opcode: 4;
    bool mask: 1;
    uint8_t payload_length_1: 7;
    uint64_t payload_length_2: 64;
};
typedef struct header_websocket_grand header_websocket_grand_t;

typedef union {
    header_websocket_petit_t header_websocket_petit;
    header_websocket_moyen_t header_websocket_moyen;
    header_websocket_grand_t header_websocket_grand;
} header_websocket_t;

struct message_thread {
    bool fin;
    uint64_t longueur;
    char message[BUFFER_LEN];
};
typedef struct message_thread message_thread_t;

#endif //HTTP_SERVEUR_WEBSOCKET_H
