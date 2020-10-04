#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <kcgi.h>
#include <kcgihtml.h>

#include "cJSON.h"
#include "file.h"
#include "helpers.h"
#include "str.h"

#include "cocktails.h"

enum key {
	KEY_TITLE,
	KEY_DRINK_NAME,
	KEY_DRINK_HREF,
	KEY_DRINK_IMG,
	KEY_DRINK_DRINKWARE,
	KEY_DRINK_SERVE,
	KEY_DRINK_GARNISH,
	KEY_DRINK_INGREDIENTS,
	KEY_DRINK_METHOD,
	KEY__MAX
};

static const char *const keys[KEY__MAX] = {
	"title",
	"drink_name",
	"drink_href",
	"drink_img",
	"drink_drinkware",
	"drink_serve",
	"drink_garnish",
	"drink_ingredients",
	"drink_method"
};

struct tstrct {
	struct khtmlreq *hr;
	struct kreq *r;
	char *name;
	char *img;
	char *serve;
	char *garnish;
	char *drinkware;
	char **ingredients;
	char *method;
};

static int
template(size_t key, void *arg)
{
	struct tstrct *p = arg;

	switch (key) {
	case (KEY_TITLE):
		if (p->name != NULL)
			khtml_puts(p->hr, p->name);
		break;
	case (KEY_DRINK_NAME):
		if (p->name != NULL)
			khtml_puts(p->hr, p->name);
		break;
	case (KEY_DRINK_HREF):
		if (p->name != NULL)
			khtml_puts(p->hr, string_cat(2, "/cocktails/drinks/", kutil_urlencode(p->name)));
		break;
	case (KEY_DRINK_IMG):
		khtml_puts(p->hr, p->img);
		break;
	case (KEY_DRINK_DRINKWARE):
		if (p->drinkware != NULL)
			khtml_puts(p->hr, p->drinkware);
		break;
	case (KEY_DRINK_SERVE):
		if (p->serve != NULL)
			khtml_puts(p->hr, p->serve);
		break;
	case (KEY_DRINK_GARNISH):
		if (p->garnish != NULL)
			khtml_puts(p->hr, p->garnish);
		break;
	case (KEY_DRINK_INGREDIENTS):
		for (int i = 0; p->ingredients[i] != NULL; i++)
			{
			khtml_elem(p->hr, KELEM_LI);
			khtml_puts(p->hr, p->ingredients[i]);
			khtml_closeelem(p->hr, 1);
			}
		break;
	case (KEY_DRINK_METHOD):
		if (p->method != NULL)
			khtml_puts(p->hr, p->method);
		break;
	default:
		return(0);
	}

	return(1);
}

char *img_missing(char *drinkware)
	{
	if (strcmp(drinkware, "Cocktail glass") == 0)
		return strdup("/img/Cocktail Glass.svg");
	else if (strcmp(drinkware, "Champagne flute") == 0)
		return strdup("/img/Champagne Flute.svg");
	else if (strcmp(drinkware, "Highball glass") == 0)
		return strdup("/img/Highball Glass.svg");
	else if (strcmp(drinkware, "Hurricane glass") == 0)
		return strdup("/img/Hurricane Glass.svg");
	else if (strcmp(drinkware, "Old Fashioned Glass") == 0)
		return strdup("/img/Old Fashioned Glass.svg");
	else if (strcmp(drinkware, "Shot glass") == 0)
		return strdup("/img/Shot Glass.svg");
	else
		return strdup("/img/250x250/Missing.jpg");
	}

struct ingredient {
	char *name;
	char *measure;
	char *unit;
};

