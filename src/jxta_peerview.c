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

static const char *__log_cat = "PV";

#include <stddef.h>
#include <assert.h>
#include <math.h>

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <apr_uuid.h>

#include "jxta_apr.h"

#include "jxta_peerview_priv.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_hashtable.h"
#include "jxta_rdv_service.h"
#include "jxta_svc.h"
#include "jxta_version.h"
#include "jxta_peerview_address_request_msg.h"
#include "jxta_peerview_address_assign_msg.h"
#include "jxta_peerview_ping_msg.h"
#include "jxta_peerview_pong_msg.h"
#include "jxta_adv_request_msg.h"
#include "jxta_adv_response_msg.h"
#include "jxta_id_uuid_priv.h"
#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_lease_options.h"

/**
*   Names of the Peerview Events
**/
const char *JXTA_PEERVIEW_EVENT_NAMES[] = { "unknown", "ADD", "REMOVE", "DEMOTE" };

const char JXTA_PEERVIEW_NAME[] = "PeerView";
const char JXTA_PEERVIEW_NS_NAME[] = "jxta";

#define PA_REFRESH_INTERVAL (5 * 60 * JPR_INTERVAL_ONE_SECOND)

/**
*   Interval at which we re-evaluate all of the peers within the peerview.
**/
static const apr_interval_time_t JXTA_PEERVIEW_MAINTAIN_INTERVAL = 20 * APR_USEC_PER_SEC;

/**
*   Number of intervals before becoming a rendezvous
**/
static const unsigned int DEFAULT_LONELINESS_FACTOR = 2;

/**
*   Interval at which we (may) attempt to nominate additional peers to join the peerview.
**/
static const apr_interval_time_t JXTA_PEERVIEW_ADD_INTERVAL = 15 * APR_USEC_PER_SEC;

#define JXTA_PEERVIEW_PVE_EXPIRATION (3 * 60 * JPR_INTERVAL_ONE_SECOND)
#define JXTA_PEERVIEW_PVE_PING_DUE (JXTA_PEERVIEW_PVE_EXPIRATION / 4)
#define JXTA_PEERVIEW_PVE_PONG_DUE (JXTA_PEERVIEW_PVE_EXPIRATION / 8)

#define JXTA_PEERVIEW_VOTING_EXPIRATION (5 * JPR_INTERVAL_ONE_SECOND)
#define JXTA_PEERVIEW_VOTING_WAIT (3 * JXTA_PEERVIEW_VOTING_EXPIRATION)

static const unsigned int DEFAULT_CLUSTERS_COUNT = 2;

static const unsigned int DEFAULT_CLUSTER_MEMBERS = 4;

static const unsigned int DEFAULT_REPLICATION_FACTOR = 2;

/**
*   The number of locate probes we will send before giving up
*   and starting a new peerview instance.
*/
static const unsigned int DEFAULT_MAX_LOCATE_PROBES = 2;

/**
*   The maximum number of address requests we will send per
*   iteration.
*/
static const unsigned int DEFAULT_MAX_ADDRESS_REQUESTS = 2;

/**
*   The maximum number of ping probes we will send per
*   iteration.
*/
static const unsigned int DEFAULT_MAX_PING_PROBES = 25;

/**
* Number of peers we will invite into the peerview with each add run.
**/
#define MAXIMUM_CLIENT_INVITATIONS DEFAULT_CLUSTERS_COUNT


struct Cluster_entry {
    BIGNUM *min;
    BIGNUM *mid;
    BIGNUM *max;

    Jxta_vector *members;
    Jxta_vector *histogram;
};

typedef struct Cluster_entry Cluster_entry;

/**
 * Internal structure of a peer used by this service.
 **/
struct _jxta_peer_peerview_entry {
    Extends(_jxta_peer_entry);

    BIGNUM *target_hash;

    BIGNUM *target_hash_radius;

    int cluster;

    BIGNUM *radius_range_1_start;
    BIGNUM *radius_range_1_end;
    BIGNUM *radius_range_2_start;
    BIGNUM *radius_range_2_end;

    Jxta_time created_at;

    Jxta_boolean pv_id_gen_set;
    apr_uuid_t pv_id_gen;

    Jxta_boolean adv_gen_set;
    apr_uuid_t adv_gen;
    Jxta_time_diff adv_exp;
    Jxta_boolean is_rendezvous;
    Jxta_boolean is_demoting;
};

typedef struct _jxta_peer_peerview_entry Peerview_entry;

typedef enum peerview_states {
    PV_STOPPED = 0,
    PV_PASSIVE,
    PV_LOCATING,
    PV_ADDRESSING,
    PV_ANNOUNCING,
    PV_MAINTENANCE
} Peerview_state;

static char *state_description[] = {"PV_STOPPED",
    "PV_PASSIVE",
    "PV_LOCATING",
    "PV_ADDRESSING",
    "PV_ANNOUNCING",
    "PV_MAINTENANCE" };
/**
*   Our fields.
**/
struct _jxta_peerview {
    Extends(Jxta_object);

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    volatile Jxta_boolean running;

    Jxta_PG *group;
    apr_thread_pool_t *thread_pool;
    Jxta_PGID *gid;
    Jxta_PID *pid;
    char *groupUniqueID;
    Jxta_boolean pv_id_gen_set;
    apr_uuid_t pv_id_gen;
    Jxta_object_compare_func candiate_order_cb;
    Jxta_object_compare_func ranking_order_cb;
    Jxta_object_compare_func candidate_cb;
    Jxta_peerview_candidate_list_func candidate_list_cb;
    Jxta_peerview_address_req_func addr_req_cb;

    char *assigned_id_str;

    /* Configuration */
    Jxta_RdvConfigAdvertisement *rdvConfig;
    Jxta_time_diff pa_refresh;

    Jxta_time_diff auto_cycle;
    int iterations_since_switch;
    int loneliness_factor;
    unsigned int my_suitability;
    unsigned int my_willingness;

    unsigned int clusters_count;
    unsigned int cluster_members;
    unsigned int replicas_count;

    Jxta_boolean active;
    unsigned int processing_callbacks;

    /**
    *   The instance mask of the peerview instance we are participating in or 
    *   NULL if we haven't yet chosen a peerview instance in which to participate.
    **/
    BIGNUM *instance_mask;

    BIGNUM *cluster_divisor;
    BIGNUM *peer_address_space;

    Jxta_endpoint_service *endpoint;
    Jxta_discovery_service *discovery;
    Jxta_rdv_service *rdv;
    void *ep_cookie;

    /**
    * The candidates to which we issue connectivity challenges. Provided by RdvServer.
    */
    Jxta_vector *candidates;

    /**
    * A hash table of all of the peers in the peerview for easy lookup.
    */
    Jxta_hashtable *pves;
    apr_uint64_t pves_modcount;

    /**
    *   An array of all of the clusters.
    */
    Cluster_entry *clusters;

    /**
    *   The cluster my pve is located.
    */
    unsigned int my_cluster;

    /**
    *   Our own pve or NULL if we have no assigned address within the peerview.
    */
    Peerview_entry *self_pve;

    /** 
    *   To generate peer view events 
    **/
    Jxta_hashtable *event_listener_table;
    /**
    *  Maintains queue of active events.  An event must be added to the list prior to 
    *  scheduling a new call_event_listeners thread
    */
    Jxta_vector *event_list;

    volatile Peerview_state state;

    /* ACTIVITY LOCATE */
    unsigned int activity_locate_probes;
    Jxta_vector *activity_locate_seeds;

    /* ACTIVITY MAINTAIN */
    Jxta_boolean activity_maintain_last_pves_modcount;
    Jxta_vector *activity_maintain_referral_peers;
    unsigned int activity_maintain_referral_pings_sent;

    /* ACTIVITY ADD */
    Jxta_boolean activity_add;
    Jxta_vector *activity_add_candidate_peers;
    Jxta_peer *activity_add_candidate_peer;
    unsigned int activity_add_candidate_pongs_sent;
    Jxta_vector * activity_add_candidate_voting_peers;
    Jxta_time activity_add_candidate_requests_expiration;
    Jxta_time activity_add_voting_ends;
    Jxta_boolean activity_add_voting;

};


typedef struct Peerview_histogram_entry Peerview_histogram_entry;

struct Peerview_histogram_entry {
    JXTA_OBJECT_HANDLE;

    BIGNUM *start;

    BIGNUM *end;

    Jxta_vector *peers;
};

Jxta_object_compare_func client_entry_creation_time_compare(Jxta_peer * me, Jxta_peer * target);


static Peerview_histogram_entry *peerview_histogram_entry_new(BIGNUM * start, BIGNUM * end);
static Peerview_histogram_entry *peerview_histogram_entry_construct(Peerview_histogram_entry * myself, BIGNUM * start,
                                                                    BIGNUM * end);
static void peerview_histogram_entry_delete(Jxta_object * addr);
static void peerview_histogram_entry_destruct(Peerview_histogram_entry * myself);

/**
*   Initialize a freshly allocated peerview object
*
*   @param obj The peerview to initialize.
*   @param parent_pool The pool in which the peerview should create it's sub-pool.
*   @return The constructed peerview object or NULL if the peerview object could not be initialized.
*/
static Jxta_peerview *peerview_construct(Jxta_peerview * myself, apr_pool_t * parent_pool);

/**
*   Deallocate all internal references before deleting the peerview object.
*
*   @param obj The peerview to destruct.
*/
static void peerview_destruct(Jxta_peerview * myself);

/**
*   Call destructors and free a peerview object.
*
*   @param obj The peerview to delete.
*/
static void peerview_delete(Jxta_object * obj);


static Peerview_entry *peerview_entry_new(Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv, apr_uuid_t * pv_id_gen);
static Peerview_entry *peerview_entry_construct(Peerview_entry * myself, Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv, apr_uuid_t *pv_id_gen);
static void peerview_entry_delete(Jxta_object * addr);
static void peerview_entry_destruct(Peerview_entry * myself);
static Jxta_status create_self_pve(Jxta_peerview * myself, BIGNUM * address);
static void peerview_update_id(Jxta_peerview * myself);

static Peerview_entry *peerview_get_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_boolean peerview_check_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_status peerview_add_pve(Jxta_peerview * myself, Peerview_entry * pve);
static Jxta_status peerview_remove_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_status peerview_remove_pve_1(Jxta_peerview * myself, Jxta_PID * pid, Jxta_boolean generate_event);
static Jxta_vector *peerview_get_all_pves(Jxta_peerview * myself);
static Jxta_status peerview_clear_pves(Jxta_peerview * myself, Jxta_boolean notify);
static Jxta_status peerview_get_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer, Jxta_vector **peers);

static Jxta_peerview_event *peerview_event_new(Jxta_Peerview_event_type event, Jxta_id * id);
static void peerview_event_delete(Jxta_object * ptr);

static Jxta_status peerview_init_cluster(Jxta_peerview * myself);

static Jxta_status JXTA_STDCALL peerview_protocol_cb(Jxta_object * obj, void *arg);
static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req);
static Jxta_status peerview_handle_address_assign(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);
static Jxta_status peerview_handle_ping(Jxta_peerview * myself, Jxta_peerview_ping_msg * ping);
static Jxta_status peerview_handle_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);
static Jxta_boolean peerview_handle_promotion(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);
static Jxta_status peerview_handle_adv_request(Jxta_peerview * me, Jxta_adv_request_msg * req);
static Jxta_status peerview_handle_adv_response(Jxta_peerview * me, Jxta_adv_response_msg * resp);
static Jxta_status peerview_send_address_request(Jxta_peerview * myself, Jxta_peer * dest);
static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean id_only);
static Jxta_status peerview_send_address_assign(Jxta_peerview * myself, Jxta_peer * dest, BIGNUM * target_hash);
static Jxta_status peerview_send_pvm(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg);
static Jxta_status peerview_send_adv_request(Jxta_peerview * myself, Jxta_id * dest_id, Jxta_vector *need_pids);

static void *APR_THREAD_FUNC call_event_listeners_thread(apr_thread_t * thread, void *arg);

static unsigned int cluster_for_hash(Jxta_peerview * myself, BIGNUM * target_hash);
static Jxta_boolean have_matching_PA(Jxta_peerview * me, Jxta_peerview_peer_info *peer, Jxta_PA ** requested_PA);

static Jxta_status joining_peerview(Jxta_peerview * me, const char *pv_mask);
static Jxta_boolean need_additional_peers(Jxta_peerview * me, int max_check);
static Jxta_boolean is_more_recent_uuid(apr_uuid_t * a, apr_uuid_t *b);
static int ranking_sort(Jxta_peer **peer_a, Jxta_peer **peer_b);
static int ranking_candidate_sort(Jxta_peer ** me, Jxta_peer ** target);
static Jxta_boolean remain_in_peerview(Jxta_peerview *me, unsigned int size);
static Jxta_boolean is_for_alternative(Jxta_peerview * me, const char *pv_mask);
static Jxta_status process_referrals(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);
static void trim_only_candidates(Jxta_vector * possible_v);
static Jxta_status get_best_rdv_candidates(Jxta_peerview *myself, Jxta_vector **candidates, int nums);
static void calculate_radius_bounds(Jxta_peerview * pv, Peerview_entry * pve);

static Jxta_status build_histogram(Jxta_vector ** histo, Jxta_vector * peers, BIGNUM * minimum, BIGNUM * maximum);

