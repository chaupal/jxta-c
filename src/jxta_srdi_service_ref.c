/*
 *  Copyright (c) 2001 Sun Microsystems, Inc.  All rights
 *  reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:c
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
#include "jxta_peergroup_private.h"
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
#include "jxta_xml_util.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_discovery_service_private.h"

#include <openssl/bn.h>
#include <openssl/sha.h>

static const char *SRDI_QUEUENAME = "Srdi";
static const char *JXTA_SRDI_NS_NAME = "jxta";
static const char *JXTA_SRDI_MESSAGE = "SrdiMessage";

static Jxta_status start(Jxta_module * mod, const char *argv[]);
static void stop(Jxta_module * this);
static void JXTA_STDCALL srdi_rdv_listener(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL srdi_service_srdi_cb(Jxta_object * obj, void *arg);
static Jxta_status get_element_bytes(Jxta_message_element *element, JString **string);
static Jxta_status get_srdi_msg_from_msg(Jxta_message *msg, Jxta_bytevector **msg_elem_bv, Jxta_SRDIMessage **srdi_msg);
static Jxta_status get_srdi_msg_from_ep_elem(Jxta_endpoint_msg_entry_element *ep_elem, Jxta_bytevector **ep_elem_bv, Jxta_SRDIMessage **srdi_msg);
static Jxta_boolean adjust_srdi_entries(Jxta_srdi_service *service, Jxta_SRDIMessage * srdi_msg
                    , Jxta_vector *queued_entries, Jxta_id *peerid, Jxta_time prev_diff, Jxta_boolean ep_entries);

static Jxta_status split_endpoint_message(Jxta_srdi_service *self, Jxta_endpoint_message *ep_msg
                            , Jxta_vector **new_ep_entries, apr_int64_t max_size, apr_int64_t *required);

static Jxta_boolean hopcount_ok(ResolverQuery * query, long count);
static void increment_hop_count(ResolverQuery * query);
static Jxta_vector *randomResult(Jxta_vector * result, int threshold);
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * me, const char *expression, Jxta_peer **alt_peer);
static Jxta_status getReplicaPeers(Jxta_srdi_service * me, const char *expression, Jxta_peer ** peer, Jxta_vector **peers);
static Jxta_peer *getNumericReplica(Jxta_srdi_service * me, Jxta_range * rge, const char *val);
static Jxta_status forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                      Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
                                      Jxta_expiration_time expiration);
static Jxta_status forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * srcPid, Jxta_vector * entries, int ttl, JString * queueName, JString * jKey);
static Jxta_status forwardSrdiEntries_priv(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * fromPid, Jxta_id * srcPid, Jxta_vector * entries, int ttl, JString * queueName, JString * jKey, Jxta_hashtable ** ret_msgs);

#ifdef UNUSED_VWF
static Jxta_status forwardSrdiEntry(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                                    Jxta_id * srcPid, Jxta_SRDIEntryElement * entry, JString * queueName);
#endif

#define DEFAULT_REPLICA_UPDATE_LIMIT (5 * 60 * 1000)  /* 5 minutes */

typedef struct {
    Extends(Jxta_srdi_service);
    Jxta_PG *group;
    const char *handlername;
    JString *instanceName;
    JString *srdi_id;
    Jxta_hashtable *srdilisteners;
    long connectPollInterval;
    long pushInterval;
    Jxta_boolean running;
    Jxta_boolean stop;
    Jxta_boolean republish;
    Jxta_PGID *gid;
    JString *groupUniqueID;
    char *groupid_c;
    Jxta_rdv_service *rendezvous;
    /* Jxta_discovery_service *discovery; */
    Jxta_endpoint_service *endpoint;
    Jxta_resolver_service *resolver;
    Jxta_PID *peerID;
    Jxta_cm *cm;
    JString *srdi_queue_name;
    Jxta_listener *rdv_listener;
    Jxta_SrdiConfigAdvertisement *config;
    Jxta_boolean dup_srdi_entries;
    void *ep_cookie;
    Jxta_time update_limit;
    apr_thread_mutex_t *ep_msgs_lock;
    Jxta_hashtable * ep_msgs;
    Jxta_vector *ep_msgs_queue;
    apr_thread_mutex_t *srdi_msgs_lock;
    Jxta_hashtable * srdi_msgs;
    Jxta_vector *srdi_msgs_queue;
    apr_pool_t *pool;
} Jxta_srdi_service_ref;

typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_PG *group;
    Jxta_listener *srdilistener;
    void *ep_cookie;
} Jxta_SRDIListenerRef;

typedef struct _srdi_msgs_thread_struct {
    JXTA_OBJECT_HANDLE;
    Jxta_srdi_service_ref *srdi_service;
    JString *instance;
    Jxta_peer *dest;
    Jxta_hashtable * msgs;
} Srdi_msgs_thread_struct ;

typedef struct _srdi_msg_thread_struct {
    JXTA_OBJECT_HANDLE;
    Jxta_srdi_service_ref *srdi_service;
    JString * instance;
    Jxta_id * dest;
    Jxta_message * msg;
    Jxta_boolean sync;
} Srdi_msg_thread_struct;

typedef struct _SRDI_parm_arg {
    JXTA_OBJECT_HANDLE;
    Jxta_message *msg;
    Jxta_time timestamp;
    JString *peerid_j;
    Jxta_id * peer_id;
    const char *ep_variable;
    JString *svc_name;
    const char *svc_param;
    Jxta_time prev_diff;
} SRDI_parm_arg;

typedef struct _SRDI_queue_entry {
    JXTA_OBJECT_HANDLE;
    Jxta_endpoint_filter_entry *f_entry;
    Jxta_SRDIMessage *srdi_msg;
    Jxta_endpoint_msg_entry_element *ep_elem;
    Jxta_bytevector *ep_elem_bv;
} SRDI_queue_entry;

static Jxta_status srdi_service_remove_replicas(Jxta_srdi_service_ref * srdi, Jxta_vector * entries);
static void get_replica_peers_priv(Jxta_srdi_service_ref * me, Jxta_SRDIEntryElement * entry, Jxta_peer **peer, Jxta_vector **peers);
static Jxta_status srdi_send_srdi_msgs(Jxta_srdi_service_ref * self, JString *instance
                                    , Jxta_peer * dest, Jxta_hashtable * msgs);

static void srdi_push_srdi_msg(Jxta_srdi_service_ref * self, JString * instance,
                            Jxta_message * msg, Jxta_id * peerid, Jxta_boolean sync);


static void srdi_parm_arg_free(Jxta_object *obj)
{
    SRDI_parm_arg *parm_arg = (SRDI_parm_arg*) obj;

    if (parm_arg->msg) JXTA_OBJECT_RELEASE(parm_arg->msg);
    if (parm_arg->peerid_j) JXTA_OBJECT_RELEASE(parm_arg->peerid_j);
    if (parm_arg->peer_id) JXTA_OBJECT_RELEASE(parm_arg->peer_id);
    if (parm_arg->svc_name) JXTA_OBJECT_RELEASE(parm_arg->svc_name);
    free(parm_arg);
}

static SRDI_parm_arg * srdi_parm_arg_new()
{
    SRDI_parm_arg *res;
    res = calloc(1, sizeof(SRDI_parm_arg));
    JXTA_OBJECT_INIT(res, srdi_parm_arg_free, NULL);
    res->prev_diff = jpr_time_now();
    return res;
}

static void srdi_q_entry_free(Jxta_object *obj)
{
    SRDI_queue_entry *q_entry = (SRDI_queue_entry *) obj;

    if (q_entry->f_entry)
        JXTA_OBJECT_RELEASE(q_entry->f_entry);
    if (q_entry->srdi_msg)
        JXTA_OBJECT_RELEASE(q_entry->srdi_msg);
    if (q_entry->ep_elem)
        JXTA_OBJECT_RELEASE(q_entry->ep_elem);
    if (q_entry->ep_elem_bv)
        JXTA_OBJECT_RELEASE(q_entry->ep_elem_bv);
    free(q_entry);
}

static SRDI_queue_entry * srdi_q_entry_new(Jxta_endpoint_filter_entry *f_entry)
{
    SRDI_queue_entry *res;
    res = calloc(1, sizeof(SRDI_queue_entry));
    JXTA_OBJECT_INIT(res, srdi_q_entry_free, NULL);
    res->f_entry = JXTA_OBJECT_SHARE(f_entry);
    return res;
}


static Jxta_boolean isRunning(Jxta_srdi_service_ref * srdi)
{
    Jxta_boolean running = FALSE;
    jxta_service_lock((Jxta_service *)srdi);
    running = srdi->running;
    jxta_service_unlock((Jxta_service *)srdi);
    return running;
}

Jxta_status jxta_srdi_service_ref_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status status;
    Jxta_srdi_service_ref *me;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_PG *parentgroup = NULL;

    me = (Jxta_srdi_service_ref *) module;

    status = JXTA_SUCCESS;

    jxta_service_init((Jxta_service *) module, group, assigned_id, impl_adv);

    me->group = group;
    jxta_PG_get_parentgroup(me->group, &parentgroup);
    jxta_PG_get_GID(group, &me->gid);
    peergroup_get_cache_manager(group, &me->cm);
    jxta_id_get_uniqueportion(me->gid, &me->groupUniqueID);
    me->groupid_c = strdup(jstring_get_string(me->groupUniqueID));
    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != parentgroup) {
        jxta_PG_get_configadv(parentgroup, &conf_adv);
    }
    if (NULL != parentgroup)
        JXTA_OBJECT_RELEASE(parentgroup);

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

    me->dup_srdi_entries = jxta_srdi_cfg_are_srdi_entries_duplicated(me->config);
    apr_pool_create(&me->pool, NULL);
    me->srdi_msgs = jxta_hashtable_new(0);
    me->srdi_msgs_queue = jxta_vector_new(0);
    apr_thread_mutex_create(&me->srdi_msgs_lock, APR_THREAD_MUTEX_NESTED,me->pool );
    me->ep_msgs = jxta_hashtable_new(0);
    me->ep_msgs_queue = jxta_vector_new(0);
    apr_thread_mutex_create(&me->ep_msgs_lock, APR_THREAD_MUTEX_NESTED,me->pool);
    if (assigned_id != 0) {
        jxta_id_to_jstring(assigned_id, &me->instanceName);
    }
    if (-1 == jxta_srdi_cfg_get_replica_update_limit(me->config)) {
        jxta_srdi_cfg_set_replica_update_limit(me->config,DEFAULT_REPLICA_UPDATE_LIMIT);
    }

    me->update_limit = cm_get_latest_timestamp(me->cm);
    me->update_limit += jxta_srdi_cfg_get_replica_update_limit(me->config);

    me->srdi_id = me->instanceName;
    jxta_PG_get_PID(group, &me->peerID);

    jxta_PG_get_rendezvous_service(group, &me->rendezvous);
    jxta_PG_get_endpoint_service(group, &(me->endpoint));
    jxta_PG_get_resolver_service(group, &(me->resolver));

    if (status != JXTA_SUCCESS)
        return JXTA_FAILED;

    jxta_advertisement_register_global_handler("jxta:GenSRDI", (JxtaAdvertisementNewFunc) jxta_srdi_message_new);

    me->srdilisteners = jxta_hashtable_new(0);
    me->srdi_queue_name = jstring_new_0();
    jstring_append_1(me->srdi_queue_name, me->instanceName);
    jstring_append_2(me->srdi_queue_name, SRDI_QUEUENAME);

    return status;
}

static void add_replica_to_peers(Jxta_srdi_service_ref *me, Jxta_hashtable *peersHash, Jxta_peer *repPeer, Jxta_SRDIEntryElement *newEntry, const char *header)
{
    JString *jRepId = NULL;
    const char *repIdChar = NULL;
    Jxta_id *repId = NULL;
    Jxta_vector *peerEntries = NULL;

    jxta_peer_get_peerid(repPeer, &repId);
    jxta_id_to_jstring(repId, &jRepId);

    if( !jxta_id_equals( repId, me->peerID) ) {

        /* Send replicas to peers other than ourself. */
        repIdChar = jstring_get_string(jRepId);
        if (jxta_hashtable_get(peersHash, repIdChar, strlen(repIdChar) + 1, JXTA_OBJECT_PPTR(&peerEntries)) != JXTA_SUCCESS) {
            peerEntries = jxta_vector_new(0);
            jxta_vector_add_object_first(peerEntries, (Jxta_object *) repPeer);
            jxta_hashtable_put(peersHash, repIdChar, strlen(repIdChar) + 1, (Jxta_object *) peerEntries);
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%sAdding entry %" APR_INT64_T_FMT " to peer %s vector %s \n", header
                 , newEntry->seqNumber, repIdChar, jstring_get_string(newEntry->key));
        jxta_vector_add_object_last(peerEntries, (Jxta_object *) newEntry);

        JXTA_OBJECT_RELEASE(peerEntries);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Don't send to ourself %s\n", jstring_get_string(jRepId));
    }
    if (repId)
        JXTA_OBJECT_RELEASE(repId);
    if (jRepId)
        JXTA_OBJECT_RELEASE(jRepId);
}

static void SRDIListenerRef_free(Jxta_object * o)
{
    Jxta_SRDIListenerRef *ref = (Jxta_SRDIListenerRef *) o;
    if (ref->srdilistener) {
        JXTA_OBJECT_RELEASE(ref->srdilistener);
        ref->srdilistener = NULL;
    }

    free(ref);
}
    

