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
 * $Id: jxta_router_client.c,v 1.95 2006/02/18 00:04:57 slowhog Exp $
 */

static const char *__log_cat = "ROUTER_CLIENT";

#include <stdlib.h> /* for atoi */

#include "jxta_apr.h"
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


struct _jxta_router_client {
    Extends(Jxta_transport);

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    Jxta_PG *group;
    Jxta_vector *peers;
    char *localPeerIdString;
    char *router_name;
    Jxta_id *localPeerId;
    Jxta_endpoint_address *localPeerAddr;
    Jxta_endpoint_service *endpoint;
    Jxta_discovery_service *discovery;
    Jxta_listener *listener;

    volatile Jxta_boolean started;
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
} Peer_entry;

extern Jxta_id *jxta_id_defaultNetPeerGroupID;

#define JXTA_ENDPOINT_PREFIX_ROUTER_NAME   "EndpointService:" 
#define JXTA_ENDPOINT_ROUTER_SVC_PAR       "EndpointRouter"
#define JXTA_ENDPOINT_ROUTER_ELEMENT_NAME  "jxta:EndpointRouterMsg"

static void JXTA_STDCALL router_client_listener(Jxta_object * msg, void *arg);
static Peer_entry *get_peer_entry(Jxta_router_client * self, Jxta_id * peerid, Jxta_boolean create);
static void peer_entry_set_forward_gateways(Peer_entry * peer, Jxta_vector * vector);
static Jxta_vector *peer_entry_get_forward_gateways(Peer_entry * peer);
static Jxta_id *get_peerid_from_endpoint_address(Jxta_endpoint_address * addr);
static Jxta_RouteAdvertisement *search_in_local_cm(Jxta_router_client * t, Jxta_id * dest);
static Jxta_endpoint_address *jxta_router_select_best_address(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap);
static Jxta_endpoint_address *jxta_router_select_best_route(Jxta_router_client * router,
                                                            Jxta_AccessPointAdvertisement * ap,
                                                            Jxta_AccessPointAdvertisement * hop);
static int reachable_address(Jxta_router_client * router, Jxta_endpoint_address * addr);

/******************
 * Module methods *
 *****************/

static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_router_client *self = PTValid(module, Jxta_router_client);
    JString *tmp = NULL;

    self->group = group;

    {
        jxta_PG_get_PID(group, &(self->localPeerId));
        jxta_id_get_uniqueportion(self->localPeerId, &tmp);
        self->localPeerIdString = malloc(jstring_length(tmp) + 1);
        strcpy(self->localPeerIdString, jstring_get_string(tmp));
        JXTA_OBJECT_RELEASE(tmp);
        jxta_PG_get_endpoint_service(group, &(self->endpoint));
    }

    {
        Jxta_id *groupId;
        int len;

        jxta_PG_get_GID(group, &groupId);
        if (jxta_id_equals(groupId, jxta_id_defaultNetPeerGroupID)) {
	        tmp = jstring_new_2("jxta-NetGroup");
        } else {
	        jxta_id_get_uniqueportion(groupId, &tmp);
        }
        JXTA_OBJECT_RELEASE(groupId);

        len=strlen(JXTA_ENDPOINT_PREFIX_ROUTER_NAME) + jstring_length(tmp);
        self->router_name = malloc( len + 1);

        strcpy(self->router_name, JXTA_ENDPOINT_PREFIX_ROUTER_NAME);
        strcat(self->router_name, jstring_get_string(tmp));

        self->router_name[len]=0;
        
        JXTA_OBJECT_RELEASE(tmp);
    }

    self->localPeerAddr = jxta_endpoint_address_new_3(self->localPeerId, NULL, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Inited.\n");

    return JXTA_SUCCESS;
}

