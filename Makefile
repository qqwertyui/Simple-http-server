CC = gcc
CFLAGS = -Wall -Wextra

all:
		$(CC) $(CFLAGS) app.c http.c socket.c utils.c log.c mime.c -o http_server
		bash ./setup.sh

clean:
		rm http_server -f
		rm *.o -f
