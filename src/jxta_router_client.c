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
 * $Id: jxta_router_client.c,v 1.59 2005/04/06 21:30:49 bondolo Exp $
 */
#include <stdlib.h>     /* for atoi */
#include <apr.h>
#include <apr_strings.h>

#include "jpr/jpr_excep_proto.h"

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jstring.h"
#include "jxta_types.h"
#include "jxta_transport_private.h"
#include "jxta_endpoint_service.h"
#include "jxta_discovery_service.h"
#include "jxta_router_service.h"
#include "jxta_peergroup.h"
#include "jxta_rm.h"
#include "jxta_vector.h"
#include "jxta_apa.h"
#include "jxta_routea.h"
#include "jxta_log.h"

static const char *__log_cat = "ROUTER_CLIENT";

struct _jxta_router_client {
    Extends(Jxta_transport);

    Jxta_PG *group;
    Jxta_vector *peers;
    char *localPeerIdString;
    Jxta_id *localPeerId;
    Jxta_endpoint_address *localPeerAddr;
    Jxta_endpoint_service *endpoint;
    Jxta_discovery_service *discovery;
    Jxta_listener *listener;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    Jxta_boolean started;
};



typedef struct {
    JxtaEndpointMessenger generic;
    Jxta_id *peerid;
    Jxta_router_client *router;
} Router_messenger;


typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_id *peerid;
    Jxta_vector *forwardGateways;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_router_client *router;
    int loc;
} Peer_entry;

extern Jxta_id *jxta_id_worldNetPeerGroupID;

#define JXTA_ENDPOINT_ROUTER_NAME          "EndpointService:jxta-NetGroup"
#define JXTA_ENDPOINT_ROUTER_SVC_PAR       "EndpointRouter"
#define JXTA_ENDPOINT_ROUTER_ELEMENT_NAME  "jxta:EndpointRouterMsg"


static void router_client_listener(Jxta_object * msg, void *arg);
static Peer_entry *get_peer_entry(Jxta_router_client * self, Jxta_id * peerid);
static void peer_entry_set_forward_gateways(Peer_entry * peer, Jxta_vector * vector);
static Jxta_vector *peer_entry_get_forward_gateways(Peer_entry * peer);
static Jxta_endpoint_address *get_endpoint_address_from_peerid(Jxta_id * peerid);
static Jxta_id *get_peerid_from_endpoint_address(Jxta_endpoint_address * addr);
static Jxta_RouteAdvertisement *search_in_local_cm(Jxta_router_client * t, Jxta_id * dest);
static Jxta_endpoint_address *jxta_router_select_best_address(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap);
static Jxta_endpoint_address *jxta_router_select_best_route(Jxta_router_client * router,
                                                            Jxta_AccessPointAdvertisement * ap,
                                                            Jxta_AccessPointAdvertisement * hop);
static Jxta_boolean reachable_address(Jxta_router_client * router, Jxta_endpoint_address * addr);

/******************
 * Module methods *
 *****************/

static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    apr_status_t res;

    Jxta_router_client *self = (Jxta_router_client *) module;
    PTValid(self, Jxta_router_client);

    self->group = group;
    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS)
        return res;

    /**
     ** Create our mutex.
     **/
    res = apr_thread_mutex_create(&(self->mutex), APR_THREAD_MUTEX_NESTED,      /* nested */
                                  self->pool);
    if (res != APR_SUCCESS)
        return res;

    {
        JString *tmp;
        jxta_PG_get_PID(group, &(self->localPeerId));
        tmp = NULL;
        jxta_id_get_uniqueportion(self->localPeerId, &tmp);
        self->localPeerIdString = malloc(jstring_length(tmp) + 1);
        strcpy(self->localPeerIdString, jstring_get_string(tmp));
        JXTA_OBJECT_RELEASE(tmp);
        jxta_PG_get_endpoint_service(group, &(self->endpoint));
    }

    self->localPeerAddr = get_endpoint_address_from_peerid(self->localPeerId);

    /**
     ** Add the router listener. Note that the Endpoint Router lives
     ** in the world peergroup.
     **/
    {
        JString *gid = NULL;

        jxta_id_get_uniqueportion(jxta_id_worldNetPeerGroupID, &gid);

        self->listener = jxta_listener_new(router_client_listener, (void *) self, 10, 100);

        jxta_endpoint_service_add_listener(self->endpoint,
                                           (char *) JXTA_ENDPOINT_ROUTER_NAME,
                                           (char *) JXTA_ENDPOINT_ROUTER_SVC_PAR, self->listener);

        jxta_listener_start(self->listener);

        JXTA_OBJECT_RELEASE(gid);
    }

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    return JXTA_SUCCESS;
}

static Jxta_status start(Jxta_module * module, char *argv[])
{
    Jxta_router_client *self = (Jxta_router_client *) module;
    PTValid(module, Jxta_router_client);

    apr_thread_mutex_lock(self->mutex);
    jxta_PG_get_discovery_service(self->group, &(self->discovery));
    self->started = TRUE;
    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

static void stop(Jxta_module * module)
{
    Jxta_router_client *self = (Jxta_router_client *) module;
    PTValid(module, Jxta_router_client);

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }
    self->started = FALSE;
    apr_thread_mutex_unlock(self->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Stopped.\n");
}


static JString *name_get(Jxta_transport * self)
{
    PTValid(self, Jxta_router_client);
    return jstring_new_2("jxta");
}


static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    Jxta_router_client *self = (Jxta_router_client *) t;
    PTValid(self, Jxta_router_client);

    JXTA_OBJECT_CHECK_VALID(self->localPeerAddr);

    JXTA_OBJECT_SHARE(self->localPeerAddr);
    return self->localPeerAddr;
}

