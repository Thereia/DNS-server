#include "server.h"
#include "common.h"
#include "dns_packet.h"

#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>

/* 打印前几个字节，便于确认真实收到的报文长什么样。 */
static void print_hex_preview(const unsigned char *buffer, int length) {
    int i;
    int preview_len = length < 16 ? length : 16;

    printf("hex=");
    for (i = 0; i < preview_len; ++i) {
        printf("%02X", buffer[i]);
        if (i + 1 < preview_len) {
            printf(" ");
        }
    }

    if (length > preview_len) {
        printf(" ...");
    }
}

int server_run(const char *listen_ip, unsigned short listen_port) {
    /* sock 是本程序用于监听的 UDP socket。 */
    SOCKET sock = INVALID_SOCKET;

    /* addr 表示本地监听地址，client_addr 表示发送方地址。 */
    struct sockaddr_in addr;
    struct sockaddr_in client_addr;
    int client_len;

    // buffer 每次保存一个收到的 UDP 数据报
    char buffer[DNSRELAY_BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    // 创建一个 IPv4 + UDP 的 socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    // 填好本地监听地址
    memset(&addr, 0, sizeof(addr)); // 把结构体清零，避免里面有垃圾数据
    addr.sin_family = AF_INET;  // IPv4 
    addr.sin_port = htons(listen_port); // 把正在监听的端口的主机字节序转换成网络字节序
    addr.sin_addr.s_addr = inet_addr(listen_ip);    // 把正在监听的 IP 转成网络字节序的整数形式
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "invalid IP address: %s\n", listen_ip);
        closesocket(sock);
        return 1;
    }

    /* bind 的含义是：
     * 把这个 socket 绑定到“本机某个 IP + 某个端口”上，
     * 以后发到这里的 UDP 包就由它来接收。 */
    if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    printf("listening on %s:%u\n", listen_ip, listen_port);
    fflush(stdout);

    /* 主循环：
     * 一直等待别人发 UDP 包过来；
     * 收到以后先打印来源信息；
     * 然后继续等下一个包。 */
    for (;;) {
        DnsRequestInfo request_info;
        int received;

        client_len = sizeof(client_addr);
        received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (received == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
            break;
        }

        /* 把发送方 IP 转成可打印的字符串，方便输出日志。 */
        strncpy(client_ip, inet_ntoa(client_addr.sin_addr), sizeof(client_ip) - 1);
        client_ip[sizeof(client_ip) - 1] = '\0';

        /* 第三阶段开始尝试把收到的内容按 DNS 查询来解析。
         * 先确认“确实收到包”，再区分是否解析成功。 */
        if (dns_parse_question((const unsigned char *)buffer, received, &request_info) == 0) {
            printf(
                "received %d bytes from %s:%u, dns id=%u, qname=%s, qtype=%u, ",
                received,
                client_ip,
                ntohs(client_addr.sin_port),
                request_info.id,
                request_info.qname,
                request_info.qtype
            );
            print_hex_preview((const unsigned char *)buffer, received);
            printf("\n");
        } else {
            printf("received %d bytes from %s:%u, non-dns-or-unparsed packet, ",
                   received,
                   client_ip,
                   ntohs(client_addr.sin_port));
            print_hex_preview((const unsigned char *)buffer, received);
            printf("\n");
        }
        fflush(stdout);
    }

    closesocket(sock);
    return 0;
}
