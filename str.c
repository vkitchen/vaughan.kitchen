#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "str.h"

char *string_cat(size_t num_params, ...)
	{
	va_list args;
	va_start(args, num_params);

	char *result = NULL;
	size_t result_len = 0;
	for (size_t i = 0; i < num_params; i++)
		{
		char *str = va_arg(args, char *);
		if (result == NULL)
			{
			result_len = strlen(str);
			result = malloc(result_len + 1);
			strlcpy(result, str, result_len + 1);
			}
		else
			{
			size_t append_start = result_len;
			result_len += strlen(str);
			result = realloc(result, result_len + 1);
			strlcpy(&result[append_start], str, result_len + 1);
			}
		}
	return result;
	}

char *string_prefix(char *pre, char *str)
	{
	while (*pre)
		if (*pre++ != *str++)
			return NULL;
	return str;
	}
