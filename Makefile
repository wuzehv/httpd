error.o: error.h error.c
	gcc -c error.h error.c

common.o: common.h common.c
	gcc -c common.h common.c

httpd: httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h error.o common.o
	gcc error.o common.o httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h -Wall -Wextra -o httpd
# gcc httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h -o httpd
