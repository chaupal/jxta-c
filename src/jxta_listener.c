/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * $Id: jxta_listener.c,v 1.36 2005/07/22 03:12:51 slowhog Exp $
 */

#include <stdlib.h>

#include "jxtaapr.h"

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_listener.h"
#include "jxta_log.h"

#include "queue.h"

static const char *__log_cat = "LISTENER";

/**********************************************************************
 ** This file implements Jxta_listener. Please refer to jxta_listener.h
 ** for detail on the API.
 **********************************************************************/

struct _jxta_listener {
    JXTA_OBJECT_HANDLE;
    Jxta_listener_func func;
    void *arg;
    int maxNbOfInvoke;
    int maxQueueSize;
    Jxta_boolean started;
    volatile int nbOfThreads;
    volatile int nbOfBusyThreads;
    Queue *queue;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
};


static const Jxta_time_diff WAITING_DELAY = 5 * 60 * 1000 * 1000;

static void _listener_stop(Jxta_listener * self)
{
    int i;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Stopping listener [%p], signal %u working threads\n", self,
                    self->nbOfThreads);
    self->started = FALSE;

    /* To awake all waiting threads */
    for (i = self->nbOfThreads; i > 0; i--) {
        queue_enqueue(self->queue, NULL);
    }

    apr_thread_mutex_unlock(self->mutex);
    /* Busy thread will stop once it finish processing */
    while (self->nbOfThreads > self->nbOfBusyThreads) {
        apr_sleep(20 * 1000);   /* 20 ms */
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                    "There is(are) %d worker thread(s) for listener[%p] still busy, will be stopped shortly\n",
                    self->nbOfBusyThreads, self);
    apr_thread_mutex_lock(self->mutex);
}

/**
 ** Free the shared dlist (invoked when the shared dlist is no longer used by anyone.
 **    - release the objects the shared list contains
 **    - free the dlist
 **    - free the mutexes
 **    - free the shared dlist object itself
 **/
static void jxta_listener_free(Jxta_object * listener)
{
    Jxta_listener *self = (Jxta_listener *) listener;

    if (self == NULL) {
        return;
    }

    /*
     * We need to take the lock in order to make sure that nobody
     * else is using the listener. However, since we are destroying
     * the object, there is no need to unlock. In other words, since
     * jxta_listener_free is not a public API, if the listener has been
     * properly shared, there should not be any external code that still
     * has a reference on this listener. So things should be safe.
     */
    apr_thread_mutex_lock(self->mutex);

    if (self->started) {
        _listener_stop(self);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting %d worker thread(s) for listener[%p] to stop ...\n",
                    self->nbOfThreads, self);
    apr_thread_mutex_unlock(self->mutex);
    while (self->nbOfThreads > 0) {
        apr_sleep(20 * 1000);   /* 20 ms */
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "All worker threads for listener[%p] have been stopped\n", self);
    apr_thread_mutex_lock(self->mutex);

    /* Release all the object contained in the listener */
    if (self->queue != NULL) {
        queue_free(self->queue);
        self->queue = NULL;
    }

    apr_thread_mutex_unlock(self->mutex);
    apr_thread_mutex_destroy(self->mutex);
    /* Free the pool containing the mutex */
    apr_pool_destroy(self->pool);
    /* Free the object itself */
    free(self);
}

/************************************************************************
 ** Allocates a new Jxta_listener. The listener is allocated and is initialized
 ** (ready to use). No need to apply JXTA_OBJECT_INIT, because new does it.
 **
 ** Note that a listener is created in a stop mode. See jxta_listener_start()
 ** and jxta_listener_stop().
 **
 ** The creator of a listener is responsible to release it when not used
 ** anymore.
 **
 ** @param func is a pointer to the function to be called when there is an
 ** event to process.
 ** @param maxNbOfInvoke is the maximum number of concurrent invokation of the
 ** the function func.
 ** @param maxQueueSize is the maximum number of event the queue associated
 ** to the listener will store. When the queue is full, the oldest element
 ** is removed, and then lost.
 ** @returns a new Jxta_listener or NULL if the system runs out of memory.
 *************************************************************************/

