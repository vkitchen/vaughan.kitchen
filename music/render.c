#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*
	FILE_SLURP()
	------------
*/
size_t
file_slurp(char const *filename, char **into)
	{
	FILE *fh;
	struct stat details;
	size_t file_length = 0;

	if ((fh = fopen(filename, "rb")) != NULL)
		{
		if (fstat(fileno(fh), &details) == 0)
			if ((file_length = details.st_size) != 0)
				{
				*into = (char *)malloc(file_length + 1);
				(*into)[file_length] = '\0';
				if (fread(*into, details.st_size, 1, fh) != 1)
					{
					free(*into);
					file_length = 0;
					}
				}
		fclose(fh);
		}

	return file_length;
	}

// Return pointer to end of prefix in str
char *
string_prefix(char *pre, char *str)
	{
	while (*pre)
		if (*pre++ != *str++)
			return NULL;
	return str;
	}

struct dynamic_array_size
	{
	uint32_t capacity;
	uint32_t length;
	size_t *store;
	};

static inline void
dynamic_array_size_init(struct dynamic_array_size *a)
	{
	a->capacity = 256;
	a->length = 0;
	a->store = malloc(a->capacity * sizeof(size_t));
	}

static inline void
dynamic_array_size_append(struct dynamic_array_size *a, size_t val)
	{
	if (a->length == a->capacity)
		{
		a->capacity *= 2;
		a->store = realloc(a->store, a->capacity * sizeof(size_t));
		}
	a->store[a->length] = val;
	a->length++;
	}

static inline size_t *
dynamic_array_size_back(struct dynamic_array_size *a)
	{
	return &a->store[a->length-1];
	}

struct post
	{
	char *href;
	char *time;
	char *title;
	char *description;
	};

char *
extract_key(char *file, size_t len, char *key)
	{
	char *res = NULL;
	for (size_t i = 0; i < len; i++)
		{
		if (file[i] == '\n')
			break;

		if (NULL != string_prefix(key, &file[i]))
			{
			while (file[i] != '"')
				i++;
			i+=1;
			size_t j = i;
			while (file[j] != '"')
				j++;

			res = malloc(j - i + 1);
			strlcpy(res, &file[i], j - i + 1);
			return res;
			}
		}
	return res;
	}

struct post *
extract_post(char *file, size_t len)
	{
	struct post *res = malloc(sizeof(struct post));
	res->href = extract_key(file, len, "href");
	res->time = extract_key(file, len, "time");
	res->title = extract_key(file, len, "description");
	res->description = extract_key(file, len, "extended");
	return res;
	}

void
post_print(struct post *post)
	{
	printf("href: %s\n", post->href);
	printf("time: %s\n", post->time);
	printf("title: %s\n", post->title);
	printf("description: %s\n", post->description);
	}

int
main(void)
	{
	char *file;
	size_t file_length;
	if (0 == (file_length = file_slurp("music.xml", &file)))
		{
		fprintf(stderr, "ERROR: Failed to read music file\n");
		return 1;
		}

	struct dynamic_array_size results;
	dynamic_array_size_init(&results);
	for (size_t i = 0; i < file_length; i++)
		{
		if (NULL != string_prefix("<post ", &file[i]))
			{
			struct post *res = extract_post(&file[i], file_length - i);
			dynamic_array_size_append(&results, (size_t)res);
			}
		}

	for (size_t i = 0; i < results.length; i++)
		post_print((struct post *)results.store[i]);

	return 0;
	}
