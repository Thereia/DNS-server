#ifndef DNSRELAY_COMMON_H
#define DNSRELAY_COMMON_H

/* 第一阶段故意不用 53 端口。
 * 这样可以先验证 UDP 收包逻辑，
 * 不会影响系统 DNS，也尽量避开系统端口冲突。 */
#define DNSRELAY_STAGE1_PORT 5533

/* 当前测试只需要一个固定大小的接收缓冲区。
 * 后面做 DNS 报文解析时，也仍然是“一次收一个 UDP 包”。 */
#define DNSRELAY_BUFFER_SIZE 1024

/* 第五阶段先使用一个固定上游 DNS，打通最小转发链路。 */
#define DNSRELAY_UPSTREAM_DNS "8.8.8.8"
#define DNSRELAY_UPSTREAM_PORT 53
#define DNSRELAY_RELAY_TIMEOUT_MS 5000

#endif
