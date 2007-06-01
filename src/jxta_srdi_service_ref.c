/*
 *  Copyright (c) 2001 Sun Microsystems, Inc.  All rights
 *  reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in
 *  the documentation and/or other materials provided with the
 *  distribution.
 *
 *  3. The end-user documentation included with the redistribution,
 *  if any, must include the following acknowledgment:
 *  "This product includes software developed by the
 *  Sun Microsystems, Inc. for Project JXTA."
 *  Alternately, this acknowledgment may appear in the software itself,
 *  if and wherever such third-party acknowledgments normally appear.
 *
 *  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must
 *  not be used to endorse or promote products derived from this
 *  software without prior written permission. For written
 *  permission, please contact Project JXTA at http://www.jxta.org.
 *
 *  5. Products derived from this software may not be called "JXTA",
 *  nor may "JXTA" appear in their name, without prior written
 *  permission of Sun.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED.  IN NO EVENT SHALL SUN MICROSYSTEMS OR
 *  ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *  ====================================================================
 *
 *  This software consists of voluntary contributions made by many
 *  individuals on behalf of Project JXTA.  For more
 *  information on Project JXTA, please see
 *  <http://www.jxta.org/>.
 *
 *  This license is based on the BSD license adopted by the Apache Foundation.
 *
*/

/**
 * Srdi is a service which provides Srdi functionalities such as :
 *
 * <ul>
 *  <li>pushing of Srdi messages to a another peer/propagate</li>
 *  <li>replication of an Srdi Message to other peers in a given peerview</li>
 *  <li>given an expression Srdi provides a independently calculated starting point</li>
 *  <li>Forwarding a ResolverQuery, and taking care of hopCount, random selection</li>
 *  <li>registers with the RendezvousService to determine when to share Srdi Entries</li>
 *    and whether to push deltas, or full a index</li>
 * </ul>
 *
 * <p/>If Srdi is started as a thread it performs periodic srdi pushes of
 * indices and also has the ability to respond to rendezvous events.
 *
 * <p/>ResolverSrdiMessages define a ttl, to indicate to the receiving service
 * whether to replicate such message or not.
 *
 * <p/>In addition A ResolverQuery defines a hopCount to indicate how many
 * hops a query has been forwarded. This element could be used to detect/stop a
 * query forward loopback hopCount is checked to make ensure a query is not
 * forwarded more than twice.
 *
 */

#include "jpr/jpr_excep.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_apr.h"
#include "jstring.h"
#include "jxta_discovery_service.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jxta_cm.h"
#include "jxta_sql.h"
#include "jxta_peergroup.h"
#include "jxta_peerview.h"
#include "jxta_peer_private.h"
#include "jxta_private.h"
#include "jxta_srdi.h"
#include "jxta_resolver_service.h"
#include "jxta_srdi_service_private.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_rdv_service_private.h" 
#include "jxta_string.h"
#include "jxta_util_priv.h"

#include <openssl/bn.h>
#include <openssl/sha.h>

static void init_e(Jxta_module * srdi, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws);
static Jxta_status start(Jxta_module * mod, const char *argv[]);
static void stop(Jxta_module * mod);
static Jxta_status jxta_srdi_service_start(Jxta_srdi_service * this);
static Jxta_status jxta_srdi_service_stop(Jxta_srdi_service * this);
static void get_interface(Jxta_service * self, Jxta_service ** svc);
static void get_mia(Jxta_service * srdi, Jxta_advertisement ** mia);
static void JXTA_STDCALL jxta_srdi_rdv_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL srdi_service_srdi_listener(Jxta_object * obj, void *arg);
static Jxta_boolean hopcount_ok(ResolverQuery * query, long count);
static void increment_hop_count(ResolverQuery * query);
static Jxta_vector *randomResult(Jxta_vector * result, int threshold);
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * this, Jxta_resolver_service * resolver, Jxta_peerview * peerview,
                                 const char *expression);
static Jxta_peer *getNumericReplica(Jxta_srdi_service * this, Jxta_resolver_service * resolver, Jxta_peerview * pvw,
                                    Jxta_range * rge, const char * val);
static Jxta_status forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                      Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
                                      Jxta_expiration_time expiration);
