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
#include <md5.h>
#include <lowdown.h>
#include <sqlbox.h>
#include <kcgi.h>
#include <kcgihtml.h>

#include "base64.h"
#include "db.h"
#include "dynarray.h"
#include "cocktails.h"
#include "file.h"
#include "shared.h"
#include "templates.h"

// TODO use the parsed values of kpair instead of the raw ones
// TODO check if fieldmap gets set when no fields are in request

#define LOGFILE "vaughan.kitchen.log"

enum page
	{
	PAGE_INDEX,
	PAGE_IMAGES,
	PAGE_NEW_IMAGE,
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
	PAGE_ROOMS,
	PAGE_CONNECT4,
	PAGE_CHINESE_CHESS,
	PAGE_LOGIN,
	PAGE_LOGOUT,
	PAGE__MAX
	};

const char *const pages[PAGE__MAX] =
	{
	"index",
	"images",
	"newimage",
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
	"rooms",
	"connect4",
	"chinese-chess",
	"login",
	"logout",
	};

struct tmpl_data
	{
	struct kreq           *r;
	struct khtmlreq       *req;
	enum page              page;
	struct user           *user;
	struct post           *post;
	struct dynarray     *posts;
	struct dynarray    *images;
	int                    raw; /* don't render markdown */
	};

enum key
	{
	KEY_LOGIN_LOGOUT_LINK,
	KEY_IMAGES_LINK,
	KEY_NEW_IMAGE_LINK,
	KEY_IMAGES,
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
	KEY__MAX,
	};

static const char *keys[KEY__MAX] =
	{
	"login-logout-link",
	"images-link",
	"new-image-link",
	"images",
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
	};

struct sqlbox_src srcs[] =
	{
	{ .fname = (char *)"db/db.db", .mode = SQLBOX_SRC_RW },
	};

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
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/login", KATTR__MAX);
				khtml_puts(data->req, "Login");
				khtml_closeelem(data->req, 1);
				}
			else
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/logout", KATTR__MAX);
				khtml_puts(data->req, "Logout");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_IMAGES_LINK):
			if (data->user != NULL)
				{
				khtml_elem(data->req, KELEM_LI);
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/images", KATTR__MAX);
				khtml_puts(data->req, "Images");
				khtml_closeelem(data->req, 2);
				}
			break;
		case (KEY_NEW_IMAGE_LINK):
			if (data->user != NULL)
				{
				khtml_attr(data->req, KELEM_A, KATTR_HREF, "/newimage", KATTR__MAX);
				khtml_puts(data->req, "New Image");
				khtml_closeelem(data->req, 1);
				}
			break;
		case (KEY_IMAGES):
			if (data->user != NULL && data->images != NULL)
				{
				khtml_elem(data->req, KELEM_UL);
				for (size_t i = 0; i < data->images->length; i++)
					{
					khtml_elem(data->req, KELEM_LI);
					snprintf(buf, sizeof(buf), "/static/img/%s.jpg", ((struct image *)data->images->store[i])->hash);
					khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
					khtml_puts(data->req, ((struct image *)data->images->store[i])->hash);
					khtml_closeelem(data->req, 2);
					}
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
						snprintf(buf, sizeof(buf), "/post/%s", ((struct post *)data->posts->store[i])->slug);
					if (data->page == PAGE_RECIPES)
						snprintf(buf, sizeof(buf), "/recipe/%s", ((struct post *)data->posts->store[i])->slug);
					khtml_attr(data->req, KELEM_A, KATTR_HREF, buf, KATTR__MAX);
					khtml_puts(data->req, ((struct post *)data->posts->store[i])->title);
					khtml_closeelem(data->req, 1);
					if (((struct post *)data->posts->store[i])->image != NULL)
						{
						khtml_elem(data->req, KELEM_BR);
						snprintf(buf, sizeof(buf), "/static/img/%s.jpg", ((struct post *)data->posts->store[i])->image);
						khtml_attr(data->req, KELEM_IMG, KATTR_SRC, buf, KATTR_WIDTH, "200", KATTR__MAX);
						}
					khtml_closeelem(data->req, 1);
					}
				khtml_closeelem(data->req, 1);
				}
			break;
		default:
			abort();
		}

	return 1;
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
handle_images(struct kreq *r, struct sqlbox *p, size_t dbid, struct user *user)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	// Not logged in
	if (user == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_IMAGES;
	data.user = user;
	data.images = db_image_list(p, dbid);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_images_data, tmpl_images_size);
	}

