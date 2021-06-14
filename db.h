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
	STMT_IMAGE_NEW,
	STMT_IMAGE_LIST,
	STMT_PAGE_GET,
	STMT_PAGE_UPDATE,
	STMT_POST_GET,
	STMT_POST_NEW,
	STMT_POST_UPDATE,
	STMT_POST_LIST,
	STMT_RECIPE_GET,
	STMT_RECIPE_NEW,
	STMT_RECIPE_UPDATE,
	STMT_RECIPE_LIST,
	STMT_COCKTAIL_GET,
	STMT_COCKTAIL_LIST,
	STMT_COCKTAIL_INGREDIENTS,
	STMT__MAX,
	};

struct sqlbox_pstmt pstmts[STMT__MAX];

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
	char                    *image;
	time_t                   ctime;
	time_t                   mtime;
	char                    *serve;
	char                    *garnish;
	char                    *drinkware;
	char                    *method;
	struct dynarray         *ingredients;
	};

struct user *
db_user_checkpass(struct sqlbox *p, size_t dbid, const char *username, const char *password);

void
db_sess_new(struct sqlbox *p, size_t dbid, int64_t cookie, struct user *user);

struct user *
db_sess_get(struct sqlbox *p, size_t dbid, int64_t cookie);

void
db_sess_del(struct sqlbox *p, size_t dbid, int64_t cookie);

void
db_image_new(struct sqlbox *p, size_t dbid, char *title, char *alt, char *attribution, char *hash, char *format);

struct dynarray *
db_image_list(struct sqlbox *p, size_t dbid);

struct post *
db_page_get(struct sqlbox *p, size_t dbid, char *page);

void
db_page_update(struct sqlbox *p, size_t dbid, char *page, char *content);

struct dynarray *
db_post_list(struct sqlbox *p, size_t dbid, size_t stmt);

struct post *
db_post_get(struct sqlbox *p, size_t dbid, size_t stmt, char *slug);

void
db_post_new(struct sqlbox *p, size_t dbid, size_t stmt, char *title, char *slug, char *snippet, char *content);

void
db_post_update(struct sqlbox *p, size_t dbid, size_t stmt, char *title, char *slug, char *snippet, char *content);

/*************/
/* COCKTAILS */
/*************/

struct dynarray *
db_ingredients_get(struct sqlbox *p, size_t dbid, int64_t cocktail_id);

void
db_cocktail_fill(struct cocktail *cocktail, const struct sqlbox_parmset *res);

struct cocktail *
db_cocktail_get(struct sqlbox *p, size_t dbid, char *slug);

struct dynarray *
db_cocktail_list(struct sqlbox *p, size_t dbid);

#endif