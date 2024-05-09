CC = gcc
CFLAGS = -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: echoserveri echoclient

echoserveri: echo.o csapp.o echoserveri.o
	$(CC) $(CFLAGS) -o echoserveri echo.o csapp.o echoserveri.o $(LIB)

echoclient: echoclient.o csapp.o
	$(CC) $(CFLAGS) -o echoclient csapp.o echoclient.o $(LIB)

echoclient.o: echoclient.c
	$(CC) $(CFLAGS) -c echoclient.c

echoserveri.o: echoserveri.c
	$(CC) $(CFLAGS) -c echoserveri.c

echo.o: echo.c
	$(CC) $(CFLAGS) -c echo.c

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

clean:
	rm -f *.o echoclient echoserveri
