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

#include "shared.h"
#include "cocktails.h"

struct ingredient
	{
	int64_t    id;
	char      *name;
	char      *measure;
	char      *unit;
	};

struct ingredient_array
	{
	size_t capacity;
	size_t length;
	struct ingredient **store;
	};

struct cocktail
	{
	int64_t                  id;
	char                    *title;
	char                    *slug;
	char                    *image;
	time_t                   ctime;
	time_t                   mtime;
	char                    *serve;
	char                    *garnish;
	char                    *drinkware;
	char                    *method;
	struct ingredient_array *ingredients;
	};

struct cocktail_array
	{
	size_t capacity;
	size_t length;
	struct cocktail **store;
	};

struct tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	char                  *title;
	char                  *query;
	struct user           *user;
	struct cocktail_array *cocktails;
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

// TODO merge this into pointer array?
// will we get type punning errors if we do that?
struct cocktail_array *
cocktail_array_new()
	{
	struct cocktail_array *a = kmalloc(sizeof(struct cocktail_array));
	a->capacity = 256;
	a->length = 0;
	a->store = kmalloc(a->capacity * sizeof(struct cocktail *));
	return a;
	}

void
cocktail_array_append(struct cocktail_array *a, struct cocktail *c)
	{
	if (a->length == a->capacity)
		{
		a->capacity *= 2;
		a->store = krealloc(a->store, a->capacity * sizeof(struct cocktail *));
		}
	a->store[a->length] = c;
	a->length++;
	}

struct ingredient_array *
ingredient_array_new()
	{
	struct ingredient_array *a = kmalloc(sizeof(struct ingredient_array));
	a->capacity = 256;
	a->length = 0;
	a->store = kmalloc(a->capacity * sizeof(struct ingredient *));
	return a;
	}

void
ingredient_array_append(struct ingredient_array *a, struct ingredient *i)
	{
	if (a->length == a->capacity)
		{
		a->capacity *= 2;
		a->store = krealloc(a->store, a->capacity * sizeof(struct ingredient *));
		}
	a->store[a->length] = i;
	a->length++;
	}

struct ingredient_array *
db_ingredients_get(struct sqlbox *p, size_t dbid, int64_t cocktail_id)
	{
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cocktail_id, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_INGREDIENTS, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct ingredient_array *ingredients = ingredient_array_new();

	const struct sqlbox_parmset *res;
	for (;;)
		{
		if ((res = sqlbox_step(p, stmtid)) == NULL)
			errx(EXIT_FAILURE, "sqlbox_step");

		if (res->code != SQLBOX_CODE_OK)
			errx(EXIT_FAILURE, "res.code");

		// no results or done
		if (res->psz == 0)
			break;

		struct ingredient *ingredient = kmalloc(sizeof(struct ingredient));
		memset(ingredient, 0, sizeof(struct ingredient));

		// id
		if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
			ingredient->id = res->ps[0].iparm;

		// name
		if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
			ingredient->name = kstrdup(res->ps[1].sparm);

		// measure
		if (res->psz >= 3 && res->ps[2].type == SQLBOX_PARM_STRING)
			ingredient->measure = kstrdup(res->ps[2].sparm);

		// unit
		if (res->psz >= 4 && res->ps[3].type == SQLBOX_PARM_STRING)
			ingredient->unit = kstrdup(res->ps[3].sparm);

		ingredient_array_append(ingredients, ingredient);
		}

	// no results
	if (ingredients->length == 0)
		{
		// TODO free
		ingredients = NULL;
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return ingredients;
	}

void
db_cocktail_fill(struct cocktail *cocktail, const struct sqlbox_parmset *res)
	{
	// id
	if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
		cocktail->id = res->ps[0].iparm;

	// title
	if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
		cocktail->title = kstrdup(res->ps[1].sparm);

	// slug
	if (res->psz >= 3 && res->ps[2].type == SQLBOX_PARM_STRING)
		cocktail->slug = kstrdup(res->ps[2].sparm);

	// image
	if (res->psz >= 4 && res->ps[3].type == SQLBOX_PARM_STRING)
		cocktail->image = kstrdup(res->ps[3].sparm);

	// ctime
	if (res->psz >= 5 && res->ps[4].type == SQLBOX_PARM_INT)
		cocktail->ctime = res->ps[4].iparm;

	// mtime
	if (res->psz >= 6 && res->ps[5].type == SQLBOX_PARM_INT)
		cocktail->mtime = res->ps[5].iparm;

	// serve
	if (res->psz >= 7 && res->ps[6].type == SQLBOX_PARM_STRING)
		cocktail->serve = kstrdup(res->ps[6].sparm);

	// garnish
	if (res->psz >= 8 && res->ps[7].type == SQLBOX_PARM_STRING)
		cocktail->garnish = kstrdup(res->ps[7].sparm);

	// drinkware
	if (res->psz >= 9 && res->ps[8].type == SQLBOX_PARM_STRING)
		cocktail->drinkware = kstrdup(res->ps[8].sparm);

	// method
	if (res->psz >= 10 && res->ps[9].type == SQLBOX_PARM_STRING)
		cocktail->method = kstrdup(res->ps[9].sparm);
	}

struct cocktail *
db_cocktail_get(struct sqlbox *p, size_t dbid, char *slug)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_GET, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	// no results
	if (res->psz == 0)
		{
		if (!sqlbox_finalise(p, stmtid))
			errx(EXIT_FAILURE, "sqlbox_finalise");

		return NULL;
		}

	struct cocktail *cocktail = kmalloc(sizeof(struct cocktail));
	memset(cocktail, 0, sizeof(struct cocktail));

	db_cocktail_fill(cocktail, res);
	cocktail->ingredients = db_ingredients_get(p, dbid, cocktail->id);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return cocktail;
	}

struct cocktail_array *
db_cocktail_list(struct sqlbox *p, size_t dbid)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_LIST, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct cocktail_array *cocktails = cocktail_array_new();

	const struct sqlbox_parmset *res;
	for (;;)
		{
		if ((res = sqlbox_step(p, stmtid)) == NULL)
			errx(EXIT_FAILURE, "sqlbox_step");

		if (res->code != SQLBOX_CODE_OK)
			errx(EXIT_FAILURE, "res.code");

		// no results or done
		if (res->psz == 0)
			break;

		struct cocktail *cocktail = kmalloc(sizeof(struct cocktail));
		memset(cocktail, 0, sizeof(struct cocktail));

		db_cocktail_fill(cocktail, res);
		cocktail->ingredients = db_ingredients_get(p, dbid, cocktail->id);

		cocktail_array_append(cocktails, cocktail);
		}

	// no results
	if (cocktails->length == 0)
		{
		// TODO free
		cocktails = NULL;
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return cocktails;
	}

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

	struct cocktail_array *cocktails = db_cocktail_list(p, dbid);

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
		if (strcasestr(cocktails->store[i]->title, query->val) != NULL)
			found = 1;

		if (!found)
			for (size_t j = 0; j < cocktails->store[i]->ingredients->length; j++)
				if (strcasestr(cocktails->store[i]->ingredients->store[j]->name, query->val) != NULL)
					{
					found = 1;
					break;
					}

		if (found)
			cocktail_array_append(cocktails, cocktails->store[i]);
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
