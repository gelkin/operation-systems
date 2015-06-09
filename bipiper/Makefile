main:
	gcc -L../lib -Wl,-rpath=../lib -I../lib -Wall -std=c99 forking.c -o forking -lbufio
	gcc -L../lib -Wl,-rpath=../lib -I../lib -Wall -std=c99 polling.c -o polling -lbufio
clean:
	rm -f forking
	rm -f polling
