#ifndef _SNMPIPBASEDOMAIN_H_
#define _SNMPIPBASEDOMAIN_H_

#include <net-snmp/types.h>

/**
 * SNMP endpoint specification.
 * @a: Address family, network address and port number.
 * @iface: Network interface name in ASCII format. May be empty.
 */
struct netsnmp_ep {
    union {
        struct sockaddr_in  sin;
#if defined(NETSNMP_TRANSPORT_UDPIPV6_DOMAIN) || \
    defined(NETSNMP_TRANSPORT_TCPIPV6_DOMAIN)
        struct sockaddr_in6 sin6;
#endif
    } a;
    char iface[16];
};

/**
 * SNMP endpoint with the network name in ASCII format.
 * @addr: Network address or host name as an ASCII string.
 * @port: Port number in host byte format.
 */
struct netsnmp_ep_str {
    char     addr[64];
    uint16_t port;
};

int netsnmp_parse_ep_str(struct netsnmp_ep_str *ep_str, const char *endpoint);

#endif /* _SNMPIPBASEDOMAIN_H_ */
