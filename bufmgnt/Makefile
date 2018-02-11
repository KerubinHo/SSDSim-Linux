# ssdsim linux support
all:bmain

clean:
	rm -f bmain *.o *~
.PHONY: clean

bmain: bmain.o avlTree.o buffer.o
	cc -g -o bmain.o avlTree.o buffer.o
bmain.o: buffer.h
	gcc -c -g bmain.c
avlTree.o: avlTree.h
	gcc -c -g avlTree.c
buffer.o: buffer.h avlTree.h
	gcc -c -g buffer.c
