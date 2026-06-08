#include "relay_table.h"
#include "timeout.h"

#include <assert.h>
#include <string.h>

int main(void) {
    RelayTable table;
    RelayEntry *entry;
    struct sockaddr_in client_addr;
    int removed_count;

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(12345);
    client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    relay_table_init(&table);
    assert(relay_table_active_count(&table) == 0);

    assert(relay_table_add(&table, 5001, 1001, &client_addr, 1000) == 0);
    assert(relay_table_active_count(&table) == 1);
    entry = relay_table_find(&table, 5001);
    assert(entry != NULL);
    assert(entry->original_id == 1001);
    assert(ntohs(entry->client_addr.sin_port) == 12345);

    removed_count = relay_table_remove_expired(&table, 2000, 500);
    assert(removed_count == 1);
    assert(relay_table_active_count(&table) == 0);
    entry = relay_table_find(&table, 5001);
    assert(entry == NULL);

    return 0;
}
