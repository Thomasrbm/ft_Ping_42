

// packet est uint8
int check_sum(void *packet, int total_size)
{
    unsigned short *buff = packet; // lit 2 par 2
    int sum = 0;

    while (total_size > 1)
    {
        sum += *buff;
        buff++; // par rapport a packet jump de 2 en 2
        total_size-= 2;
    }

    if (total_size == 1)
        sum += *(unsigned char *)buff;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}


// du 16 bit dans un resultt de 32 bit on replut sur 16 
// en decidant que a gauche ca devient un carry


// Mots :  0xAAAA  0xAAAA  0x5556
//        = 43690 + 43690 + 21846
// Somme : 43690 + 43690 + 21846 = 109226 = 0x0001AAAA 

// sum          = 0x0001AAAA = 109226     (32 bits, carry overflow)
// sum >> 16    = 0x00000001 = 1          (le carry isolé)
// sum & 0xFFFF = 0x0000AAAA = 43690      (les 16 bits bas)
// résultat     = 0x00000001 + 0x0000AAAA = 0x0000AAAB = 43691

// ici change rien le deuxiemme >> 


// deuxiemme decalage au cas encre des chiffre a gauche 


// sum = 0xFFFFFFFF = 4294967295   (le pire cas, max d'un uint32_t)

// 1er fold :
//   sum >> 16    = 0xFFFF   = 65535
//   sum & 0xFFFF = 0xFFFF   = 65535
//   sum = 65535 + 65535 = 131070 = 0x1FFFE   ← encore overflow, dépasse 65535



//   131070 ça dépasse 65535 (le max sur 16 bits). Donc on refold :
// 2ème fold :
//   sum >> 16    = 0x0001  = 1
//   sum & 0xFFFF = 0xFFFE  = 65534
//   sum = 1 + 65534 = 65535 = 0xFFFF   