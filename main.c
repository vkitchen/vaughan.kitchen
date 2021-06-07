#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> /* pledge */
#include <err.h> /* err */
#include <sys/types.h> /* size_t, ssize_t */
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

enum page
	{
	PAGE_INDEX,
	PAGE_CV,
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
	struct kreq       *r;
	struct khtmlreq   *req;
	enum page          page;
	struct user       *user;
	struct post       *post;
	struct post_array *posts;
	int                raw; /* don't render markdown */
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
	KEY_LOGIN_LOGOUT_LINK,
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
	KEY__MAX,
	};

const char *keys[KEY__MAX] =
	{
	"login-logout-link",
	/* post */
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
	STMT_CV_GET,
	STMT_CV_UPDATE,
	STMT_POST_GET,
	STMT_POST_NEW,
	STMT_POST_UPDATE,
	STMT_POST_LIST,
	STMT_RECIPE_GET,
	STMT_RECIPE_NEW,
	STMT_RECIPE_UPDATE,
	STMT_RECIPE_LIST,
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
	/* STMT_CV_GET */
	{ .stmt = (char *)"SELECT mtime,content FROM cv WHERE id=1" },
	/* STMT_CV_UPDATE */
	{ .stmt = (char *)"UPDATE cv SET mtime=?,content=? WHERE id=1" },
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
	};

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
db_cv_get(struct sqlbox *p, size_t dbid)
	{
	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_CV_GET, 0, NULL, 0)))
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
db_cv_update(struct sqlbox *p, size_t dbid, char *content)
	{
	time_t mtime = time(NULL);

	struct sqlbox_parm parms[] =
		{
		{ .iparm = mtime, .type = SQLBOX_PARM_INT },
		{ .sparm = content, .type = SQLBOX_PARM_STRING },
		};

	size_t stmtid;
	if (!(stmtid = sqlbox_prepare_bind(p, dbid, STMT_CV_UPDATE, 2, parms, 0)))
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
			khtml_int(data->req, data->post->mtime);
			break;
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
	khttp_template(r, &t, "tmpl/index.html");
	}

static void
handle_cv(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_cv_get(p, dbid);
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
	khttp_template(r, &t, "tmpl/cv.html");
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
	khttp_template(r, &t, "tmpl/blog.html");
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
	khttp_template(r, &t, "tmpl/post.html");
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
		khttp_template(r, &t, "tmpl/newpost.html");
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
		khttp_template(r, &t, "tmpl/editpost.html");
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "405 Method Not Allowed");
		}
	}

static void
handle_cocktails(struct kreq *r)
	{
	open_response(r, KHTTP_200);
	khttp_puts(r, "Not implemented yet");
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
	khttp_template(r, &t, "tmpl/recipes.html");
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
		khttp_template(r, &t, "tmpl/newrecipe.html");
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
		khttp_template(r, &t, "tmpl/editrecipe.html");
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
	khttp_template(r, &t, "tmpl/recipe.html");
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
			khttp_template(r, &t, "tmpl/login.html");
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
			handle_cocktails(&r);
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
