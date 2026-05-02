#include "ping.h"


int setup_socket(t_flags *flags)
{
    int socket_fd;

    // pas besoin de bind car af inet laisse le raw etre en layer 3 donc il fait le header et le bind
    // af packet en malcolm et layer 2 donc pas de header fait. donc pas de routage auto donc bind necessaire.
    // dans irc af inet pourrait le faire auto mais on doit choisir un port precis car on choisit cote user donc on force le bind precis
    socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP); //AF_INET : on attend que ipv4,  SOCK_RAW : on build le rest et kernel fait header ip,
    // IPPROTO_ICMP interecepte que les packet icmp  ip : code 1. 
    if (socket_fd < 0)
    {
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;  
    }

    int ttl = 64; // valeur flag pour  time to leave
    // ttl = compteur de 64 essai maximum dans un boucle de routeur (si + = routeur mal config)
    // donc on leave car boucle infini surement (routeur a pour google va voir b, b voir c c voir a etc)
    if (setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) // option socket layer ip (3) : set la ttl
    {
        // IPPROTO_IP layer ip
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;
    }
    // option a l envoie 


    // flag -W timeout value.
    // si pas le -W met valeur a 1 sec
    struct timeval tv;
    tv.tv_sec = flags->has_timeout ? flags->timeout_value : 1;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) // option socket global  SO_RCVTIMEO flag qui dit on touche au timeout. 
    {
        dprintf(2, "socket: %s\n", strerror(errno));
        return -1;
    }
    // option au receive pour les recv ft etc. pour pouvoir timeout si les ping reviennent pas.
    // si ping 1 bug et 2 revient avant lui le seq prevent ca.

    // -v : afficher les infos de socket
    if (flags->has_verbose)
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), sock6.fd: -1 (socktype: 0), hints.ai_family: AF_UNSPEC\n", socket_fd);
    return socket_fd;
}
