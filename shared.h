#ifndef SHARED_H
#define SHARED_H

#include <sys/types.h> /* size_t, ssize_t */
#include <stdarg.h> /* va_list */
#include <sqlbox.h>
#include <kcgi.h>

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "external/incbin/incbin.h"

/* A_data, A_end, A_size */
INCBIN_EXTERN(tmpl_blog);
INCBIN_EXTERN(tmpl_connect4);
INCBIN_EXTERN(tmpl_chinese_chess);
INCBIN_EXTERN(tmpl_cv);
INCBIN_EXTERN(tmpl_drink);
INCBIN_EXTERN(tmpl_drink_snippet);
INCBIN_EXTERN(tmpl_drinks_list);
INCBIN_EXTERN(tmpl_editcv);
INCBIN_EXTERN(tmpl_editpost);
INCBIN_EXTERN(tmpl_editrecipe);
INCBIN_EXTERN(tmpl_games);
INCBIN_EXTERN(tmpl_index);
INCBIN_EXTERN(tmpl_login);
INCBIN_EXTERN(tmpl_newpost);
INCBIN_EXTERN(tmpl_newrecipe);
INCBIN_EXTERN(tmpl_post);
INCBIN_EXTERN(tmpl_recipe);
INCBIN_EXTERN(tmpl_recipes);

enum param
	{
	PARAM_USERNAME,
	PARAM_PASSWORD,
	PARAM_SESSCOOKIE,
	PARAM_QUERY,
	PARAM_TITLE,
	PARAM_SLUG,
	PARAM_SNIPPET,
	PARAM_CONTENT,
	PARAM__MAX,
	};

const struct kvalid params[PARAM__MAX];

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

struct sqlbox_pstmt pstmts[STMT__MAX];

#endif
