double update_rtt_stats(t_reply *reply_struc, t_stats *stats)
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