#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> /* pledge */
#include <err.h> /* err */
#include <sys/types.h> /* size_t, ssize_t */
#include <sys/stat.h> /* struct stat */
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <inttypes.h> /* PRId64 */
#include <time.h> /* time */
#include <sys/queue.h> /* dep of lowdown */
#include <netinet/in.h> /* dep of resolv.h */
#include <resolv.h> /* b64_ntop */
#include <md5.h>
#include <lowdown.h>
#include <sqlbox.h>
#include <kcgi.h>
#include <kcgihtml.h>

#include "db.h"
#include "dynarray.h"

#define USER "users.id,users.display,users.login"
#define POST "posts.id,posts.title,posts.slug,posts.snippet,posts.ctime,posts.mtime,posts.content,images.hash"
#define RECIPE "recipes.id,recipes.title,recipes.slug,recipes.snippet,recipes.ctime,recipes.mtime,recipes.content,images.hash"

struct sqlbox_pstmt pstmts[STMT__MAX] =
	{
	/* STMT_USER_GET */
	{ .stmt = (char *)"SELECT " USER ",users.shadow FROM users WHERE login=?" },
	/* STMT_SESS_GET */
	{ .stmt = (char *)"SELECT " USER " FROM sessions INNER JOIN users ON users.id = sessions.user_id WHERE sessions.cookie=?" },
	/* STMT_SESS_NEW */
	{ .stmt = (char *)"INSERT INTO sessions (cookie,user_id) VALUES (?,?)" },
	/* STMT_SESS_DEL */
	{ .stmt = (char *)"DELETE FROM sessions WHERE cookie=?" },
	/* STMT_IMAGE_NEW */
	{ .stmt = (char *)"INSERT INTO images (title,alt,attribution,hash,format) VALUES (?,?,?,?,?)" },
	/* STMT_IMAGE_LIST */
	{ .stmt = (char *)"SELECT id,title,alt,attribution,ctime,hash,format FROM images ORDER BY id ASC" },
	/* STMT_PAGE_GET */
	{ .stmt = (char *)"SELECT mtime,content FROM pages WHERE title=?" },
	/* STMT_PAGE_UPDATE */
	{ .stmt = (char *)"UPDATE pages SET mtime=?,content=? WHERE title=?" },
	/* STMT_POST_GET */
	{ .stmt = (char *)"SELECT " POST " FROM posts LEFT JOIN images ON posts.image_id=images.id WHERE slug=?" },
	/* STMT_POST_NEW */
	{ .stmt = (char *)"INSERT INTO posts (title,slug,snippet,content,user_id) VALUES (?,?,?,?,1)" },
	/* STMT_POST_UPDATE */
	{ .stmt = (char *)"UPDATE posts SET title=?,snippet=?,mtime=?,content=? WHERE slug=?" },
	/* STMT_POST_LIST */
	{ .stmt = (char *)"SELECT " POST " FROM posts LEFT JOIN images ON posts.image_id=images.id ORDER BY posts.id DESC" },
	/* STMT_RECIPE_GET */
	{ .stmt = (char *)"SELECT " RECIPE " FROM recipes LEFT JOIN images ON recipes.image_id=images.id WHERE slug=?" },
	/* STMT_RECIPE_NEW */
	{ .stmt = (char *)"INSERT INTO recipes (title,slug,snippet,content,user_id) VALUES (?,?,?,?,1)" },
	/* STMT_RECIPE_UPDATE */
	{ .stmt = (char *)"UPDATE recipes SET title=?,snippet=?,mtime=?,content=? WHERE slug=?" },
	/* STMT_RECIPE_LIST */
	{ .stmt = (char *)"SELECT " RECIPE " FROM recipes LEFT JOIN images ON recipes.image_id=images.id ORDER BY recipes.id DESC" },
	/* STMT_COCKTAIL_GET */
	{ .stmt = (char *)"SELECT id,title,slug,image,ctime,mtime,serve,garnish,drinkware,method FROM cocktails WHERE slug=?" },
	/* STMT_COCKTAIL_LIST */
	{ .stmt = (char *)"SELECT id,title,slug,image,ctime,mtime,serve,garnish,drinkware,method FROM cocktails ORDER BY title ASC" },
	/* STMT_COCKTAIL_INGREDIENTS */
	{ .stmt = (char *)"SELECT id,name,measure,unit FROM cocktail_ingredients WHERE cocktail_id=? ORDER BY id ASC" },
	};

