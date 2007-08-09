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
 * $Id: jxta_peerview.c,v 1.45.4.8 2007/05/24 16:24:24 exocetrick Exp $
 */

static const char *__log_cat = "PV";

#include <stddef.h>
#include <assert.h>

#include <openssl/bn.h>
#include <openssl/sha.h>

#include "jxta_apr.h"

#include "jxta_peerview_priv.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_hashtable.h"
#include "jxta_rdv_service.h"
#include "jxta_svc.h"

#include "jxta_peerview_address_request_msg.h"
#include "jxta_peerview_address_assign_msg.h"
#include "jxta_peerview_ping_msg.h"
#include "jxta_peerview_pong_msg.h"

#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"

/**
*   Names of the Peerview Events
**/
const char *JXTA_PEERVIEW_EVENT_NAMES[] = { "unknown", "ADD", "REMOVE" };

const char JXTA_PEERVIEW_NAME[] = "PeerView";
const char JXTA_PEERVIEW_NS_NAME[] = "jxta";

#define PA_REFRESH_INTERVAL (5 * 60 * JPR_INTERVAL_ONE_SECOND)

/**
*   Interval at which we re-evaluate all of the peers within the peerview.
**/
static const apr_interval_time_t JXTA_PEERVIEW_MAINTAIN_INTERVAL = 30 * APR_USEC_PER_SEC;

/**
*   Interval at which we (may) attempt to nominate additional peers to join the peerview.
**/
static const apr_interval_time_t JXTA_PEERVIEW_ADD_INTERVAL = 30 * APR_USEC_PER_SEC;

#define JXTA_PEERVIEW_PVE_EXPIRATION (20 * 60 * JPR_INTERVAL_ONE_SECOND)
#define JXTA_PEERVIEW_PVE_PING_DUE (JXTA_PEERVIEW_PVE_EXPIRATION / 4)
#define JXTA_PEERVIEW_PVE_PONG_DUE (JXTA_PEERVIEW_PVE_EXPIRATION / 8)

static const unsigned int DEFAULT_CLUSTERS_COUNT = 2;

static const unsigned int DEFAULT_CLUSTER_MEMBERS = 4;

static const unsigned int DEFAULT_REPLICATION_FACTOR = 2;

/**
*   The number of locate probes we will send before giving up
*   and starting a new peerview instance.
*/
static const unsigned int DEFAULT_MAX_LOCATE_PROBES = 3;

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

    Jxta_time created_at;

    apr_uuid_t adv_gen;
    Jxta_time_diff adv_exp;
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

    char *assigned_id_str;

    /* Configuration */
    Jxta_RdvConfigAdvertisement *rdvConfig;
    Jxta_time_diff pa_refresh;
    unsigned int clusters_count;
    unsigned int cluster_members;
    unsigned int replicas_count;

    Jxta_boolean active;

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
    unsigned int activity_add_candidate_pongs_sent;

};


typedef struct Peerview_histogram_entry Peerview_histogram_entry;

struct Peerview_histogram_entry {
    JXTA_OBJECT_HANDLE;

    BIGNUM *start;

    BIGNUM *end;

    Jxta_vector *peers;
};

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


static Peerview_entry *peerview_entry_new(Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv);
static Peerview_entry *peerview_entry_construct(Peerview_entry * myself, Jxta_id * pid, Jxta_endpoint_address * ea,
                                                Jxta_PA * adv);
static void peerview_entry_delete(Jxta_object * addr);
static void peerview_entry_destruct(Peerview_entry * myself);
static Jxta_status create_self_pve(Jxta_peerview * myself, BIGNUM * address);

static Peerview_entry *peerview_get_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_boolean peerview_check_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_status peerview_add_pve(Jxta_peerview * myself, Peerview_entry * pve);
static Jxta_status peerview_remove_pve(Jxta_peerview * myself, Jxta_PID * pid);
static Jxta_vector *peerview_get_all_pves(Jxta_peerview * myself);
static Jxta_status peerview_clear_pves(Jxta_peerview * myself);

static Jxta_peerview_event *peerview_event_new(Jxta_Peerview_event_type event, Jxta_id * id);
static void peerview_event_delete(Jxta_object * ptr);

static Jxta_status peerview_init_cluster(Jxta_peerview * myself);

static Jxta_status JXTA_STDCALL peerview_protocol_cb(Jxta_object * obj, void *arg);
static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req);
static Jxta_status peerview_handle_address_assign(Jxta_peerview * myself, Jxta_peerview_address_assign_msg * addr_assign);
static Jxta_status peerview_handle_ping(Jxta_peerview * myself, Jxta_peerview_ping_msg * ping);
static Jxta_status peerview_handle_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong);

static Jxta_status peerview_send_address_request(Jxta_peerview * myself, Jxta_peer * dest);
static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen);
static Jxta_status peerview_send_address_assign(Jxta_peerview * myself, Jxta_peer * dest, BIGNUM * target_hash);
static Jxta_status peerview_send_pvm(Jxta_peerview * myself, Jxta_peer * dest, Jxta_message * msg);

static void call_event_listeners(Jxta_peerview * pv, Jxta_Peerview_event_type event, Jxta_id * id);

