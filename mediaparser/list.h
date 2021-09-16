/*
 * list.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef _LIST_H_
#define _LIST_H_
#include <pthread.h>
#include <assert.h>
typedef struct list_node
{
    struct list_node* prev;
    struct list_node* pnext;
    void* pitem;
} node;

typedef struct list_context
{
    struct list_node* head;
    struct list_node* tail;
    int count;
    int max_num;
    pthread_mutex_t* pmutex;
} list_ctx;

static list_ctx* list_context_create(int max_num)
{
    list_ctx* plist = (list_ctx*)malloc(sizeof(list_ctx));
    memset(plist, 0, sizeof(list_ctx));
    plist->pmutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(plist->pmutex, &attr);
    plist->max_num = max_num;

    return plist;
}

static void list_context_close(list_ctx** pplistctx)
{
    if(pplistctx && *pplistctx)
    {
        list_ctx* plist = *pplistctx;
        pthread_mutex_destroy(plist->pmutex);
        free(plist->pmutex);
        free(plist);
        *pplistctx = NULL;
    }
}

static int push(list_ctx*  plist, void* pitem)
{
    if(NULL == plist)
    {
        assert(0);
        return -1;
    }
    pthread_mutex_lock(plist->pmutex);
    struct list_node* pnode = (struct list_node*)malloc(sizeof(struct list_node));
    pnode->pitem = pitem;
    if(plist->tail)
    {
        plist->tail->pnext = pnode;
        pnode->prev = plist->tail;
        pnode->pnext = NULL;
        plist->tail = pnode;
        plist->count++;
    }
    else
    {
        plist->head = pnode;
        plist->tail = pnode;
        pnode->prev = NULL;
        pnode->pnext = NULL;
        plist->count++;
    }
    assert(plist->head);
    pthread_mutex_unlock(plist->pmutex);
    return 0;
}

static void* pop(list_ctx*  plist)
{
    pthread_mutex_lock(plist->pmutex);
    if(NULL == plist && plist->count <= 0)
    {
        pthread_mutex_unlock(plist->pmutex);
        return NULL;
    }
    
    assert(plist->head);
    struct list_node* pnode = plist->head;
    plist->head = pnode->pnext;
    plist->count--;
    void* pitem = pnode->pitem;
    free(pnode);
    if(NULL == plist->head)
    {
        plist->tail = NULL;
        assert(0 == plist->count);
    }
    //assert(plist->head);
    pthread_mutex_unlock(plist->pmutex);
    return pitem;
}

static void* front(list_ctx*  plist)
{
    pthread_mutex_lock(plist->pmutex);
    if(NULL == plist && plist->count <= 0)
    {
        pthread_mutex_unlock(plist->pmutex);
        return NULL;
    }
    assert(plist->head);
    void* pitem = plist->head->pitem;
    pthread_mutex_unlock(plist->pmutex);
    return pitem;
}
static int list_size(list_ctx*  plist)
{
    return plist ? plist->count : 0;
}
#endif