/**
 ** Get the Peer_entry for the destination
 **/

static Jxta_status
get_peer_forward_gateways(Jxta_router_client * self,
                          Jxta_id * peerid, Jxta_vector ** vector, Jxta_endpoint_address ** nextHopAddr)
{

    Peer_entry *peer = NULL;
    Jxta_status res;
    Jxta_id *id = NULL;
    Jxta_endpoint_address *addr = NULL;
    Jxta_vector *tmpVector = NULL;
    char *caddress = NULL;

    peer = get_peer_entry(self, peerid);

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(peerid);
    JXTA_OBJECT_CHECK_VALID(peer);

    if (peer == NULL) {
        /* No route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No existing route\n");
        return JXTA_INVALID_ARGUMENT;
    }

    *vector = peer_entry_get_forward_gateways(peer);

    if (*vector == NULL) {
        /* We do not have a valid route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Route is empty\n");
        JXTA_OBJECT_RELEASE(peer);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(*vector);

    /* We do have a valid route. Get the first element, which is
     * the next hop, and get its endpoint address */
    res = jxta_vector_get_object_at(*vector, (Jxta_object **) & addr, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "found a route %s\n", (caddress = jxta_endpoint_address_to_string(addr)));
    free(caddress);

    if ((res != JXTA_SUCCESS) || (addr == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the next hop from vector.\n");
        JXTA_OBJECT_RELEASE(*vector);
        JXTA_OBJECT_RELEASE(peer);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(addr);

    if (strcmp(jxta_endpoint_address_get_protocol_name(addr), "jxta") != 0) {
    /**
     ** The endpoint address for the next hop is a real transport address.
     ** That means that this peer has a direct connection to the destination.
     **/

        *nextHopAddr = addr;
        JXTA_OBJECT_RELEASE(peer);
        return JXTA_SUCCESS;
    }

  /**
   ** We need to find the direct endpoint address for the next hop.
   **/
    id = get_peerid_from_endpoint_address(addr);

    JXTA_OBJECT_RELEASE(addr);

    if (id == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot build Peer ID of the next hop.\n");

        JXTA_OBJECT_RELEASE(*vector);
        JXTA_OBJECT_RELEASE(peer);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_RELEASE(peer);

    peer = get_peer_entry(self, id);
    JXTA_OBJECT_RELEASE(id);

    if (peer == NULL) {
        /* No route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No existing route for next hop\n");
        JXTA_OBJECT_RELEASE(*vector);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(peer);

    tmpVector = peer_entry_get_forward_gateways(peer);

    if ((tmpVector == NULL) || (jxta_vector_size(tmpVector) == 0)) {
        /* We do not have a valid route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Route is empty for next hop\n");
        JXTA_OBJECT_RELEASE(peer);
        JXTA_OBJECT_RELEASE(*vector);
        JXTA_OBJECT_RELEASE(peer);
        if (tmpVector != NULL)
            JXTA_OBJECT_RELEASE(tmpVector);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(tmpVector);

    addr = NULL;

    res = jxta_vector_get_object_at(tmpVector, (Jxta_object **) & addr, 0);

    if ((res != JXTA_SUCCESS) || (addr == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the next hop address from vector.\n");
        JXTA_OBJECT_RELEASE(*vector);
        JXTA_OBJECT_RELEASE(peer);
        JXTA_OBJECT_RELEASE(tmpVector);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(addr);

    *nextHopAddr = addr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "next hop gateway addr %s://%s/%s/%s\n",
             jxta_endpoint_address_get_protocol_name(addr),
             jxta_endpoint_address_get_protocol_address(addr),
             jxta_endpoint_address_get_service_name(addr), jxta_endpoint_address_get_service_params(addr));

    JXTA_OBJECT_RELEASE(peer);
    JXTA_OBJECT_RELEASE(tmpVector);

    return JXTA_SUCCESS;
}

static void messenger_send(JxtaEndpointMessenger * m, Jxta_message * msg)
{

    Router_messenger *messenger = (Router_messenger *) m;
    Jxta_vector *forwardGateways = NULL;
    Jxta_status res;
    JString *dest = NULL;
    Jxta_transport *transport = NULL;
    char *transport_name;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    int routed = 1;
    Jxta_endpoint_address *dest_addr = NULL;
    Jxta_endpoint_address *addr = NULL;
    char *relay_addr;
    char *relay_proto;
    EndpointRouterMessage *rmsg = NULL;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_vector *hops = NULL;
    Jxta_AccessPointAdvertisement *ap = NULL;
    Jxta_AccessPointAdvertisement *hop = NULL;
    Jxta_endpoint_address *new_addr = NULL;
    int sz;
    char *caddress = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Router_messenger.send()\n");
    
    if (FALSE == messenger->router->started) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to send a message after router was stopped.\n");
        return JXTA_VIOLATION;
    }

  /**
   ** Check if destination is local
   **/

    /*
     * We have a pre-resolved physical address
     * as destination. This meansthat we don't have
     * to perform any route resolution as 
     * this has been done by the application.
     * We just need to bypass the route resolution
     * and send the message to the corresponding transport
     */
    if (messenger->peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Direct pre-resolved route\n");
        routed = 0;

        dest_addr = jxta_message_get_destination(msg);
        forwardGateways = jxta_vector_new(0);
    } else {
        jxta_id_get_uniqueportion(messenger->peerid, &dest);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Routing destination '%s'\n", jstring_get_string(dest));

        if (strcmp(messenger->router->localPeerIdString, jstring_get_string(dest)) == 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Local destination\n");

            /* Local destination, directely call endpoint service's demux */
            JXTA_OBJECT_SHARE(msg);

            jxta_endpoint_service_demux(messenger->router->endpoint, msg);

            JXTA_OBJECT_RELEASE(dest);
            return;
        }

        JXTA_OBJECT_RELEASE(dest);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remote destination\n");

    /**
     ** Get the Peer_entry for the destination
     **/
        res = get_peer_forward_gateways(messenger->router, messenger->peerid, &forwardGateways, &dest_addr);

        if (res != JXTA_SUCCESS) {
            /*
             * We haven't found any route. The best we
             * can do. Is try to find a route and if nothing try to
             * push the message to our relay
             * It should be able to resolve the route
             * for us for the time being.
             *
             * FIXME 20040515 tra: We should support route
             * discovery at some point from the edge peer to reduce load
             * on our relay.
             */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No route known, try to look in CM\n");

            /*
             * Check first in our CM if we can find a route for
             * that peer. We are searching for a peer advertisement. 
             */
            route = search_in_local_cm(messenger->router, messenger->peerid);

            if (route != NULL) {

                JString * routeadv;
                jxta_RouteAdvertisement_get_xml( route, &routeadv );
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Route adv : \n%s\n", jstring_get_string(routeadv) );
                JXTA_OBJECT_RELEASE(routeadv);
                
                /*
                 * We have a route so use it to send our message. We need to
                 * extract the various endpoint addresses of the route. We will try
                 * to send first to the destination trying to find a valid endpoint
                 * address (an address we can connect to). If not, we will check the
                 * last hop of the route, if there is one to try find a valid
                 * endpoint address we can connect to
                 * 
                 */
                hops = jxta_RouteAdvertisement_get_Hops(route);
                sz = jxta_vector_size(hops);
                if (sz > 0) {
                    /* 
                     * We have a long route. We just need to get the last hop
                     * that should point to the relay of the destination
                     * peer
                     */
                    res = jxta_vector_get_object_at(hops, (Jxta_object **) & hop, sz - 1);
                    if (res != JXTA_SUCCESS) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not extract last hop in route - \n");
                    }
                }

                /*
                 * we have a single hop route just select the destination
                 */
                ap = jxta_RouteAdvertisement_get_Dest(route);

                /*
                 * Figure out the best route using the following priority:
                 *    - TCP/IP 
                 *    - HTTP
                 *    _ Any relay addresses (TCP/IP or HTTP)
                 */
                new_addr = jxta_router_select_best_route(messenger->router, ap, hop);

                JXTA_OBJECT_RELEASE(ap);
                if (hop) {
                    JXTA_OBJECT_RELEASE(hop);
                }

                if (new_addr != NULL) {
                    /*
                     * Set the destination address of the message.
                     */
                    dest_addr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(new_addr),
                                                           jxta_endpoint_address_get_protocol_address(new_addr),
                                                           JXTA_ENDPOINT_ROUTER_NAME,
                                                           JXTA_ENDPOINT_ROUTER_SVC_PAR);

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Send msg to address %s://%s\n", jxta_endpoint_address_get_protocol_name(new_addr),
                             jxta_endpoint_address_get_protocol_address(new_addr));

                    /*
                     * need to add routing information. Tell the router that 
                     * we need to add the RouterMessageElement. Set the route
                     * to be direct to our destination
                     */
                    routed = 1;
                    forwardGateways = jxta_vector_new(0);
                    JXTA_OBJECT_RELEASE(new_addr);
                }

                JXTA_OBJECT_RELEASE(hops);
                JXTA_OBJECT_RELEASE(route);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Could not find route\n" );
            }

            if (dest_addr == NULL) {

                /*
                 * Check if we have a relay yet, if not, then there
                 * is no much we can do here until we implement the
                 * dynamic route discovery
                 *
                 * FIXME 20040225 tra need to implement route discovery
                 *
                 */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No route found - try to send message to our relay\n");
                relay_addr = jxta_endpoint_service_get_relay_addr(messenger->router->endpoint);

                if (relay_addr == NULL) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No relay connection, discard message\n");
                    m->status = JXTA_FAILED;
                    return;
                }

                relay_proto = jxta_endpoint_service_get_relay_proto(messenger->router->endpoint);

                /*
                 * Set the destination address of the message.
                 */
                dest_addr = jxta_endpoint_address_new2(relay_proto,
                                                       relay_addr,
                                                       (char *) JXTA_ENDPOINT_ROUTER_NAME, (char *) JXTA_ENDPOINT_ROUTER_SVC_PAR);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward msg to relay %s://%s\n", relay_proto, relay_addr);

                /*
                 * need to add routing information. Tell the router that 
                 * we need to add the RouterMessageElement. Set the route
                 * to be direct to our destination
                 */
                routed = 1;
                forwardGateways = jxta_vector_new(0);
            }
        }
    }

  /**
   ** We have a route. Build a router element, add it into the message,
   ** and send it.
   **/

    JXTA_OBJECT_CHECK_VALID(dest_addr);

    rmsg = EndpointRouterMessage_new();
    if (rmsg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate an EndpointRouterMessage\n");
        JXTA_OBJECT_RELEASE(forwardGateways);
        JXTA_OBJECT_RELEASE(dest_addr);
        return;

    } else {

        if (routed) {

            JString *xml = NULL;

            /* Set rmsg */
            EndpointRouterMessage_set_Src(rmsg, messenger->router->localPeerAddr);
            EndpointRouterMessage_set_Last(rmsg, messenger->router->localPeerAddr);
            EndpointRouterMessage_set_Dest(rmsg, messenger->generic.address);
            EndpointRouterMessage_set_GatewayForward(rmsg, forwardGateways);

            /* Builds the XML */
            res = EndpointRouterMessage_get_xml(rmsg, &xml);

            JXTA_OBJECT_CHECK_VALID(xml);

            JXTA_OBJECT_RELEASE(rmsg);

            if (res == JXTA_SUCCESS) {
                Jxta_message_element *msgElem = NULL;

            /**
	     ** FIXME 3/10/2002 lomax@jxta.org
	     **
	     ** The Jxta_message implementation does not yet completely allow
	     ** sharing of the data encapsulated within a message element. Copy is
	     ** necessary for the time being. This will have to be changed when
	     ** the Jxta_message implementation will be completed.
	     **/
                char *buffer = NULL;

                buffer = malloc(jstring_length(xml));

                memcpy(buffer, jstring_get_string(xml), jstring_length(xml));

                msgElem = jxta_message_element_new_1(JXTA_ENDPOINT_ROUTER_ELEMENT_NAME,
                                                     "text/xml", buffer, jstring_length(xml), NULL);

                free(buffer);
                jxta_message_remove_element_1(msg, JXTA_ENDPOINT_ROUTER_ELEMENT_NAME);
                jxta_message_add_element(msg, msgElem);

                JXTA_OBJECT_RELEASE(msgElem);

            /**
	     ** Set the destination address of the message.
	     **/
                dest_addr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(dest_addr),
                                                       jxta_endpoint_address_get_protocol_address(dest_addr),
                                                       (char *) JXTA_ENDPOINT_ROUTER_NAME, (char *) JXTA_ENDPOINT_ROUTER_SVC_PAR);

                jxta_message_set_destination(msg, dest_addr);
            }

            JXTA_OBJECT_RELEASE(xml);
        }

    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send destination %s\n", (caddress = jxta_endpoint_address_to_string(dest_addr)));
    free(caddress);

    transport_name = jxta_endpoint_address_get_protocol_name(dest_addr);

    transport = jxta_endpoint_service_lookup_transport(messenger->router->endpoint, transport_name);
    if (transport) {

        endpoint_messenger = jxta_transport_messenger_get(transport, dest_addr);

        if (endpoint_messenger) {
            JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
            addr = jxta_transport_publicaddr_get(transport);
            jxta_message_set_source(msg, addr);

            endpoint_messenger->jxta_send(endpoint_messenger, msg);
            m->status = endpoint_messenger->status;
            JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
            JXTA_OBJECT_RELEASE(endpoint_messenger);
            JXTA_OBJECT_RELEASE(addr);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not obtain a messenger for : %s\n", (caddress = jxta_endpoint_address_to_string(dest_addr)));
        free( caddress );
            m->status = JXTA_FAILED;
            JXTA_OBJECT_RELEASE(transport);
            return;
        }

        JXTA_OBJECT_RELEASE(transport);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not obtain a transport for : %s\n", (caddress = jxta_endpoint_address_to_string(dest_addr)));
        free( caddress );
        m->status = JXTA_FAILED;
        return;
    }

    JXTA_OBJECT_RELEASE(dest_addr);
    JXTA_OBJECT_RELEASE(forwardGateways);
}


static Jxta_boolean reachable_address(Jxta_router_client * router, Jxta_endpoint_address * addr)
{

    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *messenger = NULL;

    transport = jxta_endpoint_service_lookup_transport(router->endpoint, jxta_endpoint_address_get_protocol_name(addr));
    if (transport) {
        messenger = jxta_transport_messenger_get(transport, addr);
        if (messenger) {
            JXTA_OBJECT_RELEASE(transport);
            JXTA_OBJECT_RELEASE(messenger);
            return TRUE;
        }
        JXTA_OBJECT_RELEASE(transport);
    }

    return FALSE;
}

static Jxta_endpoint_address *jxta_router_select_best_route(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap,
                                                            Jxta_AccessPointAdvertisement * hop)
{

    Jxta_endpoint_address *addr = NULL;

    /*
     * try first the direct route
     */
    addr = jxta_router_select_best_address(router, ap);
    if (addr != NULL)
        return addr;
    /*
     * now try the relay route
     */
    if (hop != NULL) {
        addr = jxta_router_select_best_address(router, hop);
    }

    return addr;
}

static Jxta_endpoint_address *jxta_router_select_best_address(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap)
{

    Jxta_vector *addresses = NULL;
    JString *addr = NULL;
    Jxta_endpoint_address *eaddr = NULL;
    Jxta_endpoint_address *best_addr = NULL;
    int i;
    Jxta_boolean found_http = FALSE;
    char *caddress = NULL;

    addresses = jxta_AccessPointAdvertisement_get_EndpointAddresses(ap);

    for (i = 0; i < jxta_vector_size(addresses); i++) {
        jxta_vector_get_object_at(addresses, (Jxta_object **) & addr, i);

        eaddr = jxta_endpoint_address_new((char *) jstring_get_string(addr));
        JXTA_OBJECT_RELEASE(addr);

        /*
         * we try first the TCP/IP Address as being
         * the best one
         */
        if (strncmp(jxta_endpoint_address_get_protocol_name(eaddr), "tcp", 3) == 0) {

            /*
             * NOTE: only use the address if it is a reachable address.
             * We test this by checking if we can get a messenger for
             * this destination.
             */
            if (reachable_address(router, eaddr)) {
                if (best_addr != NULL)
                    JXTA_OBJECT_RELEASE(best_addr);
                best_addr = eaddr;
                break;
            } else {
                JXTA_OBJECT_RELEASE(eaddr);
                continue;
            }
        }

        /*
         * continue searching for HTTP address only if we haven't found
         * a HTTP address. We want to still search for TCP/IP addresses
         */
        if ((strncmp(jxta_endpoint_address_get_protocol_name(eaddr), "http", 4) == 0) && !found_http) {

            /*
             * Http address is the second best
             */

            /*
             * NOTE: only use the address if it is a reachable address.
             * We test this by checking if we can get a messenger for
             * this destination.
             */
            if (reachable_address(router, eaddr)) {
                if (best_addr != NULL)
                    JXTA_OBJECT_RELEASE(best_addr);
                best_addr = eaddr;
                found_http = TRUE;
                continue;
            }
        }
        JXTA_OBJECT_RELEASE(eaddr);
    }

    if (best_addr != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Best address selected %s\n", (caddress = jxta_endpoint_address_to_string(best_addr)));
        free(caddress);
    }

    JXTA_OBJECT_RELEASE(addresses);
    return best_addr;
}

static void messenger_delete(Jxta_object * obj)
{
    Router_messenger *self = (Router_messenger *) obj;

    if (self->generic.address != NULL) {
        JXTA_OBJECT_RELEASE(self->generic.address);
        self->generic.address = NULL;
    }

    if (self->router != NULL) {
        JXTA_OBJECT_RELEASE(self->router);
        self->router = NULL;
    }

    if (self->peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->peerid);
        self->peerid = NULL;
    }

    free(self);
}

