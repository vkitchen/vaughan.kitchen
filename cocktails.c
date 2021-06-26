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
#include "templates.h"

// TODO make use of khtml_printf

struct tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	struct user           *user;
	char                  *title;
	char                  *query;
	struct dynarray       *cocktails;
	size_t                 page_no; /* pagination */
	};

struct drink_tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	struct user           *user;
	struct cocktail       *cocktail;
	struct dynarray       *images;
	};

enum key
	{
	KEY_LOGIN_LOGOUT_LINK,
	KEY_IMAGES_LINK,
	KEY_TITLE,
	KEY_NEXT_PAGE_LINK,
	KEY_NEW_COCKTAIL_LINK,
	KEY_DRINKS,
	KEY__MAX,
	};

static const char *keys[KEY__MAX] =
	{
	"login-logout-link",
	"images-link",
	"title",
	"next-page-link",
	"new-cocktail-link",
	"drinks",
	};

enum drink_key
	{
	DRINK_KEY_LOGIN_LOGOUT_LINK,
	DRINK_KEY_IMAGES_LINK,
	DRINK_KEY_NEW_COCKTAIL_LINK,
	DRINK_KEY_EDIT_COCKTAIL_LINK,
	DRINK_KEY_EDIT_COCKTAIL_PATH,
	DRINK_KEY_TITLE,
	DRINK_KEY_IMAGES, /* image select */
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
	"login-logout-link",
	"images-link",
	"new-cocktail-link",
	"edit-cocktail-link",
	"edit-cocktail-path",
	"drink-title",
	"images",
	"drink-href",
	"drink-img",
	"drink-drinkware",
	"drink-serve",
	"drink-garnish",
	"drink-ingredients",
	"drink-method",
	};

// Return pointer to end of prefix in str
char *string_prefix(char *str, char *pre)
	{
	while (*pre)
		if (*pre++ != *str++)
			return NULL;
	return str;
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
		case (DRINK_KEY_LOGIN_LOGOUT_LINK):
			if (data->user == NULL)
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/login", KATTR__MAX);
				khtml_puts(data->req, "Login");
				khtml_closeelem(data->req, 1);
				}
			else
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/logout", KATTR__MAX);
				khtml_puts(data->req, "Logout");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (DRINK_KEY_IMAGES_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_LI);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/images", KATTR__MAX);
				khtml_puts(data->req, "Images");
				khtml_closeelem(data->req, 2);
				}
			break;
		case (DRINK_KEY_NEW_COCKTAIL_LINK):
			if (data->user == NULL)
				break;
			khtml_attr(data->req, KELEM_A, KATTR_HREF, "/cocktails/drinks/new", KATTR__MAX);
			khtml_puts(data->req, "New Cocktail");
			khtml_closeelem(data->req, 1);
			break;
		case (DRINK_KEY_EDIT_COCKTAIL_LINK):
			if (data->user == NULL)
				break;
			snprintf(buf, sizeof(buf), "/cocktails/drinks/edit/%s", data->cocktail->slug);
			khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
			khtml_puts(data->req, "Edit Cocktail");
			khtml_closeelem(data->req, 1);
			break;
		case (DRINK_KEY_EDIT_COCKTAIL_PATH):
			if (data->user == NULL)
				break;
			snprintf(buf, sizeof(buf), "/cocktails/drinks/edit/%s", data->cocktail->slug);
			khtml_puts(data->req, buf);
			break;
		case (DRINK_KEY_TITLE):
			khtml_puts(data->req, data->cocktail->title);
			break;
		case (DRINK_KEY_IMAGES):
			khtml_attr(data->req, KELEM_OPTION, KATTR_VALUE, "0", KATTR_SELECTED, "selected", KATTR__MAX);
			khtml_puts(data->req, "None");
			khtml_closeelem(data->req, 1);
			for (size_t i = 0; i < data->images->length; i++)
				{
				struct image *image = data->images->store[i];
				snprintf(buf, sizeof(buf), "%lld", image->id);
				khtml_attr(data->req, KELEM_OPTION, KATTR_VALUE, buf, KATTR__MAX);
				khtml_puts(data->req, image->title);
				khtml_closeelem(data->req, 1);
				}
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
				snprintf(buf, sizeof(buf), "/static/img/%s.jpg", data->cocktail->image);
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
			if (data->cocktail->ingredients != NULL)
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
		case (KEY_LOGIN_LOGOUT_LINK):
			if (data->user == NULL)
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/login", KATTR__MAX);
				khtml_puts(data->req, "Login");
				khtml_closeelem(data->req, 1);
				}
			else
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/logout", KATTR__MAX);
				khtml_puts(data->req, "Logout");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_IMAGES_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_LI);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/images", KATTR__MAX);
				khtml_puts(data->req, "Images");
				khtml_closeelem(data->req, 2);
				}
			break;
		case (KEY_TITLE):
			khtml_puts(data->req, data->title);
			break;
		case (KEY_NEW_COCKTAIL_LINK):
			if (data->user == NULL)
				break;
			khtml_attr(data->req, KELEM_A, KATTR_HREF, "/cocktails/drinks/new", KATTR__MAX);
			khtml_puts(data->req, "New Cocktail");
			khtml_closeelem(data->req, 1);
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
handle_get_new_drink(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.title = "New Drink";

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);

	khttp_template_buf(r, &t, tmpl_newdrink_data, tmpl_newdrink_size);
	}