static void *APR_THREAD_FUNC activity_peerview_locate(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_addressing(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_announce(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_maintain(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_add(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_auto_cycle(apr_thread_t * thread, void *arg);

static Peerview_entry *peerview_entry_new(Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv, apr_uuid_t * pv_id_gen)
{
    Peerview_entry *myself;

    myself = (Peerview_entry *) calloc(1, sizeof(Peerview_entry));

    if (NULL == myself) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, peerview_entry_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Creating PVE [%pp]\n", myself);

    return peerview_entry_construct(myself, pid, ea, adv, pv_id_gen);
}

static Peerview_entry *peerview_entry_construct(Peerview_entry * myself, Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv, apr_uuid_t *pv_id_gen)
{
    myself = (Peerview_entry *) peer_entry_construct((_jxta_peer_entry *) myself);

    if (NULL != myself) {
        myself->thisType = "Peerview_entry";

        if (NULL != pv_id_gen) {
            myself->pv_id_gen_set = TRUE;
            memmove(&myself->pv_id_gen, pv_id_gen, sizeof(apr_uuid_t));
        } else {
            myself->pv_id_gen_set = FALSE;
        }
        if (NULL != pid) {
            jxta_peer_set_peerid((Jxta_peer *) myself, pid);
        }

        if (NULL == ea) {
            ea = jxta_endpoint_address_new_3(pid, NULL, NULL);
            jxta_peer_set_address((Jxta_peer *) myself, ea);
            JXTA_OBJECT_RELEASE(ea);
        } else {
            jxta_peer_set_address((Jxta_peer *) myself, ea);
        }

        if (NULL != adv) {
            jxta_peer_set_adv((Jxta_peer *) myself, adv);
            myself->adv_gen_set = jxta_PA_get_SN(adv, &myself->adv_gen);
        }

        myself->target_hash = BN_new();

        myself->target_hash_radius = BN_new();
        myself->radius_range_1_start = NULL;
        myself->radius_range_1_end = NULL;
        myself->radius_range_2_start = NULL;
        myself->radius_range_2_end = NULL;

        myself->created_at = jpr_time_now();

    }

    return myself;
}

static void peerview_entry_delete(Jxta_object * addr)
{
    Peerview_entry *myself = (Peerview_entry *) addr;

    peerview_entry_destruct(myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Deleting PVE [%pp]\n", myself);

    memset(myself, 0xdd, sizeof(Peerview_entry));

    free(myself);
}

static void peerview_entry_destruct(Peerview_entry * myself)
{
    if (NULL != myself->target_hash) {
        BN_free(myself->target_hash);
        myself->target_hash = NULL;
    }

    if (NULL != myself->target_hash_radius) {
        BN_free(myself->target_hash_radius);
        myself->target_hash_radius = NULL;
    }
    if (NULL != myself->radius_range_1_start) {
        BN_free(myself->radius_range_1_start);
        myself->radius_range_1_start = NULL;
    }
    if (NULL != myself->radius_range_1_end) {
        BN_free(myself->radius_range_1_end);
        myself->radius_range_1_end = NULL;
    }
    if (NULL != myself->radius_range_2_start) {
        BN_free(myself->radius_range_2_start);
        myself->radius_range_2_start = NULL;
    }
    if (NULL != myself->radius_range_2_end) {
        BN_free(myself->radius_range_2_end);
        myself->radius_range_2_end = NULL;
    }

    peer_entry_destruct((_jxta_peer_entry *) myself);
}

JXTA_DECLARE(Jxta_peerview *) jxta_peerview_new(apr_pool_t * parent_pool)
{
    Jxta_peerview *myself;

    myself = (Jxta_peerview *) calloc(1, sizeof(Jxta_peerview));

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, peerview_delete, 0);

    return peerview_construct(myself, parent_pool);
}

static void peerview_delete(Jxta_object * obj)
{
    Jxta_peerview *myself = (Jxta_peerview *) obj;

    peerview_destruct(myself);

    memset(myself, 0xdd, sizeof(Peerview_entry));

    free(myself);
}

static Jxta_peerview *peerview_construct(Jxta_peerview * myself, apr_pool_t * parent_pool)
{
    apr_status_t res;

    myself->thisType = "Jxta_peerview";

    res = apr_pool_create(&myself->pool, parent_pool);

    if (res != APR_SUCCESS) {
        JXTA_OBJECT_RELEASE(myself);
        return NULL;
    }

    res = apr_thread_mutex_create(&myself->mutex, APR_THREAD_MUTEX_NESTED, myself->pool);

    if (APR_SUCCESS == res) {
        myself->running = FALSE;
        myself->group = NULL;
        myself->thread_pool = NULL;
        myself->gid = NULL;
        myself->pid = NULL;
        myself->groupUniqueID = NULL;
        myself->assigned_id_str = NULL;

        myself->rdvConfig = NULL;
        myself->pa_refresh = PA_REFRESH_INTERVAL;

        myself->endpoint = NULL;
        myself->discovery = NULL;
        myself->rdv = NULL;

        myself->my_cluster = INT_MAX;
        myself->clusters = NULL;
        myself->self_pve = NULL;
        myself->my_willingness = 25;
        myself->my_suitability = 50;
        myself->candiate_order_cb = (Jxta_object_compare_func) client_entry_creation_time_compare;
        myself->ranking_order_cb = (Jxta_object_compare_func) ranking_sort;
        myself->candidate_cb = (Jxta_object_compare_func) ranking_candidate_sort;

        myself->event_listener_table = jxta_hashtable_new(1);

        myself->activity_maintain_referral_peers = jxta_vector_new(0);
        myself->activity_maintain_referral_pings_sent = 0;

        myself->activity_add_candidate_peers = jxta_vector_new(0);
        myself->activity_add_candidate_pongs_sent = 0;
        myself->activity_add_candidate_voting_peers = jxta_vector_new(0);
        myself->activity_add_candidate_requests_expiration = 0;
        myself->activity_add_voting_ends = 0;
        myself->state = PV_STOPPED;
    } else {
        myself = NULL;
    }

    return myself;
}

static void peerview_destruct(Jxta_peerview * myself)
{

    if (myself->running) {
        peerview_stop(myself);
    }

    myself->thread_pool = NULL;

    if (NULL != myself->groupUniqueID) {
        free(myself->groupUniqueID);
    }

    if (NULL != myself->gid) {
        JXTA_OBJECT_RELEASE(myself->gid);
    }

    if (NULL != myself->pid) {
        JXTA_OBJECT_RELEASE(myself->pid);
    }

    if (NULL != myself->assigned_id_str) {
        free(myself->assigned_id_str);
    }

    if (NULL != myself->endpoint) {
        JXTA_OBJECT_RELEASE(myself->endpoint);
    }

    if (NULL != myself->discovery) {
        JXTA_OBJECT_RELEASE(myself->discovery);
    }

    if (NULL != myself->rdv) {
        JXTA_OBJECT_RELEASE(myself->rdv);
    }

    if (NULL != myself->rdvConfig) {
        JXTA_OBJECT_RELEASE(myself->rdvConfig);
    }

    if (NULL != myself->clusters) {
        unsigned int each_cluster;
        for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {
            BN_free(myself->clusters[each_cluster].min);
            BN_free(myself->clusters[each_cluster].mid);
            BN_free(myself->clusters[each_cluster].max);
            JXTA_OBJECT_RELEASE(myself->clusters[each_cluster].members);
            JXTA_OBJECT_RELEASE(myself->clusters[each_cluster].histogram);
        }
        memset(myself->clusters, 0xDD, sizeof(Cluster_entry *) * myself->clusters_count);
        free(myself->clusters);
    }

    if (NULL != myself->pves) {
        JXTA_OBJECT_RELEASE(myself->pves);
    }

    if (NULL != myself->self_pve) {
        JXTA_OBJECT_RELEASE(myself->self_pve);
    }

    if (NULL != myself->event_listener_table) {
        JXTA_OBJECT_RELEASE(myself->event_listener_table);
    }
    
    if (NULL != myself->event_list) {
        JXTA_OBJECT_RELEASE(myself->event_list);
    }

    if (NULL !=  myself->activity_maintain_referral_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_maintain_referral_peers);
    }

    if (NULL != myself->activity_add_candidate_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_add_candidate_peers);
    }

    if (NULL != myself->activity_add_candidate_voting_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_add_candidate_voting_peers);
    }

    if (NULL != myself->activity_locate_seeds) {
        JXTA_OBJECT_RELEASE(myself->activity_locate_seeds);
    }

    if (NULL != myself->cluster_divisor) {
        BN_free(myself->cluster_divisor);
    }

    if (NULL != myself->peer_address_space) {
        BN_free(myself->peer_address_space);
    }

    if (NULL != myself->instance_mask) {
        BN_free(myself->instance_mask);
    }

    apr_thread_mutex_destroy(myself->mutex);

    /* Free the pool */

    apr_pool_destroy(myself->pool);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Destruction of [%pp] finished\n", myself);

}

JXTA_DECLARE(Jxta_status) peerview_init(Jxta_peerview * pv, Jxta_PG * group, Jxta_id * assigned_id)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);
    JString *string = NULL;
    Jxta_rdv_service *rdv = NULL;

    jxta_id_to_jstring(assigned_id, &string);

    myself->assigned_id_str = strdup(jstring_get_string(string));

    if (NULL == myself->assigned_id_str) {
        if (string != NULL)
            JXTA_OBJECT_RELEASE(string);
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_RELEASE(string);

    myself->group = group;

    myself->thread_pool = jxta_PG_thread_pool_get(group);
    jxta_PG_get_PID(group, &myself->pid);
    jxta_PG_get_GID(group, &myself->gid);
    jxta_id_get_uniqueportion(myself->gid, &string);
    myself->groupUniqueID = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    /* Determine our configuration */
    jxta_PG_get_rendezvous_service(group, &rdv);
    if (NULL != rdv) {
        myself->rdvConfig = (Jxta_RdvConfigAdvertisement *) JXTA_OBJECT_SHARE(jxta_rdv_service_config_adv(rdv));
        JXTA_OBJECT_RELEASE(rdv);
    }

    if (NULL == myself->rdvConfig) {
        myself->rdvConfig = jxta_RdvConfigAdvertisement_new();
    }
    if (jxta_RdvConfig_get_auto_rdv_interval(myself->rdvConfig) > 0) {
        myself->my_willingness = (jxta_RdvConfig_get_config(myself->rdvConfig) == config_rendezvous) ? 75:25;
    } else {
        myself->my_willingness = (jxta_RdvConfig_get_config(myself->rdvConfig) == config_rendezvous) ? 100:0;
    }
    if (-1 == jxta_RdvConfig_pv_clusters(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_clusters(myself->rdvConfig, DEFAULT_CLUSTERS_COUNT);
    }

    if (-1 == jxta_RdvConfig_pv_members(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_members(myself->rdvConfig, DEFAULT_CLUSTER_MEMBERS);
    }

    if (-1 == jxta_RdvConfig_pv_replication(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_replication(myself->rdvConfig, DEFAULT_REPLICATION_FACTOR);
    }

    if (-1 == jxta_RdvConfig_pv_loneliness(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_loneliness(myself->rdvConfig, DEFAULT_LONELINESS_FACTOR);
    }

    if (-1 == jxta_RdvConfig_pv_max_locate_probes(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_max_locate_probes(myself->rdvConfig, DEFAULT_MAX_LOCATE_PROBES);
    }

    if (-1 == jxta_RdvConfig_pv_max_address_requests(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_max_address_requests(myself->rdvConfig, DEFAULT_MAX_ADDRESS_REQUESTS);
    }

    if (-1 == jxta_RdvConfig_pv_max_ping_probes(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_max_ping_probes(myself->rdvConfig, DEFAULT_MAX_PING_PROBES);
    }

    if (-1 == jxta_RdvConfig_pv_add_interval(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_add_interval(myself->rdvConfig, JXTA_PEERVIEW_ADD_INTERVAL);
    }

    if (-1 == jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_maintenance_interval(myself->rdvConfig, JXTA_PEERVIEW_MAINTAIN_INTERVAL);
    }

    if (-1 == jxta_RdvConfig_pv_entry_expires(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_entry_expires(myself->rdvConfig, JXTA_PEERVIEW_PVE_EXPIRATION);
    }

    if (-1 == jxta_RdvConfig_pv_ping_due(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_ping_due(myself->rdvConfig, JXTA_PEERVIEW_PVE_PING_DUE);
    }

    if (-1 == jxta_RdvConfig_pv_pong_due(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_pong_due(myself->rdvConfig, JXTA_PEERVIEW_PVE_PONG_DUE);
    }

    if (-1 == jxta_RdvConfig_pv_voting_expiration(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_voting_expiration(myself->rdvConfig, JXTA_PEERVIEW_VOTING_EXPIRATION);
    }

    if (-1 == jxta_RdvConfig_pv_voting_wait(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_voting_wait(myself->rdvConfig, JXTA_PEERVIEW_VOTING_WAIT);
    }

    myself->clusters_count = jxta_RdvConfig_pv_clusters(myself->rdvConfig);
    myself->cluster_members = jxta_RdvConfig_pv_members(myself->rdvConfig);
    myself->replicas_count = jxta_RdvConfig_pv_replication(myself->rdvConfig);
    myself->pves = jxta_hashtable_new(myself->clusters_count * myself->cluster_members * 2);
    myself->processing_callbacks = 0;
    myself->event_list = jxta_vector_new(2);

    return res;
}

static Jxta_status peerview_init_cluster(Jxta_peerview * myself)
{
    unsigned int each_cluster;
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *clusters_count;
    BIGNUM *hash_space;
    BIGNUM *cluster_members;
    BIGNUM *replicas_count;
    char *tmp;

    /* initialize the cluster divisor */
    clusters_count = BN_new();
    hash_space = BN_new();
    myself->cluster_divisor = BN_new();
    BN_set_word(clusters_count, myself->clusters_count);
    BN_lshift(hash_space, BN_value_one(), CHAR_BIT * SHA_DIGEST_LENGTH);
    tmp = BN_bn2hex(hash_space);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Hash Space : %s\n", tmp);
    free(tmp);
    BN_div(myself->cluster_divisor, NULL, hash_space, clusters_count, ctx);
    tmp = BN_bn2hex(myself->cluster_divisor);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cluster Divisor : %s\n", tmp);
    free(tmp);
    BN_free(hash_space);
    BN_free(clusters_count);

    /* Determine the portion of the address space which will normally be serviced by one peer. 
     * (may actually be larger or smaller than this value) 
     */
    myself->peer_address_space = BN_new();
    cluster_members = BN_new();
    replicas_count = BN_new();
    BN_set_word(cluster_members, myself->cluster_members);
    BN_set_word(replicas_count, myself->replicas_count);
    BN_div(myself->peer_address_space, NULL, myself->cluster_divisor, cluster_members, ctx);
    BN_mul(myself->peer_address_space, myself->peer_address_space, replicas_count, ctx);
    BN_free(replicas_count);
    BN_free(cluster_members);

    tmp = BN_bn2hex(myself->peer_address_space);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer Address Space : %s\n", tmp);
    free(tmp);

    /* Initialize cluster tables */
    myself->clusters = (Cluster_entry *) calloc(myself->clusters_count, sizeof(Cluster_entry));
    for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {
        BIGNUM *current_cluster = BN_new();
        BN_set_word(current_cluster, each_cluster);

        myself->clusters[each_cluster].min = BN_new();
        BN_mul(myself->clusters[each_cluster].min, myself->cluster_divisor, current_cluster, ctx);
        BN_free(current_cluster);

        myself->clusters[each_cluster].max = BN_new();
        BN_add(myself->clusters[each_cluster].max, myself->clusters[each_cluster].min, myself->cluster_divisor);
        BN_sub(myself->clusters[each_cluster].max, myself->clusters[each_cluster].max, BN_value_one());

        myself->clusters[each_cluster].mid = BN_dup(myself->cluster_divisor);
        BN_rshift1(myself->clusters[each_cluster].mid, myself->clusters[each_cluster].mid);
        BN_add(myself->clusters[each_cluster].mid, myself->clusters[each_cluster].mid, myself->clusters[each_cluster].min);

        myself->clusters[each_cluster].members = jxta_vector_new(myself->cluster_members);
        myself->clusters[each_cluster].histogram = NULL;

        tmp = BN_bn2hex(myself->clusters[each_cluster].min);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cluster %d min:%s\n", each_cluster, tmp);
        free(tmp);
        tmp = BN_bn2hex(myself->clusters[each_cluster].mid);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cluster %d mid:%s\n", each_cluster, tmp);
        free(tmp);
        tmp = BN_bn2hex(myself->clusters[each_cluster].max);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cluster %d max:%s\n", each_cluster, tmp);
        free(tmp);

    }

    BN_CTX_free(ctx);

    return JXTA_SUCCESS;
}

Jxta_status peerview_start(Jxta_peerview * pv)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    myself->processing_callbacks = 0;

    res = peerview_init_cluster(myself);

    /* Register Endpoint Listener for Peerview protocol. */
    jxta_PG_get_endpoint_service(myself->group, &(myself->endpoint));
    jxta_PG_add_recipient(myself->group, &myself->ep_cookie, RDV_V3_MSID, JXTA_PEERVIEW_NAME, peerview_protocol_cb, myself);

    jxta_PG_get_discovery_service(myself->group, &(myself->discovery));
    jxta_PG_get_rendezvous_service(myself->group, &(myself->rdv));

    myself->state = PV_PASSIVE;
    myself->running = TRUE;

    peerview_update_id(myself);

    if (myself->auto_cycle > 0) {
        myself->iterations_since_switch = 0;
        myself->loneliness_factor = 0;
        res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_auto_cycle, myself, myself->auto_cycle, &myself->auto_cycle);

        if (APR_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not schedule auto cycle activity : %d\n", myself, res);
        }

    }

    apr_thread_mutex_unlock(myself->mutex);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview started for group %s / %s \n", myself->groupUniqueID,
                    myself->assigned_id_str);
    }

    return res;
}

Jxta_status peerview_get_peer(Jxta_peerview * pv, Jxta_id * peer_id, Jxta_peer **peer)
{
    Jxta_status ret = JXTA_ITEM_NOTFOUND;
    Peerview_entry *pve;

    pve = peerview_get_pve(pv, peer_id);
    if (NULL != pve) {
        ret = JXTA_SUCCESS;
    }
    *peer = (Jxta_peer *) pve;

    return ret;
}

static void peerview_update_id(Jxta_peerview * myself)
{
    apr_uuid_t * uuid_ptr = NULL;
    char tmp[64];

    if (!myself->pv_id_gen_set) {
        uuid_ptr = jxta_id_uuid_new();
        if (uuid_ptr) {
            memmove(&myself->pv_id_gen, uuid_ptr, sizeof(apr_uuid_t));
            myself->pv_id_gen_set = TRUE;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to obtain a new uuid\n");
        }
    } else {
        apr_uuid_format(tmp, &myself->pv_id_gen);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incrementing uuid %s \n", tmp);
        uuid_ptr = jxta_id_uuid_increment_seq_number(&myself->pv_id_gen);
        memmove(&myself->pv_id_gen, uuid_ptr, sizeof(apr_uuid_t));
        apr_uuid_format(tmp, &myself->pv_id_gen);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incremented uuid %s \n", tmp);
    }
    apr_uuid_format(tmp, &myself->pv_id_gen);
    if (uuid_ptr) {
        free(uuid_ptr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Created a new pv_id_gen %s \n", tmp);
    }
}

JXTA_DECLARE(apr_uuid_t *) jxta_peerview_get_pv_id_gen(Jxta_peerview *pv)
{
    apr_uuid_t *result = NULL;

    if (pv->pv_id_gen_set) {
        result = calloc(1, sizeof(apr_uuid_t));
        memmove(result, &pv->pv_id_gen, sizeof(apr_uuid_t));
    }
    return result;
}

/* 
 * Create our PVE
 */
static Jxta_status create_self_pve(Jxta_peerview * myself, BIGNUM * address)
{
    Jxta_status res = JXTA_SUCCESS;
    char *tmp;
    Jxta_PA *pa;
    jxta_PG_get_PA(myself->group, &pa);
    Jxta_rdv_lease_options *rdv_options=NULL;
    Jxta_vector *option_vector;

    if (NULL != myself->self_pve) {
        peerview_remove_pve(myself, jxta_peer_peerid((Jxta_peer *) myself->self_pve));
        JXTA_OBJECT_RELEASE(myself->self_pve);
        myself->self_pve = NULL;
    }
    myself->self_pve = peerview_entry_new(myself->pid, NULL, pa, NULL);

    myself->self_pve->adv_gen_set = jxta_PA_get_SN(pa, &myself->self_pve->adv_gen);

    JXTA_OBJECT_RELEASE(pa);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing Self PVE : %x\n", myself->self_pve);

    jxta_peer_set_expires((Jxta_peer *) myself->self_pve, JPR_ABSOLUTE_TIME_MAX);

    rdv_options = jxta_rdv_lease_options_new();
    if (NULL != rdv_options) {
        /* XXX 20070913 These could be better tuned. */
        jxta_rdv_lease_options_set_suitability(rdv_options, myself->my_suitability);
        jxta_rdv_lease_options_set_willingness(rdv_options, myself->my_willingness);
    }

    option_vector = jxta_vector_new(1);
    jxta_vector_add_object_last(option_vector, (Jxta_object *) rdv_options);
    jxta_peer_set_options((Jxta_peer *) myself->self_pve, option_vector);
    JXTA_OBJECT_RELEASE(rdv_options);
    JXTA_OBJECT_RELEASE(option_vector);

    tmp = BN_bn2hex(address);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Self Target Hash : %s\n", tmp);
    free(tmp);

    BN_copy(myself->self_pve->target_hash, address);

    myself->my_cluster = cluster_for_hash(myself, myself->self_pve->target_hash);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Self Cluster : %d\n", myself->my_cluster);

    BN_copy(myself->self_pve->target_hash_radius, myself->peer_address_space);
    BN_rshift1(myself->self_pve->target_hash_radius, myself->self_pve->target_hash_radius);

    calculate_radius_bounds(myself, myself->self_pve);

    res = peerview_add_pve(myself, myself->self_pve);

    return res;
}

Jxta_status peerview_stop(Jxta_peerview * pv)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Stopping peerview : %pp\n", pv);

    apr_thread_mutex_lock(myself->mutex);
    
    if (myself->running == FALSE) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peerview(%pp) not running\n", pv);
        apr_thread_mutex_unlock(myself->mutex);
        return JXTA_FAILED;
    }

    myself->state = PV_STOPPED;

    while (myself->processing_callbacks > 0) {
        apr_thread_mutex_unlock(myself->mutex);
        /* sleep for 100 ms */
        apr_sleep(100 * 1000);
        apr_thread_mutex_lock(myself->mutex);
    }

    /* FIXME bondolo Announce that we are shutting down */

    peerview_clear_pves(myself, TRUE);

    myself->running = FALSE;

    apr_thread_mutex_unlock(myself->mutex);
    if (myself->auto_cycle > 0) {
        apr_thread_pool_tasks_cancel(myself->thread_pool, &myself->auto_cycle);
    }
    apr_thread_pool_tasks_cancel(myself->thread_pool, &myself->activity_add);
    apr_thread_pool_tasks_cancel(myself->thread_pool, myself);
    apr_thread_mutex_lock(myself->mutex);
    
    jxta_vector_clear(myself->activity_maintain_referral_peers);
    jxta_vector_clear(myself->activity_add_candidate_peers);
    jxta_vector_clear(myself->event_list);

    jxta_PG_remove_recipient(myself->group, myself->ep_cookie);
    myself->ep_cookie = NULL;

    apr_thread_mutex_unlock(myself->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_member(Jxta_peerview * me)
{

    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_boolean result;

    apr_thread_mutex_lock(myself->mutex);

    result = (NULL != myself->self_pve);

    apr_thread_mutex_unlock(myself->mutex);

    return result;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_active(Jxta_peerview * me)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_boolean result;

    apr_thread_mutex_lock(myself->mutex);

    result = myself->active;

    apr_thread_mutex_unlock(myself->mutex);

    return result;
}

JXTA_DECLARE(void) jxta_peerview_set_auto_cycle(Jxta_peerview * pv, Jxta_time_diff ttime )
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    myself->auto_cycle = 1000 * ttime;

    apr_thread_mutex_unlock(myself->mutex);

    return;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_set_active(Jxta_peerview * me, Jxta_boolean active)
{
    Jxta_boolean result;
    apr_status_t res;

    assert(PTValid(me, Jxta_peerview));

    apr_thread_mutex_lock(me->mutex);

    result = me->active;
    me->active = active;

    if (active && PV_PASSIVE == me->state) {
        me->state = PV_LOCATING;
        /* Start searching right away! */
        me->activity_locate_probes = 0;
        if (NULL != me->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(me->activity_locate_seeds);
        }
        me->activity_locate_seeds = NULL;   /* will reload when activity runs */
        res = apr_thread_pool_schedule(me->thread_pool, activity_peerview_locate, me, 
                                       apr_time_from_sec(1 << me->activity_locate_probes++), me);

        if (APR_SUCCESS != res) {
            me->state = PV_PASSIVE;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not schedule locating activity : %d\n", 
                            me, res);
        }
    }

    apr_thread_mutex_unlock(me->mutex);

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_globalview(Jxta_peerview * pv, Jxta_vector ** view)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    if (NULL != myself->self_pve) {
        /* We are an active peerview member, provide our view */
        unsigned int each_cluster;

        *view = jxta_vector_new(myself->clusters_count);

        for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {
            Jxta_peer *associate;

            res = jxta_peerview_get_associate_peer(myself, each_cluster, &associate);

            if (JXTA_SUCCESS == res) {
                jxta_vector_add_object_last(*view, (Jxta_object *) associate);
                JXTA_OBJECT_RELEASE(associate);
            }
        }
    } else {
        /* We are inactive. No view. */
        *view = jxta_vector_new(1);
    }

    res = JXTA_SUCCESS;

    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(unsigned int) jxta_peerview_get_globalview_size(Jxta_peerview * me)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);

    return myself->clusters_count;
}

JXTA_DECLARE(JString *) jxta_peerview_get_instance_mask(Jxta_peerview * me)
{
    char * tmp=NULL;
    JString * ret=NULL;

    Jxta_peerview *myself = PTValid(me, Jxta_peerview);

    if (NULL != myself->instance_mask) {
        tmp = BN_bn2hex(myself->instance_mask);
        if (NULL != tmp) {
            ret = jstring_new_2(tmp);
            free(tmp);
        }
    }
    return ret;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_localview(Jxta_peerview * me, Jxta_vector ** cluster_view)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    if (NULL != myself->self_pve) {
        /* We are an active peerview member, provide our view */

        res = jxta_vector_clone(myself->clusters[myself->my_cluster].members, cluster_view, 0, INT_MAX);
    } else {
        /* We are inactive. No view. */
        *cluster_view = jxta_vector_new(1);
        res = JXTA_SUCCESS;
    }

    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(unsigned int) jxta_peerview_get_localview_size(Jxta_peerview * me)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    unsigned int result;

    apr_thread_mutex_lock(myself->mutex);

    if (NULL != myself->self_pve) {
        result = jxta_vector_size(myself->clusters[myself->my_cluster].members);
    } else {
        result = 0;
    }

    apr_thread_mutex_unlock(myself->mutex);

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_cluster_view(Jxta_peerview * pv, unsigned int cluster, Jxta_vector ** view)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    if (cluster > myself->clusters_count - 1) {
        res = JXTA_INVALID_ARGUMENT;
    } else {
        res = jxta_vector_clone(myself->clusters[cluster].members, view, 0, INT_MAX);
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;

}

JXTA_DECLARE(unsigned int) jxta_peerview_get_cluster_number(Jxta_peerview * me)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);

    return myself->my_cluster;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_gen_hash(Jxta_peerview * me, unsigned char const *value, size_t length, BIGNUM ** hash)
{
    Jxta_status res = JXTA_SUCCESS;
    me = PTValid(me, Jxta_peerview);
    unsigned char digest[SHA_DIGEST_LENGTH];

    if (NULL == hash) {
        return JXTA_INVALID_ARGUMENT;
    }

    SHA1(value, length, (unsigned char *) digest);
    *hash = BN_new();
    BN_bin2bn(digest, SHA_DIGEST_LENGTH, *hash);

    return res;
}

static unsigned int cluster_for_hash(Jxta_peerview * myself, BIGNUM * target_hash)
{
    BN_CTX *ctx = BN_CTX_new();
    BIGNUM *bn_target_cluster = BN_new();
    char *tmp;
    unsigned int target_cluster;

    tmp = BN_bn2hex(myself->peer_address_space);
    free(tmp);

    tmp = BN_bn2hex(target_hash);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "target_hash : %s\n", tmp);
    free(tmp);

    BN_div(bn_target_cluster, NULL, target_hash, myself->cluster_divisor, ctx);
    target_cluster = (unsigned int) BN_get_word(bn_target_cluster);
    BN_free(bn_target_cluster);
    BN_CTX_free(ctx);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "target_cluster : %d\n", target_cluster);

    return target_cluster;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_peer_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer)
{
    return peerview_get_for_target_hash(me,target_hash, peer, NULL);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_peers_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer **peer, Jxta_vector ** peers)
{
    return peerview_get_for_target_hash(me,target_hash, peer, peers);
}

static Jxta_status peerview_get_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer, Jxta_vector **peers)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    BIGNUM *bn_target_hash = (BIGNUM *) target_hash;

    unsigned int target_cluster = cluster_for_hash(myself, bn_target_hash);

    apr_thread_mutex_lock(myself->mutex);

    if (NULL != peers) {
        *peers = jxta_vector_new(0);
    }
    if (NULL != myself->self_pve) {
        int i;
        if (myself->my_cluster != target_cluster) {
            Jxta_peer *associate = NULL;

            res = jxta_peerview_get_associate_peer(myself, target_cluster, &associate);
            if (NULL != peer) {
                *peer = NULL != associate ? JXTA_OBJECT_SHARE(associate):NULL;
            }
            if (NULL != peers && NULL != associate) {
                jxta_vector_add_object_last(*peers, (Jxta_object *) associate);
            }
            if (associate)
                JXTA_OBJECT_RELEASE(associate);
        } else {
            /* The target is in our cluster. We refer them to the peer whose target hash most closely matches. */
            unsigned int all_peers;
            unsigned int each_peer;
            BIGNUM *effective_target_hash = BN_new();
            BIGNUM *distance = BN_new();
            BIGNUM *best_distance = BN_new();

            if (NULL != peer) {
                *peer = JXTA_OBJECT_SHARE(myself->self_pve);
            }
            BN_sub(best_distance, bn_target_hash, myself->self_pve->target_hash);

            all_peers = jxta_vector_size(myself->clusters[myself->my_cluster].members);
            for (each_peer = 0; each_peer < all_peers; each_peer++) {
                Peerview_entry *candidate;
                unsigned int each_segment;

                res =
                    jxta_vector_get_object_at(myself->clusters[myself->my_cluster].members, JXTA_OBJECT_PPTR(&candidate),
                                              each_peer);

                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve peer entry.\n");
                    if (NULL != peer) {
                        JXTA_OBJECT_RELEASE(*peer);
                        *peer = NULL;
                    }
                    return JXTA_FAILED;
                }

                /* We do the comparison for each peer 3 times to account for address space wrapping. */
                for (each_segment = 0; each_segment < 3; each_segment++) {
                    if (0 == each_segment) {
                        /* #0 - The area below minimum wrapped around to maximum */
                        BN_sub(effective_target_hash, myself->clusters[target_cluster].min, candidate->target_hash);
                        BN_sub(effective_target_hash, myself->clusters[target_cluster].max, effective_target_hash);
                    } else if (1 == each_segment) {
                        /* #1 - The area between minimum and maximum */
                        BN_copy(effective_target_hash, candidate->target_hash);
                    } else if (2 == each_segment) {
                        /* #2 - The area above maximum wrapped around to minimum */
                        BN_sub(effective_target_hash, candidate->target_hash, myself->clusters[target_cluster].max);
                        BN_add(effective_target_hash, effective_target_hash, myself->clusters[target_cluster].min);
                    }

                    BN_sub(distance, bn_target_hash, effective_target_hash);

                    if (BN_ucmp(distance, best_distance) < 0) {
                        /* The absolute distance is lower, make this peer the destination peer. */
                        if (NULL != peer) {
                            JXTA_OBJECT_RELEASE(*peer);
                            *peer = JXTA_OBJECT_SHARE(candidate);
                        }
                        BN_copy(best_distance, distance);
                    }
                }
                if (NULL != peers) {
                    /* return this peer if the target hash is within its radius */
                    if (BN_cmp(bn_target_hash, candidate->radius_range_1_start) >= 0
                        && BN_cmp(candidate->radius_range_1_end, bn_target_hash) <= 0) {
                        jxta_vector_add_object_last(*peers, (Jxta_object *) candidate);
                    }
                    if (candidate->radius_range_2_start && (BN_cmp(bn_target_hash, candidate->radius_range_2_start) >= 0
                        && BN_cmp(candidate->radius_range_2_end, bn_target_hash) <= 0)) {
                        jxta_vector_add_object_last(*peers, (Jxta_object *) candidate);
                    }
                }
                JXTA_OBJECT_RELEASE(candidate);
            }

            BN_free(distance);
            BN_free(best_distance);
            BN_free(effective_target_hash);

            res = JXTA_SUCCESS;
        }
        for (i=0; NULL != peers && i < jxta_vector_size(*peers); i++) {
            Jxta_peer *ret_peer = NULL;
            JString * peerid_closest_j = NULL;

            jxta_id_to_jstring(jxta_peer_peerid(*peer), &peerid_closest_j);
            res = jxta_vector_get_object_at(*peers, JXTA_OBJECT_PPTR(&ret_peer), i);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error retrieving peer from return vector res:%d\n", res );
                break;
            }
            /* don't return the closest peer in the vector also */
            if (jxta_id_equals(jxta_peer_peerid(ret_peer), jxta_peer_peerid(*peer))) {
                jxta_vector_remove_object_at(*peers, NULL, i);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Remove target peer %s\n", jstring_get_string(peerid_closest_j));
                JXTA_OBJECT_RELEASE(ret_peer);
                JXTA_OBJECT_RELEASE(peerid_closest_j);
                break;
            } else {
                JString * peerid_j = NULL;

                jxta_id_to_jstring(jxta_peer_peerid(ret_peer), &peerid_j);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding peer %s that is within radius -- target %s\n"
                        , jstring_get_string(peerid_j), jstring_get_string(peerid_closest_j));

                JXTA_OBJECT_RELEASE(peerid_j);
            }
            JXTA_OBJECT_RELEASE(peerid_closest_j);
            JXTA_OBJECT_RELEASE(ret_peer);
        }
    } else {
        /* likely only during shutdown */
        res = JXTA_FAILED;
    }

    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_associate_peer(Jxta_peerview * me, unsigned int cluster, Jxta_peer ** peer)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    unsigned int each_cluster;

    if ((NULL == peer) || (cluster >= myself->clusters_count)) {
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(myself->mutex);


    if (NULL == myself->self_pve) {
        /* likely only during shutdown */
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    if (cluster == myself->my_cluster) {
        *peer = JXTA_OBJECT_SHARE(myself->self_pve);
        goto FINAL_EXIT;
    }

    *peer = NULL;

    /* Scan upwards (modulus the number of clusters) for a cluster that can provide an associate */
    for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {
        unsigned a_cluster = (cluster + each_cluster) % myself->clusters_count;
        if (a_cluster == myself->my_cluster) continue;
        if (0 != jxta_vector_size(myself->clusters[a_cluster].members)) {
            /* XXX 20061009 bondolo Rather than using the oldest associate we 
               might want to choose an associate by some other mechanism (similar
               relative hash position in cluster, least similar relative hash
               position. The current approach is more likely to produce unbalanced 
               loading of associate peers. */
            res = jxta_vector_get_object_at(myself->clusters[a_cluster].members, JXTA_OBJECT_PPTR(peer), 0);
            if (JXTA_SUCCESS == res) {
                break;
            }
        }
    }

    if (NULL == *peer) {
        res = JXTA_FAILED;
    }

  FINAL_EXIT:

    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

extern char *peerview_get_assigned_id_str(Jxta_peerview * pv)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    return strdup(myself->assigned_id_str);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_self_peer(Jxta_peerview * pv, Jxta_peer ** peer)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);


    apr_thread_mutex_lock(myself->mutex);

    *peer = myself->self_pve ? JXTA_OBJECT_SHARE(myself->self_pve) : NULL;

    apr_thread_mutex_unlock(myself->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_set_pa_refresh(Jxta_peerview * pv, Jxta_time_diff interval)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    myself->pa_refresh = interval;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_time_diff) jxta_peerview_get_pa_refresh(Jxta_peerview * pv)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    return myself->pa_refresh;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_set_ranking_order_cb(Jxta_peerview * pv, Jxta_object_compare_func cb)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    myself->ranking_order_cb = cb;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_set_candidate_order_cb(Jxta_peerview * pv, Jxta_object_compare_func cb)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    myself->candiate_order_cb = cb;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_set_candidate_list_cb(Jxta_peerview * pv,  Jxta_peerview_candidate_list_func cb)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    myself->candidate_list_cb = cb;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_set_address_req_cb(Jxta_peerview * pv,  Jxta_peerview_address_req_func cb)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    myself->addr_req_cb = cb;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_add_event_listener(Jxta_peerview * pv, const char *serviceName,
                                                           const char *serviceParam, Jxta_listener * listener)
{
    Jxta_boolean res;
    size_t str_len;
    char *str = NULL;

    /* Test arguments first */
    if ((listener == NULL) || (serviceName == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    str_len = strlen(serviceName);

    if (NULL != serviceParam) {
        str_len += strlen(serviceParam) + 1;
    }

    str = calloc(str_len + 1, sizeof(char));

    if (NULL == str) {
        return JXTA_NOMEM;
    }

    strcpy(str, serviceName);

    if (NULL != serviceParam) {
        strcat(str, "/");
        strcat(str, serviceParam);
    }

    res = jxta_hashtable_putnoreplace(pv->event_listener_table, str, str_len, (Jxta_object *) listener);

    free(str);
    return res ? JXTA_SUCCESS : JXTA_BUSY;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_remove_event_listener(Jxta_peerview * pv, const char *serviceName,
                                                              const char *serviceParam)
{
    Jxta_status res = JXTA_SUCCESS;
    size_t str_len;
    char *str;

    /* Test arguments first */
    if ((serviceName == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    str_len = strlen(serviceName);

    if (NULL != serviceParam) {
        str_len += strlen(serviceParam) + 1;
    }

    str = calloc(str_len + 1, sizeof(char));

    if (NULL == str) {
        return JXTA_NOMEM;
    }

    strcpy(str, serviceName);

    if (NULL != serviceParam) {
        strcat(str, "/");
        strcat(str, serviceParam);
    }

    res = jxta_hashtable_del(pv->event_listener_table, str, str_len, NULL);

    free(str);

    return res;
}

static void *APR_THREAD_FUNC call_event_listeners_thread(apr_thread_t * thread, void *arg)
{
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_peerview_event *pv_event = NULL;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *lis = NULL;
    
    if (myself->state == PV_STOPPED) {
        return NULL;
    }

    apr_thread_mutex_lock(myself->mutex);
    
    res = jxta_vector_remove_object_at(myself->event_list, JXTA_OBJECT_PPTR(&pv_event), 0);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "No events in queue to send to listeners\n");
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    lis = jxta_hashtable_values_get(myself->event_listener_table);
    apr_thread_mutex_unlock(myself->mutex);

    if (lis != NULL) {
        unsigned int each_listener;
        unsigned int all_listener = jxta_vector_size(lis);

        for (each_listener = 0; each_listener < all_listener; each_listener++) {
            Jxta_listener *listener = NULL;

            res = jxta_vector_get_object_at(lis, JXTA_OBJECT_PPTR(&listener), each_listener);

            if (res == JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Calling Peerview listener [%pp].\n", listener);
                jxta_listener_process_object(listener, (Jxta_object *) pv_event);
                JXTA_OBJECT_RELEASE(listener);
            }
        }

        JXTA_OBJECT_RELEASE(lis);
    }

    JXTA_OBJECT_RELEASE(pv_event);
    
    return NULL;
}

static Jxta_status peerview_send_address_request(Jxta_peerview * myself, Jxta_peer * dest)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_PA *pa;
    Jxta_peerview_address_request_msg *addr_req;
    JString *addr_req_xml = NULL;
    apr_uuid_t adv_gen;
    char *temp;

    jxta_PG_get_PA(myself->group, &pa);

    addr_req = jxta_peerview_address_request_msg_new();

    temp = BN_bn2hex(myself->instance_mask);
    jxta_peerview_address_request_msg_set_instance_mask(addr_req, temp);
    free(temp);

    if (myself->self_pve) {
        temp = BN_bn2hex(myself->self_pve->target_hash);
        jxta_peerview_address_request_msg_set_current_target_hash(addr_req, temp);
        free(temp);

        temp = BN_bn2hex(myself->self_pve->target_hash_radius);
        jxta_peerview_address_request_msg_set_current_target_hash_radius(addr_req, temp);
        free(temp);
    }

    jxta_peerview_address_request_msg_set_peer_adv(addr_req, pa);

    if (jxta_PA_get_SN(pa, &adv_gen)) {
        jxta_peerview_address_request_msg_set_peer_adv_gen(addr_req, &adv_gen);
    }
    JXTA_OBJECT_RELEASE(pa);

    jxta_peerview_address_request_msg_set_peer_adv_exp(addr_req, jxta_RdvConfig_pv_entry_expires(myself->rdvConfig));

    res = jxta_peerview_address_request_msg_get_xml(addr_req, &addr_req_xml);
    JXTA_OBJECT_RELEASE(addr_req);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate address request xml. [%pp]\n", myself);
        return res;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_ADDRESS_REQUEST_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(addr_req_xml), jstring_length(addr_req_xml), NULL);
    JXTA_OBJECT_RELEASE(addr_req_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean pv_id_only)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_peerview_ping_msg *ping;
    JString *ping_xml = NULL;

    ping = jxta_peerview_ping_msg_new();

    jxta_peerview_ping_msg_set_src_peer_id(ping, myself->pid);

    if (jxta_peer_peerid(dest)) {
        jxta_peerview_ping_msg_set_dst_peer_id(ping, jxta_peer_peerid(dest));
    } else {
        assert(jxta_peer_address(dest));
        jxta_peerview_ping_msg_set_dst_peer_ea(ping, jxta_peer_address(dest));
    }

    jxta_peerview_ping_msg_set_pv_id_only(ping, pv_id_only);

    jxta_peerview_ping_msg_set_dest_peer_adv_gen(ping, adv_gen);

    res = jxta_peerview_ping_msg_get_xml(ping, &ping_xml);
    JXTA_OBJECT_RELEASE(ping);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate ping xml. [%pp]\n", myself);
        return res;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(ping_xml), jstring_length(ping_xml), NULL);
    JXTA_OBJECT_RELEASE(ping_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_send_pong(Jxta_peerview * myself, Jxta_peer * dest, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_boolean compact)
{
    Jxta_status res;
    Jxta_peerview_pong_msg *pong = NULL;
    JString *pong_msg_xml;
    Jxta_message *msg;
    Jxta_message_element *el = NULL;
    Jxta_PA * pa;
    char *tmp;
    unsigned int i;
    Jxta_vector *peers=NULL;
    apr_uuid_t adv_gen;
    Jxta_rdv_lease_options *rdv_options=NULL;

    apr_thread_mutex_lock(myself->mutex);

    if ((NULL == myself->instance_mask) || (NULL == myself->self_pve)
        || (0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Can't send pong message now! [%pp]\n", myself);
        res = JXTA_FAILED;
        apr_thread_mutex_unlock(myself->mutex);
        return res;
    }

    pong = jxta_peerview_pong_msg_new();

    if (myself->pv_id_gen_set) {
        jxta_peerview_pong_msg_set_pv_id_gen(pong, &myself->pv_id_gen);
    }
    jxta_peerview_pong_msg_set_action(pong, action);

    jxta_peerview_pong_msg_set_peer_id(pong, myself->pid);

    tmp = BN_bn2hex(myself->instance_mask);
    jxta_peerview_pong_msg_set_instance_mask(pong, tmp);
    free(tmp);

    tmp = BN_bn2hex(myself->self_pve->target_hash);
    jxta_peerview_pong_msg_set_target_hash(pong, tmp);
    free(tmp);

    tmp = BN_bn2hex(myself->self_pve->target_hash_radius);
    jxta_peerview_pong_msg_set_target_hash_radius(pong, tmp);
    free(tmp);

    jxta_peerview_pong_msg_set_rendezvous(pong, jxta_rdv_service_is_rendezvous(myself->rdv));
    jxta_peerview_pong_msg_set_is_demoting(pong, jxta_rdv_service_is_demoting(myself->rdv));

    pa = jxta_peer_adv((Jxta_peer *) myself->self_pve);
    jxta_peerview_pong_msg_set_peer_adv(pong, pa);

    if (jxta_PA_get_SN(pa, &adv_gen)) {
        jxta_peerview_pong_msg_set_peer_adv_gen(pong, &adv_gen);
    }
    rdv_options = jxta_rdv_lease_options_new();
    if (NULL != rdv_options) {

        /* XXX 20070913 These could be better tuned. */
        jxta_rdv_lease_options_set_suitability(rdv_options, (myself->my_suitability ));
        jxta_rdv_lease_options_set_willingness(rdv_options, myself->my_willingness);
        jxta_peerview_pong_msg_add_option(pong, (Jxta_advertisement *) rdv_options);
        JXTA_OBJECT_RELEASE(rdv_options);
    }

    jxta_peerview_pong_msg_set_peer_adv_exp(pong, jxta_RdvConfig_pv_entry_expires(myself->rdvConfig));

    if (candidates_only) {
        for (i = 0; i < jxta_vector_size(myself->activity_add_candidate_peers); i++) {
            Jxta_peer * client;
            res = jxta_vector_get_object_at(myself->activity_add_candidate_peers, JXTA_OBJECT_PPTR(&client), i);
            if (JXTA_SUCCESS == res) {
                jxta_pong_msg_add_candidate_info(pong, client);
                JXTA_OBJECT_RELEASE(client);
            }
        }
    }

    res =  jxta_peerview_get_localview(myself, &peers);
    for (i = 0; NULL != peers && i < jxta_vector_size(peers); ++i) {
        Jxta_id * peerId=NULL;
        Jxta_peer * peer=NULL;
        Peerview_entry *pve = NULL;
        char *target_hash = NULL;
        char *target_hash_radius = NULL;
        JString *jPeerid=NULL;

        res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
        if (JXTA_SUCCESS != res) continue;

        jxta_peer_get_peerid(peer, &peerId);
        pve = peerview_get_pve(myself, peerId);
        if (jxta_id_equals(peerId, myself->pid) || NULL == pve) {
            JXTA_OBJECT_RELEASE(peerId);
            JXTA_OBJECT_RELEASE(peer);
            if (NULL != pve)
                JXTA_OBJECT_RELEASE(pve);
            continue; 
        }
        jxta_id_to_jstring(peerId, &jPeerid);
        target_hash = BN_bn2hex(pve->target_hash);
        target_hash_radius = BN_bn2hex(pve->target_hash_radius);
        if (NULL != jxta_peer_adv((Jxta_peer *) pve)) {
            if (jxta_PA_get_SN(jxta_peer_adv((Jxta_peer *) pve), &adv_gen)) {
                jxta_pong_msg_add_partner_info(pong, peerId, &adv_gen, target_hash, target_hash_radius);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "There's no PA for %s\n", jstring_get_string(jPeerid));
        }
        if (jPeerid)
            JXTA_OBJECT_RELEASE(jPeerid);
        free(target_hash);
        free(target_hash_radius);
        JXTA_OBJECT_RELEASE(peer);
        JXTA_OBJECT_RELEASE(peerId);
        JXTA_OBJECT_RELEASE(pve);
    }
    JXTA_OBJECT_RELEASE(peers);

    res = jxta_peerview_get_globalview(myself, &peers);

    for (i = 0; NULL != peers && i < jxta_vector_size(peers); ++i) {
        Jxta_id * peerId=NULL;
        Jxta_peer * peer=NULL;
        Peerview_entry *pve = NULL;
        char *target_hash = NULL;
        char *target_hash_radius = NULL;
        JString *jPeerid=NULL;

        res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
        if (JXTA_SUCCESS != res) continue;
        jxta_peer_get_peerid(peer, &peerId);
        pve = peerview_get_pve(myself, peerId);
        if (jxta_id_equals(peerId, myself->pid) || NULL == pve) {
            JXTA_OBJECT_RELEASE(peerId);
            JXTA_OBJECT_RELEASE(peer);
            if (NULL != pve)
                JXTA_OBJECT_RELEASE(pve);
            continue; 
        }

        jxta_id_to_jstring(peerId, &jPeerid);
        target_hash = BN_bn2hex(pve->target_hash);
        target_hash_radius = BN_bn2hex(pve->target_hash_radius);
        if (NULL != jxta_peer_adv((Jxta_peer *) pve)) {
            if (jxta_PA_get_SN(jxta_peer_adv((Jxta_peer *) pve), &adv_gen)) {
                jxta_pong_msg_add_associate_info(pong, peerId, &adv_gen, target_hash, target_hash_radius);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "There's no PA for %s\n", jstring_get_string(jPeerid));
        }
        if (jPeerid)
            JXTA_OBJECT_RELEASE(jPeerid);
        free(target_hash);
        free(target_hash_radius);
        JXTA_OBJECT_RELEASE(peer);
        JXTA_OBJECT_RELEASE(peerId);
        JXTA_OBJECT_RELEASE(pve);
    }
    JXTA_OBJECT_RELEASE(peers);

    apr_thread_mutex_unlock(myself->mutex);

    if (!compact) {
        res = jxta_peerview_pong_msg_get_xml(pong, &pong_msg_xml);
    } else {
        res = jxta_peerview_pong_msg_get_xml_1(pong, &pong_msg_xml);
    }
    JXTA_OBJECT_RELEASE(pong);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to generate pong msg xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PONG_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(pong_msg_xml), jstring_length(pong_msg_xml), NULL);
    JXTA_OBJECT_RELEASE(pong_msg_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);

  FINAL_EXIT:

    return res;
}

static Jxta_status peerview_send_address_assign(Jxta_peerview * myself, Jxta_peer * dest, BIGNUM * target_hash)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_peerview_address_assign_msg *addr_assign;
    JString *addr_assign_xml = NULL;
    char *temp;

    addr_assign = jxta_peerview_address_assign_msg_new();

    jxta_peerview_address_assign_msg_set_peer_id(addr_assign, myself->pid);

    temp = BN_bn2hex(myself->instance_mask);
    jxta_peerview_address_assign_msg_set_instance_mask(addr_assign, temp);
    free(temp);

    temp = BN_bn2hex(target_hash);
    jxta_peerview_address_assign_msg_set_target_hash(addr_assign, temp);
    free(temp);

    res = jxta_peerview_address_assign_msg_get_xml(addr_assign, &addr_assign_xml);
    JXTA_OBJECT_RELEASE(addr_assign);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate address assign xml. [%pp]\n", myself);
        return res;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_ADDRESS_ASSIGN_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(addr_assign_xml), jstring_length(addr_assign_xml), NULL);
    JXTA_OBJECT_RELEASE(addr_assign_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_send_adv_request(Jxta_peerview * myself, Jxta_id * dest_id, Jxta_vector *need_pids)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_PA *pa;
    JString *adv_req_xml = NULL;
    Jxta_id *pid;
    JString *query_string = NULL;
    Jxta_adv_request_msg *adv_req_msg = NULL;
    unsigned int i;
    Jxta_peer * dest;
    adv_req_msg = jxta_adv_request_msg_new();
    query_string = jstring_new_0();
    jstring_append_2(query_string, "/jxta:PA[");
    for (i=0; i<jxta_vector_size(need_pids); i++) {
        JString *jPeerid;
        Jxta_id *id;
        res = jxta_vector_get_object_at(need_pids, JXTA_OBJECT_PPTR(&id), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,"Unable to retrieve object from need_pids : %d\n", res);
            continue;
        }
        if (i > 0) {
            jstring_append_2(query_string, " or ");
        }
        jxta_id_to_jstring(id, &jPeerid);
        jstring_append_2(query_string, "PID='");
        jstring_append_1(query_string, jPeerid);
        jstring_append_2(query_string, "'");
        JXTA_OBJECT_RELEASE(jPeerid);
        JXTA_OBJECT_RELEASE(id);
    }
    jstring_append_2(query_string, "]");
    dest = jxta_peer_new();
    jxta_peer_set_peerid(dest, dest_id);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"Sending a search request with query - %s\n", jstring_get_string(query_string));
    res = jxta_adv_request_msg_set_query(adv_req_msg, jstring_get_string(query_string), 100);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to add query to search request. %s\n", jstring_get_string(query_string));
    }

    if (NULL != query_string)
        JXTA_OBJECT_RELEASE(query_string);


    jxta_PG_get_PA(myself->group, &pa);
    pid = jxta_PA_get_PID(pa);
    jxta_adv_request_msg_set_src_peer_id(adv_req_msg, pid);
    JXTA_OBJECT_RELEASE(pid);
    JXTA_OBJECT_RELEASE(pa);
    res = jxta_adv_request_msg_get_xml(adv_req_msg, &adv_req_xml);
    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(adv_req_msg);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate PA request xml. [%pp]\n", myself);
        return res;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_ADV_REQUEST_ELEMENT_NAME, "text/xml",
                                        jstring_get_string(adv_req_xml), jstring_length(adv_req_xml), NULL);
    JXTA_OBJECT_RELEASE(adv_req_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(adv_req_msg);
    JXTA_OBJECT_RELEASE(dest);
    return res;
}

static Jxta_status peerview_send_adv_response(Jxta_peerview * myself, Jxta_peer * dest, Jxta_adv_response_msg *adv_rsp_msg)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    JString *adv_rsp_xml = NULL;

    res = jxta_adv_response_msg_get_xml(adv_rsp_msg, &adv_rsp_xml);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate PA response xml. [%pp]\n", myself);
        return res;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_ADV_RESPONSE_ELEMENT_NAME, "text/xml",
                                        jstring_get_string(adv_rsp_xml), jstring_length(adv_rsp_xml), NULL);
    JXTA_OBJECT_RELEASE(adv_rsp_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_send_pvm(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_id *peerid;
    Jxta_endpoint_address *dest_addr;
    Jxta_endpoint_address *peer_addr;
    char *addr_str;
    JString *pid_jstr;
    Jxta_PA *peer_adv=NULL;
    jxta_peer_get_adv(dest, &peer_adv);

    /* XXX 20060925 bondolo Publish the peer advertisement? */
    if (NULL != peer_adv) {
        res = discovery_service_publish(me->discovery, (Jxta_advertisement *) peer_adv , DISC_PEER, 5L * 60L * 1000L, LOCAL_ONLY_EXPIRATION);
    }
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to publish peer advertisement\n");
    }
    peerid = jxta_peer_peerid(dest);
    if (peerid) {
        jxta_id_to_jstring(peerid, &pid_jstr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending peerview message[%pp] by peerid to : %s\n", msg,
                        jstring_get_string(pid_jstr));
        JXTA_OBJECT_RELEASE(pid_jstr);
        res = jxta_PG_async_send(me->group, msg, peerid, RDV_V3_MSID, JXTA_PEERVIEW_NAME);
    } else {
        peer_addr = jxta_peer_address(dest);
        res = jxta_PG_get_recipient_addr(me->group, jxta_endpoint_address_get_protocol_name(peer_addr),
                                         jxta_endpoint_address_get_protocol_address(peer_addr),
                                         RDV_V3_MSID, JXTA_PEERVIEW_NAME, &dest_addr);
        if (JXTA_SUCCESS == res) {
            addr_str = jxta_endpoint_address_to_string(dest_addr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending peerview message [%pp] to : %s\n", msg, addr_str);
            free(addr_str);
            res = jxta_endpoint_service_send_ex(me->endpoint, msg, dest_addr, JXTA_FALSE);
            JXTA_OBJECT_RELEASE(dest_addr);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not build dest address for [%pp]\n", dest);
            res = JXTA_UNREACHABLE_DEST;
        }
    }
    if (peer_adv)
        JXTA_OBJECT_RELEASE(peer_adv);
    return res;
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Peerview Service
 ** protocol. 
 **
 **   - Receive a peerview response when searching for a subgroup RDV
 **/
static Jxta_status JXTA_STDCALL peerview_protocol_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    enum { ADDR_REQ, ADDR_ASSIGN, PING, PONG, ADV_REQ, ADV_RESP } message_type;
    Jxta_bytevector *value;
    JString *string;

    if(myself->state == PV_STOPPED)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Recieved a callback after PV has stopped\n");
        return JXTA_FAILED;
    }

   apr_thread_mutex_lock(myself->mutex);
   myself->processing_callbacks++;
   apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peerview received a message[%pp]\n", msg);

    message_type = ADDR_REQ;
    res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_ADDRESS_REQUEST_ELEMENT_NAME, &el);

    if (JXTA_ITEM_NOTFOUND == res) {
        message_type = ADDR_ASSIGN;
        res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_ADDRESS_ASSIGN_ELEMENT_NAME, &el);
    }

    if (JXTA_ITEM_NOTFOUND == res) {
        message_type = PING;
        res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, &el);
    }

    if (JXTA_ITEM_NOTFOUND == res) {
        message_type = PONG;
        res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PONG_ELEMENT_NAME, &el);
    }

    if (JXTA_ITEM_NOTFOUND == res) {
        message_type = ADV_REQ;
        res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_ADV_REQUEST_ELEMENT_NAME, &el);
    }

    if (JXTA_ITEM_NOTFOUND == res) {
        message_type = ADV_RESP;
        res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_ADV_RESPONSE_ELEMENT_NAME, &el);
    }

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    value = jxta_message_element_get_value(el);
    string = jstring_new_3(value);
    JXTA_OBJECT_RELEASE(value);

    switch (message_type) {
    case ADDR_REQ:{
            Jxta_peerview_address_request_msg *addr_request = jxta_peerview_address_request_msg_new();

            res =
                jxta_peerview_address_request_msg_parse_charbuffer(addr_request, jstring_get_string(string),
                                                                   jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_address_request(myself, addr_request);
            }

            JXTA_OBJECT_RELEASE(addr_request);
        }
        break;

    case ADDR_ASSIGN:{
            Jxta_peerview_address_assign_msg *addr_assign = jxta_peerview_address_assign_msg_new();

            res =
                jxta_peerview_address_assign_msg_parse_charbuffer(addr_assign, jstring_get_string(string),
                                                                  jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_address_assign(myself, addr_assign);
            }

            JXTA_OBJECT_RELEASE(addr_assign);
        }
        break;
 
    case PING:{
            Jxta_peerview_ping_msg *ping = jxta_peerview_ping_msg_new();

            res = jxta_peerview_ping_msg_parse_charbuffer(ping, jstring_get_string(string), jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_ping(myself, ping);
            }

            JXTA_OBJECT_RELEASE(ping);
        }
        break;

    case PONG:{
            Jxta_peerview_pong_msg *pong = jxta_peerview_pong_msg_new();

            res = jxta_peerview_pong_msg_parse_charbuffer(pong, jstring_get_string(string), jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_pong(myself, pong);
            }

            JXTA_OBJECT_RELEASE(pong);
        }
        break;

    case ADV_REQ:{
            Jxta_adv_request_msg *req = jxta_adv_request_msg_new();

            res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) req, jstring_get_string(string), jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_adv_request(myself, req);
            }

            JXTA_OBJECT_RELEASE(req);
        }
        break;

    case ADV_RESP:{
            Jxta_adv_response_msg *resp = jxta_adv_response_msg_new();

            res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *)resp, jstring_get_string(string), jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_adv_response(myself, resp);
            }

            JXTA_OBJECT_RELEASE(resp);
        }
        break;
    }

    JXTA_OBJECT_RELEASE(string);

  FINAL_EXIT:
    if (NULL != el) {
        JXTA_OBJECT_RELEASE(el);
    }

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed processing peerview message [%pp] %d\n", msg, res);
    }

   apr_thread_mutex_lock(myself->mutex);
   myself->processing_callbacks--;
   apr_thread_mutex_unlock(myself->mutex);

    return res;
}

