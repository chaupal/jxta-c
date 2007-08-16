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
 *  $Id$
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

static const char *__log_cat = "SrdiSvc";

#include <assert.h>
#include "jpr/jpr_excep_proto.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_apr.h"
#include "jstring.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jxta_cm.h"
#include "jxta_cm_private.h"
#include "jxta_sql.h"
#include "jxta_peergroup.h"
#include "jxta_peerview.h"
#include "jxta_peer_private.h"
#include "jxta_private.h"
#include "jxta_srdi.h"
#include "jxta_resolver_service.h"
#include "jxta_srdi_service_private.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_stdpg_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_string.h"
#include "jxta_util_priv.h"
#include "jxta_endpoint_service_priv.h"

#include <openssl/bn.h>
#include <openssl/sha.h>

static Jxta_status start(Jxta_module * mod, const char *argv[]);
static void stop(Jxta_module * this);
static void JXTA_STDCALL jxta_srdi_rdv_listener(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL srdi_service_srdi_cb(Jxta_object * obj, void *arg);
static Jxta_boolean hopcount_ok(ResolverQuery * query, long count);
static void increment_hop_count(ResolverQuery * query);
static Jxta_vector *randomResult(Jxta_vector * result, int threshold);
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * this, const char *expression);
static Jxta_peer *getNumericReplica(Jxta_srdi_service * this, Jxta_range * rge, const char *val);
static Jxta_status forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                      Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
                                      Jxta_expiration_time expiration);
static Jxta_status forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                      Jxta_id * srcPid, Jxta_vector * entries, JString * queueName, JString * jKey);
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
    Jxta_endpoint_service *endpoint;
    Jxta_PID *peerID;
    Jxta_cm *cm;
    JString *srdi_queue_name;
    Jxta_listener *rdv_listener;
    Jxta_SrdiConfigAdvertisement *config;
    void *ep_cookie;
} Jxta_srdi_service_ref;

