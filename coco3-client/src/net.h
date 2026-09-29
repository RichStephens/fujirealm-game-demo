#ifndef NET_H
#define NET_H

/* Opens a raw TCP connection to host:port and measures throughput for a
 * few seconds, verifying the downlink against the 0xAA pattern
 * tools/serial_test_server.py streams by default (run it in place of the
 * game server on that port). Prints bytes received, elapsed ticks,
 * bytes/sec and pattern errors, then waits for a key. Text mode only. */
void net_measure_throughput(const char *host, unsigned int port);

#endif