void
db_post_free(struct post *p)
	{
	if (p == NULL)
		return;
	free(p->title);
	free(p->slug);
	free(p->snippet);
	free(p->content);
	free(p->image);
	free(p);
	}

void
db_post_fill(struct post *post, const struct sqlbox_parmset *res)
	{
	// id
	if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
		post->id = res->ps[0].iparm;

	// title
	if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
		post->title = kstrdup(res->ps[1].sparm);

	// slug
	if (res->psz >= 3 && res->ps[2].type == SQLBOX_PARM_STRING)
		post->slug = kstrdup(res->ps[2].sparm);

	// snippet
	if (res->psz >= 4 && res->ps[3].type == SQLBOX_PARM_STRING)
		post->snippet = kstrdup(res->ps[3].sparm);

	// ctime
	if (res->psz >= 5 && res->ps[4].type == SQLBOX_PARM_INT)
		post->ctime = res->ps[4].iparm;

	// mtime
	if (res->psz >= 6 && res->ps[5].type == SQLBOX_PARM_INT)
		post->mtime = res->ps[5].iparm;

	// content
	if (res->psz >= 7 && res->ps[6].type == SQLBOX_PARM_STRING)
		post->content = kstrdup(res->ps[6].sparm);

	// image
	if (res->psz >= 8 && res->ps[7].type == SQLBOX_PARM_STRING)
		post->image = kstrdup(res->ps[7].sparm);
	}

void
db_user_free(struct user *u)
	{
	if (u == NULL)
		return;
	free(u->name);
	free(u->username);
	free(u);
	}

void
db_user_fill(struct user *user, const struct sqlbox_parmset *res)
	{
	// id
	if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
		user->id = res->ps[0].iparm;

	// display
	if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
		user->name = kstrdup(res->ps[1].sparm);

	// login
	if (res->psz >= 3 && res->ps[2].type == SQLBOX_PARM_STRING)
		user->username = kstrdup(res->ps[2].sparm);
	}

// TODO think of new name for this function
struct user *
db_user_get(struct sqlbox *p, size_t dbid, const char *username, const char *password)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = username, .type = SQLBOX_PARM_STRING },
		};
 
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_USER_GET, 1, parms, 0)))
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

	struct user *user = kmalloc(sizeof(struct user));
	memset(user, 0, sizeof(struct user));

	db_user_fill(user, res);

	// password
	char *hash = NULL;
	if (res->psz >= 4 && res->ps[3].type == SQLBOX_PARM_STRING)
		hash = kstrdup(res->ps[3].sparm);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	if (crypt_checkpass(password, hash) != 0)
		{
		db_user_free(user);
		user = NULL;
		}

	free(hash);

	return user;
	}

void
db_sess_new(struct sqlbox *p, size_t dbid, int64_t cookie, struct user *user)
	{
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cookie, .type = SQLBOX_PARM_INT },
		{ .iparm = user->id, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_SESS_NEW, 2, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

struct user *
db_sess_get(struct sqlbox *p, size_t dbid, int64_t cookie)
	{
	if (cookie == -1)
		return NULL;
 
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cookie, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_SESS_GET, 1, parms, 0)))
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

	struct user *user = kmalloc(sizeof(struct user));
	memset(user, 0, sizeof(struct user));

	db_user_fill(user, res);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return user;
	}