static Jxta_status forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                    Jxta_id * srcPid, Jxta_vector *entries, JString * queueName, JString *jKey);
static Jxta_status forwardSrdiEntry(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                    Jxta_id * srcPid, Jxta_SRDIEntryElement * entry, JString * queueName);
typedef struct {
    Extends(Jxta_srdi_service);
    Jxta_PG *group;
    const char *handlername;
    JString *instanceName;
    JString *srdi_id;
    long connectPollInterval;
    long pushInterval;

    Jxta_boolean stop;
    Jxta_boolean republish;
    Jxta_PG *parentgroup;
    Jxta_PGID *gid;
    JString *groupUniqueID;
    Jxta_rdv_service *rendezvous;
    Jxta_discovery_service *discovery;
    Jxta_endpoint_service *endpoint;
    Jxta_PID *peerID;
    Jxta_cm *cm;
    JString *srdi_queue_name;
    Jxta_listener *rdv_listener;
    Jxta_listener *endpoint_listener;
    Jxta_SrdiConfigAdvertisement *config;
} Jxta_srdi_service_ref;

static const char *__log_cat = "SrdiService";

Jxta_status
jxta_srdi_service_ref_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status status;
    Jxta_srdi_service_ref *me;
    Jxta_listener *listener = NULL;
    Jxta_PA *conf_adv=NULL;
    Jxta_svc *svc=NULL;
    me = (Jxta_srdi_service_ref *) module;

    status = JXTA_SUCCESS;
    if (impl_adv) {
    }

    me->group = group;
    jxta_PG_get_parentgroup(me->group, &me->parentgroup);
    jxta_PG_get_GID(group, &me->gid);
    jxta_PG_get_cache_manager(group, &me->cm);
    jxta_id_get_uniqueportion(me->gid, &me->groupUniqueID);
    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != me->parentgroup) {
        jxta_PG_get_configadv(me->parentgroup, &conf_adv);
    }
    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_srdi_classid, &svc);
        if (NULL != svc ) {
            me->config = jxta_svc_get_SrdiConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }
    
    if (NULL == me->config) {
        me->config = jxta_SrdiConfigAdvertisement_new();
    }
    
    if (assigned_id != 0) {
        jxta_id_to_jstring(assigned_id, &me->instanceName);
    }
    me->srdi_id = me->instanceName;
    jxta_PG_get_PID(group, &me->peerID);

    jxta_PG_get_rendezvous_service(group, &me->rendezvous);
    jxta_PG_get_discovery_service(group, &me->discovery);
    jxta_PG_get_endpoint_service(group, &(me->endpoint));

    if (status != JXTA_SUCCESS)
        return JXTA_FAILED;

    jxta_advertisement_register_global_handler("jxta:GenSRDI", (JxtaAdvertisementNewFunc) jxta_srdi_message_new);
    if (me->rendezvous != NULL) {
        /** Add a listener to rdv events */
        listener = jxta_listener_new((Jxta_listener_func) jxta_srdi_rdv_listener, me, 1, 1);
        if (listener != NULL) {
            jxta_listener_start(listener);
            status = jxta_rdv_service_add_event_listener(me->rendezvous, (char *) "SRDIMessage", (char *) "abc", listener);
            if (status != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to add an event listener to rdv events\n");
                return status;
            }
            me->rdv_listener = listener;
        }
    }
    if (me->endpoint != NULL) {
        listener = jxta_listener_new((Jxta_listener_func) srdi_service_srdi_listener, me, 10, 100);

        me->srdi_queue_name = jstring_new_0();
        jstring_append_1(me->srdi_queue_name, me->instanceName);
        jstring_append_2(me->srdi_queue_name, "-");
        jstring_append_1(me->srdi_queue_name, me->groupUniqueID);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "adding listener to endpoint %s \n",
                        jstring_get_string(me->srdi_queue_name));
        jxta_endpoint_service_add_listener(me->endpoint, jstring_get_string(me->srdi_queue_name), "Srdi", listener);

        me->endpoint_listener = listener;

        jxta_listener_start(listener);
    }
    return status;
}

/**
 * Replicates a SRDI message to other rendezvous'
 * entries are replicated by breaking out entries out of the message
 * and sorted out into rdv distribution bins. after which smaller messages
 * are sent to other rdv's
 *
 * @param  service - this service
 * @param  resolver - resolver 
 * @param  srdiMsg srdi message to replicate
 */
