#ifndef DNSRELAY_HOSTS_H
#define DNSRELAY_HOSTS_H

#define HOSTS_MAX_RECORDS 4096
#define HOSTS_MAX_DOMAIN_LEN 255

/* 一条本地规则记录：
 * domain 是域名，ip_text 是文本形式的 IPv4 地址。 */
typedef struct HostRecord {
    char domain[HOSTS_MAX_DOMAIN_LEN + 1];
    char ip_text[16];
    int is_blocked;
} HostRecord;

/* 当前阶段先用最简单的定长数组存规则。 */
typedef struct HostsTable {
    HostRecord records[HOSTS_MAX_RECORDS];
    int count;
} HostsTable;

int hosts_init(HostsTable *table);
int hosts_load(HostsTable *table, const char *file_path);
const HostRecord *hosts_find(const HostsTable *table, const char *domain);
void hosts_free(HostsTable *table);

#endif
