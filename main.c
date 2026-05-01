#include "ping.h"


int parsing(int ac, char **av)
{
    // tous les flags sont compatibles
    if (ac < 2 || ac > 17)
        return 0;





    return 1;
}



int main(int ac, char **av)
{
    if (!parsing(ac, av))
        return 1;


    return 0;
}