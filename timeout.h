#ifndef DNSRELAY_TIMEOUT_H
#define DNSRELAY_TIMEOUT_H

#include "relay_table.h"

/* 删除超时的转发表项，并返回本次删掉了多少条。 */
int relay_table_remove_expired(RelayTable *table, unsigned long now_tick, unsigned long timeout_ms);

#endif
