//
// Created by arnaud on 10/03/24.
//

#ifndef HTTP_REPONSES_WEBSOCKET_H
#define HTTP_REPONSES_WEBSOCKET_H

void initialisations_reponses_websocket();
bool accept_websocket_si_demande(char *req, char *rep, int debut_header);
void construire_reponse_websocket(char* req, char* rep);

#endif //HTTP_REPONSES_WEBSOCKET_H
