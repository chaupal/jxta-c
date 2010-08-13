/**
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
 *    Alternately, this acknowledgment may appear in the software itmyself,
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
 **/


/**
 *
 * This is the edge Client Rendezvous Service implementation
 *
 * And edge client is capable to connect to rendezvous peers, send and receive
 * propagated messages. An edge client is not capable of acting as a rendezvous
 * nor to re-propagate a received propagated message.
 *
 * An edge client can retrieve the list of rendezvous peers it is connected to,
 * but is not capable of setting them (only configuration or myself discovery).
 **/

static const char *__log_cat = "RdvEdge";

#include <limits.h>

#include "jxta_apr.h"

#include "jxta_log.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jstring.h"
#include "jxta_vector.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_rdv_diffusion_msg.h"
#include "jxta_peergroup.h"
#include "jxta_id.h"
#include "jxta_id_uuid_priv.h"
#include "jxta_lease_request_msg.h"
#include "jxta_lease_response_msg.h"
#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_lease_options.h"
#include "jxta_rdv_service_provider_private.h"

/**
 * Time the client waits when there is a candidate list call back before attempting a rendezvous connection
**/
static const Jxta_time_diff CONNECT_TIME_INTERVAL = ((Jxta_time_diff) 15) * 1000;  /* 15 seconds */

/**
 * "Normal" time the background thread sleeps between runs. "Normal" usually
 * means that we are connected to a rdv.
 * Measured in relative microseconds.
**/
static const Jxta_time_diff CONNECT_THREAD_NAP_TIME_NORMAL = ((Jxta_time_diff) 60) * 1000;  /* 1 Minute */

/**
 * "Fast" time the background thread sleeps between runs. "Fast" usually
 * means that we are not connected to a rdv.
 * Measured in relative microseconds.
 **/
static const Jxta_time_diff CONNECT_THREAD_NAP_TIME_FAST = ((Jxta_time_diff) 4) * 1000; /* 4 Seconds */

/**
 * Minimal number of rendezvous this client will attempt to connect to.
 * If there is fewer connected rendezvous, the Rendezvous Service
 * becomes more aggressive trying to connect to rendezvous peers.
 **/
static const unsigned int MIN_NB_OF_CONNECTED_RDVS = 1;

/**
 * Minimum delay between two connection attempts to the same peer.
 **/
static const Jxta_time_diff MIN_RETRY_DELAY = ((Jxta_time_diff) 15) * 1000; /* 15 Seconds */

/**
 * Maximum delay applied between 2 connection attempts
 **/
static const Jxta_time_diff MAX_RETRY_DELAY = ((Jxta_time_diff) 45) * 1000;

/**
 *
 **/
static const Jxta_time_diff CONNECT_DELAY = ((Jxta_time_diff) 15) * 1000;
/**
 * Renewal of the leases must be done before the lease expires. The following
 * constant defines the number of microseconds to substract to the expiration
 * time in order to set the time of renewal.
 **/
static const Jxta_time_diff LEASE_RENEWAL_DELAY = ((Jxta_time_diff) 5) * 60 * 1000; /* 5 Minutes */
/**
 *  Need to delay the RDV connection in case we have a relay
 **/
static const apr_interval_time_t RDV_RELAY_DELAY_CONNECTION = ((apr_interval_time_t) 100) * 1000;   /* 100 ms */


static const Jxta_time_diff DEFAULT_LEASE_INTERVAL = ((Jxta_time_diff) 20 * 60) * 1000; /* 20 minutes */

/**
*   The number of rdv referrals we will request by default.
**/
static const int DEFAULT_REFERRALS_REQUESTED = 2;

/**
*   The number of seeds to probe in one attempt
**/
static const int DEFAULT_SEEDS_PROBED = 2;

/**
*   The maximum TTL we will allow for propagated messages.
*/
static const int DEFAULT_MAX_TTL = 2;

/**
*   The maximum time for a client to wait when searching for a rdv
*/
static const Jxta_time_diff MAX_CONNECT_CYCLE_FAST = ((Jxta_time_diff) 5) * 60 * 1000; /* 5 Minutes */

/**
 * Internal structure of a peer used by this service.
 **/
struct _jxta_peer_rdv_entry {
    Extends(_jxta_peer_entry);

    apr_uuid_t *adv_gen;
    apr_uuid_t pv_id_gen;
    Jxta_boolean pv_id_gen_set;

/**
    * Time of the last reconnection attempt.
 **/
    Jxta_time lastConnectTry;

/**
    * Interval at which we are performing reconnect attempts.
    * Starts at MIN_RETRY_DELAY and increases at MIN_RETRY_DELAY until
    * MAX_RETRY_DELAY is reached. Usually the peer expires or connection is
    * acheived before this happens.
 **/
    Jxta_time_diff connectInterval;
};

typedef struct _jxta_peer_rdv_entry _jxta_peer_rdv_entry;

/**
    Public typedef for the JXTA Rendezvous service client object.
**/
typedef Jxta_rdv_service_provider Jxta_rdv_service_client;

/**
 ** This is the Rendezvous Service Client internal object definition
 ** The first field in the structure MUST be of type _jxta_rdv_service_provider
 ** since that is the generic interface of a Rendezvous Service. 
 ** This base structure is a Jxta_object that needs to be initialized.
 **/
struct _jxta_rdv_service_client {
    Extends(_jxta_rdv_service_provider);

    apr_pool_t *pool;

    volatile Jxta_boolean running;
    Jxta_discovery_service *discovery;

    /* configuration */
    Jxta_RdvConfigAdvertisement *rdvConfig;

    /* state */
    Jxta_hashtable *rdvs;       /* connected rdv servers */
    Jxta_vector *candidates;
    _jxta_peer_rdv_entry *candidate;

    Jxta_PA *localPeerAdv;
    Jxta_object *leasing_cookie;

    Jxta_time connect_time;
    Jxta_time fast_wait_time;
};

/**
    Private typedef for the JXTA Rendezvous service client object.
**/
typedef struct _jxta_rdv_service_client _jxta_rdv_service_client;

/**
 ** Forward definition to make the compiler happy
 **/

static _jxta_rdv_service_client *jxta_rdv_service_client_construct(_jxta_rdv_service_client * myself,
                                                                   const _jxta_rdv_service_provider_methods * methods,
                                                                   apr_pool_t * pool);
static void rdv_client_delete(Jxta_object * service);
static void rdv_client_destruct(_jxta_rdv_service_client * myself);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer ** peer);
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static void process_referrals(_jxta_rdv_service_client * myself, Jxta_lease_response_msg * lease_response, Jxta_id * ignore_pid);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam, const char *target_hash, Jxta_boolean prop_to_all);
static Jxta_status disconnect_peers(Jxta_rdv_service_provider * provider);

static void *APR_THREAD_FUNC rdv_client_maintain_task(apr_thread_t * thread, void *cookie);
static Jxta_status JXTA_STDCALL lease_client_cb(Jxta_object * obj, void *arg);
static Jxta_status handle_leasing_reply(_jxta_rdv_service_client * myself, Jxta_message * msg);

static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * myself, Jxta_id * peerid, apr_uuid_t *pv_id_gen, Jxta_boolean create);

