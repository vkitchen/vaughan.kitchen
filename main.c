#include <string.h>
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <stdlib.h>
#include <stdio.h>
#include <kcgi.h>

#include "cocktails.h"
#include "rooms.h"
#include "helpers.h"

const char *const pages[PAGE__MAX] = {
	"index",
	"cocktails",
	"rooms"
};

int main(void)
	{
	struct kreq r;

	if (khttp_parse(&r, NULL, 0, pages, PAGE__MAX, 0) != KCGI_OK)
		return(EXIT_FAILURE);

	if (r.page == PAGE_COCKTAILS)
		return(serve_cocktails(&r));

	if (r.page == PAGE_ROOMS)
		return(serve_rooms(&r));

	return(serve_static(&r));
	}
