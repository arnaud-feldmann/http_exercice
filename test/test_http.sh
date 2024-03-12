#!/bin/bash

DOSSIER_TEST=$(dirname "$0")

{ echo -en "GET / HTTP/1.1\n\nGET / HTTP/1.1\n\na\r\n\r\n" ; \
    sleep 1; echo -en "GET / HTTP/1.0\n\r" ; sleep 1 ; \
    echo -en "\n" ; sleep 1 ; echo -en "GET / " ; \
    sleep 1 ; echo -en "HTTP/1.1\n\nGET /inexistant.html HTTP/1.1\n\n" ; } | \
    timeout 5s netcat localhost 55555 > "$DOSSIER_TEST"/new_test_http_snapshot

cmp --silent <(grep -av "^Date:" "$DOSSIER_TEST"/new_test_http_snapshot) <(grep -av "^Date:" "$DOSSIER_TEST"/test_http_snapshot) && \
    echo 'Test réussi !' || \
    echo 'Test échoué !'
