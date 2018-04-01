error.o: error.h error.c
	gcc -c error.h error.c

my_socket.o: my_socket.h my_socket.c
	gcc -c my_socket.h my_socket.c

httpd: httpd.c my_socket.o error.o
	gcc httpd.c my_socket.o error.o -Wall -Wextra -o httpd
