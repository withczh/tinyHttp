main:main.o common.o tinyHttp.o
	gcc main.o common.o tinyHttp.o -o main -lpthread
main.o:main.c common.h tinyHttp.h
	gcc main.c -c
common.o:common.c common.h
	gcc common.c -c
tinyHttp.o:tinyHttp.c tinyHttp.h
	gcc tinyHttp.c -c
clean:
	rm -f *.o