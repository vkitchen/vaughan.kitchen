#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <kcgi.h>

#include "file.h"

#include "helpers.h"

void response_open(struct kreq *r, enum khttp http_status)
	{
	enum kmime mime;

	/*
	 * Unknown mime -> octet-stream for binary files
	 */
	if ((mime = r->mime) == KMIME__MAX)
		mime = KMIME_APP_OCTET_STREAM;

	khttp_head(r, kresps[KRESP_STATUS],
		"%s", khttps[http_status]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE],
		"%s", kmimetypes[mime]);
	khttp_body(r);
	}

int serve_404(struct kreq *r)
	{
	response_open(r, KHTTP_404);
	khttp_puts(r, "404");
	khttp_free(r);
	return(EXIT_SUCCESS);
	}

int serve_500(struct kreq *r)
	{
	response_open(r, KHTTP_500);
	khttp_puts(r, "500");
	khttp_free(r);
	return(EXIT_SUCCESS);
	}

int serve_static(struct kreq *r)
	{
	char file_name[2048];
	char *page_data;
	size_t file_length = 0;

	if (r->page == PAGE_INDEX)
		file_length = file_slurp("html/index.html", &page_data);
	else
		{
		strlcpy(file_name, "html", 2048);
		strlcpy(&file_name[4], r->fullpath, 2044);
		if (isDirectory(file_name))
			strlcpy(&file_name[strlen(file_name)], "/index.html", 2048 - strlen(file_name));
		file_length = file_slurp(file_name, &page_data);
		}

	if (file_length == 0)
		return(serve_404(r));

	/* RESPONSE */
	response_open(r, KHTTP_200);

	khttp_write(r, page_data, file_length);

	khttp_free(r);
	return(EXIT_SUCCESS);
	}
