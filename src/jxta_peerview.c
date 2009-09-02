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
#include "jxta_em.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_version.h"
#include "jxta_peerview_address_request_msg.h"
#include "jxta_peerview_address_assign_msg.h"
#include "jxta_peerview_ping_msg.h"
#include "jxta_peerview_pong_msg.h"
#include "jxta_adv_request_msg.h"
#include "jxta_adv_response_msg.h"
#include "jxta_id_uuid_priv.h"
#include "jxta_util_priv.h"
#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_lease_options.h"
#include "jxta_peerview_monitor_entry.h"

/**
*   Names of the Peerview Events
**/
const char *JXTA_PEERVIEW_EVENT_NAMES[] = { "unknown", "ADD", "REMOVE", "DEMOTE" };

const char JXTA_PEERVIEW_NS_NAME[] = "jxta";
const char JXTA_PEERVIEW_NAME[] = "PeerView";
const char JXTA_PEERVIEW_NAME_CONS[] = "PeerView-Cons";
const char JXTA_PEERVIEW_EP_NAME_CONS[] = "EndpointService:cons";

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

#define JXTA_PEERVIEW_ADDRESS_ASSIGN_EXPIRATION (10 * JPR_INTERVAL_ONE_SECOND)

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

#define CHECK_MESSAGE_SOURCE \
    if (NULL != myself->activity_address_locking_peer) { \
        if (0 != strcmp(myself->activity_address_locking_peer, rcv_peerid_c)) { \
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received a message from the wrong source %s\n", rcv_peerid_c); \
            return JXTA_SUCCESS; \
        } \
    }


struct Cluster_entry {
    BIGNUM *min;
    BIGNUM *mid;
    BIGNUM *max;
    BIGNUM *next_address;

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
    int cluster_peers;
    Jxta_vector *free_hash_list;

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
    Jxta_time last_walk_select;
    unsigned int walk_counter;
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

static char *state_description[] = {
    "PV_STOPPED",
    "PV_PASSIVE",
    "PV_LOCATING",
    "PV_ADDRESSING",
    "PV_ANNOUNCING",
    "PV_MAINTENANCE" };

typedef enum assign_states {
    ASSIGN_IDLE = 0,
    ASSIGN_SEND_FREE,
    ASSIGN_FREE_WAITING_RSP,
    ASSIGN_SEND_LOCK,
    ASSIGN_LOCK_WAITING_RSP,
    ASSIGN_ASSIGNED_WAITING_RSP,
    ASSIGN_LOCKED
} Assign_state;

static char *assign_state_description[] = {
    "ASSIGN_IDLE",
    "ASSIGN_SEND_FREE",
    "ASSIGN_FREE_WAITING_RSP",
    "ASSIGN_SEND_LOCK",
    "ASSIGN_LOCK_WAITING_RSP",
    "ASSIGN_ASSIGNED_WAITING_RSP",
    "ASSIGN_LOCKED"
};
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
    JString *pid_j;
    char *pid_c;
    char *groupUniqueID;
    Jxta_boolean pv_id_gen_set;
    apr_uuid_t pv_id_gen;

    /* call backs */
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
    unsigned int hash_attempts;
    unsigned int max_client_invitations;

    Jxta_boolean active;
    unsigned int processing_callbacks;

    /**
    *   The instance mask of the peerview instance we are participating in or 
    *   NULL if we haven't yet chosen a peerview instance in which to participate.
    **/
    BIGNUM *instance_mask;

    BIGNUM *hash_space;
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

    /* ACTIVITY Address Locking */
    Jxta_time activity_address_locking_ends;
    Jxta_time activity_address_assigned_ends;
    Jxta_time activity_address_free_ends;
    Jxta_peer *activity_address_assign_peer;
    int activity_address_locking_peers_sent;
    Jxta_hashtable *activity_address_locked_peers;
    char *activity_address_locking_peer;
    Assign_state assign_state;
    Jxta_vector *free_hash_list;
    Jxta_vector *possible_free_list;
    int activity_address_free_peers_sent;
    Jxta_hashtable *activity_address_free_peers;

    /* ACTIVITY ADD */
    Jxta_boolean activity_add;
    Jxta_vector *activity_add_candidate_peers;
    Jxta_peer *activity_add_candidate_peer;
    unsigned int activity_add_candidate_pongs_sent;
    Jxta_vector * activity_add_candidate_voting_peers;
    Jxta_time activity_add_candidate_requests_expiration;
    Jxta_time activity_add_voting_ends;
    Jxta_boolean activity_add_voting;
    Jxta_Peerview_walk_policy walk_policy;
    int walk_peers;

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
static Jxta_status histogram_random_entry(Jxta_vector * histogram, BIGNUM *range, BIGNUM *min_bn, unsigned int attempts, BIGNUM **target_address);
static Jxta_status histogram_predictable_entry(Jxta_vector * histogram, Jxta_vector *hash_list, BIGNUM *range, BIGNUM *min_bn, BIGNUM *max_bn, BIGNUM ** target_hash);
static Jxta_status histogram_get_location( Jxta_vector * histogram, BIGNUM * address, Peerview_histogram_entry **before_entry, Peerview_histogram_entry ** after_entry);
static void histogram_get_empty_space(Jxta_vector * histogram, BIGNUM *min_bn, BIGNUM *max_bn, Jxta_vector ** empty_locations);

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
static void peerview_has_changed(Jxta_peerview * myself);

static Jxta_status peerview_maintain(Jxta_peerview *myself, Jxta_boolean all, Jxta_hashtable **ret_msgs);
static Jxta_vector * peerview_get_pvs(JString *request_group);
static Peerview_entry *peerview_get_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_boolean peerview_check_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_status peerview_add_pve(Jxta_peerview * myself, Peerview_entry * pve);
static Jxta_status peerview_remove_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_status peerview_remove_pve_1(Jxta_peerview * myself, Jxta_PID * pid, Jxta_boolean generate_event);
static Jxta_vector *peerview_get_all_pves(Jxta_peerview * myself);
static Jxta_status peerview_clear_pves(Jxta_peerview * myself, Jxta_boolean notify);
static Jxta_status peerview_get_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer, Jxta_peer **alt_peer, Jxta_vector **peers);

static Jxta_peerview_event *peerview_event_new(Jxta_Peerview_event_type event, Jxta_id * id);
static void peerview_event_delete(Jxta_object * ptr);

static Jxta_status peerview_init_cluster(Jxta_peerview * myself);

static Jxta_status JXTA_STDCALL peerview_protocol_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL peerview_protocol_consolidated_cb(Jxta_object * obj, void *arg);
static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req);
static Jxta_status peerview_handle_address_assign_msg(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);
static Jxta_status peerview_handle_ping(Jxta_peerview * myself, Jxta_peerview_ping_msg * ping, Jxta_boolean consolidated);
static Jxta_status peerview_handle_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);
static Jxta_boolean peerview_handle_promotion(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);
static Jxta_status peerview_handle_adv_request(Jxta_peerview * me, Jxta_adv_request_msg * req);
static Jxta_status peerview_handle_adv_response(Jxta_peerview * me, Jxta_adv_response_msg * resp);
static Jxta_status peerview_send_address_request(Jxta_peerview * myself, Jxta_peer * dest);
static Jxta_status peerview_send_ping_priv(Jxta_peerview *myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean id_only, Jxta_boolean peerid_only);
static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean id_only);
static Jxta_status peerview_send_ping_consolidated(Jxta_peerview * myself, Jxta_peer * dest, Jxta_peerview_ping_msg *ping);
static Jxta_status peerview_send_address_assign_msg(Jxta_peerview * myself, Jxta_peer * dest, Jxta_peer * assign_peer, Jxta_address_assign_msg_type type, BIGNUM * target_hash, Jxta_vector *free_hash_list);
static void peerview_send_address_assign_and_unlock(Jxta_peerview *myself, BIGNUM *target_hash, Jxta_vector *hash_list);

