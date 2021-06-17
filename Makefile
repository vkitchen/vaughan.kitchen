CFLAGS = -Wall -Wextra -O2 -g `pkg-config --cflags kcgi-html sqlbox lowdown`

LDFLAGS = -static `pkg-config --static --libs kcgi-html sqlbox lowdown`

TEMPLATES = \
	tmpl/blog.m4 \
	tmpl/cv.m4 \
	tmpl/drink.m4 \
	tmpl/drink_snippet.m4 \
	tmpl/drinks_list.m4 \
	tmpl/editcv.m4 \
	tmpl/editpost.m4 \
	tmpl/editrecipe.m4 \
	tmpl/games.m4 \
	tmpl/images.m4 \
	tmpl/index.m4 \
	tmpl/login.m4 \
	tmpl/newdrink.m4 \
	tmpl/newimage.m4 \
	tmpl/newpost.m4 \
	tmpl/newrecipe.m4 \
	tmpl/post.m4 \
	tmpl/recipe.m4 \
	tmpl/recipes.m4

COMPTEMPS = $(TEMPLATES:.m4=.html)

HEADERS = \
	cocktails.h \
	db.h \
	dynarray.h \
	shared.h \
	templates.h

SRC = \
	cocktails.c \
	db.c \
	dynarray.c \
	shared.c \
	templates.c

OBJECTS = $(SRC:.c=.o)

.SUFFIXES: .m4 .html .c .o

.m4.html:
	m4 $< > $@

.c.o: $(HEADERS)
	$(CC) $(CFLAGS) -c $<

all: main

main: main.o $(OBJECTS) $(HEADERS)
	$(CC) -o $@ main.o $(OBJECTS) $(LDFLAGS)

templates.c: templates.h $(COMPTEMPS)

clean:
	rm -f main main.o $(OBJECTS) $(COMPTEMPS)

upload:
	rsync -avz --include='main' --include='static/***' --exclude='*' . deltacephei:~/vaughan.kitchen

dev:
	rsync -av --include='main' --include='static/***' --exclude='*' . /var/www/htdocs/vaughan.kitchen

