define stop
	docker container stop http_exo 2> /dev/null || true
	docker container rm -f http_exo 2> /dev/null || true
endef

all: docker_image

docker_image: Dockerfile
	@$(call stop)
	docker build . --file Dockerfile --tag http_exo

run: docker_image serveur.c
	docker run -dp 55555:55555 --name http_exo http_exo
	
stop:
	@$(call stop)

test: run
	sleep 1
	./test/test.sh
	@$(call stop)

clean: stop
	docker image rm -f http_exo 2> /dev/null || true
	rm -f docker_image
