#include <stdlib.h>

#include "shared.h"

const struct kvalid params[PARAM__MAX] =
	{
	{ kvalid_stringne, "username" },
	{ kvalid_stringne, "password" },
	{ kvalid_uint, "s" }, /* session */
	{ kvalid_stringne, "q" }, /* query (search) */
	{ kvalid_uint, "page" },
	{ kvalid_stringne, "title" },
	{ kvalid_stringne, "alt" },
	{ kvalid_stringne, "attribution" },
	{ kvalid_stringne, "slug" },
	{ kvalid_string, "snippet" },
	{ kvalid_stringne, "content" },
	{ kvalid_stringne, "published" }, /* checkbox */
	{ NULL, "image" }, /* TODO is this right? What about NULL bytes */
	{ kvalid_uint, "imageid" },
	{ kvalid_stringne, "serve" },
	{ kvalid_stringne, "garnish" },
	{ kvalid_stringne, "drinkware" },
	{ kvalid_stringne, "method" },
	};

void
open_head(struct kreq *r, enum khttp code)
	{
	khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[code]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
	}

void
open_response(struct kreq *r, enum khttp code)
	{
	open_head(r, code);
	khttp_body(r);
	}

void
send_400(struct kreq *r)
	{
	open_response(r, KHTTP_400);
	khttp_puts(r, "400 Bad Request");
	}

void
send_404(struct kreq *r)
	{
	open_response(r, KHTTP_404);
	khttp_puts(r, "404 Not Found");
	}

void
send_405(struct kreq *r)
	{
	open_response(r, KHTTP_405);
	khttp_puts(r, "405 Method Not Allowed");
	}

void
send_500(struct kreq *r)
	{
	open_response(r, KHTTP_500);
	khttp_puts(r, "500 Internal Server Error");
	}