Jxta_status jxta_srdi_service_ref_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status status;
    Jxta_srdi_service_ref *me;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;

    me = (Jxta_srdi_service_ref *) module;

    status = JXTA_SUCCESS;
    if (impl_adv) {
    }

    me->group = group;
    jxta_PG_get_parentgroup(me->group, &me->parentgroup);
    jxta_PG_get_GID(group, &me->gid);
    peergroup_get_cache_manager(group, &me->cm);
    jxta_id_get_uniqueportion(me->gid, &me->groupUniqueID);
    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != me->parentgroup) {
        jxta_PG_get_configadv(me->parentgroup, &conf_adv);
    }
    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_srdi_classid, &svc);
        if (NULL != svc) {
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
    jxta_PG_get_endpoint_service(group, &(me->endpoint));

    if (status != JXTA_SUCCESS)
        return JXTA_FAILED;
    jxta_advertisement_register_global_handler("jxta:GenSRDI", (JxtaAdvertisementNewFunc) jxta_srdi_message_new);

    me->srdi_queue_name = jstring_new_0();
    jstring_append_1(me->srdi_queue_name, me->instanceName);
    jstring_append_2(me->srdi_queue_name, "-");
    jstring_append_1(me->srdi_queue_name, me->groupUniqueID);

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
    Jxta_status status;
    unsigned int i = 0;
    int ttl;
    Jxta_hashtable *peersHash = NULL;
    char **replicaLocs = NULL;
    char **replicaLocsSave = NULL;
    JString *jPkey = NULL;
    JString *jPeerId = NULL;
    Jxta_id *peerid = NULL;
    jxta_srdi_message_get_peerID(srdiMsg, &peerid);
    jxta_id_to_jstring(peerid, &jPeerId);
    JXTA_OBJECT_RELEASE(peerid); 
    ttl = jxta_srdi_message_get_ttl(srdiMsg);
    jxta_srdi_message_get_primaryKey(srdiMsg, &jPkey);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Replicate Entries\n");

    if (ttl < 1 || !jxta_rdv_service_is_rendezvous(me->rendezvous) ) {
        goto FINAL_EXIT;
    }

    status = jxta_srdi_message_get_entries(srdiMsg, &allEntries);

    if (allEntries == NULL) {
        goto FINAL_EXIT;
    }

    replicaExpression = jstring_new_0();
    peersHash = jxta_hashtable_new( 10 );
    for (i = 0; i < jxta_vector_size(allEntries); i++) {
        Jxta_peer *repPeer = NULL;
        const char *numericValue;
        Jxta_SRDIEntryElement *newEntry = NULL;
        entry = NULL;
        jxta_vector_get_object_at(allEntries, JXTA_OBJECT_PPTR(&entry), i);
        if (entry == NULL) {
            continue;
        }
        newEntry = jxta_srdi_new_element_3(entry->key, entry->value, entry->nameSpace, entry->advId, entry->range, entry->expiration, entry->seqNumber);
        if (newEntry->value == NULL) {
            if (newEntry->seqNumber > 0) {
                status = cm_get_srdi_with_seq_number(me->cm, jPeerId, newEntry->seqNumber, newEntry);
                if (JXTA_ITEM_NOTFOUND == status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                        , "Could not get the entry for seqNumber " JXTA_SEQUENCE_NUMBER_FMT " from peerid:%s in replicateEntries\n", newEntry->seqNumber, jstring_get_string(jPeerId));
                    JXTA_OBJECT_RELEASE(newEntry);
                    JXTA_OBJECT_RELEASE(entry);
                    continue;
                }
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                        , "Got the name/value from the srdi cache %s/%s\n"
                        , jstring_get_string(newEntry->key), jstring_get_string(newEntry->value));
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR
                       , "Received null value without seq no > 0 --- Should not happen and entry is being discarded\n");
                JXTA_OBJECT_RELEASE(newEntry);
                JXTA_OBJECT_RELEASE(entry);
                continue;
            }
        }

        jstring_reset(replicaExpression, NULL);
        jstring_append_1(replicaExpression, newEntry->key);
        numericValue = jstring_get_string(newEntry->value);
        if (('#' != *numericValue) || ('#' == *numericValue && jxta_srdi_cfg_get_no_range(me->config))) {
            if ('#' == *numericValue && jxta_srdi_cfg_get_no_range(me->config)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got the no range of key:%s value:%s as true\n",
                                jstring_get_string(newEntry->key), numericValue);
            }
            jstring_append_1(replicaExpression, newEntry->value);
        }
        
        if (NULL == newEntry->range || (0 == jstring_length(newEntry->range))) {
            repPeer = getReplicaPeer((Jxta_srdi_service *) me, jstring_get_string(replicaExpression));
        } else {
            Jxta_range *rge;

            rge = jxta_range_new();
            jxta_range_set_range(rge, jstring_get_string(newEntry->nameSpace), jstring_get_string(newEntry->key),
                                 jstring_get_string(newEntry->range));
            if ('#' == *numericValue) {
                numericValue++;
                repPeer = getNumericReplica((Jxta_srdi_service *) me, rge, numericValue);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid value in SRDI Numeric Entry: %s  Did not replicate\n",
                                numericValue);
            }
            JXTA_OBJECT_RELEASE(rge);
        }
        
        if (NULL != repPeer) {
            JString *jRepId = NULL;
            const char *repIdChar = NULL;
            Jxta_id *repId = NULL;
            Jxta_vector *peerEntries = NULL;

            jxta_peer_get_peerid(repPeer, &repId);
            if( !jxta_id_equals( repId, me->peerID) ) {
                /* Send replicas to peers other than ourself. */
            jxta_id_to_jstring(repId, &jRepId);
            repIdChar = jstring_get_string(jRepId);
            if (jxta_hashtable_get(peersHash, repIdChar, strlen(repIdChar) + 1, JXTA_OBJECT_PPTR(&peerEntries)) != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding replica to peer HASH - %s \n", repIdChar);
                peerEntries = jxta_vector_new(0);
                jxta_vector_add_object_first(peerEntries, (Jxta_object *) repPeer);
                jxta_hashtable_put(peersHash, repIdChar, strlen(repIdChar) + 1, (Jxta_object *) peerEntries);
            }
            newEntry->seqNumber = 0;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding entry to peer vector %s - %s \n"
                    , jstring_get_string(newEntry->key), jstring_get_string(newEntry->value));
            jxta_vector_add_object_last(peerEntries, (Jxta_object *) newEntry);
            JXTA_OBJECT_RELEASE(peerEntries);
            JXTA_OBJECT_RELEASE(jRepId);
        }
            JXTA_OBJECT_RELEASE(repId);
            JXTA_OBJECT_RELEASE(repPeer);
        }
        JXTA_OBJECT_RELEASE(entry);
        JXTA_OBJECT_RELEASE(newEntry);
    }
    replicaLocs = jxta_hashtable_keys_get(peersHash);
    replicaLocsSave = (char **) replicaLocs;
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
        if (JXTA_SUCCESS == stat && jxta_vector_size(peersVector) > 0) {
            forwardSrdiEntries((Jxta_srdi_service *) me, resolver, sendPeer, me->peerID, peersVector, queueName, jPkey);
        }
        JXTA_OBJECT_RELEASE(peersVector);
        JXTA_OBJECT_RELEASE(sendPeer);
        free(*replicaLocs++);
    }
    if (NULL != replicaLocsSave)
        free(replicaLocsSave);
    JXTA_OBJECT_RELEASE(peersHash);
    JXTA_OBJECT_RELEASE(allEntries);
    JXTA_OBJECT_RELEASE(replicaExpression);
  FINAL_EXIT:
    if (jPeerId)
        JXTA_OBJECT_RELEASE(jPeerId);
    if (jPkey)
        JXTA_OBJECT_RELEASE(jPkey);
    return status;
}

