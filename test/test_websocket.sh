#!/bin/bash

DOSSIER_TEST=$(dirname "$0")

{ echo -en "GET /chat HTTP/1.1\nHost: afeldmann.fr\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\nSec-WebSocket-Protocol: chat, superchat\nSec-WebSocket-Version: 13\n\n" ; \
    sleep 1; echo -en "\x81\x96\x12\x59\x4E\x8A\x5A\x30\x6E\xFE\x7A\x30\x3D\xAA\x7B\x2A\x6E\xFD\x77\x3B\x6E\xE9\x7E\x30\x2B\xE4\x66\x77" ; \
    sleep 1; echo -en "\x82\x96\x12\x59\x4E\x8A\x5A\x30\x6E\xFE\x7A\x30\x3D\xAA\x7B\x2A\x6E\xFD\x77\x3B\x6E\xE9\x7E\x30\x2B\xE4\x66\x77" ; \
    sleep 1; echo -en "\x8A\x80\x1B\x4D\x46\xD2" ; \
    sleep 1; echo -en "\x88\x80\xB6\x11\x4C\xD5" ; \
    sleep 1 ; } | \
    timeout 5s netcat localhost 55555 > "$DOSSIER_TEST"/new_test_websocket_snapshot

cmp --silent <(cat "$DOSSIER_TEST"/new_test_websocket_snapshot) <(cat "$DOSSIER_TEST"/test_websocket_snapshot) && \
    echo 'Test réussi !' || \
    echo 'Test échoué !'
