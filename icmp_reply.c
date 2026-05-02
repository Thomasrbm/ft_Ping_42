#include "ping.h"

// quand on recoit une erreur ICMP (type 3, 11, etc.), le paquet contient
// l ip header originale + 8 octets du icmp original. on extrait la seq de l original
// besoin de la sequence originel pour savoir quel packet a quelle erreur 

// recuperer la sequence du paquet, 2e ft car paquet erreur pas meme que paquet de reply normal
//   [ IP routeur ][ ICMP erreur ][ IP toi ][ 8 octets ICMP toi ]   
// normal =   [ IP header ][ ICMP header ][ payload ]                                                                                                                                                        
uint16_t parse_error_seq(uint8_t *reply_buffer, ssize_t buffer_len)
{
    struct iphdr   *target_ip;
    size_t          target_ip_size;
    struct iphdr   *our_ip;
    size_t          our_ip_size;
    struct icmphdr *our_icmp;

    target_ip = (struct iphdr *)reply_buffer;
    target_ip_size = target_ip->ihl * 4;
    // si pas la place pour plus, faux packet
    if ((size_t)buffer_len < target_ip_size + 8 + sizeof(struct iphdr))
        return 0;

    our_ip = (struct iphdr *)(reply_buffer + target_ip_size + 8);
    our_ip_size = our_ip->ihl * 4;
    if ((size_t)buffer_len < target_ip_size + 8 + our_ip_size + 8)
        return 0;

    our_icmp = (struct icmphdr *)(reply_buffer + target_ip_size + 8 + our_ip_size);
    return ntohs(our_icmp->un.echo.sequence);
}

// verifier si c est bien la reponse pour moi (rapport a ma request)
int validate_reply(t_reply *reply_struc, uint16_t seq)
{
    // pas un ping echo reply  == une erreur surement
    if (reply_struc->type != ICMP_ECHOREPLY)
        return 0;
    // notre pid donc bien retour de notre echo
    if (reply_struc->id != (getpid() & 0xFFFF))
        return 0;
    // la reponse par rapport au bon packet
    if (reply_struc->seq != seq)
        return 0;
    return 1;
}

// on recoit [ Header IP : 20 octets ][ Header ICMP : 8 octets ][ Payload : N octets ]
int parse_reply(uint8_t *reply_buffer, ssize_t buffer_len, t_reply *reply_struc)
{
    struct iphdr   *ip_header;
    struct icmphdr *icmp_response;
    size_t          ip_header_size;

    ip_header = (struct iphdr *)reply_buffer;
    ip_header_size = ip_header->ihl * 4; // ihl vaut mots de 32bit 4octet x nombre de mots = donne la vrai taille du header (min 20 mais 60 si options)

    // check si y a header ip + icmp sinon packet faux
    if ((size_t)buffer_len < ip_header_size + ICMP_HDR_SIZE)
        return 0;

    reply_struc->ttl = ip_header->ttl; // print le ttl de retour (peut etre 100+ car windows a 128 ttl)

    // cast le void buffer en icmp header en jumpant par dessus le ip header
    icmp_response = (struct icmphdr *)(reply_buffer + ip_header_size);

    reply_struc->type = icmp_response->type; // maj si error ou echo reply
    reply_struc->code = icmp_response->code;
    reply_struc->id   = ntohs(icmp_response->un.echo.id);
    reply_struc->seq  = ntohs(icmp_response->un.echo.sequence);

    // payload_size : taille du payload uniquement, pour savoir si y a la place pour un timestamp
    // icmp_size = afficher dans le terminal

    reply_struc->icmp_size    = buffer_len - ip_header_size;
    reply_struc->payload_size = reply_struc->icmp_size - ICMP_HDR_SIZE;

    if (reply_struc->payload_size >= sizeof(struct timeval))  // si pas de timestamp car payload trop petit alors on print pas le time
    {
        struct timeval *ts = (struct timeval *)((uint8_t *)icmp_response + ICMP_HDR_SIZE);
        reply_struc->send_time = *ts;
        reply_struc->has_timestamp = 1;
    }
    else
    {
        reply_struc->has_timestamp = 0;
    }

    return 1;
}

