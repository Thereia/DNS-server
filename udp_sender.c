#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    /* 这个程序只是当前阶段的辅助发送器。
     * 它负责给监听程序发一个 UDP 包。 */
    WSADATA wsa_data;
    SOCKET sock;
    struct sockaddr_in dest;
    const char *ip;
    unsigned short port;
    const char *message;
    int sent;

    /* 如果没有手动传参数，就默认发到本机的测试端口。 */
    ip = argc > 1 ? argv[1] : "127.0.0.1";
    port = argc > 2 ? (unsigned short)atoi(argv[2]) : 5533;
    message = argc > 3 ? argv[3] : "hello-stage1";

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    /* 填好目标地址，也就是“这个包要发给谁”。 */
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = inet_addr(ip);
    if (dest.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "invalid target ip: %s\n", ip);
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    /* sendto 的作用是：
     * 把一整份 UDP 数据报发给指定 IP 和端口。 */
    sent = sendto(sock, message, (int)strlen(message), 0, (const struct sockaddr *)&dest, sizeof(dest));
    if (sent == SOCKET_ERROR) {
        fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("sent %d bytes to %s:%u\n", sent, ip, port);
    closesocket(sock);
    WSACleanup();
    return 0;
}
