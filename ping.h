#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h> 

#define DEFAULT_PAYLOAD_SIZE 56
#define ICMP_HDR_SIZE        sizeof(struct icmphdr)

// -V => prend le dessus en 2e 

// '-?'   => prend le dessus sur tous  + entre quote sur zsh

typedef struct s_flags
{
    int     has_version;    // -V

    int     has_verbose;    // -v 
    int     has_numeric;    // -n
    int     has_quiet;      // -q
    int     has_deadline;   // -w
    int     has_timeout;    // -W
    int     has_packetsize; // -s
    int     has_interval;   // -i
    int     has_count;      // -c

    int     has_help;       // -?

    int     deadline_value;       // -w : valeur du deadline global (en sec)
    int     timeout_value;        // -W : valeur du timeout par reply
    int     packetsize_value;     // -s : taille du payload en octets
    int     interval_value;       // -i : intervalle (en sec)
    int     count_value;          // -c : nombre de paquets a send


}   t_flags;


// struct icmphdr {
//     __u8   type;        // 8 pour Echo Request, 0 pour Echo Reply
//     __u8   code;        // 0
//     __sum16 checksum;
//     union {
//         struct {
//             __be16 id; // pid du process sousvent
//             __be16 sequence; // compteur incremente a chaque paquet sent
//         } echo;
//         __be32 gateway; // pour redirect
//         struct {  // si framentation needed
//             __be16 __unused;
//             __be16 mtu;
//         } frag;
//     } un;
// };


// struct sockaddr_in {
//     sa_family_t    sin_family;    // toujours AF_INET pour IPv4
//     in_port_t      sin_port;      // port (inutile pour ICMP)
//     struct in_addr sin_addr;      // l'IP destination
//     char           sin_zero[8];   // padding, à mettre à zéro
// };

// struct iphdr {
//     unsigned int ihl:4;       // taille du header 4 octets
//     unsigned int version:4;   // 4 pour IPv4
//     uint8_t      tos;
//     uint16_t     tot_len;
//     uint16_t     id;
//     uint16_t     frag_off;
//     uint8_t      ttl;
//     uint8_t      protocol;    // 1 = ICMP
//     uint16_t     check;
//     uint32_t     saddr;       // IP source
//     uint32_t     daddr;       // IP destination
// };

int parsing(int ac, char **av, int *arg_offset, t_flags *flags);

void handle_flags(t_flags *flags);

int icmp(t_flags *flags, uint8_t *target_ip);

int ft_strcmp(const char *s1, const char *s2);
void *ft_memset(void *s, int c, size_t n);
void *ft_memcpy(void *dst, const void *src, size_t n);
int ft_isnumber(const char *s);
