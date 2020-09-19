CC = gcc
CFLAGS = -Wall -Wextra

all:
		$(CC) $(CFLAGS) app.c http.c socket.c utils.c mime.c -o http_server

clean:
		rm http_server -f
		rm *.o -f