static Jxta_status start(Jxta_module * module, const char *argv[])
{
    Jxta_router_client *self = PTValid(module, Jxta_router_client);

    apr_thread_mutex_lock(self->mutex);

    /**
     ** Add the router listener. Note that the Endpoint Router lives
     ** in the net peergroup.
     **/
    {
        JString *gid = NULL;

        jxta_id_get_uniqueportion(jxta_id_defaultNetPeerGroupID, &gid);

        self->listener = jxta_listener_new(router_client_listener, (void *) self, 10, 100);

        jxta_listener_start(self->listener);

        jxta_endpoint_service_add_listener(self->endpoint, self->router_name, 
                                           JXTA_ENDPOINT_ROUTER_SVC_PAR, self->listener);

        JXTA_OBJECT_RELEASE(gid);
    }

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    jxta_PG_get_discovery_service(self->group, &(self->discovery));

    self->started = TRUE;
    apr_thread_mutex_unlock(self->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started.\n");

    return JXTA_SUCCESS;
}

static void stop(Jxta_module * module)
{
    Jxta_router_client *self = PTValid(module, Jxta_router_client);

    apr_thread_mutex_lock(self->mutex);

    if (FALSE == self->started) {
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Router client has not started, ignore stopping request.\n");
        return;
    }

    self->started = FALSE;
    
    apr_thread_mutex_unlock(self->mutex);

    jxta_endpoint_service_remove_listener(self->endpoint, self->router_name, JXTA_ENDPOINT_ROUTER_SVC_PAR);
    jxta_listener_stop(self->listener);
    JXTA_OBJECT_RELEASE(self->listener);

    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);

    JXTA_OBJECT_RELEASE(self->endpoint);
    self->endpoint = NULL;

    if (NULL != self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}


static JString *name_get(Jxta_transport * self)
{
    PTValid(self, Jxta_router_client);
    return jstring_new_2("jxta");
}

static Jxta_endpoint_address *publicaddr_get(Jxta_transport * tpt)
{
    Jxta_router_client *self = PTValid(tpt, Jxta_router_client);

    JXTA_OBJECT_CHECK_VALID(self->localPeerAddr);

    return JXTA_OBJECT_SHARE(self->localPeerAddr);
}

/**
 ** Get the Peer_entry for the destination
 **/

static Jxta_status
get_peer_forward_gateways(Jxta_router_client * me, Jxta_id * peerid, 
                          Jxta_vector ** vector, Jxta_endpoint_address ** nextHopAddr)
{
    Jxta_status res;
    Peer_entry *peer = NULL;
    Jxta_id *id = NULL;
    Jxta_endpoint_address *addr = NULL;
    Jxta_vector *tmpVector = NULL;
    Jxta_AccessPointAdvertisement *apa = NULL;
    char *caddress = NULL;

    JXTA_OBJECT_CHECK_VALID(me);
    JXTA_OBJECT_CHECK_VALID(peerid);

    peer = get_peer_entry(me, peerid, JXTA_FALSE);
    if (peer == NULL) {
        /* No route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No existing route\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_CHECK_VALID(peer);
    *vector = peer_entry_get_forward_gateways(peer);
    if (*vector == NULL) {
        /* We do not have a valid route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Route is empty\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_CHECK_VALID(*vector);
    /* We do have a valid route. Get the first element, which is
     * the next hop, and get its endpoint address */
    res = jxta_vector_get_object_at(*vector, JXTA_OBJECT_PPTR(&apa), 0);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the next hop from forward gateway.\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    addr = jxta_router_select_best_route(me, apa, NULL);
    if (NULL == addr) {
        JString *apa_xml = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot find an physical address for next hop.\n");
        if (JXTA_SUCCESS == jxta_AccessPointAdvertisement_get_xml(apa, &apa_xml)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "APA for next hop: %s\n", jstring_get_string(apa_xml));
            JXTA_OBJECT_RELEASE(apa_xml);
        }
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_CHECK_VALID(addr);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Found a route %s\n", (caddress = jxta_endpoint_address_to_string(addr)));
    free(caddress);

    if (strcmp(jxta_endpoint_address_get_protocol_name(addr), "jxta") != 0) {
    /**
     ** The endpoint address for the next hop is a real transport address.
     ** That means that this peer has a direct connection to the destination.
     **/
        *nextHopAddr = addr;
        res = JXTA_SUCCESS;
        goto COMMON_EXIT;
    }

  /**
   ** We need to find the direct endpoint address for the next hop.
   **/
    id = get_peerid_from_endpoint_address(addr);

    JXTA_OBJECT_RELEASE(addr);

    if (id == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot build Peer ID of the next hop.\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_RELEASE(peer);
    peer = get_peer_entry(me, id, JXTA_FALSE);
    JXTA_OBJECT_RELEASE(id);

    if (peer == NULL) {
        /* No route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No existing route for next hop\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_CHECK_VALID(peer);

    tmpVector = peer_entry_get_forward_gateways(peer);

    if ((tmpVector == NULL) || (jxta_vector_size(tmpVector) == 0)) {
        /* We do not have a valid route */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Route is empty for next hop\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    JXTA_OBJECT_CHECK_VALID(tmpVector);
    apa = NULL;

    res = jxta_vector_get_object_at(tmpVector, JXTA_OBJECT_PPTR(&apa), 0);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the next hop from vector.\n");
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    addr = jxta_router_select_best_route(me, apa, NULL);
    if (NULL == addr) {
        JString *apa_xml = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot find an physical address for next hop.\n");
        if (JXTA_SUCCESS == jxta_AccessPointAdvertisement_get_xml(apa, &apa_xml)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "APA for next hop: %s\n", jstring_get_string(apa_xml));
            JXTA_OBJECT_RELEASE(apa_xml);
        }
        res = JXTA_INVALID_ARGUMENT;
        goto ERROR_EXIT;
    }

    *nextHopAddr = addr;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "next hop gateway addr %s://%s\n",
                    jxta_endpoint_address_get_protocol_name(addr), jxta_endpoint_address_get_protocol_address(addr));
    res = JXTA_SUCCESS;

COMMON_EXIT:
    if (NULL != tmpVector)
        JXTA_OBJECT_RELEASE(tmpVector);
    if (NULL != apa)
        JXTA_OBJECT_RELEASE(apa);
    if (NULL != peer)
        JXTA_OBJECT_RELEASE(peer);
    return res;

ERROR_EXIT:
    if (NULL != *vector) {
        JXTA_OBJECT_RELEASE(*vector);
        *vector = NULL;
    }
    *nextHopAddr = NULL;
    goto COMMON_EXIT;
}

static Jxta_status messenger_add_message_route(Router_messenger * messenger, Jxta_message * msg, Jxta_vector * forwardGateways)
{
    Jxta_status res;
    EndpointRouterMessage *rmsg = NULL;
    JString *xml = NULL;
    char *buffer = NULL;
    Jxta_message_element *msgElem = NULL;

    rmsg = EndpointRouterMessage_new();
    if (rmsg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate an EndpointRouterMessage\n");
        return JXTA_NOMEM;
    }

    /* Set rmsg */
    EndpointRouterMessage_set_Src(rmsg, messenger->router->localPeerAddr);
    EndpointRouterMessage_set_Last(rmsg, messenger->router->localPeerAddr);
    EndpointRouterMessage_set_Dest(rmsg, messenger->generic.address);
    EndpointRouterMessage_set_GatewayForward(rmsg, forwardGateways);

    /* Builds the XML */
    res = EndpointRouterMessage_get_xml(rmsg, &xml);
    JXTA_OBJECT_RELEASE(rmsg);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    JXTA_OBJECT_CHECK_VALID(xml);

    /**
     ** FIXME 3/10/2002 lomax@jxta.org
     **
     ** The Jxta_message implementation does not yet completely allow
     ** sharing of the data encapsulated within a message element. Copy is
     ** necessary for the time being. This will have to be changed when
     ** the Jxta_message implementation will be completed.
     **/
    buffer = malloc(jstring_length(xml));
    memcpy(buffer, jstring_get_string(xml), jstring_length(xml));
    msgElem = jxta_message_element_new_1(JXTA_ENDPOINT_ROUTER_ELEMENT_NAME, "text/xml", buffer, jstring_length(xml), NULL);
    JXTA_OBJECT_RELEASE(xml);
    free(buffer);
    if (NULL == msgElem) {
        return JXTA_FAILED;
    }

    jxta_message_remove_element_1(msg, JXTA_ENDPOINT_ROUTER_ELEMENT_NAME);
    res = jxta_message_add_element(msg, msgElem);

    JXTA_OBJECT_RELEASE(msgElem);

    return res;
}

static Jxta_status router_get_relay(Jxta_router_client * self, Jxta_endpoint_address ** addr)
{
    char *relay_addr = NULL;
    char *relay_proto = NULL;

    if (NULL == addr) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);
    if (TRUE != self->started) {
        apr_thread_mutex_unlock(self->mutex);
        *addr = NULL;
        return JXTA_VIOLATION;
    }

    relay_addr = jxta_endpoint_service_get_relay_addr(self->endpoint);

    if (relay_addr != NULL) {
        relay_proto = jxta_endpoint_service_get_relay_proto(self->endpoint);
    }

    apr_thread_mutex_unlock(self->mutex);

    if (NULL == relay_proto) {
        *addr = NULL;
    } else {
        *addr = jxta_endpoint_address_new_2(relay_proto, relay_addr, self->router_name, JXTA_ENDPOINT_ROUTER_SVC_PAR);
        free(relay_proto);
        free(relay_addr);
    }

    return (NULL != *addr) ? JXTA_SUCCESS : JXTA_FAILED;
}

static Jxta_status messenger_route(Router_messenger * messenger, Jxta_message * msg, Jxta_endpoint_address ** dest_addr)
{
    Jxta_status res;
    Jxta_vector *forwardGateways = NULL;
    Jxta_endpoint_address *new_addr = NULL;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_vector *hops = NULL;
    Jxta_AccessPointAdvertisement *ap = NULL;
    Jxta_AccessPointAdvertisement *hop = NULL;
    unsigned int sz;

    /* Get the Peer_entry for the destination */
    res = get_peer_forward_gateways(messenger->router, messenger->peerid, &forwardGateways, &new_addr);

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
            JString *routeadv;
            jxta_RouteAdvertisement_get_xml(route, &routeadv);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Route adv : \n%s\n", jstring_get_string(routeadv));
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
                res = jxta_vector_get_object_at(hops, JXTA_OBJECT_PPTR(&hop), sz - 1);
                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not extract last hop in route - \n");
                }
            }

            /*
             * we have a single hop route just select the destination
             */
            ap = jxta_RouteAdvertisement_get_Dest(route);

            new_addr = jxta_router_select_best_route(messenger->router, ap, hop);

            JXTA_OBJECT_RELEASE(ap);
            if (hop) {
                JXTA_OBJECT_RELEASE(hop);
            }

            if (new_addr != NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Send msg to address %s://%s\n",
                                jxta_endpoint_address_get_protocol_name(new_addr),
                                jxta_endpoint_address_get_protocol_address(new_addr));
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Route found in local cm is no good\n");
            }

            JXTA_OBJECT_RELEASE(hops);
            JXTA_OBJECT_RELEASE(route);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Could not find route in local cm\n");
        }

        /* Cannot find a valid route in local cm, go relay */
        if (new_addr == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No route found - try to send message to our relay\n");
            /*
             * Check if we have a relay yet, if not, then there
             * is no much we can do here until we implement the
             * dynamic route discovery
             *
             * FIXME 20040225 tra need to implement route discovery
             *
             */
            res = router_get_relay(messenger->router, &new_addr);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No relay connection, discard message\n");
                return JXTA_FAILED;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward msg to relay %s://%s\n",
                            jxta_endpoint_address_get_protocol_name(new_addr),
                            jxta_endpoint_address_get_protocol_address(new_addr));
        }

        forwardGateways = jxta_vector_new(0);
    }

    /* We have a route. Build a router element, add it into the message, and send it. */
    res = messenger_add_message_route(messenger, msg, forwardGateways);
    if (JXTA_SUCCESS == res) {
        /* Set the destination address of the message. */
        *dest_addr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(new_addr),
                                                 jxta_endpoint_address_get_protocol_address(new_addr),
                                                 messenger->router->router_name, JXTA_ENDPOINT_ROUTER_SVC_PAR);
        if (NULL == *dest_addr) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to compose destination address, discard message\n");
            res = JXTA_FAILED;
        } else {
            res = jxta_message_set_destination(msg, *dest_addr);
        }
    }

    JXTA_OBJECT_RELEASE(new_addr);
    JXTA_OBJECT_RELEASE(forwardGateways);
    return res;
}

