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
#include <sqlbox.h>
#include <kcgi.h>
#include <kcgihtml.h>

// TODO use fastcgi
// TODO read all templates upon startup and then sandbox more heavily

#define LOGFILE "vaughan.kitchen.log"

struct user {
	int64_t    id;
	char      *username; /* user's login */
	char      *name;     /* user's display name */
};

enum page
	{
	PAGE_INDEX,
	PAGE_LOGIN,
	PAGE__MAX
	};

const char *const pages[PAGE__MAX] =
	{
	"index",
	"login",
	};

enum param
	{
	PARAM_USERNAME,
	PARAM_PASSWORD,
	PARAM_SESSCOOKIE,
	PARAM__MAX,
	};

const struct kvalid params[PARAM__MAX] =
	{
	{ kvalid_stringne, "username" },
	{ kvalid_stringne, "password" },
	{ kvalid_uint, "s" },
	};

enum key
	{
	KEY__MAX,
	};

const char *keys[KEY__MAX] =
	{
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
	STMT__MAX,
	};

#define USER "users.id,users.display,users.login"

struct sqlbox_pstmt pstmts[] =
	{
	{ .stmt = (char *)"SELECT " USER ",users.shadow FROM users WHERE login=?" },
	{ .stmt = (char *)"SELECT " USER " FROM sessions INNER JOIN users ON users.id = sessions.user_id WHERE sessions.cookie=?" },
	{ .stmt = (char *)"INSERT INTO sessions (cookie,user_id) VALUES (?,?)" },
	};

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

static int
template(size_t key, void *arg)
	{
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
	

static void
handle_index(struct kreq *r, struct user *user)
	{
	struct ktemplate t;

	memset(&t, 0, sizeof(struct ktemplate));

	t.key = keys;
	t.keysz = KEY__MAX;
	t.arg = NULL;
	t.cb = template;

	open_response(r, KHTTP_200);
	khttp_template(r, &t, "tmpl/index.html");
	}

static void
handle_login(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct ktemplate t;

	int64_t		cookie;
	char		buf[64];

	memset(&t, 0, sizeof(struct ktemplate));

	t.key = keys;
	t.keysz = KEY__MAX;
	t.arg = NULL;
	t.cb = template;


	if (r->method == KMETHOD_POST)
		{
		struct kpair *username, *password;

		if ((username = r->fieldmap[PARAM_USERNAME]) == NULL ||
		    (password = r->fieldmap[PARAM_PASSWORD]) == NULL)
			{
			open_response(r, KHTTP_400);
			khttp_puts(r, "Bad Request");
			return;
			}
		
		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/");

		struct user *user = db_user_get(p, dbid, r->fields[0].val, r->fields[1].val);
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
			khttp_template(r, &t, "tmpl/login.html");
		}
	else
		{
		open_head(r, KHTTP_405);
		khttp_puts(r, "Method Not Allowed");
		}
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
		case (PAGE_LOGIN):
			handle_login(&r, p, dbid, u);
			break;
		default:
			abort();
		}

	// FREE

	khttp_free(&r);
	if (!sqlbox_close(p, dbid))
		errx(EXIT_FAILURE, "sqlbox_close");
	sqlbox_free(p);

	return 0;
	}
