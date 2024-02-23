FROM alpine:3.19
COPY ./serveur.c ./serveur.c
RUN apk add gcc musl-dev && gcc serveur.c -o serveur
CMD ["./serveur"]
