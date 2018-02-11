# ssdsim linux support
all:bmain

clean:
	rm -f bmain *.o *~
.PHONY: clean

bmain: bmain.o avlTree.o buffer.o
	cc -g -o bmain bmain.o avlTree.o buffer.o
bmain.o: buffer.h
	gcc -c -g bmain.c
avlTree.o:
	gcc -c -g avlTree.c
buffer.o:
	gcc -c -g buffer.c
