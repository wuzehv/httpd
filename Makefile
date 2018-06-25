error.o: error.h error.c
	gcc -c error.h error.c

common.o: common.h common.c
	gcc -c common.h common.c

rio.o: rio.h rio.c
	gcc -c rio.h rio.c

httpd: httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h error.o common.o rio.o
	gcc error.o common.o httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h rio.o -Wall -Wextra -o httpd
# gcc httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h -o httpd