static Jxta_status pushSrdi_priv(Jxta_srdi_service * self, Jxta_resolver_service * resolver, JString * instance,
                            ResolverSrdi * srdi, Jxta_id * peer)
{
    Jxta_status res = JXTA_SUCCESS;

    res = jxta_resolver_service_sendSrdi(resolver, srdi, peer);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot send srdi message\n");
    }
    return res;
}

static Jxta_status record_delta_entry(Jxta_srdi_service_ref *me, Jxta_id * peer, JString * instance, Jxta_SRDIMessage * msg)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int i;
    Jxta_hashtable *seq_hash = NULL;
    Jxta_vector *entries = NULL;
    JString *jPeer=NULL;
    const char *peerString = NULL;

    if (NULL == peer) {
        if (jxta_rdv_service_is_rendezvous((Jxta_rdv_service *) me->rendezvous) == TRUE) {
            return JXTA_SUCCESS;
        }
        jPeer = jstring_new_2("NoPeer");
    } else {
        jxta_id_to_jstring(peer, &jPeer);
    }
    peerString = jstring_get_string(jPeer);

    res = jxta_srdi_message_get_entries(msg, &entries);
    seq_hash = jxta_hashtable_new(0);
    for (i = 0; i < jxta_vector_size(entries); i++) {
        Jxta_status status;
        Jxta_sequence_number newSeqNumber = 0;
        Jxta_SRDIEntryElement *entry;
        Jxta_SRDIEntryElement *saveEntry;
        JString *jNewValue = NULL;
        char aTmp[64];
        jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        res = cm_save_delta_entry(me->cm, jPeer, instance, entry, &jNewValue, &newSeqNumber);
        memset(aTmp, 0, sizeof(aTmp));
        /* if the seq number changed, use it */
        if (newSeqNumber > 0) {
            entry->seqNumber = newSeqNumber;
        }
        apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "-------------- checking for seq no %s in the hash ------------ \n", aTmp);
        status = jxta_hashtable_get(seq_hash, aTmp, strlen(aTmp), JXTA_OBJECT_PPTR(&saveEntry));
        if (JXTA_SUCCESS != status) {
            jxta_hashtable_put(seq_hash, aTmp, strlen(aTmp), (Jxta_object *) entry);
            if (JXTA_SUCCESS == res) {
                if (NULL != jNewValue) {
                    jstring_reset(entry->value, NULL);
                    jstring_append_1(entry->value, jNewValue);
                    JXTA_OBJECT_RELEASE(jNewValue);
                } else {
                    JXTA_OBJECT_RELEASE(entry->value);
                    entry->value = NULL;
                }
                JXTA_OBJECT_RELEASE(entry->key);
                entry->key = NULL;
                JXTA_OBJECT_RELEASE(entry->advId);
                entry->advId = NULL;
                JXTA_OBJECT_RELEASE(entry->nameSpace);
                entry->nameSpace = NULL;
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Found %s - Removing from entries\n", aTmp);
            JXTA_OBJECT_RELEASE(saveEntry);
            jxta_vector_remove_object_at(entries, NULL, i);
            JXTA_OBJECT_RELEASE(jNewValue);
        }
        JXTA_OBJECT_RELEASE(entry);
    }

    JXTA_OBJECT_RELEASE(jPeer);
    JXTA_OBJECT_RELEASE(seq_hash);
    JXTA_OBJECT_RELEASE(entries);
    return res;
}