// retourne 1 si erreur "fatale" (pas un timeout), 0 sinon
static int handle_recv_error(uint16_t seq, t_flags *flags)
{
    if (errno == EAGAIN || errno == EWOULDBLOCK) // EAGAIN = pas de rep,  EWOULDBLOCK timeout
    {
        if (!flags->has_quiet) // -q : pas d affichage des timeouts non plus
            printf("Request timeout for icmp_seq=%u\n", seq);
        return 0;
    }
    dprintf(2, "recvfrom: %s\n", strerror(errno));
    return 0;
}

// -v : affiche les erreurs ICMP (dest unreach, time exceeded, etc.) si reply pas pour nous
static void handle_icmp_error(uint8_t *reply_buffer, ssize_t bytes_received,
                              t_reply *reply_struc, struct sockaddr_in *from_ip, t_flags *flags)
{
    if (!flags->has_verbose)
        return;
    // si autre erreur ping prevoit pas donc on print rien (paquet defectueux pour raison diff de ping)
    if (reply_struc->type != ICMP_DEST_UNREACH &&
        reply_struc->type != ICMP_TIME_EXCEEDED &&
        reply_struc->type != ICMP_REDIRECT &&
        reply_struc->type != ICMP_SOURCE_QUENCH &&
        reply_struc->type != ICMP_PARAMETERPROB)
        return;
    uint16_t orig_seq = parse_error_seq(reply_buffer, bytes_received);
    print_verbose_error(from_ip, reply_struc, orig_seq, flags);
}

// calcule le RTT et met a jour min/max/sum/sum^2/count, retourne le RTT en ms
static double update_rtt_stats(t_reply *reply_struc, t_stats *stats)
{
    if (!reply_struc->has_timestamp)
        return 0.0;

    struct timeval recv_time;
    gettimeofday(&recv_time, NULL);

    // temps d ecart en micro seconde sec
    long delta_us = (recv_time.tv_sec - reply_struc->send_time.tv_sec) * 1000000L
                  + (recv_time.tv_usec - reply_struc->send_time.tv_usec);

    // mis en ms (unite au dessus (plus grande))
    double rtt_ms = delta_us / 1000.0;

    // mdev = ecart-type : on garde sum et sum_sq pour calculer la variance a la fin
    if (stats->rtt_count == 0 || rtt_ms < stats->rtt_min)
        stats->rtt_min = rtt_ms;
    if (rtt_ms > stats->rtt_max)
        stats->rtt_max = rtt_ms;
    stats->rtt_sum += rtt_ms;
    stats->rtt_sum_sq += rtt_ms * rtt_ms;
    stats->rtt_count++;
    return rtt_ms;
}

int receive_reply(int sockfd, uint16_t seq, t_flags *flags, t_stats *stats)
{
    uint8_t reply_buffer[1024];
    struct sockaddr_in from_ip;
    socklen_t from_len = sizeof(from_ip);
    t_reply reply_struc;

    ssize_t bytes_received = recvfrom(sockfd, reply_buffer, sizeof(reply_buffer), 0,
                                (struct sockaddr *)&from_ip, &from_len);
    if (bytes_received < 0)
        return handle_recv_error(seq, flags);
    if (!parse_reply(reply_buffer, bytes_received, &reply_struc))
        return 0;
    if (!validate_reply(&reply_struc, seq))
    {
        handle_icmp_error(reply_buffer, bytes_received, &reply_struc, &from_ip, flags);
        return 0;
    }

    double rtt_ms = update_rtt_stats(&reply_struc, stats);
    stats->received++;
    display_reply(&reply_struc, &from_ip, flags, rtt_ms);
    return 1;
}