static _jxta_peer_rdv_entry *rdv_entry_new(void);
static _jxta_peer_rdv_entry *rdv_entry_new_1(Jxta_peer * peer);
static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * self);
static void rdv_entry_delete(Jxta_object * addr);
static void rdv_entry_destruct(_jxta_peer_rdv_entry * myself);

static const _jxta_rdv_service_provider_methods JXTA_RDV_SERVICE_CLIENT_METHODS = {
    "_jxta_rdv_service_provider_methods",
    init,
    start,
    stop,
    get_peer,
    get_peers,
    propagate,
    walk,
    disconnect_peers
};

static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * myself)
{
    myself = (_jxta_peer_rdv_entry *) peer_entry_construct((_jxta_peer_entry *) myself);

    if (NULL != myself) {
        myself->thisType = "_jxta_peer_rdv_entry";

        myself->adv_gen = NULL;
        myself->lastConnectTry = 0;
        myself->connectInterval = 0;
    }

    return myself;
}

static void rdv_entry_destruct(_jxta_peer_rdv_entry * myself)
{
    if (NULL != myself->adv_gen) {
        free(myself->adv_gen);
        myself->adv_gen = NULL;
    }

    peer_entry_destruct((_jxta_peer_entry *) myself);
}


static void rdv_entry_delete(Jxta_object * addr)
{
    _jxta_peer_rdv_entry *myself = (_jxta_peer_rdv_entry *) addr;

    rdv_entry_destruct(myself);

    free(myself);
}

static _jxta_peer_rdv_entry *rdv_entry_new(void)
{
    _jxta_peer_rdv_entry *myself = (_jxta_peer_rdv_entry *) calloc(1, sizeof(_jxta_peer_rdv_entry));

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, rdv_entry_delete, 0);

    return rdv_entry_construct(myself);
}

static _jxta_peer_rdv_entry *rdv_entry_new_1(Jxta_peer * peer)
{
    _jxta_peer_rdv_entry *myself = rdv_entry_new();

    if (NULL != jxta_peer_address(peer)) {
        jxta_peer_set_address((Jxta_peer *) myself, jxta_peer_address(peer));
    }

    if (NULL != jxta_peer_peerid(peer)) {
        jxta_peer_set_peerid((Jxta_peer *) myself, jxta_peer_peerid(peer));
    }

    if (NULL != jxta_peer_adv(peer)) {
        jxta_peer_set_adv((Jxta_peer *) myself, jxta_peer_adv(peer));
    }

    jxta_peer_set_expires((Jxta_peer *) myself, jxta_peer_get_expires(peer));

    return myself;
}


/**
 * Standard instantiator
 **/
Jxta_rdv_service_provider *jxta_rdv_service_client_new(apr_pool_t * pool)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_client *myself = (_jxta_rdv_service_client *) calloc(1, sizeof(_jxta_rdv_service_client));

    if (myself == NULL) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "NEW jxta_rdv_service_client [%pp].\n", myself);

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, rdv_client_delete, NULL);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service_provider *) jxta_rdv_service_client_construct(myself, &JXTA_RDV_SERVICE_CLIENT_METHODS, pool);
}


/**
 * standard constructor. This is a "final" class so it's static.
 **/
static _jxta_rdv_service_client *jxta_rdv_service_client_construct(_jxta_rdv_service_client * myself,
                                                                   const _jxta_rdv_service_provider_methods * methods,
                                                                   apr_pool_t * pool)
{
    myself = (_jxta_rdv_service_client *) jxta_rdv_service_provider_construct(PTSuper(myself), methods, pool);

    if (NULL != myself) {
        /* Set our rt type checking string */
        myself->thisType = "_jxta_rdv_service_client";

        myself->pool = PTSuper(myself)->pool;

        /** The following will be updated with initialized **/
        myself->discovery = NULL;

        /** Allocate a vector for storing the list of peers (rendezvous) **/
        myself->rdvs = jxta_hashtable_new_0(MIN_NB_OF_CONNECTED_RDVS * 2, TRUE);
        myself->candidates = jxta_vector_new(0);
        myself->candidate = NULL;
        myself->leasing_cookie = NULL;
        myself->running = FALSE;
    }

    return (_jxta_rdv_service_client *) myself;
}

/**
 * Frees an instance of the Rendezvous Service.
 * All objects and memory used by the instance must first be released/freed.
 *
 * @param obj a pointer to the instance of the Rendezvous Service to free.
 * @return error code.
 **/
static void rdv_client_delete(Jxta_object * obj)
{
    _jxta_rdv_service_client *myself = PTValid(obj, _jxta_rdv_service_client);

    /* call the hierarchy of dtors */
    rdv_client_destruct(myself);

    /* free the object itself */
    memset(myself, 0xDD, sizeof(_jxta_rdv_service_client));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FREE jxta_rdv_service_client [%pp].\n", myself);

    free(obj);
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
static void rdv_client_destruct(_jxta_rdv_service_client * myself)
{
    if (myself->running) {
        stop(PTSuper(myself));
    }

    /* Free the hashtable of peers */
    JXTA_OBJECT_RELEASE(myself->rdvs);

    JXTA_OBJECT_RELEASE(myself->candidates);

    if (NULL != myself->candidate) {
        JXTA_OBJECT_RELEASE(myself->candidate);
    }

    if (NULL != myself->leasing_cookie) {
        JXTA_OBJECT_RELEASE(myself->leasing_cookie);
    }

    /* Release the services object this instance was using */
    if (myself->discovery != NULL) {
        JXTA_OBJECT_RELEASE(myself->discovery);
        myself->discovery = NULL;
    }

    if (myself->rdvConfig != NULL)
        JXTA_OBJECT_RELEASE(myself->rdvConfig);

    /* call the base classe's dtor. */
    jxta_rdv_service_provider_destruct((_jxta_rdv_service_provider *) myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}


/**
 * Initializes an instance of the Rendezvous Service. (a module method)
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service.
 * @param group a pointer to the PeerGroup the Rendezvous Service is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * @return error code.
 **/

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);
    JString *string = NULL;
    Jxta_PA *conf_adv = NULL;
    Jxta_PG *parentgroup;
    Jxta_PG *group = jxta_service_get_peergroup_priv(PTSuper(service));
    jxta_rdv_service_provider_init(provider, service);

    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_discovery_service(group, &(myself->discovery));
    jxta_PG_get_configadv(group, &conf_adv);

    if (NULL == conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;

        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);
        if (NULL != svc) {
            myself->rdvConfig = jxta_svc_get_RdvConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == myself->rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Loading defaults\n");
        myself->rdvConfig = jxta_RdvConfigAdvertisement_new();
    }

    if (-1 == jxta_RdvConfig_get_lease_duration(myself->rdvConfig)) {
        jxta_RdvConfig_set_lease_duration(myself->rdvConfig, DEFAULT_LEASE_INTERVAL);
    }

    if (-1 == jxta_RdvConfig_get_min_connected_rendezvous(myself->rdvConfig)) {
        jxta_RdvConfig_set_min_connected_rendezvous(myself->rdvConfig, MIN_NB_OF_CONNECTED_RDVS);
    }
    if (-1 == jxta_RdvConfig_get_connect_delay(myself->rdvConfig)) {
        jxta_RdvConfig_set_connect_delay(myself->rdvConfig, CONNECT_DELAY);
    }
    if (-1 == jxta_RdvConfig_get_connect_time_interval(myself->rdvConfig)) {
        jxta_RdvConfig_set_connect_time_interval(myself->rdvConfig, CONNECT_TIME_INTERVAL);
    }
    if (-1 == jxta_RdvConfig_get_connect_cycle_fast(myself->rdvConfig)) {
        jxta_RdvConfig_set_connect_cycle_fast(myself->rdvConfig, CONNECT_THREAD_NAP_TIME_FAST);
    }
    if (-1 == jxta_RdvConfig_get_max_connect_cycle_fast(myself->rdvConfig)) {
        jxta_RdvConfig_set_max_connect_cycle_fast(myself->rdvConfig, MAX_CONNECT_CYCLE_FAST);
    }
    if (-1 == jxta_RdvConfig_get_connect_cycle_normal(myself->rdvConfig)) {
        jxta_RdvConfig_set_connect_cycle_normal(myself->rdvConfig, CONNECT_THREAD_NAP_TIME_NORMAL);
    }
    if (-1 == jxta_RdvConfig_get_lease_renewal_delay(myself->rdvConfig)) {
        jxta_RdvConfig_set_lease_renewal_delay(myself->rdvConfig, LEASE_RENEWAL_DELAY);
    }
    if (-1 == jxta_RdvConfig_get_min_retry_delay(myself->rdvConfig)) {
        jxta_RdvConfig_set_min_retry_delay(myself->rdvConfig, MIN_RETRY_DELAY);
    }
    if (-1 == jxta_RdvConfig_get_max_retry_delay(myself->rdvConfig)) {
        jxta_RdvConfig_set_max_retry_delay(myself->rdvConfig, MAX_RETRY_DELAY);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client initialized for group %s\n", provider->gid_uniq_str);

    return JXTA_SUCCESS;
}


/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically).
 *
 * @param provider The provider base class for which we are working.
 **/
