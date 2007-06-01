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
 * $Id: jxta_endpoint_service.h,v 1.14 2006/03/06 23:50:38 slowhog Exp $
 */



#ifndef JXTA_ENDPOINT_SERVICE_H
#define JXTA_ENDPOINT_SERVICE_H

#include "jxta_service.h"

#include "jxta_listener.h"
#include "jxta_transport.h"
#include "jxta_endpoint_address.h"
#include "jxta_message.h"
#include "jxta_routea.h"
#include "jxta_pa.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define  JXTA_ENDPOINT_SERVICE_NAME "EndpointService"

typedef struct _jxta_endpoint_service Jxta_endpoint_service;

typedef void (*JxtaEndpointListener) (Jxta_message * msg, void *arg);
typedef int (*JxtaEndpointFilter) (Jxta_message * msg, void *arg);

/*
 * Delivers the given message to its designated listener, according to
 * the message's destination and the listeners registered with this
 * endpoint object.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param msg (Ptr to) The message to be delivered.
 */
JXTA_DECLARE(void) jxta_endpoint_service_demux(Jxta_endpoint_service * service, Jxta_message * msg);

/*
 * Delivers the given message to its designated listener, according to
 * the message's destination and the listeners registered with this
 * endpoint object.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param dest The destination address of the message.
 * @param msg The message to be delivered.
 */
JXTA_DECLARE(void) jxta_endpoint_service_demux_addr(Jxta_endpoint_service * service, Jxta_endpoint_address * dest,
                                                    Jxta_message * msg);

/*
 * Registers a transport protocol object with this endpoint service.
 * That transport becomes usable to send messages.
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param transport (Ptr to) The transport to be registered.
 */
JXTA_DECLARE(void) jxta_endpoint_service_add_transport(Jxta_endpoint_service * service, Jxta_transport * transport);

/*
 * Unregisters a transport protocol object with this endpoint service.
 * That transport is no-longer usable to send messages.
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param transport (Ptr to) The transport to be unregistered.
 */
JXTA_DECLARE(void) jxta_endpoint_service_remove_transport(Jxta_endpoint_service * service, Jxta_transport * transport);


/*
 * Registers a filter routine with this endpoint service.
 * That filter will be invoked for every message received by this endpoint if
 * it has an element which name is identical to the given filter name.
 *
 * Filters are invoked before listeners.
 *
 * If any filter that the message is handed to returns false, the message
 * is discarded.
 *
 * Filters are invoked by an internal thread of the endpoint
 * service, so they may not sleep or perform overly lengthy operations.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 *
 * @param str The filter name (currently ignored - all messages are given to
 * all filters).
 *
 * @param arg An opaque token passed as an argument to the filter in addition
 * to the message.
 *
 * @param f The routine to be invoked. JxtaEndpointFilter's definition
 * is : int (*JxtaEndpointFilter)(Jxta_message* m, void* a).
 * Messages are handed to the filter by invoking the filter routine with
 * m set to the message being received and a set to the argument arg
 * passed to the jxta_endpoint_add_filter call.
 */
JXTA_DECLARE(void) jxta_endpoint_service_add_filter(Jxta_endpoint_service * service, char const *str, JxtaEndpointFilter f,
                                                    void *arg);

/*
 * Removes a filter routine previously registered with this endpoint service.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param filter The filter to be removed.
 */
JXTA_DECLARE(void) jxta_endpoint_service_remove_filter(Jxta_endpoint_service * service, JxtaEndpointFilter filter);

/*
 * Sends the given message to the given destination using the appropriate
 * transport according to the protocol name referenced by the given
 * destination address and the transports currently registered with this
 * endpoint service.
 *
 * @param peergroup handle reference
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param msg The message to be sent.
 * @param dest_addr The destination of the message.
 * @return Jxta_status success or failed
 */
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_sync(Jxta_PG * pg, Jxta_endpoint_service * service, Jxta_message * msg, Jxta_endpoint_address * dest_addr);

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_async(Jxta_PG * pg, Jxta_endpoint_service * service, Jxta_message * msg, Jxta_endpoint_address * dest_addr);

