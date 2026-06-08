#include "relay_table.h"

#include <string.h>

void relay_table_init(RelayTable *table) {
    memset(table, 0, sizeof(*table));
}

int relay_table_add(
    RelayTable *table,
    unsigned short relay_id,
    unsigned short original_id,
    const struct sockaddr_in *client_addr,
    unsigned long created_tick
) {
    int i;

    for (i = 0; i < RELAY_TABLE_MAX; ++i) {
        if (!table->entries[i].active) {
            table->entries[i].relay_id = relay_id;
            table->entries[i].original_id = original_id;
            table->entries[i].client_addr = *client_addr;
            table->entries[i].created_tick = created_tick;
            table->entries[i].active = 1;
            return 0;
        }
    }

    return 1;
}

RelayEntry *relay_table_find(RelayTable *table, unsigned short relay_id) {
    int i;

    for (i = 0; i < RELAY_TABLE_MAX; ++i) {
        if (table->entries[i].active && table->entries[i].relay_id == relay_id) {
            return &table->entries[i];
        }
    }

    return NULL;
}

void relay_table_remove(RelayEntry *entry) {
    entry->active = 0;
}

int relay_table_active_count(const RelayTable *table) {
    int i;
    int count = 0;

    for (i = 0; i < RELAY_TABLE_MAX; ++i) {
        if (table->entries[i].active) {
            count++;
        }
    }

    return count;
}