static Jxta_status pushSrdi_msg(Jxta_srdi_service * self, Jxta_resolver_service * resolver, JString * instance,
                            Jxta_SRDIMessage * msg, Jxta_id * peer)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    JString *messageString = NULL;
    ResolverSrdi *srdi = NULL;
    
    if (jxta_srdi_cfg_is_delta_cache_supported(me->config)) {
        record_delta_entry(me, peer, instance, msg);
    }
    res = jxta_srdi_message_get_xml(msg, &messageString);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot serialize srdi message\n");
        return JXTA_NOMEM;
    }

    srdi = jxta_resolver_srdi_new_1(instance, messageString, NULL);
    JXTA_OBJECT_RELEASE(messageString);
    if (srdi == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate a resolver srdi message\n");
        return JXTA_NOMEM;
    }

    res = pushSrdi_priv(self, resolver, instance, srdi, peer);

    JXTA_OBJECT_RELEASE(srdi);

    return res;
}

static Jxta_status pushSrdi(Jxta_srdi_service * self, Jxta_resolver_service * resolver, JString * instance,
                            ResolverSrdi * srdi, Jxta_id * peer)
{
    Jxta_status res = JXTA_SUCCESS;
    
    JXTA_DEPRECATED_API();
    res = pushSrdi_priv(self, resolver, instance, srdi, peer);
    return res;
}

/**
 *  Forwards a Query to a specific peer
 *  hopCount is incremented to indicate this query is forwarded
 *
 * @param  peer        peerid to forward query to
 * @param  query       The query
 */
static Jxta_status forwardQuery_peer(Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_id * peer, ResolverQuery * query)
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
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * me, const char *expression)
{
    Jxta_status res;
    Jxta_srdi_service_ref *myself = PTValid(me, Jxta_srdi_service_ref);
    Jxta_peerview *rpv;
    BIGNUM *bn_hash = NULL;
    Jxta_peer *peer = NULL;

    rpv = rdv_service_get_peerview_priv(myself->rendezvous);

    res = jxta_peerview_gen_hash( rpv, (unsigned char const *) expression, strlen(expression), &bn_hash );

    res = jxta_peerview_get_peer_for_target_hash( rpv, bn_hash, &peer );

    BN_free( bn_hash );

    if (JXTA_SUCCESS == res ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address in getReplicaPeer expression: %s : %s://%s\n",
                        expression,
                        jxta_endpoint_address_get_protocol_name(jxta_peer_address(peer)),
                        jxta_endpoint_address_get_protocol_address(jxta_peer_address(peer)));
        return peer;
    }

    return NULL;
}

