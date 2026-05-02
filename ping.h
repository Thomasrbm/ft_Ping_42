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
#include <arpa/inet.h>
#include <limits.h>

#define MAX_PACKET_SIZE 65507    // -s : 65535 - 20 (IP hdr) - 8 (ICMP hdr)
#define MAX_DEADLINE    INT_MAX  // -w
#define MAX_TIMEOUT     4294     // -W : UINT_MAX / 1000000 (cap iputils en us)
#define MAX_INTERVAL    2147483  // -i : INT_MAX / 1000 (cap iputils en ms)
#define MAX_COUNT       LONG_MAX // -c

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
    int     has_deadline;   // -w   max 2147483647 = INT MAX
    int     has_timeout;    // -W   max 4294 = UINT_MAX / 1000000 (cap iputils en us)
    int     has_packetsize; // -s   max 65507 =   65535 - 20 (IP hdr) - 8 (ICMP hdr)
    int     has_interval;   // -i   max 2147483 = INT_MAX / 1000 (cap iputils en ms)
    int     has_count;      // -c   max 9223372036854775807  =  LONG_MAX

    int     has_help;       // -?

    int     deadline_value;       // -w : valeur du deadline global (en sec)
    int     timeout_value;        // -W : valeur du timeout par reply
    int     packetsize_value;     // -s : taille du payload en octets
    int     interval_value;       // -i : intervalle (en sec)
    long    count_value;          // -c : nombre de paquets a send


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


// stats accumulees pendant la session, affichees a la fin (--- ping statistics ---)
typedef struct s_stats
{
    int             transmitted;     // paquets envoyes
    int             received;        // replies valides
    int             rtt_count;       // nb de RTT valides (payload >= 16)
    double          rtt_min;
    double          rtt_max;
    double          rtt_sum;
    double          rtt_sum_sq;
    struct timeval  start_time;
}   t_stats;


typedef struct s_reply
{
    // Header IP
    uint8_t  ttl;             // TTL du paquet reçu (à afficher)
    
    // Header ICMP
    uint8_t  type;            // ICMP_ECHOREPLY 0
    uint8_t  code;            // code 0
    uint16_t id;              // deja converti via ntohs
    uint16_t seq;             // same
    
    // Tailles
    size_t   icmp_size;       // taille totale ICMP (header + payload)
    size_t   payload_size;    // taille du payload uniquement
    
    // Timestamp d'envoi
    struct timeval send_time;
    int            has_timestamp; // 1 si payload_size >= 16, sinon 0
}   t_reply;

int parsing(int ac, char **av, int *arg_offset, t_flags *flags);

void handle_flags(t_flags *flags);

int icmp(t_flags *flags, uint8_t *target_ip, char *hostname, int socket_fd);
int receive_reply(int sockfd, uint16_t seq, t_flags *flags, t_stats *stats);
uint16_t parse_error_original_seq(uint8_t *reply_buffer, ssize_t buffer_len);
void print_stats(t_stats *stats, char *hostname);

int setup_socket(t_flags *flags);
unsigned short compute_checksum(void *data, int len);
uint8_t *build_packet(t_flags *flags, uint16_t seq, size_t *packet_size);
int send_packet(uint8_t *target_ip, int socket_fd, void *icmp_packet, size_t packet_size);
void print_ping_prompt(uint8_t *target_ip, char *hostname, t_flags *flags);
void print_verbose_error(struct sockaddr_in *from_ip, t_reply *reply_struc, uint16_t orig_seq, t_flags *flags);
void display_reply(t_reply *reply_struc, struct sockaddr_in *from_ip, t_flags *flags, double rtt_ms);

int ft_strcmp(char *s1, char *s2);
void *ft_memset(void *s, int c, size_t n);
void *ft_memcpy(void *dst, const void *src, size_t n);
int ft_isnumber(char *s);
long ft_strtol(const char *s, char **endptr);
double ft_sqrt(double x);
