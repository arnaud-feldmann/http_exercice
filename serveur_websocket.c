#include <stdlib.h>
#include <sys/socket.h>
#include <stdatomic.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include "serveur_websocket.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"

int sockfd_session;

pthreads_actifs_t pthreads_actifs = {0,0,0,0,0,0};
pipe_actifs_t pipe_actifs = {0,0,0,0,0,0,0,0,0,0};

atomic_bool fin_session = ATOMIC_VAR_INIT(false);

void recv_thread() {
    header_websocket_grand_t header_websocket_grand;
    header_websocket_t* header_websocket = (header_websocket_t*)&header_websocket_grand;
    message_thread_t message_thread;
    uint8_t opcode_prec = -1;
    uint8_t opcode;
    struct pollfd pollfd_session[1] = {sockfd_session, POLLIN, 0};
    while (! fin_session) {
        message_thread.fin = header_websocket->header_websocket_petit.fin;
        opcode = header_websocket->header_websocket_petit.opcode;
        if ((poll(pollfd_session, 1, 2000) <= 0)) continue;
        stop_si(recv(sockfd_session,header_websocket,2,0) < 2,"recv_header_1");
        if (header_websocket->header_websocket_petit.payload_length == 126) {
            stop_si(recv(sockfd_session,header_websocket + 2,2,0) < 2, "recv_header_2");
            message_thread.longueur = header_websocket->header_websocket_moyen.payload_length_2;
        } else if (header_websocket->header_websocket_petit.payload_length == 127) {
            stop_si(recv(sockfd_session,header_websocket + 2,8,0) < 8, "recv_header_3");
            message_thread.longueur = header_websocket->header_websocket_grand.payload_length_2;
        } else message_thread.longueur = header_websocket->header_websocket_petit.payload_length;
        if (message_thread.longueur > BUFFER_LEN) {
            fin_session = true;
            return;
        }
        stop_si(recv(sockfd_session,&(message_thread.message),message_thread.longueur,0) < message_thread.longueur, "recv_message");
        if (opcode == CONTINUATION) {
            if (opcode_prec != TEXTE && opcode_prec != BINAIRE) {
                fin_session = true;
                return;
            }
            opcode = opcode_prec;
        }
        switch (opcode) {
            case TEXTE:
                stop_si(write(pipe_actifs.recv_texte[1], &(message_thread.message), sizeof(message_thread_t)), "write_texte");
                opcode_prec = TEXTE;
                break;
            case BINAIRE:
                stop_si(write(pipe_actifs.recv_binaire[1], &(message_thread.message), sizeof(message_thread_t)), "write_binaire");
                opcode_prec = BINAIRE;
                break;
            case FERMETURE:
                stop_si(write(pipe_actifs.recv_fermeture[1], &(message_thread.message), sizeof(message_thread_t)), "write_fermeture");
                break;
            case PING:
                stop_si(write(pipe_actifs.recv_ping[1], &(message_thread.message), sizeof(message_thread_t)), "write_ping");
                break;
            case PONG:
                stop_si(write(pipe_actifs.recv_pong[1], &(message_thread.message), sizeof(message_thread_t)), "write_ping");
                break;
            default:
                fin_session = true;
                return;
        }
    }
}

void envoyer_message(void* message, uint_fast64_t longueur,opcode_t opcode) {
    uint_fast64_t longueur_complete;
    char rep[BUFFER_LEN];
    header_websocket_t* rep_header = (header_websocket_t*)rep;
    rep_header->header_websocket_petit.fin = true;
    rep_header->header_websocket_petit.rsv = 0;
    rep_header->header_websocket_petit.opcode = opcode;
    rep_header->header_websocket_petit.mask = false;
    if (longueur < 126) {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_petit_t));
        rep_header->header_websocket_petit.payload_length = longueur;
        memcpy(rep + sizeof(header_websocket_petit_t), message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_petit_t);
    } else if (longueur < 65536) {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_moyen_t));
        rep_header->header_websocket_moyen.payload_length_1 = 126;
        rep_header->header_websocket_moyen.payload_length_2 = longueur;
        memcpy(rep + sizeof(header_websocket_moyen_t), message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_moyen_t);
    } else {
        longueur = min(longueur,BUFFER_LEN - sizeof(header_websocket_grand_t));
        rep_header->header_websocket_grand.payload_length_1 = 127;
        rep_header->header_websocket_grand.payload_length_2 = longueur;
        memcpy(rep + sizeof(header_websocket_grand_t), message, longueur);
        longueur_complete = longueur + sizeof(header_websocket_grand_t);
    }
    send(sockfd_session, &rep, longueur_complete, 0);
}