/**
 * Registers a given Listner.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status registerSrdiListener(Jxta_srdi_service * self, JString * name, Jxta_listener * handler)
{
    Jxta_status res=JXTA_SUCCESS;
    char const *hashname;
    Jxta_srdi_service_ref *me = PTValid(self, Jxta_srdi_service_ref);
    JString *new_name;

    new_name = jstring_new_0();
    jstring_append_1(new_name, name);
    jstring_append_2(new_name, "/");
    jstring_append_2(new_name, SRDI_QUEUENAME);
    hashname = jstring_get_string(new_name);

    if (strlen(hashname) != 0 && JXTA_SUCCESS != jxta_hashtable_contains(me->srdilisteners, hashname, strlen(hashname))) {
        Jxta_SRDIListenerRef * listenerRef = (Jxta_SRDIListenerRef *) calloc(1, sizeof(Jxta_SRDIListenerRef));
        if (listenerRef != NULL) {
            JXTA_OBJECT_INIT(listenerRef, SRDIListenerRef_free, 0);
            listenerRef->srdilistener = JXTA_OBJECT_SHARE(handler);
            listenerRef->group = me->group;  //No need to share the group since the listener references
                                             //will be deleted before the main srdi ref object
            res = jxta_PG_add_recipient(listenerRef->group, &listenerRef->ep_cookie, jstring_get_string(name), SRDI_QUEUENAME, srdi_service_srdi_cb, me);
            jxta_hashtable_put(me->srdilisteners, hashname, strlen(hashname), (Jxta_object *) listenerRef);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "SRDI listener registered for %s in group %s\n", hashname, me->groupid_c);
            JXTA_OBJECT_RELEASE(listenerRef);
        } else {
            res = JXTA_NOMEM;
        }
    } else {
        res = JXTA_INVALID_ARGUMENT;
    }
    JXTA_OBJECT_RELEASE(new_name);
    return res;
}

/**
  * unregisters a given Listener.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status unregisterSrdiListener(Jxta_srdi_service * self, JString * name)
{
    Jxta_object *obj = NULL;
    char const *hashname;
    Jxta_status status;
    Jxta_srdi_service_ref *me = PTValid(self, Jxta_srdi_service_ref);
    JString *new_name;

    new_name = jstring_new_0();
    jstring_append_1(new_name, name);
    jstring_append_2(new_name, "/");
    jstring_append_2(new_name, SRDI_QUEUENAME);
    hashname = jstring_get_string(new_name);

    status = jxta_hashtable_del(me->srdilisteners, hashname, strlen(hashname), &obj);
    if (obj != NULL) {
        Jxta_SRDIListenerRef *listenerRef = (Jxta_SRDIListenerRef *) obj;
        if (listenerRef->group) {
            if (listenerRef->ep_cookie) {
                jxta_PG_remove_recipient(listenerRef->group, listenerRef->ep_cookie);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "SRDI Listener unregistered for %s in group %s\n", hashname, me->groupid_c);
                listenerRef->ep_cookie = NULL;
            }
            listenerRef->group = NULL;
        }
        JXTA_OBJECT_RELEASE(listenerRef);
    }
    JXTA_OBJECT_RELEASE(new_name);
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
Jxta_status replicateEntries(Jxta_srdi_service * self, Jxta_resolver_service * resolver
                    , Jxta_SRDIMessage * srdiMsg, JString * queueName, Jxta_hashtable **msgs)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_status status = JXTA_FAILED;
    Jxta_vector *allEntries = NULL;
    Jxta_SRDIEntryElement *entry;
    unsigned int i = 0;
    int ttl;
    Jxta_hashtable *peersHash = NULL;
    Jxta_hashtable *dupsHash = NULL;
    Jxta_hashtable *adsHash = NULL;
    char **replicaLocs = NULL;
    char **replicaLocsSave = NULL;
    JString *jPkey = NULL;
    JString *jPeerId = NULL;
    Jxta_id *from_peerid = NULL;
    Jxta_id *source_peerid = NULL;
    JString *jSrcPeerid=NULL;
    Jxta_id * peerLoc;
    Jxta_hashtable * loopHash;
    Jxta_boolean end = FALSE;
    Jxta_boolean busy = TRUE;

    jxta_srdi_message_get_peerID(srdiMsg, &from_peerid);
    jxta_id_to_jstring(from_peerid, &jPeerId);
    jxta_srdi_message_get_SrcPID(srdiMsg, &source_peerid);
    jxta_id_to_jstring(source_peerid, &jSrcPeerid);
    ttl = jxta_srdi_message_get_ttl(srdiMsg);
    jxta_srdi_message_get_primaryKey(srdiMsg, &jPkey);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replicate Entries\n");

    if (ttl < 1 || !jxta_rdv_service_is_rendezvous(me->rendezvous) ) {
        goto FINAL_EXIT;
    } else if (NULL != msgs) {
        status = JXTA_NOTIMP;
        goto FINAL_EXIT;
    }

    status = jxta_srdi_message_get_entries(srdiMsg, &allEntries);


    if (allEntries == NULL) {
        goto FINAL_EXIT;
    }

    peersHash = jxta_hashtable_new( 10 );
    dupsHash = jxta_hashtable_new( 10 );
    adsHash = jxta_hashtable_new(10);

    for (i = 0; i < jxta_vector_size(allEntries); i++) {
        Jxta_SRDIEntryElement *newEntry = NULL;

        entry = NULL;
        jxta_vector_get_object_at(allEntries, JXTA_OBJECT_PPTR(&entry), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Getting entry seq:" JXTA_SEQUENCE_NUMBER_FMT " %d %s\n", entry->seqNumber, i, entry->replicate == TRUE ? "TRUE":"FALSE");
        if (entry == NULL || (entry->duplicate && !entry->dup_target)) {
            if (NULL != entry)
                JXTA_OBJECT_RELEASE(entry);
            continue;
        }
        newEntry = jxta_srdi_new_element_4(entry->key, entry->value, entry->nameSpace, entry->advId, entry->range, entry->expiration, entry->seqNumber, entry->replicate, entry->re_replicate);
        if (newEntry->value == NULL) {
            Jxta_boolean ignore_entry = FALSE;
            if (newEntry->seqNumber > 0) {
                Jxta_SRDIEntryElement * get_entry = NULL;
                status = cm_get_srdi_with_seq_number(me->cm, jPeerId, jSrcPeerid, entry->seqNumber, &get_entry);
                if (JXTA_ITEM_NOTFOUND == status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING
                        , "Could not get the entry for seqNumber " JXTA_SEQUENCE_NUMBER_FMT " from peerid:%s in replicateEntries\n", newEntry->seqNumber, jstring_get_string(jPeerId));
                    ignore_entry = TRUE;
                    status = JXTA_SUCCESS;  /*reset the status so it is not retained*/
                } else if (get_entry->fwd_this) {
                    JXTA_OBJECT_RELEASE(newEntry);
                    newEntry = JXTA_OBJECT_SHARE(get_entry);
                } else {
                    newEntry->value = jstring_clone(get_entry->value);
                    if (newEntry->key)
                        JXTA_OBJECT_RELEASE(newEntry->key);
                    newEntry->key = jstring_clone(get_entry->key);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Got the name/value from the srdi cache %s/%s\n"
                        , jstring_get_string(get_entry->key), jstring_get_string(get_entry->value));
                }
                if (get_entry)
                    JXTA_OBJECT_RELEASE(get_entry);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING
                       , "Received null value without seq no > 0 --- Should not happen and entry is being discarded\n");
                ignore_entry = TRUE;
            }
            if (ignore_entry) {
                if (entry)
                    JXTA_OBJECT_RELEASE(entry);
                if (newEntry) {
                    JXTA_OBJECT_RELEASE(newEntry);
                    newEntry = NULL;
                }
                continue;
            }
        } else {
            newEntry->update_only = jxta_srdi_message_update_only(srdiMsg);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "New entry value --- %s\n", jstring_get_string(newEntry->value));
        }

        if (JXTA_SUCCESS == status && !newEntry->fwd_this) {
            Jxta_peer *advPeer=NULL;
            Jxta_boolean first_pass = TRUE;
            Jxta_version *peerVersion=NULL;
            Jxta_peer *repPeer = NULL;
            Jxta_vector *repPeers = NULL;
            Jxta_peer *alt_repPeer = NULL;

            get_replica_peers_priv(me, newEntry, &repPeer, &repPeers);
            if (repPeers) {
                jxta_vector_clone(repPeers, &newEntry->radius_peers, 0, INT_MAX);
            }

            advPeer = getReplicaPeer((Jxta_srdi_service *) me, jstring_get_string(entry->advId), &alt_repPeer);

            if (NULL != advPeer) {

                /* if target is this peer use the alternate peer */
                if (jxta_id_equals(jxta_peer_peerid(advPeer), (Jxta_id *) me->peerID) && NULL != alt_repPeer) {
                    JString *alt_peer_j = NULL;

                    JXTA_OBJECT_RELEASE(advPeer);
                    advPeer = alt_repPeer;
                    alt_repPeer = NULL;
                    jxta_id_to_jstring(jxta_peer_peerid(advPeer), &alt_peer_j);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Changed the dup %" APR_INT64_T_FMT " to alt_peer:%s\n"
                                    , newEntry->seqNumber
                                    ,jstring_get_string(alt_peer_j));
                    JXTA_OBJECT_RELEASE(alt_peer_j);
                }
                peerVersion = jxta_PA_get_version(jxta_peer_adv(advPeer));
                if(peerVersion == NULL ) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Got a PA without a peer version [%pp]\n", advPeer);
                    if (alt_repPeer)
                        JXTA_OBJECT_RELEASE(alt_repPeer);
                    if (advPeer)
                        JXTA_OBJECT_RELEASE(advPeer);
                    if (repPeer)
                        JXTA_OBJECT_RELEASE(repPeer);
                    if (repPeers)
                        JXTA_OBJECT_RELEASE(repPeers);
                    goto FINAL_EXIT;
                }
            }
            while ((repPeer && first_pass) || (NULL != repPeers && jxta_vector_size(repPeers) > 0)) {
                if (!first_pass) {
                    jxta_vector_remove_object_at(repPeers, JXTA_OBJECT_PPTR(&repPeer), 0);
                    /* if this entry is the dup don't add it */
                    if (NULL != advPeer && (jxta_version_compatible_1(SRDI_DUPLICATE_ENTRIES, peerVersion))) {
                        if (jxta_id_equals(jxta_peer_peerid(advPeer),jxta_peer_peerid(repPeer))) {
                            JXTA_OBJECT_RELEASE(repPeer);
                            repPeer = NULL;
                            continue;
                        }
                    }
                }
                if (NULL != advPeer && first_pass) {
                    Jxta_boolean same_peer = FALSE;
                    Jxta_SRDIEntryElement * clone_entry = NULL;
                    if (jxta_version_compatible_1(SRDI_DUPLICATE_ENTRIES, peerVersion)) {
                        same_peer = jxta_id_equals(jxta_peer_peerid(advPeer),jxta_peer_peerid(repPeer));
                    }
                    if (!same_peer && entry->replicate) {
                        add_replica_to_peers(me, peersHash, repPeer, newEntry, "");
                    }
                    if (jxta_version_compatible_1(SRDI_DUPLICATE_ENTRIES, peerVersion)) {
                        int j;
 
                        clone_entry = jxta_srdi_element_clone(newEntry);
                        clone_entry->duplicate = TRUE;
                        clone_entry->dup_target = same_peer;
                        add_replica_to_peers(me, dupsHash, advPeer, clone_entry, "dup ---->");
                        /* remove the entry from additional replicas */
                        for (j=0; NULL != newEntry->radius_peers && j<jxta_vector_size(newEntry->radius_peers); j++) {
                            Jxta_peer *cmp_peer;
                            jxta_vector_get_object_at(newEntry->radius_peers, JXTA_OBJECT_PPTR(&cmp_peer), j);
                            if (jxta_id_equals(jxta_peer_peerid(advPeer),jxta_peer_peerid(cmp_peer))) {
                                jxta_vector_remove_object_at(newEntry->radius_peers, NULL, j);
                                JXTA_OBJECT_RELEASE(cmp_peer);
                                break;
                            }
                            JXTA_OBJECT_RELEASE(cmp_peer);
                        }
                        JXTA_OBJECT_RELEASE(clone_entry);
                    }
                } else if (entry->replicate) {
                    add_replica_to_peers(me, peersHash, repPeer, newEntry, "");
                }
                first_pass = FALSE;
                JXTA_OBJECT_RELEASE(repPeer);
                repPeer = NULL;
            }
            if (alt_repPeer)
                JXTA_OBJECT_RELEASE(alt_repPeer);
            if (advPeer)
                JXTA_OBJECT_RELEASE(advPeer);
            if (peerVersion)
                JXTA_OBJECT_RELEASE(peerVersion);
            if (repPeer)
                JXTA_OBJECT_RELEASE(repPeer);
            if (repPeers)
                JXTA_OBJECT_RELEASE(repPeers);
        } else if (newEntry->fwd_this) {
            Jxta_id * get_id = NULL;
            Jxta_peer *fwd_peer=NULL;

            jxta_id_from_cstr( &get_id, jstring_get_string(newEntry->fwd_peerid));
            peerview_get_peer(rdv_service_get_peerview_priv(me->rendezvous), get_id, &fwd_peer);
            add_replica_to_peers(me, peersHash, fwd_peer, newEntry, "fwd --->");

            JXTA_OBJECT_RELEASE(fwd_peer);
            JXTA_OBJECT_RELEASE(get_id);
        }
        JXTA_OBJECT_RELEASE(entry);
        if (newEntry)
            JXTA_OBJECT_RELEASE(newEntry);
    }

    /* send to the peers - source is peer in the message */
    loopHash = peersHash;
    peerLoc = source_peerid;
    while (TRUE) {
        replicaLocs = jxta_hashtable_keys_get(loopHash);
        replicaLocsSave = (char **) replicaLocs;
        while (NULL != replicaLocs && *replicaLocs) {
            Jxta_status sstat;
            Jxta_peer *sendPeer;
            Jxta_vector *peersVector = NULL;

            sstat = jxta_hashtable_get(loopHash, *replicaLocs, strlen(*replicaLocs) + 1, JXTA_OBJECT_PPTR(&peersVector));
            if (JXTA_SUCCESS != sstat) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error getting entry from peers hash key:%s \n", *replicaLocs);
                free(*replicaLocs++);
                continue;
            }
            sstat = jxta_vector_remove_object_at(peersVector, JXTA_OBJECT_PPTR(&sendPeer), 0);
            if (JXTA_SUCCESS == sstat && jxta_vector_size(peersVector) > 0) {
                sstat = forwardSrdiEntries(self, resolver, sendPeer, peerLoc, peersVector, 1, queueName, jPkey);
                if (JXTA_SUCCESS != sstat) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error forwarding to %s \n", *replicaLocs);
                }
            }
            JXTA_OBJECT_RELEASE(peersVector);
            JXTA_OBJECT_RELEASE(sendPeer);
            free(*replicaLocs++);
        }
        if (NULL != replicaLocsSave)
            free(replicaLocsSave);

        if (end) break;

        /* send to duplicate peers - source of message is peer in the message */
        loopHash = dupsHash;
        peerLoc = from_peerid;
        end = TRUE;
    }
    JXTA_OBJECT_RELEASE(allEntries);
    if (peersHash)
        JXTA_OBJECT_RELEASE(peersHash);
    if (adsHash)
        JXTA_OBJECT_RELEASE(adsHash);
    if (dupsHash)
        JXTA_OBJECT_RELEASE(dupsHash);

  FINAL_EXIT:
    if (from_peerid)
        JXTA_OBJECT_RELEASE(from_peerid); 
    if (source_peerid)
        JXTA_OBJECT_RELEASE(source_peerid);
    if (jPeerId)
        JXTA_OBJECT_RELEASE(jPeerId);
    if (jSrcPeerid)
        JXTA_OBJECT_RELEASE(jSrcPeerid);
    if (jPkey)
        JXTA_OBJECT_RELEASE(jPkey);
    if (JXTA_SUCCESS == status && busy) {
        status = JXTA_BUSY;
    }
    return status;
}

static Jxta_status split_srdi_message(Jxta_srdi_service_ref *self, Jxta_SRDIMessage *srdiMsg
                            , Jxta_vector *msgs, apr_int64_t max_size, Jxta_time timestamp
                            , Jxta_boolean double_encode, apr_int64_t *required)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_vector *entries=NULL;
    Jxta_hashtable *ads_hash=NULL;
    Jxta_SRDIMessage *msg_clone=NULL;
    Jxta_SRDIMessage *working_msg=NULL;
    Jxta_vector *all_msgs=NULL;
    JString *msg_xml;
    int msg_length;
    char **keys;
    char **keys_save;
    apr_int32_t total_length=0;
    Jxta_vector *all_entries=NULL;
    Jxta_boolean init=TRUE;
    Jxta_boolean override=FALSE;

    jxta_srdi_message_get_entries(srdiMsg, &entries);

    jxta_srdi_message_clone(srdiMsg, &msg_clone, FALSE);
    jxta_srdi_message_get_xml(msg_clone, &msg_xml);
    msg_length = jstring_length(msg_xml) + 1;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Start splitting srdi [%pp] max_size:%ld \n",srdiMsg, max_size);

    if (entries == NULL) {
        goto FINAL_EXIT;
    }

    all_msgs = jxta_vector_new(0);
    all_entries = jxta_vector_new(0);

    jxta_srdi_message_get_advids(srdiMsg, &ads_hash);
    if (NULL == ads_hash) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve an ads hash from the srdi msg [%pp] \n",srdiMsg);
        goto FINAL_EXIT;
    }

    keys = jxta_hashtable_keys_get(ads_hash);
    keys_save = keys;
    total_length = msg_length;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Message length total:%ld\n", total_length, jstring_get_string(msg_xml));
    jxta_srdi_message_clone(msg_clone, &working_msg, FALSE);
    while (*keys) {
        Jxta_vector *advs_v=NULL;
        JString *entries_j=NULL;
        JString *tmp_j=NULL;
        JString *tmp_j_1=NULL;

        if (JXTA_SUCCESS != jxta_hashtable_get(ads_hash, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&advs_v))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not retrieve ads_hash with key:%s\n", *keys);
            free(*(keys++));
            continue;
        }

        entries_j = jstring_new_0();
        jxta_srdi_entries_print_xml(advs_v, entries_j, FALSE);

        jxta_xml_util_encode_jstring(entries_j, &tmp_j);
        if (double_encode) {
            jxta_xml_util_encode_jstring(tmp_j, &tmp_j_1);
        }
        total_length += jstring_length(NULL != tmp_j_1 ? tmp_j_1:tmp_j);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "key:%s length of entries %d encoded:%d total:%ld\n"
                        , *keys, jstring_length(entries_j),jstring_length(tmp_j),  total_length);

        if (total_length > max_size && !init) {
            jxta_srdi_message_set_entries(working_msg, all_entries);
            jxta_vector_add_object_last(msgs, (Jxta_object *) working_msg);
            JXTA_OBJECT_RELEASE(working_msg);
            JXTA_OBJECT_RELEASE(all_entries);
            all_entries = jxta_vector_new(0);
            total_length = jstring_length(tmp_j);
            total_length += msg_length;
            jxta_srdi_message_clone(msg_clone, &working_msg, FALSE);
            jxta_vector_addall_objects_first(all_entries, advs_v);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Clearing all objects v_size:%d\n", jxta_vector_size(all_entries));
        } else if (total_length <= max_size) {
            jxta_vector_addall_objects_first(all_entries, advs_v);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"Add all objects vector size:%d total:%ld\n", jxta_vector_size(all_entries), total_length);
            init=FALSE;
        } else {
            res = JXTA_LENGTH_EXCEEDED;
            if (!override) {
                *required = total_length;
                jxta_srdi_message_set_entries(working_msg, advs_v);
                jxta_vector_add_object_last(msgs, (Jxta_object *) working_msg);
                JXTA_OBJECT_RELEASE(working_msg);
                jxta_srdi_message_clone(msg_clone, &working_msg, FALSE);
                total_length = jstring_length(tmp_j);
                override = TRUE;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"We're starting to override\n");
            } else {
                jxta_vector_addall_objects_first(all_entries, advs_v);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID,"We're continuing to override\n");
            }
            init = TRUE;
        }
        if (tmp_j_1)
            JXTA_OBJECT_RELEASE(tmp_j_1);
        JXTA_OBJECT_RELEASE(tmp_j);
        JXTA_OBJECT_RELEASE(entries_j);
        JXTA_OBJECT_RELEASE(advs_v);
        free(*(keys++));
    }
    while (*keys) free (*(keys++));
    free(keys_save);
    if (working_msg) {
        if (jxta_vector_size(all_entries) > 0) {
            jxta_srdi_message_set_entries(working_msg, all_entries);
            jxta_vector_add_object_last(msgs, (Jxta_object *) working_msg);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,"Adding %d all_entries to the working msg [%pp]\n", jxta_vector_size(all_entries), working_msg);
            res = JXTA_SUCCESS;
        }
        JXTA_OBJECT_RELEASE(working_msg);
    }

