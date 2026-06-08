#include "dns_packet.h"

#include <string.h>

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
