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
