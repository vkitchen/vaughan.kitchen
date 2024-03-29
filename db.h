#ifndef DB_H
#define DB_H

#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */

enum stmt
	{
	STMT_USER_GET,
	STMT_SESS_GET,
	STMT_SESS_NEW,
	STMT_SESS_DEL,
	STMT_IMAGE_GET,
	STMT_IMAGE_NEW,
	STMT_IMAGE_UPDATE,
	STMT_IMAGE_LIST,
	STMT_POST_GET,
	STMT_POST_NEW,
	STMT_POST_UPDATE,
	STMT_POST_LIST,
	STMT_COCKTAIL_GET,
	STMT_COCKTAIL_NEW,
	STMT_COCKTAIL_UPDATE,
	STMT_COCKTAIL_MAX,
	STMT_COCKTAIL_LIST,
	STMT_INGREDIENT_NEW,
	STMT_COCKTAIL_INGREDIENTS,
	STMT__MAX,
	};

extern struct sqlbox_pstmt pstmts[STMT__MAX];

/*********/
/* TYPES */
/*********/

struct user
	{
	int64_t    id;
	char      *username; /* user's login */
	char      *name;     /* user's display name */
	};

struct image
	{
	int64_t    id;
	char      *title;
	char      *alt;
	char      *attribution;
	time_t     ctime;
	char      *hash;
	char      *format;
	};

struct post
	{
	int64_t    id;
	char      *title;
	char      *slug;
	char      *snippet;
	time_t     ctime;
	time_t     mtime;
	char      *content;
	char      *image;
	int64_t    published;
	};

struct ingredient
	{
	int64_t    id;
	char      *name;
	char      *measure;
	char      *unit;
	};

struct cocktail
	{
	int64_t                  id;
	char                    *title;
	char                    *slug;
	char                    *image_hash;
	struct image            *image;
	time_t                   ctime;
	time_t                   mtime;
	char                    *serve;
	char                    *garnish;
	char                    *drinkware;
	char                    *method;
	struct dynarray         *ingredients;
	};

void
user_free(struct user *u);

void
image_free(struct image *i);

void
post_free(struct post *p);

void
ingredient_free(struct ingredient *i);

void
cocktail_free(struct cocktail *c);

/**********/
/* SHARED */
/**********/

struct user *
db_user_checkpass(struct sqlbox *p, size_t dbid, const char *username, const char *password);

void
db_sess_new(struct sqlbox *p, size_t dbid, int64_t cookie, struct user *user);

struct user *
db_sess_get(struct sqlbox *p, size_t dbid, int64_t cookie);

void
db_sess_del(struct sqlbox *p, size_t dbid, int64_t cookie);

void
db_image_new(struct sqlbox *p, size_t dbid, const char *title, const char *alt, const char *attribution, const char *hash, const char *format);

void
db_image_update(struct sqlbox *p, size_t dbid, const char *hash, const char *title, const char *alt, const char *attribution);

void
db_image_list(struct sqlbox *p, size_t dbid, struct dynarray *result);

struct image *
db_image_get(struct sqlbox *p, size_t dbid, const char *hash);

void
db_post_list(struct sqlbox *p, size_t dbid, const char *category, struct dynarray *result);

struct post *
db_post_get(struct sqlbox *p, size_t dbid, const char *slug);

void
db_post_new(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *snippet, const char *content, int published, const char *category);

void
db_post_update(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *snippet, const char *content, int published);

/*************/
/* COCKTAILS */
/*************/

void
db_ingredients_get(struct sqlbox *p, size_t dbid, struct dynarray *result, int64_t cocktail_id);

void
db_cocktail_fill(struct cocktail *cocktail, const struct sqlbox_parmset *res);

struct cocktail *
db_cocktail_get(struct sqlbox *p, size_t dbid, const char *slug);

void
db_cocktail_new(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *serve, const char *garnish, const char *drinkware, const char *method, struct dynarray *ingredients);

void
db_cocktail_update(struct sqlbox *p, size_t dbid, const char *slug, const char *title, int64_t image_id, const char *serve, const char *garnish, const char *drinkware, const char *method);

void
db_cocktail_list(struct sqlbox *p, size_t dbid, struct dynarray *result);

#endif