static Jxta_status peerview_handle_address_assign_free(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_free_rsp(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_assigned(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assignment(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);
static Jxta_status peerview_handle_address_assign_lock(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_unlock(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_busy(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_lock_rsp(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c);
static Jxta_status peerview_handle_address_assign_assigned_rsp(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);
static Jxta_status peerview_handle_address_assigned_rsp_negative(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);

static Jxta_status peerview_send_pvm_priv(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg, const char *parm, Jxta_boolean sync, Jxta_boolean peerid_only);
static Jxta_status peerview_send_pvm(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg);
static Jxta_status peerview_send_pvm_1(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg, Jxta_boolean sync);
static Jxta_status peerview_send_pvm_2(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg);
static Jxta_status peerview_send_pvm_ep(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg);
static Jxta_status peerview_send_adv_request(Jxta_peerview * myself, Jxta_id * dest_id, Jxta_vector *need_pids);
static Jxta_status peerview_send_pong(Jxta_peerview *myself, Jxta_peer *dest, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_boolean compact, Jxta_boolean disconnect);
static Jxta_status peerview_send_pong_1(Jxta_peerview *myself, Jxta_peer *dest, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_boolean compact, Jxta_boolean disconnect, Jxta_boolean sync);

static Jxta_status peerview_lock_address(Jxta_peerview * myself, Jxta_peer * peer);
static Jxta_status peerview_send_address_assigned(Jxta_peerview *myself, Jxta_peer *dest, BIGNUM *target_hash);
static void peerview_send_address_assign_unlock(Jxta_peerview *myself);

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
static int target_hash_sort(Peerview_entry **peer_a, Peerview_entry **peer_b);
static void find_best_target_address(Peerview_address_assign_mode mode, Jxta_vector * histogram, BIGNUM * min, BIGNUM * max,
unsigned int attempts, BIGNUM * range, BIGNUM ** target_address);

static Jxta_status half_the_free_list(Jxta_peerview *myself, int cluster, Jxta_vector ** half_list);
static void get_empty_cluster(Jxta_peerview *myself, int * cluster);

static Jxta_status build_histogram(Jxta_vector ** histo, Jxta_vector * peers_v, BIGNUM * minimum, BIGNUM * maximum);

static void *APR_THREAD_FUNC activity_peerview_locate(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_addressing(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_address_locking(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_announce(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_maintain(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_add(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_auto_cycle(apr_thread_t * thread, void *arg);

static Jxta_status JXTA_STDCALL peerview_monitor_cb(Jxta_object * obj, Jxta_monitor_entry **arg);

typedef enum _maintain_states {
    MAINTAIN_INITIALIZE=0,
    MAINTAIN_SHUTDOWN,
    MAINTAIN_RUNNING
} Maintain_state;

static Maintain_state maintain_state=MAINTAIN_INITIALIZE;

static int _maintain_thread_initialized=0;
static apr_pool_t *maintain_pool=NULL;
static apr_thread_t *maintain_thread=NULL;
static apr_thread_mutex_t *maintain_lock=NULL;
static apr_thread_mutex_t *maintain_cond_lock=NULL;
static apr_thread_cond_t *maintain_cond=NULL;
static Jxta_peerview *maintain_peerview=NULL;
static Jxta_endpoint_service *maintain_endpoint=NULL;
static Jxta_boolean maintain_running=FALSE;
static void *maintain_cookie;

static void initiate_maintain_thread(Jxta_peerview *myself)
{
    apr_status_t rv;
    Jxta_PG *parent=NULL;
    Jxta_PG *group=NULL;
    Jxta_rdv_service *rdv=NULL;
    Jxta_endpoint_service *ep = NULL;
    char *new_param=NULL;

    if (_maintain_thread_initialized++) {
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Starting maintain thread [%pp]\n", myself);

    group = myself->group;
    while (TRUE) {
        jxta_PG_get_parentgroup(group, &parent);
        if (NULL != parent) {
            group = parent;
        } else break;
    }
    jxta_PG_get_rendezvous_service(group, &rdv);
    maintain_peerview = jxta_rdv_service_get_peerview(rdv);
    jxta_PG_get_endpoint_service(group, &ep);
    maintain_endpoint=ep;
    new_param = get_service_key(RDV_V3_MSID, JXTA_PEERVIEW_NAME_CONS);
    
    rv = endpoint_service_add_recipient(maintain_endpoint, &maintain_cookie, JXTA_PEERVIEW_EP_NAME_CONS , new_param, peerview_protocol_consolidated_cb, maintain_peerview);

    free(new_param);

    apr_pool_create(&maintain_pool, NULL);
    apr_thread_cond_create(&maintain_cond, maintain_pool);
    apr_thread_mutex_create(&maintain_cond_lock, APR_THREAD_MUTEX_DEFAULT, maintain_pool);
    apr_thread_mutex_create(&maintain_lock, APR_THREAD_MUTEX_DEFAULT, maintain_pool);
    rv = apr_thread_create(&maintain_thread, NULL, activity_peerview_maintain, myself, maintain_pool);

    JXTA_OBJECT_RELEASE(rdv);
    JXTA_OBJECT_RELEASE(parent);
}

static void terminate_maintain_thread()
{

    if (!_maintain_thread_initialized) {
        return;
    }

    _maintain_thread_initialized--;
    if (_maintain_thread_initialized) {
        return;
    }
    endpoint_service_remove_recipient(maintain_endpoint, &maintain_cookie);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopping maintain thread\n");

    if (MAINTAIN_RUNNING == maintain_state) {
        apr_thread_mutex_lock(maintain_lock);
        maintain_state = MAINTAIN_SHUTDOWN;
        apr_thread_mutex_unlock(maintain_lock);
        apr_thread_mutex_lock(maintain_cond_lock);
        apr_thread_cond_signal(maintain_cond);
        apr_thread_mutex_unlock(maintain_cond_lock);
    }

    if (maintain_peerview)
        JXTA_OBJECT_RELEASE(maintain_peerview);
    if (maintain_endpoint)
        JXTA_OBJECT_RELEASE(maintain_endpoint);
    maintain_peerview=NULL;
    maintain_endpoint=NULL;

    apr_thread_mutex_destroy(maintain_cond_lock);
    apr_thread_mutex_destroy(maintain_lock);

    maintain_lock=NULL;
    maintain_cond=NULL;
    maintain_cond_lock=NULL;

}

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
        myself->walk_counter = 1;
        myself->last_walk_select = jpr_time_now();
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
    if (NULL != myself->free_hash_list) {
        JXTA_OBJECT_RELEASE(myself->free_hash_list);
        myself->free_hash_list = NULL;
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
        myself->pid_c = NULL;
        myself->pid_j = NULL;
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

        myself->free_hash_list = jxta_vector_new(0);
        myself->activity_address_locked_peers = jxta_hashtable_new(0);
        myself->activity_address_free_peers = jxta_hashtable_new(0);
        myself->possible_free_list = jxta_vector_new(0);
        myself->assign_state = ASSIGN_IDLE;

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

    if (NULL != myself->pid_j) {
        JXTA_OBJECT_RELEASE(myself->pid_j);
    }

    if (NULL != myself->pid_c) {
        free(myself->pid_c);
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
            if (myself->clusters[each_cluster].next_address) {
                BN_free(myself->clusters[each_cluster].next_address);
                myself->clusters[each_cluster].next_address = NULL;
            }
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

    if (NULL != myself->activity_address_locked_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_address_locked_peers);
    }

    if (NULL != myself->activity_address_free_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_address_free_peers);
    }

    if (NULL != myself->activity_address_assign_peer) {
        JXTA_OBJECT_RELEASE(myself->activity_address_assign_peer);
    }

    if (NULL != myself->free_hash_list) {
        JXTA_OBJECT_RELEASE(myself->free_hash_list);
    }

    if (NULL != myself->possible_free_list) {
        JXTA_OBJECT_RELEASE(myself->possible_free_list);
    }

    if (NULL != myself->hash_space) {
        BN_free(myself->hash_space);
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
    jxta_id_to_jstring(myself->pid, &string);
    myself->pid_j = JXTA_OBJECT_SHARE(string);
    myself->pid_c = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);
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
    if (-1 == jxta_RdvConfig_pv_get_address_assign_mode(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_address_assign_mode(myself->rdvConfig, config_addr_assign_random);
    }
    if (-1 == jxta_RdvConfig_pv_get_walk_policy(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_walk_policy(myself->rdvConfig, JXTA_PV_WALK_POLICY_LOAD);
    }
    if (-1 == jxta_RdvConfig_pv_get_walk_peers(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_walk_peers(myself->rdvConfig, 1);
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

    if (-1 == jxta_RdvConfig_pv_address_assign_expiration(myself->rdvConfig)) {
        jxta_RdvConfig_pv_set_address_assign_expiration(myself->rdvConfig, JXTA_PEERVIEW_ADDRESS_ASSIGN_EXPIRATION);
    }
    myself->clusters_count = jxta_RdvConfig_pv_clusters(myself->rdvConfig);
    myself->cluster_members = jxta_RdvConfig_pv_members(myself->rdvConfig);
    myself->replicas_count = jxta_RdvConfig_pv_replication(myself->rdvConfig);
    /**
     * Number of attempts that will be made to find a suitable random hashspace address
     */
    myself->hash_attempts = (myself->replicas_count << 1) + 1;
    /**
     * Number of peers we will invite into the peerview with each add run.
     **/
    myself->max_client_invitations = myself->clusters_count;
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
    BIGNUM *cluster_members;
    BIGNUM *replicas_count;
    char *tmp;

    /* initialize the cluster divisor */
    clusters_count = BN_new();
    myself->hash_space = BN_new();
    myself->cluster_divisor = BN_new();
    BN_set_word(clusters_count, myself->clusters_count);
    BN_lshift(myself->hash_space, BN_value_one(), CHAR_BIT * SHA_DIGEST_LENGTH);
    tmp = BN_bn2hex(myself->hash_space);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Hash Space : %s\n", tmp);
    free(tmp);
    BN_div(myself->cluster_divisor, NULL, myself->hash_space, clusters_count, ctx);
    tmp = BN_bn2hex(myself->cluster_divisor);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Cluster Divisor : %s\n", tmp);
    free(tmp);
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
    Jxta_monitor_service * monitor;
    Jxta_rdv_service *rdv=NULL;

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
    myself->walk_policy = jxta_RdvConfig_pv_get_walk_policy(myself->rdvConfig);
    myself->walk_peers = jxta_RdvConfig_pv_get_walk_peers(myself->rdvConfig);

    peerview_has_changed(myself);

    if (myself->auto_cycle > 0) {
        myself->iterations_since_switch = 0;
        myself->loneliness_factor = 0;
        res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_auto_cycle, myself, myself->auto_cycle, &myself->auto_cycle);

        if (APR_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "[%pp]: Could not schedule auto cycle activity : %d\n", myself, res);
        }

    }

    monitor = jxta_monitor_service_get_instance();

    if (NULL != monitor) {
        jxta_monitor_service_register_callback(monitor, peerview_monitor_cb, "jxta:PV3MonEntry" , (Jxta_object *) pv);
    }
    jxta_PG_get_rendezvous_service(myself->group, &rdv);
    initiate_maintain_thread(myself);

    apr_thread_mutex_unlock(myself->mutex);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview started for group %s / %s \n", myself->groupUniqueID,
                    myself->assigned_id_str);
    }
    if (rdv)
        JXTA_OBJECT_RELEASE(rdv);

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

Jxta_status peerview_get_peer_address(Jxta_peer *peer, BIGNUM **address)
{
    Peerview_entry *pve;

    pve = (Peerview_entry *) peer;

    if (pve->target_hash) {
        *address = BN_dup(pve->target_hash);
    } else {
        *address = BN_new();
    }
    return JXTA_SUCCESS;
}

static Jxta_status peerview_reset_walk_counters(Jxta_peerview * myself)
{
    Jxta_status ret=JXTA_SUCCESS;
    Peerview_entry *entry=NULL;
    Jxta_vector *pves=NULL;
    int i;

    apr_thread_mutex_lock(myself->mutex);
    pves = jxta_hashtable_values_get(myself->pves);
    for (i=0; i<jxta_vector_size(pves); i++) {

        ret = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != ret) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve a peerview entry ret:%d\n", ret);
            continue;
        }
        entry->walk_counter = 1;
        JXTA_OBJECT_RELEASE(entry);
    }
    apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_RELEASE(pves);
    return ret;
}

static void peerview_has_changed(Jxta_peerview * myself)
{
    Jxta_status ret;

    ret = peerview_reset_walk_counters(myself);
    if (JXTA_SUCCESS != ret) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unable to reset the walk counters ret:%d\n", ret);
    }

    peerview_update_id(myself);

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

    terminate_maintain_thread();
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

static Jxta_boolean is_in_view(Jxta_vector *view, Jxta_id *id)
{
    Jxta_boolean result=FALSE;
    int i;

    for (i = 0; i < jxta_vector_size(view); i++) {
        Jxta_peer *peer;

        jxta_vector_get_object_at(view, JXTA_OBJECT_PPTR(&peer), i);
        if (jxta_id_equals(id, jxta_peer_peerid(peer))) {
            result = TRUE;
        }
        JXTA_OBJECT_RELEASE(peer);
        if (result) break;
    }

    return result;

}
JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_associate(Jxta_peerview * me, Jxta_id * id)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_boolean result=FALSE;
    Jxta_vector *view=NULL;

    apr_thread_mutex_lock(myself->mutex);

    jxta_peerview_get_globalview(myself, &view);
    result = is_in_view(view, id);

    apr_thread_mutex_unlock(myself->mutex);

    if (view)
        JXTA_OBJECT_RELEASE(view);
    return result;

}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_is_partner(Jxta_peerview * me, Jxta_id * id)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_boolean result;
    Jxta_vector *view=NULL;

    apr_thread_mutex_lock(myself->mutex);

    jxta_peerview_get_localview(myself, &view);

    result = is_in_view(view, id);

    apr_thread_mutex_unlock(myself->mutex);

    if (view)
        JXTA_OBJECT_RELEASE(view);

    return result;
}

static int peerview_walk_policy_lru_compare(Peerview_entry **peer_a, Peerview_entry **peer_b)
{
    if ((*peer_a)->last_walk_select < (*peer_b)->last_walk_select) {
        return -1;
    }
    if ((*peer_a)->last_walk_select > (*peer_b)->last_walk_select) {
        return 1;
    }
    return 0;
}

static int peerview_walk_policy_load_compare(Peerview_entry **peer_a, Peerview_entry **peer_b)
{
    if ((*peer_a)->walk_counter < (*peer_b)->walk_counter) {
        return -1;
    }
    if ((*peer_a)->walk_counter > (*peer_b)->walk_counter) {
        return 1;
    }
    return 0;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_select_walk_peers(Jxta_peerview * me, Jxta_vector * candidates, Jxta_Peerview_walk_policy policy, Jxta_vector **walk_peers)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_status ret=JXTA_SUCCESS;
    int i;
    Jxta_Peerview_walk_policy walk_policy;
    Jxta_vector * candidates_clone=NULL;
    Jxta_vector * candidates_pve=NULL;

    ret = jxta_vector_clone(candidates, &candidates_clone, 0, INT_MAX);
    if (JXTA_SUCCESS != ret) {
        goto FINAL_EXIT;
    }
    /* if the caller wants to use the configured policy use it */
    walk_policy = JXTA_PV_WALK_POLICY_CONFIGURED == policy ? myself->walk_policy:policy;
    *walk_peers = jxta_vector_new(0);

    /* remove the associates from the candidates and add them to the list */
    for (i = 0; i < jxta_vector_size(candidates_clone); i++) {
        Jxta_id *candidate=NULL;
        Peerview_entry *pve=NULL;

        ret = jxta_vector_get_object_at(candidates_clone, JXTA_OBJECT_PPTR(&candidate), i);
        if (JXTA_SUCCESS != ret) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE " Unable to retrieve candidate for walk\n");
            continue;
        }
        pve = peerview_get_pve(myself, candidate);
        if (NULL == pve || jxta_peer_get_expires((Jxta_peer *) pve) < jpr_time_now()) {
            JString *pid_j=NULL;

            jxta_id_to_jstring(candidate, &pid_j);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, " Candidate %s is not in the peerview\n", jstring_get_string(pid_j));
            jxta_vector_remove_object_at(candidates_clone, NULL, i--);
            if (pve)
                JXTA_OBJECT_RELEASE(pve);
            JXTA_OBJECT_RELEASE(candidate);
            JXTA_OBJECT_RELEASE(pid_j);
            continue;
        }
        /* always add an associate */
        if (jxta_peerview_is_associate(myself, candidate)) {
            jxta_vector_remove_object_at(candidates_clone, NULL, i--);
            jxta_vector_add_object_last(*walk_peers, (Jxta_object *) candidate);
        } else {
            /* need the peer objects */
            if (NULL == candidates_pve) {
                candidates_pve = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(candidates_pve, (Jxta_object *) pve);
        }
        JXTA_OBJECT_RELEASE(pve);
        JXTA_OBJECT_RELEASE(candidate);
    }

    if (NULL == candidates_pve)
    {
        goto FINAL_EXIT;
    }
    
    /* sort the list by the policy */
    switch (walk_policy) {
    case JXTA_PV_WALK_POLICY_LRU:
        jxta_vector_qsort(candidates_pve, (Jxta_object_compare_func) peerview_walk_policy_lru_compare);
        break;
    case JXTA_PV_WALK_POLICY_LOAD:
    /* use load factor for the QoS until QoS implemented */
    case JXTA_PV_WALK_POLICY_QOS:
    default:
        jxta_vector_qsort(candidates_pve, (Jxta_object_compare_func) peerview_walk_policy_load_compare);
        break;
    }

    /* retrieve the number of peers required */
    for (i=0; i < jxta_vector_size(candidates_pve); i++) {
        Jxta_peer *walk_peer=NULL;
        Jxta_id *walk_id=NULL;
        JString *pid_j=NULL;

        ret = jxta_vector_get_object_at(candidates_pve, JXTA_OBJECT_PPTR(&walk_peer), i);
        if (JXTA_SUCCESS != ret) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE " Unable to retrieve candidate pve for walk\n");
            continue;
        }
        jxta_peer_get_peerid((Jxta_peer *) walk_peer, &walk_id);
        jxta_vector_add_object_last(*walk_peers, (Jxta_object *) walk_id);

        jxta_id_to_jstring(walk_id, &pid_j);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding %s to the walk_peers\n", jstring_get_string(pid_j));

        JXTA_OBJECT_RELEASE(walk_peer);
        JXTA_OBJECT_RELEASE(walk_id);
        JXTA_OBJECT_RELEASE(pid_j);

        if (i == (myself->walk_peers - 1)) break;
    }

FINAL_EXIT:
    if (candidates_clone)
        JXTA_OBJECT_RELEASE(candidates_clone);
    if (candidates_pve)
        JXTA_OBJECT_RELEASE(candidates_pve);
    return ret;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_flag_walk_peer_usage(Jxta_peerview * me, Jxta_id *id)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    Jxta_status ret=JXTA_SUCCESS;
    Peerview_entry *pve=NULL;

    pve = peerview_get_pve(myself, id);
    if (NULL == pve) {
        return JXTA_ITEM_NOTFOUND;
    }
    /* handle wrap-around */
    if (pve->walk_counter >= (INT_MAX - 1) ) {
        peerview_reset_walk_counters(myself);
    }
    pve->walk_counter++;

    pve->last_walk_select = jpr_time_now();

    JXTA_OBJECT_RELEASE(pve);

    return ret;
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
                                       apr_time_from_sec(1 << me->activity_locate_probes), me);

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
    return peerview_get_for_target_hash(me,target_hash, peer, NULL, NULL);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_peer_for_target_hash_1(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer, Jxta_peer ** alt_peer)
{
    return peerview_get_for_target_hash(me, target_hash, peer, alt_peer, NULL);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_peers_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer **peer, Jxta_vector ** peers)
{
    return peerview_get_for_target_hash(me,target_hash, peer, NULL, peers);
}

static Jxta_status peerview_get_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer, Jxta_peer **alt_peer, Jxta_vector **peers)
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
            BIGNUM *worst_distance = BN_new();

            if (NULL != peer) {
                *peer = JXTA_OBJECT_SHARE(myself->self_pve);
            }
            BN_sub(best_distance, bn_target_hash, myself->self_pve->target_hash);
            BN_add(worst_distance, bn_target_hash, myself->self_pve->target_hash);

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
                    if (BN_ucmp(distance, worst_distance) > 0) {
                        if (NULL != alt_peer) {
                            if (*alt_peer)
                                JXTA_OBJECT_RELEASE(*alt_peer);
                            *alt_peer = JXTA_OBJECT_SHARE(candidate);
                        }
                        BN_copy(worst_distance, distance);
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
            BN_free(worst_distance);
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

static Jxta_status peerview_create_ping_msg(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean pv_id_only, Jxta_peerview_ping_msg **ping)
{
    Jxta_status res=JXTA_SUCCESS;

    *ping = jxta_peerview_ping_msg_new();

    jxta_peerview_ping_msg_set_src_peer_id(*ping, myself->pid);

    if (jxta_peer_peerid(dest)) {
        jxta_peerview_ping_msg_set_dst_peer_id(*ping, jxta_peer_peerid(dest));
    } else {
        assert(jxta_peer_address(dest));
        jxta_peerview_ping_msg_set_dst_peer_ea(*ping, jxta_peer_address(dest));
    }

    jxta_peerview_ping_msg_set_pv_id_only(*ping, pv_id_only);

    jxta_peerview_ping_msg_set_dest_peer_adv_gen(*ping, adv_gen);

    JXTA_OBJECT_CHECK_VALID(*ping);

    return res;
}

static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean pv_id_only)
{
    Jxta_status res;
    Jxta_peerview_ping_msg *ping=NULL;
    Jxta_message *msg=NULL;
    Jxta_message_element *el;
    JString *ping_xml=NULL;

    peerview_create_ping_msg(myself, dest, adv_gen, pv_id_only, &ping);

    res = jxta_peerview_ping_msg_get_xml(ping, &ping_xml);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to generate ping xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(ping_xml), jstring_length(ping_xml), NULL);
    JXTA_OBJECT_RELEASE(ping_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peerview_send_ping [%pp]\n", myself);

    res = peerview_send_pvm(myself, dest, msg);

FINAL_EXIT:

    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    if (ping)
        JXTA_OBJECT_RELEASE(ping);

    return res;
}

static Jxta_status peerview_send_ping_consolidated(Jxta_peerview * myself, Jxta_peer * dest, Jxta_peerview_ping_msg *ping)
{
    Jxta_status res;
    Jxta_message *msg=NULL;
    Jxta_message_element *el;
    JString *ping_xml=NULL;

    res = jxta_peerview_ping_msg_get_xml(ping, &ping_xml);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to generate ping xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(ping_xml), jstring_length(ping_xml), NULL);
    JXTA_OBJECT_RELEASE(ping_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peerview_send_ping_consolidated [%pp]\n", myself);

    res = peerview_send_pvm_ep(myself, dest, msg);

FINAL_EXIT:
    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_send_ping_2(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean peer_id_only)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peerview_send_ping_2 [%pp]\n", myself);

    return peerview_send_ping_priv(myself, dest, adv_gen, FALSE, peer_id_only);
}

static Jxta_status peerview_send_ping_priv(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen, Jxta_boolean pv_id_only, Jxta_boolean peerid_only)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_peerview_ping_msg *ping;
    JString *ping_xml = NULL;

    peerview_create_ping_msg(myself, dest, adv_gen, pv_id_only, &ping);

    res = jxta_peerview_ping_msg_get_xml(ping, &ping_xml);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to generate ping xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(ping_xml), jstring_length(ping_xml), NULL);
    JXTA_OBJECT_RELEASE(ping_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    if (peerid_only) {
        res = peerview_send_pvm_2(myself, dest, msg);
    } else {
        res = peerview_send_pvm(myself, dest, msg);
    }

FINAL_EXIT:
    if (ping)
        JXTA_OBJECT_RELEASE(ping);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status peerview_create_pong_msg(Jxta_peerview * myself, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_peerview_pong_msg **pong_msg, Jxta_boolean disconnect)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview_pong_msg *pong = NULL;
    Jxta_PA * pa;
    char *tmp;
    unsigned int i;
    Jxta_vector *peers=NULL;
    apr_uuid_t adv_gen;
    Jxta_rdv_lease_options *rdv_options=NULL;

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

    if(disconnect) {
        jxta_peerview_pong_msg_set_rendezvous(pong, FALSE);
    }
    else {
        jxta_peerview_pong_msg_set_rendezvous(pong, jxta_rdv_service_is_rendezvous(myself->rdv));
    }

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

    *pong_msg = pong;

    return res;
}

JXTA_DECLARE(Jxta_status) peerview_send_disconnect(Jxta_peerview * pv, Jxta_peer * dest)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);
    /*
    *   Sends a pong message with an action of demote,
    *   doesn't send any candidates, sends the message as
    *   a client to signal a disconnect
    */
    return peerview_send_pong_1(myself, dest, PONG_DEMOTE, FALSE, FALSE, TRUE, TRUE);
}

JXTA_DECLARE(Jxta_status) peerview_handle_free_hash_list_attr(const char * ptr, Jxta_vector * free_v, Jxta_boolean print)
{
    Jxta_status res=JXTA_SUCCESS;
    char tmp[64];
    int next;
    char tmp1;

    memset (tmp, 0, sizeof(tmp));
    next = 0;
    while (*ptr) {
        tmp1 = *ptr++;
        if (',' == tmp1 || '\0' == (char) *ptr) {
            JString *free_entry=NULL;
            BIGNUM *target_bn=NULL;
            char * tmp_hash=NULL;

            if ('\0' == *ptr) tmp[next] = tmp1;
            BN_hex2bn(&target_bn, tmp);
            BN_lshift(target_bn, target_bn, 140);
            tmp_hash = BN_bn2hex(target_bn);
            free_entry = jstring_new_2(tmp_hash);
            if (print)
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Received %s free_hash entry\n", jstring_get_string(free_entry));
            jxta_vector_add_object_last(free_v, (Jxta_object *) free_entry);
            next = 0;
            memset(tmp, 0, sizeof(tmp));

            free(tmp_hash);
            BN_free(target_bn);
            JXTA_OBJECT_RELEASE(free_entry);
        } else {
            tmp[next++] = tmp1;
        }
    }
    return res;
}

static Jxta_status peerview_send_pong(Jxta_peerview * myself, Jxta_peer * dest, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_boolean compact, Jxta_boolean disconnect)
{
    return peerview_send_pong_1(myself, dest, action, candidates_only, compact, disconnect, FALSE);
}


static Jxta_status peerview_send_pong_1(Jxta_peerview * myself, Jxta_peer * dest, Jxta_pong_msg_action action, Jxta_boolean candidates_only, Jxta_boolean compact, Jxta_boolean disconnect, Jxta_boolean sync)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview_pong_msg *pong;
    JString *pong_msg_xml;
    Jxta_message *msg;
    Jxta_message_element *el = NULL;

    apr_thread_mutex_lock(myself->mutex);

    if ((NULL == myself->instance_mask) || (NULL == myself->self_pve)
        || (0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Can't send pong message now! [%pp]\n", myself);
        res = JXTA_FAILED;
        apr_thread_mutex_unlock(myself->mutex);
        return res;
    }

    res = peerview_create_pong_msg(myself, action, candidates_only, &pong, disconnect);

    apr_thread_mutex_unlock(myself->mutex);

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    if (!compact) {
        jxta_peerview_pong_msg_set_free_hash_list(pong, myself->free_hash_list);
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

    res = peerview_send_pvm_1(myself, dest, msg, sync);

    JXTA_OBJECT_RELEASE(msg);

  FINAL_EXIT:

    return res;
}

static Jxta_status peerview_send_pong_2(Jxta_peerview * myself, Jxta_peer * dest, Jxta_hashtable * msgs)
{
    Jxta_status res=JXTA_SUCCESS;
    char **groups=NULL;
    char **groups_save=NULL;
    Jxta_endpoint_message *ep_msg=NULL;

    ep_msg = jxta_endpoint_msg_new();
    groups = jxta_hashtable_keys_get(msgs);
    groups_save = groups;
    while (NULL != groups && *groups) {
        JString *pong_msg_xml=NULL;

        if (JXTA_SUCCESS == jxta_hashtable_get(msgs, *groups, strlen(*groups) + 1, JXTA_OBJECT_PPTR(&pong_msg_xml))) {
            Jxta_message_element *msg_elem=NULL;
            Jxta_endpoint_msg_entry_element *elem = NULL;

            elem = jxta_endpoint_message_entry_new();
            msg_elem = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PONG_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(pong_msg_xml), jstring_length(pong_msg_xml), NULL);

            jxta_endpoint_msg_entry_set_value(elem, msg_elem);
            jxta_endpoint_msg_entry_add_attribute(elem, "groupid", *groups);
            jxta_endpoint_msg_add_entry(ep_msg, elem);

            JXTA_OBJECT_RELEASE(msg_elem);
            JXTA_OBJECT_RELEASE(elem);
        }
        if (pong_msg_xml)
            JXTA_OBJECT_RELEASE(pong_msg_xml);
        free(*(groups++));
    }
    while (NULL != groups && *groups) free(*(groups++));


    res = jxta_PG_async_send_1(myself->group, ep_msg, jxta_peer_peerid(dest), "groupid", RDV_V3_MSID, JXTA_PEERVIEW_NAME);


    if (groups_save)
        free(groups_save);
    JXTA_OBJECT_RELEASE(ep_msg);

    return res;
}

static Jxta_status peerview_send_address_assign_msg(Jxta_peerview * myself, Jxta_peer * dest, Jxta_peer * assign_peer, Jxta_address_assign_msg_type type, BIGNUM * target_hash, Jxta_vector *free_hash_list)
{
    Jxta_status res;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_peerview_address_assign_msg *addr_assign;
    JString *addr_assign_xml = NULL;
    char *temp;
    JString *dest_peerid_j;
    JString *assign_peerid_j;


    addr_assign = jxta_peerview_address_assign_msg_new();

    jxta_peerview_address_assign_msg_set_peer_id(addr_assign, myself->pid);

    temp = BN_bn2hex(myself->instance_mask);
    jxta_peerview_address_assign_msg_set_instance_mask(addr_assign, temp);
    free(temp);

    jxta_peerview_address_assign_msg_set_type(addr_assign, type);

    if (NULL != assign_peer) {
        jxta_peerview_address_assign_msg_set_assign_peer_id(addr_assign, jxta_peer_peerid(assign_peer));
    }
    if (NULL != myself->clusters[myself->my_cluster].members) {
        jxta_peerview_address_assign_msg_set_cluster_peers(addr_assign, jxta_vector_size(myself->clusters[myself->my_cluster].members));
    }

    /* add specific info for the message type */
    switch (type) {
        case ADDRESS_ASSIGN:
            if (NULL != free_hash_list) {
                jxta_peerview_address_assign_msg_set_free_hash_list(addr_assign, free_hash_list, FALSE);
            }
            break;
        case ADDRESS_ASSIGN_LOCK:
            jxta_peerview_address_assign_msg_set_expiraton(addr_assign, jxta_RdvConfig_pv_address_assign_expiration(myself->rdvConfig));
            break;
        case ADDRESS_ASSIGN_LOCK_RSP:
            if (NULL != free_hash_list) {
                jxta_peerview_address_assign_msg_set_free_hash_list(addr_assign, free_hash_list, FALSE);
            }
            break;
        case ADDRESS_ASSIGN_UNLOCK:
            break;
        case ADDRESS_ASSIGN_BUSY:
            break;
        case ADDRESS_ASSIGN_ASSIGNED:
            break;
        case ADDRESS_ASSIGN_ASSIGNED_RSP:
            if (NULL != free_hash_list) {
                jxta_peerview_address_assign_msg_set_free_hash_list(addr_assign, free_hash_list, FALSE);
            }
            break;
        case ADDRESS_ASSIGN_FREE:
            if (NULL != free_hash_list) {
                jxta_peerview_address_assign_msg_set_free_hash_list(addr_assign, free_hash_list, TRUE);
            }
            break;
        case ADDRESS_ASSIGN_FREE_RSP:
            if (NULL != free_hash_list) {
                jxta_peerview_address_assign_msg_set_free_hash_list(addr_assign, free_hash_list, FALSE);
            }
            break;
        case ADDRESS_ASSIGN_STATUS:
            break;
        default:
            break;
    }

    if (NULL != target_hash) {
        temp = BN_bn2hex(target_hash);
        jxta_peerview_address_assign_msg_set_target_hash(addr_assign, temp);
        free(temp);
    }

    res = jxta_peerview_address_assign_msg_get_xml(addr_assign, &addr_assign_xml);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate address assign xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    jxta_id_to_jstring(jxta_peer_peerid(dest), &dest_peerid_j);
    if (NULL != assign_peer) {
        jxta_id_to_jstring(jxta_peer_peerid(assign_peer), &assign_peerid_j);
    } else {
        assign_peerid_j = jstring_new_2("No assign peer");
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending addr assign %s to %s for %s\n"
                            , jxta_peerview_address_assign_msg_type_text(addr_assign)
                            , jstring_get_string(dest_peerid_j), jstring_get_string(assign_peerid_j));
    JXTA_OBJECT_RELEASE(dest_peerid_j);
    JXTA_OBJECT_RELEASE(assign_peerid_j);

    el = jxta_message_element_new_2(JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_ADDRESS_ASSIGN_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(addr_assign_xml), jstring_length(addr_assign_xml), NULL);
    JXTA_OBJECT_RELEASE(addr_assign_xml);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    res = peerview_send_pvm(myself, dest, msg);

FINAL_EXIT:

    JXTA_OBJECT_RELEASE(addr_assign);
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static JString *build_adv_req_query_string(Jxta_vector *need_pids)
{
    JString *query_string=NULL;
    int i;

    query_string = jstring_new_0();
    jstring_append_2(query_string, "/jxta:PA[");
    for (i=0; i<jxta_vector_size(need_pids); i++) {
        Jxta_status status;
        JString *jPeerid;
        Jxta_id *id;

        status = jxta_vector_get_object_at(need_pids, JXTA_OBJECT_PPTR(&id), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,"Unable to retrieve object from need_pids : %d\n", status);
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

    return query_string;
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
    Jxta_peer * dest;
    adv_req_msg = jxta_adv_request_msg_new();

    query_string = build_adv_req_query_string(need_pids);

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
    return peerview_send_pvm_priv(me, dest, msg, JXTA_PEERVIEW_NAME, FALSE, FALSE);
}

static Jxta_status peerview_send_pvm_1(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg, Jxta_boolean sync)
{
    return peerview_send_pvm_priv(me, dest, msg, JXTA_PEERVIEW_NAME, sync, FALSE);
}

static Jxta_status peerview_send_pvm_2(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg)
{
    return peerview_send_pvm_priv(me, dest, msg, JXTA_PEERVIEW_NAME, FALSE, TRUE);
}

static Jxta_status peerview_send_pvm_ep(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg)
{
    Jxta_status rv;
    char *new_param;
    JString *ep_name_j;
    Jxta_endpoint_address *dest_ea;

    ep_name_j = jstring_new_2(JXTA_PEERVIEW_EP_NAME_CONS);

    new_param = get_service_key(RDV_V3_MSID, JXTA_PEERVIEW_NAME_CONS);
    dest_ea = jxta_endpoint_address_new_3(jxta_peer_peerid(dest), jstring_get_string(ep_name_j), new_param);

    rv = jxta_endpoint_service_send_ex(maintain_endpoint, msg, dest_ea, JXTA_FALSE);

    free(new_param);
    JXTA_OBJECT_RELEASE(ep_name_j);
    JXTA_OBJECT_RELEASE(dest_ea);

    return rv;
}

static Jxta_status peerview_send_pvm_priv(Jxta_peerview * me, Jxta_peer * dest, Jxta_message * msg, const char *parm, Jxta_boolean sync, Jxta_boolean peerid_only)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_id *peerid;
    Jxta_endpoint_address *dest_addr;
    Jxta_endpoint_address *peer_addr;
    char *addr_str;
    JString *pid_jstr;
    Jxta_PA *peer_adv=NULL;
/*    jxta_peer_get_adv(dest, &peer_adv);
    if (NULL != peer_adv) {
        res = discovery_service_publish(me->discovery, (Jxta_advertisement *) peer_adv , DISC_PEER, 5L * 60L * 1000L, LOCAL_ONLY_EXPIRATION);
    }
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to publish peer advertisement\n");
    }
*/
    peerid = jxta_peer_peerid(dest);
    if (peerid) {

        jxta_id_to_jstring(peerid, &pid_jstr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending peerview message[%pp] by peerid to : %s\n", msg,
                        jstring_get_string(pid_jstr));
        JXTA_OBJECT_RELEASE(pid_jstr);
        if (sync) {
            res = jxta_PG_sync_send(me->group, msg, peerid, RDV_V3_MSID, parm);
        } else {
            res = jxta_PG_async_send(me->group, msg, peerid, RDV_V3_MSID, parm);
        }
    } else {
        peer_addr = jxta_peer_address(dest);
        res = jxta_PG_get_recipient_addr(me->group, jxta_endpoint_address_get_protocol_name(peer_addr),
                                         jxta_endpoint_address_get_protocol_address(peer_addr),
                                         RDV_V3_MSID, JXTA_PEERVIEW_NAME, &dest_addr);
        if (JXTA_SUCCESS == res) {
            int i;
            Jxta_boolean send = TRUE;

            addr_str = jxta_endpoint_address_to_string(dest_addr);

            /* only send if there is no peerid in the peerview */
            if (peerid_only) {
                Jxta_vector *peerids=NULL;
                /* get any peerids for this protocol/address */
                jxta_endpoint_service_get_connection_peers(me->endpoint, jxta_endpoint_address_get_protocol_name(peer_addr),
                                         jxta_endpoint_address_get_protocol_address(peer_addr), &peerids);

                for (i=0; peerid_only && i < jxta_vector_size(peerids); i++) {
                    Jxta_id *peerid_conn = NULL;

                    if (JXTA_SUCCESS != jxta_vector_get_object_at(peerids, JXTA_OBJECT_PPTR(&peerid_conn), i)) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve peerid from connections\n");
                        continue;
                    }

                    /* if this peer exists in  the peerview there's no reason to send this message */
                    if (jxta_peerview_is_partner(me, peerid_conn) || jxta_peerview_is_associate(me, peerid_conn)) {
                        send = FALSE;
                    }
                    if (peerid_conn)
                        JXTA_OBJECT_RELEASE(peerid_conn);
                    if (!send) {
                        /* if an entry was found and we shouldn't send this message stop searching */
                        break;
                    }
                }
                if (peerids)
                    JXTA_OBJECT_RELEASE(peerids);
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s Sending peerview message [%pp] to : %s\n", send == TRUE ? "":"Not", msg, addr_str);
            free(addr_str);
            if (send) {
                res = jxta_endpoint_service_send_ex(me->endpoint, msg, dest_addr, sync);
            }
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

static Jxta_status JXTA_STDCALL peerview_protocol_consolidated_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *value;
    JString *string=NULL;
    Jxta_peerview_ping_msg *ping=NULL;

    apr_thread_mutex_lock(myself->mutex);
    myself->processing_callbacks++;
    apr_thread_mutex_unlock(myself->mutex);

    res = jxta_message_get_element_2(msg, JXTA_PEERVIEW_NS_NAME, JXTA_PEERVIEW_PING_ELEMENT_NAME, &el);
    if (JXTA_ITEM_NOTFOUND == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Recieved a message element not a ping on the consolidated call back\n");
        goto FINAL_EXIT;
    }

    value = jxta_message_element_get_value(el);
    string = jstring_new_3(value);
    JXTA_OBJECT_RELEASE(value);

    ping = jxta_peerview_ping_msg_new();
    res = jxta_peerview_ping_msg_parse_charbuffer(ping, jstring_get_string(string), jstring_length(string));

    if (JXTA_SUCCESS == res) {
        res = peerview_handle_ping(myself, ping, TRUE);
    }

    apr_thread_mutex_lock(myself->mutex);
    myself->processing_callbacks--;
    apr_thread_mutex_unlock(myself->mutex);

    JXTA_OBJECT_RELEASE(ping);


FINAL_EXIT:
    if (string)
        JXTA_OBJECT_RELEASE(string);
    if (el)
        JXTA_OBJECT_RELEASE(el);

    return JXTA_SUCCESS;
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
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "[%pp] Peerview received a message[%pp]\n", myself, msg);

    JXTA_OBJECT_CHECK_VALID(msg);


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
                res = peerview_handle_address_assign_msg(myself, addr_assign);
            }

            JXTA_OBJECT_RELEASE(addr_assign);
        }
        break;
 
    case PING:{
            Jxta_peerview_ping_msg *ping = jxta_peerview_ping_msg_new();

            res = jxta_peerview_ping_msg_parse_charbuffer(ping, jstring_get_string(string), jstring_length(string));

            if (JXTA_SUCCESS == res) {
                res = peerview_handle_ping(myself, ping, FALSE);
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

static void print_histogram_details(Jxta_vector *histogram)
{
    int i;
    Jxta_status status;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Histogram size %d\n", jxta_vector_size(histogram));

    for (i=0; i < jxta_vector_size(histogram); i++) {
        Peerview_histogram_entry *entry;
        char *tmp_start;
        char *tmp_end;
        char *tmp_diff;
        char *tmp_target_hash;
        BIGNUM *bn_diff;
        int j;

        status = jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
        }
        tmp_start = BN_bn2hex(entry->start);
        tmp_end = BN_bn2hex(entry->end);
        bn_diff = BN_new();
        BN_sub(bn_diff, entry->end, entry->start);
        tmp_diff = BN_bn2hex(bn_diff);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "** histo s:%s e:%s diff:%s \n",tmp_start, tmp_end, tmp_diff);
        free(tmp_diff);
        free(tmp_start);
        free(tmp_end);
        BN_free(bn_diff);
        for (j=0; j<jxta_vector_size(entry->peers); j++) {
            JString *peer_j;
            Jxta_peer *peer;

            jxta_vector_get_object_at(entry->peers, JXTA_OBJECT_PPTR(&peer), j);
            jxta_id_to_jstring(jxta_peer_peerid(peer), &peer_j);
            tmp_target_hash = BN_bn2hex(((Peerview_entry *)peer)->target_hash);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "--------> peer:%s  --------> %s\n", jstring_get_string(peer_j), tmp_target_hash);
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(peer_j);
            free(tmp_target_hash);
        }
        JXTA_OBJECT_RELEASE(entry);
    }
}

static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_boolean locked;
    Peerview_entry *pve = NULL;
    Jxta_PA *dest_pa=NULL;
    Jxta_id *dest_id=NULL;
    Jxta_peer *dest=NULL;
    BIGNUM *target_address=NULL;

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    dest = jxta_peer_new();
    dest_pa = jxta_peerview_address_request_msg_get_peer_adv(addr_req);
    jxta_peer_set_adv(dest, dest_pa);
    dest_id = jxta_PA_get_PID(dest_pa);
    pve = peerview_get_pve(myself, dest_id);
    jxta_peer_set_peerid(dest, dest_id);
    JXTA_OBJECT_RELEASE(dest_pa);
    JXTA_OBJECT_RELEASE(dest_id);

    if (myself->assign_state != ASSIGN_IDLE && myself->activity_address_locking_ends > jpr_time_now()) {
        locked = FALSE;
        apr_thread_mutex_unlock(myself->mutex);
        res = peerview_send_address_assign_msg(myself, dest, dest
                    , ADDRESS_ASSIGN_BUSY, NULL, NULL);
        goto FINAL_EXIT;
    }

    jxta_peer_set_expires(dest, jpr_time_now() + jxta_peerview_address_request_msg_get_peer_adv_exp(addr_req));

    if (NULL != pve) {
        int cluster;

        target_address = BN_dup(pve->target_hash);
        JXTA_OBJECT_RELEASE(pve);
        cluster = myself->my_cluster;
        get_empty_cluster(myself, &cluster);

        locked = FALSE;
        apr_thread_mutex_unlock(myself->mutex);
        res = peerview_send_address_assign_msg(myself, dest, dest
                    , ADDRESS_ASSIGN, target_address, NULL);

    } else {
        locked = FALSE;
        apr_thread_mutex_unlock(myself->mutex);

        res = peerview_lock_address(myself, dest);
    }

  FINAL_EXIT:
    if (locked) {
        /* We may have unlocked early to send a message. */
        apr_thread_mutex_unlock(myself->mutex);
    }

    if (dest)
        JXTA_OBJECT_RELEASE(dest);
    if (target_address)
        BN_free(target_address);

    return res;
}

