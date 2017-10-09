CC=gcc
AR=ar
LD=ld

CFLAGS=-Wall -Werror -g
GIBLIBCFLAGS=`giblib-config --cflags`
GIBLIBLIBS=`giblib-config --libs`
LIB=-lX11 -lgiblib -lImlib2

all: sysscs client
	@echo "Build successful!"

sysscs: sc-server.o
	$(CC) $(GIBLIBLIBS) $^ -o $@ $(LIB)

sc-server.o: sc-server.c
	$(CC) $(CFLAGS) $(GIBLIBCFLAGS) -c $^

client: sc-client.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f *.o
	@rm -f sysscs
	@rm -f client
	@echo "Cleaned"