static Jxta_RouteAdvertisement *search_in_local_cm(Jxta_router_client * self, Jxta_id * dest)
{

    JString *pid;
    Jxta_PA *padv;
    Jxta_vector *res = NULL;
    int i;
    Jxta_RouteAdvertisement *route = NULL;

    /*
     * check if discovery pointer is valid
     */
    if (self->discovery == NULL) {
        jxta_PG_get_discovery_service(self->group, &(self->discovery));
    }

    /*
     * check if we have a route advertisement for that
     * destination. This can be either a route advertisement
     * or a peer advertisement. In the case of the peer
     * advertisement we have to extract the route.
     */

    if (self->discovery != NULL) {
        jxta_id_to_jstring(dest, &pid);
        discovery_service_get_local_advertisements(self->discovery, DISC_ADV, "DstPID", jstring_get_string(pid), &res);
        if (res != NULL) {
            for (i = 0; i < jxta_vector_size(res); i++) {
                jxta_vector_get_object_at(res, (Jxta_object **) & route, i);
                break;
            }

            if (route != NULL) {
                /*
                 * we found a route advertisement, just return it
                 */
                JXTA_OBJECT_RELEASE(res);
                JXTA_OBJECT_RELEASE(pid);
                return route;
            }
            JXTA_OBJECT_RELEASE(res);
        }

        discovery_service_get_local_advertisements(self->discovery, DISC_PEER, "PID", jstring_get_string(pid), &res);
        JXTA_OBJECT_RELEASE(pid);

        if (res != NULL) {
            for (i = 0; i < jxta_vector_size(res); i++) {
                jxta_vector_get_object_at(res, (Jxta_object **) & padv, i);
                break;
            }

            jxta_endpoint_service_get_route_from_PA(padv, &route);
            JXTA_OBJECT_RELEASE(res);
            JXTA_OBJECT_RELEASE(padv);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Discovery service is not available\n");
    }
    return route;
}

static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    Jxta_router_client *self = (Jxta_router_client *) t;
    Router_messenger *messenger = NULL;

    PTValid(self, Jxta_router_client);

    JXTA_OBJECT_CHECK_VALID(dest);
    JXTA_OBJECT_CHECK_VALID(t);

    messenger = (Router_messenger *) malloc(sizeof(Router_messenger));
    memset(messenger, 0, sizeof(Router_messenger));
    JXTA_OBJECT_INIT(messenger, messenger_delete, NULL);

    JXTA_OBJECT_SHARE(t);
    JXTA_OBJECT_SHARE(dest);
    messenger->generic.address = dest;
    messenger->generic.jxta_send = messenger_send;
    messenger->router = self;
    messenger->peerid = get_peerid_from_endpoint_address(dest);

    /* FIXME 20040316 tra need to watch for physical transport addresses
     * we cannot assume that we only have PeerIDs as now all messages
     * go through the router.
     *  if (messenger->peerid == NULL) {
     *    JXTA_LOG ("Cannot build peerid\n");
     *    JXTA_OBJECT_RELEASE (messenger);
     *    return NULL;
     *  }
     */

    if (messenger->peerid != NULL) {
        JXTA_OBJECT_CHECK_VALID(messenger->peerid);
    }

    return (JxtaEndpointMessenger *) messenger;
}

