#ifndef HTTP_SERVEUR_WEBSOCKET_H
#define HTTP_SERVEUR_WEBSOCKET_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

struct pthreads_actifs {
    pthread_t recv;
    pthread_t read_texte;
    pthread_t read_binaire;
    pthread_t read_fermeture;
    pthread_t read_ping;
    pthread_t read_pong;
    pthread_t send_ping;
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

#define FIN 0b10000000
#define OPCODE 0b00001111
#define PAYLOAD_LENGTH_1 0b01111111

struct header_websocket {
    uint8_t fin_rsv_opcode;
    uint8_t mask_payload_length_1;
};
typedef struct header_websocket header_websocket_t;

struct message_thread {
    bool fin;
    uint64_t longueur;
    uint8_t message[BUFFER_LEN];
};
typedef struct message_thread message_thread_t;

#endif //HTTP_SERVEUR_WEBSOCKET_H
