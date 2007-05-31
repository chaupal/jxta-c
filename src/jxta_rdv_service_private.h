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
 * $Id: jxta_rdv_service_private.h,v 1.20 2005/02/01 00:19:24 bondolo Exp $
 */


#ifndef JXTA_RDV_SERVICE_PRIVATE_H
#define JXTA_RDV_SERVICE_PRIVATE_H

#include "jxtaapr.h"
#include "jxta_pipe_adv.h"
#include "jxta_rdv_service.h"
#include "jxta_service_private.h"
#include "jxta_rdv_service_provider.h"


/****************************************************************
**
** This is the Rendezvous Service API for the implementations of
** this service
**
****************************************************************/

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
    /**
     ** Name of the service (as being used in forming the Endpoint Address).
     **/ extern const char *JXTA_RDV_SERVICE_NAME;
extern const char *JXTA_RDV_PROPAGATE_ELEMENT_NAME;
extern const char *JXTA_RDV_PROPAGATE_SERVICE_NAME;
extern const char *JXTA_RDV_CONNECT_REQUEST;
extern const char *JXTA_RDV_DISCONNECT_REQUEST;
extern const char *JXTA_RDV_CONNECTED_REPLY;
extern const char *JXTA_RDV_LEASE_REPLY;
extern const char *JXTA_RDV_RDVADV_REPLY;
extern const char *JXTA_RDV_PING_REQUEST;
extern const char *JXTA_RDV_PING_REPLY;
extern const char *JXTA_RDV_ADV_REPLY;
extern const char *JXTA_RDV_PEERVIEW_NAME;
extern const char *JXTA_RDV_PEERVIEW_EDGE;
extern const char *JXTA_RDV_PEERVIEW_RESPONSE;
extern const char *JXTA_RDV_PEERVIEW_RDV;
extern const char *JXTA_RDV_PEERVIEW_PEERADV;

/**
    Method table for rendezvous services.
**/
struct _jxta_rdv_service_methods {

    Extends(Jxta_service_methods);

    /******
     * @param service a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to the list of peer add.
     * @return error code.
     *******/
    Jxta_status(*add_peer) (Jxta_rdv_service * rdv, Jxta_peer * peer);

    /******
     * @param service a pointer to the instance of the Rendezvous Service
     * @param peer a pointer to a seed peer.
     * @return error code.
     *******/
    Jxta_status(*add_seed) (Jxta_rdv_service * rdv, Jxta_peer * peer);

    /******
     ** Get the list of peers used by the Rendezvous Service with their status.
     * 
     * @param service a pointer to the instance of the Rendezvous Service
     * @param pAdv a pointer to a pointer that contains the list of peers
     * @return error code.
     *******/
    Jxta_status(*get_peers) (Jxta_rdv_service * rdv, Jxta_vector ** peerlist);

    /******
     * Propagates a message within the PeerGroup for which the instance of the 
     * Rendezvous Service is running in.
     *
     * @param service a pointer to the instance of the Rendezvous Service
     * @param msg the Jxta_message* to propagate.
     * @param addr An EndpointAddress that defines on which address the message is destinated
     * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
     * @param ttl Maximum number of peers the propagated message can go through.
     * @return error code.
     *******/
    Jxta_status(*propagate) (Jxta_rdv_service * rdv, Jxta_message * msg, char *serviceName, char *serviceParam, int ttl);
};

typedef struct _jxta_rdv_service_methods _jxta_rdv_service_methods;

/**
    Object for Rendezvous services. This is a "final" class.
    
    Extensions are 
**/
struct _jxta_rdv_service {
    Extends(_jxta_service);
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    Jxta_endpoint_service *endpoint;

    Jxta_hashtable *evt_listener_table;

    Jxta_rdv_service_provider *provider;
};

/**
    Private typedef for Rendezvous services.
**/
typedef struct _jxta_rdv_service _jxta_rdv_service;

/**
    Create a new rdv event and sent it to the listeners.
    
    @param rdv Rendezvous Service instance.
    @param event The event which occured.
    @param id The peer for which the event occurred.
    
**/
extern void jxta_rdv_generate_event(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

extern Jxta_pipe_adv *jxta_rdv_get_wirepipeadv(_jxta_rdv_service * rdv);

extern Jxta_status jxta_rdv_send_rdv_request(_jxta_rdv_service * rdv, Jxta_pipe_adv * adv );

extern Jxta_endpoint_service *jxta_rdv_service_get_endpoint_priv(_jxta_rdv_service * rdv);


#define JXTA_RDV_SERVICE_VTBL(self) (((_jxta_rdv_service_methods*)(((_jxta_module*) (self))->methods))

#ifdef __cplusplus
}
#endif

#endif