static void get_empty_cluster(Jxta_peerview *myself, int * cluster)
{
    unsigned int low_cluster;
    unsigned int mid_cluster;
    unsigned int high_cluster;
    Jxta_boolean found_empty=FALSE;
    unsigned int a_cluster = UINT_MAX;
    int cluster_size;

    low_cluster = 0;

    if (1 == myself->clusters_count) {
        *cluster = 0;
        return;
    }
    high_cluster = myself->clusters_count - 1;

    do {
        mid_cluster = ((high_cluster - low_cluster) / 2) + low_cluster;
 
        if ((high_cluster - myself->my_cluster) <= (myself->my_cluster - low_cluster)) {
            for (a_cluster = low_cluster; a_cluster <= mid_cluster; a_cluster++) {
                cluster_size = jxta_vector_size(myself->clusters[a_cluster].members);
                if (0 == cluster_size) {
                    found_empty = TRUE;
                    *cluster = a_cluster;
                    break;
                } 
            }
            if (!found_empty) {
                low_cluster = mid_cluster + 1;
            }
        } else {
            for (a_cluster = high_cluster; a_cluster > mid_cluster; a_cluster--) {
                cluster_size = jxta_vector_size(myself->clusters[a_cluster].members);
                if (0 == cluster_size) {
                    found_empty = TRUE;
                    *cluster = a_cluster;
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

    if (!found_empty) {
        *cluster = a_cluster;
    }
    return;
}
  
static Jxta_status peerview_lock_address(Jxta_peerview * myself, Jxta_peer * peer)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_boolean locked=FALSE;
  
    if (ASSIGN_IDLE == myself->assign_state) {
  
        apr_thread_mutex_lock(myself->mutex);
        locked = TRUE;
  
        if (myself->activity_address_assign_peer) {
            JXTA_OBJECT_RELEASE(myself->activity_address_assign_peer);
            myself->activity_address_assign_peer = NULL;
        }
        myself->activity_address_assign_peer = JXTA_OBJECT_SHARE(peer);
        myself->assign_state = ASSIGN_SEND_LOCK;
        apr_thread_pool_tasks_cancel(myself->thread_pool, &myself->assign_state);

        if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_address_locking, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, &myself->assign_state)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate address lock activity. [%pp]\n", myself);
        }
    } else {
        res = JXTA_BUSY;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Return busy from peerview_lock_address\n", myself);
    }
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);
    return res;
}


static Jxta_status peerview_reclaim_addresses(Jxta_peerview * myself)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_boolean locked=FALSE;

    if (ASSIGN_IDLE == myself->assign_state) {

        apr_thread_mutex_lock(myself->mutex);
        locked = TRUE;

        myself->assign_state = ASSIGN_SEND_FREE;
        apr_thread_pool_tasks_cancel(myself->thread_pool, &myself->assign_state);

        if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_address_locking, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, &myself->assign_state)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate address lock activity. [%pp]\n", myself);
        }
    } else {
        res = JXTA_BUSY;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Return busy from peerview_reclaim_address\n", myself);
    }
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);
    return res;
}

static Jxta_status peerview_handle_address_assign_msg(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *msg_j=NULL;
    Jxta_address_assign_msg_type msg_type;
    Jxta_id *rcv_peerid=NULL;
    JString *rcv_peerid_j=NULL;

    msg_type = jxta_peerview_address_assign_msg_get_type(addr_assign);

    rcv_peerid = jxta_peerview_address_assign_msg_get_peer_id(addr_assign);

    jxta_id_to_jstring(rcv_peerid, &rcv_peerid_j);


    msg_j = jstring_new_2("Received address asssing msg ");

    jstring_append_2(msg_j, " type:");
    jstring_append_2(msg_j, jxta_peerview_address_assign_msg_type_text(addr_assign));

    jstring_append_2(msg_j, " from:");
    jstring_append_1(msg_j, rcv_peerid_j);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s peers:%d\n", jstring_get_string(msg_j)
                                , jxta_peerview_address_assign_msg_get_cluster_peers(addr_assign));

    if (NULL == myself->instance_mask && msg_type != ADDRESS_ASSIGN) {
        /* This has to be an address assignment msg we never asked for. Ignore it. */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unexpected address assignement message from unknown peerview. [%pp]\n",
                        myself);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    switch (msg_type) {

        case ADDRESS_ASSIGN:
            res = peerview_handle_address_assignment(myself, addr_assign);
            break;
        case ADDRESS_ASSIGN_LOCK:
            res = peerview_handle_address_assign_lock(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_LOCK_RSP:
            res = peerview_handle_address_assign_lock_rsp(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_UNLOCK:
            res = peerview_handle_address_assign_unlock(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_BUSY:
            res = peerview_handle_address_assign_busy(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_ASSIGNED:
            res = peerview_handle_address_assign_assigned(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_ASSIGNED_RSP:
            res = peerview_handle_address_assign_assigned_rsp(myself, addr_assign);
            break;
        case ADDRESS_ASSIGN_ASSIGNED_RSP_NEG:
            res = peerview_handle_address_assigned_rsp_negative(myself, addr_assign);
            break;
        case ADDRESS_ASSIGN_FREE:
            res = peerview_handle_address_assign_free(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_FREE_RSP:
            res = peerview_handle_address_assign_free_rsp(myself, addr_assign, rcv_peerid, jstring_get_string(rcv_peerid_j));
            break;
        case ADDRESS_ASSIGN_STATUS:
            break;
    }

FINAL_EXIT:

    if (msg_j)
        JXTA_OBJECT_RELEASE(msg_j);
    if (rcv_peerid_j)
        JXTA_OBJECT_RELEASE(rcv_peerid_j);
    if (rcv_peerid)
        JXTA_OBJECT_RELEASE(rcv_peerid);

    return res;
}

static Jxta_status peerview_handle_address_assign_free(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_peer *rcv_peer=NULL;
    Jxta_vector *rcv_peer_hash=NULL;
    Jxta_vector *reject_entries=NULL;
    int i;

    apr_thread_mutex_lock(myself->mutex);

    reject_entries = jxta_vector_new(0);

    rcv_peer = jxta_peer_new();
    jxta_peer_set_peerid(rcv_peer, rcv_peerid);

    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &rcv_peer_hash);

    for (i=0; NULL != rcv_peer_hash && i<jxta_vector_size(rcv_peer_hash); i++) {
        JString *hash_entry;
        Jxta_vector *pves=NULL;
        int j;
        JString *my_entry=NULL;
        Jxta_boolean equal=FALSE;

        res = jxta_vector_get_object_at(rcv_peer_hash, JXTA_OBJECT_PPTR(&hash_entry), i);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from received hash list res:%d\n", res);
            continue;
        }

        for (j=0; FALSE == equal && j < jxta_vector_size(myself->free_hash_list); j++) {

            equal=FALSE;
            res = jxta_vector_get_object_at(myself->free_hash_list, JXTA_OBJECT_PPTR(&my_entry), j);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the my free hash list res:%d\n", res);
                continue;
            }
            if (0 == jstring_equals(my_entry, hash_entry)) {
                jxta_vector_add_object_last(reject_entries, (Jxta_object *) hash_entry);
                equal = TRUE;
            }
            JXTA_OBJECT_RELEASE(my_entry);
        }
        pves = peerview_get_all_pves(myself);
        for (j=0; FALSE == equal && j<jxta_vector_size(pves); j++) {
            Peerview_entry *pve=NULL;
            char *tmp;

            equal=FALSE;
            jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&pve), j);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the pve entries res:%d\n", res);
                continue;
            }

            tmp = BN_bn2hex(pve->target_hash);
            if (0 == strcmp(tmp, jstring_get_string(hash_entry))) {
                jxta_vector_add_object_last(reject_entries, (Jxta_object *) hash_entry);
                equal = TRUE;
            }

            JXTA_OBJECT_RELEASE(pve);
            free(tmp);
        }
        for (j=0; FALSE == equal && j<jxta_vector_size(myself->possible_free_list); j++) {

            res = jxta_vector_get_object_at(myself->possible_free_list, JXTA_OBJECT_PPTR(&my_entry), j);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the possible free list res:%d\n", res);
                continue;
            }
            if (0 == jstring_equals(hash_entry, my_entry)) {
                jxta_vector_remove_object_at(myself->possible_free_list, NULL, j--);
                equal = TRUE;
            }
            JXTA_OBJECT_RELEASE(my_entry);
        }
        JXTA_OBJECT_RELEASE(pves);
        JXTA_OBJECT_RELEASE(hash_entry);
    }

    apr_thread_mutex_unlock(myself->mutex);

    peerview_send_address_assign_msg(myself, rcv_peer, NULL
            , ADDRESS_ASSIGN_FREE_RSP, NULL, reject_entries);

    if (rcv_peer_hash)
        JXTA_OBJECT_RELEASE(rcv_peer_hash);
    if (reject_entries)
        JXTA_OBJECT_RELEASE(reject_entries);
    if (rcv_peer)
        JXTA_OBJECT_RELEASE(rcv_peer);

    return res;
}

