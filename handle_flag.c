#include "ping.h"

void version_flag(void)
{
    printf("ft_ping v1.0\n");
    printf("the program copies : ping from iputils 20250605\n");
    printf("tries to have same (not really) libcap: yes, IDN: yes, NLS: yes, error.h: yes, getrandom(): yes, __fpending(): yes\n");
}

void help_flag(void)
{
    printf("Usage: ft_ping [OPTION...] HOST ...\n");
    printf("Send ICMP ECHO_REQUEST packets to network hosts.\n\n");
    printf(" Options controlling ICMP request types:\n");
    printf("  -c NUMBER          stop after sending NUMBER packets\n");
    printf("  -i NUMBER          wait NUMBER seconds between sending each packet\n");
    printf("  -n                 do not resolve host addresses\n");
    printf("  -q                 quiet output\n");
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