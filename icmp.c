#include "ping.h"

// retourne 1 si la deadline -w est atteinte, 0 sinon   
// -w deadline globale
static int deadline_reached_us(t_flags *flags, struct timeval *start)
{
    if (!flags->has_deadline)
        return 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed_us = (now.tv_sec - start->tv_sec) * 1000000L
                    + (now.tv_usec - start->tv_usec);

    // start a 10, actuel a 15   15 - 10 = 5,  dead a 20 etc.
    return elapsed_us > (long)flags->deadline_value * 1000000L;
}

// 1000 ms entre chaque sleep mais check tout les 100 deadline
void sleep_with_deadline(size_t interval_ms, t_flags *flags, struct timeval *start)
{
    size_t slices = interval_ms / 100; // slice de 100ms
    while (slices-- > 0)
    {
        if (deadline_reached_us(flags, start))
            return;
        usleep(100000);
    }
    size_t leftover_ms = interval_ms % 100; // dit 250, reste 50
    if (leftover_ms && !deadline_reached_us(flags, start))
        usleep(leftover_ms * 1000);
}

int icmp(t_flags *flags, uint8_t *target_ip, char *hostname, int socket_fd)
{
    void    *icmp_packet;
    uint16_t seq = 0;
    size_t   packet_size = 0;

    print_ping_prompt(target_ip, hostname, flags);

    // 0 ou - = inf, si pas c = inf aussi
    long remaining_flag_c = (flags->has_count && flags->count_value > 0) ? flags->count_value : -1;

    // -w borne le nb max de paquets : sends a t=0, t=interval, ..., t=k*interval <= deadline
    // donc max_by_deadline = floor(deadline_ms / interval_ms) + 1
    long max_by_deadline = -1;
    if (flags->has_deadline)
        max_by_deadline = (long)flags->deadline_value * 1000L / (long)DEFAULT_INTERVAL_MS + 1;

    t_stats stats;
    memset(&stats, 0, sizeof(stats));
    gettimeofday(&stats.start_time, NULL);

    long n_sent = 0;
    while (remaining_flag_c != 0)
    {
        if (max_by_deadline >= 0 && n_sent >= max_by_deadline)
            break;

        icmp_packet = build_packet(flags, seq, &packet_size);
        if (!icmp_packet)
            break;
        if (send_packet(target_ip, socket_fd, icmp_packet, packet_size) != 0)
        {
            free(icmp_packet);
            close(socket_fd);
            return 1;
        }
        stats.transmitted++;
        n_sent++;

        // -w : ajuste SO_RCVTIMEO au min(default/-W, deadline restant)
        // sinon le 3e recv pour "-w 2" recevrait au lieu de timeout (3 trans / 2 recv attendu)
        if (flags->has_deadline)
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            long elapsed_us = (now.tv_sec - stats.start_time.tv_sec) * 1000000L
                            + (now.tv_usec - stats.start_time.tv_usec);
            long deadline_us = (long)flags->deadline_value * 1000000L;
            long remaining_us = deadline_us - elapsed_us;
            if (remaining_us <= 0)
            {
                free(icmp_packet);
                break;
            }
            long max_recv_us = flags->has_timeout
                             ? (long)flags->timeout_value * 1000000L
                             : 1000000L;
            long recv_timeout_us = remaining_us < max_recv_us ? remaining_us : max_recv_us;
            struct timeval rcv_tv;
            rcv_tv.tv_sec = recv_timeout_us / 1000000;
            rcv_tv.tv_usec = recv_timeout_us % 1000000;
            setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &rcv_tv, sizeof(rcv_tv));
        }

        receive_reply(socket_fd, seq, flags, &stats);
        free(icmp_packet);

        seq++;
        if (remaining_flag_c > 0)
            remaining_flag_c--;
        if (remaining_flag_c == 0)
            break;
        if (max_by_deadline >= 0 && n_sent >= max_by_deadline)
            break;

        // sleep cale sur start + n_sent * interval (timing absolu, pas de derive cumulee)
        struct timeval now;
        gettimeofday(&now, NULL);
        long elapsed_ms = (now.tv_sec - stats.start_time.tv_sec) * 1000L
                        + (now.tv_usec - stats.start_time.tv_usec) / 1000L;
        long sleep_ms = (n_sent * (long)DEFAULT_INTERVAL_MS) - elapsed_ms;
        if (sleep_ms > 0)
            sleep_with_deadline((size_t)sleep_ms, flags, &stats.start_time);
    }

    print_stats(&stats, hostname);
    close(socket_fd);
    return 1;
}
