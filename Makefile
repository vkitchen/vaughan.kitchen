CFLAGS = -Wall -Wextra -O2 -g `pkg-config --cflags kcgi-html sqlbox lowdown`

LDFLAGS = -static `pkg-config --static --libs kcgi-html sqlbox lowdown`

HEADERS = \
	shared.h \
	cocktails.h

SRC = \
	shared.c \
	cocktails.c

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
	rsync -avz --exclude='.git/' --exclude='design' --exclude='db' . deltacephei:~/vaughan.kitchen

dev:
	rsync -av --exclude='.git/' --exclude='design' --exclude='db' . /var/www/htdocs/vaughan.kitchen

