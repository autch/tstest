
#ifndef pidmap_h
#define pidmap_h

#include "tstest.h"
#include "tstest_types.h"

int pidmap_init(TSTEST* ctx);
void pidmap_destroy(TSTEST* ctx);
PIDMAP* pidmap_findtail(TSTEST* ctx);
PIDMAP* pidmap_findpid(TSTEST* ctx, uint16_t pid);
int pidmap_register(TSTEST* ctx, uint16_t pid, tstest_pid_fn fn);

#endif // !pidmap_h
