#define PORT 55555
#define BUFFER_LEN 10000
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <regex.h>
#include <ctype.h>
#include <sys/time.h>
#include <locale.h>
#include <time.h>

#define TRUE 1
typedef int bool;

static int sockfd_ecoute; /* Variable globale uniquement pour pouvoir la fermer en réponse à sigint/sigterm */
static regex_t regex_requete_http_get; /* Variable globale pour ne la compiler qu'une fois. */
static regex_t regex_decoupage_requetes; /* Variable de découpage des requêtes */

void stop_si(bool condition, const char* message_perror) {
    if (condition) {
        perror(message_perror);
        exit(EXIT_FAILURE);
    }
}

void handler_sigint_sigterm(__attribute__((unused)) int sig) {
    close(sockfd_ecoute);
    chdir("..");
    exit(EXIT_SUCCESS);
}

/* Fonctions de construction du header HTTP/1.1 200 OK */

void tampon_fixe(char* rep, int* rep_len) {
    char* header_fixed = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nServer: ArnaudHTTP\r\nConnection: close\r\n";
    strcpy(rep,header_fixed);
    *rep_len += (int) strlen(header_fixed);
}

void tampon_date(char* rep, int* rep_len) {
    time_t maintenant = time(0);
    struct tm maintenant_tm = *gmtime(&maintenant);
    *rep_len += (int)strftime(rep+(*rep_len), 40, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", &maintenant_tm);
}

void tampon_taille(char* rep, int* rep_len, FILE* fichier) {
    fseek(fichier, 0, SEEK_END);
    long taille = ftell(fichier);
    fseek(fichier, 0, SEEK_SET);
    *rep_len += sprintf(rep+(*rep_len), "Content-Length: %ld\r\n", taille);
}

void tampon_vide(char* rep, int* rep_len) {
    strcpy(rep+(*rep_len),"\r\n");
    *rep_len+=2;
}

int make_header(char* rep, FILE* fichier) {
    int rep_len = 0;
    tampon_fixe(rep, &rep_len);
    tampon_date(rep,&rep_len);
    tampon_taille(rep,&rep_len, fichier);
    tampon_vide(rep,&rep_len);
    return rep_len;
}

/* Construction de la réponse du serveur. req est supposé existant */

void construire_reponse(char* req, char* rep) {
    regmatch_t matches[4];
    if (regexec(&regex_requete_http_get, req, 4, matches, 0)) {
        strcpy(rep, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
        return;
    }
    if (req[matches[2].rm_so] != '1' || req[matches[3].rm_so] != '1') {
        strcpy(rep, "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n");
        return;
    }
    FILE *fichier;
    int nom_html_match = matches[1].rm_so;
    int nom_html_longueur = matches[1].rm_eo - nom_html_match;
    char nom_html[BUFFER_LEN];
    if (nom_html_longueur == 0) strcpy(nom_html,"index.html");
    else {
        strncpy(nom_html,req+nom_html_match,nom_html_longueur);
        for(int i = 0; nom_html[i]; i++) nom_html[i] = (char)tolower(nom_html[i]);
    }
    fichier = fopen(nom_html, "r");
    if (fichier == NULL) {
        strcpy(rep, "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found");
        return;
    }
    int rep_len = make_header(rep,fichier);
    for ( ; rep_len < BUFFER_LEN - 1 ; rep_len++) {
        int temp_char = fgetc(fichier);
        if (temp_char == EOF) break;
        rep[rep_len]=(char)temp_char;
    }
    rep[rep_len]='\0';
    fclose(fichier);
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
    stop_si(regcomp(&regex_requete_http_get, "^GET\\s.*\\/([a-z]+\\.html)?\\sHTTP\\/([0-9])\\.([0-9])", REG_EXTENDED | REG_ICASE),
            "regex_requete_http_get");
    stop_si(regcomp(&regex_decoupage_requetes, "\r?\n\r?(\n)", REG_EXTENDED),
            "regex_decoupage_requetes");
    signal(SIGINT, handler_sigint_sigterm);
    signal(SIGTERM, handler_sigint_sigterm);
    signal(SIGCHLD,SIG_IGN);
    chdir("static");
    setlocale(LC_ALL, "C"); // Pour que strftime soit en anglais
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
