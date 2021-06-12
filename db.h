#ifndef DB_H
#define DB_H

#include <sys/types.h> /* size_t, ssize_t */
#include <sqlbox.h>

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