static Jxta_status start(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    apr_status_t apr_res;
    Jxta_PG * pg;
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);

    /**
     ** Add the Rendezvous Service Endpoint callback
     **/
    pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);

    rdv_service_add_cb((Jxta_rdv_service *) provider->service, &myself->leasing_cookie, RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME, lease_client_cb, myself);

    jxta_rdv_service_provider_lock_priv(provider);

    res = jxta_rdv_service_provider_start(provider);

    myself->running = TRUE;

    apr_res = apr_thread_pool_schedule(provider->thread_pool, rdv_client_maintain_task,
                                       myself, apr_time_from_sec(1), myself);

    jxta_rdv_service_provider_unlock_priv(provider);

    myself->connect_time = jpr_time_now();

    myself->fast_wait_time = jxta_RdvConfig_get_connect_cycle_fast(myself->rdvConfig);

    rdv_service_generate_event(provider->service, JXTA_RDV_BECAME_EDGE, provider->local_peer_id);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client started for group %s\n", provider->gid_uniq_str);

    return JXTA_SUCCESS;
}

/**
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static Jxta_status stop(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
#ifdef UNUSED_VWF
    Jxta_PG * pg;
#endif
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);

    jxta_rdv_service_provider_lock_priv(provider);

    if (TRUE != myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV client is not started, ignore stop request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        return APR_SUCCESS;
    }

    if (myself->leasing_cookie) {
        rdv_service_remove_cb((Jxta_rdv_service *) provider->service, myself->leasing_cookie);
        JXTA_OBJECT_RELEASE(myself->leasing_cookie);
        myself->leasing_cookie = NULL;
    }
    /* We need to tell the background task that it has to die. */
    myself->running = FALSE;

    jxta_rdv_service_provider_unlock_priv(provider);
    apr_thread_pool_tasks_cancel(provider->thread_pool, myself);
    jxta_rdv_service_provider_lock_priv(provider);

    /* Release the services object this instance was using */
    if (myself->discovery != NULL) {
        JXTA_OBJECT_RELEASE(myself->discovery);
        myself->discovery = NULL;
    }

    /**
    *  FIXME 20050203 bondolo Should send a disconnect.
    **/
    /* jxta_hashtable_clear(myself->rdvs); */

    res = jxta_rdv_service_provider_stop(provider);

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");

    return res;
}


/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 **/
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist)
{
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /*
     * Allocate a new vector and stores it the peers from our own vector: we 
     * don't want to share the our vector itmyself.
     */
    *peerlist = jxta_hashtable_values_get(myself->rdvs);

    if (NULL == *peerlist) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a new jxta_vector\n");
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

/**
 * Propagates a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param ttl Maximum number of peers the propagated message can go through.
 * @return error code.
 **/
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl)
{
    Jxta_status res;
    _jxta_rdv_service_client *myself  = PTValid(provider, _jxta_rdv_service_client);
    Jxta_rdv_diffusion *header;
    myself = myself; /*VWF, this is here to avoid "unused" compile warning.*/

    JXTA_OBJECT_CHECK_VALID(msg);

    msg = jxta_message_clone(msg);

    header = jxta_rdv_diffusion_new();

    if (NULL == header) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    jxta_rdv_diffusion_set_src_peer_id(header, provider->local_peer_id);
    jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_MULTICAST);
    jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_GLOBAL);
    jxta_rdv_diffusion_set_ttl(header, ttl > DEFAULT_MAX_TTL ? DEFAULT_MAX_TTL : ttl);
    jxta_rdv_diffusion_set_dest_svc_name(header, serviceName);
    jxta_rdv_diffusion_set_dest_svc_param(header, serviceParam);

    res = jxta_message_remove_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME);

    res = jxta_rdv_service_provider_prop_handler(provider, msg, header);

    JXTA_OBJECT_RELEASE(header);

  FINAL_EXIT:
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

