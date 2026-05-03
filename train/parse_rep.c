#include "t.h"


// on recoit [ Header IP : 20 octets ][ Header ICMP : 8 octets ][ Payload : N octets ]
int parse_reply(uint8_t *reply_buffer, ssize_t buffer_len, t_reply *reply_struc)
{
    struct iphdr   *ip_header;
    struct icmphdr *icmp_response;
    size_t          ip_header_size;


    ip_header = (struct iphdr *)reply_buffer;
    ip_header_size = ip_header->ihl * 4;

    if((ip_header_size +  ICMP_HDR_SIZE) > buffer_len)
        return 0;

    icmp_response = (struct icmphdr *)(reply_buffer + ip_header_size);



    reply_struc->ttl = ip_header->ttl;             // TTL du paquet reçu (à afficher)
    
    // Header ICMP
    reply_struc->type;            // ICMP_ECHOREPLY 0
    reply_struc->code;            // code 0
    reply_struc->id;              // deja converti via ntohs
    reply_struc->seq;             // same
    
    // Tailles
    reply_struc->icmp_size;       // taille totale ICMP (header + payload)
    reply_struc->payload_size;    // taille du payload uniquement
    
    // Timestamp d'envoi
    reply_struc->send_time;
    reply_struc->has_timestamp;



    reply_struc->inner_packet =;
    reply_struc->inner_len =;

    return 1;
}
