FROM alpine:3.19
RUN apk add gcc musl-dev
COPY ./serveur.c ./common.c ./websocket.c ./fichiers_html.c ./common.h ./websocket.h ./ fichiers_html.h ./
COPY ./static ./static
CMD gcc ./serveur.c ./websocket.c ./fichiers_html.c ./common.c -o serveur && ./serveur