static void
handle_post_new_drink(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct dynarray ingredients;
	struct kpair *title, *slug, *serve, *garnish, *drinkware, *method;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (slug = r->fieldmap[PARAM_SLUG]) == NULL ||
	    (serve = r->fieldmap[PARAM_SERVE]) == NULL ||
	    (garnish = r->fieldmap[PARAM_GARNISH]) == NULL ||
	    (drinkware = r->fieldmap[PARAM_DRINKWARE]) == NULL ||
	    (method = r->fieldmap[PARAM_METHOD]) == NULL)
		return send_400(r);

	dynarray_init(&ingredients);
	char *name = NULL, *measure = NULL, *unit = NULL;
	for (size_t i = 0; i < r->fieldsz; i++)
		{
		// TODO do we need to validate any of this?
		// TODO memory leak if maliciously constructed data
		if (strcmp(r->fields[i].key, "ingredient_name") == 0 && r->fields[i].val[0] != '\0')
			name = kstrdup(r->fields[i].val);
		if (strcmp(r->fields[i].key, "ingredient_measure") == 0 && r->fields[i].val[0] != '\0')
			measure = kstrdup(r->fields[i].val);
		if (strcmp(r->fields[i].key, "ingredient_unit") == 0 && r->fields[i].val[0] != '\0')
			unit = kstrdup(r->fields[i].val);
		if (name != NULL && measure != NULL && unit != NULL)
			{
			struct ingredient *ingredient = kmalloc(sizeof(struct ingredient));
			ingredient->name = name;
			name = NULL;
			ingredient->measure = measure;
			measure = NULL;
			ingredient->unit = unit;
			unit = NULL;
			dynarray_append(&ingredients, ingredient);
			}
		}

	db_cocktail_new(p, dbid, title->parsed.s, slug->parsed.s, serve->parsed.s, garnish->parsed.s, drinkware->parsed.s, method->parsed.s, &ingredients);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/cocktails");
	khttp_body(r);
	}

void
handle_get_edit_drink(struct kreq *r, struct sqlbox *p, size_t dbid, char *drink, struct user *user)
	{
	struct drink_tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;
	struct dynarray images;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&hr, 0, sizeof(struct khtmlreq));

	if (khtml_open(&hr, r, 0) != KCGI_OK)
		errx(EXIT_FAILURE, "khtml_open");

	dynarray_init(&images);
	db_image_list(p, dbid, &images);

	data.r = r;
	data.req = &hr;
	data.user = user;
	data.cocktail = db_cocktail_get(p, dbid, drink);
	data.images = &images;

	t.key = drink_keys;
	t.keysz = DRINK_KEY__MAX;
	t.arg = (void *)&data;
	t.cb = drink_template;

	open_response(r, KHTTP_200);
	khttp_template_buf(r, &t, tmpl_editdrink_data, tmpl_editdrink_size);
	}

