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
    DnsQuestion question;

    assert(dns_parse_question(packet, sizeof(packet), &question) == 0);
    assert(question.id == 0x1234);
    assert(strcmp(question.qname, "www.google.com") == 0);
    assert(question.qtype == 1);
    assert(question.qclass == 1);
    return 0;
}
