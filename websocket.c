#include <regex.h>
#include <stddef.h>
#include "common.h"
#include "websocket.h"

static regex_t regex_upgrade_websocket;
static regex_t regex_connection_upgrade;
static regex_t regex_websocket_key;

void compiler_regex_websocket() {
    stop_si(regcomp(&regex_upgrade_websocket, "(^|\r?\n)Upgrade: websocket($|\r?\n)", REG_EXTENDED),
            "regex_requete_upgrade_websocket");
    stop_si(regcomp(&regex_connection_upgrade, "(^|\r?\n)Connection: Upgrade($|\r?\n)", REG_EXTENDED),
            "regex_connection_upgrade");
    stop_si(regcomp(&regex_websocket_key, "(^|\r?\n)Sec-WebSocket-Key: [A-Za-z0-9+/=]{16}($|\r?\n)", REG_EXTENDED),
            "regex_websocket_key");
}

bool est_header_websocket(char* string) {
    return (regexec(&regex_upgrade_websocket, string, 0, NULL, 0) == 0) &&
           (regexec(&regex_connection_upgrade, string, 0, NULL, 0) == 0);
}

void construire_reponse_websocket(char* req, char* rep, regmatch_t matches[4]) {
    
}