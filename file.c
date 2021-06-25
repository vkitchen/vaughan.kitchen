#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h> /* struct stat */

#include "file.h"

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

// TODO error detection
size_t
file_spurt(char const *filename, char *buffer, size_t bytes)
	{
	FILE *fh;
	fh = fopen(filename, "wb");
	fwrite(buffer, 1, bytes, fh);
	fclose(fh);
	return bytes;
	}

