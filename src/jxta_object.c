/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact Project JXTA at http://www.jxta.org.
 *
 * 5. Products derived from this software may not be called "JXTA",
 *    nor may "JXTA" appear in their name, without prior written
 *    permission of Sun.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of Project JXTA.  For more
 * information on Project JXTA, please see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id: jxta_object.c,v 1.29 2005/01/31 20:44:35 slowhog Exp $
 */

/**
    Force JXTA_LOG_ENABLE for this file.
**/
#define JXTA_LOG_ENABLE

#include "jxtaapr.h"
#include "jxta_debug.h"
#include "jxta_object.h"

#define MAX_REF_COUNT 1000

/**
 * Forward definition. Makes compilers happier.
 **/
static void jxta_object_increment_refcount(Jxta_object * obj);

static int jxta_object_decrement_refcount(Jxta_object * obj);

static int jxta_object_get_refcount(Jxta_object * obj);

static void jxta_object_set_refcount(Jxta_object * obj, int count);

/**
    Change this to #define to enable the KDB Server. The KDB server
    allows you to remotely see what JXTA objects are being tracked.
**/
#undef KDB_SERVER

#ifdef KDB_SERVER
void start_kdb_server(void);
#endif

/**
 * This module uses a global mutex in order to protect its
 * critical section, such as changing the reference count of an
 * object. In therory, there should be a mutex per object, but since
 * incrementing or decrementing the reference count is very fast,
 * it is better to save some memory (only one mutex allocated versus
 * one per object).
 **/

apr_thread_mutex_t *jxta_object_mutex;
apr_pool_t *jxta_object_pool;
boolean jxta_object_initialized = FALSE;

/**
 * Initialize the global mutex. Since we do want this module
 * to be initialized "by itself", in other words, when it's first
 * used, this function must be idempotent.
 *
 * Note that this function cannot be called concurrentely. However,
 * there is a risk to have it invoked concurrentely. The final test
 * on jxta_object_initialized should catch most of the problems.
 * lomax@jxta.org: to be revisited. The proper way to initialize this
 * module would be to have a static initializer invoked at boot time.
 * jice@jxta.org - 20020204 : Actualy, the POSIX way is to use thread-once
 * because static initialization order is practicaly impossible to control.
 **/
void jxta_object_initialize(void)
{
    apr_status_t res;

    if (jxta_object_initialized) {
        /* Already initialized. Nothing to do. */
        return;
    }
    /* Initialize the global mutex. */
    res = apr_pool_create(&jxta_object_pool, NULL);
    if (res != APR_SUCCESS) {
        JXTA_LOG("Cannot allocate pool for jxta_object_mutex\n");
        return;
    }
    /* Create the mutex */
    res = apr_thread_mutex_create(&jxta_object_mutex, APR_THREAD_MUTEX_NESTED, jxta_object_pool);
    if (res != APR_SUCCESS) {
        JXTA_LOG("Cannot jxta_object_mutex\n");
        return;
    }
    if (jxta_object_initialized) {
        /* This should not be the case */
        JXTA_LOG("jxta_object_initialized invoked concurrently !!!\n");
    }
#ifdef KDB_SERVER
    start_kdb_server();
#endif

    jxta_object_initialized = TRUE;
}

/**
 * Get the mutex.
 **/
static void jxta_object_mutexGet(void)
{
    apr_status_t res;

    jxta_object_initialize();
    res = apr_thread_mutex_lock(jxta_object_mutex);
    if (res != APR_SUCCESS) {
        JXTA_LOG("Error getting jxta_object_mutex\n");
        return;
    }
}

/**
 * Release the mutex.
 **/
static void jxta_object_mutexRel(void)
{
    apr_status_t res;

    jxta_object_initialize();
    res = apr_thread_mutex_unlock(jxta_object_mutex);
    if (res != APR_SUCCESS) {
        JXTA_LOG("Error getting jxta_object_mutex\n");
        return;
    }
}

static void jxta_object_increment_refcount(Jxta_object * obj)
{
    jxta_object_mutexGet();
    obj->_refCount++;
    jxta_object_mutexRel();
}

static int jxta_object_decrement_refcount(Jxta_object * obj)
{
    int tmp;
    jxta_object_mutexGet();
    tmp = --obj->_refCount;
    jxta_object_mutexRel();
    return tmp;
}

static int jxta_object_get_refcount(Jxta_object * obj)
{
    int count;
    jxta_object_mutexGet();
    count = obj->_refCount;
    jxta_object_mutexRel();
    return count;
}

static void jxta_object_set_refcount(Jxta_object * obj, int count)
{
    jxta_object_mutexGet();
    obj->_refCount = count;
    jxta_object_mutexRel();
}