static void
handle_post_edit_drink(struct kreq *r, struct sqlbox *p, size_t dbid, char *drink)
	{
	struct kpair *title, *imageid, *serve, *garnish, *drinkware, *method;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (imageid = r->fieldmap[PARAM_IMAGEID]) == NULL ||
	    (serve = r->fieldmap[PARAM_SERVE]) == NULL ||
	    (garnish = r->fieldmap[PARAM_GARNISH]) == NULL ||
	    (drinkware = r->fieldmap[PARAM_DRINKWARE]) == NULL ||
	    (method = r->fieldmap[PARAM_METHOD]) == NULL)
		return send_400(r);

	db_cocktail_update(p, dbid, drink, title->parsed.s, imageid->parsed.i, serve->parsed.s, garnish->parsed.s, drinkware->parsed.s, method->parsed.s);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/cocktails/drinks/%s", drink); // TODO seems to be bug editing chimayo cocktail. Same with jagerbomb
	khttp_body(r);
	}

void
handle_cocktail(struct kreq *r, struct sqlbox *p, size_t dbid, char *drink, struct user *user)
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
	data.user = user;
	data.cocktail = db_cocktail_get(p, dbid, drink);

	t.key = drink_keys;
	t.keysz = DRINK_KEY__MAX;
	t.arg = (void *)&data;
	t.cb = drink_template;

	open_response(r, KHTTP_200);
	khttp_template_buf(r, &t, tmpl_drink_data, tmpl_drink_size);
	}

void
handle_search(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;
	struct dynarray cocktails;

	struct kpair *query;

	if ((query = r->fieldmap[PARAM_QUERY]) == NULL)
		return send_400(r);

	dynarray_init(&cocktails);
	db_cocktail_list(p, dbid, &cocktails);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.user = user;
	data.title = "Search Results";
	data.query = query->val;
	data.cocktails = &cocktails;

	struct kpair *page;
	if ((page = r->fieldmap[PARAM_PAGE]) != NULL)
		data.page_no = page->parsed.i;

	// TODO replace with a second array so that we can dealloc easily
	size_t length = cocktails.length;
	cocktails.length = 0; /* "blank" array */
	for (size_t i = 0; i < length; i++)
		{
		int found = 0;
		struct cocktail *cocktail = cocktails.store[i];
		if (strcasestr(cocktail->title, query->parsed.s) != NULL)
			found = 1;

		if (!found)
			for (size_t j = 0; j < cocktail->ingredients->length; j++)
				if (strcasestr(((struct ingredient *)cocktail->ingredients->store[j])->name, query->parsed.s) != NULL)
					{
					found = 1;
					break;
					}

		if (found)
			dynarray_append(&cocktails, cocktails.store[i]);
		}

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_drinks_list_data, tmpl_drinks_list_size);
	}

void
handle_cocktails(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;
	struct dynarray cocktails;
	char *path;

	if (string_prefix(r->path, "drinks/new") != NULL)
		{
		if (user == NULL)
			send_404(r);
		if (r->method == KMETHOD_GET)
			handle_get_new_drink(r);
		else if (r->method == KMETHOD_POST)
			handle_post_new_drink(r, p, dbid);
		else
			send_405(r);
		return;
		}

	if ((path = string_prefix(r->path, "drinks/edit/")) != NULL)
		{
		if (user == NULL)
			send_404(r);
		if (r->method == KMETHOD_GET)
			handle_get_edit_drink(r, p, dbid, path, user);
		else if (r->method == KMETHOD_POST)
			handle_post_edit_drink(r, p, dbid, path);
		else
			send_405(r);
		return;
		}

	if ((path = string_prefix(r->path, "drinks/")) != NULL)
		return handle_cocktail(r, p, dbid, path, user);

	if (string_prefix(r->path, "search") != NULL)
		return handle_search(r, p, dbid, user);

	dynarray_init(&cocktails);
	memset(&data, 0, sizeof(struct tmpl_data));
	data.user = user;
	data.title = "All Drinks";
	data.cocktails = &cocktails;

	db_cocktail_list(p, dbid, &cocktails);

	struct kpair *page;
	if ((page = r->fieldmap[PARAM_PAGE]) != NULL)
		data.page_no = page->parsed.i;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_drinks_list_data, tmpl_drinks_list_size);
	}
