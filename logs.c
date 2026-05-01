#include "ping.h"

void display_reply(t_reply *reply_struc, struct sockaddr_in *from_ip, t_flags *flags, double rtt_ms)
{
    if (flags->has_quiet) // -q : on n affiche pas les replies (que stat de fin)
        return;

    char src_ip_str[INET_ADDRSTRLEN]; // taille max ip v4
    inet_ntop(AF_INET, &from_ip->sin_addr, src_ip_str, INET_ADDRSTRLEN); // converit en string hexa l ip du replyer

    if (reply_struc->has_timestamp)
    {
        printf("%zu bytes from %s: icmp_seq=%u ttl=%u time=%.3f ms\n",
               reply_struc->icmp_size, src_ip_str,
               reply_struc->seq, reply_struc->ttl, rtt_ms);
    }
    else
     {
        printf("%zu bytes from %s: icmp_seq=%u ttl=%u\n",
               reply_struc->icmp_size, src_ip_str,
               reply_struc->seq, reply_struc->ttl);
    }
}

void print_ping_prompt(uint8_t *target_ip, char *hostname, t_flags *flags)
{
    size_t   payload_size = flags->has_packetsize ? (size_t)flags->packetsize_value : DEFAULT_PAYLOAD_SIZE;

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, target_ip, ip_str, INET_ADDRSTRLEN);
    printf("PING %s (%s) %zu(%zu) bytes of data.\n",
        hostname, ip_str, payload_size, payload_size + ICMP_HDR_SIZE + 20);
}

// selon code, le bon msg
static const char *icmp_error_str(uint8_t type, uint8_t code)
{
    if (type == ICMP_DEST_UNREACH)
    {
        if (code == ICMP_NET_UNREACH)  return "Destination Net Unreachable";
        if (code == ICMP_HOST_UNREACH) return "Destination Host Unreachable";
        if (code == ICMP_PROT_UNREACH) return "Destination Protocol Unreachable";
        if (code == ICMP_PORT_UNREACH) return "Destination Port Unreachable";
        if (code == ICMP_FRAG_NEEDED)  return "Fragmentation needed and DF set";
        if (code == ICMP_SR_FAILED)    return "Source Route Failed";
        return "Destination Unreachable";
    }
    if (type == ICMP_TIME_EXCEEDED)
    {
        if (code == ICMP_EXC_TTL)      return "Time to live exceeded";
        if (code == ICMP_EXC_FRAGTIME) return "Frag reassembly time exceeded";
        return "Time exceeded";
    }
    if (type == ICMP_REDIRECT)        return "Redirect";
    if (type == ICMP_SOURCE_QUENCH)   return "Source Quench";
    if (type == ICMP_PARAMETERPROB)   return "Parameter problem";
    return "Unknown ICMP";
}

// -v : afficher les erreurs ICMP recues, avc la bonne sequence de packet + code erreur
void print_verbose_error(struct sockaddr_in *from_ip, t_reply *reply_struc, uint16_t orig_seq)
{
    char src_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from_ip->sin_addr, src_ip_str, INET_ADDRSTRLEN);
    printf("From %s icmp_seq=%u %s\n",
        src_ip_str, orig_seq, icmp_error_str(reply_struc->type, reply_struc->code));
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
