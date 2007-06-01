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
 * THIS SOFTWARE IS PROVIDED AS IS'' AND ANY EXPRESSED OR IMPLIED
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
 * $Id: queue.c,v 1.20 2005/08/03 05:51:21 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "jxtaapr.h"

#include "jdlist.h"
#include "queue.h"
#include "jxta_errno.h"

struct _Queue {
    Dlist *list;
    int size;
    apr_thread_mutex_t *mutex;
    apr_thread_cond_t *cond;
};


Queue *queue_new(apr_pool_t * pool)
{
    Queue *q = (Queue *) malloc(sizeof(Queue));

    q->list = dl_make();
    q->size = 0;
    apr_thread_mutex_create(&q->mutex, APR_THREAD_MUTEX_DEFAULT, pool);

    apr_thread_cond_create(&q->cond, pool);


    return q;
}

void queue_free(Queue * q)
{
    dl_free(q->list, NULL);
    apr_thread_mutex_destroy(q->mutex);
    apr_thread_cond_destroy(q->cond);

    free(q);
}

int queue_size(Queue * q)
{
    int size;
    apr_thread_mutex_lock(q->mutex);
    size = q->size;
    apr_thread_mutex_unlock(q->mutex);
    return size;
}

int queue_enqueue(Queue * q, void *item)
{
    int size;

    apr_thread_mutex_lock(q->mutex);

    dl_insert_b(q->list, item);
    size = ++q->size;
    apr_thread_cond_signal(q->cond);

    apr_thread_mutex_unlock(q->mutex);
    return size;
}

void *queue_dequeue(Queue * q, apr_time_t max_timeout)
{
    void *obj;

    queue_dequeue_1(q, &obj, max_timeout);
    return obj;
}

Jxta_status queue_dequeue_1(Queue * q, void **obj, apr_time_t max_timeout)
{
    Dlist *head;
    /* apr_time_t time = apr_time_now (); */
    apr_time_t wakeup_time;
    apr_status_t status;

    apr_thread_mutex_lock(q->mutex);

    wakeup_time = apr_time_now() + max_timeout;

    head = dl_first(q->list);
    while (max_timeout > 0 && head == q->list) {
        status = apr_thread_cond_timedwait(q->cond, q->mutex, max_timeout);
        if (status == 0) {
            max_timeout = wakeup_time - apr_time_now();
        } else {
            /* timeout expired while we were waiting */
            apr_thread_mutex_unlock(q->mutex);
            *obj = NULL;
            return JXTA_TIMEOUT;
        }
        head = dl_first(q->list);
    }

    /* List is not empty. Remove the element */
    *obj = head->val;
    dl_delete_node(head);
    --q->size;

    apr_thread_mutex_unlock(q->mutex);

    return JXTA_SUCCESS;
}

#ifdef STANDALONE

void *APR_THREAD_FUNC dequeuer(apr_thread_t * t, void *arg)
{
    Queue *q = arg;

    printf("Dequeued %s\n", (char *) queue_dequeue(q, 5000000));

    return NULL;
}

void *APR_THREAD_FUNC enqueuer(apr_thread_t * thread, void *arg)
{
    Queue *q = arg;

    queue_enqueue(q, (void *) "something");

    return NULL;
}

void *APR_THREAD_FUNC waster(apr_thread_t * thread, void *arg)
{
    apr_thread_mutex_t *mutex;
    apr_thread_cond_t *cond;

    apr_thread_cond_create(&cond, (apr_pool_t *) arg);
    apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT, (apr_pool_t *) arg);

    apr_thread_mutex_lock(mutex);
    apr_thread_cond_wait(cond, mutex);
    apr_thread_mutex_unlock(mutex);

    return NULL;
}


int main(int argc, char **argv)
{
    apr_thread_t *deq_thread;
    apr_thread_t *enq_thread;
    apr_thread_t *waster_thread;
    apr_pool_t *pool;
    Queue *queue;
    apr_status_t status;

    apr_pool_create(&pool, NULL);
    queue = queue_new(pool);

    printf("starting...\n");


    apr_thread_create(&deq_thread, NULL,    /* no attr */
                      dequeuer, queue, pool);

    apr_thread_create(&enq_thread, NULL, enqueuer, queue, pool);

    apr_thread_create(&waster_thread, NULL, waster, pool, pool);

    apr_thread_join(&status, deq_thread);

    return 0;
}

#endif

/* vim: set ts=4 sw=4 et tw=130: */
