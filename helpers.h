#ifndef HELPERS_H
#define HELPERS_H

enum page {
	PAGE_INDEX,
	PAGE_COCKTAILS,
	PAGE_ROOMS,
	PAGE__MAX
};

enum param {
	PARAM_PAGE_NO,
	PARAM__MAX
};

void response_open(struct kreq *r, enum khttp http_status);
int serve_404(struct kreq *r);
int serve_500(struct kreq *r);
int serve_static(struct kreq *r);
int serve_static_encoded(struct kreq *r);

#endif
