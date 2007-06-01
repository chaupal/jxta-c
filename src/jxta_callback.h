/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_callback.h,v 1.4 2006/06/28 17:42:14 slowhog Exp $
 */


/**************************************************************************************************
 **  Jxta_callback
 **
 ** A Jxta_callback is used to handle events from other services in synchronous mode.
 **************************************************************************************************/

#ifndef JXTA_CALLBACK_H
#define JXTA_CALLBACK_H

#include "jxta_object.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_callback Jxta_callback;

/**
 * Prototype of the user function that eventually processes the event.
 *
 * @param obj The event object to be handled
 * @param arg The callback argument provided when creating Jxta_callback object
 * @return Jxta_status Return JXTA_SUCCESS is successfully handled. Other possible return values is a contract between the service
 * and the callback function
 */
typedef Jxta_status(JXTA_STDCALL * Jxta_callback_func) (Jxta_object * obj, void *arg);

typedef Jxta_status(JXTA_STDCALL * Jxta_callback_fn) (void * param, void *arg);

/************************************************************************
 ** Allocates a new Jxta_callback. This is a Jxta_object and reference counting applies.
 ** @see Jxta_object
 **
 ** @param instance a pointer to Jxta_callback pointer to receive the created object. NULL if the object failed to be created
 ** @param func is a pointer to the function to be called when there is an
 ** event to process.
 ** @param arg is an optional value which will be passed back to the callback function.
 ** @returns JXTA_SUCCESS if successfully created, JXTA_NOMEM if out of memory.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_callback_new(Jxta_callback ** instance, Jxta_callback_func func, void *arg);

/************************************************************************
 ** Ask the callback to process an event
 ** The calling thread invokes the callback function itself. This is
 ** a blocking operation.
 **
 ** @param listener a pointer to the listener to use.
 ** @param object a pointer to the Jxta_object to schedule. Note that the
 ** object is NOT shared by this method.
 ** @return JXTA_INVALID_ARGUMENT if arguments are invalid, JXTA_SUCCESS
 ** otherwise.
 *************************************************************************/
JXTA_DECLARE(Jxta_status) jxta_callback_process_object(Jxta_callback * me, Jxta_object * object);

/* temporary callback function to transform a listerner function */
Jxta_status JXTA_STDCALL listener_callback(Jxta_object * obj, void *arg);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_CALLBACK_H */

/* vi: set ts=4 sw=4 tw=130 et: */
