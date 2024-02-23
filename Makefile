all: serveur

serveur: serveur.c
	cc serveur.c -o serveur
	
clean:
	rm -f serveur