static void
handle_get_new_image(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_NEW_IMAGE;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_newimage_data, tmpl_newimage_size);
	}

static void
handle_post_new_image(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	char buf[2048];
	MD5_CTX md5;
	u_char digest[MD5_DIGEST_STRING_LENGTH + 1];

	struct kpair *title, *alt, *attribution, *image;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (alt = r->fieldmap[PARAM_ALT]) == NULL ||
	    (attribution = r->fieldmap[PARAM_ATTRIBUTION]) == NULL ||
	    (image = r->fieldmap[PARAM_IMAGE]) == NULL)
		return send_400(r);

	// TODO check jpg is valid
	// TODO remove EXIF

	MD5Init(&md5);
	MD5Update(&md5, image->val, image->valsz);
	MD5Final(digest, &md5);
	char *hash;
	if ((hash = base64buf_url(digest, MD5_DIGEST_LENGTH)) == NULL)
		errx(1, "error encoding base64");

	db_image_new(p, dbid, title->val, alt->val, attribution->val, hash, "jpg");

	// TODO delete DB entry if write fails
	snprintf(buf, sizeof(buf), "static/img/%s.jpg", hash);
	file_spurt(buf, image->val, image->valsz);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/images");
	khttp_body(r);
	}

static void
handle_cv(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_page_get(p, dbid, "cv");
	if (post == NULL)
		return send_500(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_CV;
	data.post = post;
	data.raw = 1;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_cv_data, tmpl_cv_size);
	}

static void
handle_get_edit_cv(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_page_get(p, dbid, "cv");
	if (post == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_EDIT_CV;
	data.post = post;
	data.raw = 1;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_editcv_data, tmpl_editcv_size);
	}

static void
handle_post_edit_cv(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct kpair *content;

	if ((content = r->fieldmap[PARAM_CONTENT]) == NULL)
		return send_400(r);

	db_page_update(p, dbid, "cv", content->val);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/cv");
	khttp_body(r);
	}

static void
handle_blog(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_BLOG;
	data.posts = db_post_list(p, dbid, STMT_POST_LIST);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_blog_data, tmpl_blog_size);
	}

static void
handle_post(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_POST_GET, r->path);
	if (post == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_POST;
	data.post = post;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_post_data, tmpl_post_size);
	}

static void
handle_get_new_post(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_NEW_POST;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_newpost_data, tmpl_newpost_size);
	}

static void
handle_post_new_post(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct kpair *title, *slug, *snippet, *content;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (slug = r->fieldmap[PARAM_SLUG]) == NULL ||
	    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
	    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
		return send_400(r);

	db_post_new(p, dbid, STMT_POST_NEW, title->val, slug->val, snippet->val, content->val);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/blag");
	khttp_body(r);
	}

static void
handle_get_edit_post(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_POST_GET, r->path);
	if (post == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_EDIT_POST;
	data.post = post;
	data.raw = 1;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_editpost_data, tmpl_editpost_size);
	}

static void
handle_post_edit_post(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct kpair *title, *snippet, *content;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
	    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
		return send_400(r);

	db_post_update(p, dbid, STMT_POST_UPDATE, title->val, r->path, snippet->val, content->val);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/post/%s", r->path);
	khttp_body(r);
	}

static void
handle_recipes(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_RECIPES;
	data.posts = db_post_list(p, dbid, STMT_RECIPE_LIST);

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_recipes_data, tmpl_recipes_size);
	}

static void
handle_get_new_recipe(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_NEW_RECIPE;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_newrecipe_data, tmpl_newrecipe_size);
	}

static void
handle_post_new_recipe(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct kpair *title, *slug, *snippet, *content;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (slug = r->fieldmap[PARAM_SLUG]) == NULL ||
	    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
	    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
		return send_400(r);

	db_post_new(p, dbid, STMT_RECIPE_NEW, title->val, slug->val, snippet->val, content->val);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/recipes");
	khttp_body(r);
	}

static void
handle_get_edit_recipe(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_RECIPE_GET, r->path);
	if (post == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_EDIT_RECIPE;
	data.post = post;
	data.raw = 1;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_editrecipe_data, tmpl_editrecipe_size);
	}