JXTA_DECLARE(Jxta_listener *) jxta_listener_new(Jxta_listener_func func, void *arg, int maxNbOfInvoke, int maxQueueSize)
{

    apr_status_t res;
    Jxta_listener *self = (Jxta_listener *) calloc(1, sizeof(Jxta_listener));

    if (self == NULL) {
        /* No more memory */
        return NULL;
    }

    /* Initialize it. */
    JXTA_OBJECT_INIT(self, jxta_listener_free, NULL);

    /*
     * Allocate the mutex 
     * Note that a pool is created for each listener. This allows to have a finer
     * granularity when allocated the memory. This constraint comes from the APR.
     */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        free(self);
        return NULL;
    }
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->func = func;
    self->arg = arg;
    self->maxNbOfInvoke = maxNbOfInvoke;
    self->maxQueueSize = maxQueueSize;
    self->started = FALSE;
    self->nbOfThreads = 0;
    self->nbOfBusyThreads = 0;

    /*
     * Creates the queue.
     */
    self->queue = queue_new(self->pool);
    return self;
}

/************************************************************************
 ** Start a listener.
 ** The provided listener function is invoked for already queued event and
 ** new events.
 **
 ** @param listener a pointer to the listener to use.
 *************************************************************************/
JXTA_DECLARE(void) jxta_listener_start(Jxta_listener * self)
{

    JXTA_OBJECT_CHECK_VALID(self);

    apr_thread_mutex_lock(self->mutex);
    self->started = TRUE;
    apr_thread_mutex_unlock(self->mutex);
}

/************************************************************************
 ** Stops a listener.
 ** 
 ** jxta_listener_stop will block until last thread that was invoking
 ** the listener function at the time jxta_listener_stop was called returns.
 ** In that respect, invoking jxta_listener_stop within the listener function
 ** itself is likely to cause a deadlock. Upon return, event are dropped until
 ** the listener is started again. The existing queue is flushed.
 **
 ** @param listener a pointer to the listener to use.
 *************************************************************************/

JXTA_DECLARE(void) jxta_listener_stop(Jxta_listener * self)
{
    JXTA_OBJECT_CHECK_VALID(self);

  /**
   ** 3/04/2002 lomax@jxta.org FIXME
   ** The following code does not wait until the last thread is processing
   ** an event, and it should do that.
   **/
    apr_thread_mutex_lock(self->mutex);
    _listener_stop(self);
    apr_thread_mutex_unlock(self->mutex);
}


static void *APR_THREAD_FUNC listener_thread_main(apr_thread_t * thread, void *arg)
{
    Jxta_listener *self = (Jxta_listener *) arg;
    Jxta_status rv;

    JXTA_OBJECT_CHECK_VALID(self);

    while (self->started) {
        Jxta_object *obj = NULL;
        JXTA_OBJECT_CHECK_VALID(self);
        rv = queue_dequeue_1(self->queue, (void **) &obj, WAITING_DELAY);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Dequeue a message[%p] for listener[%p], queue size left %d\n", obj,
                        self, queue_size(self->queue));

        if (JXTA_TIMEOUT == rv) {
            /* A timeout has triggered. Exit this thread */
            break;
        }

        apr_thread_mutex_lock(self->mutex);
        if (!self->started) {
            apr_thread_mutex_unlock(self->mutex);
            if (NULL != obj) {
                JXTA_OBJECT_RELEASE(obj);
            }
            break;
        }

        JXTA_OBJECT_CHECK_VALID(self);

        ++self->nbOfBusyThreads;
        apr_thread_mutex_unlock(self->mutex);

        /* Invoke the handler */

        self->func(obj, self->arg);
        if (NULL != obj)
            JXTA_OBJECT_RELEASE(obj);

        apr_thread_mutex_lock(self->mutex);
        --self->nbOfBusyThreads;
        apr_thread_mutex_unlock(self->mutex);
    }
    apr_thread_mutex_lock(self->mutex);
    --self->nbOfThreads;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Listener[%p] thread stopped, number of alive threads: %d\n", self,
                    self->nbOfThreads);
    apr_thread_mutex_unlock(self->mutex);

    apr_thread_exit(thread, 0);
    return NULL;
}


static void create_listener_thread(Jxta_listener * self)
{
    apr_thread_t *thread;

    JXTA_OBJECT_CHECK_VALID(self);

    apr_thread_mutex_lock(self->mutex);

    self->nbOfThreads++;
    apr_thread_create(&thread, NULL,    /* no attr */
                      listener_thread_main, (void *) self, (apr_pool_t *) self->pool);

    apr_thread_detach(thread);

    apr_thread_mutex_unlock(self->mutex);
}