static unsigned int cluster_for_hash(Jxta_peerview * myself, BIGNUM * target_hash);

static Jxta_status build_histogram(Jxta_vector ** histo, Jxta_vector * peers, BIGNUM * minimum, BIGNUM * maximum);

static void *APR_THREAD_FUNC activity_peerview_locate(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_addressing(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_announce(apr_thread_t * thread, void *arg);
static void *APR_THREAD_FUNC activity_peerview_maintain(apr_thread_t * thread, void *cookie);
static void *APR_THREAD_FUNC activity_peerview_add(apr_thread_t * thread, void *cookie);

static Peerview_entry *peerview_entry_new(Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv)
{
    Peerview_entry *myself;

    myself = (Peerview_entry *) calloc(1, sizeof(Peerview_entry));

    if (NULL == myself) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, peerview_entry_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Creating PVE [%pp]\n", myself);

    return peerview_entry_construct(myself, pid, ea, adv);
}

static Peerview_entry *peerview_entry_construct(Peerview_entry * myself, Jxta_id * pid, Jxta_endpoint_address * ea, Jxta_PA * adv)
{
    myself = (Peerview_entry *) peer_entry_construct((_jxta_peer_entry *) myself);

    if (NULL != myself) {
        myself->thisType = "Peerview_entry";

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
        }

        myself->target_hash = BN_new();

        myself->target_hash_radius = BN_new();

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
        myself->clusters_count = DEFAULT_CLUSTERS_COUNT;
        myself->cluster_members = DEFAULT_CLUSTER_MEMBERS;
        myself->replicas_count = DEFAULT_REPLICATION_FACTOR;

        myself->endpoint = NULL;
        myself->discovery = NULL;
        myself->rdv = NULL;

        myself->pves = jxta_hashtable_new(DEFAULT_CLUSTERS_COUNT * DEFAULT_CLUSTER_MEMBERS * 2);
        myself->my_cluster = INT_MAX;
        myself->clusters = NULL;
        myself->self_pve = NULL;

        myself->event_listener_table = jxta_hashtable_new(1);

        myself->activity_maintain_referral_peers = jxta_vector_new(0);
        myself->activity_maintain_referral_pings_sent = 0;

        myself->activity_add_candidate_peers = jxta_vector_new(0);
        myself->activity_add_candidate_pongs_sent = 0;

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

    if (NULL != myself->group) {
        JXTA_OBJECT_RELEASE(myself->group);
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

    if (NULL !=  myself->activity_maintain_referral_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_maintain_referral_peers);
    }

    if (NULL != myself->activity_add_candidate_peers) {
        JXTA_OBJECT_RELEASE(myself->activity_add_candidate_peers);
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
    Jxta_PA *conf_adv = NULL;
    Jxta_PG *parentgroup = NULL;
    JString *string;

    jxta_id_to_jstring(assigned_id, &string);

    myself->assigned_id_str = strdup(jstring_get_string(string));

    if (NULL == myself->assigned_id_str) {
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_RELEASE(string);

    myself->group = JXTA_OBJECT_SHARE(group);

    myself->thread_pool = jxta_PG_thread_pool_get(group);
    jxta_PG_get_parentgroup(myself->group, &parentgroup);
    jxta_PG_get_PID(group, &myself->pid);
    jxta_PG_get_GID(group, &myself->gid);
    jxta_id_get_uniqueportion(myself->gid, &string);
    myself->groupUniqueID = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    /* Determine our configuration */
    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != parentgroup) {
        jxta_PG_get_configadv(parentgroup, &conf_adv);
    }

    if (conf_adv != NULL) {
        Jxta_svc *svc;
        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);
        if (NULL != svc) {
            myself->rdvConfig = jxta_svc_get_RdvConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == myself->rdvConfig) {
        myself->rdvConfig = jxta_RdvConfigAdvertisement_new();
    }

    res = peerview_init_cluster(myself);

    if (NULL != parentgroup) {
        JXTA_OBJECT_RELEASE(parentgroup);
    }

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
    BN_lshift(hash_space, BN_value_one(), (CHAR_BIT * SHA_DIGEST_LENGTH));
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
    }

    BN_CTX_free(ctx);

    return JXTA_SUCCESS;
}

Jxta_status peerview_start(Jxta_peerview * pv)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    apr_thread_mutex_lock(myself->mutex);

    /* Register Endpoint Listener for Peerview protocol. */
    jxta_PG_get_endpoint_service(myself->group, &(myself->endpoint));
    jxta_PG_add_recipient(myself->group, &myself->ep_cookie, RDV_V3_MSID, JXTA_PEERVIEW_NAME, peerview_protocol_cb, myself);

    jxta_PG_get_discovery_service(myself->group, &(myself->discovery));

    myself->state = PV_PASSIVE;
    myself->running = TRUE;

    apr_thread_mutex_unlock(myself->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview started for group %s / %s \n", myself->groupUniqueID,
                    myself->assigned_id_str);

    return res;
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
    myself->self_pve = peerview_entry_new(myself->pid, NULL, pa);
    /* FIXME: need to get a real adv_gen from PA */
    apr_uuid_get(&myself->self_pve->adv_gen);
    JXTA_OBJECT_RELEASE(pa);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing Self PVE : %x\n", myself->self_pve);

    jxta_peer_set_expires((Jxta_peer *) myself->self_pve, JPR_ABSOLUTE_TIME_MAX);

    tmp = BN_bn2hex(address);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Self Target Hash : %s\n", tmp);
    free(tmp);

    BN_copy(myself->self_pve->target_hash, address);

    myself->my_cluster = cluster_for_hash(myself, myself->self_pve->target_hash);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Self Cluster : %d\n", myself->my_cluster);

    BN_copy(myself->self_pve->target_hash_radius, myself->peer_address_space);
    BN_rshift1(myself->self_pve->target_hash_radius, myself->self_pve->target_hash_radius);

    res = peerview_add_pve(myself, myself->self_pve);

    return res;
}

Jxta_status peerview_stop(Jxta_peerview * pv)
{
    Jxta_peerview *myself = PTValid(pv, Jxta_peerview);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopping peerview\n");

    apr_thread_mutex_lock(myself->mutex);

    /* FIXME bondolo Announce that we are shutting down */

    peerview_clear_pves(myself);

    myself->running = FALSE;

    apr_thread_pool_tasks_cancel(myself->thread_pool, myself);
    
    jxta_vector_clear(myself->activity_maintain_referral_peers);
    jxta_vector_clear(myself->activity_add_candidate_peers);

    jxta_PG_remove_recipient(myself->group, myself->ep_cookie);
    myself->state = PV_STOPPED;

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

JXTA_DECLARE(unsigned int) jxta_peerview_get_cluster_number(Jxta_peerview * me)
{
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);

    return myself->my_cluster;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_gen_hash(Jxta_peerview * me, unsigned char const *value, size_t length, BIGNUM ** hash)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
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
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Address Space : %s\n", tmp);
    free(tmp);

    tmp = BN_bn2hex(target_hash);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "target_hash : %s\n", tmp);
    free(tmp);

    BN_div(bn_target_cluster, NULL, target_hash, myself->cluster_divisor, ctx);
    target_cluster = (unsigned int) BN_get_word(bn_target_cluster);
    BN_free(bn_target_cluster);
    BN_CTX_free(ctx);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "target_cluster : %d\n", target_cluster);

    return target_cluster;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_peer_for_target_hash(Jxta_peerview * me, BIGNUM * target_hash, Jxta_peer ** peer)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview *myself = PTValid(me, Jxta_peerview);
    BIGNUM *bn_target_hash = (BIGNUM *) target_hash;

    unsigned int target_cluster = cluster_for_hash(myself, bn_target_hash);

    apr_thread_mutex_lock(myself->mutex);

    if (NULL != myself->self_pve) {
        if (myself->my_cluster != target_cluster) {
            res = jxta_peerview_get_associate_peer(myself, target_cluster, peer);
        } else {
            /* The target is in our cluster. We refer them to the peer whose target hash most closely matches. */
            unsigned int all_peers;
            unsigned int each_peer;
            BIGNUM *effective_target_hash = BN_new();
            BIGNUM *distance = BN_new();
            BIGNUM *best_distance = BN_new();


            *peer = JXTA_OBJECT_SHARE(myself->self_pve);
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
                        JXTA_OBJECT_RELEASE(*peer);
                        *peer = JXTA_OBJECT_SHARE(candidate);

                        BN_copy(best_distance, distance);
                    }
                }
                JXTA_OBJECT_RELEASE(candidate);
            }

            BN_free(distance);
            BN_free(best_distance);
            BN_free(effective_target_hash);

            res = JXTA_SUCCESS;
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


