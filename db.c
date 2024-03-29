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
#define POST "posts.id,posts.title,posts.slug,posts.snippet,posts.ctime,posts.mtime,posts.content,images.hash,posts.published"
#define COCKTAIL "cocktails.id,cocktails.title,cocktails.slug,images.hash,cocktails.ctime,cocktails.mtime,cocktails.serve,cocktails.garnish,cocktails.drinkware,cocktails.method"

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
	/* STMT_IMAGE_GET */
	{ .stmt = (char *)"SELECT id,title,alt,attribution,ctime,hash,format FROM images WHERE hash=?" },
	/* STMT_IMAGE_NEW */
	{ .stmt = (char *)"INSERT INTO images (title,alt,attribution,hash,format) VALUES (?,?,?,?,?)" },
	/* STMT_IMAGE_UPDATE */
	{ .stmt = (char *)"UPDATE images set title=?,alt=?,attribution=? WHERE hash=?" }, // TODO add mtime
	/* STMT_IMAGE_LIST */
	{ .stmt = (char *)"SELECT id,title,alt,attribution,ctime,hash,format FROM images ORDER BY id ASC" },
	/* STMT_POST_GET */
	{ .stmt = (char *)"SELECT " POST " FROM posts LEFT JOIN images ON posts.image_id=images.id WHERE slug=?" },
	/* STMT_POST_NEW */
	{ .stmt = (char *)"INSERT INTO posts (title,slug,snippet,content,user_id,published,category) VALUES (?,?,?,?,1,?,?)" },
	/* STMT_POST_UPDATE */
	{ .stmt = (char *)"UPDATE posts SET title=?,snippet=?,mtime=?,content=?,published=? WHERE slug=?" },
	/* STMT_POST_LIST */
	{ .stmt = (char *)"SELECT " POST " FROM posts LEFT JOIN images ON posts.image_id=images.id WHERE category=? ORDER BY posts.id DESC" },
	/* STMT_COCKTAIL_GET */
	{ .stmt = (char *)"SELECT " COCKTAIL " FROM cocktails LEFT JOIN images ON cocktails.image_id=images.id WHERE slug=?" },
	/* STMT_COCKTAIL_NEW */
	{ .stmt = (char *)"INSERT INTO cocktails (title,slug,serve,garnish,drinkware,method) VALUES (?,?,?,?,?,?)" },
	/* STMT_COCKTAIL_UPDATE */
	{ .stmt = (char *)"UPDATE cocktails SET title=?,mtime=?,serve=?,garnish=?,drinkware=?,method=?,image_id=? WHERE slug=?" },
	/* STMT_COCKTAIL_MAX */
	{ .stmt = (char *)"SELECT MAX(id) FROM cocktails" }, // TODO INSERT RETURNING coming real soon now
	/* STMT_COCKTAIL_LIST */
	{ .stmt = (char *)"SELECT " COCKTAIL " FROM cocktails LEFT JOIN images ON cocktails.image_id=images.id ORDER BY cocktails.title ASC" },
	/* STMT_INGREDIENT_NEW */
	{ .stmt = (char *)"INSERT INTO cocktail_ingredients (name,measure,unit,cocktail_id) VALUES (?,?,?,?)" },
	/* STMT_COCKTAIL_INGREDIENTS */
	{ .stmt = (char *)"SELECT id,name,measure,unit FROM cocktail_ingredients WHERE cocktail_id=? ORDER BY id ASC" },
	};

/*
static void
scan_float(double *p, const struct sqlbox_parmset *res, size_t i)
	{
	if (res->psz > i && res->ps[i].type == SQLBOX_PARM_FLOAT)
		*p = res->ps[i].fparm;
	}
*/

static void
scan_int(int64_t *p, const struct sqlbox_parmset *res, size_t i)
	{
	if (res->psz > i && res->ps[i].type == SQLBOX_PARM_INT)
		*p = res->ps[i].iparm;
	}

static void
scan_string(char **p, const struct sqlbox_parmset *res, size_t i)
	{
	if (res->psz > i && res->ps[i].type == SQLBOX_PARM_STRING)
		*p = kstrdup(res->ps[i].sparm);
	}

static void
user_fill(struct user *user, const struct sqlbox_parmset *res)
	{
	size_t i = 0;
	scan_int(&user->id, res, i++);
	scan_string(&user->name, res, i++);
	scan_string(&user->username, res, i++);
	}

void
user_free(struct user *u)
	{
	if (u == NULL)
		return;
	free(u->name);
	free(u->username);
	free(u);
	}

