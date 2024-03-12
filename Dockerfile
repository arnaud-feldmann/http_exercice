FROM alpine:3.19
RUN apk add gcc musl-dev openssl-dev
COPY ./serveur.c ./common.c ./websocket.c ./fichiers_html.c ./common.h ./websocket.h ./ fichiers_html.h ./
COPY ./static ./static
RUN gcc ./serveur.c ./websocket.c ./fichiers_html.c ./common.c -o serveur -lcrypto
CMD ./serveur
