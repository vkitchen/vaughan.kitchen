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
#include <lowdown.h>
#include <sqlbox.h>
#include <kcgi.h>
#include <kcgihtml.h>
#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "external/incbin/incbin.h"

/* A_data, A_end, A_size */
INCBIN(tmpl_blog, "tmpl/blog.html");
INCBIN(tmpl_cv, "tmpl/cv.html");
INCBIN(tmpl_drink, "tmpl/drink.html");
INCBIN(tmpl_drink_snippet, "tmpl/drink_snippet.html");
INCBIN(tmpl_drinks_list, "tmpl/drinks_list.html");
INCBIN(tmpl_editcv, "tmpl/editcv.html");
INCBIN(tmpl_editpost, "tmpl/editpost.html");
INCBIN(tmpl_editrecipe, "tmpl/editrecipe.html");
INCBIN(tmpl_index, "tmpl/index.html");
INCBIN(tmpl_login, "tmpl/login.html");
INCBIN(tmpl_newpost, "tmpl/newpost.html");
INCBIN(tmpl_newrecipe, "tmpl/newrecipe.html");
INCBIN(tmpl_post, "tmpl/post.html");
INCBIN(tmpl_recipe, "tmpl/recipe.html");
INCBIN(tmpl_recipes, "tmpl/recipes.html");

// TODO use fastcgi
// TODO read all templates upon startup and then sandbox more heavily

#define LOGFILE "vaughan.kitchen.log"

struct user
	{
	int64_t    id;
	char      *username; /* user's login */
	char      *name;     /* user's display name */
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
	};

struct post_array
	{
	size_t capacity;
	size_t length;
	struct post **store;
	};

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

enum page
	{
	PAGE_INDEX,
	PAGE_CV,
	PAGE_EDIT_CV,
	PAGE_BLOG,
	PAGE_POST,
	PAGE_NEW_POST,
	PAGE_EDIT_POST,
	PAGE_COCKTAILS,
	PAGE_RECIPES,
	PAGE_RECIPE,
	PAGE_NEW_RECIPE,
	PAGE_EDIT_RECIPE,
	PAGE_GAMES,
	PAGE_LOGIN,
	PAGE_LOGOUT,
	PAGE__MAX
	};

const char *const pages[PAGE__MAX] =
	{
	"index",
	"cv",
	"editcv",
	"blag",
	"post",
	"newpost",
	"editpost",
	"cocktails",
	"recipes",
	"recipe",
	"newrecipe",
	"editrecipe",
	"games",
	"login",
	"logout",
	};

struct tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	enum page              page;
	char                  *head_title;
	struct user           *user;
	struct post           *post;
	struct post_array     *posts;
	struct cocktail_array *cocktails;
	int                    raw; /* don't render markdown */
	int                    page_no; /* pagination */
	};

struct drink_tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	struct cocktail       *cocktail;
	};

enum param
	{
	PARAM_USERNAME,
	PARAM_PASSWORD,
	PARAM_SESSCOOKIE,
	PARAM_TITLE,
	PARAM_SLUG,
	PARAM_SNIPPET,
	PARAM_CONTENT,
	PARAM__MAX,
	};

const struct kvalid params[PARAM__MAX] =
	{
	{ kvalid_stringne, "username" },
	{ kvalid_stringne, "password" },
	{ kvalid_uint, "s" },
	/* post */
	{ kvalid_stringne, "title" },
	{ kvalid_stringne, "slug" },
	{ kvalid_stringne, "snippet" },
	{ kvalid_stringne, "content" },
	};

enum key
	{
	KEY_HEAD_TITLE,
	KEY_NEXT_PAGE_LINK,
	KEY_LOGIN_LOGOUT_LINK,
	KEY_EDIT_CV_LINK,
	KEY_NEW_POST_LINK,
	KEY_EDIT_POST_LINK,
	KEY_EDIT_POST_PATH,
	KEY_NEW_RECIPE_LINK,
	KEY_EDIT_RECIPE_LINK,
	KEY_EDIT_RECIPE_PATH,
	KEY_TITLE,
	KEY_SNIPPET,
	KEY_MTIME,
	KEY_CONTENT,
	KEY_POSTS,
	KEY_DRINKS,
	KEY__MAX,
	};

