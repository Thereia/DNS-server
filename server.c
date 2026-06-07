#include "server.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <ws2tcpip.h>

int server_run(const char *listen_ip, unsigned short listen_port) {
    /* sock 是本程序用于监听的 UDP socket。 */
    SOCKET sock = INVALID_SOCKET;

    /* addr 表示本地监听地址，client_addr 表示发送方地址。 */
    struct sockaddr_in addr;
    struct sockaddr_in client_addr;
    int client_len;

    /* buffer 每次保存一个收到的 UDP 数据报。 */
    char buffer[DNSRELAY_BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];

    /* 创建一个 IPv4 + UDP 的 socket。 */
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* 填好本地监听地址。 */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listen_port);
    addr.sin_addr.s_addr = inet_addr(listen_ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "invalid listen ip: %s\n", listen_ip);
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

        /* 当前阶段只证明“包确实收到了”。
         * 后面真正做 DNS 时，这里才会继续解析报文内容。 */
        printf("received %d bytes from %s:%u\n", received, client_ip, ntohs(client_addr.sin_port));
        fflush(stdout);
    }

    closesocket(sock);
    return 0;
}
