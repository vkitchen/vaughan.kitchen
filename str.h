#ifndef STR_H
#define STR_H

char *string_cat(size_t num_params, ...);
char *string_prefix(char *pre, char *str);
char *string_suffix(char *post, char *str);
char **string_split(char *sep, char *str);
void string_tolower(char *str);

#endif
