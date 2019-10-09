#include "get_my_info.h"

void GET_my_ip(char * dev, uint8_t * my_ip)
{
    /*        Get my IP Address      */
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;

    strncpy(ifr.ifr_name, dev, IFNAMSIZ-1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);
    memcpy(my_ip, &((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr), 4);
}
