#ifndef HTTP_HTTP_WEBSOCKET_HANDSHAKE_H
#define HTTP_HTTP_WEBSOCKET_HANDSHAKE_H

void initialisations_reponses_websocket();
bool websocket_handshake_si_demande(char *req, char *rep, int fin_ligne_get);

#endif //HTTP_HTTP_WEBSOCKET_HANDSHAKE_H