Jxta_status replicateEntries(Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_SRDIMessage * srdiMsg,
                             JString * queueName)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_vector *allEntries = NULL;
    Jxta_SRDIEntryElement *entry;
    JString *replicaExpression;
    Jxta_vector *localView = NULL;
    Jxta_status status;
    unsigned int i = 0;
    unsigned int rpv_size;
    Jxta_peerview *rpv = NULL;
    int ttl;
    Jxta_hashtable *peersHash=NULL;
    char **replicaLocs=NULL;
    char *replicaLocsSave=NULL;
    JString *jPkey=NULL;
    Jxta_id *srdi_owner = NULL;

    ttl = jxta_srdi_message_get_ttl(srdiMsg);
    jxta_srdi_message_get_peerID(srdiMsg, &srdi_owner);
    jxta_srdi_message_get_primaryKey(srdiMsg, &jPkey);
    rpv = jxta_rdv_service_get_peerview_priv((_jxta_rdv_service *) me->rendezvous);
    status = jxta_peerview_get_localview(rpv, &localView);
    if (localView == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Empty local view, stop replication.\n");
        status = JXTA_SUCCESS;
        goto CommonExit;
    }
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to get local view, stop replication.\n");
        goto CommonExit;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "start replicating \n");

    rpv_size = jxta_vector_size(localView);
    if (ttl < 1 || !jxta_rdv_service_is_rendezvous(me->rendezvous) || 
            rpv_size < jxta_srdi_cfg_get_replication_threshold(me->config)) {
        goto CommonExit;
    }

    status = jxta_srdi_message_get_entries(srdiMsg, &allEntries);

    if (allEntries == NULL) {
        goto CommonExit;
    }

    replicaExpression = jstring_new_0();
    peersHash = jxta_hashtable_new(rpv_size);
    for (i = 0; i < jxta_vector_size(allEntries); i++) {
        Jxta_peer *repPeer=NULL;
        const char *numericValue;

        entry = NULL;
        jxta_vector_get_object_at(allEntries, JXTA_OBJECT_PPTR(&entry), i);
        if (entry == NULL) {
            continue;
        }
        jstring_reset(replicaExpression, NULL);
        jstring_append_1(replicaExpression, entry->key);
        numericValue = jstring_get_string(entry->value);
        if (('#' != *numericValue) || ('#' == *numericValue && jxta_srdi_cfg_get_no_range(me->config))) {
            if ('#' == *numericValue && jxta_srdi_cfg_get_no_range(me->config)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got the no range of key:%s value:%s as true\n", jstring_get_string(entry->key), numericValue);
            }
            jstring_append_1(replicaExpression, entry->value);
        }
        if (NULL == entry->range || (0 == jstring_length(entry->range))) {
            repPeer = getReplicaPeer((Jxta_srdi_service *) me, resolver, rpv, jstring_get_string(replicaExpression));
        } else {
            Jxta_range *rge;

            rge = jxta_range_new();
            jxta_range_set_range(rge, jstring_get_string(entry->nameSpace), jstring_get_string(entry->key), jstring_get_string(entry->range));
            if ('#' == *numericValue) {
                numericValue++;
                repPeer = getNumericReplica((Jxta_srdi_service *) me, resolver, rpv, rge, numericValue );
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid value in SRDI Numeric Entry: %s  Did not replicate\n", numericValue);
            }
            JXTA_OBJECT_RELEASE(rge);
        }
        if (NULL != repPeer) {
            JString *jRepId = NULL;
            const char *repIdChar = NULL;
            Jxta_id *repId = NULL;
            Jxta_vector *peerEntries = NULL;

            jxta_peer_get_peerid(repPeer, &repId);
            jxta_id_to_jstring(repId, &jRepId);
            JXTA_OBJECT_RELEASE(repId);
            repIdChar = jstring_get_string(jRepId);
            if (jxta_hashtable_get(peersHash, repIdChar, strlen(repIdChar) + 1, JXTA_OBJECT_PPTR(&peerEntries)) != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding replica to peer HASH - %s \n", repIdChar);
                peerEntries = jxta_vector_new(0);
                jxta_vector_add_object_first(peerEntries, (Jxta_object *) repPeer);
                jxta_hashtable_put(peersHash, repIdChar, strlen(repIdChar) + 1, (Jxta_object *) peerEntries);
            }
            jxta_vector_add_object_last(peerEntries, (Jxta_object *)entry);
            JXTA_OBJECT_RELEASE(peerEntries);
            JXTA_OBJECT_RELEASE(repPeer);
            JXTA_OBJECT_RELEASE(jRepId);
        }
        JXTA_OBJECT_RELEASE(entry);
    }
    replicaLocs = jxta_hashtable_keys_get(peersHash);
    replicaLocsSave = (char *)replicaLocs;
    while (NULL != replicaLocs && *replicaLocs) {
        Jxta_status stat;
        Jxta_peer *sendPeer;
        Jxta_vector *peersVector = NULL;
        
        stat = jxta_hashtable_get(peersHash, *replicaLocs, strlen(*replicaLocs) + 1, JXTA_OBJECT_PPTR(&peersVector));
        if (JXTA_SUCCESS != stat) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error getting entry from peers hash key:%s \n", *replicaLocs);
            free(*replicaLocs++);
            continue;
        }
        stat = jxta_vector_remove_object_at(peersVector, JXTA_OBJECT_PPTR(&sendPeer), 0);
        if (JXTA_SUCCESS == stat) {
            forwardSrdiEntries((Jxta_srdi_service *) me, resolver, sendPeer, srdi_owner, peersVector, queueName, jPkey);
        }
        JXTA_OBJECT_RELEASE(peersVector);
        JXTA_OBJECT_RELEASE(sendPeer);
        free(*replicaLocs++);
    }
    if (NULL != replicaLocsSave ) free(replicaLocsSave);
    JXTA_OBJECT_RELEASE(peersHash);
    JXTA_OBJECT_RELEASE(allEntries);
    JXTA_OBJECT_RELEASE(replicaExpression);
