#include "ping.h"

int get_ip(char **av, int *arg_offset, uint8_t *target_ip)
{
    struct addrinfo hints;
    struct addrinfo *res;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    char *ip = av[*arg_offset];
    if (getaddrinfo(ip, NULL, &hints, &res))
    {
        printf("invalid ip adress\n");
        return 0;
    }
    ft_memcpy(target_ip, &((struct sockaddr_in *)res->ai_addr)->sin_addr, 4);  // de base ai_addr est sockaddr  generique: ip v4 cast avec sockaddr_in
    freeaddrinfo(res);
    return 1;
}

int main(int ac, char **av)
{
    int arg_offset;
    t_flags flags;
    uint8_t target_ip[4];

    if (!parsing(ac, av, &arg_offset, &flags))
        return 1;
    if (flags.has_help || flags.has_version)
    {
        handle_flags(&flags);
        return 0;
    }
    if (!get_ip(av, &arg_offset, target_ip))
        return 1;
    icmp(&flags);
    return 0;
}
