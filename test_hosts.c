#include "hosts.h"

#include <assert.h>
#include <string.h>

int main(void) {
    HostsTable table;
    const HostRecord *record;

    assert(hosts_init(&table) == 0);
    assert(hosts_load(&table, "dnsrelay.txt") == 0);

    /* 这两条记录都来自当前实际使用的 dnsrelay.txt。 */
    record = hosts_find(&table, "008.cn");
    assert(record != NULL);
    assert(strcmp(record->ip_text, "0.0.0.0") == 0);
    assert(record->is_blocked == 1);

    record = hosts_find(&table, "www.y.com.cn");
    assert(record != NULL);
    assert(strcmp(record->ip_text, "202.108.33.89") == 0);
    assert(record->is_blocked == 0);

    record = hosts_find(&table, "nonexistent.example");
    assert(record == NULL);

    hosts_free(&table);
    return 0;
}