Jxta_boolean _jxta_object_check_valid(Jxta_object * obj, const char *file, int line)
{
    Jxta_boolean res;

    if (obj == NULL) {
        JXTA_LOG_NOLINE("[%s:%d] *********** Object is NULL\n", file, line);
        return FALSE;
    }
    jxta_object_mutexGet();

#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE

    res = _jxta_object_check_initialized(obj, file, line);
    if (!res)
        goto Common_Exit;
#endif

    res = ((obj->_refCount > 0) && (obj->_refCount < MAX_REF_COUNT));
    if (!res) {
        JXTA_LOG_NOLINE("[%s:%d] *********** Object [%p] is not valid (ref count is %d)\n",
                        file, line, (void *) obj, obj->_refCount);
    }

  Common_Exit:
    jxta_object_mutexRel();

    return res;
}


/*
 * Just in case someone tries to free a static object.
 * We need to make sure that the reference count is back to one.
 */
void jxta_object_freemenot(Jxta_object * addr)
{

    if (NULL == addr)
        return;

    JXTA_OBJECT_SET_INITIALIZED(addr);
    JXTA_OBJECT_SHARE(addr);
}

#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
Jxta_boolean _jxta_object_check_initialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        JXTA_LOG_NOLINE("[%s:%d] JXTA-OBJECT: object is NULL\n", file, line);
        return FALSE;
    }

    if (JXTA_OBJECT(obj)->_initialized != JXTA_OBJECT_MAGIC_NUMBER) {
        JXTA_LOG_NOLINE("[%s:%d] JXTA-OBJECT: shared object not initialized\n", file, line);
        return FALSE;
    }

    return TRUE;
}
#endif

#ifdef JXTA_OBJECT_CHECK_UNINITIALIZED_ENABLE
Jxta_boolean _jxta_object_check_not_initialized(Jxta_object * obj, const char *file, int line)
{
    if (NULL == obj) {
        JXTA_LOG_NOLINE("[%s:%d] JXTA-OBJECT: object is NULL\n", file, line);
        return TRUE;
    }

    if (JXTA_OBJECT(obj)->_initialized == JXTA_OBJECT_MAGIC_NUMBER) {
        JXTA_LOG_NOLINE("[%s:%d] JXTA-OBJECT: object is already initialized\n", file, line);
        return TRUE;
    }

    return FALSE;
}
#endif

#ifdef JXTA_OBJECT_TRACKER
static void count_freed(Jxta_object *);
static void count_init(Jxta_object *);
#endif

void *_jxta_object_init(Jxta_object * obj, JXTA_OBJECT_FREE_FUNC freefunc, void *cookie, const char *file, int line)
{

#ifdef JXTA_OBJECT_TRACKER
    char *pt = malloc(strlen(file) + 20);
    sprintf(pt, "%s:%d", file, line);
#endif

#ifdef JXTA_OBJECT_CHECK_UNINITIALIZED_ENABLE
    (void) _jxta_object_check_not_initialized(obj, file, line);
#endif
    JXTA_OBJECT(obj)->_bitset = 0;
    JXTA_OBJECT(obj)->_refCount = 1;
    JXTA_OBJECT(obj)->_free = freefunc;
    JXTA_OBJECT(obj)->_freeCookie = (void *) (cookie);
#ifdef JXTA_OBJECT_TRACKER

    JXTA_OBJECT(obj)->_name = pt;
    count_init(obj);
#endif

    JXTA_OBJECT_SET_INITIALIZED(obj);

    return obj;
}


void *_jxta_object_share(Jxta_object * obj, const char *file, int line)
{
    if (!_jxta_object_check_initialized(obj, file, line))
        return NULL;

    jxta_object_increment_refcount(JXTA_OBJECT(obj));
    return obj;
}

void _jxta_object_release(Jxta_object * obj, const char *file, int line)
{
    if (!_jxta_object_check_valid(obj, file, line))
        return;

    if (jxta_object_decrement_refcount(JXTA_OBJECT(obj)) == 0) {
#ifdef JXTA_OBJECT_TRACKER
        count_freed(obj);
#endif

        JXTA_OBJECT_SET_UNINITIALIZED(obj);
        if (NULL != JXTA_OBJECT_GET_FREE_FUNC(obj))
            JXTA_OBJECT_GET_FREE_FUNC(obj) ((void *) (obj));
    }
}




#ifdef JXTA_OBJECT_TRACKER

/****************************************************
 ** Object tracker for debug purposes
 **
 ** We cannot use any of the tools (vector, hashtable,
 ** jstring...) since we want to use this tool in order
 ** to monitor them as well.
 ****************************************************/

#define MAX_NB_OF_ENTRIES 5000

typedef struct {
    char *name;
    int created;
    int freed;
    int current;
} _object_entry;

_object_entry *_jxta_object_table[MAX_NB_OF_ENTRIES];
int _nbOfObjectEntries = 0;

static void add_new_entry(char *name)
{

    _object_entry *o = NULL;

    jxta_object_mutexGet();

    if (_nbOfObjectEntries >= MAX_NB_OF_ENTRIES) {
        JXTA_LOG("********** JXTA OBJECT TRACKER: cannot allocate new object\n");
        jxta_object_mutexRel();
        return;
    }
    o = malloc(sizeof(_object_entry));
    o->name = malloc(strlen(name) + 1);
    strcpy(o->name, name);
    o->created = 0;
    o->freed = 0;
    o->current = 0;

    _jxta_object_table[_nbOfObjectEntries++] = o;
    jxta_object_mutexRel();
}

