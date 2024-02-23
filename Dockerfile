FROM alpine:3.19
RUN apk add gcc musl-dev
CMD gcc ./serveur.c -o serveur && ./serveur