static void
handle_post_edit_recipe(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct kpair *title, *snippet, *content;

	if ((title = r->fieldmap[PARAM_TITLE]) == NULL ||
	    (snippet = r->fieldmap[PARAM_SNIPPET]) == NULL ||
	    (content = r->fieldmap[PARAM_CONTENT]) == NULL)
		return send_400(r);

	db_post_update(p, dbid, STMT_RECIPE_UPDATE, title->val, r->path, snippet->val, content->val);

	open_head(r, KHTTP_302);
	khttp_head(r, kresps[KRESP_LOCATION], "/recipe/%s", r->path);
	khttp_body(r);
	}

static void
handle_recipe(struct kreq *r, struct sqlbox *p, size_t dbid)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	struct post *post = db_post_get(p, dbid, STMT_RECIPE_GET, r->path);
	if (post == NULL)
		return send_404(r);

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_RECIPE;
	data.post = post;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_recipe_data, tmpl_recipe_size);
	}

static void
handle_games(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_GAMES;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_games_data, tmpl_games_size);
	}

static void
handle_rooms(struct kreq *r)
	{
	char buf[2048];
	snprintf(buf, sizeof(buf), "rooms/%s", r->path);

	// TODO check request size (httpd already does some of that for us)
	if (r->fields != NULL)
		file_spurt(buf, r->fields->val, r->fields->valsz);

	char *data;
	size_t file_length = 0;
	file_length = file_slurp(buf, &data);

	if (file_length == 0)
		return send_404(r);

	open_response(r, KHTTP_200);
	khttp_write(r, data, file_length);
	}

static void
handle_connect4(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_CONNECT4;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_connect4_data, tmpl_connect4_size);
	}

static void
handle_chinese_chess(struct kreq *r)
	{
	struct tmpl_data data;
	struct ktemplate t;
	struct khtmlreq hr;

	memset(&data, 0, sizeof(struct tmpl_data));
	data.page = PAGE_CHINESE_CHESS;

	open_response(r, KHTTP_200);
	open_template(&data, &t, &hr, r);
	khttp_template_buf(r, &t, tmpl_chinese_chess_data, tmpl_chinese_chess_size);
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
			return send_400(r);
		
		open_head(r, KHTTP_302);
		khttp_head(r, kresps[KRESP_LOCATION], "/");

		struct user *user = db_user_checkpass(p, dbid, username->val, password->val);
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
		return send_405(r);
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
		case (PAGE_IMAGES):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			handle_images(&r, p, dbid, u);
			break;
		case (PAGE_NEW_IMAGE):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_new_image(&r);
			else if (r.method == KMETHOD_POST)
				handle_post_new_image(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_CV):
			handle_cv(&r, p, dbid);
			break;
		case (PAGE_EDIT_CV):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_edit_cv(&r, p, dbid);
			else if (r.method == KMETHOD_POST)
				handle_post_edit_cv(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_BLOG):
			handle_blog(&r, p, dbid);
			break;
		case (PAGE_POST):
			handle_post(&r, p, dbid);
			break;
		case (PAGE_NEW_POST):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_new_post(&r);
			else if (r.method == KMETHOD_POST)
				handle_post_new_post(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_EDIT_POST):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_edit_post(&r, p, dbid);
			else if (r.method == KMETHOD_POST)
				handle_post_edit_post(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_COCKTAILS):
			handle_cocktails(&r, p, dbid, u);
			break;
		case (PAGE_RECIPES):
			handle_recipes(&r, p, dbid);
			break;
		case (PAGE_RECIPE):
			handle_recipe(&r, p, dbid);
			break;
		case (PAGE_NEW_RECIPE):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_new_recipe(&r);
			else if (r.method == KMETHOD_POST)
				handle_post_new_recipe(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_EDIT_RECIPE):
			if (u == NULL)
				{
				send_404(&r);
				break;
				}
			if (r.method == KMETHOD_GET)
				handle_get_edit_recipe(&r, p, dbid);
			else if (r.method == KMETHOD_POST)
				handle_post_edit_recipe(&r, p, dbid);
			else
				send_405(&r);
			break;
		case (PAGE_GAMES):
			handle_games(&r);
			break;
		case (PAGE_ROOMS):
			handle_rooms(&r);
			break;
		case (PAGE_CONNECT4):
			handle_connect4(&r);
			break;
		case (PAGE_CHINESE_CHESS):
			handle_chinese_chess(&r);
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
