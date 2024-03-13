define stop
	docker container stop http_exo 2> /dev/null || true
	docker container rm -f http_exo 2> /dev/null || true
endef

all: docker_image

serveur:  serveur.c http_common.c http_websocket_handshake.c http_reponses.c serveur_websocket.c http_common.h http_websocket_handshake.h http_reponses.h
	gcc ./serveur.c ./http_websocket_handshake.c ./http_reponses.c ./http_common.c -o serveur -lcrypto
	gcc ./serveur_websocket.c -o serveur_websocket

docker_image: Dockerfile serveur.c http_common.c http_websocket_handshake.c http_reponses.c serveur_websocket.c http_common.h http_websocket_handshake.h http_reponses.h
	@$(call stop)
	docker build . --file Dockerfile --tag http_exo

run: docker_image
	docker run -dp 55555:55555 --name http_exo http_exo
	
stop:
	@$(call stop)

test: run
	sleep 1
	./test/test_http.sh
	./test/test_websocket_handshake.sh
	@$(call stop)

clean: stop
	docker image rm -f http_exo 2> /dev/null || true
	rm -f docker_image serveur