static Jxta_peer *getNumericReplica(Jxta_srdi_service * me, Jxta_range * rge, const char *val)
{
    Jxta_status status;
    Jxta_srdi_service_ref *myself = PTValid(me, Jxta_srdi_service_ref);
    Jxta_peerview *rpv;
    unsigned int cluster_count;
    int pos;
    Jxta_peer *peer = NULL;

    rpv = rdv_service_get_peerview_priv(myself->rendezvous);

    cluster_count = jxta_peerview_get_globalview_size( rpv );

    pos = jxta_range_get_position(rge, val, cluster_count);
    if (pos < 0) {
        /* no range specified */
        if (-1 == pos) {
            if (NULL == val && jxta_srdi_cfg_get_no_range(myself->config)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Query when replica includes value returns NULL\n");
            } else {
                const char *attribute;
                JString *replicaExpression = NULL;

                replicaExpression = jstring_new_2(jxta_range_get_element(rge));
                attribute = jxta_range_get_attribute(rge);
                if (NULL != attribute) {
                    jstring_append_2(replicaExpression, attribute);
                }
                if (jxta_srdi_cfg_get_no_range(myself->config)) {
                    jstring_append_2(replicaExpression, val);
                }
                peer = getReplicaPeer( (Jxta_srdi_service*) myself, jstring_get_string(replicaExpression));
                JXTA_OBJECT_RELEASE(replicaExpression);
            }
        }
    } else {
        status = jxta_peerview_get_associate_peer(rpv, pos, &peer);
        if (JXTA_SUCCESS == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address from getNumericReplica : %s://%s\n",
                            jxta_endpoint_address_get_protocol_name(jxta_peer_address(peer)),
                            jxta_endpoint_address_get_protocol_address(jxta_peer_address(peer)));
        }
    }

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
static Jxta_status forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
                   Jxta_expiration_time expiration)
{
    /* not yet impelemented */
    if (service || resolver || peer || srcPid || primaryKey || secondarykey || value || expiration) {
    }

    return JXTA_SUCCESS;
}

static Jxta_status forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * srcPid, Jxta_vector * entries, JString * queueName, JString * jKey)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) service;
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

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot allocate jxta_srdi_message_new\n");
        res = JXTA_INVALID_ARGUMENT;
        goto final;
    }

    if (jxta_srdi_cfg_is_delta_cache_supported(me->config)) {
        record_delta_entry(me, pid, queueName, msg);
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
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending to peer %s\n%s\n", jstring_get_string(jPid), jstring_get_string(messageString));


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
static Jxta_status forwardSrdiEntry(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                 Jxta_id * srcPid, Jxta_SRDIEntryElement * entry, JString * queueName)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *entries = NULL;

    entries = jxta_vector_new(1);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward SRDI Entry element:%s value:%s\n", jstring_get_string(entry->key),
                    jstring_get_string(entry->value));

    jxta_vector_add_object_last(entries, (Jxta_object *) entry);

    res = forwardSrdiEntries(service, resolver, peer, srcPid, entries, queueName, entry->key);

    if (entries)
        JXTA_OBJECT_RELEASE(entries);
    return res;
}

