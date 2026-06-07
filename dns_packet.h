#ifndef DNSRELAY_DNS_PACKET_H
#define DNSRELAY_DNS_PACKET_H

#define DNS_MAX_NAME_LEN 255

/* 当前阶段只关心 DNS 查询里最基本的几个字段。 */
typedef struct DnsQuestion {
    unsigned short id;
    unsigned short flags;
    unsigned short qtype;
    unsigned short qclass;
    char qname[DNS_MAX_NAME_LEN + 1];
} DnsQuestion;

int dns_parse_question(const unsigned char *packet, int packet_len, DnsQuestion *out_question);
unsigned short dns_read_id(const unsigned char *packet, int packet_len);

#endif
