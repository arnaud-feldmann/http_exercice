#ifndef HTTP_REPONSES_HTTP_H
#define HTTP_REPONSES_HTTP_H

#include "common.h"

void initialisations_reponses_http();
void construire_reponse_http(char* req, char* rep, enum mode_session*);

#endif //HTTP_REPONSES_HTTP_H