static Jxta_vector *searchSrdi(Jxta_srdi_service * me, const char *handler, const char *ns, const char *attr, const char *val)
{
    Jxta_srdi_service_ref *_self = (Jxta_srdi_service_ref *) me;
    Jxta_cm *cm = NULL;
    Jxta_cache_entry **cache_entries = NULL;
    JString *sqlwhere = jstring_new_0();
    JString *jAttr = jstring_new_2(attr);
    JString *jSQLoper = jstring_new_2(SQL_EQUAL);
    JString *jVal = jstring_new_2(val);
    Jxta_vector *ret = NULL;
    Jxta_cache_entry **scan;
    Jxta_vector *responses = jxta_vector_new(1);
    JString *tmp = NULL;

    peergroup_get_cache_manager(_self->group, &cm);
    if (cm == NULL)
        goto FINAL_EXIT;

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

    cache_entries = cm_sql_query_srdi_ns(cm, "*:*", sqlwhere);

    scan = cache_entries;
    while (scan != NULL && *scan) {
        Jxta_cache_entry * cache_entry;
        cache_entry = *scan;
        jxta_vector_add_object_last(responses, (Jxta_object *) cache_entry->profid);
        JXTA_OBJECT_RELEASE(cache_entry);
        scan++;
    }
    ret = getPeerids(responses);

  FINAL_EXIT:
    if (NULL != cache_entries)
        free(cache_entries);
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
    JString *jPeerid = NULL;
    const char *peerid = "(null)";
    Jxta_peer *peer = NULL;
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) arg;
    Jxta_time expires=0;
    /** Get info from the event */
    int type = rdv_event->event;

    res = jxta_id_to_jstring(rdv_event->pid, &jPeerid);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "There is no id string in the rdv event \n");
        goto finish;
    } else {
        peerid = jstring_get_string(jPeerid);
    }
    if (NULL == me->cm) {
        peergroup_get_cache_manager(me->group, &me->cm);
    }
    switch (type) {
        case JXTA_RDV_CLIENT_DISCONNECTED:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Connected rdv client disconnected peer in SRDI =%s\n", peerid);
            if (NULL != me->cm) {
                cm_remove_srdi_entries(me->cm, jPeerid);
            }
            break;
        case JXTA_RDV_CLIENT_CONNECTED:
        case JXTA_RDV_CLIENT_RECONNECTED:
        case JXTA_RDV_CONNECTED:
            res = jxta_rdv_service_get_peer(me->rendezvous, rdv_event->pid, &peer);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Got a rdv event with an unknown peerid %s\n", peerid);
            }
            if (NULL != peer) {
                expires = jxta_peer_get_expires(peer);
            }
            if (NULL != me->cm && expires > 0) {
                res = cm_update_srdi_times(me->cm, jPeerid, expires);
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Error update SRDI times %d\n", res);
                }
            }
            if (type == JXTA_RDV_CONNECTED) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                            "Connected rdv expires: %" APR_INT64_T_FMT " %s\n",expires, peerid);
            }
            else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                            "Connected rdv client expires: %" APR_INT64_T_FMT " %s\n",expires, peerid);
            }
            break;
        case JXTA_RDV_FAILED:
        case JXTA_RDV_DISCONNECTED:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Rendezvous %s %s in SRDI\n",
                            peerid, JXTA_RDV_FAILED == type ? "FAILED" : "DISCONNECTED");

            if (NULL == me->cm) {
                peergroup_get_cache_manager(me->group, &me->cm);
            }
            if (NULL == me->cm) {
                break;
            }
            if (NULL != me->cm) {
                cm_remove_srdi_entries(me->cm, jPeerid);
            }
            break;
        case JXTA_RDV_RECONNECTED:
        case JXTA_RDV_BECAME_EDGE:
        case JXTA_RDV_BECAME_RDV:
        default:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Catch All for event %d ignored\n", type);
            break;
    }

  finish:
    if (NULL != peer)
        JXTA_OBJECT_RELEASE(peer);
    if (jPeerid != NULL)
        JXTA_OBJECT_RELEASE(jPeerid);
}

/* FIXME: is this correct? slowhog 5/9/2006 */
static Jxta_status JXTA_STDCALL srdi_service_srdi_cb(Jxta_object * obj, void *arg)
{
    if (obj || arg) {
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "got the message in srdi srdi callback");
    return JXTA_SUCCESS;
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

const Jxta_srdi_service_ref_methods jxta_srdi_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_srdi_service_ref_init,
      start,
      stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl,
     service_on_option_set},
    "Jxta_srdi_service_methods",
    replicateEntries,
    pushSrdi,
    pushSrdi_msg,
    forwardQuery_peer,
    forwardQuery_peers,
    forwardQuery_threshold,
    getReplicaPeer,
    getNumericReplica,
    forwardSrdiMessage,
    searchSrdi
};

