#include <string.h>
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <kcgi.h>

#include "file.h"

int main(void)
	{
	struct kreq r;
	const char *page = "index";

	char file_name[2048];
	char *page_data;
	size_t file_length = 0;
	int wasdir = 0;
	int isbin = 0;

	if (khttp_parse(&r, NULL, 0, &page, 1, 0) != KCGI_OK)
		return 0;

	if (strlen(r.fullpath) == 0)
		file_length = file_slurp("html/index.html", &page_data);
	else
		{
		strlcpy(file_name, "html", 2048);
		strlcpy(&file_name[4], r.fullpath, 2044);
		if ((wasdir = isDirectory(file_name)))
			strlcpy(&file_name[strlen(file_name)], "/index.html", 2048 - strlen(file_name));
		file_length = file_slurp(file_name, &page_data);
		}

	if (file_length == 0)
		khttp_head(&r, kresps[KRESP_STATUS],
			"%s", khttps[KHTTP_404]);
	else
		khttp_head(&r, kresps[KRESP_STATUS],
			"%s", khttps[KHTTP_200]);
	/* html */
	if (strlen(r.fullpath) == 0 || wasdir || strstr(r.fullpath, ".html") != NULL)
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_TEXT_HTML]);
	/* css */
	else if (strstr(r.fullpath, ".css") != NULL)
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_TEXT_CSS]);
	/* json */
	else if (strstr(r.fullpath, ".json") != NULL)
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_APP_JSON]);
	/* js */
	else if (strstr(r.fullpath, ".js") != NULL)
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_APP_JAVASCRIPT]);
	/* png */
	else if (strstr(r.fullpath, ".png") != NULL)
		{
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_IMAGE_PNG]);
		isbin = 1;
		}
	/* jpg */
	else if (strstr(r.fullpath, ".jpg") != NULL)
		{
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_IMAGE_JPEG]);
		isbin = 1;
		}
	/* svg */
	else if (strstr(r.fullpath, ".svg") != NULL)
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_IMAGE_SVG_XML]);
	/* else plain text */
	else
		khttp_head(&r, kresps[KRESP_CONTENT_TYPE],
			"%s", kmimetypes[KMIME_TEXT_PLAIN]);

	/* RESPONSE */
	khttp_body(&r);

	if (file_length == 0)
		khttp_puts(&r, "404");
	else if (isbin)
		khttp_write(&r, page_data, file_length);
	else
		khttp_puts(&r, page_data);
	khttp_free(&r);
	return 0;
	}