const char *keys[KEY__MAX] =
	{
	"head-title",
	"next-page-link",
	"login-logout-link",
	/* post */
	"edit-cv-link",
	"new-post-link",
	"edit-post-link",
	"edit-post-path",
	"new-recipe-link",
	"edit-recipe-link",
	"edit-recipe-path",
	"title",
	"snippet",
	"mtime",
	"content", /* markdown content */
	/* post list */
	"posts",   /* list of posts */
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

struct sqlbox_src srcs[] =
	{
	{ .fname = (char *)"db/db.db", .mode = SQLBOX_SRC_RW },
	};

enum stmt
	{
	STMT_USER_GET,
	STMT_SESS_GET,
	STMT_SESS_NEW,
	STMT_SESS_DEL,
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

#define USER "users.id,users.display,users.login"
#define POST "id,title,slug,snippet,ctime,mtime,content"

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
	/* STMT_PAGE_GET */
	{ .stmt = (char *)"SELECT mtime,content FROM pages WHERE title=?" },
	/* STMT_PAGE_UPDATE */
	{ .stmt = (char *)"UPDATE pages SET mtime=?,content=? WHERE title=?" },
	/* STMT_POST_GET */
	{ .stmt = (char *)"SELECT " POST " FROM posts WHERE slug=?" },
	/* STMT_POST_NEW */
	{ .stmt = (char *)"INSERT INTO posts (title,slug,snippet,content,user_id) VALUES (?,?,?,?,1)" },
	/* STMT_POST_UPDATE */
	{ .stmt = (char *)"UPDATE posts SET title=?,snippet=?,mtime=?,content=? WHERE slug=?" },
	/* STMT_POST_LIST */
	{ .stmt = (char *)"SELECT " POST " FROM posts ORDER BY ctime DESC" },
	/* STMT_RECIPE_GET */
	{ .stmt = (char *)"SELECT " POST " FROM recipes WHERE slug=?" },
	/* STMT_RECIPE_NEW */
	{ .stmt = (char *)"INSERT INTO recipes (title,slug,snippet,content,user_id) VALUES (?,?,?,?,1)" },
	/* STMT_RECIPE_UPDATE */
	{ .stmt = (char *)"UPDATE recipes SET title=?,snippet=?,mtime=?,content=? WHERE slug=?" },
	/* STMT_RECIPE_LIST */
	{ .stmt = (char *)"SELECT " POST " FROM recipes ORDER BY ctime DESC" },
	/* STMT_COCKTAIL_GET */
	{ .stmt = (char *)"SELECT id,title,slug,image,ctime,mtime,serve,garnish,drinkware,method FROM cocktails WHERE slug=?" },
	/* STMT_COCKTAIL_LIST */
	{ .stmt = (char *)"SELECT id,title,slug,image,ctime,mtime,serve,garnish,drinkware,method FROM cocktails ORDER BY title ASC" },
	/* STMT_COCKTAIL_INGREDIENTS */
	{ .stmt = (char *)"SELECT id,name,measure,unit FROM cocktail_ingredients WHERE cocktail_id=? ORDER BY id ASC" },
	};

size_t file_slurp(char const *filename, char **into)
	{
	FILE *fh;
	struct stat details;
	size_t file_length = 0;

	if ((fh = fopen(filename, "rb")) != NULL)
		{
		if (fstat(fileno(fh), &details) == 0)
			if ((file_length = details.st_size) != 0)
				{
				*into = (char *)malloc(file_length + 1);
				(*into)[file_length] = '\0';
				if (fread(*into, details.st_size, 1, fh) != 1)
					{
					free(*into);
					file_length = 0;
					}
				}
		fclose(fh);
		}

	return file_length;
	}

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

void
db_post_free(struct post *);

struct post_array *
post_array_new()
	{
	struct post_array *p = kmalloc(sizeof(struct post_array));
	p->capacity = 256;
	p->length = 0;
	p->store = kmalloc(p->capacity * sizeof(struct post *));
	return p;
	}

void
post_array_append(struct post_array *p, struct post *post)
	{
	if (p->length == p->capacity)
		{
		p->capacity *= 2;
		p->store = krealloc(p->store, p->capacity * sizeof(struct post *));
		}
	p->store[p->length] = post;
	p->length++;
	}

/* also frees stored posts */
void
post_array_free(struct post_array *p)
	{
	for (size_t i = 0; i < p->length; i++)
		db_post_free(p->store[i]);
	free(p->store);
	free(p);
	}

void
db_post_free(struct post *p)
	{
	if (p == NULL)
		return;
	free(p->title);
	free(p->slug);
	free(p->snippet);
	free(p->content);
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
struct post_array *
db_post_list(struct sqlbox *p, size_t dbid, size_t stmt)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, stmt, 0, NULL, 0)))
		errx(EXIT_FAILURE, "sqlbox_prepare_bind");

	struct post_array *posts = post_array_new();

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
		post_array_append(posts, post);
		}

	// no results
	if (posts->length == 0)
		{
		post_array_free(posts);
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
				khtml_puts(data->req, data->cocktail->ingredients->store[i]->name);
				khtml_puts(data->req, " ");
				khtml_puts(data->req, data->cocktail->ingredients->store[i]->measure);
				khtml_puts(data->req, " ");
				khtml_puts(data->req, data->cocktail->ingredients->store[i]->unit);
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

	struct lowdown_opts opts;
	char *obuf;
	size_t obufsz;

	switch (key)
		{
		case (KEY_HEAD_TITLE):
			khtml_puts(data->req, data->head_title);
			break;
		case (KEY_NEXT_PAGE_LINK):
			break;
		case (KEY_LOGIN_LOGOUT_LINK):
			if (data->user == NULL)
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "login", KATTR__MAX);
				khtml_puts(data->req, "Login");
				khtml_closeelem(data->req, 1);
				}
			else
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "logout", KATTR__MAX);
				khtml_puts(data->req, "Logout");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_EDIT_CV_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_BR);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/editcv", KATTR__MAX);
				khtml_puts(data->req, "Edit CV");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_NEW_POST_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_BR);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/newpost", KATTR__MAX);
				khtml_puts(data->req, "New Post");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_EDIT_POST_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_BR);
				snprintf(buf, sizeof(buf), "/editpost/%s", data->post->slug);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
				khtml_puts(data->req, "Edit Post");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_EDIT_POST_PATH):
			if (data->user != NULL)
				{
				snprintf(buf, sizeof(buf), "/editpost/%s", data->post->slug);
				khtml_puts(data->req, buf);
				}
			break;
		case (KEY_NEW_RECIPE_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_BR);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/newrecipe", KATTR__MAX);
				khtml_puts(data->req, "New Recipe");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_EDIT_RECIPE_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_BR);
				snprintf(buf, sizeof(buf), "/editrecipe/%s", data->post->slug);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
				khtml_puts(data->req, "Edit Recipe");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_EDIT_RECIPE_PATH):
			if (data->user != NULL)
				{
				snprintf(buf, sizeof(buf), "/editrecipe/%s", data->post->slug);
				khtml_puts(data->req, buf);
				}
			break;
		case (KEY_TITLE):
			khtml_puts(data->req, data->post->title);
			break;
		case (KEY_SNIPPET):
			if (data->raw)
				khttp_puts(data->r, data->post->snippet);
			else
				khtml_puts(data->req, data->post->snippet);
			break;
		case (KEY_MTIME):
			{
			// TODO how do we get UTC?
			struct tm* tm_info = localtime(&data->post->mtime);
			strftime(buf, sizeof(buf), "%H:%M %d %b %Y", tm_info);
			khtml_puts(data->req, buf);
			break;
			}
		case (KEY_CONTENT):
			if (data->raw)
				khttp_puts(data->r, data->post->content);
			else
				{
				memset(&opts, 0, sizeof(struct lowdown_opts));
				opts.type = LOWDOWN_HTML;
				opts.feat = LOWDOWN_AUTOLINK
					| LOWDOWN_COMMONMARK
					| LOWDOWN_DEFLIST
					| LOWDOWN_FENCED
					| LOWDOWN_FOOTNOTES
					| LOWDOWN_HILITE
					| LOWDOWN_IMG_EXT
					| LOWDOWN_MATH
					| LOWDOWN_METADATA
					| LOWDOWN_STRIKE
					| LOWDOWN_SUPER
					| LOWDOWN_TABLES
					;
				opts.oflags = LOWDOWN_HTML_OWASP
					| LOWDOWN_HTML_NUM_ENT
					;

				lowdown_buf(&opts, data->post->content, strlen(data->post->content), &obuf, &obufsz, NULL);
				khttp_write(data->r, obuf, obufsz);
				}
			break;
		case (KEY_POSTS):
			if (data->posts == NULL)
				khtml_puts(data->req, "Nothing here yet");
			else
				{
				khtml_elem(data->req, KELEM_UL);
				// link to each post
				for (size_t i = 0; i < data->posts->length; i++)
					{
					khtml_elem(data->req, KELEM_LI);
					// TODO this seems hacky
					if (data->page == PAGE_BLOG)
						snprintf(buf, sizeof(buf), "/post/%s", data->posts->store[i]->slug);
					if (data->page == PAGE_RECIPES)
						snprintf(buf, sizeof(buf), "/recipe/%s", data->posts->store[i]->slug);
					khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
					khtml_puts(data->req, data->posts->store[i]->title);
					khtml_closeelem(data->req, 2);
					}
				khtml_closeelem(data->req, 1);
				}
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

			// TODO remimplement pagination
			// for (size_t i = 0 + data->page * 10; i < data->page * 10 + 10 && i < 1024 && data->cocktails->store[i] != NULL; i++)
			for (size_t i = 0; i < data->cocktails->length; i++)
				{
				d.cocktail = data->cocktails->store[i];
				khttp_template_buf(data->r, &t, tmpl_drink_snippet_data, tmpl_drink_snippet_size);
				}
			}
			break;
		default:
			abort();
		}

	return 1;
	}

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
	

