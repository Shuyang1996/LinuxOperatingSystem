schedTest.exe: schedTest.o sched.o savectx.S
	gcc -m32 -o schedTest.exe schedTest.o sched.o savectx.S

schedTest.o: schedTest.c
	gcc -m32 -c schedTest.c

sched.o: sched.h sched.c
	gcc -m32 -c sched.c

clean:
	rm *.exe *.o *.stackdump *~

debug:
	gcc -m32 -g -o schedTest.exe schedTest.c sched.c savectx.S

backup:
	test -d backups || mkdir backups
	cp *.cpp backups
	cp *.h backups