FINAL_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Done splitting srdi message [%pp]\n", srdiMsg);

    if (all_entries)
        JXTA_OBJECT_RELEASE(all_entries);
    if (all_msgs)
        JXTA_OBJECT_RELEASE(all_msgs);
    if (msg_xml)
        JXTA_OBJECT_RELEASE(msg_xml);
    if (msg_clone)
        JXTA_OBJECT_RELEASE(msg_clone);
    if (ads_hash)
        JXTA_OBJECT_RELEASE(ads_hash);
    if (entries)
        JXTA_OBJECT_RELEASE(entries);

    return res;
}

static Jxta_status create_msg_from_srdi(Jxta_SRDIMessage *srdi_msg, JString **srdi_msg_xml, Jxta_message **msg)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *el_name=NULL;
    Jxta_message_element *el;

    *msg = NULL;
    jxta_advertisement_get_xml((Jxta_advertisement *) srdi_msg, srdi_msg_xml);
    el_name = jstring_new_2(JXTA_SRDI_NS_NAME);
    jstring_append_2(el_name, ":");
    jstring_append_2(el_name, JXTA_SRDI_MESSAGE);

#ifndef GZIP_ENABLED
    el = jxta_message_element_new_1(jstring_get_string(el_name), "text/xml"
                        , jstring_get_string(*srdi_msg_xml), jstring_length(*srdi_msg_xml), NULL);
#else
    res = jxta_xml_util_compress((unsigned char *) jstring_get_string(*srdi_msg_xml), el_name, &el);
#endif

    if (res == JXTA_SUCCESS) {
        *msg = jxta_message_new();

        JXTA_OBJECT_CHECK_VALID(*msg);
        jxta_message_add_element(*msg, el);
    }
    if (el_name)
        JXTA_OBJECT_RELEASE(el_name);
    if (el)
        JXTA_OBJECT_RELEASE(el);
    return res;
}
static void *APR_THREAD_FUNC push_srdi_msg_thread(apr_thread_t * thread, void *arg)
{
    Srdi_msg_thread_struct * t_ptr = NULL;
    Jxta_srdi_service_ref *srdi = NULL;
    Jxta_status res = JXTA_FAILED;
    Jxta_boolean locked = FALSE;

    srdi = (Jxta_srdi_service_ref *) arg;

    jxta_service_lock((Jxta_service *)srdi);
    locked = TRUE;
    if (!srdi->running) {
        goto FINAL_EXIT;
    }

    if (srdi->srdi_msgs_queue != NULL) {
        res = jxta_vector_remove_object_at(srdi->srdi_msgs_queue, JXTA_OBJECT_PPTR(&t_ptr), 0);
        if (res != JXTA_SUCCESS || NULL == t_ptr) {
            goto FINAL_EXIT;
        }
    }
    else {
        goto FINAL_EXIT;
    }

    jxta_service_unlock((Jxta_service *) srdi);
    locked = FALSE;

    if (t_ptr->srdi_service->running) {
        srdi_push_srdi_msg(t_ptr->srdi_service, t_ptr->instance, t_ptr->msg, t_ptr->dest, t_ptr->sync);
    }

FINAL_EXIT:

    if (locked) {
        jxta_service_unlock((Jxta_service *) srdi);
    }

    if (NULL != t_ptr) JXTA_OBJECT_RELEASE(t_ptr);
    
    return NULL;
}

static Jxta_status return_func(Jxta_service *service, Jxta_endpoint_return_parms * return_parms, void *arg, Jxta_vector **ret_v, Jxta_endpoint_service_action action)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_SRDIMessage *srdi_msg=NULL;
    Jxta_message *msg = NULL;

    jxta_endpoint_return_parms_get_msg(return_parms, &msg);
    if (NULL == msg)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to extract Jxta message from return parms [%pp]\n", return_parms);
        goto FINAL_EXIT;
    }

    res = get_srdi_msg_from_msg(msg, NULL, &srdi_msg);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to find SRDI message in msg: [%pp] res:%ld\n", msg, res);
        goto FINAL_EXIT;
    }

    switch (action) {
        case JXTA_EP_ACTION_REDUCE:
        {
            Jxta_vector *new_srdi_msgs=NULL;
            int i;
            apr_int64_t required;

            *ret_v = jxta_vector_new(0);

            res = split_srdi_message ((Jxta_srdi_service_ref *) service, srdi_msg, *ret_v, jxta_message_max_size(msg), jxta_message_timestamp(msg), TRUE, &required);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }

            for (i = 0; i < jxta_vector_size(new_srdi_msgs); i++) {
                Jxta_message *new_msg=NULL;
                JString *srdi_msg_xml=NULL;
                Jxta_SRDIMessage *srdi=NULL;

                jxta_vector_get_object_at(new_srdi_msgs, JXTA_OBJECT_PPTR(&srdi), i);

                res = create_msg_from_srdi(srdi, &srdi_msg_xml, &new_msg);

                if (JXTA_SUCCESS != res) {
                    JXTA_OBJECT_RELEASE(srdi);
                    continue;
                }
                jxta_vector_add_object_last(*ret_v, (Jxta_object *) new_msg);

                JXTA_OBJECT_RELEASE(srdi_msg_xml);
                JXTA_OBJECT_RELEASE(new_msg);
                JXTA_OBJECT_RELEASE(srdi);
            }

            JXTA_OBJECT_RELEASE(new_srdi_msgs);
        }

            break;
        case JXTA_EP_ACTION_FILTER:
            break;
    };

FINAL_EXIT:

    if (srdi_msg)
        JXTA_OBJECT_RELEASE(srdi_msg);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    return res;
}

static Jxta_status create_queue_entries(Jxta_vector *filter_list, JString *groupid_j, Jxta_vector **queued_entries)
{
    int i;
    *queued_entries = jxta_vector_new(0);
    for (i=0; i<jxta_vector_size(filter_list); i++) {
        Jxta_endpoint_filter_entry *f_entry=NULL;
        Jxta_message *orig_msg;
        Jxta_vector *f_ep_entries_v;
        int j;
        Jxta_endpoint_return_parms *f_parms;
        SRDI_parm_arg *f_srdi_arg;

        jxta_vector_get_object_at(filter_list, JXTA_OBJECT_PPTR(&f_entry), i);
        jxta_endpoint_filter_entry_get_orig_msg(f_entry, &orig_msg);

        if (TRUE == jxta_message_remove(orig_msg)) {
            JXTA_OBJECT_RELEASE(orig_msg);
            JXTA_OBJECT_RELEASE(f_entry);
            continue;
        }
        jxta_endpoint_filter_entry_get_parms(f_entry, &f_parms);
        if (NULL == f_parms) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Found filter entry [%pp] without return parms \n", f_entry);
            JXTA_OBJECT_RELEASE(orig_msg);
            JXTA_OBJECT_RELEASE(f_entry);
            continue;
        }
        jxta_endpoint_return_parms_get_arg(f_parms, JXTA_OBJECT_PPTR(&f_srdi_arg));
        if (NULL == f_srdi_arg) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No SRDI arg in filter parms [%pp] \n", f_parms);
            JXTA_OBJECT_RELEASE(orig_msg);
            JXTA_OBJECT_RELEASE(f_entry);
            JXTA_OBJECT_RELEASE(f_parms);
            continue;

        }
        jxta_endpoint_msg_get_entries((Jxta_endpoint_message *) f_srdi_arg->msg, &f_ep_entries_v);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Start processing ep entries %d for groupid %s\n", jxta_vector_size(f_ep_entries_v), jstring_get_string(groupid_j));

        for (j=0; j<jxta_vector_size(f_ep_entries_v); j++) {
            Jxta_SRDIMessage *new_srdi_msg=NULL;
            Jxta_endpoint_msg_entry_element *ep_elem=NULL;
            Jxta_bytevector *ep_elem_bv=NULL;
            SRDI_queue_entry *q_entry=NULL;
            JString *ep_groupid_j;

            jxta_vector_get_object_at(f_ep_entries_v, JXTA_OBJECT_PPTR(&ep_elem), j);
            jxta_endpoint_msg_entry_get_attribute(ep_elem, "groupid", &ep_groupid_j);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Get the srdi msg from the ep elem and groupid:%s\n", jstring_get_string(ep_groupid_j));

            
            if (0 != jstring_equals(groupid_j, ep_groupid_j)) {
                JXTA_OBJECT_RELEASE(ep_elem);
                JXTA_OBJECT_RELEASE(ep_groupid_j);
                continue;
            }
            if (JXTA_SUCCESS != get_srdi_msg_from_ep_elem(ep_elem, &ep_elem_bv, &new_srdi_msg)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to get SRDI message from ep_elem [%pp]\n", ep_elem);
                JXTA_OBJECT_RELEASE(ep_elem);
                if (new_srdi_msg)
                    JXTA_OBJECT_RELEASE(new_srdi_msg);
                if (ep_elem_bv)
                    JXTA_OBJECT_RELEASE(ep_elem_bv);
                if (ep_groupid_j)
                    JXTA_OBJECT_RELEASE(ep_groupid_j);
                continue;
            }
            q_entry = srdi_q_entry_new(f_entry);
            q_entry->srdi_msg = new_srdi_msg;
            q_entry->ep_elem = ep_elem;
            q_entry->ep_elem_bv = ep_elem_bv;
            jxta_vector_add_object_last(*queued_entries, (Jxta_object *) q_entry);

            JXTA_OBJECT_RELEASE(q_entry);
            JXTA_OBJECT_RELEASE(ep_groupid_j);
        }
        JXTA_OBJECT_RELEASE(orig_msg);
        JXTA_OBJECT_RELEASE(f_entry);
        JXTA_OBJECT_RELEASE(f_ep_entries_v);
        JXTA_OBJECT_RELEASE(f_parms);
        JXTA_OBJECT_RELEASE(f_srdi_arg);
    }
    return JXTA_SUCCESS;
}

static void tag_removed_messages(Jxta_vector *match_entries, Jxta_vector *queued_entries)
{
    int i,j;

    for (i=0; i<jxta_vector_size(match_entries); i++) {
        Jxta_endpoint_filter_entry *f_entry;
        Jxta_boolean match=FALSE;

        jxta_vector_get_object_at(match_entries, JXTA_OBJECT_PPTR(&f_entry), i);
        for (j=0; j<jxta_vector_size(queued_entries); j++) {
            SRDI_queue_entry *q_entry;

            jxta_vector_get_object_at(queued_entries, JXTA_OBJECT_PPTR(&q_entry), j);
            if (q_entry->f_entry == f_entry) {
                match = TRUE;
            }
            JXTA_OBJECT_RELEASE(q_entry);
            if (match) break;
        }
        if (!match) {
            Jxta_message *msg;

            jxta_endpoint_filter_entry_get_orig_msg(f_entry, &msg);
            jxta_message_set_remove(msg, TRUE);

            JXTA_OBJECT_RELEASE(msg);
        }
        JXTA_OBJECT_RELEASE(f_entry);
    }
}

Jxta_status return_ep_msg_func(Jxta_service *service, Jxta_endpoint_return_parms *ret_parms, Jxta_vector *filter_list, Jxta_vector **ret_v, Jxta_endpoint_service_action action)
{
    Jxta_status res = JXTA_SUCCESS;
    SRDI_parm_arg *srdi_arg=NULL;

    jxta_endpoint_return_parms_get_arg(ret_parms, JXTA_OBJECT_PPTR(&srdi_arg));

    switch (action) {
        case JXTA_EP_ACTION_REDUCE:
        {
            Jxta_vector *new_entries;
            int i;
            apr_int64_t required;

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "JXTA_EP_ACTION_REDUCE - SRDI\n");
            res = split_endpoint_message ((Jxta_srdi_service *) service, (Jxta_endpoint_message *) srdi_arg->msg, &new_entries, jxta_endpoint_return_parms_max_length(ret_parms), &required);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to reduce msg [%pp] - May bypass traffic shaping\n", srdi_arg->msg);
            }
            *ret_v = jxta_vector_new(0);
            for (i =0; i<jxta_vector_size(new_entries); i++) {
                Jxta_message *new_msg;
                Jxta_endpoint_message *new_ep_msg=NULL;
                Jxta_endpoint_return_parms *new_return_parms=NULL;
                SRDI_parm_arg *parm_arg=NULL;
                Jxta_endpoint_address *dest=NULL;
                Jxta_message_element *msg_elem=NULL;
                JString *ep_msg_xml=NULL;
                Jxta_endpoint_msg_entry_element *new_elem;
                JString *msg_j;
                Jxta_endpoint_filter_entry *new_f_entry=NULL;
 
                jxta_vector_get_object_at(new_entries, JXTA_OBJECT_PPTR(&new_elem), i);
                new_ep_msg = jxta_endpoint_msg_new();
                jxta_endpoint_msg_set_priority(new_ep_msg, MSG_NORMAL_FLOW);
                jxta_endpoint_msg_add_entry(new_ep_msg, new_elem);
                jxta_endpoint_msg_set_timestamp(new_ep_msg, jpr_time_now());
                jxta_endpoint_msg_get_xml(new_ep_msg, FALSE, &msg_j, TRUE);
                peergroup_dest_addr_get(srdi_arg->peer_id, srdi_arg->ep_variable, jstring_get_string(srdi_arg->svc_name), srdi_arg->svc_param, &dest);

                new_msg = jxta_message_new();

                jxta_message_set_destination(new_msg, dest);
                jxta_endpoint_msg_get_xml(new_ep_msg, TRUE, &ep_msg_xml, TRUE);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Add ep msg [%pp] length:%d to new entries\n", new_ep_msg, jstring_length(ep_msg_xml));

                msg_elem = jxta_message_element_new_2(JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_ENDPOINT_MSG_ELEMENT_NAME, "text/xml", jstring_get_string(ep_msg_xml), jstring_length(ep_msg_xml), NULL);

                jxta_message_add_element(new_msg, msg_elem);
                jxta_message_set_priority(new_msg, jxta_endpoint_msg_priority(new_ep_msg));

                new_f_entry = jxta_endpoint_filter_entry_new(new_msg);

                new_return_parms = jxta_endpoint_return_parms_new();
                jxta_endpoint_return_parms_set_function(new_return_parms, (EndpointReturnFunc) return_ep_msg_func);
                jxta_endpoint_return_parms_set_msg(new_return_parms, new_msg);
                jxta_endpoint_return_parms_set_service(new_return_parms, service);
                jxta_endpoint_return_parms_set_required_length(new_return_parms, required);
                jxta_endpoint_filter_entry_set_parms(new_f_entry, new_return_parms);

                parm_arg = srdi_parm_arg_new();
                parm_arg->msg = (Jxta_message *)(JXTA_OBJECT_SHARE(new_ep_msg));
                parm_arg->peerid_j = jstring_clone(srdi_arg->peerid_j);
                parm_arg->timestamp = jpr_time_now();
                parm_arg->peer_id = JXTA_OBJECT_SHARE(srdi_arg->peer_id);
                parm_arg->ep_variable = "groupid";
                parm_arg->svc_name = JXTA_OBJECT_SHARE(srdi_arg->svc_name);
                parm_arg->svc_param = SRDI_QUEUENAME;
                jxta_endpoint_return_parms_set_arg(new_return_parms, (Jxta_object *) parm_arg);

                jxta_vector_add_object_last(*ret_v, (Jxta_object *) new_return_parms);

                JXTA_OBJECT_RELEASE(new_return_parms);
                JXTA_OBJECT_RELEASE(parm_arg);
                JXTA_OBJECT_RELEASE(new_msg);
                JXTA_OBJECT_RELEASE(new_ep_msg);
                JXTA_OBJECT_RELEASE(dest);
                JXTA_OBJECT_RELEASE(msg_elem);
                JXTA_OBJECT_RELEASE(ep_msg_xml);
                JXTA_OBJECT_RELEASE(new_elem);
                JXTA_OBJECT_RELEASE(msg_j);
                JXTA_OBJECT_RELEASE(new_f_entry);
            }
            JXTA_OBJECT_RELEASE(new_entries);
            break;
        } /* end case */
        case JXTA_EP_ACTION_FILTER:
        {
            int i;
            /* Jxta_vector *filter_list=NULL; */
            Jxta_endpoint_message *ep_msg=NULL;
            Jxta_vector *ep_entries_v=NULL;
            SRDI_parm_arg *parm_arg=NULL;

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "JXTA_EP_ACTION_FILTER - SRDI ret_parms:[%pp]\n", ret_parms);

            jxta_endpoint_return_parms_get_arg(ret_parms, JXTA_OBJECT_PPTR(&parm_arg));
            ep_msg = (Jxta_endpoint_message *) parm_arg->msg;
            if (NULL == ep_msg) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "What's going on? no ep_msg?\n");
                return JXTA_FAILED;
            }
            jxta_endpoint_msg_get_entries(ep_msg, &ep_entries_v);

            for (i=0; i<jxta_vector_size(ep_entries_v); i++) {
                int j;
                Jxta_SRDIMessage *new_srdi_msg=NULL;
                Jxta_endpoint_msg_entry_element *ep_elem=NULL;
                Jxta_bytevector *ep_elem_bv=NULL;
                JString *groupid_j=NULL;
                Jxta_vector *queued_entries=NULL;
                Jxta_vector *match_entries=NULL;

                jxta_vector_get_object_at(ep_entries_v, JXTA_OBJECT_PPTR(&ep_elem), i);
                if (JXTA_SUCCESS != get_srdi_msg_from_ep_elem(ep_elem, &ep_elem_bv, &new_srdi_msg)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to get SRDI message from ep_elem [%pp]\n", ep_elem);
                    JXTA_OBJECT_RELEASE(ep_elem);
                    if (new_srdi_msg)
                        JXTA_OBJECT_RELEASE(new_srdi_msg);
                    if (ep_elem_bv)
                        JXTA_OBJECT_RELEASE(ep_elem_bv);
                    continue;
                }
                jxta_endpoint_msg_entry_get_attribute(ep_elem, "groupid", &groupid_j);
                res = create_queue_entries(filter_list, groupid_j, &queued_entries);
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to create q entries from the filter list [%pp] res:%d\n", filter_list, res);
                    JXTA_OBJECT_RELEASE(groupid_j);
                    goto FINAL_EXIT;
                }
                match_entries = jxta_vector_new(0);
                for (j=0; j<jxta_vector_size(queued_entries); j++) {
                    SRDI_queue_entry *q_entry;
                    Jxta_endpoint_filter_entry *match_f_entry;

                    jxta_vector_get_object_at(queued_entries, JXTA_OBJECT_PPTR(&q_entry), j);
                    match_f_entry = q_entry->f_entry;
                    jxta_vector_add_object_last(match_entries, (Jxta_object *) match_f_entry);

                    JXTA_OBJECT_RELEASE(q_entry);
                }

                if (TRUE == adjust_srdi_entries ((Jxta_srdi_service *) jxta_endpoint_return_parms_service(ret_parms) , (Jxta_SRDIMessage *) new_srdi_msg, queued_entries, NULL, parm_arg->prev_diff, TRUE)) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Message should have been deleted\n");
                }
                parm_arg->prev_diff = jpr_time_now();
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%d ep msg entries have been adjusted\n", jxta_vector_size(queued_entries));

                tag_removed_messages(match_entries, queued_entries);

                JXTA_OBJECT_RELEASE(new_srdi_msg);
                JXTA_OBJECT_RELEASE(ep_elem);
                JXTA_OBJECT_RELEASE(ep_elem_bv);
                JXTA_OBJECT_RELEASE(queued_entries);
                JXTA_OBJECT_RELEASE(match_entries);
                if (groupid_j)
                    JXTA_OBJECT_RELEASE(groupid_j);
            }
            JXTA_OBJECT_RELEASE(ep_entries_v);
            JXTA_OBJECT_RELEASE(parm_arg);
            break;
        } /* end case */
    }

