all:
	# gcc -g -Wall -I libuv/include libuv/libuv.a server.c -lpthread -o server
	# gcc -g -Wall -I libuv/include libuv/libuv.a client.c -lpthread -o client
	gcc -g -Wall server.c -I libuv/include libuv/libuv.a -lpthread -o server
	gcc -g -Wall client.c -I libuv/include libuv/libuv.a -lpthread -o client
