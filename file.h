#ifndef FILE_H
#define FILE_H

size_t
file_slurp(char const *filename, char **into);

size_t
file_spurt(char const *filename, char *buffer, size_t bytes);

#endif