void
db_sess_del(struct sqlbox *p, size_t dbid, int64_t cookie)
	{
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cookie, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_SESS_DEL, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

void
db_image_new(struct sqlbox *p, size_t dbid, char *title, char *alt, char *attribution, char *hash, char *format)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = alt, .type = SQLBOX_PARM_STRING },
		{ .sparm = attribution, .type = SQLBOX_PARM_STRING },
		{ .sparm = hash, .type = SQLBOX_PARM_STRING },
		{ .sparm = format, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_IMAGE_NEW, 5, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

struct dynarray *
db_image_list(struct sqlbox *p, size_t dbid)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_IMAGE_LIST, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct dynarray *images = dynarray_new(NULL);

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

		struct image *image = kmalloc(sizeof(struct image));
		memset(image, 0, sizeof(struct image));

		// id
		if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
			image->id = res->ps[0].iparm;

		// title
		if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
			image->title = kstrdup(res->ps[1].sparm);

		// alt
		if (res->psz >= 3 && res->ps[2].type == SQLBOX_PARM_STRING)
			image->alt = kstrdup(res->ps[2].sparm);

		// attribution
		if (res->psz >= 4 && res->ps[3].type == SQLBOX_PARM_STRING)
			image->attribution = kstrdup(res->ps[3].sparm);

		// ctime
		if (res->psz >= 5 && res->ps[4].type == SQLBOX_PARM_INT)
			image->ctime = res->ps[4].iparm;

		// hash
		if (res->psz >= 6 && res->ps[5].type == SQLBOX_PARM_STRING)
			image->hash = kstrdup(res->ps[5].sparm);

		// format
		if (res->psz >= 7 && res->ps[6].type == SQLBOX_PARM_STRING)
			image->format = kstrdup(res->ps[6].sparm);

		dynarray_append(images, image);
		}

	// no results
	if (images->length == 0)
		{
		// TODO free
		images = NULL;
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return images;
	}

struct post *
db_page_get(struct sqlbox *p, size_t dbid, char *page)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = page, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_PAGE_GET, 1, parms, 0)))
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

	struct post *post = kmalloc(sizeof(struct post));
	memset(post, 0, sizeof(struct post));

	// mtime
	if (res->psz >= 1 && res->ps[0].type == SQLBOX_PARM_INT)
		post->mtime = res->ps[0].iparm;

	// content
	if (res->psz >= 2 && res->ps[1].type == SQLBOX_PARM_STRING)
		post->content = kstrdup(res->ps[1].sparm);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return post;
	}

void
db_page_update(struct sqlbox *p, size_t dbid, char *page, char *content)
	{
	time_t mtime = time(NULL);

	struct sqlbox_parm parms[] =
		{
		{ .iparm = mtime, .type = SQLBOX_PARM_INT },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		{ .sparm = page, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_PAGE_UPDATE, 3, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

// TODO restrict statements
struct dynarray *
db_post_list(struct sqlbox *p, size_t dbid, size_t stmt)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, stmt, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct dynarray *posts = dynarray_new(NULL);

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

		struct post *post = kmalloc(sizeof(struct post));
		memset(post, 0, sizeof(struct post));

		db_post_fill(post, res);
		dynarray_append(posts, post);
		}

	// no results
	if (posts->length == 0)
		{
		dynarray_free(posts);
		posts = NULL;
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return posts;
	}

struct post *
db_post_get(struct sqlbox *p, size_t dbid, size_t stmt, char *slug)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, stmt, 1, parms, 0)))
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

	struct post *post = kmalloc(sizeof(struct post));
	memset(post, 0, sizeof(struct post));

	db_post_fill(post, res);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return post;
	}

void
db_post_new(struct sqlbox *p, size_t dbid, size_t stmt, char *title, char *slug, char *snippet, char *content)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		{ .sparm = snippet, .type = SQLBOX_PARM_STRING },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, stmt, 4, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

void
db_post_update(struct sqlbox *p, size_t dbid, size_t stmt, char *title, char *slug, char *snippet, char *content)
	{
	time_t mtime = time(NULL);

	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = snippet, .type = SQLBOX_PARM_STRING },
		{ .iparm = mtime, .type = SQLBOX_PARM_INT },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, stmt, 5, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

/*************/
/* COCKTAILS */
/*************/

struct dynarray *
db_ingredients_get(struct sqlbox *p, size_t dbid, int64_t cocktail_id)
	{
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cocktail_id, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_INGREDIENTS, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct dynarray *ingredients = dynarray_new(NULL);

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

		dynarray_append(ingredients, ingredient);
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

struct dynarray *
db_cocktail_list(struct sqlbox *p, size_t dbid)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_LIST, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct dynarray *cocktails = dynarray_new(NULL);

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

		dynarray_append(cocktails, cocktail);
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