static
Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr)
{
    PTValid(t, Jxta_router_client);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "NOT YET IMPLEMENTED\n");
    return FALSE;
}

static
void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params)
{
  /**
   ** The JXTA Endpoint Router does not implement propagation.
   **/
    JXTA_OBJECT_RELEASE(msg);
}


Jxta_boolean allow_overload_p(Jxta_transport * tr)
{
    Jxta_router_client *self = (Jxta_router_client *) tr;
    PTValid(self, Jxta_router_client);
    return FALSE;
}

Jxta_boolean allow_routing_p(Jxta_transport * tr)
{
    Jxta_router_client *self = (Jxta_router_client *) tr;
    PTValid(self, Jxta_router_client);
    return TRUE;
}

Jxta_boolean connection_oriented_p(Jxta_transport * tr)
{
    Jxta_router_client *self = (Jxta_router_client *) tr;
    PTValid(self, Jxta_router_client);
    return TRUE;
}

typedef Jxta_transport_methods Jxta_router_client_methods;

Jxta_router_client_methods jxta_router_client_methods = {
    {
     "Jxta_module_methods",
     init,
     jxta_module_init_e_impl,
     start,
     stop},
    "Jxta_transport_methods",
    name_get,
    publicaddr_get,
    messenger_get,
    ping,
    propagate,
    allow_overload_p,
    allow_routing_p,
    connection_oriented_p
};

