#include "ping.h"

int main(int ac, char **av)
{
    int arg_offset;
    t_flags flags;
    if (!parsing(ac, av, &arg_offset, &flags))
        return 1;
    return 0;
}
