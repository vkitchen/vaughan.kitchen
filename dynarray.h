#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <sys/types.h> /* size_t, ssize_t */

struct dynarray
	{
	size_t capacity;
	size_t length;
	void **store;
	};

void
dynarray_init(struct dynarray *a);

void
dynarray_append(struct dynarray *a, void *val);

void
dynarray_free(struct dynarray *a);

#endif
