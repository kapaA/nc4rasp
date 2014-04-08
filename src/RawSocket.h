#pragma once

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/filter.h>
#include <netpacket/packet.h>
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
    std::string m_interface;
    int m_sock, m_if_index;
    uint32_t m_src;
    uint32_t m_dst;
    uint16_t m_port;
    struct ethhdr *m_mac;
    struct iphdr *m_ip;
    struct udphdr *m_udp;
    uint8_t *m_data;
    public :
    RawSocket()
        : m_src(0),
	      m_dst(0),
	      m_port(0)

    {

    }

    void start(std::string interface)
    {
	    
        m_interface = interface;
        open_socket();
        get_index();
        set_promisc(true);
        filter_udp();
        bind_interface();
	
    }
    void open_socket()
    {
        if ((m_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to open socket");
   }

    void get_index()
    {
        struct ifreq ifr = {0};

        strncpy(ifr.ifr_name, m_interface.c_str(), IFNAMSIZ);

        if (ioctl(m_sock, SIOCGIFINDEX, &ifr) != 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to get interface index");

        m_if_index = ifr.ifr_ifindex;
    }

    void set_promisc(bool enable)
    {
        struct packet_mreq mreq = {0};
        int action;

        mreq.mr_ifindex = m_if_index;
        mreq.mr_type = PACKET_MR_PROMISC;

        if (enable)
            action = PACKET_ADD_MEMBERSHIP;
        else
            action = PACKET_DROP_MEMBERSHIP;

        if (setsockopt(m_sock, SOL_PACKET, action, &mreq, sizeof(mreq)) != 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to set interface promisc");
    }

    void filter_udp()
    {
        struct sock_fprog prog;
        struct sock_filter filter[] = {
            { BPF_LD + BPF_H + BPF_ABS,  0, 0,     12 },
            { BPF_JMP + BPF_JEQ + BPF_K, 0, 1,  0x800 },
            { BPF_LD + BPF_B + BPF_ABS,  0, 0,     23 },
            { BPF_JMP + BPF_JEQ + BPF_K, 0, 1,   0x11 },
            { BPF_RET + BPF_K,           0, 0, 0xffff },
            { BPF_RET + BPF_K,           0, 0, 0x0000 },
        };

        prog.len = sizeof(filter)/sizeof(filter[0]);
        prog.filter = filter;

        if (setsockopt(m_sock, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog)) != 0)
            throw std::system_error(errno, std::system_category(),
                                    "unable to attach filter");
    }

    void bind_interface()
    {
        struct sockaddr_ll sa = {0};

        sa.sll_family = AF_PACKET;
        sa.sll_ifindex = m_if_index;
        sa.sll_protocol = htons(ETH_P_ALL);

        if (bind(m_sock, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) < 0)
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
        return recvFrom(reinterpret_cast<char *>(buf), len, src, port);
    }

    int recvFrom(char *buf, size_t len, std::string &src, uint16_t &port)
    {
        int res;
        char src_tmp[INET_ADDRSTRLEN];

        while (true)
        {
            res = recvfrom(m_sock, buf, len, 0, NULL, 0);

            if (res < 0 && errno == EINTR)
                return -1;

            if (res < 0)
                throw std::system_error(errno, std::system_category(),
                                        "unable to receive from socket");

            if (res < sizeof(*m_ip) + sizeof(*m_udp))
                continue;

            m_mac = reinterpret_cast<struct ethhdr *>(buf);
            m_ip = reinterpret_cast<struct iphdr *>(buf + sizeof(*m_mac));
            m_udp = reinterpret_cast<struct udphdr *>(buf + sizeof(*m_mac) + sizeof(*m_ip));
            m_data = reinterpret_cast<uint8_t *>(buf) + sizeof(*m_mac) + sizeof(*m_ip) + sizeof(*m_udp);

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
        return sizeof(*m_mac) + sizeof(*m_ip) + sizeof(*m_udp);
    }

    uint8_t *data()
    {
        return m_data;
    }
};