static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req)
{
    Jxta_status res = JXTA_SUCCESS;
    volatile Jxta_boolean locked;
    unsigned int a_cluster = UINT_MAX;
    BIGNUM *target_address=NULL;
    Jxta_PA *dest_pa;
    Jxta_id *dest_id;
    Jxta_peer *dest;
    Jxta_boolean found_empty = FALSE;
    Peerview_entry *pve = NULL;

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    /* Make sure we are an active member of a peerview. */
    if ((NULL == myself->self_pve) || (0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        /* We are either inactive or moving to a new instance. */
         goto FINAL_EXIT;
    }

    dest = jxta_peer_new();
    dest_pa = jxta_peerview_address_request_msg_get_peer_adv(addr_req);
    jxta_peer_set_adv(dest, dest_pa);
    dest_id = jxta_PA_get_PID(dest_pa);
    pve = peerview_get_pve(myself, dest_id);

    res = JXTA_NOT_CONFIGURED;
    if (NULL != myself->addr_req_cb) {
        res = (myself->addr_req_cb)(dest, &a_cluster, &target_address);
        if (JXTA_NOT_CONFIGURED != res) {
            if (UINT_MAX != a_cluster) {
                if (a_cluster >= myself->clusters_count) {
                    res = JXTA_NOT_CONFIGURED;
                }
            }
            if (NULL != target_address) {
                /* this isn't implemented - free up the number */
                BN_free(target_address);
                target_address = NULL;
            }
            found_empty = TRUE;
        }
    }

    /* Inverse binary search. We look as far away from our cluster as possible */

    if (myself->clusters_count > 1 && NULL == pve && JXTA_NOT_CONFIGURED == res) {
        /* Assign within any cluster */
        unsigned int low_cluster;
        unsigned int mid_cluster;
        unsigned int high_cluster;

        low_cluster = 0;

        high_cluster = myself->clusters_count - 1;

        do {
            mid_cluster = ((high_cluster - low_cluster) / 2) + low_cluster;
 
            if ((high_cluster - myself->my_cluster) <= (myself->my_cluster - low_cluster)) {
                for (a_cluster = low_cluster; a_cluster <= mid_cluster; a_cluster++) {
                    if (0 == jxta_vector_size(myself->clusters[a_cluster].members)) {
                        found_empty = TRUE;
                        break;
                    }
                }

                if (!found_empty) {
                    low_cluster = mid_cluster + 1;
                }
            } else {
                for (a_cluster = high_cluster; a_cluster > mid_cluster; a_cluster--) {
                    if (0 == jxta_vector_size(myself->clusters[a_cluster].members)) {
                        found_empty = TRUE;
                        break;
                    }
                }

                if (!found_empty) {
                    high_cluster = mid_cluster;
                }
            }

            if ((high_cluster == low_cluster) && (high_cluster == myself->my_cluster)) {
                break;
            }
        } while (!found_empty);
    }

    if (NULL != pve) {
        target_address = BN_dup(pve->target_hash);
        JXTA_OBJECT_RELEASE(pve);
    } else {
        target_address = BN_new();
        if (found_empty) {
            BN_pseudo_rand_range(target_address, myself->cluster_divisor);
            BN_add(target_address, target_address, myself->clusters[a_cluster].min);
        } else {
            /* We need to choose an address within our cluster */
            /* XXX 20060925 bondolo Do something stociastic with histogram. */
            BN_pseudo_rand_range(target_address, myself->cluster_divisor);
            BN_add(target_address, target_address, myself->clusters[myself->my_cluster].min);
        }
    }

    jxta_peer_set_peerid(dest, dest_id);
    JXTA_OBJECT_RELEASE(dest_pa);
    JXTA_OBJECT_RELEASE(dest_id);
    jxta_peer_set_expires(dest, jpr_time_now() + jxta_peerview_address_request_msg_get_peer_adv_exp(addr_req));

    locked = FALSE;

    apr_thread_mutex_unlock(myself->mutex);

    res = peerview_send_address_assign(myself, dest, target_address);
    BN_free(target_address);

    JXTA_OBJECT_RELEASE(dest);

  FINAL_EXIT:
    if (locked) {
        /* We may have unlocked early to send a message. */
        apr_thread_mutex_unlock(myself->mutex);
    }

    return res;
}

static Jxta_status peerview_handle_address_assign(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign)
{
    Jxta_status res = JXTA_SUCCESS;
    BIGNUM *response_instance_mask = NULL;
    BIGNUM *target_hash = NULL;
    Jxta_boolean same;

    apr_thread_mutex_lock(myself->mutex);

    if (NULL == myself->instance_mask) {
        /* This has to be an address assignment we never asked for. Ignore it. */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unexpected address assignement from unknown peerview. [%pp]\n",
                        myself);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    if ((NULL != myself->self_pve) && (0 != jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        /* If we are already part of a peerview then disregard all new address assignments */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unneeded address assignement. We already have one. [%pp]\n", myself);
        res = JXTA_BUSY;
        goto FINAL_EXIT;
    }

    if (NULL != myself->self_pve) {

        BN_hex2bn(&response_instance_mask, jxta_peerview_address_assign_msg_get_instance_mask(addr_assign));

        same = 0 == BN_cmp(myself->instance_mask, response_instance_mask);
        BN_free(response_instance_mask);

        if (!same) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Address assignement from unexpected peerview instance. [%pp]\n",
                            myself);
            res = JXTA_FAILED;
            goto FINAL_EXIT;
        }
    }
    else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Address has not been assigned yet, accepting new assignment in "
            "peerview [%pp]\n", myself);
    }

    BN_hex2bn(&target_hash, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));

    assert(PV_ADDRESSING == myself->state);
    res = create_self_pve(myself, target_hash);
    myself->state = PV_ANNOUNCING;

    BN_free(target_hash);

    if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_announce, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, myself)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate announce activity. [%pp]\n", myself);
    }
  FINAL_EXIT:

    apr_thread_mutex_unlock(myself->mutex);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Joined peerview {%s} @ address {%s}\n",
                        jxta_peerview_address_assign_msg_get_instance_mask(addr_assign),
                        jxta_peerview_address_assign_msg_get_target_hash(addr_assign));
    }

    return res;
}