char *parse_ingredient(cJSON *json)
	{
	if (json == NULL)
		return NULL;

	struct ingredient ingredient;
	memset(&ingredient, 0, sizeof(struct ingredient));

	json = json->child;

	while (json != NULL)
		{
		if (strcmp(json->string, "name") == 0)
			ingredient.name = json->valuestring;
		else if (strcmp(json->string, "measure") == 0)
			ingredient.measure = json->valuestring;
		else if (strcmp(json->string, "unit") == 0)
			ingredient.unit = json->valuestring;
		json = json->next;
		}

	if (strcmp(ingredient.measure, "0") == 0 && strcmp(ingredient.unit, "splash") == 0)
		return strdup(ingredient.name);
	else if (strcmp(ingredient.measure, "1") == 0 && strcmp(ingredient.unit, "splash") == 0)
		return string_cat(2, "A splash of ", ingredient.name);
	else if (strcmp(ingredient.unit, "splash") == 0)
		return string_cat(3, ingredient.measure, " splashes ", ingredient.name);
	else if (strcmp(ingredient.measure, "0") == 0 && strcmp(ingredient.unit, "dash") == 0)
		return strdup(ingredient.name);
	else if (strcmp(ingredient.measure, "1") == 0 && strcmp(ingredient.unit, "dash") == 0)
		return string_cat(2, "A dash of ", ingredient.name);
	else if (strcmp(ingredient.unit, "dash") == 0)
		return string_cat(3, ingredient.measure, " dashes ", ingredient.name);
	else if (strcmp(ingredient.measure, "0") == 0 && strcmp(ingredient.unit, "drop") == 0)
		return strdup(ingredient.name);
	else if (strcmp(ingredient.measure, "1") == 0 && strcmp(ingredient.unit, "drop") == 0)
		return string_cat(2, "A drop of ", ingredient.name);
	else if (strcmp(ingredient.unit, "drop") == 0)
		return string_cat(3, ingredient.measure, " drops ", ingredient.name);
	else if (strcmp(ingredient.unit, "top") == 0)
		return string_cat(2, "Top with ", ingredient.name);
	else if (strcmp(ingredient.measure, "1") == 0 && strcmp(ingredient.unit, "taste") == 0)
		return string_cat(2, ingredient.name, " to taste");
	else if (strcmp(ingredient.measure, "0") == 0 && strcmp(ingredient.unit, "none") == 0)
		return strdup(ingredient.name);
	else if (strcmp(ingredient.measure, "1") == 0 && strcmp(ingredient.unit, "none") == 0)
		return string_cat(2, "A ", ingredient.name);
	else if (strcmp(ingredient.unit, "none") == 0)
		return string_cat(3, ingredient.measure, " ", ingredient.name);
	else
		return string_cat(5, ingredient.measure, " ", ingredient.unit, " ", ingredient.name);
	}

int parse_drink(struct tstrct *p, char *page_name)
	{
	char *json_data;
	char *file_name = string_cat(3, "html/cocktails/api/v1/drinks/", page_name, ".json");
	size_t file_length = file_slurp(file_name, &json_data);
	if (file_length == 0)
		return 0;

	cJSON *json_handle = cJSON_Parse(json_data);
	cJSON *json = json_handle->child;
	if (json == NULL)
		return 0;

	// TODO validate json structure

	int num_ingredients = 0;
	while (json != NULL)
		{
		if (strcmp(json->string, "name") == 0)
			p->name = strdup(json->valuestring);
		else if (strcmp(json->string, "img") == 0)
			{
			cJSON *sub_json = json->child;
			if (sub_json != NULL)
				p->img = string_cat(2, "/img/250x250/",
					kutil_urlencode(sub_json->valuestring));
			}
		else if (strcmp(json->string, "serve") == 0)
			p->serve = strdup(json->valuestring);
		else if (strcmp(json->string, "garnish") == 0)
			p->garnish = strdup(json->valuestring);
		else if (strcmp(json->string, "drinkware") == 0)
			p->drinkware = strdup(json->valuestring);
		else if (strcmp(json->string, "ingredients") == 0)
			{
			cJSON *sub_json = json->child;
			while (sub_json != NULL)
				{
				p->ingredients[num_ingredients++] = parse_ingredient(sub_json);
				sub_json = sub_json->next;
				}
			}
		else if (strcmp(json->string, "method") == 0)
			p->method = strdup(json->valuestring);
		json = json->next;
		}

	if (p->img == NULL)
		p->img = img_missing(p->drinkware);
	p->img = string_cat(2, "/cocktails", p->img);

	// TODO check have enough data to render

	cJSON_Delete(json_handle);

	return 1;
	}

