#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

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

// Return pointer to end of prefix in str
char *string_prefix(char *pre, char *str)
	{
	while (*pre)
		if (*pre++ != *str++)
			return NULL;
	return str;
	}

// Return pointer to start of suffix in str
char *string_suffix(char *post, char *str)
	{
	size_t post_len = strlen(post);
	size_t str_len = strlen(str);
	if (str_len < post_len)
		return NULL;
	char *save = str + (str_len - post_len);
	str = save;
	while (*str)
		if (*post++ != *str++)
			return NULL;
	return save;
	}

// TODO refactor this
// TODO write test cases for this
char **string_split(char *sep, char *str)
	{
	size_t result_count = 0;
	size_t result_length = 16;
	char **result = malloc(sizeof(char **) * (result_length + 1)); // NULL terminated
	char *save = str;
	while (*str)
		{
		size_t i = 0;
		while (sep[i])
			if (sep[i] == str[i])
				i++;
			else
				break;
		if (sep[i] == '\0')
			{
			if (result_count == result_length)
				{
				result_length *= 2;
				result = realloc(result, sizeof(char **) * (result_length + 1));
				}
			if (str != save)
				result[result_count++] = strndup(save, str - save);
			str += i;
			save = str;
			continue;
			}
		str++;
		}
	if (*save)
		{
		if (result_count == result_length)
			{
			result_length *= 2;
			result = realloc(result, sizeof(char **) * (result_length + 1));
			}
		result[result_count++] = strdup(save);
		}
	result[result_count] = NULL;
	return result;
	}

// TODO refactor this
void string_tolower(char *str)
	{
	for (unsigned long i = 0; i < strlen(str); i++)
		if (isalpha(str[i]))
			str[i] = tolower(str[i]);
	}