static void call_event_listeners(Jxta_peerview * myself, Jxta_Peerview_event_type event, Jxta_id * id)
{
    Jxta_peerview_event *pv_event = peerview_event_new(event, id);
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *lis = NULL;

    if (NULL == pv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not create peerview event object.\n");
        return;
    }

    apr_thread_mutex_lock(myself->mutex);
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
    JXTA_OBJECT_RELEASE(pa);
    jxta_peerview_address_request_msg_set_peer_adv_gen(addr_req, &adv_gen);
    jxta_peerview_address_request_msg_set_peer_adv_exp(addr_req, 20 * 60 * JPR_INTERVAL_ONE_SECOND);

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

static Jxta_status peerview_send_ping(Jxta_peerview * myself, Jxta_peer * dest, apr_uuid_t * adv_gen)
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

static Jxta_status peerview_send_pong(Jxta_peerview * myself, Jxta_peer * dest)
{
    Jxta_status res;
    Jxta_peerview_pong_msg *pong = NULL;
    JString *pong_msg_xml;
    Jxta_message *msg;
    Jxta_message_element *el = NULL;
    char *tmp;
    unsigned int i;
    Jxta_vector *peers;
    apr_uuid_t adv_gen;

    apr_thread_mutex_lock(myself->mutex);

    if ((NULL == myself->instance_mask) || (NULL == myself->self_pve)
        || (0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Can't send pong message now! [%pp]\n", myself);
        res = JXTA_FAILED;
        apr_thread_mutex_unlock(myself->mutex);
        return res;
    }

    pong = jxta_peerview_pong_msg_new();

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

    jxta_peerview_pong_msg_set_peer_adv(pong, jxta_peer_adv((Jxta_peer *) myself->self_pve));

    /* TODO 20060922 bondolo Use a real advertisement generation */
    apr_uuid_get(&adv_gen);
    jxta_peerview_pong_msg_set_peer_adv_gen(pong, &adv_gen);

    jxta_peerview_pong_msg_set_peer_adv_exp(pong, JXTA_PEERVIEW_PVE_EXPIRATION);
    res = jxta_peerview_get_localview(myself, &peers);

    if (JXTA_SUCCESS == res) {

        for (i = 0; i < jxta_vector_size(peers); ++i) {
            Jxta_id * peerId=NULL;
            Jxta_peer * peer=NULL;
            res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (JXTA_SUCCESS != res) continue;

            jxta_peer_get_peerid(peer, &peerId);
            if (!jxta_id_equals(peerId, myself->pid)) {
                Peerview_entry *pve = NULL;
                pve = peerview_get_pve(myself, peerId);
                if (NULL != pve) {
                    char *target_hash = NULL;
                    char *target_hash_radius = NULL;
                    target_hash = BN_bn2hex(pve->target_hash);
                    target_hash_radius = BN_bn2hex(pve->target_hash_radius);
                    jxta_pong_msg_add_partner_info(pong, peerId, jxta_peer_adv((Jxta_peer *) pve), target_hash, target_hash_radius );
                    free(target_hash);
                    free(target_hash_radius);
                }
            }
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(peerId);
        }
        JXTA_OBJECT_RELEASE(peers);
    }
    res = jxta_peerview_get_globalview(myself, &peers);

    if (JXTA_SUCCESS == res) {

        for (i = 0; i < jxta_vector_size(peers); ++i) {
            Jxta_id * peerId=NULL;
            Jxta_peer * peer=NULL;
            res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (JXTA_SUCCESS != res) continue;
            jxta_peer_get_peerid(peer, &peerId);
            if (!jxta_id_equals(peerId, myself->pid)) {
                Peerview_entry *pve = NULL;
                pve = peerview_get_pve(myself, peerId);
                if (NULL != pve) {
                    char *target_hash = NULL;
                    char *target_hash_radius = NULL;
                    target_hash = BN_bn2hex(pve->target_hash);
                    target_hash_radius = BN_bn2hex(pve->target_hash_radius);
                    jxta_pong_msg_add_associate_info(pong, peerId, jxta_peer_adv((Jxta_peer *) pve), target_hash, target_hash_radius );
                    free(target_hash);
                    free(target_hash_radius);
                }
            }
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(peerId);
        }
        JXTA_OBJECT_RELEASE(peers);
    }

    apr_thread_mutex_unlock(myself->mutex);

    res = jxta_peerview_pong_msg_get_xml(pong, &pong_msg_xml);
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
    Jxta_PA *pa;
    Jxta_peerview_address_assign_msg *addr_assign;
    JString *addr_assign_xml = NULL;
    char *temp;

    jxta_PG_get_PA(myself->group, &pa);

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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending peerview message with peerid [%pp] to : %s\n", msg,
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
    enum { ADDR_REQ, ADDR_ASSIGN, PING, PONG } message_type;
    Jxta_bytevector *value;
    JString *string;

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
    }

    JXTA_OBJECT_RELEASE(string);

  FINAL_EXIT:
    if (NULL != el) {
        JXTA_OBJECT_RELEASE(el);
    }

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed processing peerview message [%pp] %d\n", msg, res);
    }

    return res;
}

