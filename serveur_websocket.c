#include <stdlib.h>
#include <sys/socket.h>
#include <stdatomic.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <endian.h>
#include <stdio.h>
#include "serveur_websocket.h"
#include "common.h"

#define retourne_et_fin_session_si(condition, message)\
    if (condition) {                   \
            printf(message);           \
            printf("\n");              \
            fin_session = true;        \
            return;                    \
            }                          \
    else ((void)0)

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"

int sockfd_session;

pthreads_actifs_t pthreads_actifs = {0,0,0,0,0,0};
pipe_actifs_t pipe_actifs = {0,0,0,0,0,0,0,0,0,0};

atomic_bool fin_session = ATOMIC_VAR_INIT(false);
atomic_bool pong_ok = ATOMIC_VAR_INIT(true);

pthread_cond_t condition_timer_ping = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_timer_ping = PTHREAD_MUTEX_INITIALIZER;

void afficher_bits(const void* adresse, size_t taille) {
    const uint8_t* octets = (const uint8_t*)adresse;
    for (size_t i = 0; i < taille; i++) {
        for (int j = 7; j >= 0; j--) {
            int bit = (octets[i] >> j) & 1;
            printf("%d", bit);
        }
        printf(" ");
    }
    printf("\n");
}

int recevoir_exactement(void* buf, long n) {
    long total_octets_recus = 0;
    long octets_recus;

    while (total_octets_recus < n) {
        octets_recus = recv(sockfd_session, buf + total_octets_recus, n - total_octets_recus, 0);
        afficher_bits(buf + total_octets_recus,octets_recus);
        if (octets_recus <= 0) return -1;
        total_octets_recus += octets_recus;
    }

    return 0;
}

void recv_thread() {
    header_websocket_t header_websocket;
    message_thread_t message_thread;
    uint_fast8_t opcode;
    uint_fast8_t opcode_prec = -1;
    uint_fast8_t payload_length_1;
    bool mask;
    char masking_key[4];
    struct pollfd pollfd_session[1] = {sockfd_session, POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_session, 1, 2000) <= 0) continue;
        retourne_et_fin_session_si(recevoir_exactement(&header_websocket,2), "recv payload_length_1");
        message_thread.fin = header_websocket.fin_rsv_opcode >> 7;
        opcode = header_websocket.fin_rsv_opcode & OPCODE;
        mask = header_websocket.mask_payload_length_1 >> 7;
        payload_length_1 = header_websocket.mask_payload_length_1 & PAYLOAD_LENGTH_1;
        printf("OPCODE : %d\n", opcode);
        printf("payload_length_1 : %d\n", payload_length_1);
        if (payload_length_1 == 126) {
            uint16_t payload_length_2;
            retourne_et_fin_session_si(recevoir_exactement(&payload_length_2,2),"recv payload_length_2 moyen");
            message_thread.longueur = be16toh(payload_length_2);
        } else if (payload_length_1 == 127) {
            uint64_t payload_length_2;
            retourne_et_fin_session_si(recevoir_exactement(&payload_length_2, 8), "recv payload_length_2 grand");
            message_thread.longueur = be64toh(payload_length_2);
        } else message_thread.longueur = payload_length_1;
        if (mask) {
            retourne_et_fin_session_si(recevoir_exactement(&masking_key, 4),"recv cle mask");
        }
        retourne_et_fin_session_si(message_thread.longueur > BUFFER_LEN,"recv message trop grand");
        retourne_et_fin_session_si(recevoir_exactement(message_thread.message,(long)message_thread.longueur),"recv message");
        if (mask) {
            for (int i = 0 ; i < message_thread.longueur ; i++) message_thread.message[i]=(message_thread.message[i])^masking_key[i%4];
        }
        if (opcode == CONTINUATION) {
            retourne_et_fin_session_si(opcode_prec != TEXTE && opcode_prec != BINAIRE, "CONTINUATION invalide");
            opcode = opcode_prec;
        }
        switch (opcode) {
            case TEXTE:
                retourne_et_fin_session_si(write(pipe_actifs.recv_texte[1], &message_thread, sizeof(message_thread_t)) == 0, "write_texte");
                opcode_prec = TEXTE;
                break;
            case BINAIRE:
                retourne_et_fin_session_si(write(pipe_actifs.recv_binaire[1], &message_thread, sizeof(message_thread_t)) == 0, "write_binaire");
                opcode_prec = BINAIRE;
                break;
            case FERMETURE:
                retourne_et_fin_session_si(write(pipe_actifs.recv_fermeture[1], &message_thread, sizeof(message_thread_t)) == 0, "write_fermeture");
                break;
            case PING:
                retourne_et_fin_session_si(write(pipe_actifs.recv_ping[1], &message_thread, sizeof(message_thread_t)) == 0, "write_ping");
                break;
            case PONG:
                retourne_et_fin_session_si(write(pipe_actifs.recv_pong[1], &message_thread, sizeof(message_thread_t)) == 0, "write_ping");
                break;
            default:
                retourne_et_fin_session_si(true, "opcode invalide");
        }
    }
}

void envoyer_message(void* message, uint64_t longueur,opcode_t opcode) {
    uint64_t longueur_complete;
    char rep[BUFFER_LEN];
    header_websocket_t* rep_header = (header_websocket_t*)rep;
    rep_header->fin_rsv_opcode = FIN | opcode;
    if (longueur < 126) {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_t));
        rep_header->mask_payload_length_1 = longueur;
        memcpy(rep + sizeof(header_websocket_t), message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_t);
    } else if (longueur < 65536) {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_t) - 2);
        rep_header->mask_payload_length_1 = 126;
        *((uint16_t*)(rep + sizeof(header_websocket_t))) = htobe16((uint16_t)longueur);
        memcpy(rep + sizeof(header_websocket_t) + 2, message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_t) + 2;
    } else {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_t) - 8);
        rep_header->mask_payload_length_1 = 127;
        *((uint64_t*)(rep + sizeof(header_websocket_t))) = htobe16((uint64_t)longueur);
        memcpy(rep + sizeof(header_websocket_t) + 8, message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_t) + 8;
    }
    send(sockfd_session, rep, longueur_complete, 0);
}

