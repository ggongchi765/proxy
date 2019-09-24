proxy_cache : server.o
	gcc -o proxy_cache  server.o -lcrypto -lpthread 
server.o : server.c
	gcc -c server.c -lcrypto -lpthread
