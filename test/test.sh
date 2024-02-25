#!/bin/bash

{ echo -en "GET / HTTP/1.1\n\nGET / HTTP/1.1\n\na\r\n\r\n" ; sleep 1; echo -en "GET / HTTP/1.0\n\r" ; sleep 1 ; echo -en "\n" ; sleep 1 ; echo -en "GET / " ; sleep 1 ; echo -e "HTTP/1.1\n\nGET /inexistant.html HTTP/1.1\n\n" ; } | timeout 5s netcat localhost 55555 > new_snapshot
cmp --silent <(grep -av "^Date:" ./new_snapshot) <(grep -av "^Date:" ./snapshot) && \
    echo 'Test réussi !' || \
    echo 'Test échoué !'
