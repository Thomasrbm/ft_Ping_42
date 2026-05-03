

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

#include <sys/socket.h>
#include <netinet/in.h>   // struct sockaddr_in, IPPROTO_ICMP, htons/htonl (parfois)
#include <arpa/inet.h>    // inet_pton(), inet_ntop(), htons, ntohs
#include <netdb.h>

#include <sys/time.h>

int set_socket(t_flags *flags)
{
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socket_fd < 0)
        return -1;


    int ttl = flags->has_ttl ? flags->ttl_value : 64;
    if (setsockopt(socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        close(socket_fd);
        dprintf(2, "error");
        return -1;
    }

    if (flags->has_ignore_router)
    {
        int one = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_DONTROUTE, &one, sizeof(one)) < 0)
        {
            dprintf(2, "error");
            return -1;
        }
    }

    struct timeval tv;
    tv.tv_sec = flags->has_timeout ? flags->timeout_value : 1;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        dprintf(2, "error");
        return -1;
    }


    return socket_fd;
}