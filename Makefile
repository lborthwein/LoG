
g: legendofgiselda.c
	gcc -ggdb -std=c99 -Wall -o g legendofgiselda.c -lncurses -lpthread -O3

clean:
	rm -f *.o a.out core g