FINAL_EXIT:

    if (srdi_arg)
        JXTA_OBJECT_RELEASE(srdi_arg);
    return res;
}

static void srdi_push_srdi_msg(Jxta_srdi_service_ref * self, JString * instance,
                            Jxta_message * msg, Jxta_id * peerid, Jxta_boolean sync)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_endpoint_return_parms *return_parms;
    JString *peerid_j;
    SRDI_parm_arg *parm_arg;

    jxta_id_to_jstring(peerid, &peerid_j);

    return_parms = jxta_endpoint_return_parms_new();
    jxta_endpoint_return_parms_set_function(return_parms, (EndpointReturnFunc) return_func);
    jxta_endpoint_return_parms_set_msg(return_parms, msg);
    jxta_endpoint_return_parms_set_service(return_parms, (Jxta_service *) self);
    parm_arg = srdi_parm_arg_new();
    parm_arg->msg = JXTA_OBJECT_SHARE(msg);
    parm_arg->peerid_j = JXTA_OBJECT_SHARE(peerid_j);
    parm_arg->timestamp = jpr_time_now();
    parm_arg->peer_id = JXTA_OBJECT_SHARE(peerid);

    res = jxta_PG_async_send(self->group, msg, peerid
                    , jstring_get_string(instance), SRDI_QUEUENAME,  return_parms);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to send msg [%pp] res:%d \n", msg, res);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "message sent [%pp] \n", msg);
    }

    JXTA_OBJECT_RELEASE(peerid_j);
    JXTA_OBJECT_RELEASE(return_parms);
    JXTA_OBJECT_RELEASE(parm_arg);
}

static void srdi_msg_thread_free(Jxta_object *obj)
{
    Srdi_msg_thread_struct *t_ptr = (Srdi_msg_thread_struct*)obj;

    JXTA_OBJECT_RELEASE(t_ptr->instance);
    JXTA_OBJECT_RELEASE(t_ptr->dest);
    JXTA_OBJECT_RELEASE(t_ptr->msg);
    
    free(t_ptr);
}
static Jxta_status pushSrdi_msg_priv(Jxta_srdi_service_ref * self, JString * instance,
                            Jxta_SRDIMessage * srdi, Jxta_id * peerid, Jxta_boolean send_sync, Jxta_hashtable **ret_msgs)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg=NULL;
    JString *srdi_msg_xml=NULL;

    res = create_msg_from_srdi(srdi, &srdi_msg_xml, &msg);
    if (JXTA_SUCCESS != res) {
        return res;
    }
    jxta_message_set_priority(msg, MSG_NORMAL_FLOW);

    if (NULL != ret_msgs) {
        const char *peerid_c=NULL;
        JString *peerid_j=NULL;
        Jxta_hashtable *groupid_hash=NULL;
        Jxta_vector *msg_vec=NULL;

        if (NULL == *ret_msgs) {
            *ret_msgs = jxta_hashtable_new(0);
        }
        if (NULL == peerid) {
            peerid_c = "NULL";
        } else {
            jxta_id_to_jstring(peerid, &peerid_j);
            peerid_c = jstring_get_string(peerid_j);
        }
        if (jxta_hashtable_get(*ret_msgs, peerid_c, strlen(peerid_c) + 1, JXTA_OBJECT_PPTR(&groupid_hash))) {
            groupid_hash = jxta_hashtable_new(0);
            jxta_hashtable_put(*ret_msgs, peerid_c, strlen(peerid_c) + 1, (Jxta_object *)groupid_hash);
        }
        if (jxta_hashtable_get(groupid_hash, self->groupid_c, strlen(self->groupid_c) + 1, JXTA_OBJECT_PPTR(&msg_vec))) {
            msg_vec = jxta_vector_new(0);
            jxta_hashtable_put(groupid_hash, self->groupid_c, strlen(self->groupid_c) + 1, (Jxta_object *)msg_vec);
        }
        jxta_vector_add_object_last(msg_vec, (Jxta_object *) srdi_msg_xml);

        JXTA_OBJECT_RELEASE(msg_vec);
        JXTA_OBJECT_RELEASE(groupid_hash);
        JXTA_OBJECT_RELEASE(peerid_j);

    } else if (peerid == NULL) {
        jxta_rdv_service_walk(self->rendezvous, msg, jstring_get_string(instance), SRDI_QUEUENAME);
    } else {
        apr_thread_pool_t *tp;

        tp = jxta_PG_thread_pool_get(self->group);

        if (NULL != tp && !send_sync) {
            Srdi_msg_thread_struct *thread_ptr;

            thread_ptr = calloc(1, sizeof(Srdi_msg_thread_struct));
            JXTA_OBJECT_INIT(thread_ptr, srdi_msg_thread_free, (void*)NULL);
            thread_ptr->srdi_service = self;
            thread_ptr->instance=JXTA_OBJECT_SHARE(instance);
            thread_ptr->dest=JXTA_OBJECT_SHARE(peerid);
            thread_ptr->sync = send_sync;
            /* thread_ptr->msg=JXTA_OBJECT_SHARE(msg); */
            thread_ptr->msg = jxta_message_clone(msg);
                
            jxta_service_lock((Jxta_service*) self);
            if (self->running) {
                jxta_vector_add_object_last(self->srdi_msgs_queue, (Jxta_object*)thread_ptr);
                apr_thread_pool_push(tp, push_srdi_msg_thread, self, APR_THREAD_TASK_PRIORITY_HIGHEST, self);
            }
            jxta_service_unlock((Jxta_service*) self);

            JXTA_OBJECT_RELEASE(thread_ptr);

        } else if (send_sync) {
            res = jxta_PG_sync_send(self->group, msg, peerid
                        , jstring_get_string(instance), SRDI_QUEUENAME, NULL);
        }
    }
#ifdef GZIP_ENABLED
FINAL_EXIT:
#endif

    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    if (srdi_msg_xml)
        JXTA_OBJECT_RELEASE(srdi_msg_xml);

    return res;
}

static Jxta_boolean create_zero_exp_msgs(JString *remove_peer, Jxta_SRDIEntryElement *entry, Jxta_hashtable *remove_entries, const char *seq_no)
{

    Jxta_vector *entry_v = NULL;
    int j;
    Jxta_SRDIEntryElement *zero_entry = NULL;
    Jxta_boolean found = FALSE;

    zero_entry = jxta_srdi_new_element();
    zero_entry->expiration = 0;
    zero_entry->replicate = entry->replicate;
    zero_entry->duplicate = entry->duplicate;
    jxta_srdi_element_set_advid(zero_entry, entry->advId);

    if (jxta_hashtable_get(remove_entries, jstring_get_string(remove_peer), jstring_length(remove_peer) + 1, JXTA_OBJECT_PPTR(&entry_v)) != JXTA_SUCCESS) {
        entry_v = jxta_vector_new(0);
        jxta_vector_add_object_last(entry_v, (Jxta_object *) zero_entry);
        jxta_hashtable_put(remove_entries, jstring_get_string(remove_peer), jstring_length(remove_peer) + 1, (Jxta_object *) entry_v);
    }
    for (j=0; j<jxta_vector_size(entry_v); j++) {
        Jxta_SRDIEntryElement *save_entry = NULL;
        jxta_vector_get_object_at(entry_v, JXTA_OBJECT_PPTR(&save_entry), j);
        if (!strcmp(jstring_get_string(save_entry->advId), jstring_get_string(entry->advId))) {
            if (NULL == save_entry->sn_cs_values) {
                save_entry->sn_cs_values = jstring_new_0();
            } else {
                jstring_append_2(save_entry->sn_cs_values, ",");
            }
            jstring_append_2(save_entry->sn_cs_values, seq_no);
            found = TRUE;
        }
        JXTA_OBJECT_RELEASE(save_entry);
        if (found) break;
    }
    if (!found) {
        zero_entry->sn_cs_values = jstring_new_2(seq_no);
        jxta_vector_add_object_last(entry_v, (Jxta_object *) zero_entry);
    }
    if (zero_entry)
        JXTA_OBJECT_RELEASE(zero_entry);
    JXTA_OBJECT_RELEASE(entry_v);
    JXTA_OBJECT_RELEASE(remove_peer);
    return found;
}