static Jxta_status peerview_handle_ping(Jxta_peerview * myself, Jxta_peerview_ping_msg * ping)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_id *pid=NULL;
    Jxta_peer *dest=NULL;
    Jxta_endpoint_address *dst_ea;
    Peerview_entry *pve=NULL;

    /* FIXME 20060926 bondolo Valdiate credential on ping message */
    dst_ea = jxta_peerview_ping_msg_get_dst_peer_ea(ping);
    if (dst_ea) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "[%pp] gets ping message with endpoint address: %s://%s\n", myself,
                        jxta_endpoint_address_get_protocol_name(dst_ea), jxta_endpoint_address_get_protocol_address(dst_ea));
        JXTA_OBJECT_RELEASE(dst_ea);
    } else {
        pid = jxta_peerview_ping_msg_get_dst_peer_id(ping);
        assert(!jxta_id_equals(pid, jxta_id_nullID));
        if (!jxta_id_equals(myself->pid, pid)) {
            /* It's not for me! Just ignore it. */
            goto FINAL_EXIT;
        }
        if (NULL != pid)
            JXTA_OBJECT_RELEASE(pid);
    }

    pid = jxta_peerview_ping_msg_get_src_peer_id(ping);

    if (jxta_id_equals(myself->pid, pid)) {
        /* It's me! Just ignore it. */
        goto FINAL_EXIT;
    }

    pve = peerview_get_pve(myself, pid);

    JString* pid_j;

    jxta_id_to_jstring(pid, &pid_j);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Receive ping message from %s %s\n", jstring_get_string(pid_j), NULL == pve ? " no pve": " with pve");

    JXTA_OBJECT_RELEASE(pid_j);

    dest = jxta_peer_new();
    jxta_peer_set_peerid(dest, pid);

    res = peerview_send_pong(myself, dest, PONG_STATUS, FALSE, jxta_peerview_ping_msg_is_pv_id_only(ping));

    if (pve)
        JXTA_OBJECT_RELEASE(pve);
    if (dest)
        JXTA_OBJECT_RELEASE(dest);

  FINAL_EXIT:
    if (NULL != pid) {
        JXTA_OBJECT_RELEASE(pid);
    }

    return res;
}

static Jxta_status joining_peerview(Jxta_peerview * me, const char *pv_mask)
{
    Jxta_status res = JXTA_SUCCESS;

    /* This is the first pong response from a peerview member. Oh happy day, we have found a peerview. */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "[%pp] Found peerview instance : %s\n", me, pv_mask);

    if (NULL != me->activity_locate_seeds) {
        assert(PV_LOCATING == me->state || PV_PASSIVE == me->state);
        if (NULL != me->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(me->activity_locate_seeds);
        }
        me->activity_locate_seeds = NULL;
    }

    BN_hex2bn(&me->instance_mask, pv_mask);
    me->state = PV_ADDRESSING;

    if (APR_SUCCESS != apr_thread_pool_push(me->thread_pool, activity_peerview_addressing, me,
                                            APR_THREAD_TASK_PRIORITY_HIGH, me)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate addressing activity. [%pp]\n", me);
    }

    return res;
}

/* XXX: Switch to new peerview only depends on whether we have assigned address to other RDV seems to be weak. Would it cause 
 * two rdv give up for each other under multicast environment?
 */
static Jxta_boolean is_for_alternative(Jxta_peerview * me, const char *pv_mask)
{
    Jxta_boolean rv = FALSE;
    BIGNUM *instance_mask;
    int same;
    instance_mask = BN_new();
    BN_hex2bn(&instance_mask, pv_mask);

    same = BN_cmp(me->instance_mask, instance_mask);
    /* Instance masks are not the same. This response is from a different peerview instance. */
    if (same != 0) {
        rv = TRUE;
        if (same < 0) {
            /* Signify that we are giving up on this peerview instance in favour of the new one. */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "PV[%pp] Found peerview alternate instance : %s\n", me, pv_mask);

            BN_copy(me->instance_mask, instance_mask);
            peerview_clear_pves(me, FALSE);
            if (NULL != me->self_pve) {
                jxta_peer_set_expires((Jxta_peer *) me->self_pve, 0L);
            }
            me->state = PV_LOCATING;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "PV[%pp] Nonmatching instance mask found : %s\n", me, pv_mask);
        }
    }
    BN_free(instance_mask);
    return rv;
}

static Jxta_status create_pve_from_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong, Peerview_entry ** ppve)
{
    Jxta_id *pid = NULL;
    JString *pid_str;
    Jxta_PA *pa = NULL;
    apr_uuid_t *adv_gen;
    apr_uuid_t *pv_id_gen = NULL;
    Peerview_entry * pve;
    Jxta_vector *options;
    Jxta_status res;

    adv_gen = jxta_peerview_pong_msg_get_peer_adv_gen(pong);
    if (!adv_gen) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PV[%pp] Invalid PONG message, adv_gen required for adv.\n", me);
        *ppve = NULL;
        return JXTA_INVALID_ARGUMENT;
    }
    pv_id_gen = jxta_peerview_pong_msg_get_pv_id_gen(pong);

    pid = jxta_peerview_pong_msg_get_peer_id(pong);
    assert(pid);

    pa = jxta_peerview_pong_msg_get_peer_adv(pong);
    assert(pa);

    res = jxta_id_to_jstring(pid, &pid_str);
    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PV[%pp] Creating new PVE for : %s\n", me,
                        jstring_get_string(pid_str));
        JXTA_OBJECT_RELEASE(pid_str);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "PV[%pp] Failed generating string for id[%pp].\n", me,
                        pid);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PV[%pp] Creating new PVE for PID[%pp]\n", me, pid);
    }

    pve = peerview_entry_new(pid, NULL, pa, pv_id_gen);
    free(pv_id_gen);


    BN_hex2bn(&pve->target_hash, jxta_peerview_pong_msg_get_target_hash(pong));
    BN_hex2bn(&pve->target_hash_radius, jxta_peerview_pong_msg_get_target_hash_radius(pong));

    calculate_radius_bounds(me, pve);

    options = jxta_peerview_pong_msg_get_options(pong);
    if (NULL != options) {
        jxta_peer_set_options((Jxta_peer *) pve, options);
        JXTA_OBJECT_RELEASE(options);
    }

    pve->adv_exp = jxta_peerview_pong_msg_get_peer_adv_exp(pong);
    if (-1L == pve->adv_exp) {
        pve->adv_exp = jxta_RdvConfig_pv_entry_expires(me->rdvConfig);
    }

    jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
    pve->adv_gen_set = TRUE;
    memmove(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t));
    free(adv_gen);
    jxta_peer_set_adv((Jxta_peer *) pve, pa);
    JXTA_OBJECT_RELEASE(pa);
    JXTA_OBJECT_RELEASE(pid);

    *ppve = pve;
    return JXTA_SUCCESS;
}

