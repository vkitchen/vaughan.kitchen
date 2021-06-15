#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <sqlbox.h>
#include <kcgi.h>

enum param
	{
	PARAM_USERNAME,
	PARAM_PASSWORD,
	PARAM_SESSCOOKIE,
	PARAM_QUERY,
	PARAM_PAGE,
	PARAM_TITLE,
	PARAM_ALT,
	PARAM_ATTRIBUTION,
	PARAM_SLUG,
	PARAM_SNIPPET,
	PARAM_CONTENT,
	PARAM_IMAGE,
	PARAM__MAX,
	};

const struct kvalid params[PARAM__MAX];

#endif