/**
* Walk a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
* @param rdv a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
* @param serviceName pointer to a string containing the name of the service 
* on which the listener is listening on.
* @param serviceParam pointer to a string containing the parameter associated
* to the serviceName.
* @param target_hash The hash value which is being sought.
 * @return error code.
 **/
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam, const char *target_hash, Jxta_boolean prop_to_all)
{
    Jxta_status res;
    Jxta_rdv_diffusion *header = NULL;

    _jxta_rdv_service_client *myself  = PTValid(provider, _jxta_rdv_service_client);
    JXTA_OBJECT_CHECK_VALID(msg);
    myself = myself; /*VWF, this is here to avoid "unused" compile warning.*/

    msg = jxta_message_clone(msg);

    if (NULL == msg) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    res = jxta_rdv_service_provider_get_diffusion_header(msg, &header);

    if (JXTA_ITEM_NOTFOUND == res) {
        header = jxta_rdv_diffusion_new();

        if (NULL == header) {
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        jxta_rdv_diffusion_set_src_peer_id(header, provider->local_peer_id);
        if (NULL == target_hash) {
            jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_MULTICAST);
        } else {
            jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL);
        }
        jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_GLOBAL);
        jxta_rdv_diffusion_set_ttl(header, 1);
        jxta_rdv_diffusion_set_dest_svc_name(header, serviceName);
        jxta_rdv_diffusion_set_dest_svc_param(header, serviceParam);
    } else {
        /* Validate that the addressing has not changed */
        const char *current_param = jxta_rdv_diffusion_get_dest_svc_param(header);

        if (0 != strcmp(serviceName, jxta_rdv_diffusion_get_dest_svc_name(header))) {
            res = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }

        if ((NULL != current_param) && (NULL != serviceParam)) {
            if (0 != strcmp(current_param, serviceParam)) {
                res = JXTA_INVALID_ARGUMENT;
                goto FINAL_EXIT;
            }
        } else {
            if ((NULL != current_param) || (NULL != serviceParam)) {
                res = JXTA_INVALID_ARGUMENT;
                goto FINAL_EXIT;
            }
        }
    }

    jxta_rdv_diffusion_set_target_hash(header, target_hash);

    if (JXTA_RDV_DIFFUSION_SCOPE_TERMINAL == jxta_rdv_diffusion_get_scope(header)) {
        /* We are done. */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    res = jxta_rdv_service_provider_set_diffusion_header(msg, header);

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    /* Send the message to each connected rdv peer. */
    res = jxta_rdv_service_provider_prop_to_peers(provider, msg);

  FINAL_EXIT:
    if (NULL != header) {
        JXTA_OBJECT_RELEASE(header);
    }

    if (NULL != msg) {
        JXTA_OBJECT_RELEASE(msg);
    }

    return res;
}

/**
*   Sends a lease request message to the specified peer.
 *
*   @param myself Our "this" pointer.
*   @param peer The peer from which we will request a lease.
 *   @param disconnect used to send a lease request of zero to
 *                  a connected rdv to notify the rdv that the 
 *                  client is disconnecting
 **/
static Jxta_status send_lease_request(_jxta_rdv_service_client * myself, _jxta_peer_rdv_entry * peer, Jxta_boolean disconnect)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_provider *provider = PTSuper(myself);
    Jxta_time currentTime = (Jxta_time) jpr_time_now();
    Jxta_lease_request_msg *lease_request;
    JString *lease_request_xml = NULL;
    Jxta_message *msg;
    Jxta_message_element *msgElem;
    Jxta_endpoint_address *address;
    Jxta_PA *pa;
    Jxta_PG *pg;
    
    pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);

    jxta_peer_lock((Jxta_peer *) peer);

    address = jxta_peer_address((Jxta_peer *) peer);
    if (address != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Requesting Rdv lease from %s://%s\n",
                        jxta_endpoint_address_get_protocol_name(address), 
                        jxta_endpoint_address_get_protocol_address(address));
    }

    if ((peer->lastConnectTry + peer->connectInterval) > currentTime) {
        /* Not time yet to try to connect */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Too soon for another connect.\n");
        goto FINAL_EXIT;
    }

    peer->lastConnectTry = currentTime;
    peer->connectInterval += jxta_RdvConfig_get_min_retry_delay(myself->rdvConfig);
    if (peer->connectInterval > jxta_RdvConfig_get_max_retry_delay(myself->rdvConfig)) {
        peer->connectInterval = jxta_RdvConfig_get_max_retry_delay(myself->rdvConfig);
    }

    /*
     * Create a message with a lease request.
     */
    lease_request = jxta_lease_request_msg_new();

    if (NULL == lease_request) {
        goto FINAL_EXIT;
    }

    jxta_lease_request_msg_set_client_id(lease_request, provider->local_peer_id);

    if (disconnect) {
        jxta_lease_request_msg_set_requested_lease(lease_request, 0);
        jxta_lease_request_msg_set_disconnect(lease_request, TRUE);
        jxta_lease_request_msg_set_client_adv(lease_request, provider->local_pa);
        jxta_lease_request_msg_set_client_adv_exp(lease_request, jxta_RdvConfig_get_lease_duration(myself->rdvConfig) * 2);
    }
    else {
        jxta_lease_request_msg_set_requested_lease(lease_request, jxta_RdvConfig_get_lease_duration(myself->rdvConfig));
    }

    if (peer->adv_gen) {
        jxta_lease_request_msg_set_server_adv_gen(lease_request, peer->adv_gen);
    }
    jxta_lease_request_msg_set_referral_advs(lease_request, DEFAULT_REFERRALS_REQUESTED);

    pa = jxta_peer_adv((Jxta_peer *) peer);
    if (NULL == pa) {
        /* We have only the address. We will include our advertisement as a reverse route. */
        jxta_lease_request_msg_set_client_adv(lease_request, provider->local_pa);
        jxta_lease_request_msg_set_client_adv_exp(lease_request, jxta_RdvConfig_get_lease_duration(myself->rdvConfig) * 2);
    } else {
        /* publish PA */
        if (myself->discovery == NULL) {
            jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv(PTSuper(provider->service)), &(myself->discovery));
        }

        if (myself->discovery != NULL) {
            discovery_service_publish(myself->discovery, (Jxta_advertisement *) pa, DISC_PEER,
                                      (Jxta_expiration_time) (jxta_peer_get_expires((Jxta_peer *) peer) - jpr_time_now()),
                                      LOCAL_ONLY_EXPIRATION);
        }
    }

    /* Are we an auto-rdv candidate? */
    if (-1 != jxta_rdv_service_auto_interval((Jxta_rdv_service *) provider->service)) {
        Jxta_rdv_lease_options *rdv_options = jxta_rdv_lease_options_new();

        /* XXX 20061024 These could be better tuned. */
        jxta_rdv_lease_options_set_suitability(rdv_options, 50);
        jxta_rdv_lease_options_set_willingness(rdv_options, 25);
        jxta_lease_request_msg_add_option(lease_request, (Jxta_advertisement *) rdv_options);
        JXTA_OBJECT_RELEASE(rdv_options);
    }

    jxta_lease_request_msg_get_xml(lease_request, &lease_request_xml);
    JXTA_OBJECT_RELEASE(lease_request);

    msgElem = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_LEASE_REQUEST_ELEMENT_NAME,
                                         "text/xml", jstring_get_string(lease_request_xml), jstring_length(lease_request_xml),
                                         NULL);
    JXTA_OBJECT_RELEASE(lease_request_xml);

    if (NULL == msgElem) {
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    if (NULL == msg) {
        JXTA_OBJECT_RELEASE(msgElem);
        goto FINAL_EXIT;
    }

    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);

    /*
     * Send the message
     *
     */
    if (pa) {
        Jxta_id *pid = jxta_PA_get_PID(pa);
        res = jxta_PG_sync_send(pg, msg, pid, RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME, NULL);
        JXTA_OBJECT_RELEASE(pid);
    } else if (address != NULL) {
        Jxta_endpoint_address *dest = NULL;
        res = jxta_PG_get_recipient_addr(pg, jxta_endpoint_address_get_protocol_name(address),
                                         jxta_endpoint_address_get_protocol_address(address), 
                                         RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME, &dest);
        if (JXTA_SUCCESS == res) {
            res = jxta_endpoint_service_send_ex(provider->service->endpoint, msg, dest, JXTA_TRUE, NULL);
            JXTA_OBJECT_RELEASE(dest);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot send the lease request "
                        "because both the pa and address are NULL\n");
        res = JXTA_FAILED;
    }

    /*
     * Set the destination address of the message.
     */
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send connect message\n");
        jxta_peer_set_expires((Jxta_peer *) peer, jpr_time_now());
    }

    JXTA_OBJECT_RELEASE(msg);

  FINAL_EXIT:
    jxta_peer_unlock((Jxta_peer *) peer);

    return res;
}

