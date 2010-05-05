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
 * $Id$
 */



#ifndef JXTA_ENDPOINT_SERVICE_H
#define JXTA_ENDPOINT_SERVICE_H

#include "jxta_apr.h"
#include "jxta_service.h"

#include "jxta_listener.h"
#include "jxta_transport.h"
#include "jxta_em.h"
#include "jxta_endpoint_address.h"
#include "jxta_message.h"
#include "jxta_routea.h"
#include "jxta_pa.h"
#include "jxta_traffic_shaping_priv.h"



#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define  JXTA_ENDPOINT_SERVICE_NAME "EndpointService"

typedef struct jxta_endpoint_service Jxta_endpoint_service;

/* events definitions and prototypes */
typedef enum Jxta_endpoint_event_types {
    JXTA_EP_LOCAL_ROUTE_CHANGE = 1,
    JXTA_EP_UNREACHABLE,
    JXTA_EP_NEW_INBOUND_CONNECTION,
    JXTA_EP_NEW_OUTBOUND_CONNECTION,
    JXTA_EP_CONNECTION_CLOSED
} Jxta_endpoint_event_type;

typedef struct jxta_endpoint_event {
    JXTA_OBJECT_HANDLE;
    Jxta_endpoint_service *ep_svc;
    Jxta_endpoint_event_type type;
    Jxta_PID *remote_pid;
    Jxta_endpoint_address *remote_ea;
    Jxta_endpoint_address *local_ea;
    void *extra;
} Jxta_endpoint_event;

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

/**
 * Registers an incoming socket call back with the endpoint service. The endpoint
 * service will receive the incoming streams and call back the application with 
 * the data stream.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param transport (Ptr to) The transport object.
 * @param sock Socket of the connection
 * @param arg argument to be returned to the callback
 * @param elt location to store the transport demux element for unregistering the socket
 */
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_add_poll(Jxta_endpoint_service * me, apr_socket_t * s, Jxta_callback_fn fn,
                                                         void * arg, void ** cookie);

/**
 * UnRegisters a socket call back from the endpoint service.
 *
 * @param service Handle of the endpoint service object to which the
 * operation is applied.
 * @param transport (Ptr to) The transport object.
 * @param elt Transport demux element returned by the register function
 */
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_remove_poll(Jxta_endpoint_service * me, void * cookie);

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

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_check_msg_length(Jxta_endpoint_service * service
                                            , Jxta_endpoint_address *addr, Jxta_message * msg, apr_int64_t *max_length);

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
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ep_msg_sync(Jxta_endpoint_service *service
                    , Jxta_endpoint_message * ep_msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_size);

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ep_msg_async(Jxta_endpoint_service *service
                    , Jxta_endpoint_message * ep_msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_size);

#define jxta_endpoint_service_send jxta_endpoint_service_send_async

/*
 * Sends the given message to the given destination using the appropriate
 * transport according to the protocol name referenced by the given
 * destination address and the transports currently registered with this
 * endpoint service.
 *
 * @param me Handle of the endpoint service object to which the
 * operation is applied.
 * @param msg The message to be sent.
 * @param dest_addr The destination of the message.
 * @param sync Set to TRUE to use synchronous mode, FALSE for asynchrnomous.
 * @return Jxta_status success or failed
 *
 * @note: This version does not do peergroup mangling. Use jxta_PG_send instead.
 */
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ex(Jxta_endpoint_service * me, Jxta_message * msg,
                                                        Jxta_endpoint_address * dest_addr, Jxta_boolean sync, apr_int64_t *max_size);

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
 * Send a JXTA message synchronously to the given destination.
 *
 * @param service Endpoint service object
 * @param msg message to send
 * @param dest_addr Destination address
 *
 * @return status JXTA_SUCCESS if message sent successfully.
 * 
 */
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_sync(Jxta_PG * obj, Jxta_endpoint_service * service
                    , Jxta_message * msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_length);