static void
handle_index(struct kreq *r, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_INDEX;
	data.user = user;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_index_data, tmpl_index_size);
	}

static void
handle_cv(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_page_get(p, dbid, "cv");
	if (post == NULL)
		{
		open_response(r, KHTTP_500);
		khttp_puts(r, "500 Internal Server Error");
		return;
		}

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_CV;
	data.user = user;
	data.post = post;
	data.raw = 1;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_cv_data, tmpl_cv_size);
	}

static void
handle_edit_cv(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}


	if (r->method == KMETHOD_POST)
		{
		struct kpair *content;

		if ((content = r->fieldmap[PARAM_CONTENT]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}

		db_page_update(p, dbid, "cv", content->val);

		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/cv");
		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		struct post *post = db_page_get(p, dbid, "cv");
		if (post == NULL)
			{
			open_response(r, KHTTP_200);
			khttp_puts(r, "404 Not Found");
			return;
			}

		memset(&data, 0, sizeof(struct tmpl_data));
		data.page = PAGE_EDIT_CV;
		data.user = user;
		data.post = post;
		data.raw = 1;

		open_response(r, KHTTP_200);
		open_template(&data, &t, &hr, r);
		khttp_template_buf(r, &t, tmpl_editcv_data, tmpl_editcv_size);
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_blog(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_BLOG;
	data.user = user;
	data.posts = db_post_list(p, dbid, STMT_POST_LIST);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_blog_data, tmpl_blog_size);
	}

static void
handle_post(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_POST_GET, r->path);
	if (post == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_POST;
	data.user = user;
	data.post = post;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_post_data, tmpl_post_size);
	}