static Jxta_status peerview_handle_address_request(Jxta_peerview * myself, Jxta_peerview_address_request_msg * addr_req)
{
    Jxta_status res = JXTA_SUCCESS;
    volatile Jxta_boolean locked;
    unsigned int a_cluster = UINT_MAX;
    BIGNUM *target_address;
    Jxta_PA *dest_pa;
    Jxta_id *dest_id;
    Jxta_peer *dest;
    Jxta_boolean found_empty = FALSE;


    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    /* Make sure we are an active member of a peerview. */
    if ((NULL == myself->self_pve) || (0 == jxta_peer_get_expires((Jxta_peer *) myself->self_pve))) {
        /* We are either inactive or moving to a new instance. */
         goto FINAL_EXIT;
    }

    /* XXX 20060925 bondolo Possbily use existing target hash if present */

    /* Inverse binary search. We look as far away from our cluster as possible */

    if (myself->clusters_count > 1) {
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

    target_address = BN_new();

    if (found_empty) {
        /* We need to choose an address for the specified cluster */

        BN_pseudo_rand_range(target_address, myself->cluster_divisor);
        BN_add(target_address, target_address, myself->clusters[a_cluster].min);
    } else {
        /* We need to choose an address within our cluster */

        /* XXX 20060925 bondolo Do something stociastic with histogram. */
        BN_pseudo_rand_range(target_address, myself->cluster_divisor);
        BN_add(target_address, target_address, myself->clusters[myself->my_cluster].min);
    }

    dest = jxta_peer_new();

    dest_pa = jxta_peerview_address_request_msg_get_peer_adv(addr_req);
    jxta_peer_set_adv(dest, dest_pa);
    dest_id = jxta_PA_get_PID(dest_pa);
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
    Jxta_id *assigner_id = NULL;
    Peerview_entry *assigner = NULL;

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

    BN_hex2bn(&response_instance_mask, jxta_peerview_address_assign_msg_get_instance_mask(addr_assign));

    same = 0 == BN_cmp(myself->instance_mask, response_instance_mask);
    BN_free(response_instance_mask);

    if (!same) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Address assignement from unexpected peerview instance. [%pp]\n",
                        myself);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
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

    if (NULL != assigner_id) {
        JXTA_OBJECT_RELEASE(assigner_id);
    }

    if (NULL != assigner) {
        JXTA_OBJECT_RELEASE(assigner);
    }

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
    Jxta_id *pid;
    Jxta_peer *dest;
    Jxta_endpoint_address *dst_ea;

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

    dest = jxta_peer_new();
    jxta_peer_set_peerid(dest, pid);

    res = peerview_send_pong(myself, dest);

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
        assert(PV_LOCATING == me->state);
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
        if (same < 0) {
            /* Signify that we are giving up on this peerview instance in favour of the new one. */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "PV[%pp] Found peerview alternate instance : %s\n", me, pv_mask);

            BN_copy(me->instance_mask, instance_mask);
            peerview_clear_pves(me);
            jxta_peer_set_expires((Jxta_peer *) me->self_pve, 0L);
            me->state = PV_LOCATING;
            rv = TRUE;

        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "PV[%pp] Nonmatching instance mask found : %s\n", me, pv_mask);
            rv = TRUE;
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
    Peerview_entry * pve;
    Jxta_status res;

    adv_gen = jxta_peerview_pong_msg_get_peer_adv_gen(pong);
    if (!adv_gen) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PV[%pp] Invalid PONG message, adv_gen requiredi for adv.\n", me);
        *ppve = NULL;
        return JXTA_INVALID_ARGUMENT;
    }

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

    pve = peerview_entry_new(pid, NULL, pa);

    /* Ping the new entry in maintenance task. */
    jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + JXTA_PEERVIEW_PVE_PING_DUE);

    BN_hex2bn(&pve->target_hash, jxta_peerview_pong_msg_get_target_hash(pong));
    BN_hex2bn(&pve->target_hash_radius, jxta_peerview_pong_msg_get_target_hash_radius(pong));

    pve->adv_exp = jxta_peerview_pong_msg_get_peer_adv_exp(pong);
    if (-1L == pve->adv_exp) {
        pve->adv_exp = JXTA_PEERVIEW_PVE_EXPIRATION;
    }

    memmove(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t));
    free(adv_gen);
    jxta_peer_set_adv((Jxta_peer *) pve, pa);
    JXTA_OBJECT_RELEASE(pa);
    JXTA_OBJECT_RELEASE(pid);

    *ppve = pve;
    return JXTA_SUCCESS;
}

