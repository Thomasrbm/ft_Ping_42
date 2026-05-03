

#include <stdint.h>

typedef struct s_flags
{
    int     has_version;    // -V

    int     has_verbose;    // -v ajoute id premier ligne + erreurs

    int     has_quiet;          // -q
    int     has_deadline;       // -w     min 1, max 2147483647 (INT_MAX)
    int     has_timeout;        // -W     min 1, max 2147483647 (INT_MAX)
    int     has_packetsize;     // -s     min 0, max 65399 (IP_MAXPACKET - MAXIPLEN - MAXICMPLEN)
    int     has_count;          // -c     0 ou negatif = infini, sinon nombre de paquets (long)
    int     has_ttl;            // --ttl  min 1, max 255
    int     has_ignore_router; // -r     boolean (SO_DONTROUTE)

    int     has_help;       // -?

    int     deadline_value;       // -w : valeur du deadline global (en sec)
    int     timeout_value;        // -W : valeur du timeout par reply
    int     packetsize_value;     // -s : taille du payload en octets
    long    count_value;          // -c : nombre de paquets a send
    int     ttl_value;            // --ttl : time-to-live IP


}   t_flags;

#include <netinet/ip_icmp.h>
#define ICMP_HDR_SIZE sizeof(struct icmphdr)  // = 8

#include <sys/time.h>
#include <stddef.h>

uint8_t *build_packet(t_flags *flags,  uint16_t seq, size_t *packet_size)
{

    size_t payload_size = flags->has_packetsize ? flags->packetsize_value : 56 ;
    size_t total_size = ICMP_HDR_SIZE + payload_size;


    uint8_t *packet = calloc(1, total_size);

    struct icmphdr *icmp_header = (struct icmphdr *)packet;

    icmp_header->type = 8;
    icmp_header->code = 0;
    icmp_header->checksum = 0;

    icmp_header->un.echo.id = htons(getpid() & 0xFFFF); //tronque le pid pour 16bit max
    icmp_header->un.echo.sequence = htons(seq);

    icmp_header->checksum = checksum(packet);

    if (payload_size >= sizeof(struct timeval))
    {
        struct timeval *start = (struct timeval *)(packet + ICMP_HDR_SIZE);
        gettimeofday(start, NULL);
    }

    *packet_size = total_size;
    return packet;
}

int main()
{
    void    *icmp_packet;
    t_flags *flags;
    uint16_t seq = 0; // 16 car le RFC du header imposte 16
    size_t packet_size;

    icmp_packet = build_packet(flags, seq, &packet_size);
    return 0;
}