static void calculate_radius_bounds(Jxta_peerview * pv, Peerview_entry * pve)
{
    int cluster;
    BIGNUM *peer_start = NULL;
    BIGNUM *peer_end = NULL;
    int each_segment;
    char *tmp;

    cluster = cluster_for_hash(pv, pve->target_hash);

    if (cluster != pv->my_cluster) return;

    pve->cluster = cluster;

    if (NULL != pve->radius_range_1_start) {
        BN_free(pve->radius_range_1_start);
        pve->radius_range_1_start = NULL;
    }
    if (NULL != pve->radius_range_1_end) {
        BN_free(pve->radius_range_1_end);
        pve->radius_range_1_end = NULL;
    }
    if (NULL != pve->radius_range_2_start) {
        BN_free(pve->radius_range_2_start);
        pve->radius_range_2_start = NULL;
    }
    if (NULL != pve->radius_range_2_end) {
        BN_free(pve->radius_range_2_end);
        pve->radius_range_2_end = NULL;
    }

    peer_start = BN_new();
    tmp = BN_bn2hex(pve->target_hash);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cluster: %d pve->target_hash:%s\n", cluster, tmp);
    free(tmp);
    tmp = BN_bn2hex(pve->target_hash_radius);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cluster: %d pve->target_hash_radius:%s\n", cluster, tmp);
    free(tmp);

    BN_sub(peer_start, pve->target_hash, pve->target_hash_radius);

    peer_end = BN_new();
    BN_add(peer_end, pve->target_hash, pve->target_hash_radius);

    tmp = BN_bn2hex(peer_start);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cluster: %d peer_start %s\n", cluster, tmp);
    free(tmp);
    tmp = BN_bn2hex(peer_end);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "cluster: %d peer_end %s\n", cluster, tmp);
    free(tmp);

    for (each_segment = 0; each_segment < 3; each_segment++) {
        BIGNUM *segment_start = NULL;
        BIGNUM *segment_end = NULL;

        if (0 == each_segment) {
            /* #0 - The area below minimum wrapped around to maximum */
            if (BN_cmp(peer_start, pv->clusters[cluster].min) >= 0) {
                /* nothing to do in this segment. */
                continue;
            }

            segment_start = BN_new();
            BN_sub(segment_start, pv->clusters[cluster].min, peer_start);
            BN_sub(segment_start, pv->clusters[cluster].max, segment_start);

            if (BN_cmp(segment_start, pv->clusters[cluster].min) <= 0) {
                /* The range is *really* big, keep it within bounds. */
                /* This is handled as part of segment #1 */
                BN_free(segment_start);
                continue;
            }

            segment_end = BN_dup(pv->clusters[cluster].max);
        } else if (1 == each_segment) {
            /* #1 - The area between minimum and maximum */
            if (BN_cmp(peer_start, pv->clusters[cluster].min) <= 0) {
                segment_start = BN_dup(pv->clusters[cluster].min);
            } else {
                segment_start = BN_dup(peer_start);
            }

            if (BN_cmp(peer_end, pv->clusters[cluster].max) >= 0) {
                segment_end = BN_dup(pv->clusters[cluster].max);
            } else {
                segment_end = BN_dup(peer_end);
            }
        } else if (2 == each_segment) {
            /* #2 - The area above maximum wrapped around to minimum */
            if (BN_cmp(peer_end, pv->clusters[cluster].max) <= 0) {
                /* Nothing to do for this segment. */
                continue;
            }

            segment_start = BN_dup(pv->clusters[cluster].min);

            segment_end = BN_new();
            BN_sub(segment_end, peer_end, pv->clusters[cluster].max);
            BN_add(segment_end, segment_end, pv->clusters[cluster].min);

            if (BN_cmp(segment_end, pv->clusters[cluster].max) >= 0) {
                /* The range is *really* big, keep it within bounds. */
                /* This is handled as part of segment #1 */
                BN_free(segment_end);
                continue;
            }
        }
        if (NULL != segment_start) {
            if (NULL == pve->radius_range_1_start) {
                pve->radius_range_1_start = BN_dup(segment_start);
            } else if (NULL == pve->radius_range_2_start) {
                pve->radius_range_2_start = BN_dup(segment_start);
            }
            BN_free(segment_start);
        }
        if (NULL != segment_end) {
            if (NULL == pve->radius_range_1_end) {
                pve->radius_range_1_end = BN_dup(segment_end);
            } else if (NULL == pve->radius_range_2_end) {
                pve->radius_range_2_end = BN_dup(segment_end);
            }
            BN_free(segment_end);
        }
    }
    if (pve->radius_range_1_start) {
        tmp = BN_bn2hex(pve->radius_range_1_start);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "pve->radius_range_1_start:%s \n", tmp);
        free(tmp);
    }
    if (pve->radius_range_1_end) {
        tmp = BN_bn2hex(pve->radius_range_1_end);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "pve->radius_range_1_end:%s \n", tmp);
        free(tmp);
    }
    if (pve->radius_range_2_start) {
        tmp = BN_bn2hex(pve->radius_range_2_start);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "pve->radius_range_2_start:%s \n", tmp);
        free(tmp);
    }
    if (pve->radius_range_2_end) {
        tmp = BN_bn2hex(pve->radius_range_2_end);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "pve->radius_range_2_end:%s \n", tmp);
        free(tmp);
    }

    BN_free(peer_start);
    BN_free(peer_end);

}

/* Process the associates and partners looking for unknown peers. */
static Jxta_status process_referrals(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_vector *referrals;
    Jxta_vector *associate_referrals;
    Jxta_vector *partner_referrals;
    Jxta_vector *need_pids=NULL;
    unsigned int all_referrals;
    unsigned int each_referral;
    Jxta_status res = JXTA_SUCCESS;

    /* XXX 20061009 bondolo This is much too aggressive. We don't need to 
       ping every unknown peer we learn of. We should ping only :
       - Every unknown peer in our cluster.
       - Unknown peers in other clusters if we are below the desired number of
       peers in that cluster. This gives us alternatives should our associate
       in the cluster fail.
     */
    associate_referrals = jxta_pong_msg_get_associate_infos(pong);
    partner_referrals = jxta_pong_msg_get_partner_infos(pong);
    referrals = jxta_vector_new(jxta_vector_size(associate_referrals) + jxta_vector_size(partner_referrals));
    jxta_vector_addall_objects_last(referrals, associate_referrals);
    jxta_vector_addall_objects_last(referrals, partner_referrals);
    JXTA_OBJECT_RELEASE(associate_referrals);
    JXTA_OBJECT_RELEASE(partner_referrals);

    all_referrals = jxta_vector_size(referrals);
    for (each_referral = 0; each_referral < all_referrals; each_referral++) {
        Jxta_peerview_peer_info *peer;
        Jxta_id *referral_pid;
        Jxta_id * id = NULL;
        Jxta_PA *jpa=NULL;
        JString *jPeerid=NULL;

        res = jxta_vector_get_object_at(referrals, JXTA_OBJECT_PPTR(&peer), each_referral);
        if (JXTA_SUCCESS != res) {
            continue;
        }
        referral_pid = jxta_peerview_peer_info_get_peer_id(peer);
        if (jxta_id_equals(referral_pid, me->pid)) {
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(referral_pid);
            continue;
        }
        jxta_id_to_jstring(referral_pid, &jPeerid); 
        res = jxta_peerview_peer_info_get_adv_gen_id(peer, &id);
        if (!peerview_check_pve(me, referral_pid)) {
            Jxta_peer *ping_target = NULL;
            Jxta_PA *peer_adv = NULL;

            ping_target = jxta_peer_new();
            peer_adv = jxta_peerview_peer_info_get_peer_adv(peer);
            jxta_peer_set_peerid(ping_target, referral_pid);
            jxta_peer_set_adv(ping_target, peer_adv);
            JXTA_OBJECT_RELEASE(peer_adv);

            /* Not in pve, see if we have a referral already. */

            if (!jxta_vector_contains (me->activity_maintain_referral_peers, (Jxta_object *) ping_target,
                                       (Jxta_object_equals_func) jxta_peer_equals)) {
                jxta_vector_add_object_last(me->activity_maintain_referral_peers, (Jxta_object *) ping_target);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,"Added referral: %s\n", jstring_get_string(jPeerid));
                if (FALSE == have_matching_PA(me, peer, &jpa)) {
                    if (NULL == need_pids) {
                        need_pids = jxta_vector_new(0);
                    }
                    jxta_vector_add_object_last(need_pids, (Jxta_object *) referral_pid);
                }
            }
            JXTA_OBJECT_RELEASE(ping_target);

        } else {
            if (id != NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,"Referral is duplicate: %s\n", jstring_get_string(jPeerid));
                if (FALSE == have_matching_PA(me, peer, &jpa)) {

                    if (NULL == need_pids) {
                        need_pids = jxta_vector_new(0);
                    }
                    jxta_vector_add_object_last(need_pids, (Jxta_object *) referral_pid);
                }
            }
        }

        if (jPeerid)
            JXTA_OBJECT_RELEASE(jPeerid);
        if (id)
            JXTA_OBJECT_RELEASE(id);
        if (jpa)
            JXTA_OBJECT_RELEASE(jpa);
        JXTA_OBJECT_RELEASE(referral_pid);
        JXTA_OBJECT_RELEASE(peer);
    }
    if (NULL != need_pids) {
        Jxta_id *dest_id = NULL;
        dest_id = jxta_peerview_pong_msg_get_peer_id(pong);
        peerview_send_adv_request(me, dest_id, need_pids);
        JXTA_OBJECT_RELEASE(need_pids);
        JXTA_OBJECT_RELEASE(dest_id);
    }
    JXTA_OBJECT_RELEASE(referrals);
    return JXTA_SUCCESS;
}

static Jxta_boolean peerview_handle_promotion(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_boolean rsp=FALSE;
    unsigned int i;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_id *pid=NULL;
    JString * jPeerid=NULL;
    Jxta_vector * candidates_rcvd=NULL;
    Jxta_boolean kickstart_activity_add=FALSE;

    pid = jxta_peerview_pong_msg_get_peer_id(pong);

    jxta_id_to_jstring(pid, &jPeerid);
    candidates_rcvd = jxta_pong_msg_get_candidate_infos(pong);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Handle promotion [%pp] from : %s with %d candidates\n", pong, jstring_get_string(jPeerid), jxta_vector_size(candidates_rcvd));

    apr_thread_mutex_lock(myself->mutex);

    /* if this peer started the voting and the polls are still open */
    if (myself->activity_add_voting
            && jxta_vector_size(myself->activity_add_candidate_voting_peers) > 0
            && myself->activity_add_candidate_requests_expiration > jpr_time_now()) {
        /* process the list */
        unsigned int referral_size = 0;
        unsigned int local_view_size = 0;
        for (i=0; i < jxta_vector_size(myself->activity_add_candidate_voting_peers); i++) {
            Jxta_peer * peer;
            /* only allow one response per peer */
            res = jxta_vector_get_object_at(myself->activity_add_candidate_voting_peers, JXTA_OBJECT_PPTR(&peer), i);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,"Unable to retrieve peer from voting peers [%pp]\n", myself->activity_add_candidate_voting_peers);
                continue;
            }
            if (jxta_id_equals(jxta_peer_peerid(peer), pid)) {
                jxta_vector_remove_object_at(myself->activity_add_candidate_voting_peers, NULL, i);
                JXTA_OBJECT_RELEASE(peer);
                break;
            }
            JXTA_OBJECT_RELEASE(peer);
        }

        referral_size = jxta_pong_msg_get_partners_size(pong);
        local_view_size = jxta_peerview_get_localview_size(myself) - 1;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Referral size: %d  Local view size: %d\n", referral_size, local_view_size);

        /* if there is a better candidate or referrals are available */
        if (jxta_vector_size(candidates_rcvd) > 0 || referral_size > local_view_size) {
            jxta_vector_clear(myself->activity_add_candidate_peers);
        } else if (0 == jxta_vector_size(myself->activity_add_candidate_voting_peers)) {
            kickstart_activity_add = TRUE;
        }

    } else if (!myself->activity_add_voting) {
        Jxta_vector * candidates = NULL;
        unsigned int num_candidates = 0;
        /* note the time that an election will end */
        me->activity_add_voting_ends = jpr_time_now() + jxta_RdvConfig_pv_voting_wait(myself->rdvConfig);

        /* Cannot hold the peerview mutex while making a call to get_best_rdv_candidates */
        apr_thread_mutex_unlock(myself->mutex);
        res = get_best_rdv_candidates(myself, &candidates, 1);
        apr_thread_mutex_lock(myself->mutex);

        rsp = TRUE;
        if (NULL != candidates) {
            num_candidates = jxta_vector_size(candidates);
            jxta_vector_clear(myself->activity_add_candidate_peers);
            for (i = 0; i < num_candidates; i++) {
                Jxta_peer *peer;
                res = jxta_vector_get_object_at(candidates, JXTA_OBJECT_PPTR(&peer), i);
                if (JXTA_SUCCESS != res) {
                    continue;
                }
                jxta_vector_add_object_last(myself->activity_add_candidate_peers, (Jxta_object *) peer);
                JXTA_OBJECT_RELEASE(peer);
            }
            JXTA_OBJECT_RELEASE(candidates);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Send promotion response with %d candidates to : %s\n", num_candidates, jstring_get_string(jPeerid));

    } else {
        kickstart_activity_add = TRUE;
        /* the polls are closed */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"----------------  Polls are closed - clear the voting peers \n");

        jxta_vector_clear(myself->activity_add_candidate_voting_peers);

    }
    if (kickstart_activity_add) {
        apr_status_t apr_res;
        apr_thread_pool_tasks_cancel(myself->thread_pool,&myself->activity_add);

        myself->activity_add_candidate_requests_expiration = jpr_time_now();
        myself->activity_add_voting_ends = jpr_time_now();

        apr_res = apr_thread_pool_push(myself->thread_pool, activity_peerview_add, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, &myself->activity_add);
        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "Handle promotion[%pp]: Could not start add activity.\n", pong);
            myself->activity_add = FALSE;
        } else {
            myself->activity_add = TRUE;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Handle promotion [%pp]: Started [Add].\n", pong);
        }

    }

    apr_thread_mutex_unlock(myself->mutex);
    if (NULL != pid)
        JXTA_OBJECT_RELEASE(pid);
    if (jPeerid)
        JXTA_OBJECT_RELEASE(jPeerid);
    if (candidates_rcvd)
        JXTA_OBJECT_RELEASE(candidates_rcvd);
    return rsp;
}

static Jxta_boolean is_more_recent_uuid(apr_uuid_t * a, apr_uuid_t *b)
{
    Jxta_boolean ret = FALSE;
    int cmp;
    cmp = jxta_id_uuid_compare(a, b);
    if (UUID_NOT_COMPARABLE == cmp) {
        ret = (UUID_LESS_THAN != jxta_id_uuid_time_stamp_compare(a, b)) ? TRUE:FALSE;
        if (TRUE == ret) {
            ret = (UUID_GREATER_THAN == jxta_id_uuid_seq_no_compare(a, b)) ? TRUE:FALSE;
        }
    } else {
        ret = (UUID_GREATER_THAN == cmp) ? TRUE:FALSE;
    }
    return ret;
}

static Jxta_boolean have_matching_PA(Jxta_peerview * me, Jxta_peerview_peer_info *peer, Jxta_PA ** requested_PA)
{
    Jxta_boolean ret = FALSE;
    Jxta_status status = JXTA_SUCCESS;
    JString *query=NULL;
    JString *jPeerid=NULL;
    Peerview_entry *pve=NULL;
    Jxta_PA *pa=NULL;
    apr_uuid_t * adv_gen_ptr;
    apr_uuid_t adv_gen;
    Jxta_vector * requested_PAs=NULL;
    Jxta_id *id = NULL;

    id = jxta_peerview_peer_info_get_peer_id(peer);
    adv_gen_ptr = jxta_peerview_peer_info_get_adv_gen(peer);
    if (NULL == adv_gen_ptr) goto FINAL_EXIT;
    memmove(&adv_gen, adv_gen_ptr, sizeof(apr_uuid_t));

    pve = peerview_get_pve(me, id);
    if (pve && pve->adv_gen_set) {
        ret = is_more_recent_uuid(&pve->adv_gen, &adv_gen);
        if (TRUE == ret) {
           jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Found an entry in the peerview\n");
           goto FINAL_EXIT;
        }
    }

    query = jstring_new_2("/jxta:PA[PID='");
    jxta_id_to_jstring(id, &jPeerid);
    jstring_append_1(query, jPeerid);
    jstring_append_2(query, "']");

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"Check for PA with query: %s\n", jstring_get_string(query));
    status = discovery_service_local_query(me->discovery, jstring_get_string(query), &requested_PAs);

    if (JXTA_SUCCESS != status || NULL == requested_PAs || jxta_vector_size(requested_PAs) < 1) {
        goto FINAL_EXIT;
    }

    if (jxta_vector_size(requested_PAs) > 1) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,"Not sure why we got more than 1 PA back: size is %d\n", jxta_vector_size(requested_PAs));
    }

    status = jxta_vector_get_object_at(requested_PAs, JXTA_OBJECT_PPTR(&pa), 0);
    if (JXTA_SUCCESS == status) {
        ret = jxta_PA_is_recent(pa, &adv_gen);
    }
FINAL_EXIT:
    if (id)
        JXTA_OBJECT_RELEASE(id);
    if (requested_PAs)
        JXTA_OBJECT_RELEASE(requested_PAs);
    if (jPeerid)
        JXTA_OBJECT_RELEASE(jPeerid);
    if (pa)
        *requested_PA = pa;
    if (query)
        JXTA_OBJECT_RELEASE(query);
    if (pve)
        JXTA_OBJECT_RELEASE(pve);
    return ret;
}

static Jxta_status handle_new_pve_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_id *pid = NULL;
    JString *pid_jstr;
    Jxta_PA *pa = NULL;
    Jxta_status res = JXTA_SUCCESS;
    Peerview_entry * pve;

    pa = jxta_peerview_pong_msg_get_peer_adv(pong);
    if (NULL == pa) {
        pid = jxta_peerview_pong_msg_get_peer_id(pong);
        assert(pid);
        res = jxta_id_to_jstring(pid, &pid_jstr);
        if (JXTA_SUCCESS == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            "No PVE found for shortform pong from %s. Possibly expired? [%pp]\n", 
                            jstring_get_string(pid_jstr), me);
            JXTA_OBJECT_RELEASE(pid_jstr);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "PV[%pp] Failed generating string for id[%pp]\n", 
                            me, pid);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            "PV[%pp] No PVE found for shortform pong from PID[%pp]. Possibly expired?\n",
                            me, pid);
        }
        JXTA_OBJECT_RELEASE(pid);
        return JXTA_INVALID_ARGUMENT;
    }
    JXTA_OBJECT_RELEASE(pa);
    /* only add rendezvous peers to the peerview when a rendezvous */
    if (jxta_peerview_pong_msg_is_rendezvous(pong) && jxta_rdv_service_is_rendezvous(me->rdv)) {
        res = create_pve_from_pong(me, pong, &pve);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed create peerview entry. [%pp]\n", me);
            return JXTA_FAILED;
        }

        res = peerview_add_pve(me, pve);
        JXTA_OBJECT_RELEASE(pve);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed adding entry to peerview. [%pp]\n", me);
            return JXTA_FAILED;
        }
    }
    return JXTA_SUCCESS;
}

static Jxta_status handle_existing_pve_pong(Jxta_peerview * me, Peerview_entry * pve, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_PA *pa = NULL;
    apr_uuid_t *adv_gen;
    apr_uuid_t *pv_id_gen;
    Jxta_vector *options;
    BIGNUM *num_b;
    int new_hash = 0;

    /* if the hash changed for this peer remove it */
    num_b = BN_new();
    BN_hex2bn(&num_b, jxta_peerview_pong_msg_get_target_hash(pong));
    new_hash = BN_cmp(pve->target_hash, num_b);
    BN_free(num_b);
    if (new_hash != 0) {
        jxta_peer_set_expires((Jxta_peer *) pve, 0);
        return JXTA_INVALID_ARGUMENT;
    }

    /* Update radius unconditionally */
    BN_hex2bn(&pve->target_hash_radius, jxta_peerview_pong_msg_get_target_hash_radius(pong));

    if (jxta_rdv_service_is_rendezvous(me->rdv) ) {
        jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
    }
    pa = jxta_peerview_pong_msg_get_peer_adv(pong);
    if (!pa) {
        return JXTA_SUCCESS;
    }

    /* If a PA is included, update the PA */
    adv_gen = jxta_peerview_pong_msg_get_peer_adv_gen(pong);
    if (!adv_gen) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PV[%pp] Invalid PONG message, adv_gen required for adv.\n", me);
        JXTA_OBJECT_RELEASE(pa);
        return JXTA_SUCCESS;
    }

    pv_id_gen = jxta_peerview_pong_msg_get_pv_id_gen(pong);
    if (NULL != pv_id_gen) {
        pve->pv_id_gen_set = TRUE;
        memmove(&pve->pv_id_gen, pv_id_gen, sizeof(apr_uuid_t));
        free(pv_id_gen);
    }
    pve->is_rendezvous = TRUE;
    if (!jxta_peerview_pong_msg_is_rendezvous(pong)) {
        /* peer has changed to an edge and needs to be removed from the peerview */
        pve->is_rendezvous = FALSE;
        pve->is_demoting = FALSE;
        jxta_peer_set_expires((Jxta_peer *) pve, 0);
    } else {
        pve->is_demoting = jxta_peerview_pong_msg_is_demoting(pong);
    }

    if (0 != memcmp(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t))) {
        /* Update the advertisement. */
        pve->adv_exp = jxta_peerview_pong_msg_get_peer_adv_exp(pong);

        if (-1L == pve->adv_exp) {
            pve->adv_exp = jxta_RdvConfig_pv_entry_expires(me->rdvConfig);
        }

        if ((NULL != me->self_pve) && (0 != jxta_peer_get_expires((Jxta_peer *) me->self_pve))) {
            /*FIXME: sould not do so if pve is new, not yet in our pve */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "sould not do so if pve is new, not yet in our pve\n");
            jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
        } else {
            /* We aren't in the peerview. Greatly foreshorten the expiration so that a pong msg will be sent by
               maintenance task. */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "We aren't in the peerview. Greatly foreshorten the expiration\n");
            jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + jxta_RdvConfig_pv_pong_due(me->rdvConfig));
        }

        memmove(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t));

        jxta_peer_set_adv((Jxta_peer *) pve, pa);
    }

    options = jxta_peerview_pong_msg_get_options(pong);
    if (NULL != options) {
        jxta_peer_set_options((Jxta_peer *) pve, options);
        JXTA_OBJECT_RELEASE(options);
    }

    free(adv_gen);
    if (pa)
        JXTA_OBJECT_RELEASE(pa);
    return res;
}

