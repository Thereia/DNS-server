#ifndef DNSRELAY_SERVER_H
#define DNSRELAY_SERVER_H

#include <winsock2.h>
#include "hosts.h"

/* 启动 UDP 接收循环。
 * 参数就是当前阶段最关心的两项：监听 IP 和监听端口。 */
int server_run(const char *listen_ip, unsigned short listen_port, const HostsTable *hosts_table);

#endif
