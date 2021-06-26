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
	PARAM_IMAGEID,
	PARAM_SERVE,
	PARAM_GARNISH,
	PARAM_DRINKWARE,
	PARAM_METHOD,
	PARAM__MAX,
	};

extern const struct kvalid params[PARAM__MAX];

void
open_head(struct kreq *r, enum khttp code);

void
open_response(struct kreq *r, enum khttp code);

void
send_400(struct kreq *r);

void
send_404(struct kreq *r);

void
send_405(struct kreq *r);

void
send_500(struct kreq *r);

#endif