static Jxta_status peerview_handle_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_id *pid = NULL;
    Peerview_entry *pve = NULL;
    Jxta_boolean send_pong=FALSE;
    Jxta_boolean send_ping=FALSE;
    Jxta_boolean send_candidates=FALSE;
    Jxta_pong_msg_action action=PONG_STATUS;
    Jxta_pong_msg_rdv_state state;
    JString *pid_j;
    Jxta_boolean locked = FALSE;

    apr_thread_mutex_lock(me->mutex);
    locked = TRUE;

    pid = jxta_peerview_pong_msg_get_peer_id(pong);
    assert(pid);

    if (jxta_peerview_pong_msg_is_compact(pong)) {

        pve = peerview_get_pve(me, pid);

        if (NULL != pve) {
            apr_uuid_t * pv_id_gen=NULL;

           jxta_id_to_jstring(pid, &pid_j);

            pv_id_gen = jxta_peerview_pong_msg_get_pv_id_gen(pong);
            if (NULL == pv_id_gen) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received invalid PONG msg with no peerview adv_gen\n");
            } else if (is_more_recent_uuid(pv_id_gen, &pve->pv_id_gen)) {
                char tmp[64];
                char tmp1[64];
                send_ping= TRUE;
                apr_uuid_format(tmp, pv_id_gen);
                apr_uuid_format(tmp1, &pve->pv_id_gen);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received pv_id_gen:%s ---- previous pv_id_gen:%s %s\n", tmp, tmp1, jstring_get_string(pid_j));
            } else {
                jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
            }
            if (pv_id_gen)
                free(pv_id_gen);
            JXTA_OBJECT_RELEASE(pid_j);
        }
        goto UNLOCK_EXIT;
    }
    action = jxta_peerview_pong_msg_get_action(pong);

    state = jxta_peerview_pong_msg_get_state(pong);

    JString *msg_j = jstring_new_2("Received pong msg ");

    /* TODO 20060926 bondolo Valdiate credential on pong message */


    jxta_id_to_jstring(pid, &pid_j);
    jstring_append_2(msg_j, jstring_get_string(pid_j));
    JXTA_OBJECT_RELEASE(pid_j);

    if (jxta_id_equals(me->pid, pid)) {
        JXTA_OBJECT_RELEASE(pid);
        goto UNLOCK_EXIT;
    }

    jstring_append_2(msg_j, " state:");
    jstring_append_2(msg_j, jxta_peerview_pong_msg_state_text(pong));
    locked = TRUE;

    jstring_append_2(msg_j, " action:");
    jstring_append_2(msg_j, jxta_peerview_pong_msg_action_text(pong));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s \n", jstring_get_string(msg_j));
    JXTA_OBJECT_RELEASE(msg_j);

    if (!jxta_rdv_service_is_rendezvous(me->rdv)) {
        /* Only invite while addressing is allowed when an edge */
        if (RDV_STATE_RENDEZVOUS == state && PONG_INVITE != action && PV_ADDRESSING != me->state && PV_ANNOUNCING != me->state) {
            send_pong = TRUE;
            action = PONG_STATUS;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "-------------- SENDING EDGE STATE ---------- me->state: %s\n", state_description[me->state]);
            goto UNLOCK_EXIT;
        }
    }
    if (me->instance_mask) {
        assert(PV_MAINTENANCE == me->state || PV_ADDRESSING == me->state || PV_ANNOUNCING == me->state);
        if (is_for_alternative(me, jxta_peerview_pong_msg_get_instance_mask(pong))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is for alternative\n", pong);
            if (PV_LOCATING == me->state) {
                /* retrieve an address within this instance */
                res = joining_peerview(me, jxta_peerview_pong_msg_get_instance_mask(pong));
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is joining alternate peerview\n", pong);
                assert(PV_ADDRESSING == me->state);
            } else {
                send_pong = TRUE;
                action = PONG_INVITE;
                goto UNLOCK_EXIT;
            }
        }
    } else {
        assert(PV_LOCATING == me->state || PV_PASSIVE == me->state);
        res = joining_peerview(me, jxta_peerview_pong_msg_get_instance_mask(pong));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is joining peerview\n", pong);
        assert(PV_ADDRESSING == me->state);
    }

    pve = peerview_get_pve(me, pid);

    if (NULL == pve) {
        if (!jxta_rdv_service_is_rendezvous(me->rdv) && PONG_INVITE == action) {
            me->state = PV_LOCATING;
            /* must unlock mutex while calling rdv_service functions that lock rdv mutex */
            apr_thread_mutex_unlock(me->mutex);
            rdv_service_switch_config(me->rdv, config_rendezvous);
            apr_thread_mutex_lock(me->mutex);
            if (me->self_pve) {
                jxta_peer_set_expires((Jxta_peer *) me->self_pve, 0);
            }
            res = joining_peerview(me, jxta_peerview_pong_msg_get_instance_mask(pong));
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is joining peerview after INVITE\n", pong);
            assert(PV_ADDRESSING == me->state);
        }
        if (jxta_rdv_service_is_rendezvous(me->rdv)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is being handled as new\n", pong);
            res = handle_new_pve_pong(me, pong);
            if (JXTA_SUCCESS == res) {
                send_pong = TRUE;
                if (me->state == PV_MAINTENANCE && state == RDV_STATE_RENDEZVOUS) {
                    action = PONG_INVITE;
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG announcement\n");
                } else {
                    send_pong = FALSE;
                }
            }
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PONG [%pp] is being handled as existing pong\n", pong);
        res = handle_existing_pve_pong(me, pve, pong);
        if (PONG_PROMOTE == action && JXTA_SUCCESS == res) {
            apr_thread_mutex_unlock(me->mutex);
            locked = FALSE;
            send_pong = peerview_handle_promotion(me, pong);
            send_candidates = TRUE;
        }
    }

    if (JXTA_SUCCESS == res) {
        process_referrals(me, pong);
    }

UNLOCK_EXIT:

    if (send_pong || send_ping) {
        Jxta_peer * newPeer = jxta_peer_new();
        jxta_peer_set_peerid(newPeer, pid);

        if (send_pong) {
            res = peerview_send_pong(me, newPeer, action, send_candidates, FALSE);
        }
        if (send_ping) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "------------- Sending ping for updated pv_id_gen\n");
            res = peerview_send_ping(me, newPeer, NULL, FALSE);
        }
        JXTA_OBJECT_RELEASE(newPeer);
    }
    if (locked)
        apr_thread_mutex_unlock(me->mutex);

    if (pve)
        JXTA_OBJECT_RELEASE(pve);
    if (pid)
        JXTA_OBJECT_RELEASE(pid);
    return res;
}

static Jxta_status peerview_handle_adv_request(Jxta_peerview * me, Jxta_adv_request_msg * req)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector * requested_advs = NULL;
    Jxta_adv_response_msg * adv_rsp_msg = NULL;
    unsigned int i;
    char *query = NULL;
    res = jxta_adv_request_msg_get_query(req, &query);
    if (JXTA_SUCCESS != res) {
        return res;
    }
    discovery_service_local_query(me->discovery, query, &requested_advs);
    for (i=0; i< jxta_vector_size(requested_advs); i++) {
        Jxta_advertisement * adv = NULL;

        if (JXTA_SUCCESS != jxta_vector_get_object_at(requested_advs, JXTA_OBJECT_PPTR(&adv), i)) {
            continue;
        }
        if (NULL == adv_rsp_msg) {
            adv_rsp_msg = jxta_adv_response_msg_new();
        }
        jxta_adv_response_msg_add_advertisement(adv_rsp_msg, adv, 5 * 60 * 1000);
        JXTA_OBJECT_RELEASE(adv);
    }
    if (NULL != adv_rsp_msg) {
        Jxta_peer * dest;
        Jxta_id * pid;

        pid = jxta_adv_request_msg_get_src_peer_id(req);
        dest = jxta_peer_new();
        jxta_peer_set_peerid(dest, pid);
        peerview_send_adv_response(me, dest, adv_rsp_msg);

        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(pid);
    }
    if (requested_advs)
        JXTA_OBJECT_RELEASE(requested_advs);
    if (query)
        free(query);
    if (adv_rsp_msg)
        JXTA_OBJECT_RELEASE(adv_rsp_msg);
    return res;
}

static Jxta_status peerview_handle_adv_response(Jxta_peerview * me, Jxta_adv_response_msg * resp)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector * entries;
    unsigned int i;
    Peerview_entry *pve = NULL;

    res = jxta_adv_response_get_entries(resp, &entries);
    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }
    for (i=0; i<jxta_vector_size(entries); i++) {
        Jxta_adv_response_entry *entry=NULL;
        Jxta_PA * adv = NULL;
        Jxta_id * id = NULL;
        res = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error retrieving entry from search response vector: %d\n", res);
            continue;
        }
        adv = (Jxta_PA *) jxta_adv_response_entry_get_adv(entry);
        res = discovery_service_publish(me->discovery, (Jxta_advertisement *) adv , DISC_PEER, jxta_adv_response_entry_get_expiration(entry), LOCAL_ONLY_EXPIRATION);
        if (JXTA_SUCCESS != res) {
            JXTA_OBJECT_RELEASE(entry);
            JXTA_OBJECT_RELEASE(adv);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error publishing PA from search response: %d\n", res);
            continue;
        }
        id = jxta_PA_get_PID(adv);
        pve = peerview_get_pve(me, id);
        if (NULL != pve) {
            pve->adv_gen_set = jxta_PA_get_SN(adv, &pve->adv_gen);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "changed the adv_gen after response received\n");
            JXTA_OBJECT_RELEASE(pve);
            pve = NULL;
        }
        if (entry)
            JXTA_OBJECT_RELEASE(entry);
        if (adv)
            JXTA_OBJECT_RELEASE(adv);
        if (id)
            JXTA_OBJECT_RELEASE(id);
    }
FINAL_EXIT:
    if (entries)
        JXTA_OBJECT_RELEASE(entries);
    return res;
}

/**
*   Return the peerview entry for the specified peer id.
*
*   @param myself The peerview.
*   @param pid The peer id of the PVE being sought.
*   @return The resulting pve or NULL if no matching PVE was found.
**/
static Peerview_entry *peerview_get_pve(Jxta_peerview * myself, Jxta_PID * pid)
{
    Jxta_status res;
    JString *pid_str;
    Peerview_entry *pve;

    JXTA_OBJECT_CHECK_VALID(pid);

    res = jxta_id_to_jstring(pid, &pid_str);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed generating string for id. [%pp]\n", myself);
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Getting PVE for %s\n", jstring_get_string(pid_str));

    apr_thread_mutex_lock(myself->mutex);

    res = jxta_hashtable_get(myself->pves, jstring_get_string(pid_str), jstring_length(pid_str) + 1, JXTA_OBJECT_PPTR(&pve));

    apr_thread_mutex_unlock(myself->mutex);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "No pve for %s [%pp]\n",
                        jstring_get_string(pid_str), myself);
        pve = NULL;
    }

    JXTA_OBJECT_RELEASE(pid_str);

    return pve;
}

/**
*   Return the peerview entry for the specified peer id.
*
*   @param myself The peerview.
*   @param pid The peer id of the PVE being sought.
*   @return The resulting pve or NULL if no matching PVE was found.
**/
static Jxta_boolean peerview_check_pve(Jxta_peerview * myself, Jxta_PID * pid)
{
    Jxta_boolean res;
    JString *pid_str;

    JXTA_OBJECT_CHECK_VALID(pid);

    res = jxta_id_to_jstring(pid, &pid_str);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed generating string for id. [%pp]\n", myself);
        return FALSE;
    }

    apr_thread_mutex_lock(myself->mutex);

    res = (jxta_hashtable_contains(myself->pves, jstring_get_string(pid_str), jstring_length(pid_str) + 1) == JXTA_SUCCESS) ? TRUE:FALSE;

    apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_RELEASE(pid_str);

    return res;
}


static Jxta_status peerview_add_pve(Jxta_peerview * myself, Peerview_entry * pve)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *pid_str;
    unsigned int pve_cluster = UINT_MAX;
    unsigned int each_member;
    unsigned int all_members;
    Jxta_boolean found = FALSE;

    JXTA_OBJECT_CHECK_VALID(pve);

    if (NULL == pve->target_hash) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == pve->target_hash_radius) {
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_id_to_jstring(jxta_peer_peerid((Jxta_peer *) pve), &pid_str);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Adding new PVE [%pp] for %s\n", pve, jstring_get_string(pid_str));

    apr_thread_mutex_lock(myself->mutex);

    found =
        !jxta_hashtable_putnoreplace(myself->pves, jstring_get_string(pid_str), jstring_length(pid_str) + 1, (Jxta_object *) pve);

    if (found) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not add PVE [%pp] for %s. Already in table.\n", pve,
                        jstring_get_string(pid_str));
        goto FINAL_EXIT;
    }

    pve_cluster = cluster_for_hash(myself, pve->target_hash);

    all_members = jxta_vector_size(myself->clusters[pve_cluster].members);
    for (each_member = 0; !found && (each_member < all_members); each_member++) {
        Peerview_entry *compare_pve;

        res = jxta_vector_get_object_at(myself->clusters[pve_cluster].members, JXTA_OBJECT_PPTR(&compare_pve), each_member);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve peer entry. [%pp]\n",
                            myself->clusters[pve_cluster].members);
            continue;
        }

        found =
            jxta_id_equals(jxta_peer_peerid((Jxta_peer *) compare_pve), jxta_peer_peerid((Jxta_peer *) pve));

        JXTA_OBJECT_RELEASE(compare_pve);

        if (found) {
            break;
        }
    }

    if (!found) {


        jxta_vector_add_object_last(myself->clusters[pve_cluster].members, (Jxta_object *) pve);
        pve->cluster = pve_cluster;

        if (NULL != myself->clusters[pve_cluster].histogram) {
            JXTA_OBJECT_RELEASE(myself->clusters[pve_cluster].histogram);
            myself->clusters[pve_cluster].histogram = NULL;
        }

        res = build_histogram(&myself->clusters[pve_cluster].histogram,
                              myself->clusters[pve_cluster].members,
                              myself->clusters[pve_cluster].min, myself->clusters[pve_cluster].max);

        myself->pves_modcount++;
    }

  FINAL_EXIT:

    /*
     * Notify the peerview listeners that we added a rdv peer from our local rpv 
     */
    if (!found) {

        peerview_update_id(myself);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Added PVE for %s [%pp] to cluster %d\n", jstring_get_string(pid_str),
                        pve, pve_cluster);
        if (PV_STOPPED != myself->state) {
            Jxta_peerview_event *event = peerview_event_new(JXTA_PEERVIEW_ADD, jxta_peer_peerid((Jxta_peer *) pve));
            res = jxta_vector_add_object_last(myself->event_list, (Jxta_object *)event);
            JXTA_OBJECT_RELEASE(event);
            apr_thread_pool_push(myself->thread_pool, call_event_listeners_thread, (void*) myself, 
                                 APR_THREAD_TASK_PRIORITY_HIGH, myself);
        }
    }

    apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_RELEASE(pid_str);

    res = found ? JXTA_BUSY : JXTA_SUCCESS;

    return res;
}

static Jxta_status peerview_remove_pves(Jxta_peerview * myself)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int all_pves;
    unsigned int each_pve;
    Jxta_vector * pves;
    Jxta_peer * a_pve;

    apr_thread_mutex_lock(myself->mutex);
    
    pves = peerview_get_all_pves(myself);

    all_pves = jxta_vector_size(pves);

    for (each_pve = 0; each_pve < all_pves; each_pve++) {

        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&a_pve), each_pve);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "peerview_remove_pves Could not get pve.\n", myself);
            continue;
        }

        if (jxta_id_equals(myself->pid, jxta_peer_peerid((Jxta_peer *) a_pve))) {
            /* It's myself! Handle things differently. */
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }

        peerview_remove_pve_1(myself, jxta_peer_peerid((Jxta_peer *) a_pve), FALSE);

        JXTA_OBJECT_RELEASE(a_pve);
    }
    JXTA_OBJECT_RELEASE(pves);
    apr_thread_mutex_unlock(myself->mutex);
    return res;
}

static Jxta_status peerview_remove_pve(Jxta_peerview * myself, Jxta_PID * pid)
{
    return peerview_remove_pve_1(myself, pid, TRUE);
} 

static Jxta_status peerview_remove_pve_1(Jxta_peerview * myself, Jxta_PID * pid, Jxta_boolean generate_event)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *pid_str;
    Jxta_boolean found = FALSE;
    unsigned int each_cluster;

    JXTA_OBJECT_CHECK_VALID(pid);

    jxta_id_to_jstring(pid, &pid_str);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Removing PVE for %s\n", jstring_get_string(pid_str));

    apr_thread_mutex_lock(myself->mutex);

    res = jxta_hashtable_del(myself->pves, jstring_get_string(pid_str), jstring_length(pid_str) + 1, NULL);

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    for (each_cluster = 0; !found && (each_cluster < myself->clusters_count); each_cluster++) {
        unsigned int each_member;
        unsigned int all_members = jxta_vector_size(myself->clusters[each_cluster].members);

        for (each_member = 0; !found && (each_member < all_members); each_member++) {
            Peerview_entry *compare_pve;

            res = jxta_vector_get_object_at(myself->clusters[each_cluster].members, JXTA_OBJECT_PPTR(&compare_pve), each_member);

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve peer entry. [%pp]\n",
                                myself->clusters[each_cluster].members);
                continue;
            }

            found = jxta_id_equals(jxta_peer_peerid((Jxta_peer *) compare_pve), pid);

            JXTA_OBJECT_RELEASE(compare_pve);

            if (found) {
                jxta_vector_remove_object_at(myself->clusters[each_cluster].members, NULL, each_member);

                if (NULL != myself->clusters[each_cluster].histogram) {
                    JXTA_OBJECT_RELEASE(myself->clusters[each_cluster].histogram);
                    myself->clusters[each_cluster].histogram = NULL;
                }

                res = build_histogram(&myself->clusters[each_cluster].histogram,
                                      myself->clusters[each_cluster].members,
                                      myself->clusters[each_cluster].min, myself->clusters[each_cluster].max);

                myself->pves_modcount++;
            }
        }
    }

  FINAL_EXIT:

    /*
     * Notify the peerview listeners that we removed a rdv peer from our local rpv 
     */
    if (found) {

        peerview_update_id(myself);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removed PVE for %s\n", jstring_get_string(pid_str));
        if (generate_event && (PV_STOPPED != myself->state)) {
            Jxta_peerview_event *event = peerview_event_new(JXTA_PEERVIEW_REMOVE, pid);
            res = jxta_vector_add_object_last(myself->event_list, (Jxta_object *) event);
            JXTA_OBJECT_RELEASE(event);
            apr_thread_pool_push(myself->thread_pool, call_event_listeners_thread, (void*) myself, 
                                 APR_THREAD_TASK_PRIORITY_HIGH, myself);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to remove PVE for %s. Not Found.\n",
                        jstring_get_string(pid_str));
    }

    apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_RELEASE(pid_str);

    res = found ? JXTA_SUCCESS : JXTA_ITEM_NOTFOUND;

    return res;
}

static Jxta_vector *peerview_get_all_pves(Jxta_peerview * myself)
{

    return jxta_hashtable_values_get(myself->pves);
}

static Jxta_status peerview_clear_pves(Jxta_peerview * myself, Jxta_boolean notify)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *current_pves;
    unsigned int each_cluster;
    unsigned int all_pves;
    unsigned int each_pve;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Removing PVEs [%pp]\n", myself);

    apr_thread_mutex_lock(myself->mutex);

    current_pves = jxta_hashtable_values_get(myself->pves);

    jxta_hashtable_clear(myself->pves);

    for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {

        jxta_vector_clear(myself->clusters[each_cluster].members);

        if (NULL != myself->clusters[each_cluster].histogram) {
            JXTA_OBJECT_RELEASE(myself->clusters[each_cluster].histogram);
            myself->clusters[each_cluster].histogram = NULL;
        }
    }

    myself->pves_modcount++;

    /*
     * Notify the peerview listeners that we removed the rdv peers from our local rpv.
     */
    all_pves = jxta_vector_size(current_pves);
    for (each_pve = 0; each_pve < all_pves; each_pve++) {
        Peerview_entry *a_pve;

        res = jxta_vector_remove_object_at(current_pves, JXTA_OBJECT_PPTR(&a_pve), 0);
        if (JXTA_SUCCESS == res) {
            if (notify && (PV_STOPPED != myself->state) && 
                jxta_id_equals(jxta_peer_peerid((Jxta_peer *) a_pve), myself->pid)) {
                
                Jxta_peerview_event *event = peerview_event_new(JXTA_PEERVIEW_REMOVE, jxta_peer_peerid((Jxta_peer *) a_pve));
                res = jxta_vector_add_object_last(myself->event_list, (Jxta_object *) event);
                JXTA_OBJECT_RELEASE(event);
                apr_thread_pool_push(myself->thread_pool, call_event_listeners_thread, (void*) myself, 
                                     APR_THREAD_TASK_PRIORITY_HIGH, myself);
            }
            JXTA_OBJECT_RELEASE(a_pve);
        }
    }

    peerview_update_id(myself);

    apr_thread_mutex_unlock(myself->mutex);
    
    JXTA_OBJECT_RELEASE(current_pves);

    return res;
}

