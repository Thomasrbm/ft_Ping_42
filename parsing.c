#include "ping.h"

static void print_usage(void)
{
    dprintf(2, "Usage: ./ft_ping [-V] [-v] [-n] [-q] [-w sec] [-W sec]"
               " [-s size] [-i sec] [-c count] [-?] <target_ip / DNS>\n");
}

static int parse_long_flag_value(int ac, char **av, int *i, long *flag_value, char *name_of_flag, long max)
{
    if (*i + 1 >= ac) // pas de val
    {
        dprintf(2, "ft_ping: option '%s' requires an argument\n", name_of_flag);
        print_usage();
        return 0;
    }
    if (!ft_isnumber(av[*i + 1])) // val pas nombre
    {
        dprintf(2, "ft_ping: invalid argument: '%s' for option '%s'"
                   " (positive integer required)\n", av[*i + 1], name_of_flag);
        return 0;
    }
    errno = 0;
    long v = ft_strtol(av[*i + 1], NULL); // to long
    if (errno == ERANGE || v > max)
    {
        if (ft_strcmp(name_of_flag, "-W") == 0)
            dprintf(2, "ft_ping: bad linger time: %s\n", av[*i + 1]);
        else if (ft_strcmp(name_of_flag, "-i") == 0)
            dprintf(2, "ft_ping: bad timing interval: %s\n", av[*i + 1]);
        else
            dprintf(2, "ft_ping: invalid argument: '%s': Numerical result out of range\n", av[*i + 1]);
        return 0;
    }
    if (v <= 0)
    {
        dprintf(2, "ft_ping: invalid argument: '%s' for option '%s'"
                   " (must be > 0)\n", av[*i + 1], name_of_flag);
        return 0;
    }
    *flag_value = v;
    *i += 1;
    return 1;
}

// wrapper int : pour les flags stockes en int dans t_flags (le cast est safe car max <= INT_MAX)
static int parse_value_flag_int_wrapper(int ac, char **av, int *i, int *flag_value, char *name_of_flag, long max)
{
    long v = 0;
    if (!parse_long_flag_value(ac, av, i, &v, name_of_flag, max))
        return 0;
    *flag_value = (int)v;
    return 1;
}

// 1 = ok, 0 = unknown flag, -1 = known flag mais valeur invalide (deja loggee)
static int check_flags(int ac, char **av, int *i, t_flags *flags)
{
    if (ft_strcmp(av[*i], "-V") == 0)
        flags->has_version = 1;
    else if (ft_strcmp(av[*i], "-v") == 0)
        flags->has_verbose = 1;
    else if (ft_strcmp(av[*i], "-n") == 0)
        flags->has_numeric = 1;
    else if (ft_strcmp(av[*i], "-q") == 0)
        flags->has_quiet = 1;
    else if (ft_strcmp(av[*i], "-w") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->deadline_value, "-w", MAX_DEADLINE))
            return -1;
        flags->has_deadline = 1;
    }
    else if (ft_strcmp(av[*i], "-W") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->timeout_value, "-W", MAX_TIMEOUT))
            return -1;
        flags->has_timeout = 1;
    }
    else if (ft_strcmp(av[*i], "-s") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->packetsize_value, "-s", MAX_PACKET_SIZE))
            return -1;
        flags->has_packetsize = 1;
    }
    else if (ft_strcmp(av[*i], "-i") == 0)
    {
        if (!parse_value_flag_int_wrapper(ac, av, i, &flags->interval_value, "-i", MAX_INTERVAL))
            return -1;
        flags->has_interval = 1;
    }
    else if (ft_strcmp(av[*i], "-c") == 0) // ne passe pas par le wrapper int
    {
        if (!parse_long_flag_value(ac, av, i, &flags->count_value, "-c", MAX_COUNT))
            return -1;
        flags->has_count = 1;
    }
    else if (ft_strcmp(av[*i], "-?") == 0)
        flags->has_help = 1;
    else
        return 0;
    return 1;
}

int parsing(int ac, char **av, int *arg_offset, t_flags *flags)
{
    flags->has_version = 0;
    flags->has_verbose = 0;
    flags->has_numeric = 0;
    flags->has_quiet = 0;
    flags->has_deadline = 0;
    flags->has_timeout = 0;
    flags->has_packetsize = 0;
    flags->has_interval = 0;
    flags->has_count = 0;
    flags->has_help = 0;
    flags->deadline_value = 0;
    flags->timeout_value = 0;
    flags->packetsize_value = 0;
    flags->interval_value = 0;
    flags->count_value = 0;

    if (ac < 2)
    {
        dprintf(2, "ft_ping: usage error: Destination address required\n");
        print_usage();
        return 0;
    }
    int i = 1;
    while (i < ac && av[i][0] == '-')
    {
        int ret = check_flags(ac, av, &i, flags);
        if (ret == 0)
        {
            dprintf(2, "ft_ping: invalid option: '%s'\n", av[i]);
            print_usage();
            return 0;
        }
        if (ret == -1) // bon flag mais mauvaise value pour -c -w etc. (autre msg gere avant)
            return 0;
        i++;
    }
    if (!flags->has_help && !flags->has_version && ac - i != 1) // seul -v et -? accepte de pas avoir d ip ou trop de param
    {
        dprintf(2, "ft_ping: usage error: Destination address required\n");
        print_usage();
        return 0;
    }
    *arg_offset = i;
    return 1;
}
