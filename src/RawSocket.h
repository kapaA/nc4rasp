#pragma once

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <system_error>
#include <cstdint>
#include <cstdbool>

class RawSocket
{
public:	
    RawSocket(std::string interface)
        : m_interface(interface),
          m_src(0),
	  m_dst(0),
	  m_port(0)

    {
        open_socket();
        set_promisc(true);
        bind_interface();
    }

    std::string m_interface;
    int m_sock;
    uint32_t m_src ;
    uint32_t m_dst;
    uint16_t m_port ;
    struct iphdr *m_ip;
    struct udphdr *m_udp;
    uint8_t *m_data;

    void open_socket()
    {
        if ((m_sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to open socket");
    }

    void set_promisc(bool enable)
    {
        struct ifreq ifr = {0};

        strncpy(ifr.ifr_name, m_interface.c_str(), IFNAMSIZ);

        if (ioctl(m_sock, SIOCGIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to get interface flags");

        if (enable)
            ifr.ifr_flags |= IFF_PROMISC;
        else
            ifr.ifr_flags &= ~IFF_PROMISC;

        if (ioctl(m_sock, SIOCSIFFLAGS, &ifr) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to set interface flags");
    }

    void bind_interface()
    {
        struct ifreq ifr = {0};

        strncpy(ifr.ifr_name, m_interface.c_str(), IFNAMSIZ);

        if (setsockopt(m_sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to bind to interface");
    }

  public:


    ~RawSocket()
    {
        set_promisc(false);
    }

    void filterSrc(uint32_t src)
    {
        m_src = htonl(src);
    }

    void filterSrc(std::string src)
    {
        uint32_t tmp;

        if (inet_pton(AF_INET, src.c_str(), reinterpret_cast<void *>(&tmp)) != 1)
            throw "unable to convert source address";

        filterSrc(tmp);
    }

    void filterDst(uint32_t dst)
    {
        m_dst = htonl(dst);
    }

    void filterDst(std::string &dst)
    {
        uint32_t tmp;

        if (inet_pton(AF_INET, dst.c_str(), reinterpret_cast<void *>(&tmp)) != 1)
            throw "unable to convert destination address";

        filterDst(tmp);
    }

    void filterPort(uint16_t port)
    {
        m_port = htons(port);
    }

    int recvFrom(uint8_t *buf, size_t len, std::string &src, uint16_t &port)
    {
        printf("receive 1 ");
	return recvFrom(reinterpret_cast<char *>(buf), len, src, port);
    }

    int recvFrom(char *buf, size_t len, std::string &src, uint16_t &port)
    {
        int res;
        char src_tmp[INET_ADDRSTRLEN];

        while (true)
        {
            res = recvfrom(m_sock, buf, len, 0, NULL, 0);
	    printf("receive");

            if (res < 0 && errno == EINTR)
                return -1;

            if (res < 0)
                throw std::system_error(errno, std::system_category(),
                                        "unable to receive from socket");

            if (res < sizeof(*m_ip) + sizeof(*m_udp))
                continue;

            m_ip = reinterpret_cast<struct iphdr *>(buf);
            m_udp = reinterpret_cast<struct udphdr *>(buf + sizeof(*m_ip));
            m_data = reinterpret_cast<uint8_t *>(buf) + sizeof(*m_ip) + sizeof(*m_udp);

            if (m_src && m_src != m_ip->saddr)
                continue;

            if (m_dst && m_dst != m_ip->daddr)
                continue;

            if (m_port && m_port != m_udp->dest)
                continue;

            break;
        }

        port = htons(m_udp->source);
        inet_ntop(AF_INET, (struct in_addr *)&m_ip->saddr, src_tmp, INET_ADDRSTRLEN);
        src = src_tmp;

        return htons(m_udp->len) - sizeof(*m_udp);
    }

    static size_t hdrLen()
    {
        return sizeof(*m_ip) + sizeof(*m_udp);
    }

    uint8_t *data()
    {
        return m_data;
    }
};