CommonExit:
    JXTA_OBJECT_RELEASE(srdi_owner);
    if (NULL != localView)
        JXTA_OBJECT_RELEASE(localView);
    if (jPkey) JXTA_OBJECT_RELEASE(jPkey);
    return status;
}

static Jxta_status pushSrdi(Jxta_srdi_service * self, Jxta_resolver_service * resolver, ResolverSrdi * srdi, Jxta_id * peer)
{
    Jxta_status res = JXTA_SUCCESS;
    if (self) {
    }

    res = jxta_resolver_service_sendSrdi(resolver, srdi, peer);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "cannot send srdi message\n");
    }
    return res;
}

/**
 *  Forwards a Query to a specific peer
 *  hopCount is incremented to indicate this query is forwarded
 *
 * @param  peer        peerid to forward query to
 * @param  query       The query
 */
static Jxta_status
forwardQuery_peer(Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_id * peer, ResolverQuery * query)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_status status = JXTA_FAILED;

    if (NULL == peer) {
        return JXTA_INVALID_ARGUMENT;
    }
    increment_hop_count(query);
    if (hopcount_ok(query, 3)) {
        if (jxta_id_equals(me->peerID, peer)) {
            status = JXTA_SUCCESS;
        } else {
            status = jxta_resolver_service_sendQuery(resolver, query, peer);
       }
    }
    return status;
}

/**
 *  Forwards a Query to a list of peers
 *  hopCount is incremented to indicate this query is forwarded
 *
 * @param  peers       The peerids to forward query to
 * @param  query       The query
 */
static Jxta_status forwardQuery_peers(Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_vector * peers, 
                                      ResolverQuery * query)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i;
    Jxta_id *peerId;

    increment_hop_count(query);
    if (!hopcount_ok(query, 3)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Hop Count exceeded - Query Dropped\n");
        return JXTA_SUCCESS;
    }
    for (i = 0; peers != NULL && i < jxta_vector_size(peers); i++) {
        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peerId), i);
        if (jxta_id_equals(me->peerID, peerId)) {
            status = JXTA_SUCCESS;
        } else {
            status = jxta_resolver_service_sendQuery(resolver, query, peerId);
        }
        JXTA_OBJECT_RELEASE(peerId);
    }
    return status;
}

/**
 * Forwards a Query to a list of peers
 * if the list of peers exceeds threshold, and random threshold is picked
 * from <code>peers</code>
 * hopCount is incremented to indicate this query is forwarded
 *
 * @param  peers       The peerids to forward query to
 * @param  query       The query
 */
static Jxta_status forwardQuery_threshold(Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_vector * peers,
                                          ResolverQuery * query, int threshold)
{
    int size;

    size = jxta_vector_size(peers);
    if (size <= threshold) {
        forwardQuery_peers(self, resolver, peers, query);
    } else {
        Jxta_vector *newPeers;

        newPeers = randomResult(peers, threshold);
        forwardQuery_peers(self, resolver, newPeers, query);
        JXTA_OBJECT_RELEASE(newPeers);
    }
    return JXTA_SUCCESS;
}

/**
 *returns a random vector(threshold) from a given vector
 */
static Jxta_vector *randomResult(Jxta_vector * result, int threshold)
{
    int size;
    Jxta_vector *res = NULL;

    /* TODO */
    size = jxta_vector_size(result);
    if (threshold < size) {
        res = jxta_vector_new(threshold);
    }
    return res;
}