#define jxta_endpoint_service_send jxta_endpoint_service_send_async

/*
 * Propagate the given message to the given service destination using 
 * any available propagation transport. There is no guarantee
 * the call will succeed as no propagation transport may be configured
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param msg The message to be sent.
 * @param service_name The destination service_name  of the message.
 * @param service_parameter The destination service_parameter of the message.
 * @return Jxta_status success or failed
 */
JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_propagate(Jxta_endpoint_service * service,
                                Jxta_message * msg, const char *service_name, const char *service_parameter);

/**
 ** Adds a listener to a specific service and parameters.
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @param serviceName pointer to a string containing the name of the service on which
 ** the listener is listening on.
 ** @param serviceParam pointer to a string containing the parameter associated to the
 ** serviceName.
 ** @param listener is a pointer to the Jxta_listener to add.
 ** @returns JXTA_INVALID_PARAMETERS if some arguments are invalid,
 **          JXTA_BUSY if there was already a listener registered
 **          JXTA_SUCCESS otherwise
 **/
JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_add_listener(Jxta_endpoint_service * endpoint,
                                   char const *serviceName, char const *serviceParam, Jxta_listener * listener);

/**
 ** Adds a listener to a specific service and parameters.
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @param serviceName pointer to a string containing the name of the service on which
 ** the listener is listening on.
 ** @param serviceParam pointer to a string containing the parameter associated to the
 ** serviceName.
 ** @returns JXTA_INVALID_PARAMETERS if some arguments are invalid, JXTA_SUCCESS otherise
 **/
JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_remove_listener(Jxta_endpoint_service * endpoint, char const *serviceName, char const *serviceParam);

/**
 ** lookup for a specific transport
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @param transport_naame pointer to a string containing the name of the transport
 ** we are looking for
 ** @returns a pointer to the corresponding transport
 **/
JXTA_DECLARE(Jxta_transport *) jxta_endpoint_service_lookup_transport(Jxta_endpoint_service * service,
                                                                      const char *transport_name);

/**
 ** set current relay address information after a peer connected
 ** to its relay
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @param protocol_name pointer to a string containing the name of the transport protocol
 ** @param address_name pointer to a string containing the name of the transport address
 **/
JXTA_DECLARE(void) jxta_endpoint_service_set_relay(Jxta_endpoint_service * service, const char *protocol_name,
                                                   const char *address_name);

/**
 ** get current relay protocol address information after a peer connected
 ** to its relay
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @return protocol_name pointer to a string containing the name of the transport protocol
 **/
JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_proto(Jxta_endpoint_service * service);


/**
 ** get current relay address information after a peer connected
 ** to its relay
 **
 ** @param endpoint pointer to the instance of the endpoint service.
 ** @relay address_name pointer to a string containing the name of the transport address
 **/
JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_addr(Jxta_endpoint_service * service);



/**
 ** get route address information from the peer advertisement
 **
 ** @param peer advertisement
 ** @param route advertisement associated with this peer advertisement
 **/
JXTA_DECLARE(void) jxta_endpoint_service_get_route_from_PA(Jxta_PA * padv, Jxta_RouteAdvertisement **);


/**
 ** get route address information for the local peer
 **
 ** @param Jxta_endpoint_service endpoint service
 ** @return route advertisement associated with this peer advertisement
 **/
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_endpoint_service_get_local_route(Jxta_endpoint_service * service);


/**
 ** set oute address information for the local peer
 **
 ** @param Jxta_endpoint_service endpoint service
 ** @param route advertisement associated with this peer
 **
 ** @return void
 **/
JXTA_DECLARE(void) jxta_endpoint_service_set_local_route(Jxta_endpoint_service * service, Jxta_RouteAdvertisement * route);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_ENDPOINT_SERVICE_H */

/* vim: set ts=4 sw=4 tw=130 et: */