struct user *
db_user_checkpass(struct sqlbox *p, size_t dbid, const char *username, const char *password)
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

	user_fill(user, res);

	// password
	char *hash = NULL;
	scan_string(&hash, res, 3);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	if (crypt_checkpass(password, hash) != 0)
		{
		user_free(user);
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

	user_fill(user, res);

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
db_image_new(struct sqlbox *p, size_t dbid, const char *title, const char *alt, const char *attribution, const char *hash, const char *format)
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

void
db_image_update(struct sqlbox *p, size_t dbid, const char *hash, const char *title, const char *alt, const char *attribution)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = alt, .type = SQLBOX_PARM_STRING },
		{ .sparm = attribution, .type = SQLBOX_PARM_STRING },
		{ .sparm = hash, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_IMAGE_UPDATE, 4, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

static void
image_fill(struct image *image, const struct sqlbox_parmset *res)
	{
	size_t i = 0;
	scan_int(&image->id, res, i++);
	scan_string(&image->title, res, i++);
	scan_string(&image->alt, res, i++);
	scan_string(&image->attribution, res, i++);
	scan_int(&image->ctime, res, i++);
	scan_string(&image->hash, res, i++);
	scan_string(&image->format, res, i++);
	}

void
image_free(struct image *i)
	{
	if (i == NULL)
		return;
	free(i->title);
	free(i->alt);
	free(i->attribution);
	free(i->hash);
	free(i->format);
	free(i);
	}

void
db_image_list(struct sqlbox *p, size_t dbid, struct dynarray *result)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_IMAGE_LIST, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

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
		image_fill(image, res);

		dynarray_append(result, image);
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

struct image *
db_image_get(struct sqlbox *p, size_t dbid, const char *hash)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = hash, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_IMAGE_GET, 1, parms, 0)))
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

	struct image *image = kmalloc(sizeof(struct image));
	memset(image, 0, sizeof(struct image));
	image_fill(image, res);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return image;
	}

static void
post_fill(struct post *post, const struct sqlbox_parmset *res)
	{
	size_t i = 0;
	scan_int(&post->id, res, i++);
	scan_string(&post->title, res, i++);
	scan_string(&post->slug, res, i++);
	scan_string(&post->snippet, res, i++);
	scan_int(&post->ctime, res, i++);
	scan_int(&post->mtime, res, i++);
	scan_string(&post->content, res, i++);
	scan_string(&post->image, res, i++);
	scan_int(&post->published, res, i++);
	}

void
post_free(struct post *p)
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
db_post_list(struct sqlbox *p, size_t dbid, const char *category, struct dynarray *result)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = category, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_POST_LIST, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

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
		post_fill(post, res);

		dynarray_append(result, post);
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

struct post *
db_post_get(struct sqlbox *p, size_t dbid, const char *slug)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_POST_GET, 1, parms, 0)))
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
	post_fill(post, res);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return post;
	}

void
db_post_new(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *snippet, const char *content, int published, const char *category)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		{ .sparm = snippet, .type = SQLBOX_PARM_STRING },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		{ .iparm = published, .type = SQLBOX_PARM_INT },
		{ .sparm = category, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_POST_NEW, 6, parms, 0)))
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
db_post_update(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *snippet, const char *content, int published)
	{
	time_t mtime = time(NULL);

	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = snippet, .type = SQLBOX_PARM_STRING },
		{ .iparm = mtime, .type = SQLBOX_PARM_INT },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		{ .iparm = published, .type = SQLBOX_PARM_INT },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_POST_UPDATE, 6, parms, 0)))
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

static void
ingredient_fill(struct ingredient *ingredient, const struct sqlbox_parmset *res)
	{
	size_t i = 0;
	scan_int(&ingredient->id, res, i++);
	scan_string(&ingredient->name, res, i++);
	scan_string(&ingredient->measure, res, i++);
	scan_string(&ingredient->unit, res, i++);
	}

void
ingredient_free(struct ingredient *i)
	{
	if (i == NULL)
		return;
	free(i->name);
	free(i->measure);
	free(i->unit);
	free(i);
	}

void
db_ingredients_get(struct sqlbox *p, size_t dbid, struct dynarray *result, int64_t cocktail_id)
	{
	struct sqlbox_parm parms[] =
		{
		{ .iparm = cocktail_id, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_INGREDIENTS, 1, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

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
		ingredient_fill(ingredient, res);

		dynarray_append(result, ingredient);
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}

void
db_ingredient_new(struct sqlbox *p, size_t dbid, int64_t cocktail_id, char *name, char *measure, char *unit)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = name, .type = SQLBOX_PARM_STRING },
		{ .sparm = measure, .type = SQLBOX_PARM_STRING },
		{ .sparm = unit, .type = SQLBOX_PARM_STRING },
		{ .iparm = cocktail_id, .type = SQLBOX_PARM_INT },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_INGREDIENT_NEW, 4, parms, 0)))
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

