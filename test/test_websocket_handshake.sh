#!/bin/bash

DOSSIER_TEST=$(dirname "$0")

echo -en "GET /chat HTTP/1.1\nHost: afeldmann.fr\nUpgrade: websocket\nConnection: Upgrade\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\nSec-WebSocket-Protocol: chat, superchat\nSec-WebSocket-Version: 13\n\n" | timeout 1s netcat localhost 55555 > "$DOSSIER_TEST"/new_test_websocket_handshake_snapshot

cmp --silent <(cat "$DOSSIER_TEST"/new_test_websocket_handshake_snapshot | head -n 5) <(cat "$DOSSIER_TEST"/test_websocket_handshake_snapshot | head -n 5) && \
    echo 'Test réussi !' || \
    echo 'Test échoué !'
