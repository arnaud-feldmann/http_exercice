all: docker_image

docker_image: stop Dockerfile
	docker build . --file Dockerfile --tag http_exo
	touch docker_image

run: docker_image serveur.c
	docker create -p 55555:55555 --name http_exo http_exo
	docker cp ./static/. http_exo:/static/
	docker cp ./serveur.c http_exo:/serveur.c
	docker start http_exo
	
stop:
	docker container stop http_exo 2> /dev/null || true
	docker container rm -f http_exo 2> /dev/null || true

clean: stop
	docker image rm -f http_exo 2> /dev/null || true
	rm -f docker_image