void jxta_router_client_construct(Jxta_router_client * self, Jxta_router_client_methods * methods)
{
    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods *) methods);
    self->thisType = "Jxta_router_client";

    self->endpoint = NULL;
    self->discovery = NULL;
    self->peers = jxta_vector_new(0);
    self->mutex = NULL;
    self->pool = NULL;
}

/*
 * The destructor is never called explicitly (except by subclass' code).
 * It is called by the free function (installed by the allocator) when the object
 * becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_router_client_destruct(Jxta_router_client * self)
{
    PTValid(self, Jxta_router_client);

    if (self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
        self->endpoint = NULL;
    }

    if (self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    if (self->peers) {
        JXTA_OBJECT_RELEASE(self->peers);
        self->peers = NULL;
    }


    if (self->localPeerAddr) {
        JXTA_OBJECT_RELEASE(self->localPeerAddr);
        self->peers = NULL;
    }

    if (self->pool) {
        apr_pool_destroy(self->pool);
        self->pool = NULL;
    }

    jxta_transport_destruct((Jxta_transport *) self);
}

static void router_free(Jxta_object * obj)
{
    jxta_router_client_destruct((Jxta_router_client *) obj);
    free((void *) obj);
}

Jxta_router_client *jxta_router_client_new_instance(void)
{
    Jxta_router_client *self = (Jxta_router_client *) malloc(sizeof(Jxta_router_client));
    memset(self, 0, sizeof(Jxta_router_client));
    JXTA_OBJECT_INIT(self, router_free, 0);
    jxta_router_client_construct(self, &jxta_router_client_methods);
    return self;
}

  /**
   ** The peerid (in a string representation) used by the JXTA Endpoint Router
   ** only contains the unique portion of the id. We need to build the complete
   ** URI before we can create a valid id.
   **/
