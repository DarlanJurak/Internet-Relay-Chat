all:
	gcc -o server server.c  -lpthread -Wall
	gcc -o client client.c  -lpthread -Wall

clean:
	rm -f server client