static void
handle_new_post(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}


	if (r->method == KMETHOD_POST)
		{
		struct kpair *title, *slug, *snippet, *content;

		if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
		    (slug = r->fieldmap[PARAM_SLUG]) == NULL ||
		    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
		    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}

		db_post_new(p, dbid, STMT_POST_NEW, title->val, slug->val, snippet->val, content->val);

		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/blag");
		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		memset(&data, 0, sizeof(struct tmpl_data));
		data.page = PAGE_NEW_POST;
		data.user = user;

		open_response(r, KHTTP_200);
		open_template(&data, &t, &hr, r);
		khttp_template_buf(r, &t, tmpl_newpost_data, tmpl_newpost_size);
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_edit_post(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}


	if (r->method == KMETHOD_POST)
		{
		struct kpair *title, *snippet, *content;

		if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
		    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
		    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}

		db_post_update(p, dbid, STMT_POST_UPDATE, title->val, r->path, snippet->val, content->val);

		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/post/%s", r->path);
		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		struct post *post = db_post_get(p, dbid, STMT_POST_GET, r->path);
		if (post == NULL)
			{
			open_response(r, KHTTP_200);
			khttp_puts(r, "404 Not Found");
			return;
			}

		memset(&data, 0, sizeof(struct tmpl_data));
		data.page = PAGE_EDIT_POST;
		data.user = user;
		data.post = post;
		data.raw = 1;

		open_response(r, KHTTP_200);
		open_template(&data, &t, &hr, r);
		khttp_template_buf(r, &t, tmpl_editpost_data, tmpl_editpost_size);
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
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

