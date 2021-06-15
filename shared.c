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
	{ NULL, "image" }, /* TODO is this right? What about NULL bytes */
	};