/**
 * Deletes a peerview event
 **/
static void peerview_event_delete(Jxta_object * ptr)
{
    Jxta_peerview_event *pv_event = (Jxta_peerview_event *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Event [%pp] delete \n", ptr);
    JXTA_OBJECT_RELEASE(pv_event->pid);
    pv_event->pid = NULL;

    free(pv_event);
}

/**
 * Creates a peerview event
 **/
static Jxta_peerview_event *peerview_event_new(Jxta_Peerview_event_type event, Jxta_id * id)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview_event *pv_event;
    JString *id_str = NULL;

    if (NULL == id) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "NULL event id\n");
        return NULL;
    }

    pv_event = (Jxta_peerview_event *) calloc(1, sizeof(Jxta_peerview_event));

    if (NULL == pv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not alloc peerview event.\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(pv_event, peerview_event_delete, NULL);

    pv_event->event = event;

    if (NULL != id) {
        pv_event->pid = JXTA_OBJECT_SHARE(id);

        res = jxta_id_to_jstring(id, &id_str);
        if (JXTA_SUCCESS != res) {
            id_str = jstring_new_2("invalid");
        }
    } else {
        id_str = jstring_new_2("unknown");
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "New peerview event [%pp]: {%s} for {%s}\n", pv_event,
                    JXTA_PEERVIEW_EVENT_NAMES[event], jstring_get_string(id_str));
    JXTA_OBJECT_RELEASE(id_str);

    return pv_event;
}

static Peerview_histogram_entry *peerview_histogram_entry_new(BIGNUM * start, BIGNUM * end)
{

    Peerview_histogram_entry *myself = (Peerview_histogram_entry *) calloc(1, sizeof(Peerview_histogram_entry));

    if (NULL == myself) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, peerview_histogram_entry_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Creating PHE [%pp]\n", myself);

    return peerview_histogram_entry_construct(myself, start, end);
}

static Peerview_histogram_entry *peerview_histogram_entry_construct(Peerview_histogram_entry * myself, BIGNUM * start,
                                                                    BIGNUM * end)
{
    myself->start = BN_dup(start);

    myself->end = BN_dup(end);

    myself->peers = jxta_vector_new(0);

    return myself;
}

static void peerview_histogram_entry_delete(Jxta_object * addr)
{
    Peerview_histogram_entry *myself = (Peerview_histogram_entry *) addr;

    peerview_histogram_entry_destruct(myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Deleting PHE [%pp]\n", myself);

    memset(myself, 0xdd, sizeof(Peerview_histogram_entry));

    free(myself);
}

static void peerview_histogram_entry_destruct(Peerview_histogram_entry * myself)
{
    if (NULL != myself->start) {
        BN_free(myself->start);
    }

    if (NULL != myself->end) {
        BN_free(myself->end);
    }

    if (NULL != myself->peers) {
        JXTA_OBJECT_RELEASE(myself->peers);
    }
}

/**
*   Build a histogram which maps the address ranges handled by the specified
*   peers. The histogram is useful for various mappings done by the peerview.
*
*   @param histo The destination vector into which the histogram will be placed.
*   @param peers The peers which will be used to build the histogram.
*   @param minimum  The minimum address for entries within the histogram.
*   @param maximum  The maximum address for entries within the histogram.
*   @return JXTA_SUCCESS if the histogram was built successfully.
*
**/
static Jxta_status build_histogram(Jxta_vector ** histo, Jxta_vector * peers, BIGNUM * minimum, BIGNUM * maximum)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int each_peer;
    unsigned int all_peers;
    Jxta_vector *histogram;

    JXTA_OBJECT_CHECK_VALID(peers);

    if ((NULL == minimum) || (NULL == maximum)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Min/Max not speified.\n");
        return JXTA_FAILED;
    }

    histogram = jxta_vector_new(jxta_vector_size(peers) * 2);

    all_peers = jxta_vector_size(peers);
    for (each_peer = 0; each_peer < all_peers; each_peer++) {
        Peerview_entry *peer = NULL;
        BIGNUM *peer_start;
        BIGNUM *peer_end;
        unsigned int each_segment;

        res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), each_peer);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve peer entry.\n");
            return res;
        }

        JXTA_OBJECT_CHECK_VALID(peer);

        peer_start = BN_new();
        BN_sub(peer_start, peer->target_hash, peer->target_hash_radius);

        peer_end = BN_new();
        BN_add(peer_end, peer->target_hash, peer->target_hash_radius);

        for (each_segment = 0; each_segment < 3; each_segment++) {
            BIGNUM *segment_start = NULL;
            BIGNUM *segment_end = NULL;

            unsigned int each_histogram_entry;

            if (0 == each_segment) {
                /* #0 - The area below minimum wrapped around to maximum */
                if (BN_cmp(peer_start, minimum) >= 0) {
                    /* nothing to do in this segment. */
                    continue;
                }

                segment_start = BN_new();
                BN_sub(segment_start, minimum, peer_start);
                BN_sub(segment_start, maximum, segment_start);

                if (BN_cmp(segment_start, minimum) <= 0) {
                    /* The range is *really* big, keep it within bounds. */
                    /* This is handled as part of segment #1 */
                    BN_free(segment_start);
                    continue;
                }

                segment_end = BN_dup(maximum);
            } else if (1 == each_segment) {
                /* #1 - The area between minimum and maximum */
                if (BN_cmp(peer_start, minimum) <= 0) {
                    segment_start = BN_dup(minimum);
                } else {
                    segment_start = BN_dup(peer_start);
                }

                if (BN_cmp(peer_end, maximum) >= 0) {
                    segment_end = BN_dup(maximum);
                } else {
                    segment_end = BN_dup(peer_end);
                }
            } else if (2 == each_segment) {
                /* #2 - The area above maximum wrapped around to minimum */
                if (BN_cmp(peer_end, maximum) <= 0) {
                    /* Nothing to do for this segment. */
                    continue;
                }

                segment_start = BN_dup(minimum);

                segment_end = BN_new();
                BN_sub(segment_end, peer_end, maximum);
                BN_add(segment_end, segment_end, minimum);

                if (BN_cmp(segment_end, maximum) >= 0) {
                    /* The range is *really* big, keep it within bounds. */
                    /* This is handled as part of segment #1 */
                    BN_free(segment_end);
                    continue;
                }
            }

            /* We have a segment start and end, now we need to insert it into the existing histogram. */
            for (each_histogram_entry = 0; each_histogram_entry <= jxta_vector_size(histogram); each_histogram_entry++) {
                Peerview_histogram_entry *entry = NULL;

                if (each_histogram_entry < jxta_vector_size(histogram)) {
                    /* get an existing histogram entry */
                    res = jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), each_histogram_entry);
                }

                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve histogram entry.\n");
                    return res;
                }

                /* See if we need to create a histogram entry before the current one */
                if ((NULL == entry) || (BN_cmp(segment_start, entry->start) < 0)) {
                    Peerview_histogram_entry *new_entry;
                    Jxta_boolean done = FALSE;

                    if ((NULL == entry) || (BN_cmp(segment_end, entry->start) < 0)) {
                        /* Entirely before the existing entry or no existing entry */
                        new_entry = peerview_histogram_entry_new(segment_start, segment_end);
                        done = TRUE;
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else {
                        /* Create an entry for the portion before the existing entry */
                        BIGNUM *adjusted_end = BN_new();

                        BN_sub(adjusted_end, entry->start, BN_value_one());
                        new_entry = peerview_histogram_entry_new(segment_start, adjusted_end);
                        BN_free(adjusted_end);
                        BN_copy(segment_start, entry->start);
                    }

                    jxta_vector_add_object_last(new_entry->peers, (Jxta_object *) peer);
                    jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry);
                    JXTA_OBJECT_RELEASE(new_entry);

                    /* Are we done this peer? */
                    if (done) {
                        if (NULL != entry) {
                            JXTA_OBJECT_RELEASE(entry);
                            entry = NULL;
                        }
                        break;
                    }
                }

                if (NULL == entry) {
                    continue;
                }

                /* Update the portion which overlaps with the current entry */
                if (BN_cmp(segment_start, entry->start) == 0) {
                    if (BN_cmp(segment_end, entry->end) < 0) {
                        /* Split the entry in two,  */
                        Peerview_histogram_entry *new_entry = peerview_histogram_entry_new(segment_start, segment_end);

                        BN_add(entry->start, segment_end, BN_value_one());  /* adjust the active entry start. */
                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        jxta_vector_addall_objects_last(new_entry->peers, entry->peers);
                        jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry);
                        JXTA_OBJECT_RELEASE(new_entry);
                        each_histogram_entry++; /* adjust index of the active entry. */
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else if (BN_cmp(segment_end, entry->end) == 0) {
                        /* Exactly the same range, just add this peer to the entry */
                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else {
                        /* Peer range ends after the current range */

                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        BN_add(segment_start, entry->end, BN_value_one());
                    }
                }

                JXTA_OBJECT_RELEASE(entry);
            }

            /* Add any remaining bit. */
            if (BN_cmp(segment_start, segment_end) <= 0) {
                Peerview_histogram_entry *new_entry = peerview_histogram_entry_new(segment_start, segment_end);

                jxta_vector_add_object_last(new_entry->peers, (Jxta_object *) peer);
                jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry);
                each_histogram_entry++; /* adjust index of the active entry. */
            }

            BN_free(segment_start);
            BN_free(segment_end);
        }

        BN_free(peer_start);
        BN_free(peer_end);

        JXTA_OBJECT_RELEASE(peer);
    }

    *histo = histogram;

    return res;
}

static Jxta_status create_new_peerview(Jxta_peerview * me)
{
    char * tmp;
    /* We have probed a number of times and not found a peerview. Let's make our own. */
    BIGNUM *address = BN_new();


    me->instance_mask = BN_new();
    BN_rand(me->instance_mask, (SHA_DIGEST_LENGTH * CHAR_BIT), -1, 0);

    tmp = BN_bn2hex(me->instance_mask);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "self chosen Instance Mask : %s\n", tmp);
    free(tmp);

    /* Choose our address randomly. */
    BN_rand(address, (SHA_DIGEST_LENGTH * CHAR_BIT), -1, 0);

    tmp = BN_bn2hex(address);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "self chosen Target Hash Address: %s\n", tmp);
    free(tmp);

    create_self_pve(me, address);

    BN_free(address);

    return JXTA_SUCCESS;
}

static Jxta_status probe_a_seed(Jxta_peerview * me)
{
    Jxta_status res = JXTA_FAILED;
    Jxta_boolean sent_seed = FALSE;
    Jxta_peer *seed;

    apr_thread_mutex_lock(me->mutex);

    /* Get more seeds? */
    if ((NULL == me->activity_locate_seeds) || (0 == jxta_vector_size(me->activity_locate_seeds))) {
        if (NULL != me->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(me->activity_locate_seeds);
            me->activity_locate_seeds = NULL;
        }

        apr_thread_mutex_unlock(me->mutex);
        if (NULL == me->rdv) {
            jxta_PG_get_rendezvous_service(me->group, &(me->rdv));
        }

        if (NULL != me->rdv) {
            res = rdv_service_get_seeds(me->rdv, &me->activity_locate_seeds, TRUE);

            /* Send a broadcast probe */
            res = rdv_service_send_seed_request(me->rdv);
        }
        apr_thread_mutex_lock(me->mutex);

        /* We only send increment this each time we get seeds so that the list of seeds is processed fairly quickly. */
        me->activity_locate_probes++;
    }

    /* probe a seed */
    while (!sent_seed && (jxta_vector_size(me->activity_locate_seeds) > 0)) {
        res = jxta_vector_remove_object_at(me->activity_locate_seeds, JXTA_OBJECT_PPTR(&seed), 0);
        if (JXTA_SUCCESS != res) {
            continue;
        }

        if (jxta_peer_get_expires(seed) > jpr_time_now()) {

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Ping locate seeds.\n");

            res = peerview_send_ping(me, seed, NULL, FALSE);
            sent_seed = TRUE;
        }

        JXTA_OBJECT_RELEASE(seed);
    }

    apr_thread_mutex_unlock(me->mutex);

    return res;
}

/**
*   An activity task which attempts to locate a peerview instance by pinging
*   seed peers.
**/
static void *APR_THREAD_FUNC activity_peerview_locate(apr_thread_t * thread, void *cookie)
{
    Jxta_status res;
    Jxta_peerview *myself = PTValid(cookie, Jxta_peerview);
    apr_status_t apr_res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[locate] [%pp]: Running.\n", myself);

    apr_thread_mutex_lock(myself->mutex);

    if (PV_LOCATING != myself->state) {
        if (NULL != myself->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(myself->activity_locate_seeds);
            myself->activity_locate_seeds = NULL;
        }
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    if (myself->activity_locate_probes >= jxta_RdvConfig_pv_max_locate_probes(myself->rdvConfig)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[locate] [%pp]: Locate failed. Creating a new peerview instance.\n",
                        myself);
        create_new_peerview(myself);

        myself->state = PV_MAINTENANCE;
        apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_maintain, myself, 1000, myself);
        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE 
                            "PV[%pp] ACT[locate]: Could not schedule peerview maintenance activity.\n",
                            myself);
        }

        if (NULL != myself->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(myself->activity_locate_seeds);
            myself->activity_locate_seeds = NULL;
        }
    } else {
        apr_thread_mutex_unlock(myself->mutex);
        res = probe_a_seed(myself);
        apr_thread_mutex_lock(myself->mutex);
        apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_locate, myself,
                                                        apr_time_from_sec(1 << myself->activity_locate_probes), myself);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[locate] [%pp]: Could not reschedule activity.\n",
                            myself);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[locate] [%pp]: Rescheduled for %d secs.\n",
                            myself, (1 << myself->activity_locate_probes));
        }
    }

    apr_thread_mutex_unlock(myself->mutex);
    return NULL;
}

static void *APR_THREAD_FUNC activity_peerview_addressing(apr_thread_t * thread, void *arg)
{
    Jxta_status res;
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_vector * pves;
    unsigned int all_pves;
    unsigned int each_pve;
    Peerview_entry * a_pve;
    unsigned int address_requests_sent = 0;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[addressing] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);

    if (PV_ADDRESSING != myself->state) {
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    /* FIXME: get_all_pves better not to include self_pve to avoid checking, modify get/add/check etc to include self_pve is less
     * expensive
     */
    pves = peerview_get_all_pves(myself);
    apr_thread_mutex_unlock(myself->mutex);

    all_pves = jxta_vector_size(pves);
    jxta_vector_shuffle(pves);  /* We use this for probes so shuffle it. */

    for (each_pve = 0; each_pve < all_pves; each_pve++) {

        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&a_pve), each_pve);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "ACT[addressing] [%pp]: Could not get pve.\n", myself);
            continue;
        }

        if (jxta_id_equals(myself->pid, jxta_peer_peerid((Jxta_peer *) a_pve))) {
            /* It's myself! Handle things differently. */
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }

        /* Send an address request. */
        if (address_requests_sent < jxta_RdvConfig_pv_max_address_requests(myself->rdvConfig)) {
            res = peerview_send_address_request(myself, (Jxta_peer *) a_pve);
            address_requests_sent++;
        }

        JXTA_OBJECT_RELEASE(a_pve);
    }

    JXTA_OBJECT_RELEASE(pves);
    
    /* Reschedule another check. */
    apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_addressing, myself, 
                                                    jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig), myself);

    if (APR_SUCCESS != apr_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[addressing] [%pp]: Could not reschedule activity.\n",
                        myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[addressing] [%pp]: Rescheduled.\n", myself);
    }
    return NULL;
}

static void *APR_THREAD_FUNC activity_peerview_announce(apr_thread_t * thread, void *arg)
{
    Jxta_status res;
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_vector * pves;
    unsigned int all_pves;
    unsigned int each_pve;
    Peerview_entry * a_pve;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[announce] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);

    if (PV_ANNOUNCING != myself->state) {
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    pves = peerview_get_all_pves(myself);

    apr_thread_mutex_unlock(myself->mutex);

    all_pves = jxta_vector_size(pves);

    for (each_pve = 0; each_pve < all_pves; each_pve++) {

        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&a_pve), each_pve);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "ACT[maintain] [%pp]: Could not get pve.\n", myself);
            continue;
        }

        if (jxta_id_equals(myself->pid, jxta_peer_peerid((Jxta_peer *) a_pve))) {
            /* It's myself! Handle things differently. */
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }

        res = peerview_send_pong(myself, (Jxta_peer *) a_pve, PONG_STATUS, FALSE, FALSE);
        JXTA_OBJECT_RELEASE(a_pve);
    }

    JXTA_OBJECT_RELEASE(pves);

    apr_thread_mutex_lock(myself->mutex);

    myself->state = PV_MAINTENANCE;

    apr_thread_mutex_unlock(myself->mutex);
    if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_maintain, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, myself)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate maintain activity. [%pp]\n", myself);
    }
    return NULL;
}

static void remove_connected_referrals(Jxta_peerview * me)
{
    unsigned int i;
    for (i = jxta_vector_size(me->activity_maintain_referral_peers); i > 0; i--) {
        Jxta_peer *peer = NULL;

        jxta_vector_get_object_at(me->activity_maintain_referral_peers, JXTA_OBJECT_PPTR(&peer), i - 1);

        if (peerview_check_pve(me, jxta_peer_peerid(peer))) {

            jxta_vector_remove_object_at(me->activity_maintain_referral_peers, NULL, i - 1);
        }
        JXTA_OBJECT_RELEASE(peer);
    }
}

static Jxta_status probe_referrals(Jxta_peerview * me)
{
    unsigned int each_old_ping;
    Jxta_peer *ping_peer = NULL;
    Jxta_status res;

    /* Remove pings from last round from vector */
    for (each_old_ping = 0; each_old_ping < me->activity_maintain_referral_pings_sent; each_old_ping++) {
        jxta_vector_remove_object_at(me->activity_maintain_referral_peers, NULL, 0);
    }
    me->activity_maintain_referral_pings_sent = 0;

    /* if we have a connection remove it from the referrals */
    remove_connected_referrals(me);

    /* Send ping messages to any referrals from pong responses. */

    jxta_vector_shuffle(me->activity_maintain_referral_peers);  /* We use this for probes so shuffle it. */

    while ((me->activity_maintain_referral_pings_sent < jxta_RdvConfig_pv_max_ping_probes(me->rdvConfig))
           && (me->activity_maintain_referral_pings_sent < jxta_vector_size(me->activity_maintain_referral_peers))) {

        res = jxta_vector_get_object_at(me->activity_maintain_referral_peers, JXTA_OBJECT_PPTR(&ping_peer),
                                      me->activity_maintain_referral_pings_sent);

        if (JXTA_SUCCESS == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Ping referral.\n");
            res = peerview_send_ping(me, ping_peer, NULL, FALSE);
            JXTA_OBJECT_RELEASE(ping_peer);
        }

        /* We increment unconditionally, a bad peer is one we should get rid of. */
        me->activity_maintain_referral_pings_sent++;
    }

    return JXTA_SUCCESS;
}

static Jxta_status check_pves(Jxta_peerview * me, Jxta_vector *pves)
{
    Jxta_status res;
    unsigned int all_pves;
    unsigned int each_pve;
    Peerview_entry * a_pve;
    Jxta_time now;
    Jxta_time expires;

    /* Kill, ping or request address from current pves */
    all_pves = jxta_vector_size(pves);

    for (each_pve = 0; each_pve < all_pves; each_pve++) {

        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&a_pve), each_pve);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "ACT[maintain] [%pp]: Could not get pve.\n", me);
            continue;
        }

        if (jxta_id_equals(me->pid, jxta_peer_peerid((Jxta_peer *) a_pve))) {
            /* It's me! Handle things differently. */
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
         }

        now = jpr_time_now();
        expires = jxta_peer_get_expires((Jxta_peer *) a_pve);

        if (expires <= now) {
            /* It's dead Jim. Remove it from the peerview */

            peerview_remove_pve(me, jxta_peer_peerid((Jxta_peer *) a_pve));
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }
        if ((expires - now) <= jxta_RdvConfig_pv_ping_due(me->rdvConfig)) {
            /* Ping for refresh */
            apr_uuid_t * pv_id_gen = NULL;
            Jxta_version * peer_version;

            if (a_pve->pv_id_gen_set) {
                pv_id_gen = &a_pve->pv_id_gen;
            }
            peer_version = jxta_PA_get_version(((_jxta_peer_entry *) a_pve)->adv);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending ping for maintenance\n");
            res = peerview_send_ping(me, (Jxta_peer *) a_pve, &a_pve->adv_gen, jxta_version_compatible_1(PEERVIEW_UUID_IMPLEMENTATION, peer_version) ? TRUE:FALSE);
            JXTA_OBJECT_RELEASE(peer_version);
        }

        JXTA_OBJECT_RELEASE(a_pve);
    }

    return JXTA_SUCCESS;
}

