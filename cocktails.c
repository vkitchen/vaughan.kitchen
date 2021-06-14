#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h> /* err */
#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <inttypes.h> /* PRId64 */
#include <time.h> /* time */
#include <sqlbox.h>
#include <kcgi.h>
#include <kcgihtml.h>

#include "db.h"
#include "dynarray.h"
#include "cocktails.h"
#include "shared.h"

struct tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	char                  *title;
	char                  *query;
	struct user           *user;
	struct dynarray       *cocktails;
	size_t                 page_no; /* pagination */
	};

struct drink_tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	struct cocktail       *cocktail;
	};

enum key
	{
	KEY_TITLE,
	KEY_NEXT_PAGE_LINK,
	KEY_DRINKS,
	KEY__MAX,
	};

static const char *keys[KEY__MAX] =
	{
	"title",
	"next-page-link",
	"drinks",
	};

enum drink_key
	{
	DRINK_KEY_TITLE,
	DRINK_KEY_HREF,
	DRINK_KEY_IMG,
	DRINK_KEY_DRINKWARE,
	DRINK_KEY_SERVE,
	DRINK_KEY_GARNISH,
	DRINK_KEY_INGREDIENTS,
	DRINK_KEY_METHOD,
	DRINK_KEY__MAX,
	};

const char *drink_keys[DRINK_KEY__MAX] =
	{
	"drink-title",
	"drink-href",
	"drink-img",
	"drink-drinkware",
	"drink-serve",
	"drink-garnish",
	"drink-ingredients",
	"drink-method",
	};


void
render_ingredient(char *buf, size_t bufsize, char *name, char *measure, char *unit)
	{
	if (strcmp(measure, "0") == 0 && strcmp(unit, "splash") == 0)
		snprintf(buf, bufsize, "%s", name);
	else if (strcmp(measure, "1") == 0 && strcmp(unit, "splash") == 0)
		snprintf(buf, bufsize, "A splash of %s", name);
	else if (strcmp(unit, "splash") == 0)
		snprintf(buf, bufsize, "%s splashes %s", measure, name);
	else if (strcmp(measure, "0") == 0 && strcmp(unit, "dash") == 0)
		snprintf(buf, bufsize, "%s", name);
	else if (strcmp(measure, "1") == 0 && strcmp(unit, "dash") == 0)
		snprintf(buf, bufsize, "A dash of %s", name);
	else if (strcmp(unit, "dash") == 0)
		snprintf(buf, bufsize, "%s dashes %s", measure, name);
	else if (strcmp(measure, "0") == 0 && strcmp(unit, "drop") == 0)
		snprintf(buf, bufsize, "%s", name);
	else if (strcmp(measure, "1") == 0 && strcmp(unit, "drop") == 0)
		snprintf(buf, bufsize, "A drop of %s", name);
	else if (strcmp(unit, "drop") == 0)
		snprintf(buf, bufsize, "%s drops %s", measure, name);
	else if (strcmp(unit, "top") == 0)
		snprintf(buf, bufsize, "Top with %s", name);
	else if (strcmp(measure, "1") == 0 && strcmp(unit, "taste") == 0)
		snprintf(buf, bufsize, "%s to taste", name);
	else if (strcmp(measure, "0") == 0 && strcmp(unit, "none") == 0)
		snprintf(buf, bufsize, "%s", name);
	else if (strcmp(measure, "1") == 0 && strcmp(unit, "none") == 0)
		snprintf(buf, bufsize, "A %s", name);
	else if (strcmp(unit, "none") == 0)
		snprintf(buf, bufsize, "%s %s", measure, name);
	else
		snprintf(buf, bufsize, "%s %s %s", measure, unit, name);
	}

static int
drink_template(size_t key, void *arg)
	{
	char buf[2048];
	struct drink_tmpl_data *data = arg;

	switch (key)
		{
		case (DRINK_KEY_TITLE):
			khtml_puts(data->req, data->cocktail->title);
			break;
		case (DRINK_KEY_HREF):
			snprintf(buf, sizeof(buf), "/cocktails/drinks/%s", data->cocktail->slug);
			khtml_puts(data->req, buf);
			break;
		case (DRINK_KEY_IMG):
			if (data->cocktail->image == NULL || data->cocktail->image[0] == '\0')
				khtml_puts(data->req, "/static/img/Cocktail%20Glass.svg");
			else
				{
				snprintf(buf, sizeof(buf), "/static/img/%s", data->cocktail->image);
				khtml_puts(data->req, buf);
				}
			break;
		case (DRINK_KEY_DRINKWARE):
			if (data->cocktail->drinkware != NULL)
				khtml_puts(data->req, data->cocktail->drinkware);
			break;
		case (DRINK_KEY_SERVE):
			if (data->cocktail->serve != NULL)
				khtml_puts(data->req, data->cocktail->serve);
			break;
		case (DRINK_KEY_GARNISH):
			if (data->cocktail->garnish != NULL)
				khtml_puts(data->req, data->cocktail->garnish);
			break;
		case (DRINK_KEY_INGREDIENTS):
			for (size_t i = 0; i < data->cocktail->ingredients->length; i++)
				{
				khtml_elem(data->req, KELEM_LI);
				struct ingredient *ingredient = data->cocktail->ingredients->store[i];
				render_ingredient(buf, sizeof(buf), ingredient->name, ingredient->measure, ingredient->unit);
				khtml_puts(data->req, buf);
				khtml_closeelem(data->req, 1);
				}
			break;
		case (DRINK_KEY_METHOD):
			if (data->cocktail->method != NULL)
				khtml_puts(data->req, data->cocktail->method);
			break;
		default:
			abort();
		}

	return 1;
	}