/* Process the associates and partners looking for unknown peers. */
static Jxta_status process_referrals(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_vector *referrals;
    Jxta_vector *associate_referrals;
    Jxta_vector *partner_referrals;
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

        res = jxta_vector_get_object_at(referrals, JXTA_OBJECT_PPTR(&peer), each_referral);
        if (JXTA_SUCCESS != res) {
            continue;
        }

        referral_pid = jxta_peerview_peer_info_get_peer_id(peer);

        if (!peerview_check_pve(me, referral_pid)) {
            /* Not in pve, see if we have a referral already. */
            Jxta_peer *ping_target = jxta_peer_new();
            Jxta_PA *peer_adv = jxta_peerview_peer_info_get_peer_adv(peer);
            jxta_peer_set_peerid(ping_target, referral_pid);
            jxta_peer_set_adv(ping_target, peer_adv);
            if (!jxta_vector_contains (me->activity_maintain_referral_peers, (Jxta_object *) ping_target,
                                       (Jxta_object_equals_func) jxta_peer_equals)) {
                JString *jPeerid;
                jxta_id_to_jstring(referral_pid, &jPeerid); 
                jxta_vector_add_object_last(me->activity_maintain_referral_peers, (Jxta_object *) ping_target);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,"Added referral: %s\n", jstring_get_string(jPeerid));
                JXTA_OBJECT_RELEASE(jPeerid);

            }
            JXTA_OBJECT_RELEASE(ping_target);
            JXTA_OBJECT_RELEASE(peer_adv);
        }

        JXTA_OBJECT_RELEASE(referral_pid);
        JXTA_OBJECT_RELEASE(peer);
    }

    JXTA_OBJECT_RELEASE(referrals);
    return JXTA_SUCCESS;
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

    return JXTA_SUCCESS;
}

