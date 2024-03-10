#include <regex.h>
#include <stddef.h>
#include <openssl/sha.h>
#include <b64/cencode.h>
#include <string.h>
#include "common.h"
#include "websocket.h"

const char* magic_websocket = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static regex_t regex_upgrade_websocket;
static regex_t regex_connection_upgrade;
static regex_t regex_websocket_key;
static char cle_ascii[53];

void compiler_regex_websocket() {
    stop_si(regcomp(&regex_upgrade_websocket, "(^|\r?\n)Upgrade: websocket($|\r?\n)", REG_EXTENDED),
            "regex_requete_upgrade_websocket");
    stop_si(regcomp(&regex_connection_upgrade, "(^|\r?\n)Connection: Upgrade($|\r?\n)", REG_EXTENDED),
            "regex_connection_upgrade");
    stop_si(regcomp(&regex_websocket_key, "(^|\r?\n)Sec-WebSocket-Key: ([A-Za-z0-9+/=]{16})($|\r?\n)", REG_EXTENDED),
            "regex_websocket_key");
}

bool est_demande_websocket(char* debut_header) {
    regmatch_t match_key[4];
    bool res = (regexec(&regex_upgrade_websocket, debut_header, 0, NULL, 0) == 0) &&
               (regexec(&regex_connection_upgrade, debut_header, 0, NULL, 0) == 0) &&
               (regexec(&regex_websocket_key, debut_header, 4, match_key, 0) == 0);
    strncpy(cle_ascii, debut_header + match_key[2].rm_eo,16);
    strcpy(cle_ascii + 16, magic_websocket);
    return res;
}


void construire_reponse_websocket(char* req, char* rep, regmatch_t matches[4]) {
    unsigned char hash[21];
    char sec_websocket_accept[29];
    int taille_avant_remplissage;

    SHA1((unsigned char *)cle_ascii, 52, hash);
    taille_avant_remplissage = base64_encode_block((const char *)hash, 20, sec_websocket_accept, NULL);
    base64_encode_blockend(sec_websocket_accept+taille_avant_remplissage, NULL);
}