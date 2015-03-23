main:
	make -C ./lib
	make -C ./cat
	make -C ./revwords
	make -C ./lenwords
clean:
	make clean -C ./lib
	make clean -C ./cat
	make clean -C ./revwords
	make clean -C ./lenwords
