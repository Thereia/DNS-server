#include "dns_packet.h"

#include <assert.h>
#include <string.h>

int main(void) {
    unsigned char packet[] = {
        0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x03, 'w', 'w', 'w',
        0x06, 'g', 'o', 'o', 'g', 'l', 'e',
        0x03, 'c', 'o', 'm',
        0x00,
        0x00, 0x01,
        0x00, 0x01
    };
    DnsRequestInfo request_info;

    assert(dns_parse_question(packet, sizeof(packet), &request_info) == 0);
    assert(request_info.id == 0x1234);
    assert(strcmp(request_info.qname, "www.google.com") == 0);
    assert(request_info.qtype == 1);
    assert(request_info.qclass == 1);

    {
        unsigned char response[512];
        int response_len = 0;

        assert(dns_build_a_response(packet, sizeof(packet), "1.2.3.4", response, sizeof(response), &response_len) == 0);
        assert(response_len > (int)sizeof(packet));
        assert(response[0] == 0x12);
        assert(response[1] == 0x34);
        assert(response[2] == 0x81);
        assert(response[3] == 0x80);
        assert(response[6] == 0x00);
        assert(response[7] == 0x01);
    }

    {
        unsigned char response[512];
        int response_len = 0;

        assert(dns_build_nxdomain_response(packet, sizeof(packet), response, sizeof(response), &response_len) == 0);
        assert(response_len == (int)sizeof(packet));
        assert(response[0] == 0x12);
        assert(response[1] == 0x34);
        assert(response[2] == 0x81);
        assert(response[3] == 0x83);
        assert(response[6] == 0x00);
        assert(response[7] == 0x00);
    }

    return 0;
}
