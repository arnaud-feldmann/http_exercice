FROM alpine:3.19
RUN apk add gcc musl-dev
COPY ./serveur.c ./serveur.c
COPY ./static ./static
CMD gcc ./serveur.c -o serveur && ./serveur