/************************************************************************
 ** Schedule an event to the listener
 ** The event is either queued to be processed by the next available thread.
 ** Jxta_listener_scheduled_object is guaranted to not be blocking.
 **
 ** @param listener a pointer to the listener to use.
 ** @param object a pointer to the Jxta_object to schedule. Note that the
 ** object is automatically shared by this method.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

JXTA_DECLARE(Jxta_status) jxta_listener_schedule_object(Jxta_listener * self, Jxta_object * object)
{
    int size;
    Jxta_status rv = JXTA_SUCCESS;

    JXTA_OBJECT_CHECK_VALID(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Schedule event for listener[%p]\n", self);

    apr_thread_mutex_lock(self->mutex);

    JXTA_OBJECT_CHECK_VALID(self);

    if (0 >= self->maxQueueSize) {
        rv = jxta_listener_process_object(self, object);
        apr_thread_mutex_unlock(self->mutex);
        return rv;
    }

    if (!self->started) {
        /* Do nothing */
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Listener[%p] is stopped: event discarded\n", self);
        return JXTA_SUCCESS;
    }

    if (self->func != NULL) {
        if (self->nbOfBusyThreads == self->nbOfThreads) {
            if (self->nbOfThreads < self->maxNbOfInvoke) {
                /* We can create a new listener thread */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                                "Create additional worker thread for listener[%p] to existing %d ones\n",
                                self, self->nbOfThreads);
                create_listener_thread(self);
            }
        }
    }
  /**
   ** 3/04/2002 lomax@jxta.org FIXME
   ** Here we should check the size of the queue, and if the maximum size has been
   ** reached, remove the oldest element of the queue, and queue the new element.
   **/

    if (NULL != object)
        JXTA_OBJECT_SHARE(object);

    size = queue_enqueue(self->queue, (void *) object);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Enqueued message[%p] for listener[%p], queue size %d\n", object, self,
                    size);

    JXTA_OBJECT_CHECK_VALID(self);

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}


/************************************************************************
 ** Process an event to the listener
 ** The calling thread invokes the listener function itself. This is
 ** not guaranted to be a non blocking operation, and in fact, caller of
 ** this method should expect it to be potentially blocking.
 **
 ** @param listener a pointer to the listener to use.
 ** @param object a pointer to the Jxta_object to schedule. Note that the
 ** object is NOT shared by this method.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_listener_process_object(Jxta_listener * self, Jxta_object * object)
{

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(object);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Listener[%p] is processing event\n", self);

    apr_thread_mutex_lock(self->mutex);
    if (!self->started) {
        /* Do nothing */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Listener[%p] is stoped: event discarded\n", self);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    if (self->func) {
        apr_thread_mutex_unlock(self->mutex);
        self->func(object, self->arg);
    } else {
        JXTA_OBJECT_SHARE(object);
        queue_enqueue(self->queue, (void *) object);
        apr_thread_mutex_unlock(self->mutex);
    }
    return JXTA_SUCCESS;
}


/************************************************************************
 ** Returns the number of objects that are currentely in the queue.
 ** @param listener a pointer to the listener to use.
 ** @return the number of object that are currentley in the queue.
 *************************************************************************/
JXTA_DECLARE(int) jxta_listener_queue_size(Jxta_listener * self)
{
    return queue_size(self->queue);
}


JXTA_DECLARE(Jxta_status) jxta_listener_wait_for_event(Jxta_listener * self, Jxta_time_diff timeout, Jxta_object ** obj)
{
    Jxta_status rv;

    JXTA_OBJECT_CHECK_VALID(self);
    *obj = NULL;

    if (self->func != NULL) {
        return JXTA_BUSY;
    }

    rv = queue_dequeue_1(self->queue, (void **) obj, timeout);

    JXTA_OBJECT_CHECK_VALID(self);

    if (*obj == NULL) {
        return JXTA_TIMEOUT;
    } else {
        JXTA_OBJECT_CHECK_VALID(*obj);
        return JXTA_SUCCESS;
    }
}


/* vi: set ts=4 sw=4 tw=130 et: */
