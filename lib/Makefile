all: helpers bufio

helpers: helpers.o
	gcc -shared helpers.o -o libhelpers.so
helpers.o:
	gcc -std=c99 -c -Wall -Werror -fpic helpers.c

bufio: bufio.o
	gcc -shared bufio.o -o libbufio.so
bufio.o:
	gcc -std=c99 -c -Wall -Werror -fpic bufio.c

clean:
	rm -f helpers.o libhelpers.so bufio.o libbufio.so
