#ifndef COCKTAILS_H
#define COCKTAILS_H

void
handle_cocktail(struct kreq *r, struct sqlbox *p, size_t dbid, char *drink);

void
handle_cocktails(struct kreq *r, struct sqlbox *p, size_t dbid);

#endif
