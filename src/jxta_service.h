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
 * $Id: jxta_service.h,v 1.14 2006/09/06 23:09:47 mmx2005 Exp $
 */

#ifndef JXTA_SERVICE_H
#define JXTA_SERVICE_H

#include "jxta_module.h"
#include "jxta_callback.h"
#include "jxta_id.h"
#include "jxta_qos.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct jxta_service Jxta_service;

/**
 * Service objects are not manipulated directly to protect usage
 * of the service. A Service interface is returned to access the service
 * methods.
 * NOTE: Returning a reference to the object that actually holds the state
 * is permitted. This interface is there to leave room for more complex
 * schemes if needed in the future.
 *
 * @param self handle of the Jxta_service object to which the operation is
 * applied.
 * @param service A location where to return (a ptr to) an object that implements
 * the public interface of the service.
 */
JXTA_DECLARE(void) jxta_service_get_interface(Jxta_service * self, Jxta_service ** service);

/**
 * Returns the implementation advertisement for this service.
 *
 * @param self handle of the Jxta_service object to which the operation is
 * applied.
 * @param mia A location where to return (a ptr to) the advertisement.
 * NULL may be returned if the implementation is incomplete and has no impl adv
 * to return.
 */
JXTA_DECLARE(void) jxta_service_get_MIA(Jxta_service * self, Jxta_advertisement ** mia);

/**
 * Subscribe a callback to be called for events emit by the service. The return value from the callback is ignored.
 * @param me pointer to the Jxta_service
 * @param f  the callback function to be called for the event
 * @param arg the user data to be passed to the callback
 * @return JXTA_SUCCESS if the subscription succeed, JXTA_BUSY if the same had subscribed. JXTA_FAILED otherwise
 */
JXTA_DECLARE(Jxta_status) jxta_service_events_connect(Jxta_service * me, Jxta_callback_fn f, void *arg);

/**
 * Unsubscribe a callback from receiving events
 * @param me pointer to the Jxta_service
 * @param f  the callback function to be called for the event
 * @param arg the user data to be passed to the callback
 * @return JXTA_SUCCESS if the unsubscription succeed, JXTA_ITEM_NOTFOUND if no matching subscription was found. JXTA_FAILED
 * otherwise
 */
JXTA_DECLARE(Jxta_status) jxta_service_events_disconnect(Jxta_service * me, Jxta_callback_fn f, void *arg);

/**
 * Call event handlers for this service with the specified event and user data. I.e, f(event, arg)
 * @param me pointer to the Jxta_service
 * @param event pointer to the event object. The callback function should know what it subscribed to and knows how to intepret the
 * pointer
 * @return JXTA_SUCCESS if all subscribers are called. 
 */
JXTA_DECLARE(Jxta_status) jxta_service_event_emit(Jxta_service * me, void *event);

/**
 * Lock/unlock method, only for subclass to use for internal needs of service.
 */
JXTA_DECLARE(Jxta_status) jxta_service_lock(Jxta_service * me);
JXTA_DECLARE(Jxta_status) jxta_service_unlock(Jxta_service * me);

/**
 * set a set of options for the service
 * @param me pointer to the service
 * @param ns namespace for the options
 * @prarm options the hash table contains key/value pairs
 * @return JXTA_SUCCESS 
 * @note The options hash table must have a lifespan longer than the service
 */
JXTA_DECLARE(Jxta_status) jxta_service_options_set(Jxta_service * me, const char *ns, apr_hash_t * options);

/**
 * set an option for the service
 * @param me pointer to the service
 * @param ns namespace for the option
 * @prarm key the name of the option to be set
 * @param val the value for the option
 * @return JXTA_SUCCESS if value is set successfully, status value otherwise.
 * @note key and val must have a lifespan longer than the service because the value is not copied
 */
JXTA_DECLARE(Jxta_status) jxta_service_option_set(Jxta_service * me, const char *ns, const char *key, const void *val);

/**
 * retrieve an option value for the service
 * @param me pointer to the service
 * @param ns namespace for the option
 * @prarm key the name of the option whose value to be retrieved 
 * @param val pointer to receive the value for the option
 * @return JXTA_SUCCESS if the value can be retrieved, JXTA_ITEM_NOTFOUND if the option was not set
 */
JXTA_DECLARE(Jxta_status) jxta_service_option_get(Jxta_service * me, const char *ns, const char *key, void **val);

/**
 * Set default QoS to be applied for a resource specified by Jxta_id.
 * @param me pointer to the service
 * @param id The resource ID for which this default to be applied. Different service may have different interpretation to the ID.
 * NULL indicate a default for every resources.
 * @param qos the QoS to be used as default for the resource
 * @return JXTA_SUCCESS if the value was set. JXTA_FAILED otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_service_set_default_qos(Jxta_service * me, Jxta_id * id, const Jxta_qos * qos);

/**
 * Get the default QoS setting for a resource specified by Jxta_id.
 * @param me pointer to the service
 * @param id The resource ID for which this default to be applied. Different service may have different interpretation to the ID.
 * NULL indicate a default for every resources.
 * @param qos default QoS for the resource
 * @return JXTA_SUCCESS if found successfully, JXTA_ITEM_NOTFOUND if no default was set. 
 */
JXTA_DECLARE(Jxta_status) jxta_service_default_qos(Jxta_service * me, Jxta_id * id, const Jxta_qos ** qos);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_SERVICE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