static Jxta_status handle_existing_pve_pong(Jxta_peerview * me, Peerview_entry * pve, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_PA *pa = NULL;
    apr_uuid_t *adv_gen;

    /* XXX bondolo We could check that the target hash has not changed. */
    
    /* Update radius unconditionally */
    BN_hex2bn(&pve->target_hash_radius, jxta_peerview_pong_msg_get_target_hash_radius(pong));
    jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);

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

    if (0 != memcmp(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t))) {
        /* Update the advertisement. */
        pve->adv_exp = jxta_peerview_pong_msg_get_peer_adv_exp(pong);

        if (-1L == pve->adv_exp) {
            pve->adv_exp = JXTA_PEERVIEW_PVE_EXPIRATION;
        }

        if ((NULL != me->self_pve) && (0 != jxta_peer_get_expires((Jxta_peer *) me->self_pve))) {
            /*FIXME: sould not do so if pve is new, not yet in our pve */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "sould not do so if pve is new, not yet in our pve\n");
            jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + pve->adv_exp);
        } else {
            /* We aren't in the peerview. Greatly foreshorten the expiration so that a pong msg will be sent by
               maintenance task. */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "We aren't in the peerview. Greatly foreshorten the expiration\n");
            jxta_peer_set_expires((Jxta_peer *) pve, jpr_time_now() + JXTA_PEERVIEW_PVE_PONG_DUE);
        }

        memmove(&pve->adv_gen, adv_gen, sizeof(apr_uuid_t));

        jxta_peer_set_adv((Jxta_peer *) pve, pa);
    }
    free(adv_gen);

    return res;
}

static Jxta_status peerview_handle_pong(Jxta_peerview * me, Jxta_peerview_pong_msg * pong)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_id *pid = NULL;
    Peerview_entry *pve = NULL;

    /* TODO 20060926 bondolo Valdiate credential on pong message */

    pid = jxta_peerview_pong_msg_get_peer_id(pong);
    assert(pid);

    if (jxta_id_equals(me->pid, pid)) {
        JXTA_OBJECT_RELEASE(pid);
        goto FINAL_EXIT;
    }

    apr_thread_mutex_lock(me->mutex);

    if (me->instance_mask) {
        assert(PV_MAINTENANCE == me->state || PV_ADDRESSING == me->state || PV_ANNOUNCING == me->state);
        if (is_for_alternative(me, jxta_peerview_pong_msg_get_instance_mask(pong))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PONG [%pp] is for alternative\n", pong);
            if (PV_LOCATING == me->state) {
                res = joining_peerview(me, jxta_peerview_pong_msg_get_instance_mask(pong));
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PONG [%pp] is joining alternate peerview\n");
                assert(PV_ADDRESSING == me->state);
            }
            else {
                /* ignore - we're not in that peerview */
                goto UNLOCK_EXIT;
            }
        }
    } else {
        assert(PV_LOCATING == me->state || PV_PASSIVE == me->state);
        res = joining_peerview(me, jxta_peerview_pong_msg_get_instance_mask(pong));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PONG [%pp] is joining peerview\n");
        assert(PV_ADDRESSING == me->state);
    }

    pve = peerview_get_pve(me, pid);
 
    if (NULL == pve) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PONG [%pp] is being handled as new\n", pong);
        res = handle_new_pve_pong(me, pong);
        if (JXTA_SUCCESS == res && me->state == PV_MAINTENANCE) {
            Jxta_peer * newPeer = jxta_peer_new();
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "PONG announcement\n");
            jxta_peer_set_peerid(newPeer, pid);
            peerview_send_pong(me, newPeer);
            JXTA_OBJECT_RELEASE(newPeer);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "PONG [%pp] is being handled as existing pong\n", pong);
        res = handle_existing_pve_pong(me, pve, pong);
    }

    if (JXTA_SUCCESS == res) {
        process_referrals(me, pong);
    }
UNLOCK_EXIT:
    apr_thread_mutex_unlock(me->mutex);
FINAL_EXIT:
    if (pve)
        JXTA_OBJECT_RELEASE(pve);
    if (pid)
        JXTA_OBJECT_RELEASE(pid);
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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding new PVE [%pp] for %s\n", pve, jstring_get_string(pid_str));

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
    apr_thread_mutex_unlock(myself->mutex);

    /*
     * Notify the peerview listeners that we added a rdv peer from our local rpv 
     */
    if (!found) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Added PVE for %s [%pp] to cluster %d\n", jstring_get_string(pid_str),
                        pve, pve_cluster);
        call_event_listeners(myself, JXTA_PEERVIEW_ADD, jxta_peer_peerid((Jxta_peer *) pve));
    }

    JXTA_OBJECT_RELEASE(pid_str);

    res = found ? JXTA_BUSY : JXTA_SUCCESS;

    return res;
}

static Jxta_status peerview_remove_pve(Jxta_peerview * myself, Jxta_PID * pid)
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
    apr_thread_mutex_unlock(myself->mutex);

    /*
     * Notify the peerview listeners that we removed a rdv peer from our local rpv 
     */
    if (found) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removed PVE for %s\n", jstring_get_string(pid_str));
        call_event_listeners(myself, JXTA_PEERVIEW_REMOVE, pid);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to remove PVE for %s. Not Found.\n",
                        jstring_get_string(pid_str));
    }

    JXTA_OBJECT_RELEASE(pid_str);

    res = found ? JXTA_SUCCESS : JXTA_ITEM_NOTFOUND;

    return res;
}

