define stop
	docker container stop http_exo 2> /dev/null || true
	docker container rm -f http_exo 2> /dev/null || true
endef

all: docker_image

serveur:  serveur.c common.c reponses_websocket.c reponses_http.c common.h reponses_websocket.h reponses_http.h
	gcc ./serveur.c ./reponses_websocket.c ./reponses_http.c ./common.c -o serveur -lcrypto

docker_image: Dockerfile serveur.c common.c reponses_websocket.c reponses_http.c common.h reponses_websocket.h reponses_http.h
	@$(call stop)
	docker build . --file Dockerfile --tag http_exo

run: docker_image
	docker run -dp 55555:55555 --name http_exo http_exo
	
stop:
	@$(call stop)

test: run
	sleep 1
	./test/test_http.sh
	@$(call stop)

clean: stop
	docker image rm -f http_exo 2> /dev/null || true
	rm -f docker_image serveur