static Jxta_status messenger_send(JxtaEndpointMessenger * m, Jxta_message * msg)
{
    Jxta_status status;
    Router_messenger *messenger = (Router_messenger *) m;
    JString *dest = NULL;
    Jxta_endpoint_address *dest_addr = NULL;
    char *caddress = NULL;
    Jxta_transport *transport = NULL;
    const char *transport_name;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(messenger);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Router_messenger.send()\n");

    if (FALSE == messenger->router->started) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to send a message after router was stopped.\n");
        return JXTA_FAILED;
    }

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
        dest_addr = jxta_message_get_destination(msg);
    } else {
      /**
       ** Check if destination is local
       **/
        jxta_id_get_uniqueportion(messenger->peerid, &dest);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Routing destination '%s'\n", jstring_get_string(dest));

        if (strcmp(messenger->router->localPeerIdString, jstring_get_string(dest)) == 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Local destination\n");

            apr_thread_mutex_lock(messenger->router->mutex);
            if (FALSE != messenger->router->started) {
                jxta_endpoint_service_demux(messenger->router->endpoint, msg);
                status = JXTA_SUCCESS;
            } else {
                status = JXTA_FAILED;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to send a message after router was stopped.\n");
            }
            apr_thread_mutex_unlock(messenger->router->mutex);
            JXTA_OBJECT_RELEASE(dest);
            return status;
        } else {
            JXTA_OBJECT_RELEASE(dest);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remote destination\n");
            if (JXTA_SUCCESS != messenger_route(messenger, msg, &dest_addr)) {
                return JXTA_FAILED;
            }
        }
    }

    caddress = jxta_endpoint_address_to_string(dest_addr);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send destination %s\n", caddress);

    apr_thread_mutex_lock(messenger->router->mutex);
    if (FALSE == messenger->router->started) {
        apr_thread_mutex_unlock(messenger->router->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to send a message after router was stopped.\n");
        free(caddress);
        JXTA_OBJECT_RELEASE(dest_addr);
        return JXTA_FAILED;
    }
    transport_name = jxta_endpoint_address_get_protocol_name(dest_addr);
    transport = jxta_endpoint_service_lookup_transport(messenger->router->endpoint, transport_name);
    apr_thread_mutex_unlock(messenger->router->mutex);

    if (NULL == transport) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not obtain a transport for : %s\n", caddress);
        free(caddress);
        JXTA_OBJECT_RELEASE(dest_addr);
        return JXTA_FAILED;
    }

    endpoint_messenger = jxta_transport_messenger_get(transport, dest_addr);
    if (NULL == endpoint_messenger) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not obtain a messenger for : %s\n", caddress);
        free(caddress);
        JXTA_OBJECT_RELEASE(dest_addr);
        JXTA_OBJECT_RELEASE(transport);
        return JXTA_FAILED;
    }

    free(caddress);
    JXTA_OBJECT_RELEASE(dest_addr);

    JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
    addr = jxta_transport_publicaddr_get(transport);
    jxta_message_set_source(msg, addr);
    status = endpoint_messenger->jxta_send(endpoint_messenger, msg);
    JXTA_OBJECT_RELEASE(endpoint_messenger);
    JXTA_OBJECT_RELEASE(addr);
    JXTA_OBJECT_RELEASE(transport);

    return status;
}