/**
* @return TRUE if there is remaining time left in the lease otherwise FALSE.
**/
static Jxta_boolean check_peer_lease(_jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    Jxta_time_diff remaining;

    jxta_peer_lock((Jxta_peer *) peer);

    expires = jxta_peer_get_expires((Jxta_peer *) peer);

    jxta_peer_unlock((Jxta_peer *) peer);

    remaining = (Jxta_time_diff) expires - currentTime;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Check_peer_lease - Time remaining: " JPR_DIFF_TIME_FMT "ms.\n", remaining);

    /* any lease remaining ? */
    return (remaining > 0);
}

static Jxta_boolean check_peer_connect(_jxta_rdv_service_client * myself, _jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    Jxta_time lastConnectTry;
    Jxta_time_diff connectInterval;

    jxta_peer_lock((Jxta_peer *) peer);

    expires = jxta_peer_get_expires((Jxta_peer *) peer);
    lastConnectTry = peer->lastConnectTry;
    connectInterval = peer->connectInterval;

    jxta_peer_unlock((Jxta_peer *) peer);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer->address = %s://%s\n"
                    "\tpeer->expires= " JPR_DIFF_TIME_FMT " ms"
                    "\tpeer->lastConnectTry= " JPR_DIFF_TIME_FMT " ms"
                    "\tpeer->connectInterval= " JPR_DIFF_TIME_FMT " ms\n",
                    jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                    (0 == expires) ? 0 : (expires - currentTime),
                    (0 == lastConnectTry) ? 0 : (currentTime - lastConnectTry), connectInterval);

    if (((Jxta_time_diff) (expires - currentTime)) < jxta_RdvConfig_get_lease_renewal_delay(myself->rdvConfig)) {
        Jxta_rdv_service_provider *provider = PTValid(PTSuper(myself), _jxta_rdv_service_provider);
        Jxta_boolean connect = TRUE;
        Jxta_vector * new_candidate_list = NULL;
        Jxta_boolean shuffle;
        _jxta_peer_rdv_entry * connect_peer = NULL;

        connect = rdv_service_call_candidate_list_cb((Jxta_rdv_service *) provider->service, myself->candidates, &new_candidate_list, &shuffle);
        if (NULL != new_candidate_list) {
            JXTA_OBJECT_RELEASE(myself->candidates);
            myself->candidates = new_candidate_list;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "new candidate list for checking connection\n");
            if (jxta_vector_size(new_candidate_list) > 0) {
                JString * peerid_j = NULL;
                JString * new_peerid_j = NULL;

                jxta_vector_get_object_at(new_candidate_list, JXTA_OBJECT_PPTR(&connect_peer), 0);

                jxta_id_to_jstring(jxta_peer_peerid((Jxta_peer *) peer), &peerid_j);
                jxta_id_to_jstring(jxta_peer_peerid((Jxta_peer *) connect_peer), &new_peerid_j);
                if (0 != strcmp(jstring_get_string(peerid_j), jstring_get_string(new_peerid_j))) {
                    JString *uniq;
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Removing %s and connecting to %s\n", jstring_get_string(peerid_j), jstring_get_string(new_peerid_j));

                    jxta_id_get_uniqueportion(jxta_peer_peerid((Jxta_peer *) peer), &uniq);
                    jxta_hashtable_del(myself->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, NULL);
                    JXTA_OBJECT_RELEASE(uniq);
                } else {
                    JXTA_OBJECT_RELEASE(connect_peer);
                    connect_peer = JXTA_OBJECT_SHARE(peer);
                }
                JXTA_OBJECT_RELEASE(peerid_j);
                JXTA_OBJECT_RELEASE(new_peerid_j);
            }
        } else {
            connect_peer = JXTA_OBJECT_SHARE(peer);
        }
        if (connect && connect_peer) {

            /* Time to try a connection */
            send_lease_request(myself, connect_peer, FALSE);

        } else if (!connect) {
            /* start the referral and seeding again */
            myself->connect_time = jxta_RdvConfig_get_connect_time_interval(myself->rdvConfig) + jpr_time_now();
        }
        if (connect_peer)
            JXTA_OBJECT_RELEASE(connect_peer);
    }
    return TRUE;
}

/**
 ** Process a response of a lease request.
 ** The message contains the following message elements:
 **
 **    - "jxta:LeaseResponseMessage": contains the response to our lease request.
 **/