static int
template(size_t key, void *arg)
	{
	char buf[2048];
	struct tmpl_data *data = arg;

	switch (key)
		{
		case (KEY_TITLE):
			khtml_puts(data->req, data->title);
			break;
		case (KEY_NEXT_PAGE_LINK):
			if ((data->page_no + 1) * 10 >= data->cocktails->length)
				break;
			if (data->query != NULL)
				snprintf(buf, sizeof(buf), "?q=%s&page=%zd", data->query, data->page_no + 1);
			else
				snprintf(buf, sizeof(buf), "?page=%zd", data->page_no + 1);
			khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
			khtml_puts(data->req, "Next Page");
			khtml_closeelem(data->req, 1);
			break;
		case (KEY_DRINKS):
			{
			struct ktemplate t;
			memset(&t, 0, sizeof(struct ktemplate));

			struct drink_tmpl_data d;
			memset(&d, 0, sizeof(struct drink_tmpl_data));
			d.r = data->r;
			d.req = data->req;

			t.key = drink_keys;
			t.keysz = DRINK_KEY__MAX;
			t.cb = drink_template;
			t.arg = &d;

			size_t rendered = 0;
			for (size_t i = 0; i < data->cocktails->length; i++)
				{
				if (i < data->page_no * 10)
					continue;
				d.cocktail = data->cocktails->store[i];
				khttp_template_buf(data->r, &t, tmpl_drink_snippet_data, tmpl_drink_snippet_size);
				rendered++;
				if (rendered == 10)
					break;
				}
			}
			break;
		default:
			abort();
		}

	return 1;
	}

// TODO merge with main
static void
open_head(struct kreq *r, enum khttp code)
	{
	khttp_head(r, kresps[KRESP_STATUS], "%s", khttps[code]);
	khttp_head(r, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
	}

static void
open_response(struct kreq *r, enum khttp code)
	{
	open_head(r, code);
	khttp_body(r);
	}

// TODO: Do we want to combine this with a render function so you can't try render a non opened template?
static void
open_template(struct tmpl_data *data, struct ktemplate *t, struct khtmlreq *hr, struct kreq *r)
	{
	memset(t, 0, sizeof(struct ktemplate));
	memset(hr, 0, sizeof(struct khtmlreq));

	if (khtml_open(hr, r, 0) != KCGI_OK)
		errx(EXIT_FAILURE, "khtml_open");

	data->r = r;
	data->req = hr;

	t->key = keys;
	t->keysz = KEY__MAX;
	t->arg = (void *)data;
	t->cb = template;
	}
	

void
handle_cocktail(struct kreq *r, struct sqlbox *p, size_t dbid, char *drink)
	{
	struct drink_tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&hr, 0, sizeof(struct khtmlreq));

	if (khtml_open(&hr, r, 0) != KCGI_OK)
		errx(EXIT_FAILURE, "khtml_open");

	data.r = r;
	data.req = &hr;
	data.cocktail = db_cocktail_get(p, dbid, drink);

	t.key = drink_keys;
	t.keysz = DRINK_KEY__MAX;
	t.arg = (void *)&data;
	t.cb = drink_template;

	open_response(r, KHTTP_200);
	khttp_template_buf(r, &t, tmpl_drink_data, tmpl_drink_size);
	}

void
handle_search(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct kpair *query;

	if ((query = r->fieldmap[PARAM_QUERY]) == NULL)
		{
		open_response(r, KHTTP_400);
		khttp_puts(r, "400 Bad Request");
		return;
		}

	struct dynarray *cocktails = db_cocktail_list(p, dbid);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.title = "Search Results";
	data.query = query->val;
	data.cocktails = cocktails;

	struct kpair *page;
	if ((page = r->fieldmap[PARAM_PAGE]) != NULL)
		data.page_no = page->parsed.i;

	size_t length = cocktails->length;
	cocktails->length = 0; /* "blank" array */
	for (size_t i = 0; i < length; i++)
		{
		int found = 0;
		struct cocktail *cocktail = cocktails->store[i];
		if (strcasestr(cocktail->title, query->val) != NULL)
			found = 1;

		if (!found)
			for (size_t j = 0; j < cocktail->ingredients->length; j++)
				if (strcasestr(((struct ingredient *)cocktail->ingredients->store[j])->name, query->val) != NULL)
					{
					found = 1;
					break;
					}

		if (found)
			dynarray_append(cocktails, cocktails->store[i]);
		}

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_drinks_list_data, tmpl_drinks_list_size);
	}

void
handle_cocktails(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// TODO prefix instead of strstr
	char *drink;
	if ((drink = strstr(r->path, "drinks/")) != NULL)
		return handle_cocktail(r, p, dbid, &r->path[strlen("drinks/")]);

	if (strstr(r->path, "search") != NULL)
		return handle_search(r, p, dbid);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.title = "All Drinks";
	data.cocktails = db_cocktail_list(p, dbid);

	struct kpair *page;
	if ((page = r->fieldmap[PARAM_PAGE]) != NULL)
		data.page_no = page->parsed.i;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_drinks_list_data, tmpl_drinks_list_size);
	}