static Jxta_status record_delta_entry(Jxta_srdi_service_ref *me, Jxta_id * peer, Jxta_id * orig_pid, Jxta_id * src_pid, JString * instance, Jxta_SRDIMessage * msg, Jxta_hashtable **remove_entries, Jxta_vector *xaction_entries)
{
    Jxta_status res = JXTA_SUCCESS;
    int i;
    Jxta_hashtable *seq_hash = NULL;
    Jxta_vector *entries = NULL;
    JString *jPeer=NULL;
    JString *jOrigPeer=NULL;
    JString *jSourcePeer=NULL;
    Jxta_hashtable *ads_hash = NULL;
    Jxta_version * peerVersion = NULL; 
    Jxta_PA * peer_adv = NULL;
    Jxta_boolean update_only=FALSE;

    update_only = jxta_srdi_message_update_only(msg);

    if(jxta_rdv_service_is_rendezvous((Jxta_rdv_service *) me->rendezvous) == TRUE && peer == NULL) {
        return JXTA_SUCCESS;
    }

    if (peer == NULL || orig_pid == NULL || src_pid == NULL )
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid arguments to record_delta_entry\n");
        return JXTA_FAILED;
    }

    if (!isRunning(me)) {
        return JXTA_FAILED;
    }

    jxta_id_to_jstring(peer, &jPeer);

    jxta_id_to_jstring(orig_pid, &jOrigPeer);

    jxta_id_to_jstring(src_pid, &jSourcePeer);

    peergroup_find_peer_PA(me->group, peer, 0, &peer_adv);
    if(NULL == peer_adv){
        peerVersion = jxta_version_new();
    } else {
        peerVersion = jxta_PA_get_version(peer_adv);
        JXTA_OBJECT_RELEASE(peer_adv);
    }

    res = jxta_srdi_message_get_entries(msg, &entries);

    seq_hash = jxta_hashtable_new(0);
    ads_hash = jxta_hashtable_new(0);

    for (i = 0; i < jxta_vector_size(entries); i++) {
        Jxta_status status;
        Jxta_time nnow;
        int j;
        Jxta_sequence_number newSeqNumber = 0;
        Jxta_SRDIEntryElement *entry;
        Jxta_SRDIEntryElement *saveEntry = NULL;
        Jxta_SRDIEntryElement *replace_entry = NULL;
        JString *jNewValue = NULL;
        Jxta_boolean update_srdi = TRUE;
        char aTmp[64];
        Jxta_peer *advPeer=NULL;
        JString * advPeer_j = NULL;
        JString * remove_peer_j=NULL;
        JString * remove_sequence_j = NULL;
        Jxta_boolean within_radius = FALSE;

        if (!isRunning(me)) {
            break;
        }

        nnow = jpr_time_now();
        jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        /* if this is a resend doesn't need to be saved */
        if (entry->resend) {
            JXTA_OBJECT_RELEASE(entry);
            continue;
        }
        /* replace the object with a clone in the vector since it will be modified */
        replace_entry = jxta_srdi_element_clone(entry);
        jxta_vector_replace_object_at(entries, (Jxta_object *) replace_entry, i);
        JXTA_OBJECT_RELEASE(entry);
        entry = replace_entry;


        if (jxta_rdv_service_is_rendezvous(me->rendezvous)) {
            Jxta_peer * alt_dupPeer=NULL;

            advPeer = getReplicaPeer((Jxta_srdi_service *) me, jstring_get_string(entry->advId), &alt_dupPeer);
            if (NULL != advPeer) {
                /* if target is this peer use the alternate peer */
                if (jxta_id_equals(jxta_peer_peerid(advPeer), (Jxta_id *) me->peerID) && NULL != alt_dupPeer) {
                    JString *alt_peer_j = NULL;

                    JXTA_OBJECT_RELEASE(advPeer);
                    advPeer = alt_dupPeer;
                    alt_dupPeer = NULL;
                    jxta_id_to_jstring(jxta_peer_peerid(advPeer), &alt_peer_j);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "RepPeer changed the dup %" APR_INT64_T_FMT " to alt_peer:%s\n"
                                    , entry->seqNumber
                                    ,jstring_get_string(alt_peer_j));
                    JXTA_OBJECT_RELEASE(alt_peer_j);
                }
                if (jxta_id_to_jstring(jxta_peer_peerid(advPeer), &advPeer_j) != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received an invalid replica peer id [%pp]\n",advPeer);
                    JXTA_OBJECT_RELEASE(advPeer);
                    advPeer = NULL;
                }
            }
            if (alt_dupPeer)
                JXTA_OBJECT_RELEASE(alt_dupPeer);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Starting with entry->seqNumber:" JXTA_SEQUENCE_NUMBER_FMT "\n", entry->seqNumber);
        for (j = 0; NULL != entry->radius_peers && j < jxta_vector_size(entry->radius_peers); j++) {
            Jxta_peer * radius_peer=NULL;
            JString * radius_peer_j=NULL;

            if (!isRunning(me)) {
                break;
            }

            status = jxta_vector_get_object_at(entry->radius_peers, JXTA_OBJECT_PPTR(&radius_peer), j);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve radius peer from vector status:%d\n", status);
                continue;
            }
            status = jxta_id_to_jstring(jxta_peer_peerid(radius_peer), &radius_peer_j);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid peer id returned from radius peer entry status:%d\n", status);
                JXTA_OBJECT_RELEASE(radius_peer);
                continue;
            }
            if (!strcmp(jstring_get_string(radius_peer_j), jstring_get_string(jPeer))) {
                within_radius = TRUE;
            }
            JXTA_OBJECT_RELEASE(radius_peer);
            JXTA_OBJECT_RELEASE(radius_peer_j);
            if (within_radius)
                break;
        }
        if (TRUE == update_only) {
            entry->re_replicate = TRUE;
        }
        if (isRunning(me)) {
            res = cm_save_delta_entry(me->cm, jPeer, jSourcePeer, entry->rep_peerid, advPeer_j, instance, entry, within_radius
                        , &jNewValue, &newSeqNumber, &remove_peer_j, &remove_sequence_j
                        , &update_srdi, jxta_srdi_cfg_get_delta_window(me->config), xaction_entries);
        }

        if (newSeqNumber > 0 && entry->seqNumber != newSeqNumber) {
           jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "seq changed old:" JXTA_SEQUENCE_NUMBER_FMT " new: " JXTA_SEQUENCE_NUMBER_FMT "\n", entry->seqNumber, newSeqNumber);
           entry->seqNumber = newSeqNumber;
         }

        if (NULL != remove_peer_j) {
            Jxta_SRDIEntryElement * clone_entry = NULL;
 
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received a remove peer: %s newSeqNumber :" JXTA_SEQUENCE_NUMBER_FMT " entry->seqNumber:"JXTA_SEQUENCE_NUMBER_FMT "\n",jstring_get_string(remove_peer_j), newSeqNumber, entry->seqNumber);

            if (NULL == *remove_entries) {
                *remove_entries = jxta_hashtable_new(0);
            }

            clone_entry = jxta_srdi_element_clone(entry);

            create_zero_exp_msgs(remove_peer_j, clone_entry, *remove_entries, jstring_get_string(remove_sequence_j));
            if (0 == entry->expiration) {
                jxta_vector_remove_object_at(entries, NULL, i--);
            } else {
                entry->seqNumber = newSeqNumber > 0 ? newSeqNumber:entry->seqNumber;
            }
            JXTA_OBJECT_RELEASE(clone_entry);

        } else {

            memset(aTmp, 0, sizeof(aTmp));

            /* if the seq number changed, use it */
            if (newSeqNumber > 0 && entry->seqNumber != newSeqNumber) {
                entry->seqNumber = newSeqNumber;
            }
            apr_snprintf(aTmp, sizeof(aTmp), JXTA_SEQUENCE_NUMBER_FMT, entry->seqNumber);

            status = jxta_hashtable_get(seq_hash, aTmp, strlen(aTmp), JXTA_OBJECT_PPTR(&saveEntry));

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "checking seq %s -- found: %s update?- %s compatible: %s\n"
                    , aTmp
                    , JXTA_ITEM_NOTFOUND == status ? "no":"yes"
                    , update_srdi == TRUE ? "yes":"no"
                    , TRUE == jxta_version_compatible_1(SRDI_DELTA_OPTIMIZATIONS, peerVersion) ? "true":"false");

            if (JXTA_SUCCESS != status && update_srdi && jxta_version_compatible_1(SRDI_DELTA_OPTIMIZATIONS, peerVersion) ) {
                if (jxta_version_compatible_1(SRDI_UPDATE_ONLY, peerVersion)) {
                    entry->update_only = FALSE;
                }
                jxta_hashtable_put(seq_hash, aTmp, strlen(aTmp), (Jxta_object *) entry);
                /* if there was entry in the delta it needs to be updated*/
                if (JXTA_SUCCESS == res || entry->update_only) {
                    Jxta_boolean ad = FALSE;

                    /* if the value changed send the new value */
                    if (NULL != jNewValue) {
                        jstring_reset(entry->value, NULL);
                        jstring_append_1(entry->value, jNewValue);
                        JXTA_OBJECT_RELEASE(jNewValue);
                    } else {
                        Jxta_SRDIEntryElement *ads_entry = NULL;
                        /* don't leak it */
                        JXTA_OBJECT_RELEASE(entry->value);
                        entry->value = NULL;
                        entry->seqNumber = 0;
                        if (isRunning(me)) {
                            cm_update_delta_entry(me->cm, jPeer, jSourcePeer, instance, entry, 0 == entry->next_update_time ? entry->expiration : 0);
                        }
                        /* if this advertisement is being updated remove it from the vector and add cs values*/
                        status = jxta_hashtable_get(ads_hash, jstring_get_string(entry->advId), jstring_length(entry->advId) + 1, JXTA_OBJECT_PPTR(&ads_entry));

                        if (JXTA_SUCCESS == status) {
                            jxta_vector_remove_object_at(entries, NULL, i--);
                            jstring_append_2(ads_entry->sn_cs_values, ",");
                            jstring_append_2(ads_entry->sn_cs_values, aTmp);
                            JXTA_OBJECT_RELEASE(ads_entry);
                        } else {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "expiration:%" APR_INT64_T_FMT " timeout:%" APR_INT64_T_FMT "\n", entry->expiration, entry->timeout);
                            if (entry->expiration > 0) {
                                entry->expiration = entry->timeout - nnow;
                            }
                            entry->seqNumber = 0;
                            entry->sn_cs_values = jstring_new_2(aTmp);
                            JXTA_OBJECT_RELEASE(entry->range);
                            entry->range = NULL;
                            jxta_hashtable_put(ads_hash, jstring_get_string(entry->advId), jstring_length(entry->advId) + 1, (Jxta_object *) entry);
                            ad = TRUE;
                        }
                    }
                    JXTA_OBJECT_RELEASE(entry->key);
                    entry->key = NULL;
                    if (!ad) {
                        jxta_srdi_element_set_advid(entry, NULL);
                    }
                    JXTA_OBJECT_RELEASE(entry->nameSpace);
                    entry->nameSpace = NULL;
                } else if (0 == entry->expiration) {
                    /* don't try to remove an entry that doesn't exist */
                    jxta_vector_remove_object_at(entries, NULL, i--);
                }
            } else if (JXTA_SUCCESS != status && !jxta_version_compatible_1(SRDI_DELTA_OPTIMIZATIONS, peerVersion)) {
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
                    jxta_srdi_element_set_advid(entry, NULL);

                    JXTA_OBJECT_RELEASE(entry->nameSpace);
                    entry->nameSpace = NULL;
                }
            } else {
                if (isRunning(me)) {
                    cm_update_delta_entry(me->cm, jPeer, jSourcePeer, instance, entry, 0 == entry->next_update_time ? entry->timeout - nnow : entry->next_update_time - nnow);
                }
                jxta_vector_remove_object_at(entries, NULL, i--);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Don't send sequence %s - Removing from entries\n", aTmp);
            }
        }
        if (advPeer)
            JXTA_OBJECT_RELEASE(advPeer);
        if (advPeer_j)
            JXTA_OBJECT_RELEASE(advPeer_j);
        if (remove_sequence_j)
            JXTA_OBJECT_RELEASE(remove_sequence_j);
        if (saveEntry)
            JXTA_OBJECT_RELEASE(saveEntry);
        if (jNewValue)
            JXTA_OBJECT_RELEASE(jNewValue);
        JXTA_OBJECT_RELEASE(entry);
    }

    if(peerVersion)
        JXTA_OBJECT_RELEASE(peerVersion);
 
    if (ads_hash)
        JXTA_OBJECT_RELEASE(ads_hash);
    if (jOrigPeer)
        JXTA_OBJECT_RELEASE(jOrigPeer);
    JXTA_OBJECT_RELEASE(jPeer);
    JXTA_OBJECT_RELEASE(jSourcePeer);
    JXTA_OBJECT_RELEASE(seq_hash);
    JXTA_OBJECT_RELEASE(entries);
    return res;
}

static Jxta_status prep_srdi_updates(Jxta_srdi_service *self, JString *instance, Jxta_SRDIMessage * msg, Jxta_id *peer,
Jxta_hashtable **messages, Jxta_vector *xaction_entries)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_vector *mod_entries = NULL;
    Jxta_hashtable *remove_entries=NULL;
    Jxta_vector *srdi_messages = NULL;
    JString * peer_j = NULL;
    Jxta_id * orig_pid = NULL;
    Jxta_id * src_pid = NULL;
    Jxta_id * prev_pid = NULL;
    char **remove_peers = NULL;
    char **remove_peers_save = NULL;

    if (!isRunning(me)) {
        messages = NULL;
        return JXTA_FAILED;
    }

    jxta_srdi_message_get_peerID(msg, &orig_pid);
    jxta_srdi_message_get_SrcPID(msg, &src_pid);
    jxta_srdi_message_get_PrevPID(msg, &prev_pid);

    if (jxta_srdi_cfg_is_delta_cache_supported(me->config) && NULL != jxta_srdi_message_entries(msg)) {
        record_delta_entry(me, peer, orig_pid, src_pid, instance, msg, &remove_entries, xaction_entries);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got remove entries: %s \n", NULL == remove_entries? "false":"true");
        res = jxta_srdi_message_get_entries(msg, &mod_entries);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Cannot get entries vector from srdi message\n");
            goto FINAL_EXIT;
        }
        /* if no updates now */
        if (0 == jxta_vector_size(mod_entries) && NULL == remove_entries ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No mod_entries or remove entries\n");
            goto FINAL_EXIT;
        }
        if (remove_entries) {
            remove_peers = jxta_hashtable_keys_get(remove_entries);
            remove_peers_save = remove_peers;
        }
    }

    /* the first message to send is to the original destination */
    if (jxta_id_to_jstring(peer, &peer_j) != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Couldn't get a jstring from the peer [%pp]\n", peer);
        goto FINAL_EXIT;
    }

    if (TRUE == jxta_srdi_message_update_only(msg)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "checking update limit " JPR_DIFF_TIME_FMT " against now() " JPR_DIFF_TIME_FMT "\n"
                , me->update_limit, jpr_time_now());
        if (me->update_limit < jpr_time_now()) {
            jxta_srdi_message_set_update_only(msg, FALSE);
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending SRDI message with update_only %s\n"
                        , TRUE == jxta_srdi_message_update_only(msg) ? "yes":"no");
    }
    while (TRUE) {
        Jxta_SRDIMessage *msg_clone=NULL;

        if (NULL == *messages) {
            *messages = jxta_hashtable_new(0);
        }
        if (NULL != peer_j) {
            if (jxta_hashtable_get(*messages, jstring_get_string(peer_j), jstring_length(peer_j) + 1, JXTA_OBJECT_PPTR(&srdi_messages)) != JXTA_SUCCESS) {
                srdi_messages = jxta_vector_new(0);
                jxta_hashtable_put(*messages, jstring_get_string(peer_j), jstring_length(peer_j) + 1, (Jxta_object *) srdi_messages);
            }
            JXTA_OBJECT_RELEASE(peer_j);
            peer_j = NULL;
        }
        jxta_srdi_message_clone(msg, &msg_clone, TRUE);
        if (NULL != mod_entries && jxta_vector_size(mod_entries) > 0) {
            jxta_srdi_message_set_entries(msg_clone, mod_entries);
        }

        jxta_vector_add_object_last(srdi_messages, (Jxta_object *) msg_clone);

        if (srdi_messages) {
            JXTA_OBJECT_RELEASE(srdi_messages);
            srdi_messages = NULL;
        }
        if (mod_entries) {
            JXTA_OBJECT_RELEASE(mod_entries);
            mod_entries = NULL;
        }
        if (msg_clone)
            JXTA_OBJECT_RELEASE(msg_clone);
        if (remove_peers && *remove_peers) {
            jxta_hashtable_get(remove_entries, *remove_peers, strlen(*remove_peers) + 1, JXTA_OBJECT_PPTR(&mod_entries));
            peer_j = jstring_new_2(*remove_peers);
            free(*remove_peers++);
        } else {
            break;
        }
    }

FINAL_EXIT:
    if (remove_peers_save) {
        while (*remove_peers) free(*remove_peers++);
        free(remove_peers_save);
    }
    if (orig_pid)
        JXTA_OBJECT_RELEASE(orig_pid);
    if (src_pid)
        JXTA_OBJECT_RELEASE(src_pid);
    if (prev_pid)
        JXTA_OBJECT_RELEASE(prev_pid);
    if (srdi_messages)
        JXTA_OBJECT_RELEASE(srdi_messages);
    if (remove_entries)
        JXTA_OBJECT_RELEASE(remove_entries);
    if (mod_entries)
        JXTA_OBJECT_RELEASE(mod_entries);
    return res;

}

static Jxta_status pushSrdi_msg_with_update(Jxta_srdi_service * self, JString * instance,
                            Jxta_SRDIMessage * msg, Jxta_id * peer, Jxta_boolean sync, Jxta_hashtable **ret_msgs)

{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_status status=JXTA_SUCCESS;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;
    Jxta_vector *peers = NULL;
    Jxta_hashtable *messages = NULL;
    unsigned int i = 0;
    JString * peer_j=NULL;
    Jxta_vector *xaction_entries;

    if (!isRunning(me)) {
        return JXTA_FAILED;
    }

    xaction_entries = jxta_vector_new(0);

    if (NULL != peer) {
        if (jxta_id_to_jstring(peer, &peer_j) != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received an invalid peer id [%pp]\n", peer);
            goto FINAL_EXIT;
        }
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Push SRDI message to %s\n", peer == NULL ? "NULL": jstring_get_string(peer_j));

    if(peer == NULL) {
        jxta_rdv_service_get_peers((Jxta_rdv_service*) me->rendezvous, &peers);
        for (i=0; peers!= NULL && i < jxta_vector_size(peers); i++) {
            Jxta_peer * peer_obj =  NULL;
            res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer_obj), i);
            if(res == JXTA_SUCCESS) {
                Jxta_id * peerid = NULL;

                jxta_peer_get_peerid(peer_obj, &peerid);
                if(peerid != NULL) {
                    JString * idString = NULL;
                    jxta_id_to_jstring(peerid, &idString);
                    JXTA_OBJECT_RELEASE(idString);

                    res = prep_srdi_updates(self, instance, msg, peerid, &messages, xaction_entries);

                    if(peerid != NULL){
                        JXTA_OBJECT_RELEASE(peerid);
                        peerid = NULL;
                    }
                }
            }
            if(peer_obj != NULL)
                JXTA_OBJECT_RELEASE(peer_obj);
        }
    } else {
        res = prep_srdi_updates(self, instance, msg, peer, &messages, xaction_entries);
    }
    if (NULL != messages) {
        char **srdi_peers = NULL;
        char **srdi_peers_save = NULL;

        srdi_peers = jxta_hashtable_keys_get(messages);
        srdi_peers_save = srdi_peers;
        while (*srdi_peers) {
            Jxta_id *to_peer=NULL;
            Jxta_vector *srdi_v=NULL;

            jxta_hashtable_get(messages, *srdi_peers, strlen(*srdi_peers) + 1, JXTA_OBJECT_PPTR(&srdi_v));
            status = jxta_id_from_cstr(&to_peer, *srdi_peers);
            if (JXTA_SUCCESS == status) {
                unsigned int j;

                for (j=0; j<jxta_vector_size(srdi_v); j++) {
                    Jxta_SRDIMessage *srdi=NULL;

                    if (!isRunning(me)) {
                        break;
                    }

                    jxta_vector_remove_object_at(srdi_v, JXTA_OBJECT_PPTR(&srdi), j--);

                    status = pushSrdi_msg_priv(me, instance, srdi, to_peer, sync, ret_msgs);

                    if (JXTA_SUCCESS != status) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Couldn't send message to %s\n", *srdi_peers);
                    }
                    JXTA_OBJECT_RELEASE(srdi);
                }
                JXTA_OBJECT_RELEASE(to_peer);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create peerid using %s\n", *srdi_peers);
            }
            res = status;
            if (srdi_v)
                JXTA_OBJECT_RELEASE(srdi_v);
            free(*srdi_peers++);
        }
        if (srdi_peers_save) {
            while(*srdi_peers) free(*srdi_peers++);
            free(srdi_peers_save);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No messages from prep_srdi_updates\n");
    }
    if (isRunning(me) && jxta_vector_size(xaction_entries) > 0) {
        cm_exec_prepared_transaction_query(me->cm, xaction_entries);
    }

FINAL_EXIT:
    if (messages)
        JXTA_OBJECT_RELEASE(messages);
    if (peers)
        JXTA_OBJECT_RELEASE(peers);
    if (peer_j)
        JXTA_OBJECT_RELEASE(peer_j);
    if (xaction_entries)
        JXTA_OBJECT_RELEASE(xaction_entries);
    return res;
}

static Jxta_status pushSrdi_msg(Jxta_srdi_service * self, JString * instance
                            , Jxta_SRDIMessage * msg
                            , Jxta_id * peerid, Jxta_boolean update_delta, Jxta_boolean sync, Jxta_hashtable **ret_msgs)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;

    if (!isRunning(me)) {
        return JXTA_FAILED;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                        , "********* push srdi message update delta %s synd %s\n"
                                , TRUE == update_delta ? "true":"false"
                                , TRUE == sync ? "true":"false");

    if (TRUE == update_delta) {
        return pushSrdi_msg_with_update(self, instance, msg, peerid, sync, ret_msgs);
    } else {
        return pushSrdi_msg_priv(me, instance, msg, peerid, sync, ret_msgs);
    }
}