static _object_entry *get_object_entry(Jxta_object * obj)
{

    _object_entry *o = NULL;
    int i = 0;

    jxta_object_mutexGet();
    /**
     ** 3/16/2002 FIXME lomax@jxta.org
     ** I know: the following code is not efficient. Since this code runs
     ** only when JXTA_OBJECT_TRACKER is enabled, which is not the default,
     ** this is not a huge issue.
     ** However, something smarter could be done (any volonteer is welcome),
     ** like, for instance, sorting the table in an alphabitical order: the
     ** overhead is when creating a new type of object, which is a rare case,
     ** the typical case, updating an existing type of object, being then optimized
     ** by a dichotomic search... or whatever is smarter to do anyway :-)
     **/
    for (i = 0; i < _nbOfObjectEntries; ++i) {
        o = _jxta_object_table[i];
        if (strcmp(o->name, obj->_name) == 0) {
            jxta_object_mutexRel();
            return o;
        }
    }
    jxta_object_mutexRel();
    return NULL;
}

static void count_freed(Jxta_object * obj)
{

    _object_entry *o = get_object_entry(obj);
    if (o == NULL) {
        JXTA_LOG("********** JXTA OBJECT TRACKER: cannot find object\n");
        return;
    }

    jxta_object_mutexGet();
    o->current--;
    o->freed++;
    jxta_object_mutexRel();

}

static void count_init(Jxta_object * obj)
{

    _object_entry *o = get_object_entry(obj);

    if (o == NULL) {
        add_new_entry(obj->_name);
        o = get_object_entry(obj);
        if (o == NULL) {
            return;
        }
    }

    jxta_object_mutexGet();
    o->current++;
    o->created++;
    jxta_object_mutexRel();
}

void _print_object_table(void)
{

    _object_entry *o = NULL;
    int i = 0;

    printf("\nJXTA-C Object Tracker: %d types of objects\n", _nbOfObjectEntries);
    for (i = 0; i < _nbOfObjectEntries; ++i) {
        o = _jxta_object_table[i];
        printf("%-40screated=%8d freed=%8d current=%8d\n", o->name, o->created, o->freed, o->current);
    }
    printf("\n");

}

#endif

/*****************************************************************************
 ** 3/17/2002 FIXME lomax@jxta.org
 **
 ** The following code is temporary. It should be in its own file, and also
 ** be more sophisticated. But for the time being, it's useful:
 **
 ** telnet 127.0.0.1 10000  would make the jxta-c peer display its objects.
 *****************************************************************************/

#ifdef KDB_SERVER

#include "jpr/jpr_thread.h"
#include "jpr/jpr_excep.h"

#define KDB_SERVER_HOST  "127.0.0.1"
#define KDB_SERVER_PORT  10000


static void* APR_THREAD_FUNC kdb_server_main(apr_thread_t * thread, void *arg)
{

    apr_status_t status;
    apr_pool_t *pool;
    apr_socket_t *socket;
    apr_socket_t *sock_in;
    apr_sockaddr_t *sockaddr;

    apr_pool_create(&pool, 0);

    status = apr_sockaddr_info_get(&sockaddr, KDB_SERVER_HOST, APR_INET, KDB_SERVER_PORT, SOCK_STREAM, pool);

    if (APR_STATUS_IS_SUCCESS(status)) {

        status = apr_socket_create(&socket, APR_INET, SOCK_STREAM, pool);

        apr_setsocketopt(socket, APR_SO_REUSEADDR, 1);

        if (APR_STATUS_IS_SUCCESS(status)) {

            status = apr_bind(socket, sockaddr);
            if (APR_STATUS_IS_SUCCESS(status)) {
                status = apr_listen(socket, 50);
                if (APR_STATUS_IS_SUCCESS(status)) {

                    for (;;) {
                        printf("KDB Server: listening on 127.0.0.1 port 10000\n");
                        status = apr_accept(&sock_in, socket, pool);
                        if (APR_STATUS_IS_SUCCESS(status)) {
#ifdef JXTA_OBJECT_TRACKER
                            _print_object_table();
#endif

                            apr_socket_close(sock_in);
                        }
                    }
                }
            }
        }
    }
    apr_thread_exit(thread, 0);

    return NULL;
}


void start_kdb_server(void)
{

    apr_status_t status;
    apr_pool_t *pool;
    apr_thread_t *thread;

#ifdef JXTA_OBJECT_TRACKER

    apr_pool_create(&pool, 0);

    apr_thread_create(&thread, NULL,    /* no attr */
                      kdb_server_main, (void *) NULL, pool);
#else

    printf("************* Cannot start KDB server: JXTA_OBJECT_TRACKER is not enabled\n");
#endif
}
#endif