static void
handle_cocktails(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	char *drink;
	if ((drink = strstr(r->path, "drinks/")) != NULL)
		return handle_cocktail(r, p, dbid, &r->path[strlen("drinks/")]);

	struct post post;
	memset(&post, 0, sizeof(struct post));
	post.title = "All Drinks";

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_COCKTAILS;
	data.head_title = "All Drinks";
	data.post = &post;
	data.cocktails = db_cocktail_list(p, dbid);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_drinks_list_data, tmpl_drinks_list_size);
	}

static void
handle_recipes(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_RECIPES;
	data.user = user;
	data.posts = db_post_list(p, dbid, STMT_RECIPE_LIST);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_recipes_data, tmpl_recipes_size);
	}

static void
handle_new_recipe(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}


	if (r->method == KMETHOD_POST)
		{
		struct kpair *title, *slug, *snippet, *content;

		if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
		    (slug = r->fieldmap[PARAM_SLUG]) == NULL ||
		    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
		    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}

		db_post_new(p, dbid, STMT_RECIPE_NEW, title->val, slug->val, snippet->val, content->val);

		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/recipes");
		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		memset(&data, 0, sizeof(struct tmpl_data));
		data.page = PAGE_NEW_RECIPE;
		data.user = user;

		open_response(r, KHTTP_200);
		open_template(&data, &t, &hr, r);
		khttp_template_buf(r, &t, tmpl_newrecipe_data, tmpl_newrecipe_size);
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_edit_recipe(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}


	if (r->method == KMETHOD_POST)
		{
		struct kpair *title, *snippet, *content;

		if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
		    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
		    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}

		db_post_update(p, dbid, STMT_RECIPE_UPDATE, title->val, r->path, snippet->val, content->val);

		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/recipe/%s", r->path);
		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		struct post *post = db_post_get(p, dbid, STMT_RECIPE_GET, r->path);
		if (post == NULL)
			{
			open_response(r, KHTTP_200);
			khttp_puts(r, "404 Not Found");
			return;
			}

		memset(&data, 0, sizeof(struct tmpl_data));
		data.page = PAGE_EDIT_RECIPE;
		data.user = user;
		data.post = post;
		data.raw = 1;

		open_response(r, KHTTP_200);
		open_template(&data, &t, &hr, r);
		khttp_template_buf(r, &t, tmpl_editrecipe_data, tmpl_editrecipe_size);
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_recipe(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_RECIPE_GET, r->path);
	if (post == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "404 Not Found");
		return;
		}

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_RECIPE;
	data.user = user;
	data.post = post;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_recipe_data, tmpl_recipe_size);
	}

static void
handle_games(struct kreq *r)
	{
	open_response(r, KHTTP_200);
	khttp_puts(r, "Not implemented yet");
	}

