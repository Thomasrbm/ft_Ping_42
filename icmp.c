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


// build header icmp  + payload
uint8_t       *build_packet(t_flags *flags, uint16_t seq, size_t *packet_size)
{
    size_t payload_size;
    size_t total_size;

    // flag -s  pour taille precise ou non
    payload_size = flags->has_packetsize ? flags->packetsize_value : DEFAULT_PAYLOAD_SIZE;
    total_size = ICMP_HDR_SIZE + payload_size;

    uint8_t *packet;
    packet = calloc(1, total_size);
    if (!packet)
        return NULL;

    struct icmphdr *icmp_header = (struct icmphdr *)packet;

    icmp_header->type = ICMP_ECHO;     // 8 = request
    icmp_header->code = 0;              // pas de sous-type pour Echo
    icmp_header->checksum = 0;  // remplir apres

     // host to netword (little to big) x86 to protocol
    icmp_header->un.echo.id = htons(getpid() & 0xFFFF); // pid du echo current, pour distinguer plusieur ping parralleles
    icmp_header->un.echo.sequence = htons(seq); // identifie numero du paquet (dans la boucle d envoit de plusieur paquets)

    // RTT = temps_de_reception - temps_d_envoi = le temps de distance + vitesse reseau + le temps de traitement
    // ou aller + traitement + retour  ===> les ms
    if (payload_size >= sizeof(struct timeval)) // timeval fait 16 octet, on ne peut pas faire le RTT si payload trop petit. si ping -s0 ou -s5 temps sera valeur bidon
    {
        // Buffer :  [type][code][cksum][cksum][id][id][seq][seq] -> [?][?][?] avance la.
        struct timeval *send_time = (struct timeval *)(packet + ICMP_HDR_SIZE);
        gettimeofday(send_time, NULL); // y stock moment ou packet fait
    }

    icmp_header->checksum = compute_checksum(packet, total_size);
    *packet_size = total_size;
    return packet;
}

int send_packet(t_flags *flags, uint8_t *target_ip, int socket_fd, void *icmp_packet, size_t packet_size)
{
    struct sockaddr_in dest;

    (void)flags;
    ft_memset(&dest, 0, sizeof(dest));      // sin_zero à 0
    dest.sin_family = AF_INET;              //  IPv4
    dest.sin_port = 0;                       // icmp use pas port
    ft_memcpy(&dest.sin_addr, target_ip, 4); // copie ip cible

    if (sendto(socket_fd, icmp_packet, packet_size, 0,
            (struct sockaddr *)&dest, sizeof(dest)) < 0)
    {
        dprintf(2, "sendto: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


void print_ping_prompt(uint8_t *target_ip, char *hostname, t_flags *flags)
{
    size_t   payload_size = flags->has_packetsize ? (size_t)flags->packetsize_value : DEFAULT_PAYLOAD_SIZE;

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, target_ip, ip_str, INET_ADDRSTRLEN);
    printf("PING %s (%s) %zu(%zu) bytes of data.\n",
        hostname, ip_str, payload_size, payload_size + ICMP_HDR_SIZE + 20);
}


int icmp(t_flags *flags, uint8_t *target_ip, char *hostname)
{
    int      socket_fd;
    void    *icmp_packet;
    uint16_t seq = 1;
    size_t   packet_size = 0;

    
    socket_fd = setup_socket(flags);
    if (socket_fd < 0)
        return 0;

    print_ping_prompt(target_ip, hostname, flags);

    int interval = flags->has_interval ? flags->interval_value : 1; // -i
    int remaining = flags->has_count ? flags->count_value : -1; // -c . -1 = infini

    // boucle d envoit/reception : 1 paquet, attendre reponse, sleep, recommencer
    while (remaining != 0)
    {
        icmp_packet = build_packet(flags, seq, &packet_size);
        if (!icmp_packet)
            break;
        if (send_packet(flags, target_ip, socket_fd, icmp_packet, packet_size) == 0)
            receive_reply(socket_fd, seq);
        free(icmp_packet);

        seq++;
        if (remaining > 0) // si deja a -1 restera a -1 tout le temps
            remaining--;
        if (remaining == 0)
            break;

        sleep(interval);
    }

    close(socket_fd);
    return 1;
}