/**
 *  Given an expression return a peer from the list peers in the peerview
 *  this function is used to to give a replication point, and entry point
 *  to query 
 *
 * @param  this - srdi service pointer
 * @param  resolver - resolver service pointer
 * @param  peerview - peerview used to partition the hash and select a replica
 * @param  expression  expression to derive the mapping from
 * @return             The replicaPeer value
 */
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * this, Jxta_resolver_service * resolver, Jxta_peerview * pvw,
                                 const char *expression)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) this;
    Jxta_status status;
    Jxta_peerview *rpv = (Jxta_peerview *) pvw;
    unsigned int rpv_size;
    int significantBits;
    _jxta_peer_entry *peer;
    Jxta_status res;
    BN_ULONG pos;
    BIGNUM *sizeOfSpace;
    BIGNUM *sizeOfHashSpace;
    BIGNUM *bnHash;
    BIGNUM *bnPosition;
    BIGNUM *bnRemainder;
    BIGNUM *bnTotalSizeOfSpace;
    BN_CTX *bnCtx;
    unsigned char hash[20];
    unsigned char *hashVal;

    Jxta_vector *localView = NULL;

    if (this || resolver || expression) {
    }

    rpv = jxta_rdv_service_get_peerview_priv((_jxta_rdv_service *) me->rendezvous);
    status = jxta_peerview_get_localview(rpv, &localView);

    if (status != JXTA_SUCCESS)
        return NULL;

    if (localView == NULL)
        return NULL;

    rpv_size = jxta_vector_size(localView);
    bnCtx = BN_CTX_new();
    bnHash = BN_new();
    sizeOfSpace = BN_new();
    sizeOfHashSpace = BN_new();
    bnPosition = BN_new();
    bnRemainder = BN_new();
    bnTotalSizeOfSpace = BN_new();

    BN_set_word(sizeOfSpace, (unsigned long) rpv_size);
    BN_one(sizeOfHashSpace);
    hashVal = SHA1((const unsigned char *) expression, strlen(expression), (unsigned char *) &hash);
    BN_bin2bn((const unsigned char *) &hash, 20, bnHash);
    significantBits = BN_num_bits(bnHash);
    BN_lshift(sizeOfHashSpace, sizeOfHashSpace, significantBits);
    BN_mul(bnTotalSizeOfSpace, bnHash, sizeOfSpace, bnCtx);
    BN_CTX_free(bnCtx);
    bnCtx = BN_CTX_new();
    BN_div(bnPosition, bnRemainder, bnTotalSizeOfSpace, sizeOfHashSpace, bnCtx);
    BN_CTX_free(bnCtx);
    pos = BN_get_word(bnPosition);
    BN_free(bnHash);
    BN_free(sizeOfSpace);
    BN_free(sizeOfHashSpace);
    BN_free(bnPosition);
    BN_free(bnRemainder);
    BN_free(bnTotalSizeOfSpace);
    res = jxta_vector_remove_object_at(localView, JXTA_OBJECT_PPTR(&peer), pos);
    JXTA_OBJECT_RELEASE(localView);
    if (JXTA_SUCCESS == res && pos < rpv_size) {
        PTValid(peer, _jxta_peer_entry);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address in getReplicaPeer expression: %s : %s://%s\n",
                        expression,
                        jxta_endpoint_address_get_protocol_name(jxta_peer_get_address_priv((Jxta_peer *) peer)),
                        jxta_endpoint_address_get_protocol_address(jxta_peer_get_address_priv((Jxta_peer *) peer)));
        return peer;
    }

    return NULL;
}

static Jxta_peer *getNumericReplica(Jxta_srdi_service * this, Jxta_resolver_service * resolver, Jxta_peerview * pvw,
                                    Jxta_range * rge, const char * val)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) this;
    Jxta_status status;
    Jxta_peerview *rpv = (Jxta_peerview *) pvw;
    int rpv_size;
    int pos;
    Jxta_peer* peer=NULL;
    Jxta_vector *localView = NULL;

    rpv = jxta_rdv_service_get_peerview_priv((_jxta_rdv_service *) me->rendezvous);
    status = jxta_peerview_get_localview(rpv, &localView);

    if (status != JXTA_SUCCESS)
        return NULL;

    if (localView == NULL)
        return NULL;
    rpv_size = jxta_vector_size(localView);
    pos = jxta_range_get_position(rge, val, rpv_size);
    if (pos < 0) {
        /* no range specified */
        if (-1 == pos) {          
            if (NULL == val && jxta_srdi_cfg_get_no_range(me->config)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Query when replica includes value returns NULL\n");
            } else {
                const char *attribute;
                JString *replicaExpression=NULL;

                replicaExpression = jstring_new_2(jxta_range_get_element(rge));
                attribute = jxta_range_get_attribute(rge);
                if (NULL != attribute) {
                    jstring_append_2(replicaExpression, attribute);
                }
                if (jxta_srdi_cfg_get_no_range(me->config)) {
                    jstring_append_2(replicaExpression, val);
                } 
                peer = getReplicaPeer(this, resolver, pvw, jstring_get_string(replicaExpression));
                JXTA_OBJECT_RELEASE(replicaExpression);
            }
        }
    } else {
        status = jxta_vector_remove_object_at(localView, JXTA_OBJECT_PPTR(&peer), pos);
        if (JXTA_SUCCESS == status && pos < rpv_size) {
            PTValid(peer, _jxta_peer_entry);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address from getNumericReplica : %s://%s\n",
                        jxta_endpoint_address_get_protocol_name(jxta_peer_get_address_priv((Jxta_peer *) peer)),
                        jxta_endpoint_address_get_protocol_address(jxta_peer_get_address_priv((Jxta_peer *) peer)));
        }
    }
    JXTA_OBJECT_RELEASE(localView);
    return peer;
}

    /**
     *  forward srdi message to another peer
     *
     * @param  peerid      PeerID to forward query to
     * @param  srcPid      The source originator
     * @param  primaryKey  primary key
     * @param  secondarykey secondary key
     * @param  value       value of the entry
     * @param  expiration  expiration in ms
     */
