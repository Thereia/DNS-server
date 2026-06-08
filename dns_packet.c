#include "dns_packet.h"

#include <string.h>
#include <winsock2.h>

/* 按 DNS 报文格式读取一个 16 位无符号整数。 */
static unsigned short read_u16(const unsigned char *p) {
    return (unsigned short)((p[0] << 8) | p[1]);
}

unsigned short dns_read_id(const unsigned char *packet, int packet_len) {
    if (packet_len < 2) {
        return 0;
    }

    return read_u16(packet);
}

int dns_build_a_response(
    const unsigned char *request,
    int request_len,
    const char *ip_text,
    unsigned char *response,
    int response_capacity,
    int *response_len
) {
    struct in_addr addr;
    int base_len;

    /* 当前先只支持最常见的 IPv4 A 记录应答。 */
    if (request_len < 12) {
        return 1;
    }

    if (response_capacity < request_len + 16) {
        return 1;
    }

    addr.s_addr = inet_addr(ip_text);
    if (addr.s_addr == INADDR_NONE) {
        return 1;
    }

    /* 先把原始查询完整拷贝过去。
     * 这样 Question 部分可以直接复用。 */
    memcpy(response, request, request_len);

    /* 设置响应头：
     * QR=1, RD 保留原请求，RA 先不设置。
     * ANCOUNT=1，表示有 1 条 Answer 记录。 */
    response[2] = 0x81;
    response[3] = 0x80;
    response[6] = 0x00;
    response[7] = 0x01;
    response[8] = 0x00;
    response[9] = 0x00;
    response[10] = 0x00;
    response[11] = 0x00;

    base_len = request_len;

    /* 使用压缩指针 C00C 指回原始 QNAME。 */
    response[base_len + 0] = 0xC0;
    response[base_len + 1] = 0x0C;
    response[base_len + 2] = 0x00;
    response[base_len + 3] = 0x01;
    response[base_len + 4] = 0x00;
    response[base_len + 5] = 0x01;
    response[base_len + 6] = 0x00;
    response[base_len + 7] = 0x00;
    response[base_len + 8] = 0x00;
    response[base_len + 9] = 0x3C;
    response[base_len + 10] = 0x00;
    response[base_len + 11] = 0x04;
    memcpy(response + base_len + 12, &addr, 4);

    *response_len = base_len + 16;
    return 0;
}

int dns_build_nxdomain_response(
    const unsigned char *request,
    int request_len,
    unsigned char *response,
    int response_capacity,
    int *response_len
) {
    if (request_len < 12) {
        return 1;
    }

    if (response_capacity < request_len) {
        return 1;
    }

    memcpy(response, request, request_len);

    /* NXDOMAIN:
     * QR=1，RCODE=3。
     * 当前先不带 Answer，只返回“名字不存在”。 */
    response[2] = 0x81;
    response[3] = 0x83;
    response[6] = 0x00;
    response[7] = 0x00;
    response[8] = 0x00;
    response[9] = 0x00;
    response[10] = 0x00;
    response[11] = 0x00;

    *response_len = request_len;
    return 0;
}

int dns_parse_question(const unsigned char *packet, int packet_len, DnsRequestInfo *out_request_info) {
    int pos = 12;
    int out_pos = 0;

    if (packet_len < 18) {
        return 1;
    }

    memset(out_request_info, 0, sizeof(*out_request_info));
    out_request_info->id = read_u16(packet);
    out_request_info->flags = read_u16(packet + 2);

    /* DNS 头后面先是 QNAME。
     * 它不是普通字符串，而是“长度 + 标签”的形式。 */
    while (pos < packet_len && packet[pos] != 0) {
        unsigned int label_len = packet[pos++];
        unsigned int i;

        if (label_len == 0 || pos + (int)label_len > packet_len) {
            return 1;
        }

        if (out_pos != 0) {
            if (out_pos >= DNS_MAX_NAME_LEN) {
                return 1;
            }
            out_request_info->qname[out_pos++] = '.';
        }

        if (out_pos + (int)label_len > DNS_MAX_NAME_LEN) {
            return 1;
        }

        for (i = 0; i < label_len; ++i) {
            out_request_info->qname[out_pos++] = (char)packet[pos++];
        }
    }

    /* 需要至少还有：
     * 1 字节结尾 0
     * 2 字节 QTYPE
     * 2 字节 QCLASS */
    if (pos + 5 > packet_len) {
        return 1;
    }

    out_request_info->qname[out_pos] = '\0';
    pos++;
    out_request_info->qtype = read_u16(packet + pos);
    out_request_info->qclass = read_u16(packet + pos + 2);
    return 0;
}
