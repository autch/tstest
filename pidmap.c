#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdint.h>
#include <stdlib.h>

#include "tstest.h"
#include "pidmap.h"

int pidmap_init(TSTEST* ctx)
{
    int i;

    ctx->pidmap_count = PIDMAP_DEFAULT_SIZE;
    ctx->pidmap = calloc(ctx->pidmap_count, sizeof(PIDMAP));
    for(i = 0; i < ctx->pidmap_count; i++)
    {
        ctx->pidmap[i].pid = PIDMAP_NULL;
        ctx->pidmap[i].fn = NULL;
    }
    return 0;
}

void pidmap_destroy(TSTEST* ctx)
{
    if(ctx->pidmap != NULL)
        free(ctx->pidmap);
    ctx->pidmap = NULL;
}

PIDMAP* pidmap_findtail(TSTEST* ctx)
{
    return pidmap_findpid(ctx, PIDMAP_NULL);
}

PIDMAP* pidmap_findpid(TSTEST* ctx, uint16_t pid)
{
    int i;
    for(i = 0; i < ctx->pidmap_count; i++)
    {
        if(ctx->pidmap[i].pid == pid)
        {
            if(pid != PIDMAP_NULL && i > 0)
            {
                PIDMAP temp;

                temp = ctx->pidmap[i - 1];
                ctx->pidmap[i - 1] = ctx->pidmap[i];
                ctx->pidmap[i] = temp;
                i--;
            }
            return &ctx->pidmap[i];
        }
    }
    return NULL;
}

int pidmap_register(TSTEST* ctx, uint16_t pid, tstest_pid_fn fn)
{
    PIDMAP* item;

    item = pidmap_findtail(ctx);
    if(item != NULL)
    {
        item->pid = pid & 0x1fff;
        item->fn = fn;
        return 0;
    }
    else
    {
        size_t new_count = ctx->pidmap_count + PIDMAP_DEFAULT_SIZE;
        PIDMAP* new_map;
        int i;

        new_map = realloc(ctx->pidmap, sizeof(PIDMAP) * new_count);
        if(new_map == NULL) return -1;

        new_map[ctx->pidmap_count].pid = pid & 0x1fff;
        new_map[ctx->pidmap_count].fn = fn;

        for(i = ctx->pidmap_count + 1; i < new_count; i++)
        {
            new_map[i].pid = PIDMAP_NULL;
            new_map[i].fn = NULL;
        }

        ctx->pidmap = new_map;
        ctx->pidmap_count = new_count;
    }
    return 0;
}
