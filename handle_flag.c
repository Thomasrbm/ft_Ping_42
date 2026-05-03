#include "ping.h"

void version_flag(void)
{
    printf("ft_ping v1.0\n");
    printf("the program copies : ping from (GNU inetutils) 2.0\n");
    printf("Copyright (C) 2019 Free Software Foundation, Inc.\n");
    printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
    printf("This is free software: you are free to change and redistribute it.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
    printf("Written by several authors.\n");
}

void help_flag(void)
{
    printf("Usage: ft_ping [OPTION...] HOST ...\n");
    printf("Send ICMP ECHO_REQUEST packets to network hosts.\n\n");
    printf(" Options controlling ICMP request types:\n");
    printf("  -c NUMBER          stop after sending NUMBER packets\n");
    printf("  -q                 quiet output\n");
    printf("  -r                 send directly to a host on an attached network\n");
    printf("      --ttl=N        specify N as time-to-live\n");
    printf("  -s NUMBER          send NUMBER data octets\n");
    printf("  -v                 verbose output\n");
    printf("  -w NUMBER          stop after NUMBER seconds\n");
    printf("  -W NUMBER          number of seconds to wait for response\n\n");
    printf("  -?                 give this help list\n");
    printf("  -V                 print program version\n");
}

void handle_flags(t_flags *flags)
{
    if (flags->has_help)
    {
        help_flag();
        return ;
    }
    if (flags->has_version)
    {
        version_flag();
        return ;
    }
}