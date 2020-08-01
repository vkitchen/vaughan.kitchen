#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

const char *token = "";

struct chunk {
	char *data;
	size_t length;
};

char *string_cat(size_t num_params, ...)
	{
	size_t i = 0;
	char *result = NULL;
	size_t result_len = 0;
	va_list args;

	va_start(args, num_params);

	for (i = 0; i < num_params; i++)
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

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
	{
	size_t realsize = size * nmemb;
	struct chunk *chunk = (struct chunk *)userp;

	char *ptr = realloc(chunk->data, chunk->length + realsize + 1);
	if (ptr == NULL)
		{
		fprintf(stderr, "ERROR: Allocation failed during download\n");
		return 0;
		}

	chunk->data = ptr;
	memcpy(&(chunk->data[chunk->length]), contents, realsize);
	chunk->length += realsize;
	chunk->data[chunk->length] = '\0';

	return realsize;
	}

int main(void)
	{
	CURL *curl;
	CURLcode res;
	struct chunk chunk;

	// char *url = string_cat(3, "https://api.pinboard.in/v1/posts/all?auth_token=", token, "&tag=music");
	char *url = string_cat(3, "https://api.pinboard.in/v1/posts/get?auth_token=", token, "&dt=2020-07-27&tag=music");

	chunk.data = malloc(1);
	chunk.data[0] = '\0';
	chunk.length = 0;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();

	if (!curl)
		{
		fprintf(stderr, "ERROR: Curl failed\n");
		curl_global_cleanup();
		return 1;
		}

	printf("GET: %s\n", url);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl/7.69.1");

	res = curl_easy_perform(curl);

	if (res != CURLE_OK)
		{
		fprintf(stderr, "ERROR: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		goto CLEANUP;
		}

	puts(chunk.data);

CLEANUP:
	curl_easy_cleanup(curl);

	return 0;
	}