int serve_drink(struct kreq *r, char *page_name)
	{
	char *decoded_page_name = NULL;
	if (kutil_urldecode(page_name, &decoded_page_name) != KCGI_OK)
		return(serve_500(r)); // TODO change to bad request?

	/* START send_template() */
	struct ktemplate t;
	struct tstrct p;
	struct khtmlreq hr;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&p, 0, sizeof(struct tstrct));
	memset(&hr, 0, sizeof(struct khtmlreq));

	p.r = r;
	p.hr = &hr;

	char *ingredient_list[128];
	memset(ingredient_list, 0, sizeof(ingredient_list));
	p.ingredients = ingredient_list;

	t.key = keys;
	t.keysz = KEY__MAX;
	t.arg = &p;
	t.cb = template;

	if (!parse_drink(&p, decoded_page_name))
		return(serve_500(r));

	response_open(r, KHTTP_200);
	khtml_open(&hr, r, 0);

	khttp_template(r, &t, "tmpl/drink.html");
	/* END send_template() */

	khtml_close(&hr);

	khttp_free(r);

	return(EXIT_SUCCESS);
	}

int drink_cmp(const void *va, const void *vb)
	{
	struct tstrct *a = (struct tstrct *)va;
	struct tstrct *b = (struct tstrct *)vb;
	return strcmp(a->name, b->name);
	}

int read_drinks(struct tstrct *drinks, size_t *drinks_len)
	{
	DIR *dh;
	struct dirent *d;
	dh = opendir("html/cocktails/api/v1/drinks/");

	size_t drink_count = 0;
	if (dh)
		{
		while ((d = readdir(dh)) != NULL)
			{
			char *drink_name = strdup(d->d_name);
			char *suffix;
			if ((suffix = string_suffix(".json", drink_name)) != NULL)
				{
				suffix[0] = '\0';
				drinks[drink_count].ingredients = malloc(sizeof(char **) * 128);
				memset(drinks[drink_count].ingredients, 0, sizeof(char **) * 128);
				if (!parse_drink(&drinks[drink_count], drink_name))
					{
					closedir(dh);
					return 0;
					}
				drink_count++;
				}
			}
		closedir(dh);
		}

	if (drinks_len != NULL)
		*drinks_len = drink_count;

	qsort(drinks, drink_count, sizeof(struct tstrct), drink_cmp);

	return 1;
	}

enum key_drinks_list {
	KEY_DL_TITLE,
	KEY_DL_HEADING,
	KEY_DL_DRINKS,
	KEY_NEXT_PAGE,
	KEY_DL__MAX
};

static const char *const keys_drinks_list[KEY_DL__MAX] = {
	"title",
	"heading",
	"drinks",
	"next_page",
};

struct tstrct_dl {
	struct khtmlreq *hr;
	struct kreq *r;
	char *title;
	char *heading;
	size_t page;
	struct tstrct *drinks;
};

static int
template_drinks_list(size_t key, void *arg)
{
	struct tstrct_dl *dl = arg;

	switch (key) {
	case (KEY_DL_TITLE):
		if (dl->title != NULL)
			khtml_puts(dl->hr, dl->title);
		break;
	case (KEY_DL_HEADING):
		if (dl->heading != NULL)
			khtml_puts(dl->hr, dl->heading);
		break;
	case (KEY_DL_DRINKS):
		if (dl->drinks != NULL)
			{
			// render sub template
			struct ktemplate t;
			memset(&t, 0, sizeof(struct ktemplate));

			t.key = keys;
			t.keysz = KEY__MAX;
			t.cb = template;

			for (size_t i = 0 + dl->page * 10; i < dl->page * 10 + 10 && i < 1024 && dl->drinks[i].name != NULL; i++)
				{
				t.arg = &dl->drinks[i];
				// TODO probably rereading a few hundred times is a bad idea
				khttp_template(dl->r, &t, "tmpl/drink_snippet.html");
				}
			}
		break;
	case (KEY_NEXT_PAGE):
		{
		char buf[1024];
		snprintf(buf, 1024, "?page=%zd", dl->page+1);
		khtml_attr(dl->hr, KELEM_A,
			KATTR_HREF, buf, KATTR__MAX);
		khtml_puts(dl->hr, "Next Page");
		khtml_closeelem(dl->hr, 1);
		break;
		}
	default:
		return(0);
	}

	return(1);
}