static int reachable_address(Jxta_router_client * router, Jxta_endpoint_address * addr)
{
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *messenger = NULL;

    transport = jxta_endpoint_service_lookup_transport(router->endpoint, jxta_endpoint_address_get_protocol_name(addr));
    if (transport) {
        messenger = jxta_transport_messenger_get(transport, addr);
        if (messenger) {
            JXTA_OBJECT_RELEASE(transport);
            JXTA_OBJECT_RELEASE(messenger);
            return jxta_transport_metric_get(transport);
        }
        JXTA_OBJECT_RELEASE(transport);
    }

    return INT_MIN;
}

static Jxta_endpoint_address *jxta_router_select_best_route(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap,
                                                            Jxta_AccessPointAdvertisement * hop)
{
    Jxta_endpoint_address *addr = NULL;

    apr_thread_mutex_lock(router->mutex);
    if (!router->started) {
        apr_thread_mutex_unlock(router->mutex);
        return NULL;
    }

    /* try first the direct route, then relay */
    addr = jxta_router_select_best_address(router, ap);
    if (NULL == addr && NULL != hop) {
        addr = jxta_router_select_best_address(router, hop);
    }

    apr_thread_mutex_unlock(router->mutex);
    return addr;
}

static Jxta_endpoint_address *jxta_router_select_best_address(Jxta_router_client * router, Jxta_AccessPointAdvertisement * ap)
{
    Jxta_vector *addresses = NULL;
    JString *addr = NULL;
    Jxta_endpoint_address *eaddr = NULL;
    Jxta_endpoint_address *best_addr = NULL;
    unsigned int i;
    Jxta_boolean found_http = FALSE;
    char *caddress = NULL;
    int metric, best = INT_MIN;

    addresses = jxta_AccessPointAdvertisement_get_EndpointAddresses(ap);

    for (i = 0; i < jxta_vector_size(addresses); i++) {
        jxta_vector_get_object_at(addresses, JXTA_OBJECT_PPTR(&addr), i);

        eaddr = jxta_endpoint_address_new(jstring_get_string(addr));
        JXTA_OBJECT_RELEASE(addr);

        metric = reachable_address(router, eaddr);
        if (metric > best) {
            if (best_addr != NULL)
                JXTA_OBJECT_RELEASE(best_addr);
            best_addr = eaddr;
            best = metric;
        } else {
            JXTA_OBJECT_RELEASE(eaddr);
        }
    }

    if (best_addr != NULL) {
        caddress = jxta_endpoint_address_to_string(best_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Best address selected %s\n",caddress);
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
    unsigned int i;
    Jxta_RouteAdvertisement *route = NULL;

    apr_thread_mutex_lock(self->mutex);
    if (FALSE == self->started) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Try to search local cm while router is not started\n");
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }
    /*
     * check if discovery pointer is valid
     */
    if (self->discovery == NULL) {
        jxta_PG_get_discovery_service(self->group, &(self->discovery));
    }
    if (self->discovery == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Discovery service is not available\n");
        apr_thread_mutex_unlock(self->mutex);
        return NULL;
    }

    /*
     * check if we have a route advertisement for that
     * destination. This can be either a route advertisement
     * or a peer advertisement. In the case of the peer
     * advertisement we have to extract the route.
     */
    jxta_id_to_jstring(dest, &pid);
    discovery_service_get_local_advertisements(self->discovery, DISC_ADV, "DstPID", jstring_get_string(pid), &res);
    if (res != NULL) {
        for (i = 0; i < jxta_vector_size(res); i++) {
            jxta_vector_get_object_at(res, JXTA_OBJECT_PPTR(&route), i);
            if (route != NULL)
                break;
        }
        JXTA_OBJECT_RELEASE(res);
    }

    if (NULL == route) {
        discovery_service_get_local_advertisements(self->discovery, DISC_PEER, "PID", jstring_get_string(pid), &res);
        if (res != NULL) {
            for (i = 0; i < jxta_vector_size(res); i++) {
                jxta_vector_get_object_at(res, JXTA_OBJECT_PPTR(&padv), i);
                if (NULL != padv)
                    break;
            }
            JXTA_OBJECT_RELEASE(res);

            if (NULL != padv) {
                jxta_endpoint_service_get_route_from_PA(padv, &route);
                JXTA_OBJECT_RELEASE(padv);
            }
        }
    }

    JXTA_OBJECT_RELEASE(pid);
    apr_thread_mutex_unlock(self->mutex);
    return route;
}

static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    Jxta_router_client *self = PTValid(t, Jxta_router_client);
    Router_messenger *messenger = NULL;

    JXTA_OBJECT_CHECK_VALID(dest);

    messenger = (Router_messenger *) calloc(1, sizeof(Router_messenger));

    JXTA_OBJECT_INIT(messenger, messenger_delete, NULL);

    messenger->router = JXTA_OBJECT_SHARE(self);
    messenger->generic.address = JXTA_OBJECT_SHARE(dest);
    messenger->generic.jxta_send = messenger_send;
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
void propagate(Jxta_transport * tpt, Jxta_message * msg, const char *service_name, const char *service_params)
{
    Jxta_router_client *self = PTValid(tpt, Jxta_router_client);

  /**
   ** The JXTA Endpoint Router does not implement propagation.
   **/
}

Jxta_boolean allow_overload_p(Jxta_transport * tpt)
{
    Jxta_router_client *self = PTValid(tpt, Jxta_router_client);

    return FALSE;
}

Jxta_boolean allow_routing_p(Jxta_transport * tpt)
{
    Jxta_router_client *self = PTValid(tpt, Jxta_router_client);

    return TRUE;
}

Jxta_boolean connection_oriented_p(Jxta_transport * tpt)
{
    Jxta_router_client *self = PTValid(tpt, Jxta_router_client);

    return TRUE;
}

typedef Jxta_transport_methods Jxta_router_client_methods;

const Jxta_router_client_methods jxta_router_client_methods = {
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

Jxta_router_client *jxta_router_client_construct(Jxta_router_client * self, Jxta_router_client_methods const *methods)
{
    apr_status_t res;

    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods const *) methods);
    self->thisType = "Jxta_router_client";
    self->_super.direction = JXTA_OUTBOUND;

    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    /**
     ** Create our mutex.
     **/
    res = apr_thread_mutex_create(&(self->mutex), APR_THREAD_MUTEX_NESTED,  /* nested */
                                  self->pool);
    if (res != APR_SUCCESS) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->endpoint = NULL;
    self->discovery = NULL;
    self->peers = jxta_vector_new(0);
    self->started = FALSE;

    return self;
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

    if (self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    if (self->peers) {
        JXTA_OBJECT_RELEASE(self->peers);
        self->peers = NULL;
    }

    if (self->localPeerIdString) {
        free(self->localPeerIdString);
        self->localPeerIdString = NULL;
    }

    if (self->localPeerId) {
        JXTA_OBJECT_RELEASE(self->localPeerId);
        self->localPeerId = NULL;
    }

    if (self->router_name) {
        free(self->router_name);
        self->router_name = NULL;
    }

    if (self->localPeerAddr) {
        JXTA_OBJECT_RELEASE(self->localPeerAddr);
        self->localPeerAddr = NULL;
    }

    if(self->mutex){
        apr_thread_mutex_destroy(self->mutex);
        self->mutex=NULL;
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
    Jxta_router_client *self = (Jxta_router_client *) calloc(1, sizeof(Jxta_router_client));

    JXTA_OBJECT_INIT(self, router_free, 0);

    return jxta_router_client_construct(self, &jxta_router_client_methods);
}

  /**
   ** The peerid (in a string representation) used by the JXTA Endpoint Router
   ** only contains the unique portion of the id. We need to build the complete
   ** URI before we can create a valid id.
   **/
static Jxta_id *get_peerid_from_endpoint_address(Jxta_endpoint_address * addr)
{
    const char *pt = NULL;
    char *tmp = NULL;
    int tmp_len;
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

    tmp_len = strlen(pt) + strlen("urn:jxta:") + 1;
    tmp = malloc(tmp_len);
    apr_snprintf(tmp, tmp_len, "%s%s", "urn:jxta:", pt);
    string = jstring_new_2(tmp);
    id = NULL;
    jxta_id_from_jstring(&id, string);

    JXTA_OBJECT_RELEASE(string);
    free(tmp);
    return id;
}

static void updateRouteTable(Jxta_router_client * self, Jxta_endpoint_address * dest, Jxta_vector * reverseGateways)
{
    Peer_entry *peer = NULL;
    Jxta_id *peerid = NULL;
    char *caddress = NULL;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(dest);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "UPDATE ROUTE TABLE\n");

    if (NULL == dest || NULL == reverseGateways) {
        /* Invalid */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid parameters. Bail out.\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "dest   = [%s]\n", (caddress = jxta_endpoint_address_to_string(dest)));
    free(caddress);

    /**
    ** Get the Peer_entry for dest, and set up the vector.
    **/
    peerid = get_peerid_from_endpoint_address(dest);

    JXTA_OBJECT_CHECK_VALID(peerid);

    if (peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot build peerid\n");
    } else {
        peer = get_peer_entry(self, peerid, JXTA_TRUE);

        JXTA_OBJECT_CHECK_VALID(peer);

        JXTA_OBJECT_RELEASE(peerid);

        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get a Peer_entry\n");
        } else {
            peer_entry_set_forward_gateways(peer, reverseGateways);
            JXTA_OBJECT_RELEASE(peer);
        }
    }
}