static Jxta_status
forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
                   Jxta_expiration_time expiration)
{
    /* not yet impelemented */
    if (service || resolver || peer || srcPid || primaryKey || secondarykey || value || expiration) {
    }

    return JXTA_SUCCESS;
}

static Jxta_status
forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                 Jxta_id * srcPid, Jxta_vector *entries, JString * queueName, JString *jKey)
{
    Jxta_status res = JXTA_SUCCESS;
    ResolverSrdi *srdi = NULL;
    Jxta_id *pid = NULL;
    JString *messageString = NULL;
    Jxta_SRDIMessage *msg = NULL;
    JString *jSourceId = NULL;
    JString *jPid = NULL;
    
    jxta_id_to_jstring(srcPid, &jSourceId);
    
    jxta_peer_get_peerid(peer, &pid);
    if (pid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot retrieve peer id from jxta_peer. Message discarded\n");
        res = JXTA_FAILED;
        goto final;
    }
    jxta_id_to_jstring(pid, &jPid);

    if (!strcmp(jstring_get_string(jPid), jstring_get_string(jSourceId))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Don't send it to myself. Message ignored\n");
        res = JXTA_FAILED;
        goto final;
    }
    msg = jxta_srdi_message_new_1(1, srcPid, (char *) jstring_get_string(jKey), entries);
    entries = NULL;

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot allocate jxta_srdi_message_new\n");
        res = JXTA_INVALID_ARGUMENT;
        goto final;
    }

    res = jxta_srdi_message_get_xml(msg, &messageString);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot serialize srdi message\n");
        res = JXTA_NOMEM;
        goto final;
    }
    srdi = jxta_resolver_srdi_new_1(queueName, messageString, srcPid);
    if (srdi == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot allocate a resolver srdi message\n");
        res = JXTA_NOMEM;
        goto final;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending to peer %s\n", jstring_get_string(jPid));

    res = jxta_resolver_service_sendSrdi(resolver, srdi, pid);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot send srdi message\n");
    }

  final:
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    if (jSourceId)
        JXTA_OBJECT_RELEASE(jSourceId);
    if (pid)
        JXTA_OBJECT_RELEASE(pid);
    if (jPid)
        JXTA_OBJECT_RELEASE(jPid);
    if (messageString)
        JXTA_OBJECT_RELEASE(messageString);
    if (srdi)
        JXTA_OBJECT_RELEASE(srdi);
    return res;   
}

    /**
     *  forward srdi message to another peer
     *
     * @param  peerid      PeerID to forward query to
     * @param  srcPid      The source originator
     * @param  entry       SRDI entry element
     */
static Jxta_status
forwardSrdiEntry(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                 Jxta_id * srcPid, Jxta_SRDIEntryElement * entry, JString * queueName)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *entries = NULL;

    entries = jxta_vector_new(1);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward SRDI Entry element:%s value:%s\n", jstring_get_string(entry->key),
                    jstring_get_string(entry->value));

    jxta_vector_add_object_last(entries, (Jxta_object *) entry);

    res = forwardSrdiEntries(service, resolver, peer, srcPid, entries, queueName, entry->key);
    
    if (entries) JXTA_OBJECT_RELEASE(entries);
    return res;
}

