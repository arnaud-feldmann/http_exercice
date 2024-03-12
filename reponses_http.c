#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#include "common.h"
#include "reponses_http.h"
#include "reponses_websocket.h"

static regex_t regex_requete_http_get; /* Variable globale pour ne la compiler qu'une fois. */

void initialisations_reponses_http() {
    stop_si(regcomp(&regex_requete_http_get, "^GET\\s.*\\/([a-z_]+\\.html)?\\sHTTP\\/([0-9])\\.([0-9])", REG_EXTENDED | REG_ICASE),
            "regex_requete_http_get");
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

void construire_reponse_http(char* req, char* rep, enum mode_session* mode_session_courante) {
    regmatch_t matches[4];
    if (regexec(&regex_requete_http_get, req, 4, matches, 0)) {
        strcpy(rep, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\n\r\n");
        return;
    }
    if (req[matches[2].rm_so] != '1' || req[matches[3].rm_so] != '1') {
        strcpy(rep, "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 0\r\n\r\n");
        return;
    }
    if (accept_websocket_si_demande(req, rep, matches[3].rm_so + 1)) {
        *mode_session_courante = WEBSOCKET;
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