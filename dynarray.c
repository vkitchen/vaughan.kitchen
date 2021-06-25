#include "dynarray.h"

#include <stdlib.h> /* free */
#include <stddef.h> /* NULL */
#include <stdarg.h> /* va_list */
#include <kcgi.h> /* kmalloc */

void
dynarray_init(struct dynarray *a)
	{
	a->capacity = 256;
	a->length = 0;
	a->store = kmalloc(a->capacity * sizeof(void *));
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
	free(a->store);
	}