static Jxta_vector * searchSrdi(Jxta_srdi_service *me, const char *handler, const char *ns, const char *attr, const char *val)
{
    Jxta_srdi_service_ref *_self = (Jxta_srdi_service_ref *) me;
    Jxta_cm *cm = NULL;
    Jxta_object **advStrings=NULL;
    JString *sqlwhere = jstring_new_0();
    JString *jAttr = jstring_new_2(attr);
    JString *jSQLoper = jstring_new_2(SQL_EQUAL);
    JString *jVal = jstring_new_2(val);
    Jxta_vector *ret = NULL;
    Jxta_object **scan;
    Jxta_vector *responses = jxta_vector_new(1);
    JString *tmp = NULL;

    jxta_PG_get_cache_manager(_self->group, &cm);
    if (cm == NULL)
        goto Release_exit;
  
    jxta_query_build_SQL_operator(jAttr, jSQLoper, jVal, sqlwhere);

    jstring_append_2(sqlwhere, SQL_AND CM_COL_Handler SQL_EQUAL);
    tmp = jstring_new_2(handler);
    SQL_VALUE(sqlwhere, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (NULL != ns) {
        jstring_append_2(sqlwhere, SQL_AND CM_COL_NameSpace SQL_EQUAL);
        tmp = jstring_new_2(ns);
        SQL_VALUE(sqlwhere, tmp);
        JXTA_OBJECT_RELEASE(tmp);
    }
  
    advStrings = jxta_cm_sql_query_srdi(cm, NULL, sqlwhere);
  
    scan = advStrings;
    while (scan != NULL && *scan) {
        jxta_vector_add_object_last(responses, *(scan + 1));
        JXTA_OBJECT_RELEASE(*scan);
        JXTA_OBJECT_RELEASE(*(scan + 1));
        scan += 2;
    }
    ret = getPeerids(responses);

Release_exit:
    if (NULL != advStrings)
        free(advStrings);
    JXTA_OBJECT_RELEASE(sqlwhere);
    JXTA_OBJECT_RELEASE(jAttr);
    JXTA_OBJECT_RELEASE(jSQLoper);
    JXTA_OBJECT_RELEASE(jVal);
    JXTA_OBJECT_RELEASE(responses);
    if (cm != NULL) 
        JXTA_OBJECT_RELEASE(cm);

  return ret;
}

static void JXTA_STDCALL jxta_srdi_rdv_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = 0;
    JString *string = NULL;
    const char *peerid = "(null)";
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) arg;
        /** Get info from the event */
    int type = rdv_event->event;

    if (arg) {
    }

    res = jxta_id_to_jstring(rdv_event->pid, &string);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "got no id string in the rdv event \n");
        goto finish;
    } else {
        peerid = jstring_get_string(string);
    }
    switch (type) {
    case JXTA_RDV_CLIENT_DISCONNECTED:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Connected rdv client disconnected peer in SRDI =%s\n", peerid);

        if (NULL == me->cm) {
            jxta_PG_get_cache_manager(me->group, &me->cm);
        }
        if (NULL != me->cm) {
            jxta_cm_remove_srdi_entries(me->cm, string);
        }
        break;
    case JXTA_RDV_CLIENT_CONNECTED:
    case JXTA_RDV_CLIENT_RECONNECTED:
    case JXTA_RDV_CONNECTED:
    case JXTA_RDV_FAILED:
    case JXTA_RDV_DISCONNECTED:
    case JXTA_RDV_RECONNECTED:
    case JXTA_RDV_BECAME_EDGE:
    case JXTA_RDV_BECAME_RDV:
    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Catch All for event %d ignored\n", type);
        break;
    }

  finish:
    if (string != NULL)
        JXTA_OBJECT_RELEASE(string);
}

static void JXTA_STDCALL srdi_service_srdi_listener(Jxta_object * obj, void *arg)
{
    if (obj || arg) {
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "got the listener in srdi srdi listener");
}

static Jxta_boolean hopcount_ok(ResolverQuery * query, long count)
{
    if (jxta_resolver_query_get_hopcount(query) < count)
        return TRUE;
    return FALSE;
}

static void increment_hop_count(ResolverQuery * query)
{
    long count = jxta_resolver_query_get_hopcount(query);

    if (count < 0) {
        count = 0;
    }
    count++;
    jxta_resolver_query_set_hopcount(query, count);
}

typedef Jxta_srdi_service_methods Jxta_srdi_service_ref_methods;

Jxta_srdi_service_ref_methods jxta_srdi_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_srdi_service_ref_init,
      init_e,
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface},
    "Jxta_srdi_service_methods",
    replicateEntries,
    pushSrdi,
    forwardQuery_peer,
    forwardQuery_peers,
    forwardQuery_threshold,
    getReplicaPeer,
    getNumericReplica,
    forwardSrdiMessage,
    searchSrdi
};

/**
 * Initializes an instance of the SRDI Service. (exception variant).
 * 
 * @param service a pointer to the instance of the SRDI Service.
 * @param group a pointer to the PeerGroup the Discovery Service is 
 * initialized for.
 *
 */

static void init_e(Jxta_module * srdi, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws)
{
    Jxta_status s;
    
    s = jxta_srdi_service_ref_init(srdi,group,assigned_id,impl_adv);
    if (s != JXTA_SUCCESS) {
        Throw(s);
    }
}

