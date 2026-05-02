#include "ping.h"


int icmp(t_flags *flags, uint8_t *target_ip, char *hostname, int socket_fd)
{
    void    *icmp_packet;
    uint16_t seq = 1;
    size_t   packet_size = 0;

    print_ping_prompt(target_ip, hostname, flags);

    int interval_flag_i = flags->has_interval ? flags->interval_value : 1; // -i
    long remaining_flag_c = flags->has_count ? flags->count_value : -1; // -c . -1 = infini

    // stats / -w : init a 0 start
    t_stats stats;  // stats = le prompt resume de fin
    ft_memset(&stats, 0, sizeof(stats));
    gettimeofday(&stats.start_time, NULL);

    // boucle d envoit/reception : 1 paquet, attendre reponse, sleep, recommencer
    while (remaining_flag_c != 0)
    {
        icmp_packet = build_packet(flags, seq, &packet_size);
        if (!icmp_packet)
            break;
        if (send_packet(target_ip, socket_fd, icmp_packet, packet_size) == 0)
        {
            stats.transmitted++;
            // va maj stat et parser la reply.
            receive_reply(socket_fd, seq, flags, &stats); // pas full safe meme si dans condi car si sort timout de ping 1. revient envoit ping2 et la recoit pinng 1 (donc seq)
        }
        free(icmp_packet);

        seq++;
        if (remaining_flag_c > 0) // si deja a -1 restera a -1 tout le temps
            remaining_flag_c--;
        if (remaining_flag_c == 0)
            break;

        // -w : deadline passed , sort
        if (flags->has_deadline)
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            if ((now.tv_sec - stats.start_time.tv_sec) >= flags->deadline_value)
                break;
        }

        sleep(interval_flag_i);
    }

    print_stats(&stats, hostname);
    close(socket_fd);
    return 1;
}
