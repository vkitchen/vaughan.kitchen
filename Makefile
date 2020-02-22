CFLAGS = -I/usr/local/include -Wall -Wextra -O2 -g

LDFLAGS = -static -L/usr/local/lib -lkcgi -lz

HEADERS = \
	file.h

SRC = \
	file.c

OBJECTS = $(SRC:.c=.o)

.SUFFIXES: .c .o

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $<

all: main

main: main.o $(OBJECTS) $(HEADERS)
	$(CC) -o $@ main.o $(OBJECTS) $(LDFLAGS)

clean:
	rm -f main main.o $(OBJECTS)

upload:
	rsync -avz --exclude='.git/' . deltacephei:~/vaughan.kitchen

dev:
	rsync -av --exclude='.git/' . /var/www/htdocs/vaughan.kitchen