static Jxta_id *get_peerid_from_endpoint_address(Jxta_endpoint_address * addr)
{
    char *pt = NULL;
    char *tmp = NULL;
    Jxta_id *id = NULL;
    JString *string = NULL;

    JXTA_OBJECT_CHECK_VALID(addr);

    if (addr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "addr is NULL\n");
        return NULL;
    }
    pt = jxta_endpoint_address_get_protocol_address(addr);
    if (pt == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve string from endpoint address.\n");
        return NULL;
    }

    tmp = malloc(strlen(pt) + strlen("urn:jxta:") + 1);
    sprintf(tmp, "%s%s", "urn:jxta:", pt);
    string = jstring_new_2(tmp);
    id = NULL;
    jxta_id_from_jstring(&id, string);

    JXTA_OBJECT_RELEASE(string);
    free(tmp);
    return id;
}

static Jxta_endpoint_address *get_endpoint_address_from_peerid(Jxta_id * peerid)
{
    JString *tmp = NULL;
    Jxta_endpoint_address *addr = NULL;
    char *pt;

    JXTA_OBJECT_CHECK_VALID(peerid);

    tmp = NULL;
    jxta_id_get_uniqueportion(peerid, &tmp);

    JXTA_OBJECT_CHECK_VALID(tmp);

    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the unique portion of the peer id\n");
        return NULL;
    }

    pt = (char *) jstring_get_string(tmp);
    addr = jxta_endpoint_address_new2((char *) "jxta", pt, NULL, NULL);

    JXTA_OBJECT_CHECK_VALID(addr);

    JXTA_OBJECT_RELEASE(tmp);
    return addr;
}

