#include "ping.h"


int setup_socket(t_flags *flags)
{
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socket_fd < 0)
    {
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;
    }

    int ttl = 64; // valeur flag pour  time to leave
    // ttl = compteur de 64 essai maximum dans un boucle de routeur (si + = routeur mal config)
    // donc on leave car boucle infini surement (routeur a pour google va voir b, b voir c c voir a etc)
    if (setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        // IPPROTO_IP layer ip
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;
    }


    // flag -W timeout value.
    // si pas le -W met valeur a 1 sec
    struct timeval tv;
    tv.tv_sec = flags->has_timeout ? flags->timeout_value : 1;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;

    }
    // -v : afficher les infos de socket
    if (flags->has_verbose)
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), sock6.fd: -1 (socktype: 0), hints.ai_family: AF_UNSPEC\n", socket_fd);
    return socket_fd;
}
