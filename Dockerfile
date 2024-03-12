FROM alpine:3.19
RUN apk add gcc musl-dev openssl-dev
COPY ./serveur.c ./common.c ./reponses_websocket.c ./reponses_http.c ./common.h ./reponses_websocket.h ./ reponses_http.h ./
COPY ./static ./static
RUN gcc ./serveur.c ./reponses_websocket.c ./reponses_http.c ./common.c -o serveur -lcrypto
CMD ./serveur
