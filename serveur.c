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

#define TRUE 1
#define FALSE 0
typedef int bool;

void reuseaddr(int sockfd) {
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EXIT_FAILURE);
    }
}

void bind_port(int sockfd,uint16_t port) {
    reuseaddr(sockfd);
    struct sockaddr_in adresse;
    adresse.sin_family=AF_INET;
    adresse.sin_port= htons(port);
    adresse.sin_addr.s_addr=INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &adresse, sizeof(adresse))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
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
    bind_port(sockfd, PORT);
    listen(sockfd, 15);
    struct sockaddr client_adr;
    socklen_t client_addrlen;
    regex_t regex;
    regmatch_t matches[4];
    if (regcomp(&regex, "^GET\\s.*\\/([a-z]+\\.html)?\\sHTTP\\/([0-9])\\.([0-9])", REG_EXTENDED|REG_ICASE) != 0) {
        perror("regex");
        exit(EXIT_FAILURE);
    }
    signal(SIGTERM,handler_sigterm);
    chdir("static");
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
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
                            if (fichier == NULL) strcpy(rep,"HTTP/1.1 404 Not Found\r\n");
                            else {
                                strcpy(rep,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nServer: ArnaudHTTP\r\n\r\n");
                                int header_len = (int)strlen(rep);
                                fgets(rep+header_len,BUFFER_LEN-header_len-1,fichier);
                                fclose(fichier);
                                continuer_session = FALSE;
                            }
                        }
                        else {
                            strcpy(rep,"HTTP/1.1 505 HTTP Version Not Supported\r\n");
                            rep[5]=vermaj;
                            rep[7]=vermin;
                        }
                    }
                    else strcpy(rep,"HTTP/1.1 501 Not Implemented\r\n");
                }
                send(session_sockfd, rep, strlen(rep), 0);
            }
            close(session_sockfd);
            return 0;
        } else close(session_sockfd);
    }
#pragma clang diagnostic pop
}
