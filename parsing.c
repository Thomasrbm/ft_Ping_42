#include "ping.h"

int check_flags(char *curr_av, t_flags *flags)
{
    if (ft_strcmp(curr_av, "-V") == 0)
        flags->has_version = 1;
    else if (ft_strcmp(curr_av, "-v") == 0)
        flags->has_verbose = 1;
    else if (ft_strcmp(curr_av, "-n") == 0)
        flags->has_numeric = 1;
    else if (ft_strcmp(curr_av, "-q") == 0)
        flags->has_quiet = 1;
    else if (ft_strcmp(curr_av, "-w") == 0)
        flags->has_deadline = 1;
    else if (ft_strcmp(curr_av, "-W") == 0)
        flags->has_timeout = 1;
    else if (ft_strcmp(curr_av, "-s") == 0)
        flags->has_packetsize = 1;
    else if (ft_strcmp(curr_av, "-i") == 0)
        flags->has_interval = 1;
    else if (ft_strcmp(curr_av, "-c") == 0)
        flags->has_count = 1;
    else if (ft_strcmp(curr_av, "-?") == 0)
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

    // tous les flags sont compatibles
    if (ac < 2 || ac > 17)
    {
        printf("ft_ping: usage error: Destination address required\n");
        printf("Usage: ./ft_ping <target_ip / DNS>\n");
        printf("Flags: ./ft_ping [-?] for more details\n");
        return 0;
    }
    int i = 1;
    while (i < ac && av[i][0] == '-')
    {
        if (!check_flags(av[i], flags))
        {
            printf("Usage: ./ft_ping [-V] [-v] [-n] [-q] [-w] [-W] [-s] [-i] [-c] [-?] <target_ip / DNS>\n");
            printf("Usage: ./ft_ping [-?] for more details\n");
            return 0;
        }
        i++;
    }
    if (ac - i != 1)
    {
        printf("Usage: ./ft_ping [-V] [-v] [-n] [-q] [-w] [-W] [-s] [-i] [-c] [-?] <target_ip / DNS>\n");
        return 0;
    }
    *arg_offset = i;
    return 1;
}