static Jxta_status handle_leasing_reply(_jxta_rdv_service_client * myself, Jxta_message * msg)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_rdv_service_provider *provider = PTValid(PTSuper(myself), _jxta_rdv_service_provider);
    Jxta_message_element *el = NULL;
    Jxta_lease_response_msg *lease_response = NULL;
    Jxta_id *server_peerid = NULL;
    JString *server_peerid_str = NULL;
    Jxta_time_diff lease = -1L;
    Jxta_lease_adv_info *server_info = NULL;
    Jxta_PA *server_adv = NULL;
    apr_uuid_t *server_adv_gen = NULL;
    Jxta_time_diff server_adv_exp = -1L;
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_Rendezvous_event_type lease_event;
    apr_uuid_t *pv_id_gen = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Processing lease response message [%pp]\n", msg);

    status = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_LEASE_RESPONSE_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS != status) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No Lease Response in msg [%pp]\n", msg);
        /* no reason to flag as error - could be received in multicast as a probe */
        status = JXTA_SUCCESS;
        goto FINAL_EXIT;
    } else {
        /*
         * This element contains the peer id of the rendezvous peer
         * that has granted the lease.
         */
        Jxta_bytevector *value = jxta_message_element_get_value(el);
        JString *string;

        JXTA_OBJECT_RELEASE(el);

        /* Build the proper endpoint address */
        string = jstring_new_3(value);
        JXTA_OBJECT_RELEASE(value);

        lease_response = jxta_lease_response_msg_new();

        if (NULL == lease_response) {
            JXTA_OBJECT_RELEASE(string);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure creating Lease Request Message [%pp]\n", msg);
            status = JXTA_FAILED;
            goto FINAL_EXIT;
        }

        status = jxta_lease_response_msg_parse_charbuffer(lease_response, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);

        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure parsing Lease Response Message [%pp]\n", msg);
            goto FINAL_EXIT;
        }
    }

    server_peerid = jxta_lease_response_msg_get_server_id(lease_response);

    if (NULL == server_peerid) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No server id in response [%pp]\n", lease_response);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    pv_id_gen = jxta_lease_response_msg_get_pv_id_gen(lease_response);

    jxta_id_to_jstring(server_peerid, &server_peerid_str);

    lease = jxta_lease_response_msg_get_offered_lease(lease_response);

    peer = get_peer_entry(myself, server_peerid, pv_id_gen, (lease > 0));

    if (NULL == peer) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Ignoring lease offer from [%s] with lease of %d\n",
                        jstring_get_string(server_peerid_str), lease);
        status = JXTA_FAILED;

        process_referrals(myself, lease_response, server_peerid);

        goto FINAL_EXIT;
    }

    server_info = jxta_lease_response_msg_get_server_adv_info(lease_response);

    if (NULL == server_info) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No server adv info in response [%pp]\n", lease_response);
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    server_adv = jxta_lease_adv_info_get_adv(server_info);
    server_adv_gen = jxta_lease_adv_info_get_adv_gen(server_info);
    server_adv_exp = jxta_lease_adv_info_get_adv_exp(server_info);

    jxta_peer_lock((Jxta_peer *) peer);

    if (lease > 0) {

        Jxta_time currentTime = jpr_time_now();

        if ((NULL != server_adv) && (NULL != server_adv_gen)) {
            jxta_peer_set_adv((Jxta_peer *) peer, server_adv);

            if (NULL != peer->adv_gen) {
                free(peer->adv_gen);
                peer->adv_gen = NULL;
            }
            peer->adv_gen = server_adv_gen;
            server_adv_gen = NULL;
        } else {
            /* publish the *current* peer advertisement with lifetime of the lease length */
            if (NULL != server_adv) {
                JXTA_OBJECT_RELEASE(server_adv);
            }
            jxta_peer_get_adv((Jxta_peer *) peer, &server_adv);
            server_adv_exp = lease;
        }

        if (NULL != pv_id_gen && TRUE == peer->pv_id_gen_set && UUID_GREATER_THAN == jxta_id_uuid_time_stamp_only_compare(pv_id_gen, &peer->pv_id_gen)) {
            lease_event = JXTA_RDV_CONNECTED;
            memmove(&peer->pv_id_gen,pv_id_gen, sizeof(apr_uuid_t));
            peer->pv_id_gen_set = TRUE;
        } else {
            lease_event =
                jxta_peer_get_expires((Jxta_peer *) peer) != JPR_ABSOLUTE_TIME_MAX ? JXTA_RDV_RECONNECTED : JXTA_RDV_CONNECTED;
        }

        jxta_peer_set_expires((Jxta_peer *) peer, currentTime + lease);

        peer->connectInterval = 0;
        peer->lastConnectTry = currentTime;
    } else {
        peer->connectInterval = jxta_RdvConfig_get_min_retry_delay(myself->rdvConfig);
        jxta_peer_set_expires((Jxta_peer *) peer, 0);
        lease_event = JXTA_RDV_DISCONNECTED;
    }

    jxta_peer_unlock((Jxta_peer *) peer);

    /* publish PA */
    if ((lease > 0) && (NULL != server_adv)) {
        if (myself->discovery == NULL) {
            jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv(PTSuper(provider->service)), &(myself->discovery));
        }

        if (myself->discovery != NULL) {
            discovery_service_publish(myself->discovery, (Jxta_advertisement *) server_adv, DISC_PEER,
                                      (Jxta_expiration_time) server_adv_exp, LOCAL_ONLY_EXPIRATION);
        }
    }

    process_referrals(myself, lease_response, NULL);

    /*
     *  notify the RDV listeners about the lease activity.
     */
    rdv_service_generate_event((_jxta_rdv_service *)
                               jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) myself),
                               lease_event, server_peerid);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous connection with [%s] for " JPR_DIFF_TIME_FMT " ms\n",
                    jstring_get_string(server_peerid_str), lease);

    status = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != lease_response) {
        JXTA_OBJECT_RELEASE(lease_response);
    }

    if (NULL != server_peerid) {
        JXTA_OBJECT_RELEASE(server_peerid);
    }

    if (NULL != server_peerid_str) {
        JXTA_OBJECT_RELEASE(server_peerid_str);
    }

    if (NULL != server_info) {
        JXTA_OBJECT_RELEASE(server_info);
    }

    if (NULL != server_adv) {
        JXTA_OBJECT_RELEASE(server_adv);
    }

    if (NULL != server_adv_gen) {
        free(server_adv_gen);
    }
    if (pv_id_gen) {
        free(pv_id_gen);
    }

    if (NULL != peer) {
        JXTA_OBJECT_RELEASE(peer);
    }

    return status;
}

