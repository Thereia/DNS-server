#include "server.h"
#include "common.h"
#include "dns_packet.h"
#include "relay_table.h"
#include "timeout.h"

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

int server_run(const char *listen_ip, unsigned short listen_port, const HostsTable *hosts_table) {
    /* sock 是本程序用于监听的 UDP socket。 */
    SOCKET sock = INVALID_SOCKET;
    SOCKET upstream_sock = INVALID_SOCKET;
    RelayTable relay_table;
    unsigned short next_relay_id = 1;

    /* addr 表示本地监听地址，client_addr 表示发送方地址。 */
    struct sockaddr_in addr;
    struct sockaddr_in client_addr;
    struct sockaddr_in upstream_addr;
    int client_len;

    // buffer 每次保存一个收到的 UDP 数据报
    char buffer[DNSRELAY_BUFFER_SIZE];
    char upstream_buffer[DNSRELAY_BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    RelayEntry *added_entry;

    // 创建一个 IPv4 + UDP 的 socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        return 1;
    }

    /* 第五阶段新增一个专门和上游 DNS 通信的 UDP socket。 */
    upstream_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (upstream_sock == INVALID_SOCKET) {
        fprintf(stderr, "upstream socket failed: %d\n", WSAGetLastError());
        closesocket(sock);
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
        closesocket(upstream_sock);
        closesocket(sock);
        return 1;
    }

    memset(&upstream_addr, 0, sizeof(upstream_addr));
    upstream_addr.sin_family = AF_INET;
    upstream_addr.sin_port = htons(DNSRELAY_UPSTREAM_PORT);
    upstream_addr.sin_addr.s_addr = inet_addr(DNSRELAY_UPSTREAM_DNS);
    if (upstream_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "invalid upstream DNS address: %s\n", DNSRELAY_UPSTREAM_DNS);
        closesocket(upstream_sock);
        closesocket(sock);
        return 1;
    }

    /* 转发表必须先清零，否则 active 标记会受栈上垃圾值影响。 */
    relay_table_init(&relay_table);

    printf("listening on %s:%u\n", listen_ip, listen_port);
    fflush(stdout);

    /* 主循环：
     * 一直等待别人发 UDP 包过来；
     * 收到以后先打印来源信息；
     * 然后继续等下一个包。 */
    for (;;) {
        const HostRecord *record;
        fd_set read_fds;
        int max_fd;
        unsigned char response[512];
        int response_len;
        DnsRequestInfo request_info;
        int received;
        struct timeval timeout_value;
        int expired_count;

        expired_count = relay_table_remove_expired(&relay_table, GetTickCount(), DNSRELAY_RELAY_TIMEOUT_MS);
        if (expired_count > 0) {
            printf(
                "relay timeout cleanup removed %d expired request(s), active=%d\n",
                expired_count,
                relay_table_active_count(&relay_table)
            );
            fflush(stdout);
        }

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(upstream_sock, &read_fds);
        max_fd = (sock > upstream_sock) ? (int)sock : (int)upstream_sock;

        timeout_value.tv_sec = 1;
        timeout_value.tv_usec = 0;

        if (select(max_fd + 1, &read_fds, NULL, NULL, &timeout_value) == SOCKET_ERROR) {
            fprintf(stderr, "select failed: %d\n", WSAGetLastError());
            break;
        }

        if (FD_ISSET(sock, &read_fds)) {
            client_len = sizeof(client_addr);
            received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
            if (received == SOCKET_ERROR) {
                fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
                break;
            }

            strncpy(client_ip, inet_ntoa(client_addr.sin_addr), sizeof(client_ip) - 1);
            client_ip[sizeof(client_ip) - 1] = '\0';

            if (dns_parse_question((const unsigned char *)buffer, received, &request_info) == 0) {
                record = hosts_find(hosts_table, request_info.qname);
                if (record != NULL) {
                    int build_ok;

                    if (record->is_blocked) {
                        build_ok = dns_build_nxdomain_response(
                            (const unsigned char *)buffer,
                            received,
                            response,
                            sizeof(response),
                            &response_len
                        ) == 0;
                    } else {
                        build_ok = dns_build_a_response(
                            (const unsigned char *)buffer,
                            received,
                            record->ip_text,
                            response,
                            sizeof(response),
                            &response_len
                        ) == 0;
                    }

                    if (build_ok) {
                        sendto(
                            sock,
                            (const char *)response,
                            response_len,
                            0,
                            (const struct sockaddr *)&client_addr,
                            client_len
                        );
                        printf(
                            "received %d bytes from %s:%u, qname=%s, local %s=%s\n",
                            received,
                            client_ip,
                            ntohs(client_addr.sin_port),
                            request_info.qname,
                            record->is_blocked ? "block" : "hit",
                            record->ip_text
                        );
                        fflush(stdout);
                        continue;
                    }
                }

                while (relay_table_find(&relay_table, next_relay_id) != NULL) {
                    next_relay_id++;
                    if (next_relay_id == 0) {
                        next_relay_id = 1;
                    }
                }

                added_entry = NULL;
                if (dns_set_id((unsigned char *)buffer, received, next_relay_id) == 0 &&
                    relay_table_add(
                        &relay_table,
                        next_relay_id,
                        request_info.id,
                        &client_addr,
                        GetTickCount()
                    ) == 0) {
                    added_entry = relay_table_find(&relay_table, next_relay_id);
                }

                if (added_entry != NULL &&
                    sendto(
                        upstream_sock,
                        (const char *)buffer,
                        received,
                        0,
                        (const struct sockaddr *)&upstream_addr,
                        sizeof(upstream_addr)
                    ) != SOCKET_ERROR) {
                    printf(
                        "received %d bytes from %s:%u, qname=%s, relayed as id=%u to upstream %s, active=%d\n",
                        received,
                        client_ip,
                        ntohs(client_addr.sin_port),
                        request_info.qname,
                        next_relay_id,
                        DNSRELAY_UPSTREAM_DNS,
                        relay_table_active_count(&relay_table)
                    );
                    fflush(stdout);
                    next_relay_id++;
                    if (next_relay_id == 0) {
                        next_relay_id = 1;
                    }
                    continue;
                }

                if (added_entry != NULL) {
                    relay_table_remove(added_entry);
                }

                printf(
                    "received %d bytes from %s:%u, dns id=%u, qname=%s, relay setup failed, ",
                    received,
                    client_ip,
                    ntohs(client_addr.sin_port),
                    request_info.id,
                    request_info.qname
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

        if (FD_ISSET(upstream_sock, &read_fds)) {
            RelayEntry *entry;
            struct sockaddr_in upstream_reply_addr;
            int upstream_reply_len = sizeof(upstream_reply_addr);
            unsigned short relay_id;
            int upstream_received = recvfrom(
                upstream_sock,
                upstream_buffer,
                sizeof(upstream_buffer),
                0,
                (struct sockaddr *)&upstream_reply_addr,
                &upstream_reply_len
            );

            if (upstream_received == SOCKET_ERROR) {
                fprintf(stderr, "upstream recvfrom failed: %d\n", WSAGetLastError());
                break;
            }

            relay_id = dns_read_id((const unsigned char *)upstream_buffer, upstream_received);
            entry = relay_table_find(&relay_table, relay_id);
            if (entry != NULL) {
                dns_set_id((unsigned char *)upstream_buffer, upstream_received, entry->original_id);
                sendto(
                    sock,
                    (const char *)upstream_buffer,
                    upstream_received,
                    0,
                    (const struct sockaddr *)&entry->client_addr,
                    sizeof(entry->client_addr)
                );
                printf(
                    "upstream response id=%u restored to original id=%u and sent back to client, active=%d\n",
                    relay_id,
                    entry->original_id,
                    relay_table_active_count(&relay_table) - 1
                );
                fflush(stdout);
                relay_table_remove(entry);
            } else {
                printf("upstream response id=%u has no active relay entry\n", relay_id);
                fflush(stdout);
            }
        }
    }

    closesocket(upstream_sock);
    closesocket(sock);
    return 0;
}