int serve_index(struct kreq *r)
	{
	struct tstrct drinks[1024];
	memset(&drinks, 0, sizeof(struct tstrct) * 1024);

	if (!read_drinks(drinks, NULL))
		return(serve_500(r));

	struct ktemplate t;
	struct khtmlreq hr;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&hr, 0, sizeof(struct khtmlreq));

	for (size_t i = 0; i < 1024; i++)
		{
		drinks[i].r = r;
		drinks[i].hr = &hr;
		}

	/* SETUP outer template */
	struct tstrct_dl dl;
	memset(&dl, 0, sizeof(struct tstrct_dl));

	dl.r = r;
	dl.hr = &hr;
	dl.title = "All Drinks";
	dl.heading = "All Drinks";
	dl.page = 0;
	dl.drinks = drinks;

	// TODO don't suppress errors
	if (r->fieldmap[PARAM_PAGE_NO] != NULL)
		dl.page = strtol(r->fieldmap[PARAM_PAGE_NO]->val, NULL, 10);

	t.key = keys_drinks_list;
	t.keysz = KEY_DL__MAX;
	t.arg = &dl;
	t.cb = template_drinks_list;

	response_open(r, KHTTP_200);
	khtml_open(&hr, r, 0);

	khttp_template(r, &t, "tmpl/drinks_list.html");

	khtml_close(&hr);

	khttp_free(r);

	return(EXIT_SUCCESS);
	}

void filter_drinks(struct tstrct *drinks, size_t *drinks_len, char *term)
	{
	// TODO needs to be signed?
	for (size_t i = 0; i < *drinks_len; i++)
		{
		if (strcasestr(drinks[i].name, term) != NULL)
			continue;

		for (size_t j = 0; j < 128 && drinks[i].ingredients[j] != NULL; j++)
			if (strcasestr(drinks[i].ingredients[j], term) != NULL)
				goto OUTER;

		// fall-through = swap delete as did not contain term
		if (i < *drinks_len - 1) // not last item
			drinks[i] = drinks[*drinks_len - 1];
		memset(&drinks[*drinks_len - 1], 0, sizeof(struct tstrct));
		(*drinks_len)--;
		i--;

		OUTER: ;
		}

	qsort(drinks, *drinks_len, sizeof(struct tstrct), drink_cmp);
	}

// TODO refactor out overlapping structure with serve_index()
int serve_search(struct kreq *r)
	{
	struct tstrct drinks[1024];
	memset(&drinks, 0, sizeof(struct tstrct) * 1024);

	size_t drinks_len = 0;
	if (!read_drinks(drinks, &drinks_len))
		return(serve_500(r));

	// TODO refactor this if
	if (r->fields != NULL && strcmp(r->fields->key, "q") == 0
		&& r->fields->val != NULL
		&& r->fields->val[0] != '\0')
		{
		char **terms = string_split(" ", r->fields->val);
		for (size_t i = 0; terms[i] != NULL; i++)
			filter_drinks(drinks, &drinks_len, terms[i]);
		}

	struct ktemplate t;
	struct khtmlreq hr;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&hr, 0, sizeof(struct khtmlreq));

	for (size_t i = 0; i < 1024; i++)
		{
		drinks[i].r = r;
		drinks[i].hr = &hr;
		}

	/* SETUP outer template */
	struct tstrct_dl dl;
	memset(&dl, 0, sizeof(struct tstrct_dl));

	dl.r = r;
	dl.hr = &hr;
	dl.title = "Search Results";
	dl.heading = "Search Results";
	dl.drinks = drinks;

	t.key = keys_drinks_list;
	t.keysz = KEY_DL__MAX;
	t.arg = &dl;
	t.cb = template_drinks_list;

	response_open(r, KHTTP_200);
	khtml_open(&hr, r, 0);

	khttp_template(r, &t, "tmpl/drinks_list.html");

	khtml_close(&hr);

	khttp_free(r);

	return(EXIT_SUCCESS);
	}

int serve_cocktails(struct kreq *r)
	{
	if (r->mime != KMIME_TEXT_HTML)
		return(serve_static_encoded(r));

	char *page = NULL;
	if ((page = string_prefix("drinks/", r->path)) != NULL)
		return(serve_drink(r, page));

	if (string_prefix("search", r->path) != NULL)
		return(serve_search(r));

	if (r->path[0] == '\0')
		return(serve_index(r));

	return(serve_404(r));
	}
