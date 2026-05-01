#include "ping.h"

int ft_isnumber(char *s)
{
    if (!s || !*s)
        return 0;
    if (*s == '+')
        s++;
    if (!*s)
        return 0;
    while (*s)
    {
        if (!isdigit((unsigned char)*s))
            return 0;
        s++;
    }
    return 1;
}

int ft_strcmp(char *s1, char *s2)
{
	size_t i;

	i = 0;
	while (s1[i] && s1[i] == s2[i])
		i++;
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

void *ft_memset(void *s, int c, size_t n)
{
	unsigned char *ptr;
	size_t         i;

	ptr = (unsigned char *)s;
	i = 0;
	while (i < n)
		ptr[i++] = (unsigned char)c;
	return (s);
}

void *ft_memcpy(void *dst, const void *src, size_t n)
{
	unsigned char       *d;
	const unsigned char *s;
	size_t               i;

	d = (unsigned char *)dst;
	s = (const unsigned char *)src;
	i = 0;
	while (i < n)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}
