CFLAGS = -Wall -Wextra -O2 -g `pkg-config --cflags kcgi-html sqlbox lowdown`

LDFLAGS = -static `pkg-config --static --libs kcgi-html sqlbox lowdown`

.SUFFIXES: .c .o

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $<

all: main

# main: main.o $(OBJECTS) $(HEADERS)
#	$(CC) -o $@ main.o $(OBJECTS) $(LDFLAGS)

main: main.o
	$(CC) -o $@ main.o $(LDFLAGS)

clean:
	rm -f main main.o $(OBJECTS)

upload:
	rsync -avz --exclude='.git/' --exclude='design' . deltacephei:~/vaughan.kitchen

dev:
	rsync -av --exclude='.git/' --exclude='design' . /var/www/htdocs/vaughan.kitchen

