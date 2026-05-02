#include "ping.h"
#include <limits.h>

// errno Erange (errno sorte de global implicite)
long ft_strtol(const char *s, char **endptr)
{
    long result = 0;
    int  sign = 1;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '+')
        s++;
    else if (*s == '-')
    {
        sign = -1;
        s++;
    }
    while (*s >= '0' && *s <= '9')
    {
        int digit = *s - '0';
        if (result > (LONG_MAX - digit) / 10)
        {
            errno = ERANGE;
            while (*s >= '0' && *s <= '9')
                s++;
            if (endptr)
                *endptr = (char *)s;
            return (sign == 1) ? LONG_MAX : LONG_MIN;
        }
        result = result * 10 + digit;
        s++;
    }
    if (endptr)
        *endptr = (char *)s;
    return result * sign;
}

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

// méthode de Héron
// x = 9, s = 9 (estimation initiale)
// itér 1 : s = (9 + 9/9) / 2     = 5
// itér 2 : s = (5 + 9/5) / 2     = 3.4
// itér 3 : s = (3.4 + 9/3.4) / 2 ≈ 3.0235
// itér 4 : s ≈ 3.00009
// itér 5 : s ≈ 3.0000000001
double ft_sqrt(double x)
{
    if (x <= 0)
        return 0;
    double s = x;
    for (int i = 0; i < 20; i++)
        s = 0.5 * (s + x / s); // x0.5 = div par 2
    return s;
}
