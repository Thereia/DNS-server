#include "hosts.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* 把域名统一转成小写，避免大小写影响查找。 */
static void lowercase_copy(char *dest, const char *src, size_t size) {
    size_t i;

    for (i = 0; src[i] != '\0' && i + 1 < size; ++i) {
        dest[i] = (char)tolower((unsigned char)src[i]);
    }
    dest[i] = '\0';
}

int hosts_init(HostsTable *table) {
    table->count = 0;
    return 0;
}

int hosts_load(HostsTable *table, const char *file_path) {
    FILE *fp;
    char ip[16];
    char domain[HOSTS_MAX_DOMAIN_LEN + 1];

    /* 每次重新加载前先清空旧内容，避免重复累加。 */
    table->count = 0;

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        return 1;
    }

    while (table->count < HOSTS_MAX_RECORDS && fscanf(fp, "%15s %255s", ip, domain) == 2) {
        HostRecord *record = &table->records[table->count];

        strcpy(record->ip_text, ip);
        lowercase_copy(record->domain, domain, sizeof(record->domain));
        record->is_blocked = (strcmp(ip, "0.0.0.0") == 0);
        table->count++;
    }

    fclose(fp);
    return 0;
}

const HostRecord *hosts_find(const HostsTable *table, const char *domain) {
    char normalized[HOSTS_MAX_DOMAIN_LEN + 1];
    int i;

    lowercase_copy(normalized, domain, sizeof(normalized));

    for (i = 0; i < table->count; ++i) {
        if (strcmp(table->records[i].domain, normalized) == 0) {
            return &table->records[i];
        }
    }

    return NULL;
}

void hosts_free(HostsTable *table) {
    table->count = 0;
}
