#include <stddef.h>
#include <stdint.h>

size_t my_strlen(const char *s);
void *my_memset(void *dest, int c, size_t n);
char *my_strchrnul(const char *s, int c);
size_t my_strspn(const char *s, const char *c);
size_t my_strcspn(const char *s, const char *c);
char *my_strtok(char *restrict s, const char *restrict sep);
char *my_strcpy(char *restrict dest, const char *restrict src);
