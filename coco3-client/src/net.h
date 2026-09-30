#ifndef NET_H
#define NET_H

/* Opens and closes a TCP connection to host:port and says whether it
 * connected. Text mode only. */
void net_check_host(const char *host, unsigned int port);

#endif
