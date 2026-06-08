#include "timeout.h"

int relay_table_remove_expired(RelayTable *table, unsigned long now_tick, unsigned long timeout_ms) {
    int i;
    int removed_count = 0;

    for (i = 0; i < RELAY_TABLE_MAX; ++i) {
        if (table->entries[i].active && now_tick - table->entries[i].created_tick > timeout_ms) {
            table->entries[i].active = 0;
            removed_count++;
        }
    }

    return removed_count;
}
