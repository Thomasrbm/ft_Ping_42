
#include "t.h"

int get_ip(char **av, int *off_set, uint8_t *ip)
{
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_ = AF_INET;
    if (!getaddrinfo(av[*off_set] , AF_INET, &hints, &res))
    {
        dprintf(2, "error");
        return 0;
    }
    memcpy(ip, &((struct sockaddr_in *)res->ai_addr)->sin_addr, 4);
    freeaddrinfo(res);
    return 1;
}