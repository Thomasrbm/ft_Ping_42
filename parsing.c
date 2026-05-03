#include "ping.h"

void print_try_help(void)
{
    dprintf(2, "Try 'ft_ping -?' for more information.\n");
}

int parse_long_flag_value(int ac, char **av, int *i, long *flag_value, char *name_of_flag, long min, long max)
{
    if (*i + 1 >= ac) // pas de val
    {
        dprintf(2, "ft_ping: option requires an argument -- '%c'\n", name_of_flag[1]);
        print_try_help();
        return 0;
    }
    if (!ft_isnumber(av[*i + 1])) // val pas nombre
    {
        dprintf(2, "ft_ping: invalid value (`%s' near `%s')\n", av[*i + 1], av[*i + 1]);
        return 0;
    }
    errno = 0;
    unsigned long v = strtoul(av[*i + 1], NULL, 10); // negatifs deja filtres par ft_isnumber
    if (errno == ERANGE || v > (unsigned long)max)
    {
        dprintf(2, "ft_ping: option value too big: %s\n", av[*i + 1]);
        return 0;
    }
    if (v < (unsigned long)min)
    {
        dprintf(2, "ft_ping: invalid value (`%s' near `%s')\n", av[*i + 1], av[*i + 1]);
        return 0;
    }
    *flag_value = (long)v;
    *i += 1;
    return 1;
}

// wrapper int : pour les flags stockes en int dans t_flags (le cast est safe car max <= INT_MAX)
int parse_value_flag_int_wrapper(int ac, char **av, int *i, int *flag_value, char *name_of_flag, long min, long max)
{
    long v = 0;
    if (!parse_long_flag_value(ac, av, i, &v, name_of_flag, min, max))
        return 0;
    *flag_value = (int)v;
    return 1;
}

//   v == 0           -> option value too small
//   v < 0 / v > 255  -> option value too big (negatif lu en unsigned)
//   non-numerique    -> invalid value
int parse_ttl(int ac, char **av, int *i, t_flags *flags)
{
    char *value_str;
    int   need_next_val; // si on a =

    if (strncmp(av[*i], "--ttl=", 6) == 0)
    {
        value_str = av[*i] + 6;
        need_next_val = 0;
    }
    else
    {
        if (*i + 1 >= ac)
        {
            dprintf(2, "ft_ping: option '--ttl' requires an argument\n");
            print_try_help();
            return 0;
        }
        value_str = av[*i + 1];
        need_next_val = 1;
    }
    if (value_str[0] == '-') // negatif lu comme unsigned
    {
        dprintf(2, "ft_ping: option value too big: %s\n", value_str);
        return 0;
    }
    if (!ft_isnumber(value_str))
    {
        dprintf(2, "ft_ping: invalid value (`%s' near `%s')\n", value_str, value_str);
        return 0;
    }
    errno = 0;
    unsigned long v = strtoul(value_str, NULL, 10);
    if (errno == ERANGE || v > MAX_TTL)
    {
        dprintf(2, "ft_ping: option value too big: %s\n", value_str);
        return 0;
    }
    if (v < 1)
    {
        dprintf(2, "ft_ping: option value too small: %s\n", value_str);
        return 0;
    }
    flags->ttl_value = (int)v;
    flags->has_ttl = 1;
    *i += need_next_val;
    return 1;
}

static int parse_count_value(int ac, char **av, int *i, t_flags *flags)
{
    if (*i + 1 >= ac)
    {
        dprintf(2, "ft_ping: option requires an argument -- 'c'\n");
        print_try_help();
        return 0;
    }
    char *value = av[*i + 1];
    char *p = value;
    if (*p == '+' || *p == '-')
        p++;
    if (!*p) // que le signe, rien apres
    {
        dprintf(2, "ft_ping: invalid value (`%s' near `%s')\n", value, value);
        return 0;
    }
    while (*p)
    {
        if (!isdigit((unsigned char)*p))
        {
            dprintf(2, "ft_ping: invalid value (`%s' near `%s')\n", value, value);
            return 0;
        }
        p++;
    }
    errno = 0;
    flags->count_value = (long)strtoul(value, NULL, 10); // si plus que ULONG max il renvoit ulong max et set errno
    flags->has_count = 1;
    *i += 1;
    return 1;
}

// 1 = ok, 0 = unknown flag, -1 = known flag mais valeur invalide (deja loggee)
int check_flags(int ac, char **av, int *i, t_flags *flags)
{
    if (strcmp(av[*i], "-V") == 0)
        flags->has_version = 1;
    else if (strcmp(av[*i], "-v") == 0)
        flags->has_verbose = 1;
    else if (strcmp(av[*i], "-q") == 0)
        flags->has_quiet = 1;
    else if (strcmp(av[*i], "-r") == 0)
        flags->has_ignore_router = 1;
    else if (strcmp(av[*i], "--ttl") == 0 || strncmp(av[*i], "--ttl=", 6) == 0)
    {
        if (!parse_ttl(ac, av, i, flags))
            return -1;
    }
    else if (strcmp(av[*i], "-w") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->deadline_value, "-w", 1, MAX_DEADLINE))
            return -1;
        flags->has_deadline = 1;
    }
    else if (strcmp(av[*i], "-W") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->timeout_value, "-W", 1, MAX_TIMEOUT))
            return -1;
        flags->has_timeout = 1;
    }
    else if (strcmp(av[*i], "-s") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->packetsize_value, "-s", 0, MAX_PACKET_SIZE))
            return -1;
        flags->has_packetsize = 1;
    }
    else if (strcmp(av[*i], "-c") == 0)
    {
        if (!parse_count_value(ac, av, i, flags))
            return -1;
    }
    else if (strcmp(av[*i], "-?") == 0)
        flags->has_help = 1;
    else
        return 0;
    return 1;
}

int parsing(int ac, char **av, int *arg_offset, t_flags *flags)
{
    flags->has_version = 0;
    flags->has_verbose = 0;
    flags->has_quiet = 0;
    flags->has_deadline = 0;
    flags->has_timeout = 0;
    flags->has_packetsize = 0;
    flags->has_count = 0;
    flags->has_ttl = 0;
    flags->has_ignore_router = 0;
    flags->has_help = 0;
    flags->deadline_value = 0;
    flags->timeout_value = 0;
    flags->packetsize_value = 0;
    flags->count_value = 0;
    flags->ttl_value = 0;

    if (ac < 2)
    {
        dprintf(2, "ft_ping: missing host operand\n");
        print_try_help();
        return 0;
    }
    int i = 1;
    while (i < ac && av[i][0] == '-')
    {
        int ret = check_flags(ac, av, &i, flags);
        if (ret == 0)
        {
            dprintf(2, "ft_ping: invalid option -- '%c'\n", av[i][1]);
            print_try_help();
            return 0;
        }
        if (ret == -1) // bon flag mais mauvaise value pour -c -w etc. (autre msg gere avant)
            return 0;
        i++;
    }
    if (!flags->has_help && !flags->has_version && ac - i != 1) // seul -v et -? accepte de pas avoir d ip ou trop de param
    {
        dprintf(2, "ft_ping: missing host operand\n");
        print_try_help();
        return 0;
    }
    *arg_offset = i;
    return 1;
}