static Jxta_status peerview_handle_address_assign_free_rsp(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_vector *rcv_peer_hash=NULL;
    Jxta_boolean locked = FALSE;
    Jxta_boolean equal = FALSE;
    int i;

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    if (JXTA_SUCCESS != jxta_hashtable_contains(myself->activity_address_free_peers
                , rcv_peerid_c, strlen(rcv_peerid_c) + 1)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received Free Response either too late or an invalid peer: %s\n"
                                , rcv_peerid_c);
        goto FINAL_EXIT;
    }
    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &rcv_peer_hash);

    for (i=0; NULL != rcv_peer_hash && i < jxta_vector_size(rcv_peer_hash); i++) {
        JString *hash_entry=NULL;
        int j;

        res = jxta_vector_get_object_at(rcv_peer_hash, JXTA_OBJECT_PPTR(&hash_entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the received hash list res:%d\n", res);
            continue;
        }
        for (j=0; FALSE == equal && j<jxta_vector_size(myself->possible_free_list); j++) {
            JString *my_entry=NULL;

            jxta_vector_get_object_at(myself->possible_free_list, JXTA_OBJECT_PPTR(&my_entry), j);

            if (0 == jstring_equals(my_entry, hash_entry)) {
                jxta_vector_remove_object_at(myself->possible_free_list, NULL, j--);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Hash address still in use: %s\n", jstring_get_string(my_entry));
            }
            JXTA_OBJECT_RELEASE(my_entry);
        }
        JXTA_OBJECT_RELEASE(hash_entry);
    }

    if (0 >= --myself->activity_address_free_peers_sent) {
        for (i=0; i < jxta_vector_size(myself->possible_free_list); i++) {
            Jxta_object *free_entry=NULL;

            res = jxta_vector_get_object_at(myself->possible_free_list, &free_entry, i);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the possible free hash list res:%d\n", res);
                continue;
            }

            jxta_vector_add_object_last(myself->free_hash_list,free_entry);
            jxta_vector_remove_object_at(myself->possible_free_list, NULL, i--);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Reclaimed hash address: %s\n", jstring_get_string((JString *) free_entry));

            JXTA_OBJECT_RELEASE(free_entry);
        }
        myself->assign_state = ASSIGN_IDLE;
        myself->activity_address_free_ends = jpr_time_now();
        if (myself->activity_address_locking_peer) {
            free(myself->activity_address_locking_peer);
            myself->activity_address_locking_peer = NULL;
       }
    }

FINAL_EXIT:
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);
    if (rcv_peer_hash)
        JXTA_OBJECT_RELEASE(rcv_peer_hash);
    return res;
}

static void match_hash_to_list(Jxta_hashtable * peers, BIGNUM *space, BIGNUM *hash, Jxta_peer **peer, BIGNUM **target_hash)
{
    Jxta_status res=JXTA_SUCCESS;
    BIGNUM *hash_diff=NULL;
    BIGNUM *temp_diff=NULL;
    BIGNUM *temp_bn=NULL;
    int closest_idx=0;
    Jxta_vector *closest_v=NULL;
    char *assign_peer=NULL;
    JString *hash_entry;

    char **keys;
    char **keys_save;
    char *hashc;

    assert(NULL != hash);
    hashc = BN_bn2hex(hash);

    *peer = NULL;
    *target_hash = NULL;
    keys = jxta_hashtable_keys_get(peers);
    keys_save = keys;

    hash_diff = BN_new();
    BN_copy(hash_diff, space);
    temp_diff = BN_new();

    while (*keys) {
        int i;
        Jxta_vector *hash_list=NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Find a target hash at peer %s for hash %s\n", *keys, hashc);

        if (JXTA_SUCCESS == jxta_hashtable_get(peers, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&hash_list))) {

            for (i=0; i < jxta_vector_size(hash_list); i++) {
                Jxta_boolean have_a_match=FALSE;

                Jxta_boolean closest=FALSE;

                res = jxta_vector_get_object_at(hash_list, JXTA_OBJECT_PPTR(&hash_entry), i);
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the free list res:%d\n", res);
                    continue;
                }
                BN_hex2bn(&temp_bn, jstring_get_string(hash_entry));
  
                if (BN_cmp(temp_bn, hash) > 0) {
                    BN_sub(temp_diff, temp_bn, hash);
                } else if (BN_cmp(temp_bn, hash) < 0) {
                    BN_sub(temp_diff, hash, temp_bn);
                } else {
                    have_a_match = TRUE;
                }
                if (!have_a_match) {
                    if (BN_cmp(temp_diff, hash_diff) < 0) {
                        BN_free(hash_diff);
                        hash_diff = BN_dup(temp_diff);
                        closest=TRUE;
                        closest_idx = i;
                        closest_v = hash_list;
                    }
                } else {
                    closest=TRUE;
                    closest_idx = i;
                    closest_v = hash_list;
                }
                if (closest) {
                    if (NULL == assign_peer || (NULL != assign_peer && 0 != strcmp(assign_peer, *keys))) {
                        if (NULL != assign_peer) free(assign_peer);
                        assign_peer = strdup(*keys);
                    }
                }
  
                BN_free(temp_bn);
                temp_bn = NULL;

                JXTA_OBJECT_RELEASE(hash_entry);
                if (have_a_match) break;
            }
            if (hash_list)
                JXTA_OBJECT_RELEASE(hash_list);
        } else {
  
        }
        free(*(keys++));
    }
    while (*keys) free(*(keys++));
    free(keys_save);
  
    if (NULL != closest_v) {
        Jxta_id *closest_id;

        res = jxta_vector_remove_object_at(closest_v, JXTA_OBJECT_PPTR(&hash_entry), closest_idx);
        BN_hex2bn(&temp_bn, jstring_get_string(hash_entry));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found closest hash %s for %s at %s\n", jstring_get_string(hash_entry), hashc, assign_peer);

        *target_hash = temp_bn;
        *peer = jxta_peer_new();
        jxta_id_from_cstr(&closest_id, assign_peer);
        jxta_peer_set_peerid(*peer, closest_id);
        JXTA_OBJECT_RELEASE(closest_id);
        JXTA_OBJECT_RELEASE(hash_entry);
    }
    if (assign_peer)
        free(assign_peer);
    if (hash_diff)
        BN_free(hash_diff);
    if (temp_diff)
        BN_free(temp_diff);
    free(hashc);
}

static Jxta_status peerview_handle_address_assign_lock_rsp(Jxta_peerview *myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_vector *peer_hash_list=NULL;
    Peerview_entry *pve=NULL;
    Jxta_vector *rcv_peer_hash_list=NULL;
    int i;

    apr_thread_mutex_lock(myself->mutex);

    pve = peerview_get_pve( myself, rcv_peerid);

    if (NULL == pve) {
        goto FINAL_EXIT;
    }
    pve->cluster_peers = jxta_peerview_address_assign_msg_get_cluster_peers(addr_assign);

    if (JXTA_SUCCESS != jxta_hashtable_get(myself->activity_address_locked_peers
                , rcv_peerid_c, strlen(rcv_peerid_c) + 1, JXTA_OBJECT_PPTR(&peer_hash_list))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received Lock Response either too late or an invalid peer: %s\n"
                                , rcv_peerid_c);
        goto FINAL_EXIT;
    }

    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &rcv_peer_hash_list);
    for (i=0; NULL != rcv_peer_hash_list && i<jxta_vector_size(rcv_peer_hash_list); i++) {
        JString *hash_entry;

        res = jxta_vector_get_object_at(rcv_peer_hash_list, JXTA_OBJECT_PPTR(&hash_entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the received hash list res:%d\n", res);
            continue;
        }

        jxta_vector_add_object_last(peer_hash_list, (Jxta_object *) hash_entry);

        JXTA_OBJECT_RELEASE(hash_entry);
    }
    if (jxta_peerview_is_associate(myself, rcv_peerid)) {
        int peer_cluster;

        peer_cluster = cluster_for_hash(myself, pve->target_hash);
        if (NULL != myself->clusters[peer_cluster].next_address) {
            BN_free(myself->clusters[peer_cluster].next_address);
            myself->clusters[peer_cluster].next_address = NULL;
        }

        BN_hex2bn(&myself->clusters[peer_cluster].next_address, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Setting next_address for cluster %d to %s\n"
                                    , peer_cluster, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));
    }
    if (rcv_peer_hash_list)
        JXTA_OBJECT_RELEASE(rcv_peer_hash_list);
    if (pve)
        JXTA_OBJECT_RELEASE(pve);

    /* add the entries to the free list */
    if (0 >= --myself->activity_address_locking_peers_sent) {
        Jxta_peer *dest=NULL;
        BIGNUM *target_hash=NULL;
        int cluster;
        BIGNUM *target_address=NULL;

        cluster = myself->my_cluster;
        get_empty_cluster(myself, &cluster);
        /* if there was no empty cluster find the least populated cluster */
        if (myself->my_cluster == cluster) {
            Jxta_vector *pves=NULL;
            int smallest_cluster=myself->my_cluster;
            int smallest_cluster_peers=jxta_vector_size(myself->clusters[myself->my_cluster].members);

            pves = peerview_get_all_pves(myself);

            for (i=0; NULL != pves && i < jxta_vector_size(pves); i++) {

                res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&pve), i);
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from the pv entries res:%d\n", res);
                    continue;
                }
                if (pve->cluster != myself->my_cluster && pve->cluster_peers < smallest_cluster_peers) {
                    smallest_cluster = pve->cluster;
                    smallest_cluster_peers = pve->cluster_peers;
                }
                JXTA_OBJECT_RELEASE(pve);
            }
            cluster = smallest_cluster;
            if (NULL != pves)
                JXTA_OBJECT_RELEASE(pves);
        }
        /* if this peer's cluster is the smallest find a partner address */
        if (myself->my_cluster == cluster) {

            find_best_target_address(jxta_RdvConfig_pv_get_address_assign_mode(myself->rdvConfig)
                        , myself->clusters[cluster].histogram
                        , myself->clusters[cluster].min
                        , myself->clusters[cluster].max
                        , myself->hash_attempts
                        , myself->peer_address_space
                        , &target_address);

            if (target_address != NULL) {
                jxta_hashtable_put(myself->activity_address_locked_peers
                                        , myself->pid_c, strlen(myself->pid_c) + 1, (Jxta_object *) myself->free_hash_list);

                match_hash_to_list(myself->activity_address_locked_peers, myself->cluster_divisor, target_address, &dest, &target_hash);
                if (NULL != dest && jxta_id_equals(jxta_peer_peerid(dest), myself->pid)) {
                    JXTA_OBJECT_RELEASE(dest);
                    dest = NULL;
                }
                BN_free(target_address);
                target_address = NULL;
            }
        } else {
            jxta_hashtable_put(myself->activity_address_locked_peers
                    , myself->pid_c, strlen(myself->pid_c) + 1, (Jxta_object *) myself->free_hash_list);
            if (NULL == myself->clusters[cluster].next_address) {
                target_address = BN_new();
                BN_set_word(target_address, 0);
                BN_add(target_address, target_address, myself->clusters[cluster].mid);
                myself->clusters[cluster].next_address = BN_dup(target_address);
            }
            match_hash_to_list(myself->activity_address_locked_peers, myself->cluster_divisor, myself->clusters[cluster].next_address, &dest, &target_hash);
            if (NULL != dest && jxta_id_equals(jxta_peer_peerid(dest), myself->pid)) {
                JXTA_OBJECT_RELEASE(dest);
                dest = NULL;
            }
            BN_rshift1(myself->clusters[cluster].next_address, myself->clusters[cluster].next_address);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "best address for cluster %d target_hash:%s dest:%s\n"
                                    , cluster, NULL == target_hash ? "false":"true", NULL == dest ? "false":"true");
  
        if (NULL != target_hash && NULL == dest) {
            Jxta_vector *half_list=NULL;
  
            half_the_free_list(myself, cluster, &half_list);
            peerview_send_address_assign_and_unlock(myself, target_hash, half_list);
  
            if (half_list)
               JXTA_OBJECT_RELEASE(half_list);
        } else if (NULL != dest) {
            peerview_send_address_assigned(myself, dest, target_hash);
        } else {
            peerview_send_address_assign_unlock(myself);
        }
        if (target_address)
            BN_free(target_address);
        if (target_hash)
            BN_free(target_hash);
        if (dest)
            JXTA_OBJECT_RELEASE(dest);
    }

FINAL_EXIT:
    apr_thread_mutex_unlock(myself->mutex);

    if (peer_hash_list)
        JXTA_OBJECT_RELEASE(peer_hash_list);
    return res;
}
  
  
static Jxta_status peerview_handle_address_assign_busy(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *peer_j=NULL;

    CHECK_MESSAGE_SOURCE

    if (jxta_hashtable_contains(myself->activity_address_locked_peers, rcv_peerid_c, strlen(rcv_peerid_c) + 1)) {

        jxta_hashtable_del(myself->activity_address_locked_peers, rcv_peerid_c, strlen(rcv_peerid_c) + 1, NULL);
        myself->activity_address_locking_peers_sent--;
    }

    if (peer_j)
        JXTA_OBJECT_RELEASE(peer_j);
    return res;
}


static Jxta_status peerview_handle_address_assign_assigned(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;

    Jxta_peer *rcv_peer=NULL;
    const char *target_hash_c=NULL;
    Jxta_boolean found=FALSE;
    int i;

    CHECK_MESSAGE_SOURCE

    rcv_peer = jxta_peer_new();
    jxta_peer_set_peerid(rcv_peer, rcv_peerid);

    target_hash_c = jxta_peerview_address_assign_msg_get_target_hash(addr_assign);

    for (i=0; FALSE == found && i<jxta_vector_size(myself->free_hash_list); i++) {
        JString *hash_entry=NULL;

        res = jxta_vector_get_object_at(myself->free_hash_list, JXTA_OBJECT_PPTR(&hash_entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from my free hash list res:%d\n", res);
            continue;
        }

        if (0 == strcmp(jstring_get_string(hash_entry), target_hash_c)) {
            jxta_vector_remove_object_at(myself->free_hash_list, NULL, i--);
            found = TRUE;
        }
        JXTA_OBJECT_RELEASE(hash_entry);
    }
    if (found) {
        Jxta_vector *half_list=NULL;
        BIGNUM *target_hash=NULL;

        BN_hex2bn(&target_hash, target_hash_c);

        half_the_free_list(myself, myself->my_cluster, &half_list);

        peerview_send_address_assign_msg(myself, rcv_peer, NULL
            , ADDRESS_ASSIGN_ASSIGNED_RSP, target_hash, half_list);

        JXTA_OBJECT_RELEASE(half_list);
        BN_free(target_hash);
    } else {
        BIGNUM *target_bn=NULL;

        BN_hex2bn(&target_bn, target_hash_c);

        peerview_send_address_assign_msg(myself, rcv_peer, NULL, ADDRESS_ASSIGN_ASSIGNED_RSP_NEG, target_bn, NULL);

        BN_free(target_bn);
    }
    myself->assign_state = ASSIGN_IDLE;
    if (myself->activity_address_locking_peer) {
        free(myself->activity_address_locking_peer);
        myself->activity_address_locking_peer = NULL;
    }

    JXTA_OBJECT_RELEASE(rcv_peer);
    return res;
}

static Jxta_status peerview_handle_address_assign_assigned_rsp(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign)
{
    Jxta_status res=JXTA_SUCCESS;
    BIGNUM *target_hash = NULL;
    Jxta_vector *free_hash_list=NULL;

    BN_hex2bn(&target_hash, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));
    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &free_hash_list);
    peerview_send_address_assign_and_unlock(myself, target_hash, free_hash_list);

    jxta_hashtable_clear(myself->activity_address_locked_peers);
    if (free_hash_list)
        JXTA_OBJECT_RELEASE(free_hash_list);
    BN_free(target_hash);
    return res;
}

static Jxta_status peerview_handle_address_assigned_rsp_negative(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign)
{
    Jxta_status res=JXTA_SUCCESS;
    BIGNUM *target_hash = NULL;
    Jxta_vector *free_hash_list=NULL;
    JString *back_in_free=NULL;

    back_in_free = jstring_new_2(jxta_peerview_address_assign_msg_get_target_hash(addr_assign));
    jxta_vector_add_object_last(myself->free_hash_list, (Jxta_object *) back_in_free);

    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &free_hash_list);

    jxta_vector_addall_objects_last(myself->free_hash_list, free_hash_list);
    peerview_send_address_assign_unlock(myself);

    jxta_hashtable_clear(myself->activity_address_locked_peers);
    JXTA_OBJECT_RELEASE(free_hash_list);
    JXTA_OBJECT_RELEASE(back_in_free);
    BN_free(target_hash);
    return res;
}

static Jxta_status peerview_handle_address_assign_unlock(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    CHECK_MESSAGE_SOURCE
            myself->assign_state = ASSIGN_IDLE;
            myself->activity_address_locking_ends = jpr_time_now();
            if (myself->activity_address_locking_peer) {
                free(myself->activity_address_locking_peer);
                myself->activity_address_locking_peer = NULL;
            }
    return JXTA_SUCCESS;
}

