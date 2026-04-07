#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FlexQL FlexQL;

int flexql_open(const char *host, int port, FlexQL **db);

int flexql_close(FlexQL *db);

int flexql_exec(
    FlexQL *db,
    const char *sql,
    int (*callback)(void *, int, char **, char **),
    void *arg,
    char **errmsg);

int flexql_exec_batch(
    FlexQL *db,
    const char *const *sql_statements,
    int statement_count,
    char **errmsg);

void flexql_free(void *ptr);

#ifdef __cplusplus
}
#endif
