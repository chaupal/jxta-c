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
 * $Id$
 */

#ifndef JXTA_ENDPOINT_SERVICE_PRIV_H
#define JXTA_ENDPOINT_SERVICE_PRIV_H

#include "jxta_endpoint_service.h"
#include "jxta_transport_private.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


/*
 * Callback for transport events
 * 
 * @param me Handle of the endpoint service object to which the
 * operation is applied.
 * @param Jxta_transport_event* e pointer to the Jxta_transport_event object
 */
void jxta_endpoint_service_transport_event(Jxta_endpoint_service * me, Jxta_transport_event * e);

/**
 * Adds a callback to a specific service and parameters.
 *
 * @param me pointer to the instance of the endpoint service.
 * @param recipient pointer to a string containing the recipient portion of an endpoint address
 * @param cb is a pointer to the Jxta_callback handles messages delivered to the endpoint address. Set to NULL to ignore messages
 * for the recipient.
 *
 * @see Jxta_endpoint_address
 * @returns JXTA_SUCCESS if the callback is successfully installed
 *          JXTA_BUSY if there was already a callback registered
 *          JXTA_FAILED for other errors
 */
Jxta_status endpoint_service_add_recipient(Jxta_endpoint_service * me, void **cookie, const char *name, const char *param,
                                           Jxta_callback_func f, void *arg);

Jxta_status endpoint_service_remove_recipient(Jxta_endpoint_service * me, void *cookie);
Jxta_status endpoint_service_remove_recipient_by_addr(Jxta_endpoint_service * me, const char *name, const char *param);

Jxta_status endpoint_service_poll(Jxta_endpoint_service * me, apr_interval_time_t timeout);
Jxta_status endpoint_service_demux(Jxta_endpoint_service * me, const char *name, const char *param, Jxta_message * msg);

Jxta_status endpoint_service_propagate_by_group(Jxta_PG * pg, Jxta_message * msg, const char *service_name, const char *service_param);

Jxta_status endpoint_messenger_get(Jxta_endpoint_service * me, Jxta_endpoint_address * dest
                            , JxtaEndpointMessenger **msgr);

JXTA_DECLARE(Jxta_status) endpoint_msg_get_ep_msg(Jxta_message *msg, Jxta_endpoint_message **ep_msg);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_ENDPOINT_SERVICE_PRIV_H */

/* vim: set ts=4 sw=4 tw=130 et: */