/**
*   The maintain task is responsible for :
*       - expiring dead PVEs
*       - pinging near-dead PVEs
*       - pinging new potential PVEs
*       - sending address request messages. (optional)
**/
static void *APR_THREAD_FUNC activity_peerview_maintain(apr_thread_t * thread, void *cookie)
{

    Jxta_peerview *myself = PTValid(cookie, Jxta_peerview);
    Jxta_vector * current_pves;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[maintain] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);

    if ((PV_ADDRESSING != myself->state) && (PV_MAINTENANCE != myself->state)) {
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    current_pves = peerview_get_all_pves(myself);

    apr_thread_mutex_unlock(myself->mutex);

    check_pves(myself, current_pves);
    JXTA_OBJECT_RELEASE(current_pves);

    probe_referrals(myself);

    if (PV_ADDRESSING != myself->state && PV_MAINTENANCE != myself->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[maintain] [%pp]: done. \n", myself);
        return NULL;
    }

    /* Reschedule another check. */
    apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_maintain, myself, 
                                                    jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig), myself);

    if (APR_SUCCESS != apr_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[maintain] [%pp]: Could not reschedule activity.\n",
                        myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: Rescheduled.\n", myself);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] activity add: %s\n", myself->activity_add ? "true":"false");

    if (!myself->activity_add && need_additional_peers(myself, myself->cluster_members)) {
        /* Start the add activity? */

        myself->activity_add = TRUE;
        apr_res = apr_thread_pool_push(myself->thread_pool, activity_peerview_add, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, &myself->activity_add);
        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "ACT[maintain] [%pp]: Could not start add activity.\n", myself);
            myself->activity_add = FALSE;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: Scheduled [Add].\n", myself);
        }
    }
    return NULL;
}

Jxta_vector *client_entry_get_options(Jxta_peer * me);

static Jxta_boolean need_additional_peers(Jxta_peerview * me, int max_check)
{
    unsigned int each_cluster;
    Jxta_boolean need_additional = FALSE;

    /* Survey and determine if additional Peerview members are needed in other clusters */
    for (each_cluster = 0; each_cluster < me->clusters_count; each_cluster++) {
        unsigned int member_count = jxta_vector_size(me->clusters[each_cluster].members);
        if (0 == member_count) {
            need_additional = TRUE;
            break;
        }
    }

    if (!need_additional) {
        /* Survey and determine if additional Peerview members are needed in our cluster */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Are peers %d less than %d. \n",jxta_vector_size(me->clusters[me->my_cluster].members), max_check  );
        if (jxta_vector_size(me->clusters[me->my_cluster].members) < max_check) {
            need_additional = TRUE;
        }
    }
    return need_additional;
}

static Jxta_status get_best_rdv_candidates(Jxta_peerview *myself, Jxta_vector **candidates, int nums)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *rdv_clients = NULL;
    Jxta_peer * peer = NULL;
    unsigned int i;
    Jxta_vector * candidates_v = NULL;

    res = jxta_rdv_service_get_peers(myself->rdv, &rdv_clients);
    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }
    trim_only_candidates(rdv_clients);

    if (jxta_vector_size(rdv_clients) < 1) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No Candidates received from the rendezvous.\n");
        res = JXTA_ITEM_NOTFOUND;
        goto FINAL_EXIT;
    }

    jxta_vector_qsort(rdv_clients, myself->candidate_cb);

    for (i = 0; i < jxta_vector_size(rdv_clients) && i < nums; i++) {
        res = jxta_vector_get_object_at(rdv_clients, JXTA_OBJECT_PPTR(&peer), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error getting an entry from the rdv clients.\n");
            continue;
        }
        if (NULL == candidates_v) {
            candidates_v = jxta_vector_new(0);
            if (NULL == candidates_v) {
                res = JXTA_NOMEM;
                goto FINAL_EXIT;
            }
        }
        jxta_vector_add_object_last(candidates_v, (Jxta_object *) peer);
        JXTA_OBJECT_RELEASE(peer);
        peer = NULL;
    }

FINAL_EXIT:

    *candidates = candidates_v;
    if (peer)
        JXTA_OBJECT_RELEASE(peer);
    if (rdv_clients)
        JXTA_OBJECT_RELEASE(rdv_clients);
    return res;
}

/**
*   The add task is responsible for :
*       - adding additional peers to the peerview.
 **/
static void *APR_THREAD_FUNC activity_peerview_add(apr_thread_t * thread, void *cookie)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(cookie, Jxta_peerview);
    Jxta_boolean locked = FALSE;
    Jxta_vector *rdv_clients=NULL;
    unsigned int all_clients;
    unsigned int each_client;
    Jxta_vector * pves=NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[add] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);
    myself->activity_add = TRUE;

    locked = TRUE;

    if (PV_STOPPED == myself->state || !myself->active || !need_additional_peers(myself, myself->cluster_members)) {
        myself->activity_add = FALSE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[add] [%pp]: Done. \n", myself);
        goto FINAL_EXIT;
    }

    /* waiting for other candidates */
    if ((myself->activity_add_candidate_requests_expiration > jpr_time_now()
            && jxta_vector_size(myself->activity_add_candidate_voting_peers) > 0)
            || myself->activity_add_voting_ends > jpr_time_now()) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] Can't send msgs right now because an election is being held\n");
        goto RERUN_EXIT;
    }

    if (NULL != myself->candidate_list_cb && 0 != jxta_vector_size(myself->activity_add_candidate_peers)) {
        locked = FALSE;
        apr_thread_mutex_unlock(myself->mutex);
        /* Get candidates from the application */
        res = (* myself->candidate_list_cb)(myself->activity_add_candidate_peers, &rdv_clients);

    }

    if (JXTA_SUCCESS != res) {
        goto RERUN_EXIT;

    }

    if (locked == FALSE) {
        apr_thread_mutex_lock(myself->mutex);
        locked = TRUE;
    }
    /* Get some candidates from all the peers in the peerview. */

    if (0 == jxta_vector_size(myself->activity_add_candidate_peers) && !myself->activity_add_voting) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[add] [%pp]: Attempting to recruit peerview members.\n", myself);

        res = jxta_peerview_get_localview(myself, &pves);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve the local view\n");
            goto RERUN_EXIT;
        }
        all_clients = jxta_vector_size(pves);

        /* get_best_rdv_candidates makes a call to jxta_rdv_service which requires the rdv_service mutex.  Must not hold the
         * peerview mutex while making this call to prevent deadlock*/
        apr_thread_mutex_unlock(myself->mutex);
        res = get_best_rdv_candidates(myself, &rdv_clients, 1);
        apr_thread_mutex_lock(myself->mutex);

        if (JXTA_SUCCESS != res || jxta_vector_size(rdv_clients) < 1) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] No Candidates found.\n");
            goto RERUN_EXIT;
        }
        if (myself->candidate_cb) {
            jxta_vector_qsort(rdv_clients, myself->candidate_cb);
        }

        if (0 != all_clients) {
            if (myself->activity_add_candidate_peers)
                JXTA_OBJECT_RELEASE(myself->activity_add_candidate_peers);

            myself->activity_add_candidate_peers = JXTA_OBJECT_SHARE(rdv_clients);

            for (each_client = 0; each_client < all_clients; each_client++) {
                Jxta_peer * peer;
                res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&peer), each_client);
                if (JXTA_SUCCESS != res) {
                    continue;
                }
                if (jxta_id_equals(jxta_peer_peerid(peer), (Jxta_id *) myself->pid)) {
                    JXTA_OBJECT_RELEASE(peer);
                    continue;
                }
                myself->activity_add_voting = TRUE;
                res = peerview_send_pong(myself, peer, PONG_PROMOTE, TRUE, FALSE);
                if (JXTA_SUCCESS == res) {
                    jxta_vector_add_object_last(myself->activity_add_candidate_voting_peers, (Jxta_object *) peer);
                }
                JXTA_OBJECT_RELEASE(peer);
            }
            if (myself->activity_add_voting) {
                myself->activity_add_candidate_requests_expiration = jpr_time_now() + jxta_RdvConfig_pv_voting_expiration(myself->rdvConfig);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] ------------- INIT ELECTION ---------- \n");
            }
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] Voting has ended - recruit any peers that we proposed\n");
        myself->activity_add_voting = FALSE;
        all_clients = jxta_vector_size(myself->activity_add_candidate_peers);
        if (0 == all_clients) {
            goto RERUN_EXIT;
        }
    }

    if (NULL != myself->candidate_cb) {
        /* order the candidates */
        res = jxta_vector_qsort(myself->activity_add_candidate_peers, myself->candidate_cb);
    }

    myself->activity_add_candidate_pongs_sent = 0;


    for (each_client = 0; !myself->activity_add_voting &&  (each_client < all_clients) && (each_client < MAXIMUM_CLIENT_INVITATIONS); each_client++) {
        Jxta_peer *invitee;

        res = jxta_vector_remove_object_at(myself->activity_add_candidate_peers, JXTA_OBJECT_PPTR(&invitee), 0);

        if (JXTA_SUCCESS != res) {
            continue;
        }
        res = peerview_send_pong(myself, invitee, PONG_INVITE, FALSE, FALSE);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] [%pp]: Pong invite failed.\n", myself);
        } else {
            JString * jPeerid;
            jxta_id_to_jstring(jxta_peer_peerid(invitee), &jPeerid);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add]: Pong invite to %s.\n", jstring_get_string(jPeerid));
            myself->activity_add_candidate_pongs_sent++;
            JXTA_OBJECT_RELEASE(jPeerid);
        }
        JXTA_OBJECT_RELEASE(invitee);
    }


  RERUN_EXIT:


    if (myself->activity_add_candidate_pongs_sent < MAXIMUM_CLIENT_INVITATIONS && !myself->activity_add_voting) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add]: Probing seeds \n");
        apr_thread_mutex_unlock(myself->mutex);
        probe_a_seed(myself);
        apr_thread_mutex_lock(myself->mutex);
    }

    if (myself->running) {
        /* Reschedule another check. */
        apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_add, myself, 
                                                        jxta_RdvConfig_pv_add_interval(myself->rdvConfig), &myself->activity_add);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[add] [%pp]: Could not reschedule activity.\n",
                            myself);
        } else {
            myself->activity_add = TRUE;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] [%pp]: Rescheduled.\n", myself);
        }
    }
  FINAL_EXIT:
    if (rdv_clients)
        JXTA_OBJECT_RELEASE(rdv_clients);

    if (pves)
        JXTA_OBJECT_RELEASE(pves);

    if (locked) {
        apr_thread_mutex_unlock(myself->mutex);
    }

    return NULL;
}

static void trim_only_candidates(Jxta_vector * possible_v)
{
    Jxta_status status;
    unsigned int i;
    Jxta_vector * options = NULL;
    Jxta_peer * peer = NULL;
    unsigned int j;
    unsigned int willingness;
    
    for (i = 0; i < jxta_vector_size(possible_v); i++) {
        willingness = 0;

        status = jxta_vector_get_object_at(possible_v, JXTA_OBJECT_PPTR(&peer), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        if (jxta_peer_get_expires(peer) > jpr_time_now()) {
            jxta_peer_get_options(peer, &options);
            if (NULL == options) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: peer options were NULL.\n", peer);
                JXTA_OBJECT_RELEASE(peer);
                peer = NULL;
                continue;
            }
            for (j=0; j < jxta_vector_size(options); j++) {
                Jxta_status res;
                Jxta_rdv_lease_options * lease_option;

                res = jxta_vector_get_object_at(options, JXTA_OBJECT_PPTR(&lease_option), j);

                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not get lease option.\n", options);
                    continue;
                }
                if (NULL == lease_option) continue;
                if (strcmp(jxta_advertisement_get_document_name((Jxta_advertisement *) lease_option), "jxta:RdvLeaseOptions")) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Not a rendezvous lease option\n");
                    JXTA_OBJECT_RELEASE(lease_option);
                    lease_option = NULL;
                    continue;
                }
                willingness = (jxta_rdv_lease_options_get_willingness(lease_option));
                JXTA_OBJECT_RELEASE(lease_option);
            }
            JXTA_OBJECT_RELEASE(options);
        }
        if (willingness < 1) {
            jxta_vector_remove_object_at(possible_v, NULL, i);
            i--;
        }
        JXTA_OBJECT_RELEASE(peer);
        peer = NULL;
    }
}

static int ranking_sort(Jxta_peer **peer_a, Jxta_peer **peer_b)
{
    Jxta_vector * options_a = NULL;
    Jxta_vector * options_b = NULL;
    unsigned int i;
    float *suitability;
    float *willingness;
    float suit_a;
    float suit_b;
    float will_a;
    float will_b;
    float ranking_a = 0.0;
    float ranking_b = 0.0;
    int res;

    Jxta_vector * options=NULL;
    Jxta_rdv_lease_options ** lease_option=NULL;
    Jxta_rdv_lease_options * lease_option_a=NULL;
    Jxta_rdv_lease_options * lease_option_b=NULL;

    JString *peerida_j = NULL;
    JString *peeridb_j = NULL;

    jxta_id_to_jstring(jxta_peer_peerid(*peer_a), &peerida_j);
    jxta_id_to_jstring(jxta_peer_peerid(*peer_b), &peeridb_j);

    jxta_peer_get_options(*peer_a, &options_a);
    jxta_peer_get_options(*peer_b, &options_b);

    if (NULL == options_a || NULL == options_b) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "No options for %s  %s\n"
                , options_a == NULL ? jstring_get_string(peerida_j) : ""
                , options_b == NULL ? jstring_get_string(peeridb_j) : "");
        res = 0;
        goto FINAL_EXIT;
    }
    options = options_a;
    lease_option = &lease_option_a;
    suitability = &suit_a;
    willingness = &will_a;
    while (TRUE) {
       for (i=0; i < jxta_vector_size(options); i++) {
            Jxta_status rres;

            rres = jxta_vector_get_object_at(options, JXTA_OBJECT_PPTR(lease_option), i);

            if (JXTA_SUCCESS != rres) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not get options.\n", options);
                continue;
            }
            if (NULL == *lease_option) continue;
            if (strcmp(jxta_advertisement_get_document_name((Jxta_advertisement *) *lease_option), "jxta:RdvLeaseOptions")) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Not a rendezvous lease option\n");
                JXTA_OBJECT_RELEASE(*lease_option);
                *lease_option = NULL;
                continue;
            }
            *suitability = (.01 * jxta_rdv_lease_options_get_suitability(*lease_option));
            *willingness = (.01 * jxta_rdv_lease_options_get_willingness(*lease_option));

            JXTA_OBJECT_RELEASE(*lease_option);
            *lease_option = NULL;
        }

        if (options == options_b) break;
        options = options_b;
        lease_option = &lease_option_b;
        suitability = &suit_b;
        willingness = &will_b;
    }
    ranking_a = suit_a * will_a;
    ranking_b = suit_b * will_b;
    if (lease_option_a)
        JXTA_OBJECT_RELEASE(lease_option_a);
    if (lease_option_b)
        JXTA_OBJECT_RELEASE(lease_option_b);
    if (fabs(ranking_a - ranking_b) > 0.0001) {
        res = 1;
        if (ranking_a > ranking_b) res = -1;
     } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Auto-rendezous ranking equal check peers %s %s\n"
                , jstring_get_string(peerida_j), jstring_get_string(peeridb_j) );
        res = strcmp(jstring_get_string(peerida_j), jstring_get_string(peeridb_j));
     }
FINAL_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Auto-rendezous ranking_a:%f ranking_b:%f res:%d\n" ,ranking_a, ranking_b, res);
    if (options_a)
        JXTA_OBJECT_RELEASE(options_a);
    if (options_b)
        JXTA_OBJECT_RELEASE(options_b);
    if (peerida_j)
        JXTA_OBJECT_RELEASE(peerida_j);
    if (peeridb_j)
        JXTA_OBJECT_RELEASE(peeridb_j);
    return res;
}

static int ranking_candidate_sort(Jxta_peer ** me, Jxta_peer ** target)
{
    int res;
    res = ranking_sort(me, target);
    return res;

}

static Jxta_boolean remain_in_peerview(Jxta_peerview *me, unsigned int size)
{
    unsigned int all_pves;
    unsigned int each_pve;
    Jxta_vector * pves;
    Jxta_boolean remain = FALSE;

    jxta_peerview_get_localview(me, &pves);

    all_pves = jxta_vector_size(pves);

    jxta_vector_qsort(pves, me->ranking_order_cb);

    for (each_pve = 0; each_pve < all_pves && each_pve < size; each_pve++) {
        Jxta_status res;
        Jxta_peer * peer;
        Jxta_id *pid;
        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&peer), each_pve);
        if (JXTA_SUCCESS != res) {
            continue;
        }
        jxta_peer_get_peerid(peer, &pid);
        if (jxta_id_equals(pid, me->pid)) {
            remain = TRUE;
        }
        JXTA_OBJECT_RELEASE(pid);
        JXTA_OBJECT_RELEASE(peer);
        if (remain == TRUE) {
            break;
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Remain in Peerview: %s\n", remain == TRUE ? "true":"false");

    JXTA_OBJECT_RELEASE(pves);

    return remain;
}

static void *APR_THREAD_FUNC activity_peerview_auto_cycle(apr_thread_t * thread, void *arg)
{
    Jxta_peerview * me = PTValid(arg, Jxta_peerview);
    Jxta_rdv_service *rdv=NULL;
    Jxta_status res = JXTA_SUCCESS;
    RdvConfig_configuration new_config;
    RdvConfig_configuration tmp_config;
    Jxta_boolean demoted = FALSE;
    Jxta_vector *rdvs = NULL;
    Jxta_id *id = NULL;
    Jxta_boolean check_peers_size = FALSE;
    Jxta_boolean increment_loneliness = FALSE;
    int tmp_iterations_since_switch = -1;
    Jxta_boolean locked = FALSE;


    if (PV_STOPPED == me->state) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[auto-cycle] %pp RUN.\n", me);

    apr_thread_mutex_lock(me->mutex);
    locked = TRUE;

    if (PV_ADDRESSING == me->state && me->auto_cycle > 0) {

        goto RERUN_EXIT;
    }

    if (NULL == me->rdv) {
        jxta_PG_get_rendezvous_service(me->group, &me->rdv);
    }
    rdv = me->rdv;

    if (NULL == rdv) {
        goto RERUN_EXIT;
    }

    if (NULL != me->self_pve) {
        res = jxta_peer_get_peerid((Jxta_peer *)me->self_pve, &id);
        if (JXTA_SUCCESS != res) {
            goto RERUN_EXIT;
        }
    }
    
    new_config = jxta_rdv_service_config(rdv);
    tmp_config = new_config;

    /* if we're demoting and we've shed all obligations switch to edge */
    if (jxta_rdv_service_is_rendezvous(rdv) && jxta_rdv_service_is_demoting(rdv)) {
        check_peers_size = TRUE;
        demoted = TRUE;
        tmp_config = config_edge;
    }

    /* if there are too many peers in the pv */
    if (!need_additional_peers(me, me->cluster_members) ) {
        me->loneliness_factor = 0;
        if (!demoted) {
            if (remain_in_peerview(me, DEFAULT_CLUSTER_MEMBERS) == FALSE) {
                check_peers_size = TRUE;
                tmp_config = config_edge;
            }
        }
    } else if (jxta_rdv_service_config(rdv) == config_edge) {
        check_peers_size = TRUE;
        increment_loneliness = TRUE;
        if ((me->loneliness_factor + 1) > jxta_RdvConfig_pv_loneliness(me->rdvConfig)) {
            /* force it */
            tmp_iterations_since_switch = 3;
            tmp_config = config_rendezvous;
        }
    }

    apr_thread_mutex_unlock(me->mutex);
    locked = FALSE;
    
    if (check_peers_size == TRUE) {
        res = jxta_rdv_service_get_peers(rdv, &rdvs);
        if (JXTA_SUCCESS == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Should be switching if %d is 0\n", jxta_vector_size(rdvs));
            if (0 == jxta_vector_size(rdvs)) {
                if (increment_loneliness == TRUE) {
                    ++me->loneliness_factor;
                }
                if (tmp_iterations_since_switch >= 0) {
                    me->iterations_since_switch = tmp_iterations_since_switch;
                }
                new_config = tmp_config;
            }
            JXTA_OBJECT_RELEASE(rdvs);
        }
    }

    /* We can perform these operations without a mutex since the iterations_since_switch is only modified by the auto_cycle thread
     * except for when starting the peerview.  But in that case, the auto_cycle has not been started yet.  
     * The peerview_remove_pves will lock the peerview separately.
     * The peerview lock must not be held when calling either the call_event_listeners or the rdv_service_switch_config
     */
     
    /* Refuse to switch if we just switched */
    if ((jxta_rdv_service_config(rdv) != new_config) && (me->iterations_since_switch > 2)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO,"Auto-rendezvous new_config -- %s\n", new_config == config_edge ? "edge":"rendezvous");
        if (config_edge == new_config) {
            /* the switch is done in the rdv_service in this case */
            peerview_remove_pves(me);
            apr_thread_mutex_lock(me->mutex);
            if (PV_STOPPED != me->state) {
                Jxta_peerview_event *event = peerview_event_new(JXTA_PEERVIEW_DEMOTE, id);
                res = jxta_vector_add_object_last(me->event_list, (Jxta_object *) event);
                JXTA_OBJECT_RELEASE(event);
                apr_thread_pool_push(me->thread_pool, call_event_listeners_thread, (void*) me, 
                                     APR_THREAD_TASK_PRIORITY_HIGH, me);
            }
            peerview_update_id(me);

            apr_thread_mutex_unlock(me->mutex);
        }
        else {
            rdv_service_switch_config(rdv, new_config);
        }
        me->iterations_since_switch = 0;
    } else {
        me->iterations_since_switch++;
    }


RERUN_EXIT:
    if (id)
        JXTA_OBJECT_RELEASE(id);

    if (locked == FALSE)
        apr_thread_mutex_lock(me->mutex);
    
    if (me->auto_cycle > 0) {
        res = apr_thread_pool_schedule(me->thread_pool, activity_peerview_auto_cycle, me, me->auto_cycle, &me->auto_cycle);

        if (APR_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not schedule auto cycle activity : %d\n", me, res);
        }
    }

    apr_thread_mutex_unlock(me->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[auto-cycle] %pp DONE.\n", me);

    return NULL;
}

/* vim: set ts=4 sw=4 et tw=130: */
