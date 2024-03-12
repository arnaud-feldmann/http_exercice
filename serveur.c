#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <regex.h>
#include <sys/time.h>
#include <locale.h>
#include "common.h"
#include "reponses_websocket.h"
#include "reponses_http.h"

static int sockfd_ecoute; /* Variable globale uniquement pour pouvoir la fermer en réponse à sigint/sigterm */
static regex_t regex_decoupage_requetes; /* Variable de découpage des requêtes */
enum mode_session mode_session_courante;

void handler_sigint_sigterm(__attribute__((unused)) int sig) {
    close(sockfd_ecoute);
    chdir("..");
    exit(EXIT_SUCCESS);
}

/* Construction de la réponse du serveur. req est supposé existant */

void construire_reponse(char* req, char* rep) {
    switch (mode_session_courante) {
        case HTTP:
            construire_reponse_http(req, rep, &mode_session_courante);
            break;
        case WEBSOCKET:
            construire_reponse_websocket(req, rep);
            break;
    }
}

/* Lit la requete qui est pointée par req, de longueur longueur_requete, puis copie le reste du buffer de requête
 * à son début. */

void consommer_requete(int sockfd_session, char* req, char* rep, long longueur_requete, long* longueur_totale) {
    char temp = req[longueur_requete];
    req[longueur_requete] = '\0';
    construire_reponse(req, rep);
    send(sockfd_session, rep, strlen(rep) + 1, 0);
    req[longueur_requete] = temp;
    strcpy(req, req + longueur_requete); /* Tout ce qui vient après le /n/n */
    *longueur_totale -= longueur_requete;
}

/* Gestion d'un socket de session ouvert */

/* Note : On scanne d'abord que le segment qui vient d'être envoyé, car on veut éviter de tout rescanner ce qu'on nous
 * a envoyé si quelqu'un s'amuse à envoyer des caractères un à un. Puis après on scanne le reste du message.
 * Puisque \r?\n\r?(\n) fait 4 caractères, on est obligés toutefois de scanner 3 caractères dans en arrière pour
 * être sûr de ne rien manquer dans la première étape. */

void repondre_sur(int sockfd_session) {
    char req_moins_trois[BUFFER_LEN + 3] = "   ";
    char* req = req_moins_trois + 3;
    char rep[BUFFER_LEN];
    long longueur_segment;
    long longueur_totale = 0;
    regmatch_t matches[2];
    while ((longueur_segment = recv(sockfd_session, req + longueur_totale, BUFFER_LEN - longueur_totale - 1, 0)) > 0) {
        char * segment_moins_trois = req_moins_trois + longueur_totale;
        longueur_totale += longueur_segment;
        req[longueur_totale] = '\0';
        if (regexec(&regex_decoupage_requetes, segment_moins_trois, 2, matches, 0) == 0) {
            consommer_requete(sockfd_session, req, rep, matches[1].rm_eo + segment_moins_trois - req, &longueur_totale);
            while (regexec(&regex_decoupage_requetes, req, 2, matches, 0) == 0) {
                consommer_requete(sockfd_session, req, rep, matches[1].rm_eo, &longueur_totale);
            }
        }
    }
}

/* Fonctions relatives à la mnipulation de socket */

void reuseaddr(int sockfd) {
    const int enable = 1;
    stop_si(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0,
            "setsockopt(SO_REUSEADDR)");
}

void bind_port(int sockfd,uint16_t port) {
    reuseaddr(sockfd);
    struct sockaddr_in adresse;
    memset(&adresse, 0, sizeof(struct sockaddr_in));
    adresse.sin_family=AF_INET;
    adresse.sin_port=htons(port);
    adresse.sin_addr.s_addr=INADDR_ANY;
    stop_si(bind(sockfd, (struct sockaddr *) &adresse, sizeof(struct sockaddr_in)) < 0, "bind");
}

void socket_timeout(int sockfd) {
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

int main() {
    sockfd_ecoute = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    stop_si(sockfd_ecoute < 0, "socket");
    socket_timeout(sockfd_ecoute);
    bind_port(sockfd_ecoute, PORT);
    stop_si(listen(sockfd_ecoute, 15) < 0,"listen");
    stop_si(regcomp(&regex_decoupage_requetes, "\r?\n\r?(\n)", REG_EXTENDED),
            "regex_decoupage_requetes");
    initialisations_reponses_http();
    initialisations_reponses_websocket();
    signal(SIGINT, handler_sigint_sigterm);
    signal(SIGTERM, handler_sigint_sigterm);
    signal(SIGCHLD,SIG_IGN);
    chdir("static");
    setlocale(LC_ALL, "C"); // Pour que strftime soit en anglais
    mode_session_courante = HTTP;
    while (TRUE) {
        int sockfd_session = accept(sockfd_ecoute, NULL, NULL);
        if (sockfd_session < 0) sleep(1);
        else if (fork() == 0) {
            close(sockfd_ecoute);
            repondre_sur(sockfd_session);
            close(sockfd_session);
            exit(EXIT_SUCCESS);
        } else close(sockfd_session);
    }
}