/**
 * Send a JXTA message asynchronously to the given destination.
 *
 * @param service Endpoint service object
 * @param msg message to send
 * @param dest_addr Destination address
 *
 * @return status JXTA_SUCCESS if message sent successfully.
 * 
 */

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_async(Jxta_PG * obj, Jxta_endpoint_service * service
                    ,Jxta_message * msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_length);
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
 **
 ** @deprecated
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
 **
 ** @deprecated
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

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_thread_pool(Jxta_endpoint_service *me, apr_thread_pool_t **tp);

/**
 ** Get the endpoint entries for the network connection
 **
 ** @param Jxta_endpoint_service * endpoint service
 ** @param const char * protocol - protocol of the connection
 ** @param const char * address - address of the connection
 ** @param Jxta_vector ** endpoints - location to store a vector of peerids for the connection
 ** @param int *pointer to store the number of unique entries
 ** 
 ** @return Jxta_status
 **/
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_connection_peers(Jxta_endpoint_service * service, const char * protocol, const char * address, Jxta_vector **endpoints, int *unique);

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_fc(Jxta_endpoint_service * me, Jxta_endpoint_address *ea, Jxta_boolean normal);
/**
 * Callback to provide an ordered vector of destination addresses that are used to determine priority of messages
 *
 * @param group - group object
 * @param dest_eps Vector of destination peers.
 * @param ordered_eps Order peers should be used.
 *
 * @return JXTA_SUCCESS If the dest_eps vector is valid.
 **/
typedef Jxta_status (JXTA_STDCALL * Jxta_endpoint_flow_list_func) (Jxta_PG *group, Jxta_vector const *dest_eps, Jxta_vector **ordered_eps);

/**
 * Set the callback function to order the list of destination addresses for flow control
 *
 * @param Jxta_endpoint_service * endpoint service
 * @param group - group pointer
 * @param Jxta_endpoint_flow_list_func - pointer to callback
 *
 **/
JXTA_DECLARE(void) jxta_endpoint_service_set_flow_list_func(Jxta_endpoint_service * service, Jxta_PG *group, Jxta_endpoint_flow_list_func *func);

/**
 ** Get the flow control metrics for a destination - These are received from the peer.
 **
 ** @param Jxta_endpoint_service * endpoint service
 ** @param group - group pointer
 ** @param dest Jxta_endpoint_address of the peer requested. If NULL return the local peer's flow control metrics
 ** @param fcs location to store a vector of flow control objects with the metrics of the endpoint requested.
 **
 ** @return JXTA_SUCCESS if found JXTA_ITEM_NOT_FOUND if no entry for the endpoint
**/
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_rcv_flow_control(Jxta_endpoint_service * service, Jxta_PG *group, Jxta_endpoint_address *dest, Jxta_vector **fcs);

/**
 ** Get the endpoint metrics for all connected peers of a specific type - These are received from the peer
 **
 ** @param Jxta_endpoint_service service
 ** @param group - group pointer
 ** @param type the requested type
 ** @param fcs location to store a Vector of Jxta_ep_flow_control objects of the type requested
 **
 ** @return JXTA_SUCCESS if found JXTA_ITEM_NOT_FOUND if no entries for the type.
**/
JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_rcv_flow_control_of_type(Jxta_endpoint_service * service, Jxta_PG * group, const char *type, Jxta_vector **fcs);

/**
 ** Get the transmit endpoint metrics of the local peer.
 **
 ** @param Jxta_endpoint_service service
 ** @param group - group pointer
 ** @param dest Jxta_endpoint_address of the peer requested. If NULL return the local peer's flow control metrics
 ** @param fcs location to store a vector of flow control objects of the local peer's transmit endpoint connections
 **
**/
JXTA_DECLARE(void) jxta_endpoint_service_get_xmit_flow_control(Jxta_endpoint_service *service, Jxta_PG * group, Jxta_endpoint_address *dest, Jxta_vector **fcs);

/**
 ** Get the transmit endpoint metrics of the local peer of msg type.
 **
 ** @param Jxta_endpoint_service service
 ** @param group - group pointer
 ** @param type Message types
 ** @param fc location to store a vector of flow control objects of the local peer's transmit endpoint connections
 **
**/
JXTA_DECLARE(void) jxta_endpoint_service_get_xmit_flow_control_of_type(Jxta_endpoint_service *service, Jxta_PG *group, const char *type, Jxta_vector **fcs);

