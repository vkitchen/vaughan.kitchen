CFLAGS = -I/usr/local/include -Wall -Wextra -O2 -g

LDFLAGS = -static -L/usr/local/lib -lpthread -lcurl -lnghttp2 -lssl -lcrypto -lssl -lcrypto -lz

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: main

main: main.o
	$(CC) -o $@ main.o $(LDFLAGS)

clean:
	rm -f main main.o