static Jxta_status  remove_advid_entries_from_queued_entries(Jxta_vector *queued_entries, const char *advid)
{
    Jxta_status res=JXTA_SUCCESS;
    int i;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Processing queued entries size %d remove advid:%s\n", jxta_vector_size(queued_entries), advid);

    for (i=0; i<jxta_vector_size(queued_entries); i++) {
        SRDI_queue_entry *q_entry;
        Jxta_vector * srdi_msg_entries=NULL;

        jxta_vector_get_object_at(queued_entries, JXTA_OBJECT_PPTR(&q_entry), i);

        jxta_srdi_message_remove_advid_entries(q_entry->srdi_msg, advid);

        res = jxta_srdi_message_get_entries(q_entry->srdi_msg, &srdi_msg_entries);
        if (JXTA_SUCCESS != res) {
            JXTA_OBJECT_RELEASE(q_entry);
            continue;
        }

        if (jxta_vector_size(srdi_msg_entries) == 0) {
            Jxta_message *msg;

            jxta_endpoint_filter_entry_get_orig_msg(q_entry->f_entry, &msg);
            jxta_message_set_remove(msg, TRUE);
            jxta_vector_remove_object_at(queued_entries, NULL, i--);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove q_entry->f_entry [%pp] \n",q_entry->f_entry);
            JXTA_OBJECT_RELEASE(msg);
        } else if (i == (jxta_vector_size(queued_entries) - 1)) {
            JString *srdi_msg_xml=NULL;

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rebuild the ep element\n");

            /* rebuild the ep entry element */
            jxta_srdi_message_get_xml(q_entry->srdi_msg, &srdi_msg_xml);
            /* clear the entry and replace the value */
            jxta_bytevector_clear(q_entry->ep_elem_bv, jstring_length(srdi_msg_xml));
            jxta_bytevector_add_bytes_at(q_entry->ep_elem_bv
                        , (unsigned char const *) jstring_get_string(srdi_msg_xml)
                        , 0,jstring_length(srdi_msg_xml));

            JXTA_OBJECT_RELEASE(srdi_msg_xml);
        }
        if (srdi_msg_entries)
            JXTA_OBJECT_RELEASE(srdi_msg_entries);
        if (q_entry)
            JXTA_OBJECT_RELEASE(q_entry);
    }
    return res;
}

static Jxta_boolean adjust_srdi_entries(Jxta_srdi_service *service, Jxta_SRDIMessage * srdi_msg
                    , Jxta_vector *queued_entries, Jxta_id *peerid, Jxta_time prev_diff, Jxta_boolean ep_entries)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) service;
    Jxta_boolean res=FALSE;
    int i;
    Jxta_time diff_time;
    Jxta_hashtable *srdi_hash=NULL;
    char **keys;
    char **keys_save;

    jxta_srdi_message_get_advids(srdi_msg, &srdi_hash);

    keys=jxta_hashtable_keys_get(srdi_hash);
    keys_save = keys;
    diff_time = jpr_time_now() - prev_diff;

    while (*keys) {
        Jxta_boolean remove_advid=FALSE;
        Jxta_vector *srdi_entries=NULL;

        if (jxta_hashtable_get(srdi_hash, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&srdi_entries))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve entry from the hash with key:%s\n", *keys);
            free(*(keys++));
            continue;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adjust srdi msg [%pp] with %d srdi entries\n", srdi_msg, jxta_vector_size(srdi_entries));

        for (i=0; i<jxta_vector_size(srdi_entries) && isRunning(me); i++) {
            Jxta_SRDIEntryElement *entry_elem;

            jxta_vector_get_object_at(srdi_entries, JXTA_OBJECT_PPTR(&entry_elem), i);
            if (0 != entry_elem->expiration) {
                entry_elem->expiration -= diff_time; 
                if (entry_elem->expiration < 0) {
                    jxta_vector_remove_object_at(srdi_entries, NULL, i--);
                    res = TRUE;
                }
            } else {
                remove_advid = TRUE;
            }
            JXTA_OBJECT_RELEASE(entry_elem);
        }
        if (remove_advid) {
            remove_advid_entries_from_queued_entries(queued_entries, *keys);
        }
        free(*(keys++));
        if (srdi_entries)
            JXTA_OBJECT_RELEASE(srdi_entries);
    }

    while (*keys) free(*(keys++));
    free(keys_save);

    if (srdi_hash)
        JXTA_OBJECT_RELEASE(srdi_hash);
    return res;
}

static Jxta_status get_srdi_msg_from_msg(Jxta_message *msg
                                    , Jxta_bytevector **msg_elem_bv, Jxta_SRDIMessage **srdi_msg)
{
    Jxta_status res;
    Jxta_message_element *el=NULL;
    JString *msg_entry_j=NULL;
    Jxta_bytevector *b_element=NULL;

    res = jxta_message_get_element_2(msg, (char *) JXTA_SRDI_NS_NAME, (char *) JXTA_SRDI_MESSAGE, &el);
    if (JXTA_SUCCESS != res) goto FINAL_EXIT;

    b_element = jxta_message_element_get_value(el);
    get_element_bytes(el, &msg_entry_j);
    *srdi_msg = jxta_srdi_message_new();
    res = jxta_srdi_message_parse_charbuffer(*srdi_msg
                                , jstring_get_string(msg_entry_j)
                                , jstring_length(msg_entry_j));
    if (NULL != msg_elem_bv)
        *msg_elem_bv = b_element;
    else
        JXTA_OBJECT_RELEASE(b_element);

FINAL_EXIT:
    if (el)
        JXTA_OBJECT_RELEASE(el);
    if (msg_entry_j)
        JXTA_OBJECT_RELEASE(msg_entry_j);

    return res;
}

static Jxta_status get_srdi_msg_from_ep_elem(Jxta_endpoint_msg_entry_element *ep_elem, Jxta_bytevector **ep_elem_bv, Jxta_SRDIMessage **srdi_msg)
{
    Jxta_status res;
    Jxta_message_element *el=NULL;
    JString *ep_msg_entry_j=NULL;
    Jxta_bytevector *b_element=NULL;

    jxta_endpoint_msg_entry_get_value(ep_elem, &el);
    b_element = jxta_message_element_get_value(el);
    get_element_bytes(el, &ep_msg_entry_j);
    *srdi_msg = jxta_srdi_message_new();

    res = jxta_srdi_message_parse_charbuffer(*srdi_msg
                                , jstring_get_string(ep_msg_entry_j)
                                , jstring_length(ep_msg_entry_j));
    *ep_elem_bv = b_element;


    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(ep_msg_entry_j);
    return res;
}

static Jxta_status split_endpoint_message(Jxta_srdi_service *self, Jxta_endpoint_message *ep_msg
                            , Jxta_vector **new_ep_entries, apr_int64_t max_size, apr_int64_t *required)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *entries;
    JString *ep_xml;
    int i;
    Jxta_vector *new_msgs=NULL;
    int ep_msg_length;

    new_msgs = jxta_vector_new(0);

    jxta_endpoint_msg_get_xml(ep_msg, FALSE, &ep_xml, FALSE);
    jxta_endpoint_msg_get_entries(ep_msg, &entries);
    ep_msg_length = jstring_length(ep_xml);
    *new_ep_entries = NULL;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Start splitting endpoint [%pp] max_size:%" APR_INT64_T_FMT "\n",ep_msg, max_size);

    for (i=0; i<jxta_vector_size(entries); i++) {
        Jxta_SRDIMessage *srdi_msg=NULL;
        Jxta_endpoint_msg_entry_element *elem=NULL;
        Jxta_message_element *el=NULL;
        JString *ep_msg_entry_j=NULL;
        Jxta_vector *msgs=NULL;
        int j;
        JString *groupid_j=NULL;

        if (JXTA_SUCCESS != jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&elem), i)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve from [%pp] endpoint entry element at %d\n", entries , i);
            continue;
        }

        jxta_endpoint_msg_entry_get_value(elem, &el);
        jxta_endpoint_msg_entry_get_attribute(elem, "groupid", &groupid_j);

        get_element_bytes(el, &ep_msg_entry_j);

        srdi_msg = jxta_srdi_message_new();

        msgs = jxta_vector_new(0);
        jxta_vector_add_object_first(msgs, (Jxta_object *) srdi_msg);

        res = jxta_srdi_message_parse_charbuffer(srdi_msg
                                , jstring_get_string(ep_msg_entry_j)
                                , jstring_length(ep_msg_entry_j));

        res = split_srdi_message((Jxta_srdi_service_ref *) self, srdi_msg, new_msgs, max_size - ep_msg_length, jxta_endpoint_msg_timestamp(ep_msg), TRUE, required);

        for (j=0; j<jxta_vector_size(new_msgs); j++) {
            Jxta_endpoint_msg_entry_element *ep_element=NULL;
            Jxta_message_element *msg_elem=NULL;
            Jxta_SRDIMessage *new_srdi_msg=NULL;
            JString  *srdi_msg_xml=NULL;
            JString *el_name=NULL;
            Jxta_vector *entries_v = NULL;

            jxta_vector_get_object_at(new_msgs, JXTA_OBJECT_PPTR(&new_srdi_msg), j);
 
            ep_element = jxta_endpoint_message_entry_new();
            jxta_advertisement_get_xml((Jxta_advertisement *) new_srdi_msg, &srdi_msg_xml);
            jxta_srdi_message_get_entries(new_srdi_msg, &entries_v);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "New SRDI msg [%pp] length:%d entries:%d\n"
                                , new_srdi_msg, jstring_length(srdi_msg_xml), jxta_vector_size(entries_v));
            JXTA_OBJECT_RELEASE(entries_v);
            el_name = jstring_new_2(JXTA_SRDI_NS_NAME);
            jstring_append_2(el_name, ":");
            jstring_append_2(el_name, JXTA_SRDI_MESSAGE);

#ifndef GZIP_ENABLED
             msg_elem = jxta_message_element_new_1(jstring_get_string(el_name), "text/xml"
                        , jstring_get_string(srdi_msg_xml), jstring_length(srdi_msg_xml), NULL);
#else
            res = jxta_xml_util_compress((unsigned char *) jstring_get_string(srdi_msg_xml), el_name, &msg_elem);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
#endif
            jxta_endpoint_msg_entry_set_value(ep_element, msg_elem);
            if (NULL != groupid_j) {
                jxta_endpoint_msg_entry_add_attribute(ep_element, "groupid", jstring_get_string(groupid_j));
            }
            if (NULL == *new_ep_entries) {
                *new_ep_entries = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(*new_ep_entries, (Jxta_object *) ep_element);

            JXTA_OBJECT_RELEASE(msg_elem);
            JXTA_OBJECT_RELEASE(ep_element);
            JXTA_OBJECT_RELEASE(el_name);
            JXTA_OBJECT_RELEASE(new_srdi_msg);
            JXTA_OBJECT_RELEASE(srdi_msg_xml);
        }
        if (new_msgs)
            jxta_vector_clear(new_msgs);
        if (srdi_msg)
            JXTA_OBJECT_RELEASE(srdi_msg);
        if (elem)
            JXTA_OBJECT_RELEASE(elem);
        if (el)
            JXTA_OBJECT_RELEASE(el);
        if (ep_msg_entry_j)
            JXTA_OBJECT_RELEASE(ep_msg_entry_j);
        if (msgs)
            JXTA_OBJECT_RELEASE(msgs);
        if (groupid_j)
            JXTA_OBJECT_RELEASE(groupid_j);
        if (JXTA_SUCCESS != res) break;
    }
#ifdef GZIP_ENABLED
FINAL_EXIT:
#endif

    if (entries)
        JXTA_OBJECT_RELEASE(entries);
    if (ep_xml);
        JXTA_OBJECT_RELEASE(ep_xml);
    if (new_msgs)
        JXTA_OBJECT_RELEASE(new_msgs);

    return res;
}

static void *APR_THREAD_FUNC srdi_send_srdi_msgs_thread(apr_thread_t * thread, void *arg)
{
    Srdi_msgs_thread_struct * t_ptr = NULL;
    Jxta_srdi_service_ref *srdi = NULL;
    Jxta_status res = JXTA_FAILED;
    Jxta_boolean locked = FALSE;

    srdi = (Jxta_srdi_service_ref *) arg;

    jxta_service_lock((Jxta_service *)srdi);
    locked = TRUE;
    if (!srdi->running) {
        goto FINAL_EXIT;
    }

    if (srdi->ep_msgs_queue != NULL) {
        res = jxta_vector_remove_object_at(srdi->ep_msgs_queue, JXTA_OBJECT_PPTR(&t_ptr), 0);
        if (res != JXTA_SUCCESS || NULL == t_ptr) {
            goto FINAL_EXIT;
        }
    }
    else {
        goto FINAL_EXIT;
    }

    jxta_service_unlock((Jxta_service *) srdi);
    locked = FALSE;

    if (t_ptr->srdi_service->running) {
        srdi_send_srdi_msgs(t_ptr->srdi_service, t_ptr->instance, t_ptr->dest, t_ptr->msgs);
    }

FINAL_EXIT:

    if (locked) {
        jxta_service_unlock((Jxta_service *) srdi);
    }

    if (NULL != t_ptr) JXTA_OBJECT_RELEASE(t_ptr);
    
    return NULL;
}

static Jxta_status srdi_send_srdi_msgs(Jxta_srdi_service_ref * self, JString *instance
                                    , Jxta_peer * dest, Jxta_hashtable * msgs_hash)
{
    Jxta_status res=JXTA_SUCCESS;
    char **groups=NULL;
    char **groups_save=NULL;
    Jxta_vector *msgs=NULL;
    Jxta_endpoint_message *ep_msg=NULL;
    Jxta_endpoint_address *ep_addr=NULL;
    JString *ep_msg_xml=NULL;
    JString *peerid_j=NULL;
    Jxta_boolean locked = FALSE;

    jxta_id_to_jstring(jxta_peer_peerid(dest), &peerid_j);
    apr_thread_mutex_lock(self->ep_msgs_lock);
    locked = TRUE;
    if (JXTA_SUCCESS != jxta_hashtable_get(self->ep_msgs
                        , jstring_get_string(peerid_j), jstring_length(peerid_j) +1, JXTA_OBJECT_PPTR(&msgs))) {
        msgs = jxta_vector_new(0);
        jxta_hashtable_put(self->ep_msgs
                        , jstring_get_string(peerid_j), jstring_length(peerid_j) + 1,(Jxta_object *) msgs);
    }
    locked = FALSE;
    apr_thread_mutex_unlock(self->ep_msgs_lock);
    ep_msg = jxta_endpoint_msg_new();
    jxta_endpoint_msg_set_priority(ep_msg, MSG_NORMAL_FLOW);
    jxta_endpoint_msg_set_timestamp(ep_msg, jpr_time_now());
    groups = jxta_hashtable_keys_get(msgs_hash);
    groups_save = groups;
    while (NULL != groups && *groups) {
        Jxta_vector *msg_vec=NULL;

        if (JXTA_SUCCESS == jxta_hashtable_get(msgs_hash, *groups, strlen(*groups) + 1, JXTA_OBJECT_PPTR(&msg_vec))) {
            int i;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending %d msgs for group: %s\n", jxta_vector_size(msg_vec), *groups);
            for (i=0; i<jxta_vector_size(msg_vec); i++) {
                Jxta_message_element *msg_elem=NULL;
                Jxta_endpoint_msg_entry_element *elem = NULL;
                JString *srdi_msg_xml=NULL;

                jxta_vector_get_object_at(msg_vec, JXTA_OBJECT_PPTR(&srdi_msg_xml), i);
                elem = jxta_endpoint_message_entry_new();
                msg_elem = jxta_message_element_new_2("jxta", JXTA_SRDI_MESSAGE, "text/xml",
                                    jstring_get_string(srdi_msg_xml), jstring_length(srdi_msg_xml), NULL);

                jxta_endpoint_msg_entry_set_value(elem, msg_elem);
                jxta_endpoint_msg_entry_add_attribute(elem, "groupid", *groups);
                jxta_endpoint_msg_add_entry(ep_msg, elem);
                jxta_endpoint_msg_set_timestamp(ep_msg, jpr_time_now());

                JXTA_OBJECT_RELEASE(msg_elem);
                JXTA_OBJECT_RELEASE(elem);
                if (srdi_msg_xml)
                    JXTA_OBJECT_RELEASE(srdi_msg_xml);
            }
        }
        if (msg_vec)
            JXTA_OBJECT_RELEASE(msg_vec);
        free(*(groups++));
    }
    while (NULL != groups && *groups) free(*(groups++));

    jxta_peer_get_address(dest, &ep_addr);
    jxta_endpoint_msg_get_xml(ep_msg, TRUE, &ep_msg_xml, TRUE);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send srdi_send_srdi_msgs length:%d\n", jstring_length(ep_msg_xml));

    if (!locked) {
        apr_thread_mutex_lock(self->ep_msgs_lock);
        locked = TRUE;
    }
    jxta_vector_add_object_first(msgs, (Jxta_object *) ep_msg);

    /* rsp_lock must be locked to enter processing of this loop
     * the lock will be re-obtained at the end of the while loop
     */

   while (jxta_vector_size(msgs) > 0 && isRunning(self)) {
        Jxta_endpoint_message *send_ep_msg=NULL;
        Jxta_vector *ep_msg_entries=NULL;
        Jxta_boolean break_out=FALSE;
        int j;
        Jxta_time_diff diff=0;
        Jxta_endpoint_return_parms *return_parms=NULL;
        SRDI_parm_arg *parm_arg=NULL;

        jxta_vector_remove_object_at(msgs, JXTA_OBJECT_PPTR(&send_ep_msg), 0);
        apr_thread_mutex_unlock(self->ep_msgs_lock);
        locked = FALSE;
        jxta_endpoint_msg_get_entries(send_ep_msg, &ep_msg_entries);
        for (j=0; j<jxta_vector_size(ep_msg_entries); j++) {
            Jxta_endpoint_msg_entry_element *msg_elem=NULL;
            Jxta_SRDIMessage *srdi_msg=NULL;
            Jxta_id *src_pid=NULL;
            JString *srdi_msg_xml=NULL;
            Jxta_bytevector *ep_entry_bv=NULL;

            jxta_vector_get_object_at(ep_msg_entries, JXTA_OBJECT_PPTR(&msg_elem), j);

            get_srdi_msg_from_ep_elem(msg_elem, &ep_entry_bv, &srdi_msg);

            diff = jpr_time_now() - jxta_endpoint_msg_timestamp(send_ep_msg);

            jxta_srdi_message_get_SrcPID(srdi_msg, &src_pid);

            jxta_srdi_message_get_xml(srdi_msg, &srdi_msg_xml);
            /* clear the entry and replace the value */
            jxta_bytevector_clear(ep_entry_bv, jstring_length(srdi_msg_xml));
            jxta_bytevector_add_bytes_at(ep_entry_bv
                        , (unsigned char const *) jstring_get_string(srdi_msg_xml)
                        , 0,jstring_length(srdi_msg_xml));

            JXTA_OBJECT_RELEASE(srdi_msg);
            JXTA_OBJECT_RELEASE(ep_entry_bv);
            JXTA_OBJECT_RELEASE(srdi_msg_xml);
            JXTA_OBJECT_RELEASE(msg_elem);
            JXTA_OBJECT_RELEASE(src_pid);
        }
        return_parms = jxta_endpoint_return_parms_new();
        jxta_endpoint_return_parms_set_function(return_parms, (EndpointReturnFunc) return_ep_msg_func);
        jxta_endpoint_return_parms_set_msg(return_parms, (Jxta_message *) send_ep_msg);
        jxta_endpoint_return_parms_set_service(return_parms, (Jxta_service *) self);
        parm_arg = srdi_parm_arg_new();
        parm_arg->msg = (Jxta_message *)(JXTA_OBJECT_SHARE(send_ep_msg));
        parm_arg->peerid_j = JXTA_OBJECT_SHARE(peerid_j);
        parm_arg->timestamp = jpr_time_now();
        parm_arg->peer_id = JXTA_OBJECT_SHARE(jxta_peer_peerid(dest));
        parm_arg->ep_variable = "groupid";
        parm_arg->svc_name = JXTA_OBJECT_SHARE(instance);
        parm_arg->svc_param = SRDI_QUEUENAME;
        jxta_endpoint_return_parms_set_arg(return_parms, (Jxta_object *) parm_arg);

        res = jxta_PG_async_send_1(self->group, send_ep_msg, jxta_peer_peerid(dest), "groupid"
                            , jstring_get_string(instance), SRDI_QUEUENAME, return_parms);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SRDI messages send unsuccessful res:%d\n",res);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "SRDI messages send successful\n");

        }
        JXTA_OBJECT_RELEASE(parm_arg);
        JXTA_OBJECT_RELEASE(return_parms);
        JXTA_OBJECT_RELEASE(send_ep_msg);
        JXTA_OBJECT_RELEASE(ep_msg_entries);
        if (break_out) break;

        /* re-lock to check the beginning of the while loop */
        if (!locked) {
            apr_thread_mutex_lock(self->ep_msgs_lock);
            locked = TRUE;
        }
    }

    if (locked) {
        apr_thread_mutex_unlock(self->ep_msgs_lock);
    }
    if (ep_msg_xml)
        JXTA_OBJECT_RELEASE(ep_msg_xml);
    if (groups_save)
        free(groups_save);
    if (ep_addr)
        JXTA_OBJECT_RELEASE(ep_addr);
    if (ep_msg)
        JXTA_OBJECT_RELEASE(ep_msg);
    if (msgs)
        JXTA_OBJECT_RELEASE(msgs);
    if (peerid_j)
        JXTA_OBJECT_RELEASE(peerid_j);

    return res;
}