static Jxta_vector *peerview_get_all_pves(Jxta_peerview * myself)
{

    return jxta_hashtable_values_get(myself->pves);
}

static Jxta_status peerview_clear_pves(Jxta_peerview * myself)
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

    apr_thread_mutex_unlock(myself->mutex);

    /*
     * Notify the peerview listeners that we removed the rdv peers from our local rpv.
     */
    all_pves = jxta_vector_size(current_pves);
    for (each_pve = 0; each_pve < all_pves; each_pve++) {
        Peerview_entry *a_pve;

        res = jxta_vector_remove_object_at(current_pves, JXTA_OBJECT_PPTR(&a_pve), 0);

        if (JXTA_SUCCESS == res) {
            call_event_listeners(myself, JXTA_PEERVIEW_REMOVE, jxta_peer_peerid((Jxta_peer *) a_pve));
            JXTA_OBJECT_RELEASE(a_pve);
        }
    }

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

    /* Get more seeds? */
    if ((NULL == me->activity_locate_seeds) || (0 == jxta_vector_size(me->activity_locate_seeds))) {
        if (NULL != me->activity_locate_seeds) {
            JXTA_OBJECT_RELEASE(me->activity_locate_seeds);
            me->activity_locate_seeds = NULL;
        }

        if (NULL == me->rdv) {
            jxta_PG_get_rendezvous_service(me->group, &(me->rdv));
        }

        if (NULL != me->rdv) {
            res = rdv_service_get_seeds(me->rdv, &me->activity_locate_seeds);

            /* Send a broadcast probe */
            res = rdv_service_send_seed_request(me->rdv);
        }

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
            res = peerview_send_ping(me, seed, NULL);
            sent_seed = TRUE;
        }

        JXTA_OBJECT_RELEASE(seed);
    }

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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[locate] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);

    if (PV_LOCATING != myself->state) {
        assert(NULL == myself->activity_locate_seeds);
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    if (myself->activity_locate_probes >= DEFAULT_MAX_LOCATE_PROBES) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "ACT[locate] [%pp]: Locate failed. Creating a new peerview instance.\n",
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
        res = probe_a_seed(myself);
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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[addressing] [%pp]: Running. \n", myself);

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
        if (address_requests_sent < DEFAULT_MAX_ADDRESS_REQUESTS) {
            res = peerview_send_address_request(myself, (Jxta_peer *) a_pve);
            address_requests_sent++;
        }

        JXTA_OBJECT_RELEASE(a_pve);
    }

    JXTA_OBJECT_RELEASE(pves);
    
    /* Reschedule another check. */
    apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_addressing, myself, 
                                                    JXTA_PEERVIEW_MAINTAIN_INTERVAL, myself);

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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[announce] [%pp]: Running. \n", myself);

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

        res = peerview_send_pong(myself, (Jxta_peer *) a_pve);
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
    /* Send ping messages to any referrals from pong responses. */

    jxta_vector_shuffle(me->activity_maintain_referral_peers);  /* We use this for probes so shuffle it. */

    while ((me->activity_maintain_referral_pings_sent < DEFAULT_MAX_PING_PROBES)
           && (me->activity_maintain_referral_pings_sent < jxta_vector_size(me->activity_maintain_referral_peers))) {

        res = jxta_vector_get_object_at(me->activity_maintain_referral_peers, JXTA_OBJECT_PPTR(&ping_peer),
                                      me->activity_maintain_referral_pings_sent);

        if (JXTA_SUCCESS == res) {
            res = peerview_send_ping(me, ping_peer, NULL);
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
            /* It's dead Jim. Remove it from the peerview. */

            peerview_remove_pve(me, jxta_peer_peerid((Jxta_peer *) a_pve));
            JXTA_OBJECT_RELEASE(a_pve);
            continue;
        }
        if ((expires - now) <= JXTA_PEERVIEW_PVE_PING_DUE) {
            /* Ping for refresh */
            res = peerview_send_ping(me, (Jxta_peer *) a_pve, &a_pve->adv_gen);
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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: Running. \n", myself);
    apr_thread_mutex_lock(myself->mutex);

    if (PV_ADDRESSING != myself->state && PV_MAINTENANCE != myself->state) {
        apr_thread_mutex_unlock(myself->mutex);
        return NULL;
    }

    current_pves = peerview_get_all_pves(myself);

    apr_thread_mutex_unlock(myself->mutex);

    check_pves(myself, current_pves);
    JXTA_OBJECT_RELEASE(current_pves);

    probe_referrals(myself);

    if (PV_ADDRESSING != myself->state && PV_MAINTENANCE != myself->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: done. \n", myself);
        return NULL;
    }

    /* Reschedule another check. */
    apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_maintain, myself, 
                                                    JXTA_PEERVIEW_MAINTAIN_INTERVAL, myself);

    if (APR_SUCCESS != apr_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[maintain] [%pp]: Could not reschedule activity.\n",
                        myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: Rescheduled.\n", myself);
    }

    if (!myself->activity_add) {
        /* Start the add activity? */
        apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_add, myself, 
                                                        JXTA_PEERVIEW_ADD_INTERVAL, myself);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "ACT[maintain] [%pp]: Could not schedule add activity.\n", myself);
        } else {
            myself->activity_add = TRUE;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[maintain] [%pp]: Scheduled [Add].\n", myself);
        }
    }

    return NULL;
}

