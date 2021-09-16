/*
 * list.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef _CACHE_BUFFER_H_
#define _CACHE_BUFFER_H_
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
typedef struct cache_context
{
    uint8_t*    pcache_buf;
    int         nmax_cache_len;
    int         nbegin;
    int         nend;
    pthread_mutex_t* pmutex;
} cache_ctx;

#ifndef lbtrace
#define lbtrace printf
#endif
#ifndef lberror
#define lberror printf
#endif

#define INIT_RECURSIVE_MUTEX(pmutex) pthread_mutexattr_t attr; \
memset(&attr, 0, sizeof(attr)); \
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
pthread_mutex_init(pmutex, &attr);

static cache_ctx* cache_context_open(int max_cache_size)
{
    cache_ctx* pcache = (cache_ctx*)malloc(sizeof(cache_ctx));
    pcache->pcache_buf = (uint8_t*)malloc(max_cache_size);
    pcache->nmax_cache_len = max_cache_size;
    pcache->nbegin = 0;
    pcache->nend = 0;
    pcache->pmutex = malloc(sizeof(pthread_mutex_t));
    assert(pcache->pmutex);
    INIT_RECURSIVE_MUTEX(pcache->pmutex);

    return pcache;
}

static void cache_context_closep(cache_ctx** ppcache_ctx)
{
    if(ppcache_ctx && *ppcache_ctx)
    {
        cache_ctx* pcache = *ppcache_ctx;
        pthread_mutex_lock(pcache->pmutex);
        free(pcache->pcache_buf);
        pthread_mutex_unlock(pcache->pmutex);
        pthread_mutex_destroy(pcache->pmutex);
        free(pcache->pmutex);
        free(pcache);
        *ppcache_ctx = NULL;
    }
}
static void cache_buffer_flush(cache_ctx* pcache)
{
    if(NULL == pcache)
    {
        return ;
    }

    pthread_mutex_lock(pcache->pmutex);
    if(pcache->nbegin > 0)
    {
        int bmalloc = 0;
        uint8_t* pcopybuf = NULL;
        int copylen = pcache->nend - pcache->nbegin;
        if(pcache->nbegin < pcache->nend - pcache->nbegin)
        {
            pcopybuf = malloc(copylen);
            memcpy(pcopybuf, pcache->pcache_buf + pcache->nbegin, copylen);
            bmalloc = 1;
        }
        else
        {
            pcopybuf = pcache->pcache_buf + pcache->nbegin;
        }
        memcpy(pcache->pcache_buf, pcopybuf, copylen);
        pcache->nbegin = 0;
        pcache->nend = copylen;
        if(bmalloc)
        {
            free(pcopybuf);
        }
    }
    pthread_mutex_unlock(pcache->pmutex);
}
static int cache_deliver_data(cache_ctx* pcache, uint8_t* pdata, int len)
{
    if(!pcache || !pdata)
    {
        lberror("Invalid parameter !pcache:%p || !pdata:%p\n", pcache, pdata);
        return -1;
    }
    pthread_mutex_lock(pcache->pmutex);
    if(pcache->nmax_cache_len - pcache->nend + pcache->nbegin < len)
    {
        lberror("no enought cache buffer for data cache, remain:%d, len:%d\n", pcache->nmax_cache_len - pcache->nend + pcache->nbegin, len);
        pthread_mutex_unlock(pcache->pmutex);
        return -1;
    }
    
    if(pcache->nmax_cache_len - pcache->nend < len)
    {
        cache_buffer_flush(pcache);
    }
    memcpy(pcache->pcache_buf + pcache->nend, pdata, len);
    pcache->nend += len;
    pthread_mutex_unlock(pcache->pmutex);
    return 0;
}

static int cache_fetch_data(cache_ctx* pcache, uint8_t* pdata, int len)
{
    if(!pcache || !pdata)
    {
        lberror("Invalid parameter !pcache:%p || !pdata:%p\n", pcache, pdata);
        return -1;
    }
    
    pthread_mutex_lock(pcache->pmutex);
    if(pcache->nend - pcache->nbegin < len)
    {
        pthread_mutex_unlock(pcache->pmutex);
        return 0;
    }
    
    memcpy(pdata, pcache->pcache_buf + pcache->nbegin, len);
    pcache->nbegin += len;
    pthread_mutex_unlock(pcache->pmutex);
    return len;
}
#endif