/** Endpoint flow control entry objects **/

JXTA_DECLARE(Jxta_ep_flow_control *) jxta_ep_flow_control_new();


/** 
 * get/set the message type for this entry
 *
 * @param ep_fc flow control entry 
 *               type values "Endpoint" "GenSRDI" "DiscoveryRequest" "DiscoveryResponse" "EndpointMessage"
 **/

/** get/set the traffic shaping object set by the remote peer **/
JXTA_DECLARE(Jxta_status) jxta_ep_fc_set_traffic_shaping(Jxta_ep_flow_control *ep_fc, Jxta_traffic_shaping *ts);
JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_traffic_shaping(Jxta_ep_flow_control *ep_fc, Jxta_traffic_shaping **ts);

JXTA_DECLARE(void) jxta_ep_fc_set_msg_type(Jxta_ep_flow_control *ep_fc, const char *type);
JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_msg_type(Jxta_ep_flow_control *ep_fc, char **msg_type);

/** get/set the peer group for this entry - Flow control entries can be defined for any group **/
JXTA_DECLARE(void) jxta_ep_fc_set_group(Jxta_ep_flow_control *ep_fc, Jxta_PG *group);
JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_group(Jxta_ep_flow_control *ep_fc, Jxta_PG **group);

/** get/set the endpoint address for this entry - Only valid for direction inbound **/
JXTA_DECLARE(void) jxta_ep_fc_set_ea(Jxta_ep_flow_control *ep_fc, Jxta_endpoint_address *ea);
JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_ea(Jxta_ep_flow_control *ep_fc, Jxta_endpoint_address **ea);

/** get/set outbound TRUE or FALSE - Local peer **/
JXTA_DECLARE(void) jxta_ep_fc_set_outbound(Jxta_ep_flow_control *ep_fc, Jxta_boolean);
JXTA_DECLARE(Jxta_boolean) jxta_ep_fc_outbound(Jxta_ep_flow_control *ep_fc);

/** get/set inbound TRUE or FALSE - Local peer **/
JXTA_DECLARE(void) jxta_ep_fc_set_inbound(Jxta_ep_flow_control *ep_fc, Jxta_boolean inbound);
JXTA_DECLARE(Jxta_boolean) jxta_ep_fc_inbound(Jxta_ep_flow_control *ep_fc);

/** get/set the rate allowed within the rate window **/
JXTA_DECLARE(Jxta_status) jxta_ep_fc_set_rate(Jxta_ep_flow_control *ep_fc, apr_int32_t rate);
JXTA_DECLARE(apr_int32_t) jxta_ep_fc_rate(Jxta_ep_flow_control *ep_fc);

/** get/set the size of the window for rate and burst rate **/
JXTA_DECLARE(void) jxta_ep_fc_set_rate_window(Jxta_ep_flow_control *ep_fc, int window);
JXTA_DECLARE(int) jxta_ep_fc_rate_window(Jxta_ep_flow_control *ep_fc);

/** get/set the expected delay for a message of indicated size **/
JXTA_DECLARE(void) jxta_ep_fc_set_expect_delay(Jxta_ep_flow_control *ep_fc, Jxta_time time);
JXTA_DECLARE(Jxta_time) jxta_ep_fc_expect_delay(Jxta_ep_flow_control *ep_fc);

/** get/set the message size available within the current window **/
JXTA_DECLARE(void) jxta_ep_fc_set_msg_size(Jxta_ep_flow_control *ep_fc, apr_int64_t size);
JXTA_DECLARE(apr_int64_t) jxta_ep_fc_msg_size(Jxta_ep_flow_control *ep_fc);

/** get/set the reserve percentage for the bytes available **/
JXTA_DECLARE(void) jxta_ep_fc_set_reserve(Jxta_ep_flow_control *ep_fc, int reserve);
JXTA_DECLARE(int) jxta_ep_fc_reserve(Jxta_ep_flow_control *ep_fc);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_ENDPOINT_SERVICE_H */

/* vim: set ts=4 sw=4 tw=130 et: */