static void
updateRouteTable(Jxta_router_client * self,
                 Jxta_endpoint_address * src, Jxta_endpoint_address * last, Jxta_vector * reverseGateways)
{
    Jxta_vector *vector = NULL;
    Peer_entry *peer = NULL;
    Jxta_id *peerid = NULL;
    Jxta_status res;
    char *caddress = NULL;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(src);
    JXTA_OBJECT_CHECK_VALID(last);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "UPDATE ROUTE TABLE\n");

    if ((src == NULL) || (last == NULL)) {
        /* Invalid */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid parameters. Bail out.\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "src   = [%s]\n", (caddress = jxta_endpoint_address_to_string(src)));
    free(caddress);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "last  = [%s]\n", (caddress = jxta_endpoint_address_to_string(last)));
    free(caddress);

    /*
     * If both the src and last address are the same
     * then we have a direct route to the peer. No
     * routing is require. Should just send
     * message. In the current case, this means
     * forward the message to our relay
     */
    if (jxta_endpoint_address_equals(src, last)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No route update, direct connection\n");
        return;
    }

    if (reverseGateways == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No reverse gateways\n");
        vector = jxta_vector_new(1);

        JXTA_OBJECT_CHECK_VALID(vector);

        res = jxta_vector_add_object_first(vector, (Jxta_object *) last);
    } else {
        Jxta_endpoint_address *addr = NULL;
        vector = reverseGateways;

        JXTA_OBJECT_SHARE(vector);
    /**
     ** Make sure that the first element of the vector is last.
     **/
        res = jxta_vector_get_object_at(vector, (Jxta_object **) & addr, 0);
        if ((res != JXTA_SUCCESS) || (!jxta_endpoint_address_equals(addr, last))) {
      /**
       ** The first element of the reverse gateways is not last. Add last.
       **/
            res = jxta_vector_add_object_first(vector, (Jxta_object *) last);
            if (res != JXTA_SUCCESS) {
        /**
	 ** Something is wrong. Display a warning and bail out.
	 **/
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot add the last hop into the reverse gateays. Route is ignored.\n");
                JXTA_OBJECT_RELEASE(addr);
                JXTA_OBJECT_RELEASE(vector);
                return;
            }
        }
    }

  /**
   ** Get the Peer_entry for src, and set up the vector.
   ** At this point, either if reverseGateways was provided, or a new 
   ** vector has been created, we do have shared it already.
   **/
    peerid = get_peerid_from_endpoint_address(src);

    JXTA_OBJECT_CHECK_VALID(peerid);

    if (peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot build peerid\n");
    } else {
        peer = get_peer_entry(self, peerid);

        JXTA_OBJECT_CHECK_VALID(peer);

        JXTA_OBJECT_RELEASE(peerid);

        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get a Peer_entry\n");
        } else {
            peer_entry_set_forward_gateways(peer, vector);
            JXTA_OBJECT_RELEASE(peer);
        }
    }
    JXTA_OBJECT_RELEASE(vector);
}


/**
 ** This implementation only handles routed messages that are destinated for the
 ** the local peer.
 **/
