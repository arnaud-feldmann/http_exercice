#include <regex.h>
#include <stddef.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <string.h>
#include "common.h"
#include "websocket.h"

const char* magic_websocket = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static regex_t regex_upgrade_websocket;
static regex_t regex_connection_upgrade;
static regex_t regex_websocket_key;
static char cle_ascii[53];
BIO* bio_base64;
BIO* bio_mem;

void initialisations_websocket() {
    stop_si(regcomp(&regex_upgrade_websocket, "(^|\r?\n)Upgrade: websocket($|\r?\n)", REG_EXTENDED),
            "regex_requete_upgrade_websocket");
    stop_si(regcomp(&regex_connection_upgrade, "(^|\r?\n)Connection: Upgrade($|\r?\n)", REG_EXTENDED),
            "regex_connection_upgrade");
    stop_si(regcomp(&regex_websocket_key, "(^|\r?\n)Sec-WebSocket-Key: ([A-Za-z0-9+/=]{16})($|\r?\n)", REG_EXTENDED),
            "regex_websocket_key");
    bio_base64 = BIO_new(BIO_f_base64());
    bio_mem = BIO_new(BIO_s_mem());
    BIO_push(bio_base64, bio_mem);
    BIO_set_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);
}

bool est_demande_websocket(char* debut_header) {
    regmatch_t match_key[4];
    bool res = (regexec(&regex_upgrade_websocket, debut_header, 0, NULL, 0) == 0) &&
               (regexec(&regex_connection_upgrade, debut_header, 0, NULL, 0) == 0) &&
               (regexec(&regex_websocket_key, debut_header, 4, match_key, 0) == 0);
    if (! res) return FALSE;
    strncpy(cle_ascii, debut_header + match_key[2].rm_eo,16);
    strcpy(cle_ascii + 16, magic_websocket);
    return TRUE;
}


void construire_reponse_websocket(char* req, char* rep, regmatch_t matches[4]) {
    unsigned char hash[21];
    char sec_websocket_accept[29];

    SHA1((unsigned char *)cle_ascii, 52, hash);
    BIO_write(bio_base64, hash, 20);
    BIO_flush(bio_base64);
    BIO_get_mem_data(bio_mem, sec_websocket_accept);
    sec_websocket_accept[28] = '\0';
}