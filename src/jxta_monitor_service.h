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
 * $Id$
 */

#ifndef __JXTA_MONITOR_H__
#define __JXTA_MONITOR_H__

#include "jxta_id.h"
#include "jxta_pipe_adv.h"
#include "jxta_advertisement.h"
#include "jxta_listener.h"
#include "jxta_message.h"
#include "jxta_monitor_entry.h"

typedef struct _jxta_monitor_service Jxta_monitor_service;
typedef struct _jxta_monitor_listener Jxta_monitor_listener;

typedef Jxta_status(JXTA_STDCALL * Jxta_monitor_callback_func) (Jxta_object * obj, Jxta_monitor_entry **arg);

#include "jxta_monitor_msg.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define MONITOR_BROADCAST_SEED (unsigned char *) "MonitorBroadcastPipe"
#define MONITOR_BROADCAST_SEED_LENGTH sizeof(MONITOR_BROADCAST_SEED)


/**
 * create a monitor 
*/
JXTA_DECLARE(Jxta_monitor_service *) jxta_monitor_service_new_instance(void);

/**
 *  retrieve the instance
*/
JXTA_DECLARE(Jxta_monitor_service *) jxta_monitor_service_get_instance();

/**
 * set the pipe for broadcast 
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_set_broadcast_pipe(Jxta_monitor_service * monitor, Jxta_pipe_adv * pipe_adv);

/**
 * send a monitor message on the broadcast pipe or to an individual peer
 * A peer may be specified for directed messages
 * A listener is specified for messages that are directed to a peer requesting a response
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_send_msg(Jxta_monitor_service * monitor, Jxta_monitor_msg * msg, Jxta_id * peer, Jxta_listener * listener);

/**
 * add a monitor entry. Services use this function to add entries at their own intervals
 * When the service broadcasts at its interval all accrued entries are added.
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_add_monitor_entry(Jxta_monitor_service * monitor, const char *group_id, const char *service_id, Jxta_monitor_entry * entry, Jxta_boolean flush);

/**
 *
 * Create a monitor listener
 * @param listener_func A jxta listener func to receive monitor entries
 * @param context The context of the listener
 * @param sub_context The sub_context of the listener
 * @param listener A monitor listener to call
 *
 */
JXTA_DECLARE(Jxta_status) jxta_monitor_service_listener_new(Jxta_listener_func listener_func, const char *context, const char *sub_context, const char * type, Jxta_monitor_listener ** monitor_listener);

/**
 * Add a listener for broadcast messages.
 * A type may be specified to filter entries
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_add_listener(Jxta_monitor_service * monitor, Jxta_monitor_listener *listener);

/**
 * Remove a previous listener
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_remove_listener(Jxta_monitor_service * monitor, Jxta_monitor_listener *listener);

/**
 * Register a call back that will be invoked every monitor cycle.
 * @param monitor Monitor service
 * @param Jxta_monitor_callback_func Function to invoke
 * @param monitor_type Type of monitor entry
 * @param service Service requesting a callback
 * @param arg argument for the callback
*/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_register_callback(Jxta_monitor_service * monitor, Jxta_monitor_callback_func func, const char * monitor_type, Jxta_object * obj);

/**
 * Remove monitor callback
 *
**/
JXTA_DECLARE(Jxta_status) jxta_monitor_service_remove_callback(Jxta_monitor_service * monitor, const char * monitor_type);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_MONITOR_H__ */

/* vim: set ts=4 sw=4 et tw=130: */
