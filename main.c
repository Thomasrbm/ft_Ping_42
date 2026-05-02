#include "ping.h"

int get_ip(char **av, int *arg_offset, uint8_t *target_ip, t_flags *flags)
{
    struct addrinfo hints; // filtre ipv4
    struct addrinfo *res;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    char *ip = av[*arg_offset];

    // "active -n" si input en numerique (8.8.8.8), le printera tel quel
    struct in_addr tmp;
    if (inet_pton(AF_INET, ip, &tmp) == 1) // string to ip decimal (si lettre etc marche pas)
        flags->has_numeric = 1;

    if (getaddrinfo(ip, NULL, &hints, &res)) // autant decimal que dns
    {
        printf("invalid ip adress\n");
        return 0;
    }
    ft_memcpy(target_ip, &((struct sockaddr_in *)res->ai_addr)->sin_addr, 4);  // de base ai_addr est sockaddr  generique: ip v4 cast avec sockaddr_in
    
    // -v : afficher les infos de resolution
    if (flags->has_verbose)
        printf("ping: ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", ip);
    freeaddrinfo(res);
    return 1;
}

int main(int ac, char **av)
{
    int arg_offset;
    t_flags flags;
    uint8_t target_ip[4];
    int socket_fd;

    if (!parsing(ac, av, &arg_offset, &flags))
        return 1;
    if (flags.has_help || flags.has_version)
    {
        handle_flags(&flags);
        return 0;
    }
    socket_fd = setup_socket(&flags);
    if (socket_fd < 0)
        return 1;
    if (!get_ip(av, &arg_offset, target_ip, &flags))
        return 1;
    icmp(&flags, target_ip, av[arg_offset], socket_fd);
    return 0;
}
