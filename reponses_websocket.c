#include <regex.h>
#include <stddef.h>
#include <openssl/sha.h>
#include <openssl/pem.h>
#include <string.h>
#include "common.h"
#include "reponses_websocket.h"

const char* magic_websocket = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char* header_accept = "HTTP/1.1 101 Switching Protocols\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Accept: ";

static regex_t regex_upgrade_websocket;
static regex_t regex_connection_upgrade;
static regex_t regex_websocket_key;

void initialisations_reponses_websocket() {
    stop_si(regcomp(&regex_upgrade_websocket, "(^|\r?\n)Upgrade: websocket($|\r?\n)", REG_EXTENDED),
            "regex_requete_upgrade_websocket");
    stop_si(regcomp(&regex_connection_upgrade, "(^|\r?\n)Connection: Upgrade($|\r?\n)", REG_EXTENDED),
            "regex_connection_upgrade");
    stop_si(regcomp(&regex_websocket_key, "(^|\r?\n)Sec-WebSocket-Key: ([A-Za-z0-9+/=]{24})($|\r?\n)", REG_EXTENDED),
            "regex_websocket_key");
}

bool accept_websocket_si_demande(char* req, char* rep, int debut_header) {
    regmatch_t match_key[4];
    char cle_ascii[61];
    unsigned char hash[21];
    char* header = req + debut_header;
    bool res = (regexec(&regex_upgrade_websocket, header, 0, NULL, 0) == 0) &&
               (regexec(&regex_connection_upgrade, header, 0, NULL, 0) == 0) &&
               (regexec(&regex_websocket_key, header, 4, match_key, 0) == 0);
    if (! res) return FALSE;
    strncpy(cle_ascii, header + match_key[2].rm_so, 24);
    strcpy(cle_ascii + 24, magic_websocket);
    strcpy(rep,header_accept);
    SHA1((unsigned char *)cle_ascii, 60, hash);
    EVP_EncodeBlock((unsigned char*)rep + strlen(header_accept), hash, 20);
    rep[strlen(header_accept) + 28] = '\0';
    return TRUE;
}

void construire_reponse_websocket(char* req, char* rep) {

}