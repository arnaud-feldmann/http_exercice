FROM alpine:3.19
RUN apk add gcc musl-dev openssl-dev
COPY serveur.c ./http_common.c ./http_websocket_handshake.c ./http_reponses.c ./serveur_websocket.c ./http_common.h ./http_websocket_handshake.h ./ http_reponses.h ./
COPY ./static ./static
RUN gcc ./serveur.c ./http_websocket_handshake.c ./http_reponses.c ./http_common.c -o serveur -lcrypto
RUN gcc ./serveur_websocket.c -o serveur_websocket

CMD ./serveur
