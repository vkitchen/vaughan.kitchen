#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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
	"drink_img",
	"drink_drinkware",
	"drink_serve",
	"drink_garnish",
	"drink_ingredients",
	"drink_method"
};

struct tstrct {
	struct khtmlreq hr;
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
			khtml_puts(&p->hr, p->name);
		break;
	case (KEY_DRINK_NAME):
		if (p->name != NULL)
			khtml_puts(&p->hr, p->name);
		break;
	case (KEY_DRINK_IMG):
		khtml_puts(&p->hr, p->img);
		break;
	case (KEY_DRINK_DRINKWARE):
		if (p->drinkware != NULL)
			khtml_puts(&p->hr, p->drinkware);
		break;
	case (KEY_DRINK_SERVE):
		if (p->serve != NULL)
			khtml_puts(&p->hr, p->serve);
		break;
	case (KEY_DRINK_GARNISH):
		if (p->garnish != NULL)
			khtml_puts(&p->hr, p->garnish);
		break;
	case (KEY_DRINK_INGREDIENTS):
		for (int i = 0; p->ingredients[i] != NULL; i++)
			{
			khtml_elem(&p->hr, KELEM_LI);
			khtml_puts(&p->hr, p->ingredients[i]);
			khtml_closeelem(&p->hr, 1);
			}
		break;
	case (KEY_DRINK_METHOD):
		if (p->method != NULL)
			khtml_puts(&p->hr, p->method);
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

int serve_drink(struct kreq *r, char *page_name)
	{
	char file_name[2048];
	char *json_data;
	size_t file_length = 0;

	strlcpy(file_name, "html/cocktails/api/v1/drinks/", 2048);
	strlcpy(&file_name[strlen(file_name)], page_name, 2048 - strlen(file_name));
	strlcpy(&file_name[strlen(file_name)], ".json", 2048 - strlen(file_name));
	file_length = file_slurp(file_name, &json_data);
	if (file_length == 0)
		return(serve_500(r));

	cJSON *json_handle = cJSON_Parse(json_data);
	cJSON *json = json_handle->child;
	if (json == NULL)
		return(serve_500(r));

	/* START send_template() */
	struct ktemplate t;
	struct tstrct p;

	memset(&t, 0, sizeof(struct ktemplate));
	memset(&p, 0, sizeof(struct tstrct));

	p.r = r;

	int num_ingredients = 0;
	char *ingredient_list[128];
	memset(ingredient_list, 0, sizeof(ingredient_list));
	p.ingredients = ingredient_list;

	t.key = keys;
	t.keysz = KEY__MAX;
	t.arg = &p;
	t.cb = template;

	/* START json_parse() */
	// TODO validate json structure

	while (json != NULL)
		{
		if (strcmp(json->string, "name") == 0)
			p.name = json->valuestring;
		else if (strcmp(json->string, "img") == 0)
			{
			cJSON *sub_json = json->child;
			if (sub_json != NULL)
				p.img = string_cat(2, "/img/250x250/", sub_json->valuestring);
			}
		else if (strcmp(json->string, "serve") == 0)
			p.serve = json->valuestring;
		else if (strcmp(json->string, "garnish") == 0)
			p.garnish = json->valuestring;
		else if (strcmp(json->string, "drinkware") == 0)
			p.drinkware = json->valuestring;
		else if (strcmp(json->string, "ingredients") == 0)
			{
			cJSON *sub_json = json->child;
			while (sub_json != NULL)
				{
				p.ingredients[num_ingredients++] = parse_ingredient(sub_json);
				sub_json = sub_json->next;
				}
			}
		else if (strcmp(json->string, "method") == 0)
			p.method = json->valuestring;
		json = json->next;
		}

	if (p.img == NULL)
		p.img = img_missing(p.drinkware);
	p.img = string_cat(2, "/cocktails", p.img);

	// TODO check have enough data to render

	/* END json_parse() */

	response_open(r, KHTTP_200);
	khtml_open(&p.hr, r, 0);

	khttp_template(r, &t, "tmpl/drink.html");
	/* END send_template() */

	khtml_close(&p.hr);

	khttp_free(r);

	cJSON_Delete(json_handle);

	return(EXIT_SUCCESS);
	}

int serve_cocktails(struct kreq *r)
	{
	if (r->mime != KMIME_TEXT_HTML)
		return(serve_static(r));

	char *page = NULL;
	if ((page = string_prefix("drinks/", r->path)) != NULL)
		return(serve_drink(r, page));

	return(serve_404(r));
	}
