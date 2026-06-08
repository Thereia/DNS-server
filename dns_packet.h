#ifndef DNSRELAY_DNS_PACKET_H
#define DNSRELAY_DNS_PACKET_H

#define DNS_MAX_NAME_LEN 255

/* 这个结构体保存“当前阶段我们关心的一次 DNS 请求信息”，
 * 包括报头里的少数字段，以及 Question 里的域名/类型/类。 */
typedef struct DnsRequestInfo {
    unsigned short id;
    unsigned short flags;
    unsigned short qtype;
    unsigned short qclass;
    char qname[DNS_MAX_NAME_LEN + 1];
} DnsRequestInfo;

int dns_parse_question(const unsigned char *packet, int packet_len, DnsRequestInfo *out_request_info);
unsigned short dns_read_id(const unsigned char *packet, int packet_len);
int dns_build_a_response(
    const unsigned char *request,
    int request_len,
    const char *ip_text,
    unsigned char *response,
    int response_capacity,
    int *response_len
);
int dns_build_nxdomain_response(
    const unsigned char *request,
    int request_len,
    unsigned char *response,
    int response_capacity,
    int *response_len
);

#endif
