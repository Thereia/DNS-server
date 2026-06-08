#ifndef DNSRELAY_RELAY_TABLE_H
#define DNSRELAY_RELAY_TABLE_H

#include <winsock2.h>

#define RELAY_TABLE_MAX 256

/* 一条“转发中”的请求记录。
 * relay_id 是发给上游时使用的内部 ID。
 * original_id 是客户端原始请求 ID。 */
typedef struct RelayEntry {
    unsigned short relay_id;
    unsigned short original_id;
    struct sockaddr_in client_addr;
    int active;
    unsigned long created_tick;
} RelayEntry;

typedef struct RelayTable {
    RelayEntry entries[RELAY_TABLE_MAX];
} RelayTable;

void relay_table_init(RelayTable *table);
int relay_table_add(
    RelayTable *table,
    unsigned short relay_id,
    unsigned short original_id,
    const struct sockaddr_in *client_addr,
    unsigned long created_tick
);
RelayEntry *relay_table_find(RelayTable *table, unsigned short relay_id);
void relay_table_remove(RelayEntry *entry);
int relay_table_active_count(const RelayTable *table);

#endif
