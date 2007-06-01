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
 * $Id: jxta_rdv_service_private.h,v 1.35 2006/08/15 21:18:27 bondolo Exp $
 */


#ifndef JXTA_RDV_SERVICE_PRIVATE_H
#define JXTA_RDV_SERVICE_PRIVATE_H

#include "jxta_apr.h"
#include "jxta_pipe_adv.h"
#include "jxta_rdv_service.h"
#include "jxta_service_private.h"
#include "jxta_rdv_service_provider.h"
#include "jxta_peerview.h"
#include "jxta_rdv_config_adv.h"


/****************************************************************
**
** This is the Rendezvous Service API for the implementations of
** this service
**
****************************************************************/

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

    /**
     ** Name of the service (as being used in forming the Endpoint Address).
     **/
extern const char JXTA_RDV_NS_NAME[];
extern const char JXTA_RDV_SERVICE_NAME[];
extern const char JXTA_RDV_PROPAGATE_ELEMENT_NAME[];
extern const char JXTA_RDV_PROPAGATE_SERVICE_NAME[];
extern const char JXTA_RDV_CONNECT_REQUEST_ELEMENT_NAME[];
extern const char JXTA_RDV_DISCONNECT_REQUEST_ELEMENT_NAME[];
extern const char JXTA_RDV_CONNECTED_REPLY_ELEMENT_NAME[];
extern const char JXTA_RDV_LEASE_REPLY_ELEMENT_NAME[];
extern const char JXTA_RDV_RDVADV_REPLY_ELEMENT_NAME[];
extern const char JXTA_RDV_ADV_ELEMENT_NAME[];

/**
*    Method table for rendezvous services.
**/
struct _jxta_rdv_service_methods {

    Extends(Jxta_service_methods);
};

typedef struct _jxta_rdv_service_methods _jxta_rdv_service_methods;

enum RendezVousStatuses {
    status_none,
    status_unknown,
    status_adhoc,
    status_edge,
    status_auto_edge,
    status_auto_rendezvous,
    status_rendezvous
};

typedef enum RendezVousStatuses RendezVousStatus;


/**
*    Object for Rendezvous services. This is a "final" class.
*    
**/
struct _jxta_rdv_service {
    Extends(Jxta_service);
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    volatile Jxta_boolean running;
    apr_thread_cond_t *periodicCond;
    apr_thread_mutex_t *periodicMutex;
    volatile apr_thread_t *periodicThread;

    Jxta_PG *parentgroup;

    RdvConfig_configuration config;

    volatile Jxta_boolean auto_interval_lock;
    Jxta_time_diff auto_rdv_interval;

    Jxta_endpoint_service *endpoint;

    Jxta_hashtable *evt_listener_table;

    RendezVousStatus status;

    volatile Jxta_rdv_service_provider *provider;

    JString *peerviewNameString;
    Jxta_peerview *peerview;
};

/**
*    Private typedef for Rendezvous services.
**/
typedef struct _jxta_rdv_service _jxta_rdv_service;

/**
*    Create a new rdv event and sent it to the listeners.
*    
*    @param rdv Rendezvous Service instance.
*    @param event The event which occured.
*    @param id The peer for which the event occurred.
*    
**/
JXTA_DECLARE(void) jxta_rdv_generate_event(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

Jxta_endpoint_service * jxta_rdv_service_endpoint_svc(_jxta_rdv_service * rdv);

JXTA_DECLARE(Jxta_peerview *) jxta_rdv_service_get_peerview_priv(_jxta_rdv_service * rdv);

/**
*   define the instantiator method for creating a rdv adhoc. This should come from a header file.
**/ 
extern Jxta_rdv_service_provider* jxta_rdv_service_adhoc_new(void);

/**
*   define the instantiator method for creating a rdv client. This should come from a header file.
**/ 
extern Jxta_rdv_service_provider* jxta_rdv_service_client_new(void);

/**
*   define the instantiator method for creating a rdv server. This should come from a header file.
**/
extern Jxta_rdv_service_provider* jxta_rdv_service_server_new(void);

extern _jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * rdv, const _jxta_rdv_service_methods * methods);
/**
 * Create a new message id
 * 
 * @return JString containing the message ID.
 */
JXTA_DECLARE(JString *) message_id_new(void);

#define JXTA_RDV_SERVICE_VTBL(self) (((_jxta_rdv_service_methods*)(((_jxta_module*) (self))->methods))

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif

/* vi: set ts=4 sw=4 tw=130 et: */
