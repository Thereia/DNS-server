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
    return 0;
}
