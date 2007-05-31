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
 * $Id: jxta_rdv_service.h,v 1.11 2005/02/26 01:52:33 slowhog Exp $
 */

#ifndef __JXTA_RDV_SERVICE_H__
#define __JXTA_RDV_SERVICE_H__

#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_endpoint_address.h"
#include "jxta_message.h"
#include "jxta_peer.h"

/**
 *
 * This is the Rendezvous Service API
 *
 **/

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
    /*
     * API types.
     */ typedef Jxta_service Jxta_rdv_service;

  /**
   * The Rendezvous Service event types
   **/
typedef enum Jxta_Rendezvous_event_types {
    JXTA_RDV_CONNECTED = 1,
    JXTA_RDV_RECONNECTED = 2,
    JXTA_RDV_FAILED = 3,
    JXTA_RDV_DISCONNECTED = 4,
    JXTA_RDV_CLIENT_CONNECTED = 5,
    JXTA_RDV_CLIENT_RECONNECTED = 6,
    JXTA_RDV_CLIENT_DISCONNECTED = 8,
    JXTA_RDV_BECAME_EDGE = 9,
    JXTA_RDV_BECAME_RDV = 10,

    /* @deprecated To be removed very soon */
    JXTA_RDV_LEASE_REQUEST = 11,

    /* @deprecated To be removed very soon */
    JXTA_RDV_LEASE_GRANTED = 12
} Jxta_Rendezvous_event_type;

/**
    The event structure
*/
typedef struct _jxta_rdv_event {
    JXTA_OBJECT_HANDLE;
    Jxta_rdv_service *source;
    Jxta_Rendezvous_event_type event;
    Jxta_id *pid;
} Jxta_rdv_event;

/**
    A Listener for JXTA Rendezvous Service Events
*/
typedef Jxta_listener Jxta_rdv_event_listener;

    /**
     * Creates a new instance of the Rendezvous Service. The Service
     * is not initialized. Initialization must be done by jxta_rdv_service_init().
     *
     * @return a non initialized instance of the Rendezvous Service
     **/
Jxta_rdv_service *jxta_rdv_service_new(void);

    /**
     * Add a peer to the list of peers that the Rendezvous Service will
     * try to connect to.
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the peer to add
     * @return error code.
     **/

Jxta_status jxta_rdv_service_add_peer(Jxta_rdv_service * rdv, Jxta_peer * peer);


    /**
     * Add a peer to the list of seed peers that the Rendezvous Service will
     * try to connect to.
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the peer to add
     * @return error code.
     **/

Jxta_status jxta_rdv_service_add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer);

    /**
     * Get the list of peers used by the Rendezvous Service with their status.
     * 
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peerlist the list of peers (Jxta_peer*)
     * @return error code.
     **/
Jxta_status jxta_rdv_service_get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist);

    /**
     * Adds an Event listener that will receive rendezvous Service event, such
     * as connection, disconnection, lease request, lease grant events. Note that
     * the peer object is automatically shared by this method.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param serviceName pointer to a string containing the name of the service on which
     * the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated to the
     * serviceName.
     * @param listener a pointer to an event listener. 
     * @return error code.
     **/
Jxta_status jxta_rdv_service_add_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam, void *listener);

    /**
     * Removes an Event listener
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param serviceName pointer to a string containing the name of the service on which
     * the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated to the
     * serviceName.
     * @return error code.
     **/
Jxta_status jxta_rdv_service_remove_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam);

    /**
     * Test if the peer is a rendezvous peer for the given instance of rendezvous Service.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @return TRUE if the peer is a rendezvous, FALSE otherwise.
     **/
boolean jxta_rdv_service_is_rendezvous(Jxta_rdv_service * rdv);

    /**
     * Adds an Propagation listener. The listener is invoked when there is a incoming
     * propagated message for the given EndpointAddress.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param serviceName pointer to a string containing the name of the service on which
     * the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated to the
     * serviceName.
     * @param listener a pointer to an Jxta_listener.
     * @return error code. Fails if there was already a listener for that address.
     **/
Jxta_status jxta_rdv_service_add_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam,
                                                    Jxta_listener * listener);

    /**
     * Removes a Propagation listener.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param serviceName pointer to a string containing the name of the service on which
     * the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated to the
     * serviceName.
     * @param listener a pointer to the listener to be removed.
     * @return error code. Fails if there was not already a listener for that address.
     **/
Jxta_status jxta_rdv_service_remove_propagate_listener(Jxta_rdv_service * rdv,
                                                       char *serviceName, char *serviceParam, Jxta_listener * listener);

    /**
     * Propagates a message within the PeerGroup for which the instance of the 
     * Rendezvous Service is running in.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param msg the Jxta_message* to propagate.
     * @param serviceName pointer to a string containing the name of the service on which
     * the listener is listening on.
     * @param serviceParam pointer to a string containing the parameter associated to the
     * serviceName.
     * @param ttl Maximum number of peers the propagated message can go through.
     * @return error code.
     **/
Jxta_status jxta_rdv_service_propagate(Jxta_rdv_service * rdv, Jxta_message * msg, char *serviceName, char *serviceParam,
                                       int ttl);

    /**
     * Test if the given peer is connected
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the Jxta_peer to test upon
     * @return TRUE if the peer is connected
     **/
boolean jxta_rdv_service_peer_is_connected(Jxta_rdv_service * rdv, Jxta_peer * peer);

    /**
     * Return the time in absolute milliseconds time at which the provided peer will expire if not renewed.
     
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the Jxta_peer to test upon
     * @return the time in absolute milliseconds time at which the provided peer will expire if not renewed.
     **/
Jxta_time jxta_rdv_service_peer_get_expires(Jxta_rdv_service * rdv, Jxta_peer * peer);

    /**
     * Test if the given peer is known to be a rendezvous
     *
     * @deprecated If the peer is configured as a client and then getpeers returns a list of the connected rdvs.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the Jxta_peer to test upon
     * @return TRUE if the peer is a rendezvous, FALSE otherwise.
     **/
boolean jxta_rdv_service_peer_is_rdv(Jxta_rdv_service * rdv, Jxta_peer * peer);

#ifdef __cplusplus
}
#endif
#endif