/**
*  stop the current running thread
*/
static void stop(Jxta_module * me)
{
    Jxta_srdi_service_ref *myself = PTValid( me, Jxta_srdi_service_ref );

    jxta_listener_stop(myself->rdv_listener);
    jxta_rdv_service_remove_event_listener(myself->rendezvous, "SRDIMessage", "abc");
    JXTA_OBJECT_RELEASE(myself->rdv_listener);

    endpoint_service_remove_recipient(myself->endpoint, myself->ep_cookie);
}

static Jxta_status start(Jxta_module * me, const char *argv[])
{
    Jxta_srdi_service_ref *myself = PTValid( me, Jxta_srdi_service_ref );
    Jxta_listener * listener;
    Jxta_status status;

    assert(myself->rendezvous);
    assert(myself->endpoint);

    /** Add a listener to rdv events */
    listener = jxta_listener_new((Jxta_listener_func) jxta_srdi_rdv_listener, myself, 1, 1);
    if (listener != NULL) {
        jxta_listener_start(listener);
        status = jxta_rdv_service_add_event_listener(myself->rendezvous, "SRDIMessage", "abc", listener);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to add an event listener to rdv events\n");
            return status;
        }
        myself->rdv_listener = listener;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "adding recipient %s\n", jstring_get_string(myself->srdi_queue_name));
    status = endpoint_service_add_recipient(myself->endpoint, &myself->ep_cookie, jstring_get_string(myself->srdi_queue_name), "Srdi",
                                            srdi_service_srdi_cb, myself);

    return JXTA_SUCCESS;
}

static void jxta_srdi_service_ref_construct(Jxta_srdi_service_ref * srdi, Jxta_srdi_service_ref_methods const * methods)
{
    /*
     * we do not extend Jxta_srdi_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_srdi_service_methods);

    jxta_srdi_service_construct((Jxta_srdi_service *) srdi, (Jxta_srdi_service_methods const *) methods);

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
    if (srdi->groupUniqueID)
        JXTA_OBJECT_RELEASE(srdi->groupUniqueID);
    if (srdi->gid)
        JXTA_OBJECT_RELEASE(srdi->gid);
    if (srdi->parentgroup)
        JXTA_OBJECT_RELEASE(srdi->parentgroup);

    srdi->thisType = NULL;

    /* call the base classe's dtor. */
    jxta_srdi_service_destruct((Jxta_srdi_service *) srdi);

}

/**
 * Frees an instance of the SRDI Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the SRDI Service to free.
 */
static void srdi_free(Jxta_object *service)
{
    /* call the hierarchy of dtors */
    jxta_srdi_service_ref_destruct((Jxta_srdi_service_ref *) service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "FREE SRDI Service Ref\n");

    /* free the object itself */
    free(service);
}

/**
 * Creates a new instance of the SRDI Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_srdi_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the SRDI Service.
 * @return a non initialized instance of the SRDI Service
 */

Jxta_srdi_service_ref *jxta_srdi_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_srdi_service_ref *srdi;

    srdi = (Jxta_srdi_service_ref *) calloc(1, sizeof(Jxta_srdi_service_ref));

    if (srdi == NULL) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "NEW SRDI Service Ref\n");

    /* Initialize the object */
    JXTA_OBJECT_INIT(srdi, srdi_free, 0);

    /* call the hierarchy of ctors */
    jxta_srdi_service_ref_construct(srdi, &jxta_srdi_service_ref_methods);

    /* return the new object */
    return srdi;
}

/* vim: set ts=4 sw=4 et tw=130: */