static void
handle_login(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	int64_t		cookie;
	char		buf[64];

	memset(&data, 0, sizeof(struct tmpl_data));
	data.user = user;

	if (r->method == KMETHOD_POST)
		{
		struct kpair *username, *password;

		if ((username = r->fieldmap[PARAM_USERNAME]) == NULL ||
		    (password = r->fieldmap[PARAM_PASSWORD]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "400 Bad Request");
			return;
			}
		
		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/");

		struct user *user = db_user_get(p, dbid, username->val, password->val);
		if (user != NULL)
			{
			cookie = arc4random();
			db_sess_new(p, dbid, cookie, user);
			khttp_epoch2str(time(NULL) + 60 * 60 * 24 * 365, buf, sizeof(buf));
			khttp_head(r, kresps[KRESP_SET_COOKIE], "s=%" PRId64 "; HttpOnly; path=/; expires=%s", cookie, buf);
			}

		khttp_body(r);
		}
	else if (r->method == KMETHOD_GET)
		{
		open_response(r, KHTTP_200);
		if (user != NULL)
			khttp_puts(r, "Already logged in");
		else
			{
			open_template(&data, &t, &hr, r);
			khttp_template_buf(r, &t, tmpl_login_data, tmpl_login_size);
			}
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_logout(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	char buf[64];

	if (user == NULL)
		{
		open_response(r, KHTTP_200);
		khttp_puts(r, "Not logged in");
		return;
		}

	db_sess_del(p, dbid, NULL != r->cookiemap[PARAM_SESSCOOKIE] ? r->cookiemap[PARAM_SESSCOOKIE]->parsed.i : -1);
	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/");
	khttp_epoch2str(0, buf, sizeof(buf));
	khttp_head(r, kresps[KRESP_SET_COOKIE], "s=; HttpOnly; path=/; expires=%s", buf);
	khttp_body(r);
	}

int
main(void)
	{
	// DB init

	size_t dbid;
	struct sqlbox *p;
	struct sqlbox_cfg cfg;

	if (pledge("stdio rpath cpath wpath flock proc fattr", NULL) == -1)
		err(EXIT_FAILURE, "pledge");

	memset(&cfg, 0, sizeof(struct sqlbox_cfg));
	cfg.msg.func_short = warnx;
	cfg.srcs.srcs = srcs;
	cfg.srcs.srcsz = 1;
	cfg.stmts.stmts = pstmts;
	cfg.stmts.stmtsz = STMT__MAX;

	if ((p = sqlbox_alloc(&cfg)) == NULL)
		errx(EXIT_FAILURE, "sqlbox_alloc");
	if (!(dbid = sqlbox_open(p, 0)))
		errx(EXIT_FAILURE, "sqlbox_open");

	// TODO restrictively pledge at this point

	// KCGI start

	kutil_openlog(LOGFILE);

	struct kreq r;

	if (khttp_parse(&r, params, PARAM__MAX, pages, PAGE__MAX, PAGE_INDEX) != KCGI_OK)
		errx(EXIT_FAILURE, "khttp_parse");

	struct user *u = db_sess_get(p, dbid, NULL != r.cookiemap[PARAM_SESSCOOKIE] ? r.cookiemap[PARAM_SESSCOOKIE]->parsed.i : -1);

	switch (r.page)
		{
		case (PAGE_INDEX):
			handle_index(&r, u);
			break;
		case (PAGE_CV):
			handle_cv(&r, p, dbid, u);
			break;
		case (PAGE_EDIT_CV):
			handle_edit_cv(&r, p, dbid, u);
			break;
		case (PAGE_BLOG):
			handle_blog(&r, p, dbid, u);
			break;
		case (PAGE_POST):
			handle_post(&r, p, dbid, u);
			break;
		case (PAGE_NEW_POST):
			handle_new_post(&r, p, dbid, u);
			break;
		case (PAGE_EDIT_POST):
			handle_edit_post(&r, p, dbid, u);
			break;
		case (PAGE_COCKTAILS):
			handle_cocktails(&r, p, dbid);
			break;
		case (PAGE_RECIPES):
			handle_recipes(&r, p, dbid, u);
			break;
		case (PAGE_RECIPE):
			handle_recipe(&r, p, dbid, u);
			break;
		case (PAGE_NEW_RECIPE):
			handle_new_recipe(&r, p, dbid, u);
			break;
		case (PAGE_EDIT_RECIPE):
			handle_edit_recipe(&r, p, dbid, u);
			break;
		case (PAGE_GAMES):
			handle_games(&r);
			break;
		case (PAGE_LOGIN):
			handle_login(&r, p, dbid, u);
			break;
		case (PAGE_LOGOUT):
			handle_logout(&r, p, dbid, u);
			break;
		default:
			open_response(&r, KHTTP_404);
			khttp_puts(&r, "404 Not Found");
		}

	// FREE

	khttp_free(&r);
	if (!sqlbox_close(p, dbid))
		errx(EXIT_FAILURE, "sqlbox_close");
	sqlbox_free(p);

	return 0;
	}