void texte_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_texte[1] = {pipe_actifs.recv_texte[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_texte, 1, 2000)) continue;
        read(pipe_actifs.recv_texte[0], &message_thread, sizeof(message_thread_t));
        envoyer_message(&(message_thread.message),1,TEXTE);
    }
}

void binaire_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_binaire[1] = {pipe_actifs.recv_binaire[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_binaire, 1, 2000)) continue;
        read(pipe_actifs.recv_binaire[0], &message_thread, sizeof(message_thread_t));
    }
}

void fermeture_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_fermeture[1] = {pipe_actifs.recv_fermeture[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_fermeture, 1, 2000)) continue;
        read(pipe_actifs.recv_fermeture[0], &message_thread, sizeof(message_thread_t));
        fin_session = true;
    }
}

void ping_thread() {
    message_thread_t message_thread;
    struct pollfd pollfd_recv_texte[1] = {pipe_actifs.recv_texte[0], POLLIN, 0};
    while (! fin_session) {
        if (poll(pollfd_recv_texte, 1, 2000)) continue;
        read(pipe_actifs.recv_texte[0], &message_thread, sizeof(message_thread_t));
        envoyer_message(&(message_thread.message),message_thread.longueur,PONG);
    }
}

void pong_thread() {
    fcntl(pipe_actifs.recv_pong[0], F_SETFL, O_NONBLOCK);

    struct pollfd pollfd_recv_pong[1] = {pipe_actifs.recv_pong[0], POLLIN, 0};
    message_thread_t message_thread;

    for(; ! fin_session; sleep(15)) {
        envoyer_message("",0,PING);
        if ((poll(pollfd_recv_pong, 1, 2000) <= 0) ||
            (read(pipe_actifs.recv_pong[0], &message_thread, sizeof(message_thread_t)) <= 0) ||
            message_thread.fin != 0 ||
            message_thread.longueur != 0 ||
            (poll(pollfd_recv_pong, 1, 0) != 0)) {
            /* RFC dit qu'on doit renvoyer exactement la même charge, je n'envoie que des rep_ping de */
            /* Charge utile nulle donc on ne devrait avoir que des charges utiles nulles. Le dernier poll sert à
             * vérifier que le buffer est vide après retrait et ne pas permettre ceux qui spamment de "pong" */
            fin_session = true;
            return;
        }
    }
}

int main(__attribute__((unused)) int argc, char * argv[]) {
    sockfd_session = atoi(argv[1]);
    stop_si(pipe(pipe_actifs.recv_texte), "pipe_texte");
    stop_si(pipe(pipe_actifs.recv_binaire), "pipe_binaire");
    stop_si(pipe(pipe_actifs.recv_fermeture), "pipe_fermeture");
    stop_si(pipe(pipe_actifs.recv_ping), "pipe_ping");
    stop_si(pipe(pipe_actifs.recv_pong), "pipe_pong");
    stop_si(pthread_create(&(pthreads_actifs.recv), NULL, (void *(*)(void *)) &recv_thread, NULL),
            "recv_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.recv_texte), NULL, (void *(*)(void *)) &texte_thread, NULL),
            "texte_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.recv_binaire), NULL, (void *(*)(void *)) &binaire_thread, NULL),
            "binaire_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.recv_fermeture), NULL, (void *(*)(void *)) &fermeture_thread, NULL),
            "fermeture_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.recv_ping), NULL, (void *(*)(void *)) &ping_thread, NULL),
            "ping_pthread_create");
    stop_si(pthread_create(&(pthreads_actifs.recv_pong), NULL, (void *(*)(void *)) &pong_thread, NULL),
            "pong_pthread_create");
    pthread_join(pthreads_actifs.recv,NULL);
    pthread_join(pthreads_actifs.recv_texte, NULL);
    pthread_join(pthreads_actifs.recv_binaire, NULL);
    pthread_join(pthreads_actifs.recv_fermeture, NULL);
    pthread_join(pthreads_actifs.recv_ping, NULL);
    pthread_join(pthreads_actifs.recv_pong, NULL);
    close(pipe_actifs.recv_pong[1]);
    close(pipe_actifs.recv_texte[1]);
    close(pipe_actifs.recv_binaire[1]);
    close(pipe_actifs.recv_fermeture[1]);
    close(pipe_actifs.recv_ping[1]);
    close(pipe_actifs.recv_pong[1]);
    close(sockfd_session);
}

#pragma clang diagnostic pop
