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
#define FALSE 0
typedef int bool;

/* Les tampons servent à construire le header. Ils prenent le pointeur vers le rep de réponse et une longueur,
 * écrivent leur truc et incrémentent la longueur du header */

void tampon_fixed(char* rep, int* rep_len) {
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
    tampon_fixed(rep,&rep_len);
    tampon_date(rep,&rep_len);
    tampon_taille(rep,&rep_len, fichier);
    tampon_vide(rep,&rep_len);
    return rep_len;
}

void erreur_si(bool condition, const char* message_perror) {
    if (condition) {
        perror(message_perror);
        exit(EXIT_FAILURE);
    }
}

void reuseaddr(int sockfd) {
    const int enable = 1;
    erreur_si(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0,
              "setsockopt(SO_REUSEADDR)");
}

void bind_port(int sockfd,uint16_t port) {
    reuseaddr(sockfd);
    struct sockaddr_in adresse;
    adresse.sin_family=AF_INET;
    adresse.sin_port= htons(port);
    adresse.sin_addr.s_addr=INADDR_ANY;
    erreur_si(bind(sockfd, (struct sockaddr *) &adresse, sizeof(adresse)), "bind");
}

void socket_timeout(int sockfd) {
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}

int sockfd;

void handler_sigterm(__attribute__((unused)) int sig) {
    close(sockfd);
    chdir("..");
    exit(0);
}

int main() {
    char req[BUFFER_LEN];
    char rep[BUFFER_LEN];
    bool continuer_session = TRUE;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    erreur_si(socket < 0, "socket");
    socket_timeout(sockfd);
    bind_port(sockfd, PORT);
    listen(sockfd, 15);
    struct sockaddr client_adr;
    socklen_t client_addrlen;
    regex_t regex;
    regmatch_t matches[4];
    erreur_si(regcomp(&regex, "^GET\\s.*\\/([a-z]+\\.html)?\\sHTTP\\/([0-9])\\.([0-9])", REG_EXTENDED|REG_ICASE),
              "regex");
    signal(SIGINT,handler_sigterm);
    signal(SIGTERM,handler_sigterm);
    chdir("static");
    setlocale(LC_ALL, "C"); // Pour que strftime soit en anglais
    while (TRUE) {
        int session_sockfd = accept(sockfd, &client_adr, &client_addrlen);
        if (fork() == 0) {
            close(sockfd);
            while (continuer_session) {
                memset(req,0,BUFFER_LEN);
                long bytes = recv(session_sockfd,&req,BUFFER_LEN,0);
                if (bytes > 0) {
                    if (regexec(&regex, req, 4, matches, 0) == 0) {
                        char vermaj = req[matches[2].rm_so];
                        char vermin = req[matches[3].rm_so];
                        if (vermaj == '1' && vermin == '1') {
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
                            if (fichier == NULL) strcpy(rep,"HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found");
                            else {
                                int rep_len = make_header(rep,fichier);
                                for ( ; rep_len < BUFFER_LEN - 1 ; rep_len++) {
                                    int temp_char = fgetc(fichier);
                                    if (temp_char == EOF) break;
                                    rep[rep_len]=(char)temp_char;
                                }
                                rep[rep_len]='\0';
                                fclose(fichier);
                            }
                        }
                        else {
                            strcpy(rep,"HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n");
                            rep[5]=vermaj;
                            rep[7]=vermin;
                        }
                    }
                    else strcpy(rep,"HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
                    send(session_sockfd, rep, strlen(rep) + 1, 0);
                }
                else continuer_session = FALSE;
            }
            close(session_sockfd);
            return 0;
        } else close(session_sockfd);
    }
}
