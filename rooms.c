#include <string.h>
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <stdlib.h>
#include <stdio.h>
#include <kcgi.h>

#include "helpers.h"
#include "file.h"
#include "str.h"

#include "rooms.h"

int serve_rooms(struct kreq *r)
	{
	char *file_name;
	file_name = string_cat(2, "rooms/", r->path);

	if (r->fields != NULL) {
		file_spurt(file_name, r->fields->val, r->fields->valsz);
	}
		
	char *page_data;
	size_t file_length = 0;
	file_length = file_slurp(file_name, &page_data);

	if (file_length == 0)
		return(serve_404(r));

	/* RESPONSE */
	response_open(r, KHTTP_200);

	khttp_write(r, page_data, file_length);

	khttp_free(r);
	return(EXIT_SUCCESS);
	}
