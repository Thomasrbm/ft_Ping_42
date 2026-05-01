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
    return socket_fd;
}


// valeur qui depend de toutes les autres valeur du packet
// verifier a l arriver sur aucune data n a ete corrompue en cours de route

// d habitude en sock_stream  le kernel le fait pour nous
// en raw le kernell fait que le header ip
unsigned short compute_checksum(void *data, int len)
{
    unsigned short *buf = data;
    unsigned int    sum = 0;

    // add tous les mots de 16 bits
    while (len > 1)
    {
        sum += *buf;
        buf++;
        len -= 2;
    }

    // si len  impaire, ajouter le dernier octet
    if (len == 1)
        sum += *(unsigned char *)buf;

    // on a add des  nombre de 16 bit mais peuvent depasser sur 32
    // si result trop grand

    // replier les 16 bits hauts dans les 16 bits bas
    // pour faire un su de 16bit a la fin
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);  // au cas où le repli a généré un nouveau carry

    //  Complément à 1 = inversion de tous les bit 
    return ~sum;
}


int icmp(t_flags *flags, uint8_t *target_ip)
{
    int socket_fd;

    (void)target_ip;
    socket_fd = setup_socket(flags);
    if (socket_fd < 0)
        return 0;
    handle_flags(flags);


    struct icmphdr;



    return 1;
}