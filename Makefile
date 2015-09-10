all: ibody

clean:
	rm -f *.o
	rm -f ibody

ibody: ibody.c
	gcc -o ibody ibody.c --pedantic
