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
 * $Id: jxta_listener.h,v 1.9 2006/06/17 09:24:13 mmx2005 Exp $
 */


/**************************************************************************************************
 **  Jxta_listener
 **
 ** A Jxta_listener is a shared object that is used to handle events from other services.
 ** A Jxta_listener is associated to a queue of events and a threading mechanism:
 **
 ** On the listener side (the creator of the listener, and the receiver of the events), a
 ** listener is set to a maximum number of concururent invocation and a function to call, the
 ** "real" listener, which can be blocking: if the maximum number of concurrent invocation is
 ** reached, event are queued.
 **
 ** On the service side (the provider of the event), there is two manners to push an new event
 ** onto the listener:
 **
 **     - synchronous: the current thread (the service thread) will invoke the listener
 **       itself. This method is for service which do not care to have their threads blocked.
 **       In general, this is dangerous, but can be useful if done carefully, in order to
 **       minimize the number of thread switching.
 **
 **     - asynchronous: the prefered method: the listener will either schedule a new thread
 **       to process the event, or queue the event.
 ** 
 **
 **************************************************************************************************/

#ifndef JXTA_LISTENER_H
#define JXTA_LISTENER_H

#include "jxta_apr.h"
#include "jxta_object.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * Jxta_listener is the public type of the vector
 **/
typedef struct _jxta_listener Jxta_listener;

/**
 ** Prototype of the user function that eventually processes the event.
 **/
typedef void (JXTA_STDCALL *Jxta_listener_func) (Jxta_object * obj, void *arg);


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
 ** event to process. If func is set to NULL, then events are queued and can
 ** be received using jxta_listener_wait_for_event().
 ** @param arg is an optional value which is not processed by the listener
 ** but passed back to the listener function.
 ** @param maxNbOfInvoke is the maximum number of concurrent invokation of the
 ** the function func.
 ** @param maxQueueSize is the maximum number of event the queue associated
 ** to the listener will store. When the queue is full, the oldest element
 ** is removed, and then lost.
 ** @returns a new Jxta_listener or NULL if the system runs out of memory.
 *************************************************************************/

JXTA_DECLARE(Jxta_listener *)
    jxta_listener_new(Jxta_listener_func func, void *arg, int maxNbOfInvoke, int maxQueueSize);


/************************************************************************
 ** Start a listener.
 ** The provided listener function is invoked for already queued event and
 ** new events.
 **
 ** @param listener a pointer to the listener to use.
 *************************************************************************/
JXTA_DECLARE(void) jxta_listener_start(Jxta_listener * listener);

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

JXTA_DECLARE(void) jxta_listener_stop(Jxta_listener * listener);



/************************************************************************
 ** Schedule an event to the listener
 ** The event is either queued to be processed by the next available thread.
 ** Jxta_listener_scheduled_object is guaranteed to not be blocking.
 **
 ** @param listener a pointer to the listener to use.
 ** @param object a pointer to the Jxta_object to schedule. Note that the
 ** object is automatically shared by this method.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/

JXTA_DECLARE(Jxta_status)
    jxta_listener_schedule_object(Jxta_listener * listener, Jxta_object * object);


/************************************************************************
 ** Process an event to the listener
 ** The calling thread invokes the listener function itself. This is
 ** not guaranteed to be a non blocking operation, and in fact, caller of
 ** this method should expect it to be potentially blocking.
 **
 ** @param listener a pointer to the listener to use.
 ** @param object a pointer to the Jxta_object to schedule. Note that the
 ** object is NOT shared by this method.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status)
    jxta_listener_process_object(Jxta_listener * listener, Jxta_object * object);

JXTA_DECLARE(Jxta_status) jxta_listener_pool_object(Jxta_listener * listener, Jxta_object * object, apr_thread_pool_t *tpool);

/************************************************************************
 ** Returns the number of objects that are currently in the queue.
 ** @param listener a pointer to the listener to use.
 ** @return the number of object that are currently in the queue.
 *************************************************************************/
JXTA_DECLARE(int) jxta_listener_queue_size(Jxta_listener * listener);


/************************************************************************
 ** Blocks for an event to be triggered or until the timeout is reached.
 ** Note. This will fail if a function has been passed when creating the
 ** Jxta_listener.
 **
 ** @param self pointer to the Jxta_listener
 ** @param timeout delay maximum to wait for.
 ** @param obj return value: the event object
 ** @return JXTA_SUCCESS when successful,
 **         JXTA_BUSY when a handler function has already been set.
 **/
JXTA_DECLARE(Jxta_status)
    jxta_listener_wait_for_event(Jxta_listener * self, Jxta_time_diff timeout, Jxta_object ** obj);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_LISTENER_H */

/* vi: set ts=4 sw=4 tw=130 et: */
