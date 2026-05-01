#pragma once

#include <stdio.h>

// -V => prend le dessus en 2e 

// -v => tous activable en meme temps
// -n 
// -w 
// -W 
// -s
// -i
// -q
// -c

// '-?'   => prend le dessus sur tous  + entre quote sur zsh

typedef struct s_flags
{
    int     has_version;    // -V : affiche la version et quitte

    int     has_verbose;    // -v 
    int     has_numeric;    // -n
    int     has_quiet;      // -q
    int     has_deadline;   // -w
    int     has_timeout;    // -W
    int     has_packetsize; // -s
    int     has_interval;   // -i
    int     has_count;      // -c

    int     has_help;       // -? : affiche l'aide et quitte

    int     deadline_value;       // -w : valeur du deadline global (en sec)
    int     timeout_value;        // -W : valeur du timeout par reply
    int     packetsize_value;     // -s : taille du payload en octets
    int     interval_value;       // -i : intervalle (en sec)
    int     count_value;          // -c : nombre de paquets a send


}   t_flags;


int parsing(int ac, char **av, int *arg_offset, t_flags *flags);



int ft_strcmp(const char *s1, const char *s2);
