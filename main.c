#include "common.h"
#include "hosts.h"
#include "server.h"

#include <stdio.h>
#include <winsock2.h>

int main(void) {
    const char *listen_ip = "127.0.0.1";
    unsigned short listen_port = DNSRELAY_STAGE1_PORT;
    HostsTable hosts_table;
    WSADATA wsa_data;
    int exit_code;

    /* 现在只监听本机回环地址。
     * 这样所有测试都只发生在当前电脑上。 */

    /* 所有 Winsock 程序都要先初始化套接字环境。 */
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    /* 第二阶段先把本地规则表读进来。
     * 现在虽然还没有真正拿它处理 DNS 请求，
     * 但启动阶段已经具备“读取配置文件”的能力。 */
    if (hosts_init(&hosts_table) != 0) {
        fprintf(stderr, "hosts_init failed\n");
        WSACleanup();
        return 1;
    }

    if (hosts_load(&hosts_table, "dnsrelay.txt") != 0) {
        fprintf(stderr, "hosts_load failed\n");
        WSACleanup();
        return 1;
    }

    /* 真正的 UDP 监听和收包逻辑放在 server.c 里。 */
    exit_code = server_run(listen_ip, listen_port);

    hosts_free(&hosts_table);

    /* 程序结束前释放 Winsock 资源。 */
    WSACleanup();
    return exit_code;
}
