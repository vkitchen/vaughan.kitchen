#include "shared.h"

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "external/incbin/incbin.h"

/* A_data, A_end, A_size */
INCBIN(tmpl_blog, "tmpl/blog.html");
INCBIN(tmpl_connect4, "tmpl/connect4.html");
INCBIN(tmpl_chinese_chess, "tmpl/chinese-chess.html");
INCBIN(tmpl_cv, "tmpl/cv.html");
INCBIN(tmpl_drink, "tmpl/drink.html");
INCBIN(tmpl_drink_snippet, "tmpl/drink_snippet.html");
INCBIN(tmpl_drinks_list, "tmpl/drinks_list.html");
INCBIN(tmpl_editcv, "tmpl/editcv.html");
INCBIN(tmpl_editpost, "tmpl/editpost.html");
INCBIN(tmpl_editrecipe, "tmpl/editrecipe.html");
INCBIN(tmpl_games, "tmpl/games.html");
INCBIN(tmpl_index, "tmpl/index.html");
INCBIN(tmpl_login, "tmpl/login.html");
INCBIN(tmpl_newpost, "tmpl/newpost.html");
INCBIN(tmpl_newrecipe, "tmpl/newrecipe.html");
INCBIN(tmpl_post, "tmpl/post.html");
INCBIN(tmpl_recipe, "tmpl/recipe.html");
INCBIN(tmpl_recipes, "tmpl/recipes.html");

const struct kvalid params[PARAM__MAX] =
	{
	{ kvalid_stringne, "username" },
	{ kvalid_stringne, "password" },
	{ kvalid_uint, "s" },
	{ kvalid_stringne, "q" },
	{ kvalid_uint, "page" },
	/* post */
	{ kvalid_stringne, "title" },
	{ kvalid_stringne, "slug" },
	{ kvalid_stringne, "snippet" },
	{ kvalid_stringne, "content" },
	};

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