/**
 ** This implementation only handles routed messages that are destinated for the
 ** the local peer.
 **/
static void JXTA_STDCALL router_client_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_router_client *self = PTValid(arg, Jxta_router_client); 
    Jxta_message_element *el = NULL;
    EndpointRouterMessage *rmsg;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_endpoint_address *realDest;
    Jxta_endpoint_address *realSrc;
    Jxta_endpoint_address *last;
    Jxta_endpoint_address *origSrc;
    Jxta_AccessPointAdvertisement *apa = NULL;
    Jxta_id *last_hop = NULL;
    Jxta_vector *reverseGateways;
    Jxta_bytevector *bytes = NULL;
    JString *string = NULL;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Router received a message\n");

    res = jxta_message_get_element_1(msg, JXTA_ENDPOINT_ROUTER_ELEMENT_NAME, &el);

    if (JXTA_SUCCESS != res || NULL == el) {
        /* No destination drop message */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, 
                        "No router message element in msg [%p]. Incoming message is dropped.\n", msg );

        return;
    }

    origSrc = jxta_message_get_source(msg);
    bytes = jxta_message_element_get_value(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    bytes = NULL;
    rmsg = EndpointRouterMessage_new();
    res = EndpointRouterMessage_parse_charbuffer(rmsg, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);
    string = NULL;
    JXTA_OBJECT_RELEASE(el);
    el = NULL;
    
    if( res != JXTA_SUCCESS ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed parsing router message element in msg [%p]. Incoming message is dropped.\n", msg );
        JXTA_OBJECT_RELEASE(rmsg);
        return;
    }

    realDest = EndpointRouterMessage_get_Dest(rmsg);
    realSrc = EndpointRouterMessage_get_Src(rmsg);
    last = EndpointRouterMessage_get_Last(rmsg);

    JXTA_OBJECT_CHECK_VALID(realDest);
    JXTA_OBJECT_CHECK_VALID(realSrc);
    JXTA_OBJECT_CHECK_VALID(last);
    JXTA_OBJECT_CHECK_VALID(origSrc);

    /*
     * Check if we can learn a new route off this incoming message.
     */
    apa = jxta_AccessPointAdvertisement_new();
    if (NULL == apa) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to allocate APA\n");
        goto ERROR_EXIT;
    }

    last_hop = get_peerid_from_endpoint_address(last);
    if (NULL == last_hop) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Endpoint address %s cannot be converted to a Peer ID.\n",
                        last);
        goto ERROR_EXIT;
    }
    jxta_AccessPointAdvertisement_set_PID(apa, last_hop);

    if (! jxta_endpoint_address_equals(last, origSrc)) {
        jxta_AccessPointAdvertisement_add_EndpointAddress(apa, origSrc);
        reverseGateways = jxta_vector_new(1);
        if (NULL == reverseGateways) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to allocate vector\n");
            goto ERROR_EXIT;
        }

        res = jxta_vector_add_object_first(reverseGateways, (Jxta_object *) apa);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE
                            "Cannot add the last hop into the reverse gateays. Route is ignored.\n");
        } else {
            updateRouteTable(self, last, reverseGateways);
        }
        JXTA_OBJECT_RELEASE(reverseGateways);
        reverseGateways = NULL;
    }

    if (!jxta_endpoint_address_equals(realSrc, last)) {
        reverseGateways = EndpointRouterMessage_get_GatewayReverse(rmsg);
        JXTA_OBJECT_CHECK_VALID(reverseGateways);
        res = jxta_vector_add_object_first(reverseGateways, (Jxta_object *) apa);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE
                            "Cannot add the last hop into the reverse gateays. Route is ignored.\n");
        } else {
            updateRouteTable(self, realSrc, reverseGateways);
        }
        JXTA_OBJECT_RELEASE(reverseGateways);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No route update, direct connection\n");
    }

    JXTA_OBJECT_CHECK_VALID(realDest);
    JXTA_OBJECT_CHECK_VALID(realSrc);
    JXTA_OBJECT_CHECK_VALID(last);
    JXTA_OBJECT_CHECK_VALID(origSrc);

    /*
     * Check if this peer is the final destination of the message.
     */
    if (realDest == NULL) {
        /* No destination drop message */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No destination specified. Incoming message is dropped.\n");
    } else {
        if (strcmp(jxta_endpoint_address_get_protocol_address(realDest), self->localPeerIdString) == 0) {

            /* This is a message for the local peer */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received a routed message for the local peer\n");

            /**
         ** Invoke the endpoint service demux again with the new addresses
         **/

            JXTA_OBJECT_CHECK_VALID(msg);
            JXTA_OBJECT_CHECK_VALID(realDest);
            JXTA_OBJECT_CHECK_VALID(realSrc);

            jxta_message_set_source(msg, realSrc);

            jxta_endpoint_service_demux_addr(self->endpoint, realDest, msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Recevied a routed message [%p] for an unrecognized destination [%s]\n", msg, jxta_endpoint_address_get_protocol_address(realDest) );
        }
    }

ERROR_EXIT:
    if (apa)
        JXTA_OBJECT_RELEASE(apa);
    if (last_hop)
        JXTA_OBJECT_RELEASE(last_hop);
    if (realDest)
        JXTA_OBJECT_RELEASE(realDest);
    if (realSrc)
        JXTA_OBJECT_RELEASE(realSrc);
    if (last)
        JXTA_OBJECT_RELEASE(last);
    if (origSrc)
        JXTA_OBJECT_RELEASE(origSrc);

    JXTA_OBJECT_RELEASE(rmsg);
}

