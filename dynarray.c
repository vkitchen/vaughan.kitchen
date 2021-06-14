#include "dynarray.h"

#include <stdlib.h> /* free */
#include <stddef.h> /* NULL */
#include <stdarg.h> /* va_list */
#include <kcgi.h> /* kmalloc */

struct dynarray *
dynarray_new(void (*destructor)(void *))
	{
	struct dynarray *a = kmalloc(sizeof(struct dynarray));
	a->capacity = 256;
	a->length = 0;
	a->store = kmalloc(a->capacity * sizeof(void *));
	a->destructor = destructor;
	return a;
	}

void
dynarray_append(struct dynarray *a, void *val)
	{
	if (a->length == a->capacity)
		{
		a->capacity *= 2;
		a->store = krealloc(a->store, a->capacity * sizeof(void *));
		}
	a->store[a->length++] = val;
	}

void
dynarray_free(struct dynarray *a)
	{
	if (a->destructor != NULL)
		for (size_t i = 0; i < a->length; i++)
			a->destructor(a->store[i]);
	free(a->store);
	free(a);
	}
