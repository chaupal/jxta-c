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

extern const char* RDV_V3_MCID;
extern const char* RDV_V3_MSID;

    /**
 *  Namespace for message elements we will send.
     **/
extern const char JXTA_RDV_NS_NAME[];

/**
*   Service Param of endpoint addresses for leasing messages.
**/
extern const char JXTA_RDV_LEASING_SERVICE_NAME[];

/**
*   Service Param of endpoint addresses for walker messages.
**/
extern const char JXTA_RDV_WALKER_SERVICE_NAME[];

/**
*   Service Param of endpoint addresses for propagate messages.
**/
extern const char JXTA_RDV_PROPAGATE_SERVICE_NAME[];

/**
*    Method table for rendezvous services.
**/
struct _jxta_rdv_service_methods {

    Extends(Jxta_service_methods);
};

typedef struct _jxta_rdv_service_methods _jxta_rdv_service_methods;

enum RendezVousStatuses {
    status_none=0,
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
    apr_thread_mutex_t *cb_mutex;
    apr_pool_t *pool;

    volatile Jxta_boolean running;

    Jxta_PG *group;
    Jxta_id *pid;
    char *assigned_id_str;

    Jxta_RdvConfigAdvertisement *rdvConfig;
    RdvConfig_configuration current_config;
    RdvConfig_configuration config;
    RdvConfig_configuration new_config;
    Jxta_boolean switch_config;

    Jxta_vector *active_seeds;
    Jxta_time last_seeding_update;
    Jxta_boolean shuffle;

    volatile Jxta_boolean auto_interval_lock;
    Jxta_time_diff auto_rdv_interval;

    Jxta_endpoint_service *endpoint;

    Jxta_hashtable *evt_listener_table;
    Jxta_hashtable *callback_table;
    Jxta_rendezvous_candidate_list_func candidate_list_func;
    Jxta_time connect_time;

    RendezVousStatus status;

    Jxta_rdv_service_provider *provider;

    Jxta_peerview *peerview;
    Jxta_listener *peerview_listener;

    void *ep_cookie_leasing;
    void *ep_cookie_walker;
    void *ep_cookie_prop;

    JString *lease_key_j;
    JString *walker_key_j;
    JString *prop_key_j;

    Jxta_boolean is_demoting;
    Jxta_boolean switching;
    apr_thread_mutex_t *switching_mutex;
    apr_thread_cond_t *switching_cv;

    Jxta_time service_start;
    unsigned int processing_callbacks;
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
extern void rdv_service_generate_event(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

extern Jxta_peerview *rdv_service_get_peerview_priv(Jxta_rdv_service * rdv);

/**
*   Returns the vector of active seeds peers. 
*
*  @param me A pointer to the instance of the Rendezvous Service
*  @param seeds A vector of seeds. Each is a Jxta_endpoint_address
*  @return error code.
*/
extern Jxta_status rdv_service_get_seeds(Jxta_rdv_service * me, Jxta_vector ** seeds, Jxta_boolean shuffle);

/**
*  Adds a referral seed.
*/
extern Jxta_status rdv_service_add_referral_seed(Jxta_rdv_service * me, Jxta_peer * seed);

/**
 * 
 * Add a callback for the rendezvous service
**/
extern Jxta_status rdv_service_add_cb(Jxta_rdv_service * me, Jxta_object **cookie, const char *name, const char *param,
                                                Jxta_callback_func func, void *arg);

extern Jxta_boolean rdv_service_call_candidate_list_cb(Jxta_rdv_service * rdv, Jxta_vector * candidates, Jxta_vector ** new_candidates, Jxta_boolean *shuffle);

/**
 *
 * Remove the callback from the rendezvous service
 *
**/
extern Jxta_status rdv_service_remove_cb(Jxta_rdv_service * me, Jxta_object *cookie);

/**
 *
 * Switch the config
 *
**/
extern Jxta_status rdv_service_switch_config(Jxta_rdv_service * rdv, RdvConfig_configuration config);

/**
*   Sends a broadcast request for seeds.
*/
extern Jxta_status rdv_service_send_seed_request(Jxta_rdv_service * rdv);

/**
*   define the instantiator method for creating a rdv ad-hoc. This should come from a header file.
**/
extern Jxta_rdv_service_provider *jxta_rdv_service_adhoc_new(apr_pool_t * pool);

/**
*   define the instantiator method for creating a rdv client. This should come from a header file.
**/
extern Jxta_rdv_service_provider *jxta_rdv_service_client_new(apr_pool_t * pool);

/**
*   define the instantiator method for creating a rdv server. This should come from a header file.
**/
extern Jxta_rdv_service_provider *jxta_rdv_service_server_new(apr_pool_t * pool);


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