/**
*  stop the current running thread
*/
static Jxta_status jxta_srdi_service_stop(Jxta_srdi_service * this)
{
    Jxta_srdi_service_ref *self = (Jxta_srdi_service_ref *) this;

    if (self->stop) {
        return JXTA_SUCCESS;
    }

    self->stop = TRUE;

    jxta_listener_stop(self->rdv_listener);
    jxta_rdv_service_remove_event_listener(self->rendezvous, (char *) "SRDIMessage", (char *) "abc");
    JXTA_OBJECT_RELEASE(self->rdv_listener);

    jxta_listener_stop(self->endpoint_listener);
    jxta_endpoint_service_remove_listener(self->endpoint, jstring_get_string(self->srdi_queue_name), "Srdi");
    JXTA_OBJECT_RELEASE(self->endpoint_listener);
    return JXTA_SUCCESS;
}

static Jxta_status start(Jxta_module * mod, const char *argv[])
{
    if (argv) {
    }

    return jxta_srdi_service_start((Jxta_srdi_service *) mod);
}

static void stop(Jxta_module * mod)
{
    jxta_srdi_service_stop((Jxta_srdi_service *) mod);
}

static Jxta_status jxta_srdi_service_start(Jxta_srdi_service * this)
{
    Jxta_srdi_service_ref *self = (Jxta_srdi_service_ref *) this;
    if (self->stop == FALSE) {
        return JXTA_SUCCESS;
    }

    self->stop = FALSE;
    return JXTA_SUCCESS;
}

static void jxta_srdi_service_ref_construct(Jxta_srdi_service_ref * srdi, Jxta_srdi_service_ref_methods * methods)
{
    /*
     * we do not extend Jxta_discovery_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_srdi_service_methods);

    jxta_srdi_service_construct((Jxta_srdi_service *) srdi, (Jxta_srdi_service_methods *) methods);

    /* Set our rt type checking string */
    srdi->thisType = "Jxta_srdi_service_ref";
}

static void jxta_srdi_service_ref_destruct(Jxta_srdi_service_ref * srdi)
{
    PTValid(srdi, Jxta_srdi_service_ref);

    if (srdi->endpoint != 0)
        JXTA_OBJECT_RELEASE(srdi->endpoint);
    if (srdi->rendezvous != 0)
        JXTA_OBJECT_RELEASE(srdi->rendezvous);

    if (srdi->instanceName != 0)
        JXTA_OBJECT_RELEASE(srdi->instanceName);

    if (srdi->srdi_queue_name)
        JXTA_OBJECT_RELEASE(srdi->srdi_queue_name);
    if (srdi->cm != 0)
        JXTA_OBJECT_RELEASE(srdi->cm);
    if (srdi->peerID != 0)
        JXTA_OBJECT_RELEASE(srdi->peerID);
    if (srdi->config) 
        JXTA_OBJECT_RELEASE(srdi->config);
    if(srdi->groupUniqueID)
        JXTA_OBJECT_RELEASE(srdi->groupUniqueID);
    if(srdi->gid)
        JXTA_OBJECT_RELEASE(srdi->gid);
    if(srdi->parentgroup)
        JXTA_OBJECT_RELEASE(srdi->parentgroup);
    

    /* call the base classe's dtor. */
    jxta_srdi_service_destruct((Jxta_srdi_service *) srdi);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction of SRDI finished\n");
}

/**
 * Frees an instance of the SRDI Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Discovery Service to free.
 */
static void srdi_free(void *service)
{
    /* call the hierarchy of dtors */
    jxta_srdi_service_ref_destruct((Jxta_srdi_service_ref *) service);

    /* free the object itself */
    free(service);
}

/**
 * Creates a new instance of the Discovery Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_srdi_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Discovery Service.
 * @return a non initialized instance of the Discovery Service
 */

Jxta_srdi_service_ref *jxta_srdi_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_srdi_service_ref * srdi ;
    
    srdi= (Jxta_srdi_service_ref *)malloc(sizeof(Jxta_srdi_service_ref));

    if (srdi == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(srdi, 0, sizeof(Jxta_srdi_service_ref));
    JXTA_OBJECT_INIT(srdi, (JXTA_OBJECT_FREE_FUNC) srdi_free, 0);

    /* call the hierarchy of ctors */
    jxta_srdi_service_ref_construct(srdi, &jxta_srdi_service_ref_methods);

    /* return the new object */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "New SRDI\n");
    return srdi;
}

/**
 * return an object that proxies this service. (a service method).
 *
 * @return this service object itself.
 */
static void get_interface(Jxta_service * self, Jxta_service ** svc)
{
    PTValid(self, Jxta_srdi_service_ref);

    JXTA_OBJECT_RELEASE(svc);
    *svc = self;
}

/**
 * return an object that proxies this service. (a service method).
 *
 * @return this service object itself.
 */
static void get_mia(Jxta_service * srdi, Jxta_advertisement ** mia)
{
    if (srdi || mia) {
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
