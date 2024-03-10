//
// Created by arnaud on 10/03/24.
//

#ifndef HTTP_WEBSOCKET_H
#define HTTP_WEBSOCKET_H

void compiler_regex_websocket();
bool est_demande_websocket(char* debut_header);
void construire_reponse_websocket(char* req, char* rep, regmatch_t matches[4]);

#endif //HTTP_WEBSOCKET_H
