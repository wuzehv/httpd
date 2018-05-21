error.o: error.h error.c
	gcc -c error.h error.c

httpd: httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h
	gcc httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h -Wall -Wextra -o httpd
# gcc httpd.c my_socket.c my_socket.h fastcgi.c fastcgi.h -o httpd