static void srdi_msgs_thread_free(Jxta_object *obj)
{
    Srdi_msgs_thread_struct *t_ptr = (Srdi_msgs_thread_struct*)obj;

    JXTA_OBJECT_RELEASE(t_ptr->instance);
    JXTA_OBJECT_RELEASE(t_ptr->dest);
    JXTA_OBJECT_RELEASE(t_ptr->msgs);
    
    free(t_ptr);
}


static Jxta_status pushSrdi_msgs(Jxta_srdi_service * service, JString * instance, Jxta_hashtable *msgs)
{
    Jxta_srdi_service_ref *self = (Jxta_srdi_service_ref *) service;
    char **peers;
    char **peers_save;

    peers = jxta_hashtable_keys_get(msgs);
    peers_save = peers;
    while (NULL != peers && *peers) {
        Jxta_hashtable *groupid_hash=NULL;


        if (JXTA_SUCCESS != jxta_hashtable_get(msgs, *peers, strlen(*peers) + 1, JXTA_OBJECT_PPTR(&groupid_hash))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve groupid_hash entry - %s\n", *peers);
        } else {
            Jxta_status status;
            Jxta_peer *dest=NULL;
            Jxta_id *destid=NULL;
            apr_thread_pool_t *tp;

            jxta_id_from_cstr(&destid, *peers);
            if(destid != NULL ) {

            dest = jxta_peer_new();
            jxta_peer_set_peerid(dest, destid);

            tp = jxta_PG_thread_pool_get(self->group);

            if (NULL != tp) {
                Srdi_msgs_thread_struct *thread_ptr;

                thread_ptr = calloc(1, sizeof(Srdi_msgs_thread_struct));
                JXTA_OBJECT_INIT(thread_ptr, srdi_msgs_thread_free, (void*)NULL);
                thread_ptr->srdi_service = self;
                thread_ptr->instance=JXTA_OBJECT_SHARE(instance);
                thread_ptr->dest=JXTA_OBJECT_SHARE(dest);
                thread_ptr->msgs=JXTA_OBJECT_SHARE(groupid_hash);

                jxta_service_lock((Jxta_service*) self);
                if (self->running) {
                    jxta_vector_add_object_last(self->ep_msgs_queue, (Jxta_object*)thread_ptr);
                    apr_thread_pool_push(tp, srdi_send_srdi_msgs_thread, self, APR_THREAD_TASK_PRIORITY_HIGHEST, self);
                }
                jxta_service_unlock((Jxta_service*) self);

                JXTA_OBJECT_RELEASE(thread_ptr);
            }
            /* status = srdi_send_srdi_msgs(self, instance, dest, groupid_hash); */
            status = JXTA_SUCCESS;
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error:%d sending SRDI msgs to %s\n", status, *peers);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sent SRDI msgs hash [%pp] to %s\n", groupid_hash, *peers);
            }

            JXTA_OBJECT_RELEASE(dest);
            JXTA_OBJECT_RELEASE(destid);
            }
            else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to get dest id from key\n"); 
            }

        }
        if (groupid_hash)
            JXTA_OBJECT_RELEASE(groupid_hash);
        free(*(peers++));
    }
    if (NULL != peers_save) free(peers_save);

    return JXTA_SUCCESS;
}


static Jxta_status pushSrdi(Jxta_srdi_service * self, JString * instance,
                            Jxta_SRDIMessage * srdi, Jxta_id * peer, Jxta_boolean sync)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) self;

    JXTA_DEPRECATED_API();

    res = pushSrdi_msg_priv(me, instance, srdi, peer, sync, NULL);
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
            JString * peerid_j;

            jxta_id_to_jstring(peer, &peerid_j);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Forwarding query to %s\n", jstring_get_string(peerid_j));
            JXTA_OBJECT_RELEASE(peerid_j);
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
        /* TODO ExocetRick - change return to JXTA_TTL_EXPIRED and move diagnostic to caller */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Hop Count exceeded - Query is Dropped (check discovery state) and processing query complete\n");        
        return JXTA_SUCCESS;
    }
    for (i = 0; peers != NULL && i < jxta_vector_size(peers); i++) {
        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peerId), i);
        if (jxta_id_equals(me->peerID, peerId)) {
            status = JXTA_SUCCESS;
        } else {
            JString * peerid_j;

            jxta_id_to_jstring(peerId, &peerid_j);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forwarding query id: %ld to %s\n", jxta_resolver_query_get_queryid(query), jstring_get_string(peerid_j));

            JXTA_OBJECT_RELEASE(peerid_j);
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

static void get_replica_peers_priv(Jxta_srdi_service_ref * me, Jxta_SRDIEntryElement * entry, Jxta_peer **peer, Jxta_vector **peers)
{
    JString * replica_expression=NULL;
    const char *numeric_value=NULL;

    replica_expression = jstring_new_0();

    jstring_append_1(replica_expression, entry->key);
    numeric_value = jstring_get_string(entry->value);
    if (('#' != *numeric_value) || ('#' == *numeric_value && jxta_srdi_cfg_get_no_range(me->config))) {
        if ('#' == *numeric_value && jxta_srdi_cfg_get_no_range(me->config)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got the no range of key:%s value:%s as true\n",
                                jstring_get_string(entry->key), numeric_value);
        }
        jstring_append_1(replica_expression, entry->value);
    }
    if (NULL == entry->range || (0 == jstring_length(entry->range))) {
        JString * jPeerid = NULL;

        getReplicaPeers((Jxta_srdi_service *) me, jstring_get_string(replica_expression), peer, peers);
        if (NULL != *peer) {
            jxta_id_to_jstring(jxta_peer_peerid(*peer), &jPeerid);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got replica peer %s for %s\n", jstring_get_string(jPeerid), jstring_get_string(replica_expression));
            JXTA_OBJECT_RELEASE(jPeerid);
        }

    } else {
        Jxta_range *rge;

        rge = jxta_range_new();
        jxta_range_set_range(rge, jstring_get_string(entry->nameSpace), jstring_get_string(entry->key),
                                 jstring_get_string(entry->range));
        if ('#' == *numeric_value) {
            numeric_value++;
            *peer = getNumericReplica((Jxta_srdi_service *) me, rge, numeric_value);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid value in SRDI Numeric Entry: %s  Did not replicate\n",
                                numeric_value);
        }
        JXTA_OBJECT_RELEASE(rge);
    }
    if (replica_expression)
        JXTA_OBJECT_RELEASE(replica_expression);
    return;
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
static Jxta_peer *getReplicaPeer(Jxta_srdi_service * me, const char *expression, Jxta_peer **alt_peer)
{
    Jxta_status res;
    Jxta_srdi_service_ref *myself = PTValid(me, Jxta_srdi_service_ref);
    Jxta_peerview *rpv;
    BIGNUM *bn_hash = NULL;
    Jxta_peer *peer = NULL;

    rpv = rdv_service_get_peerview_priv(myself->rendezvous);

    if (NULL != rpv) {
        res = jxta_peerview_gen_hash( rpv, (unsigned char const *) expression, strlen(expression), &bn_hash );
        if (NULL == alt_peer) {
            res = jxta_peerview_get_peer_for_target_hash( rpv, bn_hash, &peer );
        } else {
            res = jxta_peerview_get_peer_for_target_hash_1(rpv, bn_hash, &peer, alt_peer);
        }

        BN_free( bn_hash );

        if (JXTA_SUCCESS == res && peer) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address in getReplicaPeer expression: %s : %s://%s\n",
                        expression,
                        jxta_endpoint_address_get_protocol_name(jxta_peer_address(peer)),
                        jxta_endpoint_address_get_protocol_address(jxta_peer_address(peer)));
            return peer;
        }
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
                const char *aattribute;
                JString *replicaExpression = NULL;

                replicaExpression = jstring_new_2(jxta_range_get_element(rge));
                aattribute = jxta_range_get_attribute(rge);
                if (NULL != aattribute) {
                    jstring_append_2(replicaExpression, aattribute);
                }
                if (jxta_srdi_cfg_get_no_range(myself->config)) {
                    jstring_append_2(replicaExpression, val);
                }
                peer = getReplicaPeer( (Jxta_srdi_service*) myself, jstring_get_string(replicaExpression), NULL);
                JXTA_OBJECT_RELEASE(replicaExpression);
            }
        }
    } else {
        status = jxta_peerview_get_best_associate_peer(rpv, pos, &peer);
        if (JXTA_SUCCESS == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Replica address from getNumericReplica : %s://%s\n",
                            jxta_endpoint_address_get_protocol_name(jxta_peer_address(peer)),
                            jxta_endpoint_address_get_protocol_address(jxta_peer_address(peer)));
        }
    }

    return peer;
}

static Jxta_status getReplicaPeers(Jxta_srdi_service * me, const char *expression, Jxta_peer ** peer, Jxta_vector **peers)
{
    Jxta_status res;
    Jxta_srdi_service_ref *myself = PTValid(me, Jxta_srdi_service_ref);
    Jxta_peerview *rpv;
    BIGNUM *bn_hash = NULL;

    rpv = rdv_service_get_peerview_priv(myself->rendezvous);

    res = jxta_peerview_gen_hash( rpv, (unsigned char const *) expression, strlen(expression), &bn_hash );

    res = jxta_peerview_get_peers_for_target_hash( rpv, bn_hash, peer, peers );

    BN_free( bn_hash );

    if (JXTA_SUCCESS == res ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%d Replica peers found for expression:%s\n"
                        , jxta_vector_size(*peers)
                        , expression);
    }

    return res;
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

/**
 *  forward srdi message to another peer
 *
 * @param  service The SRDI service
 * @param  resolver Resolver service
 * @param  peer       Destination peer
 * @param  fromPID   from peer
 * @param  srcPID    source peer of the entry
 * @param  entries vector of SRDI elements
 * @param  ttl time to live
 * @param  queueName name for the destination
 * @param  jKey jstring of the primary key
 * @param  ret_msgs if NOT NULL don't send messages --- put them in this hash keyed by peer and group.
 *                  if NULL send the messages
*/
static Jxta_status forwardSrdiEntries_priv(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * fromPid, Jxta_id * srcPid, Jxta_vector * entries, int ttl, JString * queueName, JString * jKey, Jxta_hashtable ** ret_msgs)
{
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) service;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_id *pid = NULL;
    Jxta_SRDIMessage *msg = NULL;
    JString *jSourceId = NULL;
    JString *jPid = NULL;
    Jxta_vector * mod_entries = NULL;

    jxta_id_to_jstring(srcPid, &jSourceId);

    jxta_peer_get_peerid(peer, &pid);
    if (pid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve peer id from jxta_peer. Message discarded\n");
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    jxta_id_to_jstring(pid, &jPid);
    if (!strcmp(jstring_get_string(jPid), jstring_get_string(jSourceId))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Don't send it to myself. Message ignored\n");
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    msg = jxta_srdi_message_new_2(ttl, NULL == fromPid ? me->peerID:fromPid, srcPid , (char *) jstring_get_string(jKey), entries);

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot allocate jxta_srdi_message_new\n");
        res = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    if (NULL == ret_msgs) {
        res = pushSrdi_msg(service, queueName, msg, pid, TRUE, FALSE, NULL);
    } else {
        JString *peerid_j=NULL;
        Jxta_vector *msg_v=NULL;
        Jxta_hashtable *groupid_hash=NULL;

        jxta_id_to_jstring(pid, &peerid_j);

        if (NULL == *ret_msgs) {
            *ret_msgs = jxta_hashtable_new(0);
        }
        if (JXTA_SUCCESS != jxta_hashtable_get(*ret_msgs, jstring_get_string(peerid_j), jstring_length(peerid_j) + 1, JXTA_OBJECT_PPTR(&groupid_hash))) {
            groupid_hash = jxta_hashtable_new(0);
            jxta_hashtable_put(*ret_msgs, jstring_get_string(peerid_j), jstring_length(peerid_j) + 1, (Jxta_object *) groupid_hash);
        }
        if (JXTA_SUCCESS != jxta_hashtable_get(groupid_hash, me->groupid_c, strlen(me->groupid_c) + 1, JXTA_OBJECT_PPTR(&msg_v))) {
            msg_v = jxta_vector_new(0);
            jxta_hashtable_put(groupid_hash, me->groupid_c, strlen(me->groupid_c) + 1, (Jxta_object *) msg_v);
        }
        jxta_vector_add_object_last(msg_v, (Jxta_object *) msg);

        JXTA_OBJECT_RELEASE(peerid_j);
        JXTA_OBJECT_RELEASE(msg_v);
        JXTA_OBJECT_RELEASE(groupid_hash);
    }


  FINAL_EXIT:
    if (mod_entries)
        JXTA_OBJECT_RELEASE(mod_entries);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    if (jSourceId)
        JXTA_OBJECT_RELEASE(jSourceId);
    if (pid)
        JXTA_OBJECT_RELEASE(pid);
    if (jPid)
        JXTA_OBJECT_RELEASE(jPid);
    return res;
}

static Jxta_status forwardSrdiEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver, Jxta_peer * peer,
                   Jxta_id * srcPid, Jxta_vector * entries, int ttl, JString * queueName, JString * jKey)
{
    return forwardSrdiEntries_priv(service, resolver, peer, NULL, srcPid, entries, ttl, queueName, jKey, NULL);
}