static void process_referrals(_jxta_rdv_service_client * myself, Jxta_lease_response_msg * lease_response, Jxta_id * ignore_pid)
{
    Jxta_vector * referrals = NULL;
    unsigned int all_referrals;
    unsigned int each_referral;
    Jxta_status status;

    /* Handle referrals! */
    referrals = jxta_lease_response_msg_get_referral_advs(lease_response);
    if (NULL == referrals) {
        goto FINAL_EXIT;
    }
    all_referrals = jxta_vector_size(referrals);
    for (each_referral = 0; each_referral < all_referrals; each_referral++) {
        Jxta_lease_adv_info *referral = NULL;
        unsigned int each_candidate;

        status = jxta_vector_get_object_at(referrals, JXTA_OBJECT_PPTR(&referral), each_referral);
        if (JXTA_SUCCESS != status) {
            continue;
        }

        if (jxta_lease_adv_info_get_adv_exp(referral) > 0) {
            JString *jPeerid;
            Jxta_id *pid;
            Jxta_peer *referral_candidate = NULL;
            Jxta_PA *referral_adv = jxta_lease_adv_info_get_adv(referral);
            unsigned int all_candidates=0;
            Jxta_boolean added=FALSE;

            pid  = jxta_PA_get_PID(referral_adv);
            if (NULL != ignore_pid && jxta_id_equals(pid, ignore_pid) && all_referrals > 1) {
                JXTA_OBJECT_RELEASE(pid);
                JXTA_OBJECT_RELEASE(referral);
                JXTA_OBJECT_RELEASE(referral_adv);
                continue;
            }

            if (myself->candidates) {
                all_candidates = jxta_vector_size(myself->candidates);
            }

            for (each_candidate=0; each_candidate < all_candidates; each_candidate++) {
                Jxta_peer *candidate=NULL;

                status = jxta_vector_get_object_at(myself->candidates, JXTA_OBJECT_PPTR(&candidate), each_candidate);
                if (JXTA_SUCCESS != status) {
                    continue;
                }
                if (jxta_id_equals(jxta_peer_peerid(candidate), pid)) {
                    /* will be released later */
                    referral_candidate = candidate;
                    break;
                }
                JXTA_OBJECT_RELEASE(candidate);
            }
            if (NULL == referral_candidate) {
                referral_candidate = (Jxta_peer *) rdv_entry_new();
                added = TRUE;
            }
            jxta_peer_set_adv(referral_candidate, referral_adv);
            JXTA_OBJECT_RELEASE(referral_adv);
            jxta_peer_set_peerid(referral_candidate, pid);
            jxta_peer_set_expires(referral_candidate, jpr_time_now() + jxta_lease_adv_info_get_adv_exp(referral));
            jxta_id_to_jstring(pid, &jPeerid);
           
            if (added == TRUE)
            {
                jxta_vector_add_object_first(myself->candidates, (Jxta_object *) referral_candidate);
            }
            
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%s referral %s \n", added == TRUE ? "Added":"Updated", jstring_get_string(jPeerid));

            JXTA_OBJECT_RELEASE(jPeerid);
            JXTA_OBJECT_RELEASE(pid);
            JXTA_OBJECT_RELEASE(referral_candidate);
        }
        JXTA_OBJECT_RELEASE(referral);
    }
FINAL_EXIT:
    if (referrals)
        JXTA_OBJECT_RELEASE(referrals);
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Service
 ** leasing protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - grant of lease from the rendezvous peer.
 **/
static Jxta_status JXTA_STDCALL lease_client_cb(Jxta_object * obj, void *arg)
{
    _jxta_rdv_service_client *myself = PTValid(arg, _jxta_rdv_service_client);
    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Service Client received a leasing message [%pp]\n", msg);

    return handle_leasing_reply(myself, msg);
}

/**
 *  The entry point of the thread that maintains the rendezvous connection on a
 *  periodic basis.
 *
 *  @param thread The thread we are running on.
 *  @param arg Our object.
 **/
static void *APR_THREAD_FUNC rdv_client_maintain_task(apr_thread_t * thread, void *cookie)
{
    Jxta_status res;
    _jxta_rdv_service_client *myself = PTValid(cookie, _jxta_rdv_service_client);
    Jxta_rdv_service_provider *provider = PTValid(PTSuper(myself), _jxta_rdv_service_provider);

    Jxta_vector *rdvs;
    Jxta_vector *failed_rdvs = jxta_vector_new(0);
    unsigned int nbOfConnectedRdvs = 0;
    unsigned int each_rdv;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Maintain Task RUN [%pp].\n", myself);

    if (!myself->running) {
        goto FINAL_EXIT;
    }

    jxta_rdv_service_provider_lock_priv(provider);

    rdvs = jxta_hashtable_values_get(myself->rdvs);

    jxta_rdv_service_provider_unlock_priv(provider);

    for (each_rdv = 0; each_rdv < jxta_vector_size(rdvs); each_rdv++) {
        _jxta_peer_rdv_entry *peer;

        res = jxta_vector_get_object_at(rdvs, JXTA_OBJECT_PPTR(&peer), each_rdv);
        if (res != JXTA_SUCCESS) {
            /* We should print a LOG here. Just continue for the time being */
            continue;
        }

        if (check_peer_lease(peer)) {
            ++nbOfConnectedRdvs;
        } else {
            /** No lease left. Get rid of this rdv **/
            Jxta_id *peerid;
            JString *uniq;
            jxta_peer_get_peerid((Jxta_peer *) peer, &peerid);
            jxta_vector_add_object_last(failed_rdvs, (Jxta_object *) peerid);
            jxta_id_get_uniqueportion(peerid, &uniq);
            JXTA_OBJECT_RELEASE(peerid);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removing expired RDV : %s\n", jstring_get_string(uniq));

            res = jxta_hashtable_del(myself->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, NULL);

            JXTA_OBJECT_RELEASE(uniq);
        }

        JXTA_OBJECT_RELEASE(peer);
    }

    JXTA_OBJECT_RELEASE(rdvs);

    jxta_rdv_service_provider_lock_priv(provider);
    /*
     *   The second pass, wherein we try to renew any existing leases as needed via check_peer_connect.
     */
    rdvs = jxta_hashtable_values_get(myself->rdvs);

    jxta_rdv_service_provider_unlock_priv(provider);
    
    for (each_rdv = 0; each_rdv < jxta_vector_size(rdvs); each_rdv++) {
        _jxta_peer_rdv_entry *peer;

        res = jxta_vector_get_object_at(rdvs, JXTA_OBJECT_PPTR(&peer), each_rdv);
        if (res != JXTA_SUCCESS) {
            /* We should print a LOG here. Just continue for the time being */
            continue;
        }

        check_peer_connect(myself, peer);

        JXTA_OBJECT_RELEASE(peer);
    }

    JXTA_OBJECT_RELEASE(rdvs);

    /*
     *   The third leg, wherein we try to locate new rdvs as needed.
     */
    if (nbOfConnectedRdvs < (unsigned int) jxta_RdvConfig_get_min_connected_rendezvous(myself->rdvConfig)) {
        Jxta_boolean connect = FALSE;
        Jxta_vector * new_candidate_list = NULL;

        if (0 == jxta_vector_size(myself->candidates)) {
            /* Get additional candidates */
            Jxta_vector *active_seeds = NULL;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Get additional candidates by obtaining seeds.\n");

            /* Attempt to locate other unknown rendezvous nodes
             * The first cycle through the list of candidates will consider seeds only, but subsequent
             * cycles will make use of any discovered rendezvous
             */
            rdv_service_send_seed_request((Jxta_rdv_service *) (PTSuper(myself))->service);

            /* get the current list of known rendezvous nodes */
            res = rdv_service_get_seeds((Jxta_rdv_service *) provider->service, &active_seeds, provider->service->shuffle);

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not get any seeds.\n");
            } else {
                /* Grow the seeds to candidates */
                unsigned int all_seeds = jxta_vector_size(active_seeds);
                unsigned int each_seed;

                for (each_seed = 0; each_seed < all_seeds; each_seed++) {
                    Jxta_peer *seed;
                    Jxta_peer *seed_candidate;

                    res = jxta_vector_remove_object_at(active_seeds, JXTA_OBJECT_PPTR(&seed), 0);

                    if (JXTA_SUCCESS != res) {
                        /* Not an error. We probably just ran out of peers. */
                        continue;
                    }

                    JXTA_OBJECT_CHECK_VALID(seed);

                    seed_candidate = (Jxta_peer *) rdv_entry_new_1(seed);

                    JXTA_OBJECT_RELEASE(seed);

                    /* the candidate expires in 10 minutes. */
                    if (0 == jxta_peer_get_expires(seed_candidate)) {
                        jxta_peer_set_expires(seed_candidate, (jpr_time_now() + 10 * 60 * JPR_INTERVAL_ONE_SECOND));
                    }

                    jxta_vector_add_object_last(myself->candidates, (Jxta_object *) seed_candidate);
                    JXTA_OBJECT_RELEASE(seed_candidate);
                }

                JXTA_OBJECT_RELEASE(active_seeds);
            }
        }

        /* Final lap -- try to connect to a candidate rdv */
        if (NULL != myself->candidate) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Try connecting to candidate.\n");

            if ((myself->candidate->lastConnectTry + myself->candidate->connectInterval) > jpr_time_now()) {
                /* Time for the next candidate. */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Candidate timed out - try next\n");
                JXTA_OBJECT_RELEASE(myself->candidate);
                myself->candidate = NULL;
            }
        }

        if (myself->connect_time < jpr_time_now()) {
            new_candidate_list = NULL;
            connect = rdv_service_call_candidate_list_cb((Jxta_rdv_service *) provider->service, myself->candidates, &new_candidate_list, &provider->service->shuffle);
            if (!connect) {
                myself->connect_time = jxta_RdvConfig_get_connect_time_interval(myself->rdvConfig) + jpr_time_now();
            }
        }

        if (NULL != new_candidate_list) {
            if (NULL != myself->candidates) {
                JXTA_OBJECT_RELEASE(myself->candidates);
            }
            myself->candidates = new_candidate_list;
        }
        while (connect && (NULL == myself->candidate) && (jxta_vector_size(myself->candidates) > 0)) {
            /* Try connecting to a new candidate */
            res = jxta_vector_remove_object_at(myself->candidates, JXTA_OBJECT_PPTR(&myself->candidate), 0);

            if (res != JXTA_SUCCESS) {
                /* Failed. Try another! */
                continue;
            }
            if (jxta_peer_get_expires((Jxta_peer *) myself->candidate) < jpr_time_now()) {
                /* Expired candidate. Get another. */
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Candidate expired - try next\n");
                JXTA_OBJECT_RELEASE(myself->candidate);
                myself->candidate = NULL;
                continue;
            }

        }
        if (NULL != myself->candidate) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send lease request to candidate\n");
            if(send_lease_request(myself, myself->candidate, FALSE) != JXTA_SUCCESS)
            {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send lease request to candidate\n");
            }
        }
    } 

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Rendezvous Client Periodic RUN DONE.\n");

    jxta_rdv_service_provider_lock_priv(provider);

    if (myself->running) {
        /* Reschedule another check. */
        apr_status_t apr_res;
        apr_interval_time_t wait_time;

        if (nbOfConnectedRdvs < (unsigned int) jxta_RdvConfig_get_min_connected_rendezvous(myself->rdvConfig)) {
            if (myself->fast_wait_time >= jxta_RdvConfig_get_max_connect_cycle_fast(myself->rdvConfig)) 
            {
                wait_time = jxta_RdvConfig_get_max_connect_cycle_fast(myself->rdvConfig);
                myself->fast_wait_time = jxta_RdvConfig_get_max_connect_cycle_fast(myself->rdvConfig);
            } else {
                wait_time = myself->fast_wait_time;
                myself->fast_wait_time += myself->fast_wait_time;
            }
        } else {
            wait_time = jxta_RdvConfig_get_connect_cycle_normal(myself->rdvConfig);
            /* now that we are connected, reset the fast wait time so we can attempt
               a quick connect again when needed */
            myself->fast_wait_time = jxta_RdvConfig_get_connect_cycle_fast(myself->rdvConfig);
        }

        wait_time *= 1000;  /* msec to microseconds */

        apr_res = apr_thread_pool_schedule(provider->thread_pool, rdv_client_maintain_task, myself, wait_time, myself);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not reschedule maintain task.\n", myself);
        }
    }

    jxta_rdv_service_provider_unlock_priv(provider);

  FINAL_EXIT:

    if (0 != jxta_vector_size(failed_rdvs)) {
        /* Now outside of sync, send notification of failed Rdvs. */
        unsigned int all_failed = jxta_vector_size(failed_rdvs);
        unsigned int each_failed;

        for (each_failed = 0; each_failed < all_failed; each_failed++) {
            Jxta_id *failed;

            res = jxta_vector_remove_object_at(failed_rdvs, JXTA_OBJECT_PPTR(&failed), 0);

            if (JXTA_SUCCESS == res) {
                rdv_service_generate_event(provider->service, JXTA_RDV_FAILED, failed);
                JXTA_OBJECT_RELEASE(failed);
            }
        }
    }
    JXTA_OBJECT_RELEASE(failed_rdvs);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Maintain Task Done [%pp].\n", myself);

    return NULL;
}

