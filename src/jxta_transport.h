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

#ifndef JXTA_TRANSPORT_H
#define JXTA_TRANSPORT_H

#include "jxta_module.h"

#include "jstring.h"
#include "jxta_endpoint_address.h"
#include "jxta_endpoint_messenger.h"
#include "jxta_message.h"
#include "jxta_qos.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_transport Jxta_transport;
typedef enum _jxta_directions {
    JXTA_INBOUND = 1,
    JXTA_OUTBOUND = 2,
    JXTA_BIDIRECTION = 3
} Jxta_direction;

/*
 * Transport API.
 */

/*
 * Returns the name of the protocol that this transport handles.
 * After this transport is registered, it may be used to send messages which
 * destination address have this name as the protocol name field.
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return JString* The name of the protocol handled by this transport as
 * a JString.
 */
JXTA_DECLARE(JString *) jxta_transport_name_get(Jxta_transport * that);

/*
 * Returns the address of this peer by this transport.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_endpoint_address* (Ptr to) the address.
 */
JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_publicaddr_get(Jxta_transport * that);

/*
 * Returns a messenger object which may be used to send one or more messages
 * to the given destination via this transport.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @param there The destination address where the returned messenger will
 * send all messages.
 * @return JxtaEndpointMessenger* (Ptr to) the requested messenger object.
 * Returns NULL if no adequate messenger could be constructed.
 */
JXTA_DECLARE(JxtaEndpointMessenger *) jxta_transport_messenger_get(Jxta_transport * that, Jxta_endpoint_address * there);

/*
 * Tests the reachability of the given address via this transport.
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @param there The address to be tested.
 * @return Jxta_boolean TRUE if this transport claims that a message would
 * have a chance to reach that destination.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_ping(Jxta_transport * that, Jxta_endpoint_address * there);

/*
 * Propagates a message to multiple destinations in one operation as permitted
 * by the media used by this transport. No effort is made at emulating the
 * propagation effect by sending the messages multiple times.
 * Transport have no obligation to make this call have any effect.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @param msg The message to be propagated.
 * @param service_name The name of the recipient service
 * @param service_param The service specific parameter.
 * All each peer that each reaches, the message will be delivered to the
 * listener which name is identical to the concatenation of servie_name
 * and service_param. 
 */
JXTA_DECLARE(void)
jxta_transport_propagate(Jxta_transport * that, Jxta_message * msg, const char *service_name, const char *service_params);


/*
 * Tells if this transport may be overloaded by a transport with the same
 * protocol name in a descendent group.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_boolean TRUE if overloading is acceptable, FALSE otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_overload_p(Jxta_transport * that);

/*
 * Tells if this transport may be used as vehicle by the router.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_boolean TRUE if use by the router is advisable, FALSE
 * otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_routing_p(Jxta_transport * that);

/*
 * Tells if this transport maintains connections between points.
 *
 * @param that Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_boolean TRUE if connection-oriented, FALSE otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_connection_oriented_p(Jxta_transport * that);

/*
 * Tells if this transport allows inbound messages
 *
 * @param me Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_boolean TRUE if inbound message is allowed, FALSE otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_inbound(Jxta_transport * me);

/*
 * Tells if this transport allows outbound messages
 *
 * @param me Handle of the Jxta_transport object to whih the operation is
 * applied.
 * @return Jxta_boolean TRUE if outbound message is allowed, FALSE otherwise.
 */
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_outbound(Jxta_transport * me);

/**
 * Set default QoS to be applied for an endpoint.
 * @param me pointer to the transport. 
 * @param ea The endpoint address for which this default is for. NULL to indicate all peers as well as propagation.
 * @param qos the QoS to be used as default for the endpoint 
 * @return JXTA_SUCCESS if the value was set. JXTA_FAILED otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_transport_set_default_qos(Jxta_transport * me, Jxta_endpoint_address * ea, const Jxta_qos * qos);

/**
 * Get the default QoS setting for endpoints
 * @param me pointer to the transport
 * @param ea The endpoint address whose default QoS to be retrieved. NULL to indicate all peers as well as propagation.
 * @param qos default QoS for the specified endpoint
 * @return JXTA_SUCCESS if the default is retrieved successfully, JXTA_ITEM_NOTFOUND is no default was set.
 */
JXTA_DECLARE(Jxta_status) jxta_transport_default_qos(Jxta_transport * me, Jxta_endpoint_address * ea, const Jxta_qos ** qos);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_TRANSPORT_H */

/* vim: set sw=4 ts=4 et tw=130: */