static void peer_entry_delete(Jxta_object * addr)
{

    Peer_entry *self = (Peer_entry *) addr;

    if (self->peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->peerid);
        self->peerid = NULL;
    }

    if (self->forwardGateways != NULL) {
        JXTA_OBJECT_RELEASE(self->forwardGateways);
        self->forwardGateways = NULL;
    }

    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    free(self);
}

static Peer_entry *peer_entry_new(Jxta_id * peerid)
{

    JString *pid;
    Peer_entry *self = (Peer_entry *) malloc(sizeof(Peer_entry));
    apr_status_t res;

    /* Initialize the object */
    memset(self, 0, sizeof(Peer_entry));
    JXTA_OBJECT_INIT(self, peer_entry_delete, 0);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
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
        JXTA_OBJECT_RELEASE(pid);
    } else {
        self->peerid = NULL;
    }

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

static Peer_entry *get_peer_entry(Jxta_router_client * self, Jxta_id * peerid, Jxta_boolean create)
{
    Peer_entry *peer = NULL;
    Jxta_status res = 0;
    unsigned int i;
    Jxta_boolean found = FALSE;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(peerid);

    apr_thread_mutex_lock(self->mutex);

    for (i = 0; i < jxta_vector_size(self->peers); ++i) {

        res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);

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
        if (create) {
            /* We need to create a new Peer_entry */
            peer = peer_entry_new(peerid);
            if (peer == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create a Peer_entry\n");
            } else {
                /* Add it into the vector */
                res = jxta_vector_add_object_last(self->peers, (Jxta_object *) peer);
                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot insert Peer_entry into vector peers\n");
                }
            }
        } else {
            peer = NULL;
        }
    }
    apr_thread_mutex_unlock(self->mutex);

    return peer;
}

/* vim: set ts=4 sw=4 tw=130 et: */