/**
 * Return the peer record for the specified peer id.
 * 
 * @param provider The instance of the Rendezvous Service Provider
 * @param peerid The peerID of the peer sought.
 * @param peer The result.
 * @return JXTA_SUCCESS if peer is returned otherwise JXTA_ITEM_NOTFOUND.
 **/
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer ** peer)
{
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);

    /* Test arguments first */
    if ((NULL == peerid) || (NULL == peer)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    *peer = (Jxta_peer *) get_peer_entry(myself, peerid, NULL, FALSE);

    if (NULL == *peer) {
        return JXTA_ITEM_NOTFOUND;
    }

    return JXTA_SUCCESS;
}

/**
 * Returns the _jxta_peer_rdv_entry associated to a peer or optionally create one
 *
 *  @param myself The rendezvous server
 *  @param addr The address of the desired peer.
 *  @param create If TRUE then create a new entry if there is no existing entry 
 **/
static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * myself, Jxta_id * peerid, apr_uuid_t *pv_id_gen, Jxta_boolean create)
{
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_status res = 0;
    Jxta_boolean found = FALSE;
    JString *uniq;
    size_t currentPeers;

    jxta_id_get_uniqueportion(peerid, &uniq);

    jxta_rdv_service_provider_lock_priv(PTSuper(myself));

    res = jxta_hashtable_get(myself->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, JXTA_OBJECT_PPTR(&peer));

    found = res == JXTA_SUCCESS;

    jxta_hashtable_stats(myself->rdvs, NULL, &currentPeers, NULL, NULL, NULL);

    if (!found && create && ((unsigned int) jxta_RdvConfig_get_min_connected_rendezvous(myself->rdvConfig) > currentPeers)) {
        /* We need to create a new _jxta_peer_rdv_entry */
        peer = rdv_entry_new();
        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot create a _jxta_peer_rdv_entry\n");
        } else {
            Jxta_endpoint_address *clientAddr = jxta_endpoint_address_new_2("jxta", jstring_get_string(uniq), NULL, NULL);

            jxta_peer_set_address((Jxta_peer *) peer, clientAddr);
            JXTA_OBJECT_RELEASE(clientAddr);
            jxta_peer_set_peerid((Jxta_peer *) peer, peerid);
            jxta_peer_set_expires((Jxta_peer *) peer, JPR_ABSOLUTE_TIME_MAX);
            if (NULL != pv_id_gen) {
                memmove(&peer->pv_id_gen, pv_id_gen, sizeof(apr_uuid_t));
                peer->pv_id_gen_set = TRUE;
            }
            jxta_hashtable_put(myself->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, (Jxta_object *) peer);
        }
    }

    jxta_rdv_service_provider_unlock_priv(PTSuper(myself));

    JXTA_OBJECT_RELEASE(uniq);

    return peer;
}

static Jxta_status disconnect_peers(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_client *myself = PTValid(provider, _jxta_rdv_service_client);
    Jxta_status res = JXTA_SUCCESS;
    unsigned int each_rdv;
    Jxta_vector *rdvs = jxta_hashtable_values_get(myself->rdvs);

    for (each_rdv = 0; each_rdv < jxta_vector_size(rdvs); each_rdv++) {
        _jxta_peer_rdv_entry *peer;

        res = jxta_vector_get_object_at(rdvs, JXTA_OBJECT_PPTR(&peer), each_rdv);
        if (JXTA_SUCCESS != res) {
            continue;
        }

        if (check_peer_lease(peer)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "sending a lease request with time of zero to disconnect\n");
            send_lease_request(myself, peer, TRUE);
        }
        JXTA_OBJECT_RELEASE(peer);
    }

    JXTA_OBJECT_RELEASE(rdvs);

    return res;
}
        
/* vim: set ts=4 sw=4 tw=130 et: */