static void
cocktail_fill(struct cocktail *cocktail, const struct sqlbox_parmset *res)
	{
	size_t i = 0;
	scan_int(&cocktail->id, res, i++);
	scan_string(&cocktail->title, res, i++);
	scan_string(&cocktail->slug, res, i++);
	scan_string(&cocktail->image_hash, res, i++);
	scan_int(&cocktail->ctime, res, i++);
	scan_int(&cocktail->mtime, res, i++);
	scan_string(&cocktail->serve, res, i++);
	scan_string(&cocktail->garnish, res, i++);
	scan_string(&cocktail->drinkware, res, i++);
	scan_string(&cocktail->method, res, i++);
	}

void
cocktail_free(struct cocktail *c)
	{
	if (c == NULL)
		return;
	free(c->title);
	free(c->slug);
	free(c->image_hash);
	free(c->serve);
	free(c->garnish);
	free(c->drinkware);
	free(c->method);
	// TODO properly free ingredients
	if (c->ingredients != NULL)
		dynarray_free(c->ingredients);
	free(c);
	}

struct cocktail *
db_cocktail_get(struct sqlbox *p, size_t dbid, const char *slug)
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

	cocktail_fill(cocktail, res);

	if (cocktail->image_hash != NULL && cocktail->image_hash[0] != '\0')
		cocktail->image = db_image_get(p, dbid, cocktail->image_hash);

	cocktail->ingredients = kmalloc(sizeof(struct dynarray));
	dynarray_init(cocktail->ingredients);
	db_ingredients_get(p, dbid, cocktail->ingredients, cocktail->id);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return cocktail;
	}

int64_t
db_cocktail_max(struct sqlbox *p, size_t dbid)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_MAX, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	if (res->psz == 0)
		errx(EXIT_FAILURE, "res.zero");

	// row id
	int64_t cocktail_id;
	scan_int(&cocktail_id, res, 0);

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	return cocktail_id;
	}

void
db_cocktail_new(struct sqlbox *p, size_t dbid, const char *title, const char *slug, const char *serve, const char *garnish, const char *drinkware, const char *method, struct dynarray *ingredients)
	{
	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		{ .sparm = serve, .type = SQLBOX_PARM_STRING },
		{ .sparm = garnish, .type = SQLBOX_PARM_STRING },
		{ .sparm = drinkware, .type = SQLBOX_PARM_STRING },
		{ .sparm = method, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_NEW, 6, parms, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	const struct sqlbox_parmset *res;
	if ((res = sqlbox_step(p, stmtid)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_step");

	if (res->code != SQLBOX_CODE_OK)
		errx(EXIT_FAILURE, "res.code");

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");

	int64_t cocktail_id = db_cocktail_max(p, dbid);

	for (size_t i = 0; i < ingredients->length; i++)
		{
		struct ingredient *ingredient = ingredients->store[i];
		db_ingredient_new(p, dbid, cocktail_id, ingredient->name, ingredient->measure, ingredient->unit);
		}
	}

void
db_cocktail_update(struct sqlbox *p, size_t dbid, const char *slug, const char *title, int64_t image_id, const char *serve, const char *garnish, const char *drinkware, const char *method)
	{
	time_t mtime = time(NULL);

	struct sqlbox_parm parms[] =
		{
		{ .sparm = title, .type = SQLBOX_PARM_STRING },
		{ .iparm = mtime, .type = SQLBOX_PARM_INT },
		{ .sparm = serve, .type = SQLBOX_PARM_STRING },
		{ .sparm = garnish, .type = SQLBOX_PARM_STRING },
		{ .sparm = drinkware, .type = SQLBOX_PARM_STRING },
		{ .sparm = method, .type = SQLBOX_PARM_STRING },
		{ .iparm = image_id, .type = SQLBOX_PARM_INT },
		{ .sparm = slug, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_UPDATE, 8, parms, 0)))
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
db_cocktail_list(struct sqlbox *p, size_t dbid, struct dynarray *result)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_COCKTAIL_LIST, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

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

		cocktail_fill(cocktail, res);
		// TODO move into fill?
		cocktail->ingredients = kmalloc(sizeof(struct dynarray));
		dynarray_init(cocktail->ingredients);
		db_ingredients_get(p, dbid, cocktail->ingredients, cocktail->id);

		dynarray_append(result, cocktail);
		}

	// finalise
	if (!sqlbox_finalise(p, stmtid))
		errx(EXIT_FAILURE, "sqlbox_finalise");
	}