#ifdef UNUSED_VWF
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
#endif

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

    cache_entries = cm_sql_query_srdi(cm, "*:*", sqlwhere, TRUE);

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

static void expire_entries(Jxta_vector * srdi_entries)
{
    int i;
    Jxta_status res = JXTA_SUCCESS;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Expire %d total entries\n", jxta_vector_size(srdi_entries));

    for (i = 0; i < jxta_vector_size(srdi_entries); i++) {
        Jxta_SRDIEntryElement *entry = NULL;
        res = jxta_vector_get_object_at(srdi_entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve SRDI entry\n");
            continue;
        }
        /* expire this entry */
        entry->expiration = 0;
        JXTA_OBJECT_RELEASE(entry);
    }
}

static void JXTA_STDCALL srdi_rdv_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = 0;
    JString *jPeerid = NULL;
    const char *peerid = "(null)";
    Jxta_peer *peer = NULL;
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;
    Jxta_srdi_service_ref *me = (Jxta_srdi_service_ref *) arg;
    Jxta_time expires=0;
    Jxta_vector * srdi_entries = NULL;
    Jxta_vector * replica_entries = NULL;

    if (!isRunning(me)) {
        goto finish;
    }

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
        case JXTA_RDV_CLIENT_EXPIRED:
        case JXTA_RDV_CLIENT_DISCONNECTED:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Connected rdv client disconnected peer in SRDI =%s\n", peerid);
            if (NULL != me->cm) {
                cm_remove_srdi_entries(me->cm, jPeerid, &srdi_entries);
                if (NULL != srdi_entries) {
                    /* srdi_service_remove_replicas(me, srdi_entries); */
                }
            }
            break;
        case JXTA_RDV_CLIENT_CONNECTED:
        case JXTA_RDV_CLIENT_RECONNECTED:
        case JXTA_RDV_CONNECTED:
            res = jxta_rdv_service_get_peer(me->rendezvous, rdv_event->pid, &peer);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Got a rdv event with an unknown peerid %s\n", peerid);
                break;
            }
            if (NULL != peer) {
                expires = jxta_peer_get_expires(peer);
            }

            if (type == JXTA_RDV_CONNECTED) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                            "Connected rdv expires: %" APR_INT64_T_FMT " %s\n",expires - jpr_time_now(), peerid);
            }
            else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                            "Connected rdv client expires: %" APR_INT64_T_FMT " %s\n",expires - jpr_time_now(), peerid);
            }
            break;
        case JXTA_RDV_FAILED:
        case JXTA_RDV_DISCONNECTED:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Rendezvous %s %s SRDI in group %s\n",
                            peerid, JXTA_RDV_FAILED == type ? "FAILED" : "DISCONNECTED", jstring_get_string(me->groupUniqueID));

            if (NULL != me->cm) {
                res = cm_get_replica_index_entries(me->cm, jPeerid, &replica_entries);
                if (JXTA_SUCCESS == res && NULL != replica_entries) {
                    int i;

                    /* change forward peer to hash of the adv ID */
                    for (i=0; i<jxta_vector_size(replica_entries); i++) {
                        Jxta_srdi_idx_entry * entry=NULL;
                        Jxta_id * rep_id = NULL;

                        res = jxta_vector_get_object_at(replica_entries, JXTA_OBJECT_PPTR(&entry), i);
                        if (JXTA_SUCCESS != res) {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to get object from replica entries vector error:%d\n", res);
                            continue;
                        }
                        if (!strcmp(jstring_get_string(entry->dup_id), "''")) {
                            jxta_vector_remove_object_at(replica_entries, NULL, i--);
                            JXTA_OBJECT_RELEASE(entry);
                            continue;
                        }
                        if(entry->adv_id == NULL ) {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Entry does not have an adv_id [%pp]\n", entry);
                            JXTA_OBJECT_RELEASE(entry);
                            break;
                        }
 
                        res = jxta_id_from_jstring(&rep_id, entry->dup_id);
                        if (JXTA_SUCCESS != res) {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error creating an id from %s\n", jstring_get_string(entry->dup_id));
                            JXTA_OBJECT_RELEASE(entry);
                            continue;
                        }
                        if (entry->peer_id) {
                            JXTA_OBJECT_RELEASE(entry->peer_id);
                        }
                        /* this peer and the lost peer cannot be used as the forwarding peer */
                        if (!jxta_id_equals(rep_id, me->peerID) && !jxta_id_equals(rep_id,rdv_event->pid)) {
                            jxta_id_to_jstring(rep_id, &entry->peer_id);
                        } else {
                            entry->peer_id = jstring_new_2("''");
                        }

                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Changing replica: (%s) from %s to (%s)\n", 
                            jstring_get_string(entry->key), 
                            jstring_get_string(jPeerid), 
                            strncmp(jstring_get_string(entry->peer_id), "''", 2) == 0 ? "Invalid":jstring_get_string(entry->peer_id));

                        if (rep_id)
                            JXTA_OBJECT_RELEASE(rep_id);
                        if (entry)
                            JXTA_OBJECT_RELEASE(entry);
                    }
                    if (isRunning(me) && jxta_vector_size(replica_entries) > 0) {
                        cm_update_replicating_peers(me->cm, replica_entries, jPeerid);
                    }
                }
                cm_remove_srdi_entries(me->cm, jPeerid, &srdi_entries);
                if (isRunning(me) && srdi_entries) {
                    srdi_service_remove_replicas(me, srdi_entries);
                }
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
    if (srdi_entries)
        JXTA_OBJECT_RELEASE(srdi_entries);
    if (replica_entries)
        JXTA_OBJECT_RELEASE(replica_entries);
}

static Jxta_status srdi_service_remove_replicas(Jxta_srdi_service_ref * srdi, Jxta_vector * entries)
{

    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *advs_v = NULL;
    Jxta_vector *peers_v = NULL;
    Jxta_vector *groups_v = NULL;
    JString * pkey=NULL;
    int i;
    Jxta_discovery_service *discovery=NULL;

    expire_entries(entries);

    for (i = 0; i < jxta_vector_size(entries); i++) {
        Jxta_SRDIEntryElement * entry;
        jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (0 == strcmp(jstring_get_string(entry->nameSpace), "jxta:PA")) {
            if (NULL == peers_v) {
                peers_v = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(peers_v,(Jxta_object *) entry);
        } else if (0 == strcmp(jstring_get_string(entry->nameSpace), "jxta:PGA")) {
            if (NULL == groups_v) {
                groups_v = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(groups_v, (Jxta_object *) entry);
        } else {
            if (NULL == advs_v) {
                advs_v = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(advs_v, (Jxta_object *) entry);
        }
        JXTA_OBJECT_RELEASE(entry);
    }

    pkey = jstring_new_0();

    jxta_PG_get_discovery_service(srdi->group, &discovery);
    if (NULL == discovery) goto FINAL_EXIT;

    if (NULL != peers_v) {
        jstring_reset(pkey, NULL);
        jstring_append_2(pkey, "Peers");
        discovery_send_srdi(discovery, pkey, peers_v, NULL, FALSE, FALSE, NULL);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Expire %d peer entries\n", jxta_vector_size(peers_v));
    }
    if (NULL != groups_v) {
        jstring_reset(pkey, NULL);
        jstring_append_2(pkey, "Groups");
        discovery_send_srdi(discovery, pkey, groups_v, NULL, FALSE, FALSE, NULL);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Expire %d group entries\n", jxta_vector_size(groups_v));
    }
    if (NULL != advs_v) {
        jstring_reset(pkey, NULL);
        jstring_append_2(pkey, "Adv");
        discovery_send_srdi(discovery, pkey, advs_v, NULL, FALSE, FALSE, NULL);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Expire %d adv entries\n", jxta_vector_size(advs_v));
    }

    JXTA_OBJECT_RELEASE(discovery);

FINAL_EXIT:

    if (peers_v)
        JXTA_OBJECT_RELEASE(peers_v);
    if (groups_v)
        JXTA_OBJECT_RELEASE(groups_v);
    if (advs_v)
        JXTA_OBJECT_RELEASE(advs_v);
    if (pkey)
        JXTA_OBJECT_RELEASE(pkey);

    return res;
}

static Jxta_status get_element_bytes(Jxta_message_element *element, JString **string)
{
    Jxta_status res = JXTA_FAILED;
    const char *mime_type = jxta_message_element_get_mime_type(element);

    *string = NULL;
    if (mime_type != NULL) {
        unsigned char *bytes = NULL;
        unsigned char *uncompr = NULL;
        unsigned long uncomprLen = 0;
        int size;

        Jxta_bytevector *jb = jxta_message_element_get_value(element);
        size = jxta_bytevector_size(jb);
        bytes = calloc(1, size);
        if (bytes == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            JXTA_OBJECT_RELEASE(jb);
            JXTA_OBJECT_RELEASE(element);
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        jxta_bytevector_get_bytes_at(jb, bytes, 0, size);
        JXTA_OBJECT_RELEASE(jb);
        if (!strcmp(mime_type, "application/gzip")) {
#ifdef GUNZIP_ENABLED
            res = jxta_xml_util_uncompress(bytes, size, &uncompr, &uncomprLen);
            free(bytes);
#else
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Received GZip mime type but GZip is disabled \n");
#endif
        } else {
            uncompr = bytes;
            uncomprLen = size;
        }
        if (NULL != uncompr) {
            *string = jstring_new_0();
            jstring_append_0(*string, (const char *) uncompr, uncomprLen);
        }
        free(uncompr);
    }
FINAL_EXIT:
    return res;
}

static Jxta_status JXTA_STDCALL srdi_service_srdi_cb(Jxta_object * obj, void *arg)
{
    Jxta_srdi_service_ref *myself = (Jxta_srdi_service_ref *) arg;
    Jxta_status res=JXTA_FAILED;
    Jxta_message *msg;
    Jxta_endpoint_address *address=NULL;
    Jxta_SRDIListenerRef *listenerRef=NULL;
    const char *service_parms;
    Jxta_message_element *element=NULL;

    if (!isRunning(myself)) {
        goto FINAL_EXIT;
    }
    /* share the srdi service reference so that things
     * are not torn down while processing the message
     */
    JXTA_OBJECT_SHARE(myself);
    jxta_service_unlock((Jxta_service *) myself);
    msg = (Jxta_message *) obj;

    address = jxta_message_get_destination(msg);

    service_parms = jxta_endpoint_address_get_service_params(address);

    if (NULL == service_parms) {
        goto FINAL_EXIT;
    }
    jxta_hashtable_get(myself->srdilisteners, service_parms, strlen(service_parms), JXTA_OBJECT_PPTR(&listenerRef));

    if (NULL == listenerRef || NULL == listenerRef->srdilistener) {
        goto FINAL_EXIT;
    }

    res = jxta_message_get_element_2(msg, (char *) JXTA_SRDI_NS_NAME, (char *) JXTA_SRDI_MESSAGE, &element);
    if (element) {
        JString *parm_string;

        res = get_element_bytes(element, &parm_string);

        if (NULL != parm_string) {
            jxta_listener_process_object(listenerRef->srdilistener, (Jxta_object *) parm_string);
            JXTA_OBJECT_RELEASE(parm_string);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "no element for %s\n", JXTA_SRDI_MESSAGE);
    }

FINAL_EXIT:
    if (myself)
        JXTA_OBJECT_RELEASE(myself);
    if (address)
        JXTA_OBJECT_RELEASE(address);
    if (listenerRef)
        JXTA_OBJECT_RELEASE(listenerRef);
    if (element)
        JXTA_OBJECT_RELEASE(element);
    return res;
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
    registerSrdiListener,
    unregisterSrdiListener,
    replicateEntries,
    pushSrdi,
    pushSrdi_msg,
    pushSrdi_msgs,
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
    apr_thread_pool_t *tp = NULL;

    Jxta_srdi_service_ref *myself = PTValid( me, Jxta_srdi_service_ref );

    jxta_service_lock((Jxta_service *) myself);
    myself->running = FALSE;
    jxta_service_unlock((Jxta_service *) myself);

    tp = jxta_PG_thread_pool_get(myself->group);
    if (NULL != tp) {
        apr_thread_pool_tasks_cancel(tp, myself);
    }

    while(jxta_vector_size(myself->srdi_msgs_queue) > 0) {
        Srdi_msg_thread_struct *srdi_thread = NULL;
        jxta_vector_remove_object_at(myself->srdi_msgs_queue, JXTA_OBJECT_PPTR(&srdi_thread), 0);
        if (NULL != srdi_thread) JXTA_OBJECT_RELEASE(srdi_thread);
    }

    while(jxta_vector_size(myself->ep_msgs_queue) > 0) {
        Srdi_msgs_thread_struct *ep_thread = NULL;
        jxta_vector_remove_object_at(myself->ep_msgs_queue, JXTA_OBJECT_PPTR(&ep_thread), 0);
        if (NULL != ep_thread) JXTA_OBJECT_RELEASE(ep_thread);
    }


    jxta_listener_stop(myself->rdv_listener);
    jxta_rdv_service_remove_event_listener(myself->rendezvous, "SRDIMessage", "abc");
    JXTA_OBJECT_RELEASE(myself->rdv_listener);

    Jxta_status status = endpoint_service_remove_recipient(myself->endpoint, myself->ep_cookie);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to remove endpoint listener for service (%x)\n", myself);
    }

    status = jxta_endpoint_service_cleanup_pending_msgs(myself->endpoint, (Jxta_service *)myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}


static Jxta_status start(Jxta_module * me, const char *argv[])
{
    Jxta_srdi_service_ref *myself = PTValid( me, Jxta_srdi_service_ref );
    Jxta_listener * listener;
    Jxta_status status;

    assert(myself->rendezvous);
    assert(myself->endpoint);
    assert(myself->resolver);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Starting...\n");

    myself->running = TRUE;

    /** Add a listener to rdv events */
    listener = jxta_listener_new((Jxta_listener_func) srdi_rdv_listener, myself, 1, 1);
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

    status = jxta_PG_add_recipient(myself->group, &myself->ep_cookie, jstring_get_string(myself->instanceName), SRDI_QUEUENAME, srdi_service_srdi_cb, myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started.\n");

    return status;
}

static void jxta_srdi_service_ref_construct(Jxta_srdi_service_ref * srdi, Jxta_srdi_service_ref_methods const * methods)
{
    /*
     * we do not extend Jxta_srdi_service_methods; so the type string
     * is that of the base table
     */
    Jxta_srdi_service_methods* service_methods = PTValid(methods, Jxta_srdi_service_methods);

    jxta_srdi_service_construct((Jxta_srdi_service *) srdi, service_methods);

    /* Set our rt type checking string */
    srdi->thisType = "Jxta_srdi_service_ref";
}

static void jxta_srdi_service_ref_destruct(Jxta_srdi_service_ref * srdi)
{
    Jxta_srdi_service_ref *myself = PTValid(srdi, Jxta_srdi_service_ref);

    if (myself->endpoint != 0)
        JXTA_OBJECT_RELEASE(myself->endpoint);
    if (myself->rendezvous != 0)
        JXTA_OBJECT_RELEASE(myself->rendezvous);
    if (myself->resolver != 0)
        JXTA_OBJECT_RELEASE(myself->resolver);
    if (myself->instanceName != 0)
        JXTA_OBJECT_RELEASE(myself->instanceName);
    if (myself->srdilisteners != 0)
        JXTA_OBJECT_RELEASE(myself->srdilisteners);
    if (myself->srdi_queue_name)
        JXTA_OBJECT_RELEASE(myself->srdi_queue_name);
    if (myself->cm != 0)
        JXTA_OBJECT_RELEASE(myself->cm);
    if (myself->peerID != 0)
        JXTA_OBJECT_RELEASE(myself->peerID);
    if (myself->config)
        JXTA_OBJECT_RELEASE(myself->config);
    if (myself->groupUniqueID)
        JXTA_OBJECT_RELEASE(myself->groupUniqueID);
    if (myself->gid)
        JXTA_OBJECT_RELEASE(myself->gid);
    if (myself->srdi_msgs)
        JXTA_OBJECT_RELEASE(myself->srdi_msgs);
    if (myself->srdi_msgs_queue)
        JXTA_OBJECT_RELEASE(myself->srdi_msgs_queue);
    if (myself->ep_msgs)
        JXTA_OBJECT_RELEASE(myself->ep_msgs);
    if (myself->ep_msgs_queue)
        JXTA_OBJECT_RELEASE(myself->ep_msgs_queue);
    if (myself->groupid_c)
        free(myself->groupid_c);
    if (myself->srdi_msgs_lock)
        apr_thread_mutex_destroy(myself->srdi_msgs_lock);
    if (myself->ep_msgs_lock)
        apr_thread_mutex_destroy(myself->ep_msgs_lock);
    if (myself->pool)
        apr_pool_destroy(myself->pool);
    myself->thisType = NULL;

    /* call the base classe's dtor. */
    jxta_srdi_service_destruct((Jxta_srdi_service *) myself);

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

    srdi->running = FALSE;
    /* call the hierarchy of ctors */
    jxta_srdi_service_ref_construct(srdi, &jxta_srdi_service_ref_methods);

    /* return the new object */
    return srdi;
}

/* vim: set ts=4 sw=4 et tw=130: */