int client_entry_creation_time_compare(Jxta_peer * me, Jxta_peer * target);
Jxta_vector *client_entry_get_options(Jxta_peer * me);

/**
*   The add task is responsible for :
*       - adding additional peers to the peerview.
 **/
static void *APR_THREAD_FUNC activity_peerview_add(apr_thread_t * thread, void *cookie)
{
    Jxta_status res;
    Jxta_peerview *myself = PTValid(cookie, Jxta_peerview);
    Jxta_boolean locked = FALSE;
    unsigned int each_cluster;
    Jxta_boolean need_additional = FALSE;
    Jxta_vector *rdv_clients;
    unsigned int all_clients;
    unsigned int each_client;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] [%pp]: Running. \n", myself);

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    if (PV_STOPPED == myself->state) {
        apr_thread_mutex_unlock(myself->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] [%pp]: Done. \n", myself);
        return NULL;
    }

    /* Survey and determine if additional Peerview members are needed in other clusters */
    for (each_cluster = 0; each_cluster < myself->clusters_count; each_cluster++) {
        unsigned int member_count = jxta_vector_size(myself->clusters[each_cluster].members);
        if (0 == member_count) {
            need_additional = TRUE;
            break;
        }
    }

    if (!need_additional) {
        /* Survey and determine if additional Peerview members are needed in our cluster */
        if (jxta_vector_size(myself->clusters[myself->my_cluster].members) < DEFAULT_CLUSTER_MEMBERS) {
            need_additional = TRUE;
        }
    }

    if (!need_additional) {
        goto RERUN_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "ACT[add] [%pp]: Attempting to recruit peerview members.\n",
                    myself);


    /* Get some candidates from the RDV Server. We won't be running unless the peer is a rdv. 
     */
    jxta_vector_clear(myself->activity_add_candidate_peers);

    apr_thread_mutex_unlock(myself->mutex);
    locked = FALSE;

    res = jxta_rdv_service_get_peers(myself->rdv, &rdv_clients);

    if (JXTA_SUCCESS != res) {
        goto RERUN_EXIT;
    }

    all_clients = jxta_vector_size(rdv_clients);


    for (each_client = 0; each_client < all_clients; each_client++) {
        Jxta_peer *client;
        Jxta_vector *options;

        res = jxta_vector_remove_object_at(rdv_clients, JXTA_OBJECT_PPTR(&client), 0);

        if (JXTA_SUCCESS != res) {
            continue;
        }

        options = client_entry_get_options(client);

        /* XXX 20061023 bondolo Use options as part of ranking. */
        /* if (NULL != options) { */
            jxta_vector_add_object_last(myself->activity_add_candidate_peers, (Jxta_object *) client);
        /* } */

        if (NULL != options) {
            JXTA_OBJECT_RELEASE(options);
        }

        JXTA_OBJECT_RELEASE(client);
    }

    /* Sort by age. Use oldest client peers as candidates. */
    res =
        jxta_vector_qsort(myself->activity_add_candidate_peers, (Jxta_object_compare_func) client_entry_creation_time_compare);

    /* Now process the candidates */
    all_clients = jxta_vector_size(myself->activity_add_candidate_peers);
    for (each_client = 0; (each_client < all_clients) && (each_client < MAXIMUM_CLIENT_INVITATIONS); each_client++) {
        Jxta_peer *invitee;

        res = jxta_vector_remove_object_at(myself->activity_add_candidate_peers, JXTA_OBJECT_PPTR(&invitee), 0);

        if (JXTA_SUCCESS != res) {
            continue;
        }

        res = peerview_send_pong(myself, invitee);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ACT[add] [%pp]: Pong invite failed.\n", myself);
        } else {
            myself->activity_add_candidate_pongs_sent++;
        }

        JXTA_OBJECT_RELEASE(invitee);
    }

    JXTA_OBJECT_RELEASE(rdv_clients);
    probe_a_seed(myself);
  RERUN_EXIT:
    if (myself->running) {
        /* Reschedule another check. */
        apr_status_t apr_res = apr_thread_pool_schedule(myself->thread_pool, activity_peerview_add, myself, 
                                                        JXTA_PEERVIEW_ADD_INTERVAL, myself);

        if (APR_SUCCESS != apr_res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ACT[add] [%pp]: Could not reschedule activity.\n",
                            myself);
        } else {
            myself->activity_add = TRUE;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "ACT[add] [%pp]: Rescheduled.\n", myself);
        }
    }

    if (locked) {
        apr_thread_mutex_unlock(myself->mutex);
    }

    return NULL;
}

/* vim: set ts=4 sw=4 et tw=130: */
