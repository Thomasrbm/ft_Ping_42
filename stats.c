#include "ping.h"


void print_ping_prompt(uint8_t *target_ip, char *hostname, t_flags *flags)
{
    size_t   payload_size = flags->has_packetsize ? (size_t)flags->packetsize_value : DEFAULT_PAYLOAD_SIZE;

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, target_ip, ip_str, INET_ADDRSTRLEN);
    printf("PING %s (%s) %zu(%zu) bytes of data.\n",
        hostname, ip_str, payload_size, payload_size + ICMP_HDR_SIZE + 20);
}

void print_stats(t_stats *stats, char *hostname)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    long elapsed_ms = (end.tv_sec - stats->start_time.tv_sec) * 1000L
                    + (end.tv_usec - stats->start_time.tv_usec) / 1000L;

    int loss = 0;
    // produit en crois par rapport au transmitted  pour avoir le ratio de perdues en pourcentage
    if (stats->transmitted > 0)
        loss = (stats->transmitted - stats->received) * 100 / stats->transmitted;

    printf("--- %s ping statistics ---\n", hostname);
    printf("%d packets transmitted, %d received, %d%% packet loss, time %ldms\n",
           stats->transmitted, stats->received, loss, elapsed_ms);

    if (stats->rtt_count > 0)
    {
        // moyenne
        double avg = stats->rtt_sum / stats->rtt_count;
        // mdev = ecart-type des RTT : sqrt(moyenne(x^2) - moyenne(x)^2) = la valeur typique des ecarts (standar deviation)
        // donc racine carre de la variance qui elle est valeur qui indique l etallement des valeur par rapport a la moyenne
        double variance = stats->rtt_sum_sq / stats->rtt_count - avg * avg;
        if (variance < 0)
            variance = 0;
        double mdev = ft_sqrt(variance);
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
               stats->rtt_min, avg, stats->rtt_max, mdev);
    }
}
