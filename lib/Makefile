main: helpers.o
	gcc -shared helpers.o -o libhelpers.so
helpers.o:
	gcc -c -Wall -Werror -fpic helpers.c
clean:
	rm -f helpers.o libhelpers.so
