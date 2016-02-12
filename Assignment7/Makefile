problem4.exe: problem4.o fifo.o sem.o tas64.s
	gcc -o problem4.exe problem4.o fifo.o sem.o tas64.s

problem4.o: problem4.c
	gcc -c problem4.c

fifo.o: fifo.h fifo.c
	gcc -c fifo.c

sem.o: sem.h sem.c
	gcc -c sem.c

clean:
	rm *.exe *.o *.stackdump *~

debug:
	gcc -g -o memoryMapebug.exe memorymap.c

backup:
	test -d backups || mkdir backups
	cp *.cpp backups
	cp *.h backups