static Jxta_status peerview_handle_address_assign_lock(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign, Jxta_id *rcv_peerid, const char *rcv_peerid_c)
{
    Jxta_status res=JXTA_SUCCESS;
    BIGNUM *target_hash=NULL;
    Jxta_peer *dest=NULL;
    Jxta_peer *assign_peer=NULL;
    Jxta_id *assign_peerid=NULL;
    int cmp_result=0;

    dest = jxta_peer_new();

    jxta_peer_set_peerid(dest, rcv_peerid);

    assign_peer = jxta_peer_new();
    assign_peerid = jxta_peerview_address_assign_msg_get_assign_peer_id(addr_assign);
    assert(NULL != assign_peerid);

    jxta_peer_set_peerid(assign_peer, assign_peerid);

    if (NULL != myself->activity_address_locking_peer) {
        cmp_result = strcmp(myself->activity_address_locking_peer, rcv_peerid_c);
        if (cmp_result < 0) {
            peerview_send_address_assign_msg(myself, dest, assign_peer, ADDRESS_ASSIGN_BUSY, NULL, NULL);
            goto FINAL_EXIT;
        } else if (cmp_result > 0) {
            free(myself->activity_address_locking_peer);
            myself->activity_address_locking_peer = strdup(rcv_peerid_c);
        }
    } else {
        myself->activity_address_locking_peer = strdup(rcv_peerid_c);
    }

    BN_hex2bn(&target_hash, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));

    if (ASSIGN_IDLE != myself->assign_state) {
        peerview_send_address_assign_msg(myself, dest, assign_peer, ADDRESS_ASSIGN_BUSY, NULL, NULL);
    } else {
        Jxta_vector *free_hash_list=NULL;

        if (jxta_peerview_is_associate(myself, rcv_peerid)) {

            free_hash_list = JXTA_OBJECT_SHARE(myself->free_hash_list);

            find_best_target_address(jxta_RdvConfig_pv_get_address_assign_mode(myself->rdvConfig)
                        , myself->clusters[myself->my_cluster].histogram
                        , myself->clusters[myself->my_cluster].min
                        , myself->clusters[myself->my_cluster].max
                        , myself->hash_attempts
                        , myself->peer_address_space
                        , &target_hash);
        } else {
            free_hash_list = JXTA_OBJECT_SHARE(myself->free_hash_list);
        }

        peerview_send_address_assign_msg(myself,  dest, assign_peer, ADDRESS_ASSIGN_LOCK_RSP, target_hash, free_hash_list);

        myself->assign_state = ASSIGN_LOCKED;
        myself->activity_address_locking_ends = jpr_time_now() + jxta_peerview_address_assign_msg_get_expiration(addr_assign);

        apr_thread_pool_tasks_cancel(myself->thread_pool, &myself->assign_state);

        if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_address_locking, myself,
            APR_THREAD_TASK_PRIORITY_HIGH, &myself->assign_state)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate address lock activity. [%pp]\n", myself);
        }
        if (free_hash_list)
            JXTA_OBJECT_RELEASE(free_hash_list);
    }

FINAL_EXIT:
    if (target_hash)
        BN_free(target_hash);
    if (dest)
        JXTA_OBJECT_RELEASE(dest);
    if (assign_peer)
        JXTA_OBJECT_RELEASE(assign_peer);
    if (assign_peerid)
        JXTA_OBJECT_RELEASE(assign_peerid);
    return res;
}

static Jxta_status peerview_handle_address_assignment(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *free_hash_list=NULL;
    BIGNUM *instance_mask = NULL;
    BIGNUM *target_hash = NULL;
    Jxta_boolean same;
    int i;

    apr_thread_mutex_lock(myself->mutex);

    BN_hex2bn(&instance_mask, jxta_peerview_address_assign_msg_get_instance_mask(addr_assign));

    same = 0 == BN_cmp(myself->instance_mask, instance_mask);

    if (!same) {
        /* this is the peer we requested the address from and it switched peerviews */
        if (myself->instance_mask) {
           BN_free(myself->instance_mask);
        }
        myself->instance_mask = BN_dup(instance_mask);
        if (myself->free_hash_list) {
            JXTA_OBJECT_RELEASE(myself->free_hash_list);
            myself->free_hash_list = NULL;
        }
        if (myself->self_pve) {
            if (NULL != myself->self_pve->free_hash_list) {
                JXTA_OBJECT_RELEASE(myself->self_pve->free_hash_list);
                myself->self_pve->free_hash_list = NULL;
            }
        }
    }

    if ((NULL != myself->self_pve) && (0 != jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        /* If we are already part of a peerview then disregard all new address assignments */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unneeded address assignement. We already have one. [%pp]\n", myself);
        res = JXTA_BUSY;
        goto FINAL_EXIT;
    }

    BN_hex2bn(&target_hash, jxta_peerview_address_assign_msg_get_target_hash(addr_assign));

    assert(PV_ADDRESSING == myself->state);
    assert(NULL != target_hash);
    res = create_self_pve(myself, target_hash);
    myself->state = PV_ANNOUNCING;

    BN_free(target_hash);

    jxta_peerview_address_assign_msg_get_free_hash_list(addr_assign, &free_hash_list);
    for (i=0; NULL != free_hash_list && i<jxta_vector_size(free_hash_list); i++) {
        JString *hash_entry=NULL;
        Jxta_boolean exists = FALSE;
        int j;

        jxta_vector_get_object_at(free_hash_list, JXTA_OBJECT_PPTR(&hash_entry), i);
        exists = FALSE;
        for (j=0; j<jxta_vector_size(myself->free_hash_list) && exists == FALSE; j++) {
            JString *have_hash=NULL;

            jxta_vector_get_object_at(myself->free_hash_list, JXTA_OBJECT_PPTR(&have_hash), j);
            if (0 == jstring_equals(hash_entry, have_hash)) {
                exists = TRUE;
            }

            if (have_hash)
                JXTA_OBJECT_RELEASE(have_hash);
        }
        if (FALSE == exists) {
            jxta_vector_add_object_last(myself->free_hash_list, (Jxta_object *) hash_entry);
        }
        JXTA_OBJECT_RELEASE(hash_entry);
    }
    if (NULL == myself->self_pve->free_hash_list) {
        myself->self_pve->free_hash_list = JXTA_OBJECT_SHARE(myself->free_hash_list);
    }
    if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_announce, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, myself)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate announce activity. [%pp]\n", myself);
    }
  FINAL_EXIT:

    apr_thread_mutex_unlock(myself->mutex);

    BN_free(instance_mask);
    if (free_hash_list)
        JXTA_OBJECT_RELEASE(free_hash_list);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Joined peerview {%s} @ address {%s}\n",
                        jxta_peerview_address_assign_msg_get_instance_mask(addr_assign),
                        jxta_peerview_address_assign_msg_get_target_hash(addr_assign));
    }

    return res;
}

static Jxta_status peerview_handle_ping(Jxta_peerview * myself, Jxta_peerview_ping_msg * ping, Jxta_boolean consolidated)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_id *pid=NULL;
    Jxta_peer *dest=NULL;
    Jxta_endpoint_address *dst_ea;
    Peerview_entry *pve=NULL;
    Jxta_vector *entries=NULL;
    Jxta_hashtable *pong_msgs=NULL;
    Jxta_peerview_pong_msg *pong_msg=NULL;
    JString *pong_msg_xml=NULL;
    const char *groupid_c=NULL;
    Jxta_id *groupid=NULL;
    JString *groupid_j=NULL;
    JString* pid_j=NULL;
    Jxta_boolean send=FALSE;
    int i;

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
    jxta_id_to_jstring(pid, &pid_j);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Receive ping message [%pp] from %s\n", ping, jstring_get_string(pid_j));


    if (consolidated) {
        pong_msgs = jxta_hashtable_new(0);
        jxta_peerview_ping_msg_get_group_entries(ping, &entries);
    } else {
        entries = jxta_vector_new(0);
    }
    if ((NULL == myself->instance_mask || NULL == myself->self_pve
        || 0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve)) && (0 == jxta_vector_size(entries))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Can't send pong message now! [%pp]\n", myself);
    } else if (0 == jxta_vector_size(entries)) {
        res = peerview_create_pong_msg(myself, PONG_STATUS, FALSE, &pong_msg, FALSE);

        if (consolidated) {
            jxta_PG_get_GID(myself->group, &groupid);
            jxta_id_get_uniqueportion(groupid, &groupid_j);

            groupid_c = jstring_get_string(groupid_j);

            if (JXTA_SUCCESS != jxta_hashtable_contains(pong_msgs, groupid_c, strlen(groupid_c) + 1)) {
                if (jxta_peerview_ping_msg_is_pv_id_only(ping)) {
                    jxta_peerview_pong_msg_get_xml_1(pong_msg, &pong_msg_xml);
                } else {
                    jxta_peerview_pong_msg_get_xml(pong_msg, &pong_msg_xml);
                }
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding %s to the pong_msgs hash [%pp]\n", groupid_c, pong_msgs);
                jxta_hashtable_put(pong_msgs, groupid_c, strlen(groupid_c) +1, (Jxta_object *) pong_msg_xml);
                JXTA_OBJECT_RELEASE(pong_msg_xml);
            }
            send = TRUE;
            JXTA_OBJECT_RELEASE(pong_msg);
            pong_msg = NULL;
            JXTA_OBJECT_RELEASE(groupid);
            groupid = NULL;
            JXTA_OBJECT_RELEASE(groupid_j);
            groupid_j = NULL;
        } else {
            dest = jxta_peer_new();
            jxta_peer_set_peerid(dest, pid);
            res = peerview_send_pong(myself, dest, PONG_STATUS, FALSE, jxta_peerview_ping_msg_is_pv_id_only(ping), FALSE);
            send = FALSE;
            JXTA_OBJECT_RELEASE(dest);
            dest = NULL;
        }
    } else for (i=0; i < jxta_vector_size(entries); i++) {
        Jxta_pv_ping_msg_group_entry *entry=NULL;
        Jxta_peerview *pv=NULL;
        JString *group_j=NULL;
        Jxta_vector *pvs=NULL;

        jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);

        group_j = jxta_peerview_ping_msg_entry_group_name(entry);
        pvs = peerview_get_pvs(group_j);

        if (jxta_vector_size(pvs) > 0) {

            jxta_vector_get_object_at(pvs, JXTA_OBJECT_PPTR(&pv), 0);

            pve = peerview_get_pve(pv, pid);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "pvs:%d Receive ping msg group:%s %s\n"
                            , jxta_vector_size(pvs), jstring_get_string(group_j), NULL == pve ? " with no pve": " with pve");

        }
        if (pve && pv) {
            if ((NULL == pv->instance_mask) || (NULL == pv->self_pve)
                || (0 == jxta_peer_get_expires((Jxta_peer *) pv->self_pve))) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Can't send pong message now! [%pp]\n", myself);
            } else {
                jxta_PG_get_GID(pv->group, &groupid);
                jxta_id_get_uniqueportion(groupid, &groupid_j);

                groupid_c = jstring_get_string(groupid_j);
                jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
                res = peerview_create_pong_msg(pv, PONG_STATUS, FALSE, &pong_msg, FALSE);

                if (JXTA_SUCCESS != jxta_hashtable_contains(pong_msgs, groupid_c, strlen(groupid_c) + 1)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding %s to the pong_msgs hash [%pp] with %s getting the pv\n"
                                    , groupid_c, pong_msgs, jstring_get_string(group_j));

                    if (jxta_peerview_ping_msg_entry_is_pv_id_only(entry)) {
                        jxta_peerview_pong_msg_get_xml_1(pong_msg, &pong_msg_xml);
                    } else {
                        jxta_peerview_pong_msg_get_xml(pong_msg, &pong_msg_xml);
                    }
                    jxta_hashtable_put(pong_msgs, groupid_c, strlen(groupid_c) +1, (Jxta_object *) pong_msg_xml);
                    JXTA_OBJECT_RELEASE(pong_msg_xml);
                }
                send = TRUE;
                JXTA_OBJECT_RELEASE(pong_msg);
                pong_msg = NULL;
                JXTA_OBJECT_RELEASE(groupid);
                groupid = NULL;
                JXTA_OBJECT_RELEASE(groupid_j);
                groupid_j = NULL;
            }
        } else {
            res = JXTA_UNREACHABLE_DEST;
        }
        if (group_j)
            JXTA_OBJECT_RELEASE(group_j);
        if (pvs)
            JXTA_OBJECT_RELEASE(pvs);
        if (pve)
            JXTA_OBJECT_RELEASE(pve);
        if (pv)
            JXTA_OBJECT_RELEASE(pv);
        JXTA_OBJECT_RELEASE(entry);
    }

    if (send) {
        dest = jxta_peer_new();
        jxta_peer_set_peerid(dest, pid);
        res = peerview_send_pong_2(myself, dest, pong_msgs);
    }

  FINAL_EXIT:

    if (dest)
        JXTA_OBJECT_RELEASE(dest);
    if (groupid_j)
        JXTA_OBJECT_RELEASE(groupid_j);
    if (pong_msg)
        JXTA_OBJECT_RELEASE(pong_msg);
    if (pong_msgs)
        JXTA_OBJECT_RELEASE(pong_msgs);
    if (NULL != pid)
        JXTA_OBJECT_RELEASE(pid);
    if (NULL != entries)
        JXTA_OBJECT_RELEASE(entries);
    if (NULL != pid_j)
        JXTA_OBJECT_RELEASE(pid_j);
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
            jxta_vector_clear(me->free_hash_list);
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
        res = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    /* only add rendezvous peers to the peerview when a rendezvous */
    if (jxta_peerview_pong_msg_is_rendezvous(pong) && jxta_rdv_service_is_rendezvous(me->rdv)) {
        res = create_pve_from_pong(me, pong, &pve);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed create peerview entry. [%pp]\n", me);
            goto FINAL_EXIT;
        }
        res = discovery_service_publish(me->discovery, (Jxta_advertisement *) pa , DISC_PEER, pve->adv_exp, LOCAL_ONLY_EXPIRATION);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error publishing PA from new PONG: %d\n", res);
        }

        res = peerview_add_pve(me, pve);
        JXTA_OBJECT_RELEASE(pve);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed adding entry to peerview. [%pp]\n", me);
            goto FINAL_EXIT;
        }
        res = discovery_service_publish(me->discovery, (Jxta_advertisement *) pa , DISC_PEER, jxta_peerview_pong_msg_get_peer_adv_exp(pong), LOCAL_ONLY_EXPIRATION);
    }

FINAL_EXIT:
    if (pa)
        JXTA_OBJECT_RELEASE(pa);

    return JXTA_SUCCESS;
}

static Jxta_status handle_existing_pve_pong(Jxta_peerview * me, Peerview_entry * pve, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_PA *pa = NULL;
    apr_uuid_t *adv_gen;
    apr_uuid_t *pv_id_gen;
    Jxta_vector *free_hash_list=NULL;
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

    free_hash_list = jxta_peerview_pong_msg_get_free_hash_list(pong);
    if (NULL != free_hash_list) {
        if (pve->free_hash_list)
            JXTA_OBJECT_RELEASE(pve->free_hash_list);

        pve->free_hash_list = free_hash_list;
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
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "[%pp] Received invalid PONG msg with no peerview adv_gen\n", me);
            } else if (is_more_recent_uuid(pv_id_gen, &pve->pv_id_gen)) {
                char tmp[64];
                char tmp1[64];
                send_ping= TRUE;
                apr_uuid_format(tmp, pv_id_gen);
                apr_uuid_format(tmp1, &pve->pv_id_gen);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "[%pp] Received pv_id_gen:%s ---- previous pv_id_gen:%s %s\n", me, tmp, tmp1, jstring_get_string(pid_j));
            } else {
                jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
            }
            if (pv_id_gen)
                free(pv_id_gen);
            JXTA_OBJECT_RELEASE(pid_j);
        } else {
           jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "[%pp] Received compact PONG msg that has no pve\n", me);
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "[%pp] Received PONG msg from myself\n", me);
        JXTA_OBJECT_RELEASE(pid);
        goto UNLOCK_EXIT;
    }

    jstring_append_2(msg_j, " state:");
    jstring_append_2(msg_j, jxta_peerview_pong_msg_state_text(pong));
    locked = TRUE;

    jstring_append_2(msg_j, " action:");
    jstring_append_2(msg_j, jxta_peerview_pong_msg_action_text(pong));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "[%pp] %s \n", me, jstring_get_string(msg_j));
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
            initiate_maintain_thread(me);
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
            res = peerview_send_pong(me, newPeer, action, send_candidates, FALSE, FALSE);
        }
        if (send_ping) {
            JString *peerid_j=NULL;

            jxta_id_to_jstring(pid, &peerid_j);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "------------- Sending ping for updated pv_id_gen to %s\n", jstring_get_string(peerid_j));
            res = peerview_send_ping(me, newPeer, NULL, FALSE);

            JXTA_OBJECT_RELEASE(peerid_j);
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
    Jxta_query_context *jContext=NULL;
    unsigned int i;
    char *query = NULL;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";
    Jxta_vector *remaining_pids=NULL;

    res = jxta_adv_request_msg_get_query(req, &query);
    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    /* check pves first */
    jContext = jxta_query_new(query);

    /* compile the XPath statement */
    res = jxta_query_compound_XPath(jContext, xmlString, FALSE);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot compile query in peerview_handle_adv_request --- %s\n", query);
        goto FINAL_EXIT;
    }
    for (i = 0; i < jxta_vector_size(jContext->queries); i++) {
        Jxta_query_element *elem;
        Jxta_id *pid;
        Jxta_peer *pve_peer;
        Jxta_PA *peer_adv;

        jxta_vector_get_object_at(jContext->queries, JXTA_OBJECT_PPTR(&elem), i);

        jxta_id_from_cstr(&pid, jstring_get_string(elem->jValue));
        peerview_get_peer(me, pid, &pve_peer);
        if (NULL != pve_peer) {
            if (NULL == adv_rsp_msg) {
                adv_rsp_msg = jxta_adv_response_msg_new();
            }
            jxta_peer_get_adv(pve_peer, &peer_adv);
            jxta_adv_response_msg_add_advertisement(adv_rsp_msg, (Jxta_advertisement *) peer_adv, ((Peerview_entry *)pve_peer)->adv_exp);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found PeerAdv elem - %s %s \n", jstring_get_string(elem->jName), jstring_get_string(elem->jValue));
            JXTA_OBJECT_RELEASE(peer_adv);
            JXTA_OBJECT_RELEASE(pve_peer);
        } else {
            if (NULL == remaining_pids) {
                remaining_pids = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(remaining_pids, (Jxta_object *) pid);
        }

        JXTA_OBJECT_RELEASE(elem);
        JXTA_OBJECT_RELEASE(pid);
    }
    if (NULL != remaining_pids) {
        JString *query_j=NULL;

        query_j = build_adv_req_query_string(remaining_pids);

        discovery_service_local_query(me->discovery, jstring_get_string(query_j), &requested_advs);
        for (i=0; i< jxta_vector_size(requested_advs); i++) {
            Jxta_advertisement * adv = NULL;
            Jxta_expiration_time expiration;
            Jxta_id *adv_id=NULL;

            if (JXTA_SUCCESS != jxta_vector_get_object_at(requested_advs, JXTA_OBJECT_PPTR(&adv), i)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve adv from vector:[%pp] at %d\n", requested_advs, i);
                continue;
            }
            adv_id = jxta_advertisement_get_id(adv);
            if (NULL == adv_id) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve ID from adv:[%pp]\n", adv);
                JXTA_OBJECT_RELEASE(adv);
                continue;
            }
            if (JXTA_SUCCESS != discovery_service_get_lifetime(me->discovery, DISC_PEER, adv_id, &expiration)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve lifetime from adv:[%pp]\n", adv);
                JXTA_OBJECT_RELEASE(adv);
                continue;
            }
            if (NULL == adv_rsp_msg) {
                adv_rsp_msg = jxta_adv_response_msg_new();
            }

            jxta_adv_response_msg_add_advertisement(adv_rsp_msg, adv, expiration - jpr_time_now());
            JXTA_OBJECT_RELEASE(adv);
        }
        JXTA_OBJECT_RELEASE(remaining_pids);
        if (query_j)
            JXTA_OBJECT_RELEASE(query_j);
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
        JXTA_OBJECT_RELEASE(adv_rsp_msg);
    }