static void router_client_listener(Jxta_object * obj, void *arg)
{

    Jxta_message_element *el = NULL;
    EndpointRouterMessage *rmsg;
    Jxta_router_client *self = (Jxta_router_client *) arg;
    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(arg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Router received a message\n");

    jxta_message_get_element_1(msg, JXTA_ENDPOINT_ROUTER_ELEMENT_NAME, &el);

    if (el) {
        Jxta_endpoint_address *realDest;
        Jxta_endpoint_address *realSrc;
        Jxta_endpoint_address *last;
        Jxta_endpoint_address *origSrc;
        Jxta_vector *reverseGateways;
        Jxta_bytevector *bytes = NULL;
        JString *string = NULL;

        origSrc = jxta_message_get_source(msg);
        rmsg = EndpointRouterMessage_new();
        bytes = jxta_message_element_get_value(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        EndpointRouterMessage_parse_charbuffer(rmsg, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);
        string = NULL;
        JXTA_OBJECT_RELEASE(el);
        el = NULL;

        realDest = EndpointRouterMessage_get_Dest(rmsg);
        realSrc = EndpointRouterMessage_get_Src(rmsg);
        last = EndpointRouterMessage_get_Last(rmsg);
        reverseGateways = EndpointRouterMessage_get_GatewayReverse(rmsg);

        JXTA_OBJECT_CHECK_VALID(realDest);
        JXTA_OBJECT_CHECK_VALID(realSrc);
        JXTA_OBJECT_CHECK_VALID(last);
        JXTA_OBJECT_CHECK_VALID(origSrc);
        JXTA_OBJECT_CHECK_VALID(reverseGateways);


    /**
     ** Check if we can learn a new route off this incoming message.
     **/

        updateRouteTable(self, last, origSrc, NULL);
        if (!jxta_endpoint_address_equals(realSrc, last)) {
            updateRouteTable(self, realSrc, last, reverseGateways);
        }

        JXTA_OBJECT_CHECK_VALID(realDest);
        JXTA_OBJECT_CHECK_VALID(realSrc);
        JXTA_OBJECT_CHECK_VALID(last);
        JXTA_OBJECT_CHECK_VALID(origSrc);
        JXTA_OBJECT_CHECK_VALID(reverseGateways);

    /**
     ** Check if this peer is the final destination of the message.
     **/

        if (realDest == NULL) {
            /* No destination drop message */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No router message element. Incoming message is dropped.\n");
        } else {

            if (strcmp(jxta_endpoint_address_get_protocol_address(realDest), self->localPeerIdString) == 0) {

                /* This is a message for the local peer */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received a routed message for the local peer\n");

        /**
	 ** Invoke the endpoint service demux again with the new addresses
	 **/

                jxta_message_set_destination(msg, realDest);
                jxta_message_set_source(msg, realSrc);

                JXTA_OBJECT_CHECK_VALID(msg);
                JXTA_OBJECT_CHECK_VALID(realDest);
                JXTA_OBJECT_CHECK_VALID(realSrc);
                JXTA_OBJECT_CHECK_VALID(last);
                JXTA_OBJECT_CHECK_VALID(origSrc);
                JXTA_OBJECT_CHECK_VALID(reverseGateways);

                jxta_endpoint_service_demux(self->endpoint, msg);
            }
        }

        if (realDest)
            JXTA_OBJECT_RELEASE(realDest);
        if (realSrc)
            JXTA_OBJECT_RELEASE(realSrc);
        if (last)
            JXTA_OBJECT_RELEASE(last);
        if (origSrc)
            JXTA_OBJECT_RELEASE(origSrc);
        if (reverseGateways)
            JXTA_OBJECT_RELEASE(reverseGateways);

        JXTA_OBJECT_RELEASE(rmsg);
    }
}

static void peer_entry_delete(Jxta_object * addr)
{

    Peer_entry *self = (Peer_entry *) addr;
    Peer_entry *entry = NULL;

    /* FIXME 20040316 tra we may have a memory leak here
     * as the entry we remove may be clone
     */
    jxta_vector_remove_object_at(self->router->peers, (Jxta_object **) & entry, self->loc);

    if (self->peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->peerid);
        self->peerid = NULL;
    }

    if (self->forwardGateways != NULL) {
        JXTA_OBJECT_RELEASE(self->forwardGateways);
        self->forwardGateways = NULL;
    }

    apr_pool_destroy(self->pool);

    free(self);
}

static Peer_entry *peer_entry_new(Jxta_router_client * router, Jxta_id * peerid)
{

    JString *pid;
    Peer_entry *self = (Peer_entry *) malloc(sizeof(Peer_entry));
    apr_status_t res;

    /* Initialize the object */
    memset(self, 0, sizeof(Peer_entry));
    JXTA_OBJECT_INIT(self, peer_entry_delete, 0);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->forwardGateways = NULL;
    if (peerid != NULL) {
        jxta_id_to_jstring(peerid, &pid);
        jxta_id_from_jstring(&self->peerid, pid);
    } else {
        self->peerid = NULL;
    }

    /*
     * keep a reference to the router
     * we need this to cleanup entry
     * from router entry vector list
     */
    self->router = router;
    return self;
}

static Jxta_vector *peer_entry_get_forward_gateways(Peer_entry * peer)
{

    Jxta_vector *vector = NULL;

    JXTA_OBJECT_CHECK_VALID(peer);

    apr_thread_mutex_lock(peer->mutex);

    if (peer->forwardGateways != NULL) {
        JXTA_OBJECT_SHARE(peer->forwardGateways);
        vector = peer->forwardGateways;
    }

    apr_thread_mutex_unlock(peer->mutex);
    return vector;
}

static void peer_entry_set_forward_gateways(Peer_entry * peer, Jxta_vector * vector)
{

    JXTA_OBJECT_CHECK_VALID(peer);
    JXTA_OBJECT_CHECK_VALID(vector);

    apr_thread_mutex_lock(peer->mutex);

    if (peer->forwardGateways != NULL) {
        JXTA_OBJECT_RELEASE(peer->forwardGateways);
        peer->forwardGateways = NULL;
    }

    if (vector != NULL) {
        JXTA_OBJECT_SHARE(vector);
        peer->forwardGateways = vector;
    }

    apr_thread_mutex_unlock(peer->mutex);
}

static Peer_entry *get_peer_entry(Jxta_router_client * self, Jxta_id * peerid)
{

    Peer_entry *peer = NULL;
    Jxta_status res = 0;
    int i;
    Jxta_boolean found = FALSE;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(peerid);

    apr_thread_mutex_lock(self->mutex);

    for (i = 0; i < jxta_vector_size(self->peers); ++i) {

        res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);

        JXTA_OBJECT_CHECK_VALID(peer);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot get Peer_entry\n");
            continue;
        }

        if (jxta_id_equals(peerid, peer->peerid)) {
            found = TRUE;
            break;
        }

        /* We are done with this peer. Let release it */
        JXTA_OBJECT_RELEASE(peer);
    }

    if (!found) {
        /* We need to create a new Peer_entry */
        peer = peer_entry_new(self, peerid);
        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create a Peer_entry\n");
        } else {
            /* Add it into the vector */
            peer->loc = jxta_vector_size(self->peers);
            res = jxta_vector_add_object_last(self->peers, (Jxta_object *) peer);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot insert Peer_entry into vector peers\n");
            }
        }
    }
    apr_thread_mutex_unlock(self->mutex);

    return peer;
}

/* vim: set ts=4 sw=4 tw=130 et: */
