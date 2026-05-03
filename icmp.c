#include "ping.h"

// retourne 1 si la deadline -w est atteinte, 0 sinon
int deadline_reached(t_flags *flags, struct timeval *start)
{
    if (!flags->has_deadline)
        return 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - start->tv_sec) >= flags->deadline_value;
}

// sleep d au plus interval_ms millisecondes, decoupe en 100ms pour pouvoir sortir si deadline
void sleep_with_deadline(size_t interval_ms, t_flags *flags, struct timeval *start)
{
    size_t slices = interval_ms / 100; // slice de 100ms
    while (slices-- > 0)
    {
        if (deadline_reached(flags, start))
            return;
        usleep(100000);
    }
    size_t leftover_ms = interval_ms % 100; // < 100ms restants (cas 0,2 -> 200ms = 2 slices, leftover 0)
    if (leftover_ms && !deadline_reached(flags, start))
        usleep(leftover_ms * 1000);
}

int icmp(t_flags *flags, uint8_t *target_ip, char *hostname, int socket_fd)
{
    void    *icmp_packet;
    uint16_t seq = 0;
    size_t   packet_size = 0;

    print_ping_prompt(target_ip, hostname, flags);

    long remaining_flag_c = flags->has_count ? flags->count_value : -1; // -c . -1 = infini

    t_stats stats;
    ft_memset(&stats, 0, sizeof(stats));
    gettimeofday(&stats.start_time, NULL);

    while (remaining_flag_c != 0)
    {
        if (deadline_reached(flags, &stats.start_time))
            break;

        icmp_packet = build_packet(flags, seq, &packet_size);
        if (!icmp_packet)
            break;
        if (send_packet(target_ip, socket_fd, icmp_packet, packet_size) != 0)
        {
            // inet : exit immediat sur send fail (ENETUNREACH avec -r par exemple), pas de stats finales
            free(icmp_packet);
            close(socket_fd);
            return 1;
        }
        stats.transmitted++;
        receive_reply(socket_fd, seq, flags, &stats);
        free(icmp_packet);

        seq++;
        if (remaining_flag_c > 0)
            remaining_flag_c--;
        if (remaining_flag_c == 0 || deadline_reached(flags, &stats.start_time))
            break;

        sleep_with_deadline(DEFAULT_INTERVAL_MS, flags, &stats.start_time);
    }

    print_stats(&stats, hostname);
    close(socket_fd);
    return 1;
}