FINAL_EXIT:
    if (jContext)
        JXTA_OBJECT_RELEASE(jContext);
    if (requested_advs)
        JXTA_OBJECT_RELEASE(requested_advs);
    if (query)
        free(query);
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
    Jxta_boolean local_view_changed = TRUE;

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
        /* only one associate is currently used - if an associate existed the view hasn't changed */
        local_view_changed = (myself->my_cluster != pve_cluster && (jxta_vector_size(myself->clusters[pve_cluster].members) > 1)) ? FALSE:TRUE;
    }

  FINAL_EXIT:
    /*
     * Notify the peerview listeners that we added a rdv peer from our local rpv 
     */
    if (!found && local_view_changed) {

        peerview_has_changed(myself);

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
    char *tmp;
    JString *tmp_j;
    Peerview_entry *pve;

    JXTA_OBJECT_CHECK_VALID(pid);

    jxta_id_to_jstring(pid, &pid_str);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removing PVE for %s\n", jstring_get_string(pid_str));

    apr_thread_mutex_lock(myself->mutex);

    res = jxta_hashtable_del(myself->pves, jstring_get_string(pid_str), jstring_length(pid_str) + 1, JXTA_OBJECT_PPTR(&pve));

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    tmp = BN_bn2hex(pve->target_hash);
    tmp_j = jstring_new_2(tmp);
    jxta_vector_add_object_last(myself->possible_free_list, (Jxta_object *) tmp_j);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Possible free target hash: %s\n", jstring_get_string(tmp_j));

    if (NULL != pve->free_hash_list) {
        jxta_vector_addall_objects_last(myself->possible_free_list, pve->free_hash_list);
    }

    JXTA_OBJECT_RELEASE(pve);

    free(tmp);
    JXTA_OBJECT_RELEASE(tmp_j);


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

        peerview_has_changed(myself);

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

    peerview_has_changed(myself);

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

static void print_start_end(const char * description, int each_segment, int each_histogram_entry, BIGNUM *start, BIGNUM *end)
{
    char *tmp_start;
    char *tmp_end;
    JString *desc_j;

    desc_j = jstring_new_0();
    tmp_start = BN_bn2hex(start);
    tmp_end = BN_bn2hex(end);

    jstring_append_2(desc_j, "%d %d ");
    jstring_append_2(desc_j, description);
    jstring_append_2(desc_j, " start:%s end:%s\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, jstring_get_string(desc_j) ,each_segment, each_histogram_entry, tmp_start, tmp_end);
    free(tmp_start);
    free(tmp_end);
    JXTA_OBJECT_RELEASE(desc_j);

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
static Jxta_status build_histogram(Jxta_vector ** histo, Jxta_vector * peers_v, BIGNUM * minimum, BIGNUM * maximum)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int each_peer;
    unsigned int all_peers;
    Jxta_vector *histogram;
    Jxta_vector *peers=NULL;

    JXTA_OBJECT_CHECK_VALID(peers_v);

    if ((NULL == minimum) || (NULL == maximum)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Min/Max not speified.\n");
        return JXTA_FAILED;
    }
    jxta_vector_clone(peers_v, &peers, 0, INT_MAX);

    jxta_vector_qsort(peers, (Jxta_object_compare_func) target_hash_sort);

    histogram = jxta_vector_new(jxta_vector_size(peers) * 2);

    all_peers = jxta_vector_size(peers);
    for (each_peer = 0; each_peer < all_peers; each_peer++) {
        Peerview_entry *peer = NULL;
        BIGNUM *peer_start;
        BIGNUM *peer_end;
        unsigned int each_segment;
        char *tmp_start;
        char *tmp_end;
        char *tmp_entry_start;
        char *tmp_entry_end;
        char *tmp_target_hash;
        JString *tmp_peer_j;

        res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), each_peer);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve peer entry.\n");
            goto FINAL_EXIT;
        }

        JXTA_OBJECT_CHECK_VALID(peer);

        peer_start = BN_new();
        BN_sub(peer_start, peer->target_hash, peer->target_hash_radius);

        peer_end = BN_new();
        BN_add(peer_end, peer->target_hash, peer->target_hash_radius);

/************************* */
        tmp_start = BN_bn2hex(peer_start);
        tmp_end = BN_bn2hex(peer_end);
        tmp_target_hash = BN_bn2hex(peer->target_hash);
        jxta_id_to_jstring(jxta_peer_peerid((Jxta_peer *) peer), &tmp_peer_j);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,  "******** histo **************\npeer:%s hash: %s \nstart:%s end:%s\n", jstring_get_string(tmp_peer_j)
                            , tmp_target_hash, tmp_start, tmp_end);
        free(tmp_start);
        free(tmp_end);
        free(tmp_target_hash);
        JXTA_OBJECT_RELEASE(tmp_peer_j);

/*************************** */
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
                    BN_free(segment_start);
                    BN_free(segment_end);
                    continue;
                }
            }
            tmp_start = BN_bn2hex(segment_start);
            tmp_end = BN_bn2hex(segment_end);

            print_start_end("SEGMENT ", each_segment, 0, segment_start, segment_end);
            free(tmp_start);
            free(tmp_end);

            /* We have a segment start and end, now we need to insert it into the existing histogram. */
            for (each_histogram_entry = 0; each_histogram_entry <= jxta_vector_size(histogram); each_histogram_entry++) {
                Peerview_histogram_entry *entry = NULL;
                Peerview_histogram_entry *new_entry=NULL;
                Jxta_boolean done = FALSE;
                BIGNUM *adjusted_end = NULL;

                if (each_histogram_entry < jxta_vector_size(histogram)) {
                    /* get an existing histogram entry */
                    res = jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), each_histogram_entry);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Going through without an entry\n");
                }
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not retrieve histogram entry.\n");
                    /* bad status will result in loops breaking out and cleanup on allocated objects */
                    break;
                }

                tmp_start = BN_bn2hex(segment_start);
                tmp_end = BN_bn2hex(segment_end);
                tmp_entry_start = NULL != entry ? BN_bn2hex(entry->start):"(null)";
                tmp_entry_end = NULL != entry ? BN_bn2hex(entry->end):"(null)";

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%d Process seg_start:%s seg_end:%s\n",each_segment, tmp_start, tmp_end);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%d         entry_start:%s entry_end:%s\n",each_segment,tmp_entry_start, tmp_entry_end);
                free(tmp_start);
                free(tmp_end);
                if (entry) {
                    free(tmp_entry_start);
                    free(tmp_entry_end);
                }
                /* See if we need to create a histogram entry before the current one */
                if ((NULL == entry) || (BN_cmp(segment_start, entry->start) < 0)) {

                    if ((NULL == entry) || (BN_cmp(segment_end, entry->start) < 0)) {
                        /* Entirely before the existing entry or no existing entry */
                        new_entry = peerview_histogram_entry_new(segment_start, segment_end);
                        done = TRUE;
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else {
                        /* Create an entry for the portion before the existing entry */
                        adjusted_end = BN_new();
                        BN_sub(adjusted_end, entry->start, BN_value_one());
                        new_entry = peerview_histogram_entry_new(segment_start, adjusted_end);
                        BN_copy(segment_start, entry->start);
                    }
                /* if we need to create an overlap entry within the current entry */
                } else if ((BN_cmp(segment_start, entry->end) < 0) && (BN_cmp(entry->start, segment_start) < 0)) {
                    /* adjust the current entry */

                    adjusted_end = BN_new();
                    BN_copy(adjusted_end, entry->end);
                    BN_sub(entry->end, segment_start, BN_value_one());
                    new_entry = peerview_histogram_entry_new(segment_start, adjusted_end);
                    print_start_end(" --------> Add overlap entry (0) ", each_segment, each_histogram_entry, new_entry->start, new_entry->end);

                    jxta_vector_add_object_last(new_entry->peers, (Jxta_object *) peer);
                    jxta_vector_addall_objects_last(new_entry->peers, entry->peers);
                    jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, ++each_histogram_entry);
                    /* each_histogram_entry++; */
                    if (BN_cmp(segment_end, maximum) <= 0) {
                        BN_add(segment_start, new_entry->end, BN_value_one());
                    } else {
                        done = TRUE;
                    }
                    JXTA_OBJECT_RELEASE(new_entry);
                    new_entry = NULL;
                }
                if (new_entry) {
                    print_start_end(" --------> Add new_entry ", each_segment, each_histogram_entry, new_entry->start, new_entry->end);

                    jxta_vector_add_object_last(new_entry->peers, (Jxta_object *) peer);
                    jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry);
                    JXTA_OBJECT_RELEASE(new_entry);
                    new_entry = NULL;
                }
                if (adjusted_end)
                    BN_free(adjusted_end);
                /* Are we done this peer? */
                if (done) {
                    if (NULL != entry) {
                        JXTA_OBJECT_RELEASE(entry);
                        entry = NULL;
                    }
                    break;
                }

                if (NULL == entry) {
                    continue;
                }

                /* Update the portion which overlaps with the current entry */
                if (BN_cmp(segment_start, entry->start) == 0) {
                    if (BN_cmp(segment_end, entry->end) < 0 && BN_cmp(segment_end, segment_start) != 0) {
                        new_entry = peerview_histogram_entry_new(segment_start, segment_end);

                        BN_add(entry->start, segment_end, BN_value_one());  /* adjust the active entry start. */

                        /* Split the entry in two,  */

                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        jxta_vector_addall_objects_last(new_entry->peers, entry->peers);
                        print_start_end(" --------> Add overlap entry (1) ", each_segment, each_histogram_entry, new_entry->start, new_entry->end);
                        jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry++);
                        JXTA_OBJECT_RELEASE(new_entry);
                        new_entry = NULL;
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else if (BN_cmp(segment_end, entry->end) == 0) {
                        /* Exactly the same range, just add this peer to the entry */
                        print_start_end(" --------> Add overlap peer (2) ", each_segment, each_histogram_entry, entry->start, entry->end);

                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        BN_add(segment_start, segment_end, BN_value_one());
                    } else {
                        /* Peer range ends after the current range */
                        print_start_end(" --------> Add overlap peer (3) ", each_segment, each_histogram_entry, entry->start, entry->end);

                        jxta_vector_add_object_last(entry->peers, (Jxta_object *) peer);
                        BN_add(segment_start, entry->end, BN_value_one());
                    }
                }
                if (new_entry) {
                    JXTA_OBJECT_RELEASE(new_entry);
                    new_entry = NULL;
                }
                JXTA_OBJECT_RELEASE(entry);
                if (BN_cmp(segment_start, segment_end) > 0) {
                    break;
                }
            }

            if (JXTA_SUCCESS == res) {

                /* Add any remaining bit. */
                if (BN_cmp(segment_start, segment_end) <= 0) {
                    Peerview_histogram_entry *new_entry = peerview_histogram_entry_new(segment_start, segment_end);

                    print_start_end(" --------> Add remaining new_entry ", each_segment, each_histogram_entry, new_entry->start, new_entry->end);

                    jxta_vector_add_object_last(new_entry->peers, (Jxta_object *) peer);
                    jxta_vector_add_object_at(histogram, (Jxta_object *) new_entry, each_histogram_entry++);
                }
            }
            else
            {
                BN_free(segment_start);
                BN_free(segment_end);
                break;
            }

            BN_free(segment_start);
            BN_free(segment_end);
        }

        BN_free(peer_start);
        BN_free(peer_end);

        JXTA_OBJECT_RELEASE(peer);
    
        if (JXTA_SUCCESS != res)
            break;
    }

FINAL_EXIT:

    *histo = histogram;
    /* print_histogram_details(histogram); */
    if (peers)
        JXTA_OBJECT_RELEASE(peers);
    return res;
}

static Jxta_status create_new_peerview(Jxta_peerview * me)
{
    char * tmp;
    int i;
    int clusters;
    BIGNUM *seg_per_address=NULL;
    BIGNUM *addr_per_cluster=NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *address = NULL;
    BIGNUM *self_address = NULL;

    /* We have probed a number of times and not found a peerview. Let's make our own. */
    ctx = BN_CTX_new();
    addr_per_cluster = BN_new();
    address = BN_new();
    self_address = BN_new();
    me->instance_mask = BN_new();
    BN_rand(me->instance_mask, (SHA_DIGEST_LENGTH * CHAR_BIT), -1, 0);

    tmp = BN_bn2hex(me->instance_mask);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "self chosen Instance Mask : %s\n", tmp);
    free(tmp);

    if (jxta_RdvConfig_pv_get_address_assign_mode(me->rdvConfig) == config_addr_assign_predictable) {
        /* start in the middle of cluster 0 */
        BN_add(self_address, self_address, me->cluster_divisor);
        BN_rshift(self_address, self_address, 1);

    } else {
        /* Choose our address randomly. */
        BN_rand(self_address, (SHA_DIGEST_LENGTH * CHAR_BIT), -1, 0);
        /* always create an address in cluster 0 */
        BN_rshift(self_address, self_address, me->clusters_count - 1);
    }

    tmp = BN_bn2hex(self_address);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "self chosen Target Hash Address: %s\n", tmp);
    free(tmp);

    create_self_pve(me, self_address);
    clusters = me->clusters_count;
  
    seg_per_address = BN_new();
    BN_set_word(addr_per_cluster, 16);

    BN_div(seg_per_address, NULL, me->cluster_divisor, addr_per_cluster, ctx);

    while (0 < clusters--) {

        /* create 16 addresses in each cluster */
        for (i=0; i<16; i++) {
            JString *hash_j;

            BN_add(address, address, seg_per_address);
            tmp = BN_bn2hex(address);

            hash_j = jstring_new_2(tmp);

            if (0 != BN_cmp(address, self_address) && BN_cmp(address, me->hash_space) < 0 ) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Created %s free_hash entry\n", jstring_get_string(hash_j));
                jxta_vector_add_object_last(me->free_hash_list, (Jxta_object *) hash_j);
            }

            free(tmp);
            JXTA_OBJECT_RELEASE(hash_j);
        }
    }

    BN_free(address);
    BN_free(self_address);
    BN_free(addr_per_cluster);
    BN_free(seg_per_address);
    BN_CTX_free(ctx);
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
    while (!sent_seed && NULL != me->activity_locate_seeds && (jxta_vector_size(me->activity_locate_seeds) > 0)) {
        res = jxta_vector_remove_object_at(me->activity_locate_seeds, JXTA_OBJECT_PPTR(&seed), 0);
        if (JXTA_SUCCESS != res) {
            continue;
        }

        if (jxta_peer_get_expires(seed) > jpr_time_now()) {

            res = peerview_send_ping_2(me, seed, NULL, TRUE);
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
/*        apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_maintain, myself, 1000, myself);
        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE 
                            "PV[%pp] ACT[locate]: Could not schedule peerview maintenance activity.\n",
                            myself);
        }
*/
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

    
    /* Reschedule another check. */
    apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_addressing, myself, 
                                                    jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig), myself);

    if (APR_SUCCESS != apr_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[addressing] [%pp]: Could not reschedule activity.\n",
                        myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[addressing] [%pp]: Rescheduled.\n", myself);
    }
    JXTA_OBJECT_RELEASE(pves);
    return NULL;
}

static void peerview_send_address_assign_unlock(Jxta_peerview *myself)
{
    char **keys=NULL;
    char **keys_save=NULL;

    myself->assign_state = ASSIGN_IDLE;
    myself->activity_address_locking_peers_sent=0;
    if (myself->activity_address_locking_peer) {
        free(myself->activity_address_locking_peer);
        myself->activity_address_locking_peer = NULL;
    }
    myself->activity_address_locking_ends = jpr_time_now();
    keys = jxta_hashtable_keys_get(myself->activity_address_locked_peers);
    keys_save = keys;
    while (*keys) {
        Jxta_id *peer_id=NULL;

        jxta_id_from_cstr(&peer_id, *keys);
        if (NULL != peer_id && 0 != strcmp(*keys, myself->pid_c)) {
            Jxta_peer *dest=NULL;

            dest = jxta_peer_new();
            jxta_peer_set_peerid(dest, peer_id);
            peerview_send_address_assign_msg(myself, dest, myself->activity_address_assign_peer
                                , ADDRESS_ASSIGN_UNLOCK, NULL, NULL);

            JXTA_OBJECT_RELEASE(dest);
        }
        if (peer_id)
            JXTA_OBJECT_RELEASE(peer_id);
        free(*(keys++));
    }
    while(*keys) free(*(keys++));
    free(keys_save);

}

static void peerview_send_address_assign_and_unlock(Jxta_peerview *myself, BIGNUM *target_hash, Jxta_vector *hash_list)
{

    if (myself->activity_address_assign_peer && NULL != target_hash) {
        peerview_send_address_assign_msg(myself, myself->activity_address_assign_peer, myself->activity_address_assign_peer
            , ADDRESS_ASSIGN
            , target_hash, hash_list);
    }

    peerview_send_address_assign_unlock(myself);

    JXTA_OBJECT_RELEASE(myself->activity_address_assign_peer);
    myself->activity_address_assign_peer = NULL;
}

static Jxta_status peerview_send_address_assigned(Jxta_peerview *myself, Jxta_peer *dest, BIGNUM *target_hash)
{
    Jxta_status res;

    res = peerview_send_address_assign_msg(myself, dest, NULL
        , ADDRESS_ASSIGN_ASSIGNED, target_hash, NULL);
    if (res != JXTA_SUCCESS) {
        
    } else {
        apr_status_t apr_res;

        myself->activity_address_assigned_ends = jpr_time_now() + jxta_RdvConfig_pv_voting_wait(myself->rdvConfig);
        myself->assign_state = ASSIGN_ASSIGNED_WAITING_RSP;
        /* Reschedule another check. */
        apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_address_locking, myself
            , jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig), &myself->assign_state);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[address locking] [%pp]: Could not reschedule activity.\n",
                        myself);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[address locking] [%pp]: Rescheduled.\n", myself);
        }
    }
    return res;
}

static Jxta_status half_the_free_list(Jxta_peerview *myself, int cluster, Jxta_vector ** half_list)
{
    Jxta_status res = JXTA_SUCCESS;
    int i;
    Jxta_vector *cluster_list=NULL;
    Jxta_boolean toggle=FALSE;
    Jxta_hashtable *cluster_hash=NULL;
    char **keys;
    char **keys_save;

    cluster_hash = jxta_hashtable_new(0);
    for (i=0; i < jxta_vector_size(myself->free_hash_list); i++) {
        JString *hash_j=NULL;
        BIGNUM *target_hash=NULL;
        int cluster_from_hash=0;
        char tmp[16];


        res = jxta_vector_get_object_at(myself->free_hash_list, JXTA_OBJECT_PPTR(&hash_j), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve entry from free_hash_list res:%d\n", res);
            continue;
        }
        BN_hex2bn(&target_hash, jstring_get_string(hash_j));
        cluster_from_hash = cluster_for_hash(myself, target_hash);
        memset(tmp, 0, sizeof(tmp));
        apr_snprintf(tmp, sizeof(tmp), "%d", cluster_from_hash);

        if (JXTA_SUCCESS != jxta_hashtable_get(cluster_hash, tmp, strlen(tmp) + 1, JXTA_OBJECT_PPTR(&cluster_list))) {
            cluster_list = jxta_vector_new(0);
            jxta_hashtable_put(cluster_hash, tmp, strlen(tmp) + 1, (Jxta_object *) cluster_list);
        }
        jxta_vector_add_object_last(cluster_list, (Jxta_object *) hash_j);
        jxta_vector_remove_object_at(myself->free_hash_list, NULL, i--);

        JXTA_OBJECT_RELEASE(hash_j);
        JXTA_OBJECT_RELEASE(cluster_list);
        cluster_list = NULL;
        BN_free(target_hash);
    }
    keys = jxta_hashtable_keys_get(cluster_hash);
    keys_save = keys;
    *half_list = jxta_vector_new(0);
    while (*keys) {
        if (JXTA_SUCCESS != jxta_hashtable_get(cluster_hash, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&cluster_list))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve entry from cluster_hash key:%s res:%d\n", *keys, res);
            free(*(keys++));
            continue;
        }
        toggle = TRUE;

        for (i = 0; i < jxta_vector_size(cluster_list); i++) {
            Jxta_object *hash=NULL;

            res = jxta_vector_get_object_at(cluster_list, JXTA_OBJECT_PPTR(&hash), i);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve entry from cluster_list res:%d\n", res);
                continue;
            }

            if (toggle) {
                jxta_vector_add_object_last(*half_list, hash);
                toggle = FALSE;
            } else {
                jxta_vector_add_object_last(myself->free_hash_list, hash);
                toggle = TRUE;
            }
            JXTA_OBJECT_RELEASE(hash);
        }
        JXTA_OBJECT_RELEASE(cluster_list);
        cluster_list = NULL;
        free(*(keys++));
    }
    while (*keys) free(*(keys++));
    free(keys_save);

    JXTA_OBJECT_RELEASE(cluster_hash);
    if (cluster_list)
        JXTA_OBJECT_RELEASE(cluster_list);
    return res;
}

