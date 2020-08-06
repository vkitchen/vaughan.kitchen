CFLAGS = -I/usr/local/include -Wall -Wextra -O2 -g

LDFLAGS = -static -L/usr/local/lib -lpthread -lcurl -lnghttp2 -lssl -lcrypto -lssl -lcrypto -lz

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: main render

main: main.o
	$(CC) -o $@ main.o $(LDFLAGS)

render: render.o
	$(CC) -o $@ render.o $(LDFLAGS)

clean:
	rm -f main main.o
