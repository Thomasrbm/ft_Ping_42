#include "ping.h"

int get_ip(char **av, int *arg_offset, uint8_t *target_ip)
{
    struct addrinfo hints; // filtre ipv4
    struct addrinfo *res;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    char *ip = av[*arg_offset];

    if (getaddrinfo(ip, NULL, &hints, &res)) // autant decimal que dns
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
    int socket_fd;

    // line-buffered : sinon stdout est block-buffered quand piped, et la
    // ligne 'PING ...' sort apres les erreurs stderr (ordre incoherent vs inet)
    setvbuf(stdout, NULL, _IOLBF, 0);

    // strtod respecte la locale pour le separateur decimal (',' fr_FR / '.' C)
    // inetutils fait pareil -> '-i 0,5' marche en fr_FR mais pas '-i 0.5'
    setlocale(LC_NUMERIC, "");

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
    if (!get_ip(av, &arg_offset, target_ip))
        return 1;
    icmp(&flags, target_ip, av[arg_offset], socket_fd);
    return 0;
}