static Jxta_status peerview_broadcast_address_assign(Jxta_peerview *myself, Jxta_address_assign_msg_type msg_type, int *count, Jxta_hashtable *peers, Jxta_peer *assign_peer, Jxta_vector *send_free_entries, Assign_state state)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_vector * pves=NULL;
    unsigned int all_pves;
    unsigned int each_pve;
    Peerview_entry * a_pve=NULL;

    pves = peerview_get_all_pves(myself);

    all_pves = jxta_vector_size(pves);
    (*count) = 0;
    jxta_hashtable_clear(peers);
    for (each_pve = 0; each_pve < all_pves; each_pve++) {
        res = jxta_vector_get_object_at(pves, JXTA_OBJECT_PPTR(&a_pve), each_pve);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "ACT[address locking] [%pp]: Could not get pve.\n", myself);
            continue;
        }

        if (jxta_id_equals(myself->pid, jxta_peer_peerid((Jxta_peer *) a_pve))) {
            /* It's myself! Handle things differently. */
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }
        if (jxta_peer_get_expires((Jxta_peer *) a_pve) > jpr_time_now()) {
            JString *pve_peer_j=NULL;

            jxta_id_to_jstring(((Jxta_peer *) a_pve)->peerid, &pve_peer_j);
            res = peerview_send_address_assign_msg(myself, (Jxta_peer *) a_pve, assign_peer
                    , msg_type, NULL, send_free_entries);
            if (JXTA_SUCCESS == res) {

                myself->assign_state = state;
                (*count)++;
                if (JXTA_SUCCESS != jxta_hashtable_contains(peers
                    ,jstring_get_string(pve_peer_j), jstring_length(pve_peer_j) + 1)) {
                    Jxta_vector *free_hash_entries=NULL;

                    free_hash_entries = jxta_vector_new(0);
                    jxta_hashtable_put(peers
                        ,jstring_get_string(pve_peer_j), jstring_length(pve_peer_j) + 1, (Jxta_object *) free_hash_entries);

                    JXTA_OBJECT_RELEASE(free_hash_entries);
                }
            }
            JXTA_OBJECT_RELEASE(pve_peer_j);
        }
    JXTA_OBJECT_RELEASE(a_pve);
    }
    if (pves)
        JXTA_OBJECT_RELEASE(pves);

    return res;
}

static void *APR_THREAD_FUNC activity_peerview_address_locking(apr_thread_t * thread, void *arg)
{
    Jxta_status res=JXTA_SUCCESS;
    apr_status_t apr_res;
    Jxta_peerview *myself = PTValid(arg, Jxta_peerview);
    Jxta_boolean locked = FALSE;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[address locking] [%pp]: Running. state:%s\n", myself, assign_state_description[myself->assign_state]);

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    switch (myself->assign_state) {
        case ASSIGN_IDLE:
            goto FINAL_EXIT;
            break;
        case ASSIGN_SEND_FREE:
            if (JXTA_SUCCESS != peerview_broadcast_address_assign(myself
                        , ADDRESS_ASSIGN_FREE
                        , &myself->activity_address_free_peers_sent
                        , myself->activity_address_free_peers
                        , myself->activity_address_assign_peer
                        , myself->possible_free_list
                        , ASSIGN_FREE_WAITING_RSP)) {
                goto FINAL_EXIT;
            }
            if (ASSIGN_FREE_WAITING_RSP) {
                myself->activity_address_free_ends = jpr_time_now() + jxta_RdvConfig_pv_address_assign_expiration(myself->rdvConfig);
            } else {
                goto FINAL_EXIT;
            }
            break;
        case ASSIGN_FREE_WAITING_RSP:

            if (myself->activity_address_free_ends < jpr_time_now()) {
                myself->assign_state = ASSIGN_IDLE;
                if (myself->activity_address_locking_peer) {
                    free(myself->activity_address_locking_peer);
                    myself->activity_address_locking_peer = NULL;
                }
                goto FINAL_EXIT;
            }
            break;
        case ASSIGN_SEND_LOCK:
            /* FIXME: get_all_pves better not to include self_pve to avoid checking, modify get/add/check etc to include self_pve is less
            * expensive
            */
            if (JXTA_SUCCESS != peerview_broadcast_address_assign(myself
                        , ADDRESS_ASSIGN_LOCK
                        , &myself->activity_address_locking_peers_sent
                        , myself->activity_address_locked_peers
                        , myself->activity_address_assign_peer
                        , NULL
                        , ASSIGN_LOCK_WAITING_RSP)) {
                goto FINAL_EXIT;
            }

            /* if locks were sent start the timer and wait else send an assignment */
            if (ASSIGN_LOCK_WAITING_RSP == myself->assign_state) {
                myself->activity_address_locking_ends = jpr_time_now() + jxta_RdvConfig_pv_address_assign_expiration(myself->rdvConfig);
            } else {
                Jxta_peer *dest=NULL;
                Jxta_vector *half_list=NULL;
                BIGNUM *target_hash=NULL;
                int i;
                int cluster;

                cluster = myself->my_cluster;
                get_empty_cluster(myself, &cluster);

                for (i=0; target_hash == NULL && i<jxta_vector_size(myself->free_hash_list); i++) {
                    JString *hash_entry=NULL;

                    res = jxta_vector_get_object_at(myself->free_hash_list, JXTA_OBJECT_PPTR(&hash_entry), i);
                    if (JXTA_SUCCESS != res) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve entry from my free hash list res:%d\n", res);
                        continue;
                    }

                    BN_hex2bn(&target_hash, jstring_get_string(hash_entry));
                    if (cluster != cluster_for_hash(myself, target_hash)) {
                        BN_free(target_hash);
                        target_hash = NULL;
                    } else {
                        if (BN_cmp(target_hash, myself->clusters[cluster].mid) < 0) {
                            BN_free(target_hash);
                            target_hash = NULL;
                        } else {
                            jxta_vector_remove_object_at(myself->free_hash_list, NULL, i--);
                        }
                    }
                    JXTA_OBJECT_RELEASE(hash_entry);
                }
                half_the_free_list(myself, cluster, &half_list);
                peerview_send_address_assign_and_unlock(myself, target_hash, half_list);

                myself->assign_state = ASSIGN_IDLE;
                if (myself->activity_address_locking_peer) {
                    free(myself->activity_address_locking_peer);
                    myself->activity_address_locking_peer = NULL;
                }
                JXTA_OBJECT_RELEASE(dest);
                if (half_list)
                    JXTA_OBJECT_RELEASE(half_list);
                if (target_hash)
                    BN_free(target_hash);
                /* process is over - do not reschedule */
                goto FINAL_EXIT;
            }
            break;
        case ASSIGN_LOCK_WAITING_RSP:

            /* if the locking cycle has expired */

            if (myself->activity_address_locking_ends < jpr_time_now()) {
                peerview_send_address_assign_and_unlock(myself, NULL, NULL);
                goto FINAL_EXIT;
            }
            break;
        case ASSIGN_ASSIGNED_WAITING_RSP:

            /* if the assigned response message was never received can't use the address */
            if (myself->activity_address_assigned_ends < jpr_time_now()) {
                myself->assign_state = ASSIGN_IDLE;
                myself->activity_address_locking_ends = jpr_time_now();
                if (myself->activity_address_locking_peer) {
                    free(myself->activity_address_locking_peer);
                    myself->activity_address_locking_peer = NULL;
                }
                goto FINAL_EXIT;
            }
            break;
        case ASSIGN_LOCKED:
            if (myself->activity_address_locking_ends < jpr_time_now()) {
                myself->assign_state = ASSIGN_IDLE;
                if (myself->activity_address_locking_peer) {
                    free(myself->activity_address_locking_peer);
                    myself->activity_address_locking_peer = NULL;
                }
                goto FINAL_EXIT;
            }
            break;
        default:
            break;
    }

/* RESCHEDULE_EXIT: */

    /* Reschedule another check. */
    apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_address_locking, myself
            , jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig), &myself->assign_state);

    if (APR_SUCCESS != apr_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[address locking] [%pp]: Could not reschedule activity.\n",
                        myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[address locking] [%pp]: Rescheduled.\n", myself);
    }

FINAL_EXIT:

    if (locked)
        apr_thread_mutex_unlock(myself->mutex);

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

        res = peerview_send_pong(myself, (Jxta_peer *) a_pve, PONG_STATUS, FALSE, FALSE, FALSE);
        JXTA_OBJECT_RELEASE(a_pve);
    }

    JXTA_OBJECT_RELEASE(pves);

    apr_thread_mutex_lock(myself->mutex);

    myself->state = PV_MAINTENANCE;

    apr_thread_mutex_unlock(myself->mutex);

    apr_thread_mutex_lock(maintain_cond_lock);
    apr_thread_cond_signal(maintain_cond);
    apr_thread_mutex_unlock(maintain_cond_lock);

/*    if (APR_SUCCESS != apr_thread_pool_push(myself->thread_pool, activity_peerview_maintain, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, myself)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not initiate maintain activity. [%pp]\n", myself);
    }
*/
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
            JString *peerid_j=NULL;

            jxta_id_to_jstring(jxta_peer_peerid(ping_peer), &peerid_j);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Ping referral %s\n", jstring_get_string(peerid_j));
            res = peerview_send_ping(me, ping_peer, NULL, FALSE);

            JXTA_OBJECT_RELEASE(ping_peer);
            JXTA_OBJECT_RELEASE(peerid_j);
        }

        /* We increment unconditionally, a bad peer is one we should get rid of. */
        me->activity_maintain_referral_pings_sent++;
    }

    return JXTA_SUCCESS;
}

static Jxta_status check_pves(Jxta_peerview * me, Jxta_vector *pves, Jxta_boolean all, Jxta_hashtable **msgs)
{
    Jxta_status res;
    unsigned int all_pves;
    unsigned int each_pve;
    Peerview_entry * a_pve;
    Jxta_time now;
    Jxta_time expires;

    /* Kill, ping or request address from current pves */
    all_pves = jxta_vector_size(pves);

    if (NULL == *msgs) {
        *msgs = jxta_hashtable_new(0);
    }

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
        if ((expires - now) <= jxta_RdvConfig_pv_ping_due(me->rdvConfig) || all) {
            /* Ping for refresh */
            apr_uuid_t * pv_id_gen = NULL;
            Jxta_version * peer_version=NULL;
            JString *peerid_j=NULL;
            JString *grp_id_j=NULL;

            if (a_pve->pv_id_gen_set) {
                pv_id_gen = &a_pve->pv_id_gen;
            }
            peer_version = jxta_PA_get_version(((_jxta_peer_entry *) a_pve)->adv);
            jxta_id_to_jstring(jxta_peer_peerid((Jxta_peer *) a_pve), &peerid_j);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending ping for maintenance to %s\n", jstring_get_string(peerid_j));
            if (jxta_version_compatible_1(ENDPOINT_CONSOLIDATION, peer_version)) {
                Jxta_pv_ping_msg_group_entry *entry=NULL;
                Jxta_hashtable *grp_hash=NULL;

                if (JXTA_SUCCESS != jxta_hashtable_get(*msgs, jstring_get_string(peerid_j), jstring_length(peerid_j) + 1, JXTA_OBJECT_PPTR(&grp_hash))) {
                    grp_hash = jxta_hashtable_new(0);
                    jxta_hashtable_put(*msgs, jstring_get_string(peerid_j), jstring_length(peerid_j) + 1, (Jxta_object *) grp_hash);
                }
                jxta_id_get_uniqueportion(me->gid, &grp_id_j);

                entry = jxta_pv_ping_msg_entry_new((Jxta_peer *) a_pve, a_pve->adv_gen, a_pve->adv_gen_set, jstring_get_string(grp_id_j)
                                        , jxta_version_compatible_1(PEERVIEW_UUID_IMPLEMENTATION, peer_version) ? TRUE:FALSE);


                jxta_hashtable_put(grp_hash, jstring_get_string(grp_id_j), jstring_length(grp_id_j) + 1, (Jxta_object *) entry);

                jxta_hashtable_put(*msgs, jstring_get_string(peerid_j), jstring_length(peerid_j) + 1, (Jxta_object *) grp_hash);

                if (grp_hash)
                    JXTA_OBJECT_RELEASE(grp_hash);
                JXTA_OBJECT_RELEASE(entry);
            } else {
                Jxta_peer *single_dest;

                single_dest = jxta_peer_new();
                jxta_peer_set_peerid(single_dest, jxta_peer_peerid((Jxta_peer *) a_pve));
                peerview_send_ping(me, single_dest,  &a_pve->adv_gen, jxta_version_compatible_1(PEERVIEW_UUID_IMPLEMENTATION, peer_version) ? TRUE:FALSE);

                JXTA_OBJECT_RELEASE(single_dest);
            }

            if (peer_version)
                JXTA_OBJECT_RELEASE(peer_version);
            JXTA_OBJECT_RELEASE(peerid_j);
            JXTA_OBJECT_RELEASE(grp_id_j);
        }

        JXTA_OBJECT_RELEASE(a_pve);
    }

    return JXTA_SUCCESS;
}

static void peerview_add_monitor_entry(Jxta_peerview * pv)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_monitor_entry *entry;
    Jxta_monitor_service *monitor;
    Jxta_peerview_monitor_entry * pv_entry;
    Jxta_peerview_pong_msg * pong;
    JString * rdv_id_j;

    monitor = jxta_monitor_service_get_instance();

    if (NULL == monitor) return;

    pv_entry = jxta_peerview_monitor_entry_new();

    jxta_peerview_monitor_entry_set_rdv_time(pv_entry, 3000);
    jxta_peerview_monitor_entry_set_src_peer_id(pv_entry, pv->pid); 

    apr_thread_mutex_lock(pv->mutex);

    res = peerview_create_pong_msg(pv, PONG_STATUS, FALSE, &pong, FALSE);

    apr_thread_mutex_unlock(pv->mutex);

    if (JXTA_SUCCESS == res) {
        jxta_peerview_monitor_entry_set_pong_msg(pv_entry, pong);
        JXTA_OBJECT_RELEASE(pong);
    }

    entry = jxta_monitor_entry_new();

    jxta_monitor_entry_set_entry(entry, (Jxta_advertisement *) pv_entry);

    jxta_id_to_jstring(jxta_rendezvous_classid, &rdv_id_j);

    jxta_monitor_entry_set_context(entry, pv->groupUniqueID);
    jxta_monitor_entry_set_sub_context(entry, jstring_get_string(rdv_id_j));
    jxta_monitor_entry_set_type(entry, "jxta:PV3MonEntry");

    jxta_monitor_service_add_monitor_entry(monitor, pv->groupUniqueID , jstring_get_string(rdv_id_j), entry, TRUE);

    JXTA_OBJECT_RELEASE(rdv_id_j);
    JXTA_OBJECT_RELEASE(entry);
    JXTA_OBJECT_RELEASE(pv_entry);
}

static Jxta_vector * peerview_get_pvs(JString *request_group)
{
    Jxta_vector *groups;
    Jxta_vector *pvs;
    int i;

    pvs = jxta_vector_new(0);
    groups = jxta_get_registered_groups();
    for (i=0; NULL != groups && i<jxta_vector_size(groups); i++) {
        Jxta_PG *group;
        JString *group_j;
        Jxta_rdv_service *rdv;
        Jxta_peerview *pv;
        Jxta_boolean found=TRUE;

        jxta_vector_get_object_at(groups, JXTA_OBJECT_PPTR(&group), i);
        if (NULL != request_group) {
            Jxta_id *groupid;
            found = FALSE;

            jxta_PG_get_GID(group, &groupid);
            jxta_id_get_uniqueportion(groupid, &group_j);

            found = (0 == jstring_equals(group_j, request_group)) ? TRUE:FALSE;

            JXTA_OBJECT_RELEASE(groupid);
            JXTA_OBJECT_RELEASE(group_j);
        }
        if (found) {
            jxta_PG_get_rendezvous_service(group, &rdv);

            pv = rdv_service_get_peerview_priv(rdv);

            if (NULL != pv) {
                jxta_vector_add_object_last(pvs, (Jxta_object *) pv);
            }
            JXTA_OBJECT_RELEASE(rdv);
        }
        JXTA_OBJECT_RELEASE(group);
        if (found && NULL !=request_group) break;
    }

    JXTA_OBJECT_RELEASE(groups);
    return pvs;
}