void read_texte_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_texte[1] = {pipe_actifs.recv_texte[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_texte, 1, 2000) <= 0) continue;
        read(pipe_actifs.recv_texte[0], &message_thread, sizeof(message_thread_t));
        fwrite(message_thread.message, sizeof(char), message_thread.longueur, stdout);
        printf("\n");
        envoyer_message(message_thread.message,message_thread.longueur,TEXTE);
    }
}

void read_binaire_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_binaire[1] = {pipe_actifs.recv_binaire[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_binaire, 1, 2000) <= 0) continue;
        read(pipe_actifs.recv_binaire[0], &message_thread, sizeof(message_thread_t));
    }
}

void read_fermeture_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_fermeture[1] = {pipe_actifs.recv_fermeture[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_fermeture, 1, 2000) <= 0) continue;
        read(pipe_actifs.recv_fermeture[0], &message_thread, sizeof(message_thread_t));
        envoyer_message(message_thread.message,message_thread.longueur,FERMETURE);
        retourne_et_fin_session_si(true, "message fermeture");
    }
}

void read_ping_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_texte[1] = {pipe_actifs.recv_ping[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_texte, 1, 2000) <= 0) continue;
        read(pipe_actifs.recv_texte[0], &message_thread, sizeof(message_thread_t));
        retourne_et_fin_session_si(message_thread.longueur > 125, "longeur ping > 125");
        envoyer_message(message_thread.message,message_thread.longueur,PONG);
    }
}

void read_pong_thread() {
    struct pollfd pollfd_recv_pong[1] = {pipe_actifs.recv_pong[0], POLLIN, 0};
    message_thread_t message_thread;

    while (! fin_session) {
        if (poll(pollfd_recv_pong, 1, 2000) <= 0) continue;
        retourne_et_fin_session_si(read(pipe_actifs.recv_pong[0], &message_thread, sizeof(message_thread_t)) <= 0 ||
                                   message_thread.fin != 1 ||
                                   message_thread.longueur != 0, "pong invalide");
        pong_ok = true;
    }
}

void send_ping_thread() {
    sleep(2);
    pthread_mutex_lock(&mutex_timer_ping);
    while (! fin_session) {
        retourne_et_fin_session_si(! pong_ok, "pong timeout");
        envoyer_message("",0,PING);
        pong_ok = false;
        struct timespec timer;
        clock_gettime(CLOCK_REALTIME, &timer);
        timer.tv_sec += 30;
        pthread_cond_timedwait(&condition_timer_ping,&mutex_timer_ping,&timer);
    }
    pthread_mutex_unlock(&mutex_timer_ping);
}

int main(__attribute__((unused)) int argc, char * argv[]) {
    sockfd_session = atoi(argv[1]);
    socket_timeout(sockfd_session,60);
    stop_si(pipe(pipe_actifs.recv_texte), "pipe_texte");
    stop_si(pipe(pipe_actifs.recv_binaire), "pipe_binaire");
    stop_si(pipe(pipe_actifs.recv_fermeture), "pipe_fermeture");
    stop_si(pipe(pipe_actifs.recv_ping), "pipe_ping");
    stop_si(pipe(pipe_actifs.recv_pong), "pipe_pong");
    stop_si(pthread_create(&(pthreads_actifs.recv), NULL, (void *(*)(void *)) &recv_thread, NULL),
            "recv_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.read_texte), NULL, (void *(*)(void *)) &read_texte_thread, NULL),
            "texte_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.read_binaire), NULL, (void *(*)(void *)) &read_binaire_thread, NULL),
            "binaire_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.read_fermeture), NULL, (void *(*)(void *)) &read_fermeture_thread, NULL),
            "fermeture_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.read_ping), NULL, (void *(*)(void *)) &read_ping_thread, NULL),
            "ping_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.read_pong), NULL, (void *(*)(void *)) &read_pong_thread, NULL),
            "pong_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.send_ping), NULL, (void *(*)(void *)) &send_ping_thread, NULL),
            "pong_pthread_create");
    pthread_join(pthreads_actifs.recv,NULL);
    pthread_cond_signal(&condition_timer_ping);
    pthread_join(pthreads_actifs.read_texte, NULL);
    pthread_join(pthreads_actifs.read_binaire, NULL);
    pthread_join(pthreads_actifs.read_fermeture, NULL);
    pthread_join(pthreads_actifs.read_ping, NULL);
    pthread_join(pthreads_actifs.read_pong, NULL);
    pthread_join(pthreads_actifs.send_ping, NULL);
    close(pipe_actifs.recv_texte[0]);
    close(pipe_actifs.recv_binaire[0]);
    close(pipe_actifs.recv_fermeture[0]);
    close(pipe_actifs.recv_ping[0]);
    close(pipe_actifs.recv_pong[0]);
    close(pipe_actifs.recv_texte[1]);
    close(pipe_actifs.recv_binaire[1]);
    close(pipe_actifs.recv_fermeture[1]);
    close(pipe_actifs.recv_ping[1]);
    close(pipe_actifs.recv_pong[1]);
    close(sockfd_session);
    printf("session websocket terminée");
}

#pragma clang diagnostic pop