/**
*   The maintain task is responsible for :
*       - expiring dead PVEs
*       - pinging near-dead PVEs
*       - pinging new potential PVEs
*       - adding monitor entries to the monitor service
*       - sending address request messages. (optional)
**/
static void *APR_THREAD_FUNC activity_peerview_maintain(apr_thread_t * thread, void *cookie)
{
    Jxta_peerview *myself = PTValid(cookie, Jxta_peerview);

    while (TRUE) {
        Jxta_vector *all_pvs=NULL;
        int i;
        Jxta_status res;
        Jxta_hashtable *ret_msgs=NULL;
        char **peers=NULL;
        char **peers_save=NULL;
        Jxta_hashtable *grps_hash=NULL;
        Jxta_version * peer_version=NULL;
        size_t msgs_size=0;
        apr_interval_time_t wwait;
        apr_status_t apr_status;

        maintain_state = MAINTAIN_RUNNING;
        maintain_running = TRUE;
        apr_thread_mutex_lock(maintain_lock);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[maintain] [%pp]: Running. \n", myself);

        all_pvs = peerview_get_pvs(NULL);

        if (jxta_vector_size(all_pvs) < 1) {
            JXTA_OBJECT_RELEASE(all_pvs);
            maintain_state = MAINTAIN_SHUTDOWN;
            break;
        }
        if (NULL != ret_msgs) {
            jxta_hashtable_stats(ret_msgs, NULL, &msgs_size, NULL, NULL, NULL);
        }
        for (i=0; i < jxta_vector_size(all_pvs); i++) {
            Jxta_peerview *pv=NULL;
            Jxta_rdv_service *rdv=NULL;
            Jxta_boolean is_rdv;

            if (JXTA_SUCCESS != jxta_vector_get_object_at(all_pvs, JXTA_OBJECT_PPTR(&pv), i)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ACT[maintain] [%pp]: Unable to retrieve a peerview [%pp] i:%d.\n", myself, all_pvs, i);
                continue;
            }
            jxta_PG_get_rendezvous_service(pv->group, &rdv);
            is_rdv = jxta_rdv_service_is_rendezvous(rdv);
            JXTA_OBJECT_RELEASE(rdv);
            if (!is_rdv) {
                /* terminate_maintain_thread(); */
                JXTA_OBJECT_RELEASE(pv);
                continue;
            }
            if (PV_MAINTENANCE == pv->state) {
                peerview_maintain(pv, msgs_size > 0 ? TRUE:FALSE, &ret_msgs);
            }
            JXTA_OBJECT_RELEASE(pv);
        }
        if (NULL != ret_msgs) {
            peers = jxta_hashtable_keys_get(ret_msgs);
        }
        peers_save = peers;
        while (NULL != peers && *peers) {
            Jxta_peerview_ping_msg *ping=NULL;
            char **groups=NULL;
            char **groups_save=NULL;
            Jxta_id *peerid;

            if (JXTA_SUCCESS != jxta_hashtable_get(ret_msgs, *peers, strlen(*peers) + 1, JXTA_OBJECT_PPTR(&grps_hash))) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ACT[maintain] [%pp]: Able to retrieve a group hash key:%s\n",myself, *peers);
                free(*(peers++));
                continue;
            }
            jxta_id_from_cstr(&peerid, *peers);

            groups = jxta_hashtable_keys_get(grps_hash);
            groups_save = groups;
            while (NULL != groups && *groups) {
                Jxta_pv_ping_msg_group_entry *entry=NULL;
                Peerview_entry *a_pve=NULL;

                if (JXTA_SUCCESS != jxta_hashtable_get(grps_hash, *groups, strlen(*groups) + 1, JXTA_OBJECT_PPTR(&entry))) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ACT[maintain] [%pp]: grps_hash:[%pp]: Unable to retrieve a group:%s\n", myself, grps_hash, *groups);
                } else {
                    a_pve = (Peerview_entry *) jxta_peerview_ping_msg_entry_get_pve(entry);
                    if (NULL == a_pve || NULL == ((_jxta_peer_entry *) a_pve)->adv) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ACT[maintain] [%pp]: Able to retrieve a pve?:%s for %s\n", myself
                            , NULL == a_pve ? "no":"yes", *peers);
                        JXTA_OBJECT_RELEASE(peerid);
                        JXTA_OBJECT_RELEASE(grps_hash);
                        if (a_pve)
                            JXTA_OBJECT_RELEASE(a_pve);
                        continue;
                    }
                    peer_version = jxta_PA_get_version(((_jxta_peer_entry *) a_pve)->adv);
                    if (jxta_version_compatible_1(ENDPOINT_CONSOLIDATION, peer_version) ) {
                        if (NULL == ping) {
                            res = peerview_create_ping_msg(myself, (Jxta_peer *) a_pve, &a_pve->adv_gen
                                , jxta_version_compatible_1(PEERVIEW_UUID_IMPLEMENTATION, peer_version) ? TRUE:FALSE
                                , &ping);

                        }
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "ACT[maintain] [%pp]: Add a group to the ping message:%s\n"
                                            , myself, *groups);
                        jxta_peerview_ping_msg_add_group_entry(ping, entry);
                    } else {
                        JString *group_j=NULL;
                        Jxta_vector *pvs=NULL;

                        pvs = peerview_get_pvs(group_j);
                        if (jxta_vector_size(pvs) > 0) {
                            Jxta_peerview *pv=NULL;

                            jxta_vector_get_object_at(pvs, JXTA_OBJECT_PPTR(&pv), 0);
                            res = peerview_send_ping(pv, (Jxta_peer *) a_pve, &a_pve->adv_gen
                                , jxta_version_compatible_1(PEERVIEW_UUID_IMPLEMENTATION, peer_version) ? TRUE:FALSE);
                            JXTA_OBJECT_RELEASE(pv);
                        }
                        JXTA_OBJECT_RELEASE(pvs);
                    }
                }

                JXTA_OBJECT_RELEASE(entry);
                free(*(groups++));
                if (a_pve)
                    JXTA_OBJECT_RELEASE(a_pve);
                if (peer_version)
                    JXTA_OBJECT_RELEASE(peer_version);
                peer_version = NULL;
            }
            if (NULL != ping) {
                Jxta_peer *dest;

                dest = jxta_peer_new();
                jxta_peer_set_peerid(dest, peerid);
                res = peerview_send_ping_consolidated(myself, dest, ping);
                JXTA_OBJECT_RELEASE(dest);
            }

            if (groups) {
                while (*groups) free(*(groups++));
            }
            if (grps_hash)
                JXTA_OBJECT_RELEASE(grps_hash);
            if (peer_version)
                JXTA_OBJECT_RELEASE(peer_version);
            if (groups_save)
                free(groups_save);
            if (ping)
                JXTA_OBJECT_RELEASE(ping);
            if (peerid)
                JXTA_OBJECT_RELEASE(peerid);
            free(*(peers++));
        }
        if (peers) {
            while (*peers) free(*(peers++));
        }

        if (peers_save)
            free(peers_save);
        if (ret_msgs)
            JXTA_OBJECT_RELEASE(ret_msgs);
        if (all_pvs)
            JXTA_OBJECT_RELEASE(all_pvs);


        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[maintain] [%pp]: Rescheduled.\n", myself);

        wwait = jxta_RdvConfig_pv_maintenance_interval(myself->rdvConfig);
        apr_thread_mutex_unlock(maintain_lock);

        apr_status = apr_thread_cond_timedwait(maintain_cond, maintain_cond_lock, wwait);

        /* if it was a signal and not a time out */
        if (APR_TIMEUP != apr_status && MAINTAIN_SHUTDOWN == maintain_state) {
            break;
        }
    }
    maintain_state = MAINTAIN_SHUTDOWN;
    maintain_running = FALSE;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "ACT[maintain] [%pp]: Exit.\n", myself);
    apr_thread_exit(thread, APR_SUCCESS);
    return NULL;

}

static Jxta_status peerview_maintain(Jxta_peerview *myself, Jxta_boolean all, Jxta_hashtable **ret_msgs)
{
    Jxta_vector * current_pves;
    apr_status_t apr_res;


    apr_thread_mutex_lock(myself->mutex);

    if ((PV_ADDRESSING != myself->state) && (PV_MAINTENANCE != myself->state)) {
        apr_thread_mutex_unlock(myself->mutex);
        return JXTA_SUCCESS;
    }

    current_pves = peerview_get_all_pves(myself);

    check_pves(myself, current_pves, all, ret_msgs);

    apr_thread_mutex_unlock(myself->mutex);


    JXTA_OBJECT_RELEASE(current_pves);

    probe_referrals(myself);

    if (PV_ADDRESSING != myself->state && PV_MAINTENANCE != myself->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "peerview_maintain [%pp]: done. \n", myself);
        return JXTA_SUCCESS;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peerview_maintain [%pp]\n", myself);

    if (jxta_vector_size(myself->possible_free_list) > 0) {
        peerview_reclaim_addresses(myself);
    }
    if (!myself->activity_add && need_additional_peers(myself, myself->cluster_members)) {
        /* Start the add activity? */

        myself->activity_add = TRUE;
        apr_res = apr_thread_pool_push(myself->thread_pool, activity_peerview_add, myself,
                                            APR_THREAD_TASK_PRIORITY_HIGH, &myself->activity_add);
        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "peerview_maintain [%pp]: Could not start add activity.\n", myself);
            myself->activity_add = FALSE;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peerview_maintain [%pp]: Scheduled [Add].\n", myself);
        }
    }
    return JXTA_SUCCESS;
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
                res = peerview_send_pong(myself, peer, PONG_PROMOTE, TRUE, FALSE, FALSE);
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


    for (each_client = 0; !myself->activity_add_voting &&  (each_client < all_clients) && (each_client < myself->max_client_invitations); each_client++) {
        Jxta_peer *invitee;

        res = jxta_vector_remove_object_at(myself->activity_add_candidate_peers, JXTA_OBJECT_PPTR(&invitee), 0);

        if (JXTA_SUCCESS != res) {
            continue;
        }
        res = peerview_send_pong(myself, invitee, PONG_INVITE, FALSE, FALSE, FALSE);

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


    if (myself->activity_add_candidate_pongs_sent < myself->max_client_invitations && !myself->activity_add_voting) {
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

static int target_hash_sort(Peerview_entry **peer_a, Peerview_entry **peer_b)
{
    return BN_cmp((*peer_a)->target_hash, (*peer_b)->target_hash);
}

static void find_best_target_address(Peerview_address_assign_mode mode, Jxta_vector * histogram, BIGNUM * min, BIGNUM * max,
unsigned int attempts, BIGNUM * range, BIGNUM ** target_address)
{
    Jxta_status status;

    switch (mode) {
        case config_addr_assign_random:
        {
            status = histogram_random_entry(histogram, range, min, attempts, target_address);
            break;
        }
        case config_addr_assign_predictable:
        {
            status = histogram_predictable_entry(histogram, NULL, range, min, max, target_address);
            break;
        }
        case config_addr_assign_hybrid:
        case config_addr_assign_managed:
        default:
        {
            status = histogram_random_entry(histogram, range, min, attempts, target_address);
            break;
        }
    };
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error finding a target address - status:%d\n", status);
    }


}

static Jxta_status histogram_random_entry(Jxta_vector * histogram, BIGNUM * range, BIGNUM *min_bn, unsigned int attempts, BIGNUM **target_address)
{
    int i;
    BIGNUM **addresses;
    int best_loc=0;

    addresses = calloc(attempts, sizeof(BIGNUM *));
    for (i = 0; i < attempts; i++) {
        addresses[i] = BN_new();
        BN_pseudo_rand_range(addresses[i] , range);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Find best address with a Histogram size of %d\n", jxta_vector_size(histogram));

    for (i = 0; i < attempts; i++) {
        Peerview_histogram_entry *before_entry=NULL;
        Peerview_histogram_entry *after_entry=NULL;
        BIGNUM *best_start = NULL;
        char *tmp_target_hash;

        best_start = BN_new();
        tmp_target_hash = BN_bn2hex(addresses[i]);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "address %d -------------> %s\n", i , tmp_target_hash );
        histogram_get_location(histogram, addresses[i], &before_entry, &after_entry);
        if (NULL != before_entry && NULL != after_entry) {
            BN_sub(best_start, after_entry->start,  before_entry->end);
        }
        free(tmp_target_hash);
    }
    for (i = 0; i < attempts; i++) {
        if (best_loc == i) {
            *target_address = addresses[i];
        } else {
            free (addresses[i]);
        }
    }
    return JXTA_SUCCESS;
}

static Jxta_status histogram_predictable_entry(Jxta_vector * histogram, Jxta_vector *hash_list, BIGNUM *range, BIGNUM *min_bn, BIGNUM *max_bn, BIGNUM ** target_hash)
{
    Jxta_status res=JXTA_SUCCESS;
    int gap_idx = -1;
    int i;
    Peerview_histogram_entry * entry=NULL;
    BIGNUM *gap_bn=BN_new();
    BIGNUM *biggest_gap=BN_new();
    char * tmp_biggest_gap;
    BIGNUM *target_address=NULL;
    Jxta_vector *empty_space = NULL;

    histogram_get_empty_space(histogram, min_bn, max_bn, &empty_space);

    /* check for empty space */
    if (jxta_vector_size(empty_space) > 0) {
        /* find the largest empty space */
        for (i=0; i < jxta_vector_size(empty_space); i++) {
            jxta_vector_get_object_at(empty_space, JXTA_OBJECT_PPTR(&entry), i);
            BN_sub(gap_bn, entry->end, entry->start);
            if (BN_cmp(gap_bn, biggest_gap) > 0) {
                BN_copy(biggest_gap, gap_bn);
                gap_idx = i;
            }
            JXTA_OBJECT_RELEASE(entry);
       }
    }
    target_address = BN_new();
    if (0 == jxta_vector_size(empty_space) || -1 == gap_idx) {
        int num_peers = -1;
        BIGNUM *size_of_entry;
        BIGNUM *tmp_size;
        BIGNUM *split_address;

        size_of_entry = BN_new();
        BN_set_word(size_of_entry, 0);
        tmp_size = BN_new();
        split_address = BN_new();
        BN_set_word(tmp_size, 0);
        for (i = 0; i < jxta_vector_size(histogram); i++) {
            jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), i);
            BN_sub(tmp_size, entry->end, entry->start);
            if (BN_cmp(tmp_size, size_of_entry) > 0) {
                if (jxta_vector_size(entry->peers) < num_peers) {
                    num_peers = jxta_vector_size(entry->peers);
                    gap_idx = i;
                }
            }
            BN_copy(size_of_entry, tmp_size);
            JXTA_OBJECT_RELEASE(entry);
        }
        BN_free(size_of_entry);
        BN_free(tmp_size);
        BN_free(split_address);

        if (-1 == gap_idx) {
            BN_pseudo_rand_range(target_address , range);
        } else {
            BIGNUM *peer_hash=NULL;
            BIGNUM *how_far=NULL;

            peer_hash = BN_new();
            BN_set_word(peer_hash, 0);
            how_far = BN_new();
            jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), gap_idx);
            for (i=0; i < jxta_vector_size(entry->peers); i++) {
                Peerview_entry *peer=NULL;

                jxta_vector_get_object_at(entry->peers, JXTA_OBJECT_PPTR(&peer), i);
                /* BN_sub(how_far, peer->target_hash, entry->start); */
                BN_sub(how_far, entry->end, peer->target_hash);
                if (BN_cmp(how_far, peer_hash) > 0) {
                    BN_copy(peer_hash, peer->target_hash);
                }
                JXTA_OBJECT_RELEASE(peer);
            }

/*             BN_sub(*target_address, peer_hash, entry->start); */
            BN_sub(target_address, entry->end, peer_hash);
            BN_rshift1(target_address, target_address);

            BN_free(peer_hash);
            BN_free(how_far);
            JXTA_OBJECT_RELEASE(entry);
        }
        BN_add(target_address, target_address, min_bn);
    } else {
        tmp_biggest_gap = BN_bn2hex(biggest_gap);
        free(tmp_biggest_gap);

        jxta_vector_get_object_at(empty_space, JXTA_OBJECT_PPTR(&entry), gap_idx);
        if (BN_cmp(biggest_gap, range) > 0) {
            BN_copy(biggest_gap, range);
        }
        /* split the gap in half */
        BN_rshift1(biggest_gap, biggest_gap);
        BN_add(target_address, entry->start, biggest_gap);

        JXTA_OBJECT_RELEASE(entry);
    }
    tmp_biggest_gap = BN_bn2hex(target_address);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Assigning address:%s\n", tmp_biggest_gap);
    free(tmp_biggest_gap);

    *target_hash = target_address;
    if (biggest_gap)
        BN_free(biggest_gap);
    if (gap_bn)
        BN_free(gap_bn);
    if (empty_space)
        JXTA_OBJECT_RELEASE(empty_space);
    return res;
}

static Jxta_status histogram_get_location( Jxta_vector * histogram, BIGNUM * address, Peerview_histogram_entry **before_entry, Peerview_histogram_entry ** after_entry)
{
    Jxta_status status = JXTA_SUCCESS;
    Peerview_histogram_entry *last_entry=NULL;
    int i;

    for (i=0; i < jxta_vector_size(histogram); i++) {
        Peerview_histogram_entry *entry=NULL;

        status = jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), i);
        if (NULL != last_entry) {
            if ((BN_cmp(address, last_entry->end) > 0) && BN_cmp(address, entry->start) < 0) {
                *before_entry = last_entry;
                *after_entry = entry;
                return JXTA_SUCCESS;
            }
        }
        if (last_entry) {
            JXTA_OBJECT_RELEASE(last_entry);
            last_entry = JXTA_OBJECT_SHARE(entry);
        }

        JXTA_OBJECT_RELEASE(entry);
    }
    return status;
}

static void histogram_get_empty_space(Jxta_vector * histogram, BIGNUM *min_bn, BIGNUM *max_bn, Jxta_vector ** empty_locations)
{
    Jxta_status status;
    Peerview_histogram_entry *last_entry=NULL;
    Peerview_histogram_entry *free_space=NULL;
    int i;

    *empty_locations = jxta_vector_new(0);
    for (i=0; i < jxta_vector_size(histogram); i++) {
        Peerview_histogram_entry *entry=NULL;

        status = jxta_vector_get_object_at(histogram, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve entry from the histogram res:%d\n", status);
            continue;
        }
        if (NULL != last_entry) {
            BIGNUM *diff = NULL;
            diff = BN_new();
            BN_sub(diff, entry->start, last_entry->end);
            if (BN_cmp(diff, BN_value_one()) > 0) {
                free_space = peerview_histogram_entry_new(last_entry->end, entry->start);
            }
            JXTA_OBJECT_RELEASE(last_entry);
            last_entry = entry;
            if (diff)
                BN_free(diff);
        } else {
            if (BN_cmp(entry->start, min_bn) > 0) {
                free_space = peerview_histogram_entry_new(min_bn, entry->start);
            }
        }
        if (NULL != free_space) {
            jxta_vector_add_object_last(*empty_locations, (Jxta_object *) free_space);
            JXTA_OBJECT_RELEASE(free_space);
            free_space = NULL;
        }
        last_entry = entry;
    }
    if (last_entry) {
        if (BN_cmp(last_entry->end, max_bn) < 0) {
            free_space = peerview_histogram_entry_new(last_entry->end, max_bn);
            jxta_vector_add_object_last(*empty_locations, (Jxta_object *) free_space);
            JXTA_OBJECT_RELEASE(free_space);
        }
        JXTA_OBJECT_RELEASE(last_entry);
    }
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
            if (remain_in_peerview(me, me->cluster_members) == FALSE) {
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
            peerview_has_changed(me);
            terminate_maintain_thread();
            apr_thread_mutex_unlock(me->mutex);
        } else if (config_rendezvous == new_config) {
            initiate_maintain_thread(me);
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

static Jxta_status JXTA_STDCALL peerview_monitor_cb(Jxta_object * obj, Jxta_monitor_entry **arg)
{
    Jxta_peerview * pv;

    Jxta_status res = JXTA_SUCCESS;
    Jxta_monitor_entry *entry;
    Jxta_monitor_service *monitor;
    Jxta_peerview_monitor_entry * pv_entry;
    Jxta_peerview_pong_msg * pong;
    JString * rdv_id_j;

    pv = (Jxta_peerview *) obj;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Great !!!!!!!!!!! Working.\n");

    monitor = jxta_monitor_service_get_instance();

    if (NULL == monitor) return JXTA_ITEM_NOTFOUND;

    pv_entry = jxta_peerview_monitor_entry_new();

    jxta_peerview_monitor_entry_set_rdv_time(pv_entry, 3000);
    jxta_peerview_monitor_entry_set_cluster_number(pv_entry, jxta_peerview_get_cluster_number(pv));

    apr_thread_mutex_lock(pv->mutex);

    if ((NULL != pv->instance_mask) && (NULL != pv->self_pve)) {
        res = peerview_create_pong_msg(pv, PONG_STATUS, FALSE, &pong, FALSE);
        jxta_peerview_monitor_entry_set_pong_msg(pv_entry, pong);
        JXTA_OBJECT_RELEASE(pong);
    }
    apr_thread_mutex_unlock(pv->mutex);

    entry = jxta_monitor_entry_new();

    jxta_monitor_entry_set_entry(entry, (Jxta_advertisement *) pv_entry);

    jxta_id_to_jstring(jxta_rendezvous_classid, &rdv_id_j);

    jxta_monitor_entry_set_context(entry, pv->groupUniqueID);
    jxta_monitor_entry_set_sub_context(entry, jstring_get_string(rdv_id_j));
    jxta_monitor_entry_set_type(entry, "jxta:PV3MonEntry");

    *arg = JXTA_OBJECT_SHARE(entry);

    JXTA_OBJECT_RELEASE(rdv_id_j);
    JXTA_OBJECT_RELEASE(entry);
    JXTA_OBJECT_RELEASE(pv_entry);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_client_pong_msg(Jxta_object * obj, Jxta_peerview_pong_msg ** pong_msg)
{
    Jxta_peerview * pv = (Jxta_peerview*) obj;
    Jxta_status res = JXTA_SUCCESS;

    Jxta_peerview_pong_msg * pong;
    res = peerview_create_pong_msg(pv, PONG_STATUS, TRUE, &pong, FALSE);
    if(res != JXTA_SUCCESS)
        return res;
    else
        *pong_msg = JXTA_OBJECT_SHARE(pong);

    return JXTA_SUCCESS;
}



/* vim: set ts=4 sw=4 et tw=130: */
