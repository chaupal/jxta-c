/*
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights reserved.
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

#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#include "jpr/jpr_excep_proto.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "math.h"
#include "jxta_apr.h"
#include "jxta_id.h"
#include "jxta_cm.h"
#include "jxta_cm_private.h"
#include "jxta_rq.h"
#include "jxta_rr.h"
#include "jxta_dq.h"
#include "jxta_dr.h"
#include "jxta_dr_priv.h"
#include "jxta_rdv.h"
#include "jxta_hashtable.h"
#include "jxta_pa.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_query.h"
#include "jxta_routea.h"
#include "jxta_private.h"
#include "jxta_proffer.h"
#include "jxta_discovery_service_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service.h"
#include "jxta_sql.h"
#include "jxta_srdi.h"
#include "jxta_stdpg_private.h"
#include "jxta_rsrdi.h"
#include "jxta_srdi_service.h"
#include "jxta_test_adv.h"
#include "jxta_apr.h"
#include "jxta_range.h"
#include "jxta_util_priv.h"
#include "jxta_discovery_config_adv.h"

#define HAVE_POOL

typedef struct {
    Extends(Jxta_discovery_service);

    volatile Jxta_boolean running;

    Jxta_DiscoveryConfigAdvertisement *config;
    JString *instanceName;
    Jxta_PG *group;
    JString *gid_str;
    Jxta_resolver_service *resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_rdv_service *rdv;
    Jxta_srdi_service *srdi;
    /* vector to hold generic discovery listeners */
    Jxta_vector *listener_vec;
    /* hashtable to hold specific query listeners */
    Jxta_hashtable *listeners;
    Jxta_PID *localPeerId;
    char *pid_str;
    Jxta_id *assigned_id;
    Jxta_advertisement *impl_adv;
    Jxta_cm *cm;
    Jxta_boolean cm_on;
    Jxta_listener *my_listeners[3];
    volatile Jxta_time_diff expired_loop;
    volatile Jxta_time_diff delta_loop;
} Jxta_discovery_service_ref;

static Jxta_status discovery_service_send_to_replica(Jxta_discovery_service_ref * discovery, ResolverQuery * rq,
                                                     JString * replicaExpression, Jxta_query_context * jContext);
static Jxta_status JXTA_STDCALL discovery_service_query_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_srdi_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_response_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_rdv_listener(Jxta_object * obj, void *arg);
static void *APR_THREAD_FUNC advertisement_handling_func(apr_thread_t * thread, void *arg);
static Jxta_status discovery_service_send_delta_updates(Jxta_discovery_service_ref * discovery, const char * key, Jxta_hashtable * entries_update);
static Jxta_status discovery_service_send_delta_update_msg(Jxta_discovery_service_ref * discovery, Jxta_PID *source_id, const char * key, Jxta_hashtable * entries_update);

Jxta_status jxta_discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta);

static Jxta_status query_replica(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, Jxta_vector ** peerids);
static Jxta_status query_srdi(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, Jxta_id * dropId, Jxta_vector ** peerids);
Jxta_status publish(Jxta_discovery_service * service, Jxta_advertisement * adv, short type,
                    Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers);

static const char *__log_cat = "DiscoveryService";

enum e_dirname {
    FOLDER_MIN = 0,
    FOLDER_PEER_ADV = 0,
    FOLDER_GROUP_ADV,
    FOLDER_OTHER_ADV,
    FOLDER_MAX
};

#define FOLDER_DEFAULT FOLDER_OTHER_ADV

static const char *dirname[] = { "Peers", "Groups", "Adv" };
static const char *xmltype[] = { "jxta:PA", "jxta:PGA", "*:*" };

void discovery_result_free (Jxta_object * obj)
{
    Jxta_query_result * res;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "free discovery result\n");
    res = (Jxta_query_result *) obj;
    JXTA_OBJECT_RELEASE(res->adv);
    free(res);
}

/*
 * module methods
 */

/**
 * Initializes an instance of the Discovery Service.
 * 
 * @param service a pointer to the instance of the Discovery Service.
 * @param group a pointer to the PeerGroup the Discovery Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_callback *callback = NULL;
    Jxta_listener *listener = NULL;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_PGID *gid = NULL;
    JString *jPid;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) module;
    const char *p_keywords[][2] = {
        {"Name", NULL},
        {"PID", NULL},
        {"Desc", NULL},
        {NULL, NULL}
    };
    const char *g_keywords[][2] = {
        {"Name", NULL},
        {"GID", NULL},
        {"Desc", NULL},
        {"MSID", NULL},
        {NULL, NULL}
    };
    const char *a_keywords[][2] = {
        {"Name", NULL},
        {"Name", "NameAttribute"},
        {"Id", NULL},
        {"MSID", NULL},
        {"RdvGroupId", NULL},
        {"DstPID", NULL},
        {NULL, NULL}
    };

    /* Test arguments first */
    if ((discovery == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_service_init((Jxta_service *) module, group, assigned_id, impl_adv);

    /* store a copy of our assigned id */
    if (assigned_id != NULL) {
        JXTA_OBJECT_SHARE(assigned_id);
        discovery->assigned_id = assigned_id;
    }

    /* keep a reference to our group and impl adv */
    discovery->group = group;   /* not shared because that would create a circular dependence */
    discovery->impl_adv = impl_adv ? JXTA_OBJECT_SHARE(impl_adv) : NULL;
    jxta_PG_get_configadv(group, &conf_adv);
    if (!conf_adv) {
        Jxta_PG *parentgroup=NULL;

        jxta_PG_get_parentgroup(discovery->group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }
    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_discovery_classid, &svc);
        if (NULL != svc) {
            discovery->config = jxta_svc_get_DiscoveryConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == discovery->config) {
        discovery->config = jxta_DiscoveryConfigAdvertisement_new();
    }

    /**
     ** Build the local name of the instance
     **/
    discovery->instanceName = NULL;
    jxta_id_to_jstring(discovery->assigned_id, &discovery->instanceName);
    jxta_PG_get_resolver_service(group, &(discovery->resolver));
    jxta_PG_get_endpoint_service(group, &(discovery->endpoint));
    jxta_PG_get_rendezvous_service(group, &(discovery->rdv));
    jxta_PG_get_srdi_service(group, &(discovery->srdi));
    peergroup_get_cache_manager(group, &(discovery->cm));
    jxta_PG_get_PID(group, &(discovery->localPeerId));
    jxta_id_to_jstring(discovery->localPeerId, &jPid);
    discovery->pid_str = strdup(jstring_get_string(jPid));
    JXTA_OBJECT_RELEASE(jPid);
    jxta_PG_get_GID(group, &gid);
    jxta_id_to_jstring(gid, &discovery->gid_str);
    JXTA_OBJECT_RELEASE(gid);

    if (discovery->cm == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Could not get the Cache Manager - THIS MAY BE FATAL \n");
        discovery->cm_on = FALSE;
    } else {
        discovery->cm_on = TRUE;
    }

    if (discovery->cm_on) {
        /* create the folders for the group */
        cm_create_folder(discovery->cm, (char *) dirname[DISC_PEER], p_keywords[0]);
        cm_create_folder(discovery->cm, (char *) dirname[DISC_GROUP], g_keywords[0]);
        cm_create_folder(discovery->cm, (char *) dirname[DISC_ADV], a_keywords[0]);
    }

    status = jxta_callback_new(&callback, discovery_service_query_listener, discovery);
    if (JXTA_SUCCESS == status) {
        status = jxta_resolver_service_registerQueryHandler(discovery->resolver, discovery->instanceName, callback);
        JXTA_OBJECT_RELEASE(callback);
    }

    if (status == JXTA_SUCCESS) {
        listener = jxta_listener_new((Jxta_listener_func) discovery_service_response_listener, discovery, 1, 200);
        jxta_listener_start(listener);
        status = jxta_resolver_service_registerResHandler(discovery->resolver, discovery->instanceName, listener);
        discovery->my_listeners[0] = listener;
    }

    listener = jxta_listener_new((Jxta_listener_func) discovery_service_srdi_listener, discovery, 1, 200);
    jxta_listener_start(listener);

    status = jxta_resolver_service_registerSrdiHandler(discovery->resolver, discovery->instanceName, listener);
    discovery->my_listeners[1] = listener;

    /*
     * Register to RDV service fro RDV events
     */
    listener = jxta_listener_new((Jxta_listener_func) discovery_service_rdv_listener, discovery, 1, 1);

    jxta_listener_start(listener);

    status = jxta_rdv_service_add_event_listener(discovery->rdv,
                                                 (char *) jstring_get_string(discovery->instanceName),
                                                 (char *) jstring_get_string(discovery->gid_str), listener);
    discovery->my_listeners[2] = listener;

    /* Create vector and hashtable for the listeners */
    discovery->listener_vec = jxta_vector_new(1);
    discovery->listeners = jxta_hashtable_new(1);

    /* Mark the service as running now. */
    discovery->running = TRUE;
    return status;
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param discovery a pointer to the instance of the Discovery Service
 * @param argv a vector of string arguments.
 */
static Jxta_status start(Jxta_module * discovery, const char *argv[])
{
    Jxta_discovery_service_ref * me = (Jxta_discovery_service_ref *) discovery;
    Jxta_PA *padv;


    jxta_PG_get_PA(me->group, &padv);

    publish((Jxta_discovery_service *) me, (Jxta_advertisement *) padv, DISC_PEER, PA_EXPIRATION_LIFE,
            PA_REMOTE_EXPIRATION);
    JXTA_OBJECT_RELEASE(padv);

    /* start an SRDI push and Cache clean up function */
    advertisement_handling_func(NULL, me);

    return JXTA_SUCCESS;
}

/**
 * Stops an instance of the Discovery Service.
 * 
 * @param service a pointer to the instance of the Discovery Service
 * @return Jxta_status
 */
static void stop(Jxta_module * module)
{
    apr_status_t status;
    int i;
    apr_thread_pool_t *tp;

    Jxta_discovery_service_ref *discovery = PTValid(module, Jxta_discovery_service_ref);

    /* Test arguments first */
    if (discovery == NULL) {
        /* Invalid args. */
        return;
    }
    jxta_service_lock((Jxta_service*) discovery);
    /* We need to tell the background thread that it has to die. */
    discovery->running = FALSE;

    tp = jxta_PG_thread_pool_get(discovery->group);
    if (NULL != tp) {
        apr_thread_pool_tasks_cancel(tp, discovery);
    }
    jxta_service_unlock((Jxta_service*) discovery);

    for (i = 0; i < 3; i++) {
        jxta_listener_stop(discovery->my_listeners[i]);
    }
    status = jxta_resolver_service_unregisterQueryHandler(discovery->resolver, discovery->instanceName);
    status = jxta_resolver_service_unregisterResHandler(discovery->resolver, discovery->instanceName);
    status = jxta_resolver_service_unregisterSrdiHandler(discovery->resolver, discovery->instanceName);

    jxta_rdv_service_remove_event_listener(discovery->rdv,
                                           (char *) jstring_get_string(discovery->instanceName),
                                           (char *) jstring_get_string(discovery->gid_str));
    for (i = 0; i < 3; i++) {
        JXTA_OBJECT_RELEASE(discovery->my_listeners[i]);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

/*
 * service methods 
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Discovery Service
 * @return the advertisement.
 */
static void get_mia(Jxta_service * discovery, Jxta_advertisement ** mia)
{
    Jxta_discovery_service_ref *self = PTValid(discovery, Jxta_discovery_service_ref);

    JXTA_OBJECT_SHARE(self->impl_adv);
    *mia = self->impl_adv;
}

/**
 * return an object that proxies this service. (a service method).
 *
 * @return this service object itself.
 */
static void get_interface(Jxta_service * self, Jxta_service ** svc)
{
    Jxta_discovery_service_ref* myself = PTValid(self, Jxta_discovery_service_ref);

    JXTA_OBJECT_SHARE(myself);
    *svc = self;
}

static Jxta_status discovery_service_forward_from_local(Jxta_discovery_service_ref * discovery, ResolverQuery * res_query, const
                                                        char *query)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_query_context *jContext = NULL;
    JString *replicaExpression = NULL;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";

    if (NULL != query) {
        /* create a query context */
        jContext = jxta_query_new(query);
        if (jContext == NULL) {
            status = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }
        /* compile the XPath statement */
        status = jxta_query_XPath(jContext, xmlString, FALSE);
    }

    replicaExpression = jstring_new_0();
    if (JXTA_SUCCESS == status) {
        Jxta_vector *srdi_results;

        if (NULL != query) {
            status = query_srdi(discovery, jContext, NULL, &srdi_results);
            if (JXTA_ITEM_NOTFOUND == status) {
                status = query_replica(discovery, jContext, &srdi_results);
            } else {
                /* decrease hopcount before forward, so that the hop count remains unchanged.
                   because we know the peer published this index should able to answer the query.
                   This is a hack to fix the hop count could be 4 for complex query as an additional stop at the replacating rdv
                 */
                jxta_resolver_query_set_hopcount(res_query, jxta_resolver_query_get_hopcount(res_query) - 1);
            }
        } else {
            srdi_results = jxta_srdi_searchSrdi(discovery->srdi, jstring_get_string(discovery->instanceName), NULL, NULL, NULL);
        }
        if (NULL != srdi_results && jxta_vector_size(srdi_results) > 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "forward the query \n");
            status = jxta_srdi_forwardQuery_peers(discovery->srdi, discovery->resolver, srdi_results, res_query);
            JXTA_OBJECT_RELEASE(srdi_results);
        } else {
            status = discovery_service_send_to_replica(discovery, res_query, replicaExpression, jContext);
        }
    }
  FINAL_EXIT:
    if (jContext)
        JXTA_OBJECT_RELEASE(jContext);
    if (replicaExpression)
        JXTA_OBJECT_RELEASE(replicaExpression);
    return status;
}

static JString* build_extended_query(Jxta_discovery_query * dq)
{
    JString * eq;
    JString * attr;
    JString * val;

    eq = jstring_new_2("/");
    jstring_append_2(eq, xmltype[jxta_discovery_query_get_type(dq)]);
    jxta_discovery_query_get_attr(dq, &attr);
    jxta_discovery_query_get_value(dq, &val);

    if (NULL != attr && 0 != jstring_length(attr)) {
        jstring_append_2(eq, "[");
        jstring_append_1(eq, attr);
        jstring_append_2(eq, "=");
        if (NULL != val) {
            jstring_append_2(eq, "\"");
            jstring_append_1(eq, val);
            jstring_append_2(eq, "\"]");
        } else {
            jstring_append_2(eq, "*]");
        }
    }
    JXTA_OBJECT_RELEASE(attr);
    JXTA_OBJECT_RELEASE(val);
    return eq;
}

/** 
 * 
**/
long remoteQuery(Jxta_discovery_service * service, Jxta_id * peerid, Jxta_discovery_query *query,
                    Jxta_discovery_listener * listener)
{
    Jxta_status status;
    long qid = 0;
    JString *qdoc = NULL;
    ResolverQuery *rq = NULL;
    char buf[128];              /* More than enough */
    JString * ext_query;
    const Jxta_qos * qos;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    status = jxta_discovery_query_get_xml(query, &qdoc);
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remoteQuery failed to construct query: %ld\n", status);
        qid = -1;
        goto FINAL_EXIT;
    }
    status = jxta_resolver_service_create_query(discovery->resolver, discovery->instanceName, qdoc, &rq);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remoteQuery failed to create resolver query: %ld\n", status);
        goto FINAL_EXIT;
    }

    qos = jxta_discovery_query_qos(query);
    if (!qos) {
        status = jxta_service_default_qos((Jxta_service*) service, peerid ? peerid : jxta_id_nullID, &qos);
    }
    if (JXTA_ITEM_NOTFOUND == status) {
        status = jxta_service_default_qos((Jxta_service*) service, NULL, &qos);
    }
    jxta_resolver_query_attach_qos(rq, qos);

    qid = jxta_resolver_query_get_queryid(rq);
    if (listener != NULL) {
        apr_snprintf(buf, 128, "%ld", qid);
        jxta_hashtable_put(discovery->listeners, buf, strlen(buf), (Jxta_object *) listener);
    }

    if (!jxta_rdv_service_is_rendezvous(discovery->rdv)) {
        status = jxta_resolver_service_sendQuery(discovery->resolver, rq, peerid);
    } else {
        jxta_discovery_query_get_extended_query(query, &ext_query);
        if (!ext_query) {
            ext_query = build_extended_query(query);
        }
        status = discovery_service_forward_from_local(discovery, rq, jstring_get_string(ext_query));
        JXTA_OBJECT_RELEASE(ext_query);
        if (JXTA_ITEM_NOTFOUND == status) {
            jxta_resolver_query_set_hopcount(rq, 1);
            status = jxta_resolver_service_sendQuery(discovery->resolver, rq, NULL);
        }
    }
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remoteQuery failed to send query: %ld\n", status);
        jxta_hashtable_del(discovery->listeners, buf, strlen(buf), NULL);
        goto FINAL_EXIT;
    }

  FINAL_EXIT:
    if (rq)
        JXTA_OBJECT_RELEASE(rq);
    if (qdoc)
        JXTA_OBJECT_RELEASE(qdoc);
    return qid;
}

Jxta_status cancelRemoteQuery(Jxta_discovery_service * me, long query_id, Jxta_discovery_listener ** listener)
{
    Jxta_discovery_service_ref *me2 = (Jxta_discovery_service_ref *) me;
    char buf[128];

    apr_snprintf(buf, sizeof(buf), "%ld", query_id);
    return jxta_hashtable_del(me2->listeners, (const void *) buf, strlen(buf), (Jxta_object **) listener);
}

Jxta_status getLocalAdv(Jxta_discovery_service * service, short type, const char *aattribute, const char *value,
                        Jxta_vector ** advertisements)
{
    char **keys;
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    int i = 0;
    Jxta_discovery_service_ref *discovery = PTValid(service, Jxta_discovery_service_ref);


    if (FOLDER_MAX <= type || FOLDER_MIN > type) {
        type = FOLDER_DEFAULT;
    }

    if (aattribute == NULL || aattribute[0] == 0) {
        keys = cm_get_primary_keys(discovery->cm, (char *) dirname[type], FALSE);
    } else {

        keys = cm_search(discovery->cm, (char *) dirname[type], aattribute, value, FALSE, INT_MAX);
    }

    if ((keys == NULL) || (keys[0] == NULL)) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No items found \n");

        /*
         * The convention so far seems to be that returning NULL
         * is the way to say "nothing found. Since we always
         * return JXTA_SUCCESS, we'd better set NULL forcefully,
         * otherwise the reading of the result would always depend on
         * the initial value of the return ptr, which is bad practice.
         */

        *advertisements = NULL;

        free(keys);
        return JXTA_SUCCESS;
    }

    while (keys[i]) {
        JString *res_bytes = NULL;
        Jxta_advertisement *root_adv = NULL;
        Jxta_vector *res_vect = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "retrieving %s \n", keys[i]);
        status = cm_restore_bytes(discovery->cm, (char *) dirname[type], keys[i], &res_bytes);
        free(keys[i]);
        if (status != JXTA_SUCCESS) {
            ++i;
            continue;
        }
        root_adv = jxta_advertisement_new();
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Parsing response : %s \n", jstring_get_string(res_bytes) );
        
        status = jxta_advertisement_parse_charbuffer(root_adv, jstring_get_string(res_bytes), jstring_length(res_bytes));
        JXTA_OBJECT_RELEASE(res_bytes);
        if (status != JXTA_SUCCESS) {
            ++i;
            continue;
        }

        jxta_advertisement_get_advs(root_adv, &res_vect);
        JXTA_OBJECT_RELEASE(root_adv);
        if (res_vect == NULL) {
            ++i;
            continue;
        }
        if (jxta_vector_size(res_vect) != 0) {

            /*
             * Since we're supposed to return NULL when we find nothing,
             * we have to create the vector only if we get at least one
             * successfully processed result.
             */
            if (responses == NULL) {
                responses = jxta_vector_new(0);
            }

            jxta_vector_addall_objects_last(responses, res_vect);
        }
        JXTA_OBJECT_RELEASE(res_vect);
        i++;
    }

    free(keys);

    *advertisements = responses;
    return JXTA_SUCCESS;
}

static Jxta_status queryLocal(Jxta_discovery_service_ref * discovery, Jxta_credential *scope[],
                                        Jxta_query_context * jContext, Jxta_vector ** advertisements, int threshold, Jxta_boolean ads_only)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_cache_entry **entries;
    Jxta_vector *responses;
    int i;
    unsigned int j;

    assert(TRUE == jContext->compound_query);
    responses = jxta_vector_new(1);

    for (j = 0; j < jxta_vector_size(jContext->queries); j++) {
        Jxta_query_element *elem;

        jxta_vector_get_object_at(jContext->queries, JXTA_OBJECT_PPTR(&elem), j);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s elem - %s \n", jstring_get_string(elem->jBoolean),
                        jstring_get_string(elem->jSQL));
        JXTA_OBJECT_RELEASE(elem);
    }

    entries = cm_query_ctx(discovery->cm, scope, threshold, jContext);

    i = 0;
    while (entries != NULL && entries[i] != NULL) {
        Jxta_advertisement * adv;
        Jxta_cache_entry * cache_entry;
        Jxta_vector * adv_vec=NULL;

        cache_entry = entries[i++];
        adv = cache_entry->adv;

        jxta_advertisement_get_advs(adv, &adv_vec);

        for (j=0; NULL != adv_vec && j<jxta_vector_size(adv_vec); j++) {
            Jxta_advertisement * advertisement;
            jxta_vector_get_object_at(adv_vec, JXTA_OBJECT_PPTR(&advertisement), j);
            if (ads_only) {
                jxta_vector_add_object_last(responses, (Jxta_object *) advertisement);
            } else {
                Jxta_query_result * qres = NULL;
                qres = calloc(1, sizeof(Jxta_query_result));
                JXTA_OBJECT_INIT(qres, discovery_result_free, NULL);
                qres->adv = JXTA_OBJECT_SHARE(advertisement);
                qres->lifetime = cache_entry->lifetime;
                qres->expiration = cache_entry->expiration;
                jxta_vector_add_object_last(responses, (Jxta_object *) qres);
                JXTA_OBJECT_RELEASE(qres);
            }
            JXTA_OBJECT_RELEASE(advertisement);
        }
        if (NULL != adv_vec)
            JXTA_OBJECT_RELEASE(adv_vec);
        if (NULL != cache_entry)
            JXTA_OBJECT_RELEASE(cache_entry);
    }

    if (entries)
        free(entries);
    *advertisements = responses;
    return status;
}

static Jxta_status authorizeGroup(Jxta_PG * check_group, Jxta_credential *groupCred, Jxta_credential * cred)
{
    Jxta_status status = JXTA_SUCCESS;
#ifdef UNUSED_VWF
    Jxta_PG * pg=NULL;
    Jxta_PG * parent=NULL;
    Jxta_boolean authorized=FALSE;
    Jxta_id *groupId=NULL;
#endif
/* DISABLED */
/*
    jxta_credential_get_peergroupid(groupCred, &groupId);
    jxta_lookup_group_instance(groupId, &pg);
    if (pg == check_group) {
        authorized = TRUE;
        status = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }
    jxta_PG_get_parentgroup(pg, &parent);
    authorized = FALSE;
    while (parent != NULL) {
        if (parent == check_group) {
            authorized = TRUE;
            break;
        }
        jxta_PG_get_parentgroup(pg, &parent);
        JXTA_OBJECT_RELEASE(pg);
        pg = parent;
    }

FINAL_EXIT:
    if (groupId)
        JXTA_OBJECT_RELEASE(groupId);
    if (pg)
        JXTA_OBJECT_RELEASE(pg);
    if (parent)
        JXTA_OBJECT_RELEASE(parent);
    if (!authorized) status = JXTA_FAILED;
*/
    return status;
}

Jxta_status getLocalGroupsQuery (Jxta_discovery_service * self, const char *query, Jxta_credential *scope[], Jxta_vector ** results, int threshold, Jxta_boolean ads_only)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) self;
    Jxta_status status = JXTA_FAILED;
    Jxta_query_context *jContext = NULL;
    Jxta_vector * rejects=NULL;
    Jxta_credential **l_scope=NULL;
    Jxta_credential **l_scope_save=NULL;

    int j=0;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";

    if (NULL == results) {
        return JXTA_INVALID_ARGUMENT;
    }

    /* create a query context */
    jContext = jxta_query_new(query);
    if (jContext == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Malformed query in getLocalGroupsQuery --- %s\n", query);
        return JXTA_INVALID_ARGUMENT;
    }

    /* compile the XPath statement */
    status = jxta_query_compound_XPath(jContext, xmlString, FALSE);
    if (JXTA_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot compile query in getLocalGroupsQuery --- %s\n", query);
        goto FINAL_EXIT;
    }
    l_scope = scope;
    /* determine the scope of the query */
    while (NULL != l_scope && *l_scope) {
        /* DISABLE */
        if (TRUE) break;
        j++;
        status = authorizeGroup(discovery->group, *l_scope, NULL);
        if (JXTA_SUCCESS != status) {
            JString * jgroupid;
            Jxta_id * gId;
            if (NULL == rejects) {
                rejects = jxta_vector_new(0);
            }
            if (NULL == rejects) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create rejects vector\n");
                continue;
            }
            status = jxta_vector_add_object_last(rejects, (Jxta_object *) *l_scope);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to add object to rejects vector\n");
                continue;
            }
            status = jxta_credential_get_peergroupid(*l_scope, &gId);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve peergroupid from credential\n");
                continue;
            }
            jxta_id_to_jstring(gId, &jgroupid);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "rejected group %s\n", jstring_get_string(jgroupid));
            JXTA_OBJECT_RELEASE(jgroupid);
            JXTA_OBJECT_RELEASE(gId);
        }
        l_scope++;
    }
    l_scope = NULL;
    if (rejects) {
        int newScopeLen;
        Jxta_credential ** tmp;
        newScopeLen = sizeof(Jxta_id *) * j;
        newScopeLen -= jxta_vector_size(rejects);
        l_scope = calloc(1, newScopeLen + 1);
        l_scope_save = l_scope;
        tmp = scope;
        while (*tmp) {
            Jxta_boolean reject = FALSE;
            int i;
            for (i=0; i < jxta_vector_size(rejects); i++) {
                Jxta_credential * cred = NULL;
                jxta_vector_get_object_at(rejects, JXTA_OBJECT_PPTR(&cred), i);
                if (*tmp == cred) {
                    reject = TRUE;
                }
                JXTA_OBJECT_RELEASE(cred);
                if (reject) break;
            }
            if (!reject) *l_scope++ = *tmp;
            tmp++;
        }
        *l_scope = NULL;
        scope = l_scope_save;
        JXTA_OBJECT_RELEASE(rejects);
    }

    status = queryLocal(discovery, scope, jContext, results, threshold, ads_only);
    if (NULL != l_scope_save)
        free(l_scope_save);

  FINAL_EXIT:
    if (jContext)
        JXTA_OBJECT_RELEASE(jContext);
    return status;
}

static Jxta_status getLocalQuery(Jxta_discovery_service * self, const char *query, Jxta_vector ** advertisements)
{
    return getLocalGroupsQuery(self, query, NULL, advertisements, INT_MAX, TRUE);
}


/**
 * Query the replica table with specified query, return a vector of Jxta_id of peer ID
 */
static Jxta_status query_replica(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, Jxta_vector ** peerids)
{
    Jxta_vector *peers; /* a vector of JString, each is a string of peer ID */

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Query replica: %s \n", jstring_get_string(jContext->queryNameSpace));
    peers = cm_query_replica(self->cm, jContext->queryNameSpace, jContext->queries);
    if (!peers) {
        *peerids = NULL;
        return JXTA_ITEM_NOTFOUND;
    }

    *peerids = getPeerids(peers);
    JXTA_OBJECT_RELEASE(peers);
    return (*peerids) ? JXTA_SUCCESS : JXTA_FAILED;
}

static Jxta_status query_srdi(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, Jxta_id * dropId, Jxta_vector ** peerids)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) self;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_cache_entry **cache_entries;
    Jxta_cache_entry **scan;
    Jxta_vector *responses = NULL;
    JString * jDrop = NULL;
    JString * sql_cmd = NULL;
    sql_cmd = jstring_new_0();
    jstring_append_1(sql_cmd, jContext->sqlcmd);
    if (NULL != dropId) {
        jxta_id_to_jstring(dropId, &jDrop);
        jstring_append_2(sql_cmd, SQL_AND CM_COL_Peerid SQL_NOT_EQUAL);
        SQL_VALUE(sql_cmd, jDrop);
        JXTA_OBJECT_RELEASE(jDrop);
    }
    cache_entries = cm_sql_query_srdi_ns(discovery->cm, jstring_get_string(jContext->queryNameSpace), sql_cmd);
    if (NULL == cache_entries || NULL == *cache_entries) {
        JXTA_OBJECT_RELEASE(sql_cmd);
        *peerids = NULL;
        return JXTA_ITEM_NOTFOUND;
    }

    /* build proffers to see if match the query */
    responses = jxta_vector_new(1);
    scan = cache_entries;
    while (scan != NULL && *scan) {
        Jxta_cache_entry *cache_entry;

        cache_entry = *scan;

        jxta_vector_add_object_last(responses, (Jxta_object *) cache_entry->profid);

        JXTA_OBJECT_RELEASE(cache_entry);
        scan++;
    }
    free(cache_entries);
    if (JXTA_SUCCESS == status) {
        *peerids = getPeerids(responses);
        if (!*peerids) {
            status = JXTA_ITEM_NOTFOUND;
        }
    } else {
        *peerids = NULL;
    }
    JXTA_OBJECT_RELEASE(sql_cmd);
    JXTA_OBJECT_RELEASE(responses);
    return status;
}

static Jxta_status publish_priv(Jxta_discovery_service * service, Jxta_advertisement * adv, short type,
                    Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers, Jxta_boolean update)
{
    int sstat = JXTA_FAILED;
    Jxta_id *id = NULL;
    JString *id_str = NULL;
    Jxta_status status = JXTA_FAILED;
    Jxta_discovery_service_ref *self = PTValid(service, Jxta_discovery_service_ref);

    if (FOLDER_MAX <= type || FOLDER_MIN > type) {
        type = FOLDER_DEFAULT;
    }

    id = jxta_advertisement_get_id(adv);
    if (id != NULL) {
        status = jxta_id_get_uniqueportion(id, &id_str);
        if (status != JXTA_SUCCESS) {
            /* something went wrong with getting the ID */
            JXTA_OBJECT_RELEASE(id);
            return status;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Publishing doc id of :%s \n", jstring_get_string(id_str));

    } else {
        /* Advertisement does not have an id, hash the document and use the
           hash as a key */
        JString *tmps = NULL;
        char *hashname = calloc(1, 128);
        long hash = 0;

        jxta_advertisement_get_xml((Jxta_advertisement *) adv, &tmps);
        if (tmps == NULL) {
            /* abort nothing to publish */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Error NULL Advertisement String\n");
            free(hashname);
            return JXTA_FAILED;
        }
        hash = cm_hash(jstring_get_string(tmps), jstring_length(tmps));
        apr_snprintf(hashname, 128, "%lu", hash);
        id_str = jstring_new_2(hashname);
        free(hashname);
        JXTA_OBJECT_RELEASE(tmps);
    }
    sstat = cm_save(self->cm, (char *) dirname[type], (char *) jstring_get_string(id_str), adv, lifetime, lifetimeForOthers, update);
    JXTA_OBJECT_RELEASE(id);
    JXTA_OBJECT_RELEASE(id_str);
    return sstat;
}

Jxta_status publish(Jxta_discovery_service * service, Jxta_advertisement * adv, short type,
                    Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers)
{
    return publish_priv(service, adv, type, lifetime, lifetimeForOthers, TRUE);
}

Jxta_status remotePublish(Jxta_discovery_service * service, Jxta_id * peerid, Jxta_advertisement * adv,
                          short type, Jxta_expiration_time expirationtime, const Jxta_qos * qos)
{
    Jxta_status status;
    JString *peerdoc = NULL;
    JString *rdoc = NULL;
    Jxta_DiscoveryResponse *response = NULL;
    ResolverResponse *res_response = NULL;
    Jxta_PA *padv = NULL;
    Jxta_vector *vec = jxta_vector_new(1);
    Jxta_DiscoveryResponseElement *anElement = NULL;
    JString *tmpString;

    Jxta_discovery_service_ref *discovery = PTValid(service, Jxta_discovery_service_ref);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Remote publish starts\n");

    JXTA_OBJECT_CHECK_VALID(service);
    if (peerid) {
        JXTA_OBJECT_CHECK_VALID(peerid);
    }
    JXTA_OBJECT_CHECK_VALID(adv);
    jxta_PG_get_PA(discovery->group, &padv);
    status = jxta_PA_get_xml(padv, &peerdoc);
    JXTA_OBJECT_RELEASE(padv);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remotePublish failed to retrieve MyPeer Advertisement\n");
        return -1;
    }

    jxta_advertisement_get_xml((Jxta_advertisement *) adv, &tmpString);
    anElement = jxta_discovery_response_new_element_1(tmpString, expirationtime);

    jxta_vector_add_object_last(vec, (Jxta_object *) anElement);

    JXTA_OBJECT_RELEASE(anElement);
    JXTA_OBJECT_RELEASE(tmpString);

    response = jxta_discovery_response_new_1(type, NULL, NULL, 1, peerdoc, vec);
    JXTA_OBJECT_RELEASE(peerdoc);
    JXTA_OBJECT_RELEASE(vec);

    status = jxta_discovery_response_get_xml(response, &rdoc);
    JXTA_OBJECT_RELEASE(response);

    if (status == JXTA_SUCCESS) {
        res_response = jxta_resolver_response_new_2(discovery->instanceName, rdoc,
                                                    /* remote publish is always a qid of 0 */
                                                    0, discovery->localPeerId);
        if (!qos) {
            status = jxta_service_default_qos((Jxta_service*) service, peerid ? peerid : jxta_id_nullID, &qos);
        }
        if (JXTA_ITEM_NOTFOUND == status) {
            status = jxta_service_default_qos((Jxta_service*) service, NULL, &qos);
        }
        jxta_resolver_response_attach_qos(res_response, qos);

        JXTA_OBJECT_RELEASE(rdoc);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending remote publish request\n");
        status = jxta_resolver_service_sendResponse(discovery->resolver, res_response, peerid);
        JXTA_OBJECT_RELEASE(res_response);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remotePublish failed to construct response: %ld\n",
                        status);
        return JXTA_FAILED;
    }
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remotePublish failed to send response: %ld\n", status);
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

Jxta_status flush(Jxta_discovery_service * service, char *id, short type)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;
    Jxta_status sstat = JXTA_FAILED;

    Jxta_discovery_service_ref* self = PTValid(discovery, Jxta_discovery_service_ref);

    if (FOLDER_MAX <= type || FOLDER_MIN > type) {
        type = FOLDER_DEFAULT;
    }

    if (id != NULL) {
        sstat = cm_remove_advertisement(self->cm, (char *) dirname[type], id);
    } else {
        char **keys = cm_get_primary_keys(self->cm, (char *) dirname[type], FALSE);
        if (NULL != keys) {
            char **keysSave;
            sstat = cm_remove_advertisements(self->cm, (char *) dirname[type], keys);
            keysSave = keys;
            while (*keys)
                free(*keys++);
            free(keysSave);
        }
    }
    return sstat;
}

Jxta_status getLifetime(Jxta_discovery_service * service, short type, Jxta_id * advId, Jxta_expiration_time * eexp)
{
    Jxta_status status;
    JString *jId;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    status = jxta_id_get_uniqueportion(advId, &jId);
    if (status != JXTA_SUCCESS) {
        /* something went wrong with getting the ID */
        if (NULL != jId) {
            JXTA_OBJECT_RELEASE(jId);
        }
        return status;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Retrieving lifetime from id of :%s \n", jstring_get_string(jId));
    status = cm_get_expiration_time(discovery->cm, (char *) dirname[type], (char *) jstring_get_string(jId), eexp);
    JXTA_OBJECT_RELEASE(jId);

    return status;
}

Jxta_status getExpiration(Jxta_discovery_service * service, short type, Jxta_id * advId, Jxta_expiration_time * eexp)
{
    Jxta_status status;
    JString *jId;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    status = jxta_id_get_uniqueportion(advId, &jId);
    if (status != JXTA_SUCCESS) {
        /* something went wrong with getting the ID */
        if (NULL != jId) {
            JXTA_OBJECT_RELEASE(jId);
        }
        return status;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Retrieving expiration from id of :%s \n", jstring_get_string(jId));
    status = cm_get_others_time(discovery->cm, (char *) dirname[type], (char *) jstring_get_string(jId), eexp);
    JXTA_OBJECT_RELEASE(jId);

    return status;
}

Jxta_status add_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    return (jxta_vector_add_object_last(discovery->listener_vec, (Jxta_object *) listener));
}

Jxta_status remove_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
{
    Jxta_discovery_service_ref *me = (Jxta_discovery_service_ref *) service;

    if (!service) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (jxta_vector_remove_object(me->listener_vec, (Jxta_object *) listener) < 0) {
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_discovery_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_discovery_service_methods -> Jxta_discovery_service_ref_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_discovery_service_methods Jxta_discovery_service_ref_methods;

Jxta_discovery_service_ref_methods jxta_discovery_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      init,
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface,
     service_on_option_set},
    "Jxta_discovery_service_methods",
    remoteQuery,
    cancelRemoteQuery,
    getLocalAdv,
    getLocalQuery,
    publish,
    remotePublish,
    flush,
    add_listener,
    remove_listener,
    getLifetime,
    getExpiration
};

void jxta_discovery_service_ref_construct(Jxta_discovery_service_ref * discovery, Jxta_discovery_service_ref_methods * methods)
{
    /*
     * we do not extend Jxta_discovery_service_methods; so the type string
     * is that of the base table
     */
    Jxta_discovery_service_methods* service_methods = PTValid(methods, Jxta_discovery_service_methods);

    jxta_discovery_service_construct((Jxta_discovery_service *) discovery, (Jxta_discovery_service_methods *) service_methods);

    /* Set our rt type checking string */
    discovery->thisType = "Jxta_discovery_service_ref";
}

void jxta_discovery_service_ref_destruct(Jxta_discovery_service_ref * discovery)
{
    Jxta_discovery_service_ref * self = PTValid(discovery, Jxta_discovery_service_ref);

    /* release/free/destroy our own stuff */

    if (self->resolver != NULL) {
        JXTA_OBJECT_RELEASE(self->resolver);
    }
    if (self->endpoint != NULL) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }
    if (self->localPeerId != NULL) {
        JXTA_OBJECT_RELEASE(self->localPeerId);
    }
    if (self->pid_str != NULL) {
        free(self->pid_str);
    }
    if (self->srdi != NULL) {
        JXTA_OBJECT_RELEASE(self->srdi);
    }
    if (self->instanceName != NULL) {
        JXTA_OBJECT_RELEASE(self->instanceName);
    }

    self->group = NULL;

    if (self->listener_vec != NULL) {
        JXTA_OBJECT_RELEASE(self->listener_vec);
    }
    if (self->listeners != NULL) {
        JXTA_OBJECT_RELEASE(self->listeners);
    }
    if (self->impl_adv != NULL) {
        JXTA_OBJECT_RELEASE(self->impl_adv);
    }
    if (self->assigned_id != NULL) {
        JXTA_OBJECT_RELEASE(self->assigned_id);
    }
    if (NULL != discovery->rdv) {
        JXTA_OBJECT_RELEASE(self->rdv);
    }
    if (NULL != discovery->config) {
        JXTA_OBJECT_RELEASE(self->config);
    }
    JXTA_OBJECT_RELEASE(self->gid_str);

    if (self->cm != NULL) {
        JXTA_OBJECT_RELEASE(self->cm);
    }

    /* call the base classe's destructor. */
    jxta_discovery_service_destruct((Jxta_discovery_service *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction of Discovery finished\n");
}

/**
 * Frees an instance of the Discovery Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Discovery Service to free.
 */
static void discovery_free(Jxta_object *service)
{
    /* call the hierarchy of dtors */
    jxta_discovery_service_ref_destruct((Jxta_discovery_service_ref *) service);

    /* free the object itself */
    free(service);
}

/**
 * Creates a new instance of the Discovery Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_discovery_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Discovery Service.
 * @return a non initialized instance of the Discovery Service
 */

Jxta_discovery_service_ref *jxta_discovery_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_discovery_service_ref *discovery;

    discovery = (Jxta_discovery_service_ref *) malloc(sizeof(Jxta_discovery_service_ref));

    if (discovery == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(discovery, 0, sizeof(Jxta_discovery_service_ref));
    JXTA_OBJECT_INIT(discovery, discovery_free, 0);

    /* call the hierarchy of ctors */
    jxta_discovery_service_ref_construct(discovery, &jxta_discovery_service_ref_methods);

    /* return the new object */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "New Discovery\n");
    return discovery;
}

/* END OF STANDARD SERVICE IMPLEMENTATION CODE */
static Jxta_status discovery_service_send_to_replica(Jxta_discovery_service_ref * discovery,
                                                     ResolverQuery * rq, JString * replicaExpression,
                                                     Jxta_query_context * jContext)
{
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i;
    Jxta_hashtable *peerHash = NULL;
    const char *thisIdChar = NULL;
    Jxta_vector *peerReplicas = NULL;
    Jxta_boolean walk_it = FALSE;
    Jxta_peer *replicaPeer = NULL;
    long hc = -1;

    thisIdChar = discovery->pid_str;
    hc = jxta_resolver_query_get_hopcount(rq);
    if (hc == 0) {
        for (i = 0; NULL != jContext && jxta_vector_size(jContext->queries) > i; i++) {
            const char *expression;
            JString *jRepId = NULL;
            Jxta_id *repId = NULL;
            Jxta_query_element *elem = NULL;

            replicaPeer = NULL;
            jxta_vector_get_object_at(jContext->queries, JXTA_OBJECT_PPTR(&elem), i);
            if (!elem->isReplicated) {
                JXTA_OBJECT_RELEASE(elem);
                continue;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "%s elem - %s \n", jstring_get_string(elem->jBoolean),
                            jstring_get_string(elem->jSQL));
            if (elem->isNumeric) {
                Jxta_range *rge;
                const char *val;

                rge = jxta_range_new_1(jstring_get_string(jContext->queryNameSpace), jstring_get_string(elem->jName));
                val = jstring_get_string(elem->jValue);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "get a numeric replica peer\n");
                replicaPeer = jxta_srdi_getNumericReplica(discovery->srdi, rge, NULL);
                JXTA_OBJECT_RELEASE(rge);
            } else {
                if (!elem->hasWildcard) {
                    jstring_reset(replicaExpression, NULL);
                    jstring_append_1(replicaExpression, elem->jName);
                    jstring_append_1(replicaExpression, elem->jValue);
                    expression = jstring_get_string(replicaExpression);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "get a replica peer: %s\n", expression);
                    replicaPeer = jxta_srdi_getReplicaPeer(discovery->srdi, expression);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Wildcard query\n");
                }
            }
            JXTA_OBJECT_RELEASE(elem);
            if (NULL != replicaPeer) {
                const char *repIdChar = NULL;
                Jxta_id *idTemp = NULL;

                jxta_peer_get_peerid(replicaPeer, &repId);
                jxta_id_to_jstring(repId, &jRepId);
                repIdChar = jstring_get_string(jRepId);
                if (strcmp(repIdChar, thisIdChar)) {
                    if (NULL == peerHash)
                        peerHash = jxta_hashtable_new(1);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding replica to search list - %s \n", repIdChar);
                    if (jxta_hashtable_get(peerHash, repIdChar, strlen(repIdChar), JXTA_OBJECT_PPTR(&idTemp)) != JXTA_SUCCESS) {
                        jxta_hashtable_put(peerHash, repIdChar, strlen(repIdChar), (Jxta_object *) repId);
                    }
                    if (NULL != idTemp)
                        JXTA_OBJECT_RELEASE(idTemp);
                }
                JXTA_OBJECT_RELEASE(jRepId);
                JXTA_OBJECT_RELEASE(repId);
                JXTA_OBJECT_RELEASE(replicaPeer);
            }
        }
        if (NULL != peerHash) {
            peerReplicas = jxta_hashtable_values_get(peerHash);
            JXTA_OBJECT_RELEASE(peerHash);
            for (i = 0; i < jxta_vector_size(peerReplicas); i++) {
                Jxta_id *id = NULL;
                JString *sTemp = NULL;

                jxta_vector_get_object_at(peerReplicas, JXTA_OBJECT_PPTR(&id), i);
                jxta_id_to_jstring(id, &sTemp);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward to replica %s\n", jstring_get_string(sTemp));
                jxta_srdi_forwardQuery_peer(discovery->srdi, discovery->resolver, id, rq);
                JXTA_OBJECT_RELEASE(id);
                JXTA_OBJECT_RELEASE(sTemp);
            }
            JXTA_OBJECT_RELEASE(peerReplicas);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Walk the query - no peers found \n");
            jxta_resolver_query_set_hopcount(rq, 1);
            walk_it = TRUE;
        }
    } else {
        hc++;
        jxta_resolver_query_set_hopcount(rq, hc);
        if (hc < 3) {
            walk_it = TRUE;
        }
    }
    if (walk_it) {
        status = JXTA_ITEM_NOTFOUND;
    }
    return status;
}

static Jxta_status JXTA_STDCALL discovery_service_query_listener(Jxta_object * obj, void *arg)
{
    JString *peeradv = NULL;
    JString *query = NULL;
    Jxta_DiscoveryQuery *dq = NULL;
    JString *attr = NULL;
    JString *val = NULL;
    JString *extendedQuery = NULL;
    JString *replicaExpression = NULL;
    Jxta_DiscoveryResponse *dr = NULL;
    ResolverResponse *res_response = NULL;
    JString *rdoc = NULL;
    char **keys = NULL;
    Jxta_vector *peers = NULL;
    JString *id_str = NULL;
    Jxta_id *id = NULL;
    Jxta_advertisement *foundAdv;
    Jxta_query_context *jContext = NULL;

    Jxta_PA *padv = NULL;
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    Jxta_status res = JXTA_ITEM_NOTFOUND;
    Jxta_id *rqId = NULL;
    long qid = -1;
    unsigned int i;
    int num_of_res = 0;
    int threshold = 0;
    ResolverQuery *rq = (ResolverQuery *) obj;
    Jxta_id *src_pid = NULL;
    Jxta_vector *qAdvs = NULL;
    short type = 0;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";
    Jxta_discovery_service_ref *discovery =  PTValid(arg, Jxta_discovery_service_ref);

    qid = jxta_resolver_query_get_queryid(rq);
    jxta_resolver_query_get_query(rq, &query);
    rqId = jxta_resolver_query_get_src_peer_id(rq);
    jxta_id_to_jstring(rqId, &id_str);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Discovery rcvd resolver query from: %s\n", jstring_get_string(id_str));
    JXTA_OBJECT_RELEASE(id_str);
    if (query == NULL) {
        /*we're done, nothing to proccess */
        return JXTA_SUCCESS;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "qid received %ld %s\n", qid, jstring_get_string(query));
    dq = jxta_discovery_query_new();
    status = jxta_discovery_query_parse_charbuffer(dq, jstring_get_string(query), jstring_length(query));
    JXTA_OBJECT_RELEASE(query);

    if (status != JXTA_SUCCESS) {
        /* we're done go no further */
        JXTA_OBJECT_RELEASE(dq);
        return JXTA_SUCCESS;
    }
    jxta_discovery_query_get_peeradv(dq, &peeradv);

    if (peeradv && jstring_length(peeradv) > 0) {
        padv = jxta_PA_new();
        jxta_PA_parse_charbuffer(padv, jstring_get_string(peeradv), jstring_length(peeradv));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Publishing querying Peer Adv.\n");
        src_pid = jxta_PA_get_PID(padv);
        if (src_pid != NULL) {
            JString *sourceID = NULL;
            const char *srcString;
            Jxta_boolean match = FALSE;

            jxta_id_to_jstring(src_pid, &sourceID);
            srcString = jstring_get_string(sourceID);
            if (!strcmp(srcString, discovery->pid_str)) {
                match = TRUE;
            }
            JXTA_OBJECT_RELEASE(sourceID);
            JXTA_OBJECT_RELEASE(src_pid);
            src_pid = NULL;
            if (TRUE == match)
                goto finishTop;
        }
        publish((Jxta_discovery_service *) discovery,
                (Jxta_advertisement *) padv, DISC_PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
    }
    jxta_discovery_query_get_attr(dq, &attr);
    jxta_discovery_query_get_value(dq, &val);
    jxta_discovery_query_get_extended_query(dq, &extendedQuery);
    type = jxta_discovery_query_get_type(dq);
    if (FOLDER_MAX <= type || FOLDER_MIN > type) {
        type = FOLDER_DEFAULT;
    }

    replicaExpression = jstring_new_0();

    if (attr && jstring_length(attr) != 0) {
        char *drName = (char *) dirname[type];
        char *at = (char *) jstring_get_string(attr);
        char *vl = (char *) jstring_get_string(val);
        JString *jSearch = NULL;

        jstring_reset(replicaExpression, NULL);
        jstring_append_2(replicaExpression, drName);
        jstring_append_2(replicaExpression, at);
        jstring_append_2(replicaExpression, vl);
        keys = cm_search(discovery->cm, drName, at, vl, TRUE, jxta_discovery_query_get_threshold(dq));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Search %s\n", jstring_get_string(replicaExpression));
        jSearch = jstring_new_2("/*:*[");
        jstring_append_1(jSearch, attr);
        jstring_append_2(jSearch, "=\"");
        jstring_append_1(jSearch, val);
        jstring_append_2(jSearch, "\"]");
        jContext = jxta_query_new(jstring_get_string(jSearch));
        JXTA_OBJECT_RELEASE(jSearch);
        status = jxta_query_XPath(jContext, xmlString, FALSE);
        if (status != JXTA_SUCCESS) {
            goto finishTop;
        }

    } else if (!extendedQuery || jstring_length(extendedQuery) == 0) {
        keys = cm_get_primary_keys(discovery->cm, (char *) dirname[type], TRUE);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Search everything \n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Search query %s\n", jstring_get_string(extendedQuery));
        /* create a query context */
        jContext = jxta_query_new(jstring_get_string(extendedQuery));

        if (jContext == NULL) {
            goto finishTop;
        }

        /* compile the XPath statement */
        status = jxta_query_compound_XPath(jContext, xmlString, FALSE);

        if (status != JXTA_SUCCESS) {
            goto finishTop;
        }

        status = queryLocal(discovery, NULL, jContext, &qAdvs, threshold, FALSE);
        if (status != JXTA_SUCCESS) {
            goto finishTop;
        }

        keys = (char **) calloc(1, sizeof(char *) * (jxta_vector_size(qAdvs) + 1));
        
        unsigned int found_count = 0;
        for (i = 0; i < jxta_vector_size(qAdvs); i++) {
            Jxta_query_result * qres = NULL;
            jxta_vector_get_object_at(qAdvs, JXTA_OBJECT_PPTR(&qres), i);
            if (qres->expiration == 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found a local advertisement but it is local only\n");
                JXTA_OBJECT_RELEASE(qres);
                continue;
            }
            foundAdv = JXTA_OBJECT_SHARE(qres->adv);
            JXTA_OBJECT_RELEASE(qres);
            id = jxta_advertisement_get_id(foundAdv);
            if (id != NULL) {
                status = jxta_id_get_uniqueportion(id, &id_str);
                JXTA_OBJECT_RELEASE(id);
                if (status != JXTA_SUCCESS) {
                    /* something went wrong with getting the ID */
                    goto finishTop;
                }
            } else {
                id_str = jstring_new_2(jxta_advertisement_get_local_id_string(foundAdv));
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found Id - %s.\n", jstring_get_string(id_str));
            keys[found_count++] = strdup(jstring_get_string(id_str));
            JXTA_OBJECT_RELEASE(id_str);
            JXTA_OBJECT_RELEASE(foundAdv);
        }
        JXTA_OBJECT_RELEASE(qAdvs);
        qAdvs = NULL;
        keys[found_count] = NULL;
        if (0 == found_count) {
            if (NULL != jContext)
                JXTA_OBJECT_RELEASE(jContext);
            jContext = jxta_query_new(jstring_get_string(extendedQuery));
            jxta_query_set_compound_table(jContext, CM_TBL_SRDI_SRC);
            jxta_query_compound_XPath(jContext, xmlString, FALSE);
        }
    }
    if (NULL == keys || NULL == keys[0]) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No keys locally \n");
        if (jxta_rdv_service_is_rendezvous(discovery->rdv) != TRUE) {
            goto finishTop;
        }
        if (NULL != jContext) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "query the SRDI\n");
            status = query_srdi(discovery, jContext, rqId, &peers);
            if (JXTA_ITEM_NOTFOUND == status) {
                status = query_replica(discovery, jContext, &peers);
            } else {
                /* decrease hopcount before forward, so that the hop count remains unchanged.
                   because we know the peer published this index should able to answer the query.
                   This is a hack to fix the hop count could be 4 for complex query as an additional stop at the replacating rdv
                 */
                jxta_resolver_query_set_hopcount(rq, jxta_resolver_query_get_hopcount(rq) - 1);
            }

        } else {
            peers = jxta_srdi_searchSrdi(discovery->srdi, jstring_get_string(discovery->instanceName), NULL,
                                         jstring_get_string(attr), jstring_get_string(val));
        }
        if (NULL != peers && jxta_vector_size(peers) > 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "forward the query \n");
            res = jxta_srdi_forwardQuery_peers(discovery->srdi, discovery->resolver, peers, rq);
        } else {
            res = discovery_service_send_to_replica(discovery, rq, replicaExpression, jContext);
        }
    }

  finishTop:

    if (NULL != rqId)
        JXTA_OBJECT_RELEASE(rqId);
    if (NULL != replicaExpression)
        JXTA_OBJECT_RELEASE(replicaExpression);
    if (NULL != extendedQuery)
        JXTA_OBJECT_RELEASE(extendedQuery);
    if (NULL != jContext)
        JXTA_OBJECT_RELEASE(jContext);
    if (NULL != peers)
        JXTA_OBJECT_RELEASE(peers);
    if (NULL != padv)
        JXTA_OBJECT_RELEASE(padv);
    if (NULL != peeradv)
        JXTA_OBJECT_RELEASE(peeradv);
    if (NULL != qAdvs)
        JXTA_OBJECT_RELEASE(qAdvs);
    if ((NULL == keys) || (NULL == keys[0])) {
        /* we're done go no further */
        JXTA_OBJECT_RELEASE(dq);
        if (NULL != attr)
            JXTA_OBJECT_RELEASE(attr);
        if (NULL != val)
            JXTA_OBJECT_RELEASE(val);
        if (NULL != keys)
            free(keys);
        return res;
    }

    /* construct response and send it back */
    i = 0;
    responses = jxta_vector_new(1);
    threshold = jxta_discovery_query_get_threshold(dq);
    while (keys[i]) {
        JString *responseString = NULL;
        Jxta_expiration_time lifetime = 0;
        Jxta_expiration_time expiration = 0;
        Jxta_expiration_time responseTime = 0;
        Jxta_DiscoveryResponseElement *dre = NULL;

        status = cm_get_expiration_time(discovery->cm, (char *) dirname[type], keys[i], &lifetime);
        /*
         * Make sure that the entry did not expire
         */
        if ((status != JXTA_SUCCESS) || (lifetime == 0)
            || (lifetime < (Jxta_expiration_time) jpr_time_now())) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Expired Document :%s :%" APR_INT64_T_FMT " time now: %"
                            APR_INT64_T_FMT "\n", keys[i], lifetime, jpr_time_now());
            free(keys[i++]);
            continue;
        }
        status = cm_restore_bytes(discovery->cm, (char *) dirname[type], keys[i], &responseString);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error restoring bytes of advid %s\n", keys[i]);
            free(keys[i++]);
            continue;
        }
        /*
         * Make sure we are passing the delta remaining expiration
         * time not the absolute expiration time
         */

        status = cm_get_others_time(discovery->cm, (char *) dirname[type], keys[i], &expiration);
        /*
         * Make sure that it is a valid expiration time
         */
        if ((status != JXTA_SUCCESS) || (expiration <= 0)) {
            if (0 != expiration) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid Expiration Time :%" APR_INT64_T_FMT " for "
                            "Document :%s\n", expiration, keys[i]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Expiration of 0 found and no response sent\n");
            }
            free(keys[i++]);
            JXTA_OBJECT_RELEASE(responseString);
            continue;
        }

        if ((lifetime - (Jxta_expiration_time) jpr_time_now()) < expiration)
            responseTime = lifetime - jpr_time_now();
        else
            responseTime = expiration;

        dre = jxta_discovery_response_new_element_1(responseString, responseTime);
        jxta_vector_add_object_last(responses, (Jxta_object *) dre);
        /* The vector counts his reference. Release ours */
        JXTA_OBJECT_RELEASE(dre);

        /* The element counts his reference. Release ours */
        JXTA_OBJECT_RELEASE(responseString);

        free(keys[i]);
        i++;
        if ((i + 1) > (unsigned int) threshold) {
            while (keys[i]) {
                free(keys[i++]);
            }
            break;
        }
    }
    free(keys);

    num_of_res = jxta_vector_size(responses);
    if (0 == num_of_res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No responses being sent\n");
        goto FINAL_EXIT;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Number of responses being sent: %i\n", num_of_res);
    dr = jxta_discovery_response_new_1(jxta_discovery_query_get_type(dq), attr ? jstring_get_string(attr) : NULL,
                                       val ? jstring_get_string(val) : NULL, num_of_res, NULL, responses);

    status = jxta_discovery_response_get_xml(dr, &rdoc);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Response - %s \n", jstring_get_string(rdoc));
    JXTA_OBJECT_RELEASE(dr);

    if (status == JXTA_SUCCESS) {
        res_response =
            jxta_resolver_response_new_2(discovery->instanceName, rdoc, jxta_resolver_query_get_queryid(rq),
                                         discovery->localPeerId);
        JXTA_OBJECT_RELEASE(rdoc);

        src_pid = jxta_resolver_query_get_src_peer_id(rq);
        jxta_resolver_response_attach_qos(res_response, jxta_resolver_query_qos(rq));

        status = jxta_resolver_service_sendResponse(discovery->resolver, res_response, src_pid);
        JXTA_OBJECT_RELEASE(res_response);

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery::remotePublish failed to construct response: %ld\n",
                        status);
    }

  FINAL_EXIT:
    if (dq != NULL)
        JXTA_OBJECT_RELEASE(dq);
    if (attr != NULL)
        JXTA_OBJECT_RELEASE(attr);
    if (val != NULL)
        JXTA_OBJECT_RELEASE(val);
    if (responses)
        JXTA_OBJECT_RELEASE(responses);
    if (src_pid)
        JXTA_OBJECT_RELEASE(src_pid);
    return JXTA_SUCCESS;
}

Jxta_status discovery_send_srdi_msg(Jxta_discovery_service_ref *discovery, Jxta_id * peerid, Jxta_id *source_peerid, JString *jPrimaryKey, Jxta_vector * entries)
{
    Jxta_SRDIMessage *msg = NULL;
    JString *messageString = NULL;
    ResolverSrdi * srdi = NULL;
    Jxta_status res=JXTA_SUCCESS;

    msg = jxta_srdi_message_new_2(1, discovery->localPeerId, source_peerid, (char *) jstring_get_string(jPrimaryKey), entries);

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate jxta_srdi_message_new\n");
        goto FINAL_EXIT;
    }
    res = jxta_srdi_message_get_xml(msg, &messageString);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot serialize srdi message\n");
        goto FINAL_EXIT;
    }

    srdi = jxta_resolver_srdi_new_1(discovery->instanceName, messageString, peerid);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%s\n", jstring_get_string(messageString));
    JXTA_OBJECT_RELEASE(messageString);
    if (srdi == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a resolver srdi message\n");
        goto FINAL_EXIT;
    }

    res = jxta_resolver_service_sendSrdi(discovery->resolver, srdi, peerid);
FINAL_EXIT:
    if (srdi)
        JXTA_OBJECT_RELEASE(srdi);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    return res;
}

static Jxta_status discovery_filter_advid_duplicates(Jxta_discovery_service_ref *discovery, Jxta_vector * entries, Jxta_vector ** dupEntries)
{
    Jxta_status res = JXTA_SUCCESS;
    int i;

    assert(NULL != dupEntries);
    assert(NULL != entries);

    *dupEntries = jxta_vector_new(0);

    for (i=0; i<jxta_vector_size(entries); i++) {
        Jxta_SRDIEntryElement * entry;
        Jxta_peer *replicaPeer = NULL;
        res = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve srdi element res: %d\n", res);
            goto FINAL_EXIT;
        }
        if (entry->duplicate) {
            jxta_vector_add_object_last(*dupEntries, (Jxta_object *) entry);
            jxta_vector_remove_object_at(entries, NULL, i--);
        } else {
            replicaPeer = jxta_srdi_getReplicaPeer(discovery->srdi, jstring_get_string(entry->advId));

            if (NULL != replicaPeer && !jxta_id_equals(discovery->localPeerId, jxta_peer_peerid(replicaPeer))) {
                jxta_id_to_jstring(jxta_peer_peerid(replicaPeer), &entry->dup_peerid);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "got a replica peer for duplicate in SRDI entry element  %s\n", jstring_get_string(entry->dup_peerid));
            }
        }
        if (replicaPeer)
            JXTA_OBJECT_RELEASE(replicaPeer);
        JXTA_OBJECT_RELEASE(entry);
    }

FINAL_EXIT:

    return res;
}

static void JXTA_STDCALL discovery_service_srdi_listener(Jxta_object * obj, void *arg)
{
    Jxta_SRDIMessage *smsg;
    Jxta_vector *entries = NULL;
    Jxta_vector *msg_entries = NULL;
    Jxta_vector *resendDelta=NULL;
    Jxta_hashtable *resend_hash = NULL;
    Jxta_status status;
    JString *jPeerid = NULL;
    JString *jSrcPeerid = NULL;
    JString *jPrimaryKey = NULL;
    Jxta_peer *peer;
    Jxta_boolean bReplica = TRUE;
    Jxta_boolean am_rdv = FALSE;
    Jxta_id *peerid = NULL;
    Jxta_id *src_peerid = NULL;
    Jxta_rdv_service *rdv;
    unsigned int i;
    Jxta_discovery_service_ref *discovery = PTValid(arg, Jxta_discovery_service_ref);

    JXTA_OBJECT_CHECK_VALID(obj);

    rdv = discovery->rdv;

    am_rdv = jxta_rdv_service_is_rendezvous((Jxta_rdv_service *) rdv);
    smsg = jxta_srdi_message_new();

    status = jxta_srdi_message_parse_charbuffer(smsg, jstring_get_string((JString *) obj), jstring_length((JString *) obj));
    if( JXTA_SUCCESS != status ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed parsing SRDI message\n");
        goto FINAL_EXIT;
    }

    jxta_srdi_message_get_peerID(smsg, &peerid);
    jxta_id_to_jstring(peerid, &jPeerid);

    jxta_srdi_message_get_SrcPID(smsg, &src_peerid);
    if (NULL != src_peerid) {
        jxta_id_to_jstring(src_peerid, &jSrcPeerid);
    } else {
        jSrcPeerid = JXTA_OBJECT_SHARE(jPeerid);
    }

    jxta_srdi_message_get_primaryKey(smsg, &jPrimaryKey);

    if (jPeerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message without a peerid \n");
        goto FINAL_EXIT;
    }

    status = jxta_srdi_message_get_resend_entries(smsg, &entries);

    if (!am_rdv && entries == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message without resend entries and i'm not a rendezvous \n");
        goto FINAL_EXIT;
    }


    /* process resend entries */
    if (NULL != entries) {
        JString *peerid_j = NULL;
        if (!am_rdv) {
            peerid_j = jstring_new_2("NoPeer");
        }
        cm_get_resend_delta_entries(discovery->cm, NULL == peerid_j ? jPeerid : peerid_j, entries, &resend_hash);

        if (NULL != peerid_j)
            JXTA_OBJECT_RELEASE(peerid_j);

        if (NULL != resend_hash) {
            char **source_peers;
            char **source_peers_save;

            source_peers = jxta_hashtable_keys_get(resend_hash);
            source_peers_save = source_peers;

            /* Need to add source id to message since the same advertisement may have come from multiple sources */
            while (*source_peers) {
                Jxta_id * source_peerid=NULL;

                status = jxta_hashtable_get(resend_hash, *source_peers, strlen(*source_peers) + 1, JXTA_OBJECT_PPTR(&resendDelta));
                if (JXTA_SUCCESS != status) {
                    free(*(source_peers++));
                    continue;
                }
                
                jxta_id_from_cstr(&source_peerid, *source_peers);

                discovery_send_srdi_msg(discovery, peerid, source_peerid, jPrimaryKey, resendDelta);
                JXTA_OBJECT_RELEASE(source_peerid);
                JXTA_OBJECT_RELEASE(resendDelta);
                resendDelta = NULL;
                free(*(source_peers++));
            }
            free(source_peers_save);
            JXTA_OBJECT_RELEASE(resend_hash);
        }
        JXTA_OBJECT_RELEASE(entries);
        entries = NULL;
        if (!am_rdv)
            goto FINAL_EXIT;
    }

    status = jxta_srdi_message_get_entries(smsg, &msg_entries);


    if (status != JXTA_SUCCESS || NULL == msg_entries) {
        goto FINAL_EXIT;
    }

    status = cm_expand_delta_entries(discovery->cm, msg_entries, jPeerid, jSrcPeerid, &entries);

    resendDelta = NULL;

    for (i=0; i < jxta_vector_size(entries); i++) {
        Jxta_SRDIEntryElement * entry;
        status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        if (entry->resend) {
            if (NULL == resendDelta) {
                resendDelta = jxta_vector_new(0);
            }
            jxta_vector_add_object_last(resendDelta, (Jxta_object *) entry);
            jxta_vector_remove_object_at(entries, NULL, i--);
        }
        JXTA_OBJECT_RELEASE(entry);
    }
    if (NULL != resendDelta) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Request %d resend entries \n", jxta_vector_size(resendDelta));

        discovery_send_srdi_msg(discovery, peerid, src_peerid , jPrimaryKey, resendDelta);

        JXTA_OBJECT_RELEASE(resendDelta);
        resendDelta = NULL;
    }

    JXTA_OBJECT_RELEASE(msg_entries);

    if (0 == jxta_vector_size(entries)) {
        goto FINAL_EXIT;
    }
    jxta_srdi_message_set_entries(smsg, entries);

    /* is this from an edge ? */
    if (JXTA_SUCCESS == jxta_rdv_service_get_peer(discovery->rdv, peerid, &peer)) {
        bReplica = FALSE;
        JXTA_OBJECT_RELEASE(peer);
    }
    /* process normal srdi entries */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "got srdi with peerid -- %s and vector_size %i\n",
                    jstring_get_string(jPeerid)
                    , jxta_vector_size(entries));

    if (!bReplica) {
        jxta_srdi_replicateEntries(discovery->srdi, discovery->resolver, smsg, discovery->instanceName);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Number of srdi/replica elements received:%d %s\n",jxta_vector_size(entries), jstring_get_string((JString *) obj));

    if (NULL != discovery->cm && jxta_vector_size(entries) > 0) {
        Jxta_vector * resendEntries = NULL;
        Jxta_vector * dupEntries = NULL;


        if (!bReplica) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "calling cm save srdi with %s peerid\n",
                                jstring_get_string(jPeerid));
            status = cm_save_srdi_elements(discovery->cm, discovery->instanceName, jPeerid, jSrcPeerid, jPrimaryKey, entries, (Jxta_vector **) &resendEntries);
        } else {

            status = discovery_filter_advid_duplicates(discovery, entries, &dupEntries);
            if (JXTA_SUCCESS == status) {
                if (dupEntries && jxta_vector_size(dupEntries) > 0) {

                    /* duplicates are stored in the srdi cache */

                    status = cm_save_srdi_elements(discovery->cm, discovery->instanceName, jPeerid, jSrcPeerid, jPrimaryKey, dupEntries, (Jxta_vector **) &resendEntries);
                }

                if (entries && jxta_vector_size(entries) > 0) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "calling cm save replica with %s peerid\n",
                                jstring_get_string(jPeerid));
                    status = cm_save_replica_elements(discovery->cm, discovery->instanceName, jPeerid, jPrimaryKey, entries, (Jxta_vector **) &resendEntries);
                }
            }
        }
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to save %s in discovery_service_srdi_listener %d\n",
                                        bReplica == FALSE ? "srdi":"replica", status);
        }
        if (resendEntries) {
            discovery_send_srdi_msg(discovery, peerid, NULL, jPrimaryKey, resendEntries);
            JXTA_OBJECT_RELEASE(resendEntries);
        }
        if (dupEntries) {
            JXTA_OBJECT_RELEASE(dupEntries);
        }
    }
  FINAL_EXIT:
    if (peerid)
        JXTA_OBJECT_RELEASE(peerid);
    if (src_peerid)
        JXTA_OBJECT_RELEASE(src_peerid);
    if (jPeerid != NULL)
        JXTA_OBJECT_RELEASE(jPeerid);
    if (jSrcPeerid)
        JXTA_OBJECT_RELEASE(jSrcPeerid);
    if (entries != NULL)
        JXTA_OBJECT_RELEASE(entries);
    JXTA_OBJECT_RELEASE(smsg);
    JXTA_OBJECT_RELEASE(jPrimaryKey);
}

static Jxta_boolean process_response_listener(Jxta_discovery_service_ref * discovery, long qid, Jxta_DiscoveryResponse * res)
{
    char buf[128];      /* More than enough */
    Jxta_status status;
    Jxta_listener *listener = NULL;
    Jxta_boolean has_listener = FALSE;
    unsigned int i;

    apr_snprintf(buf, sizeof(buf), "%ld", qid);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Response received qid: %ld\n", qid);
    status = jxta_hashtable_get(discovery->listeners, buf, (size_t) strlen(buf), JXTA_OBJECT_PPTR(&listener));
    if (listener != NULL) {
        has_listener = TRUE;
        jxta_listener_process_object(listener, (Jxta_object *) res);
        JXTA_OBJECT_RELEASE(listener);
    }

    /* call general discovery listeners */
    for (i = 0; i < jxta_vector_size(discovery->listener_vec); i++) {
        status = jxta_vector_get_object_at(discovery->listener_vec, JXTA_OBJECT_PPTR(&listener), i);
        if (listener != NULL) {
            has_listener = TRUE;
            jxta_listener_process_object(listener, (Jxta_object *) res);
            JXTA_OBJECT_RELEASE(listener);
        }
    }
    return has_listener;
}

static void JXTA_STDCALL discovery_service_rdv_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) obj;
    Jxta_boolean republish = FALSE;
    Jxta_boolean delta = FALSE;
        /** Get info from the event */
    JString *string = NULL;
    int type = rdv_event->event;

    res = jxta_id_to_jstring(rdv_event->pid, &string);
    if (string == NULL) {
        if (rdv_event == NULL)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "rdv_event is NULL \n");
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Got a null in Discovery rdv listener with result %i \n", (int) res);
        return;
    }
    switch (type) {
    case JXTA_RDV_CLIENT_DISCONNECTED:
        /* flush the cache for the peer */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Disconnected to rdv peer in Discovery =%s\n",
                        jstring_get_string(string));
        break;
    case JXTA_RDV_CLIENT_CONNECTED:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rdv Client connected - %s \n", jstring_get_string(string));
        break;
    case JXTA_RDV_CONNECTED:
        republish = TRUE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Connected to rdv peer in Discovery =%s\n", jstring_get_string(string));
        break;
    case JXTA_RDV_FAILED:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed connection to rdv peer in Discovery=%s\n",
                        jstring_get_string(string));
        break;
    case JXTA_RDV_DISCONNECTED:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Disconnect from rdv peer in Discovery =%s\n",
                        jstring_get_string(string));
        break;
    case JXTA_RDV_RECONNECTED:
        republish = TRUE;
        delta = TRUE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Reconnect from rdv peer in Discovery %s\n", jstring_get_string(string));
        break;
    case JXTA_RDV_BECAME_EDGE:
        cm_set_delta(discovery->cm, TRUE);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous became edge in Discovery %s\n", jstring_get_string(string));
        break;
    case JXTA_RDV_BECAME_RDV:
        cm_set_delta(discovery->cm, TRUE);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Became Rendezvous in Discovery %s\n", jstring_get_string(string));
        break;
    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Event not caught in Discovery= %d\n", type);
    }

    /*
     * Update SRDI indexes for all advertisements in our cm 
     * to our new RDV
     */
    if (republish && jxta_rdv_service_is_rendezvous(discovery->rdv) != TRUE) {
        res = jxta_discovery_push_srdi(discovery, republish, delta);
    }
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to push SRDI entries to new RDV res=%i\n", res);
    }
    if (NULL != string) {
        JXTA_OBJECT_RELEASE(string);
    }
}

static void JXTA_STDCALL discovery_service_response_listener(Jxta_object * obj, void *arg)
{
    JString *response = NULL;
    Jxta_DiscoveryResponse *dr = NULL;
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    long qid = -1;
    Jxta_PA *peerAdv = NULL;
    Jxta_id *id = NULL;
    JString *jID = NULL;
    JString *jName = NULL;
    const char *sID = NULL;
    const char *sName = NULL;
    ResolverResponse *rr = (ResolverResponse *) obj;
    Jxta_id *rr_res_pid = NULL;
    JString *rr_res_pid_jstring = NULL;
    Jxta_advertisement *radv = NULL;

    Jxta_discovery_service_ref *discovery = PTValid(arg, Jxta_discovery_service_ref);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Discovery received a response\n");

    qid = jxta_resolver_response_get_queryid(rr);
    jxta_resolver_response_get_response(rr, &response);
    if (response == NULL) {
        /* we're done, nothing to proccess */
        return;
    }
    rr_res_pid = jxta_resolver_response_get_res_peer_id(rr);
    if (rr_res_pid != NULL) {
        jxta_id_to_jstring(rr_res_pid, &rr_res_pid_jstring);
        JXTA_OBJECT_RELEASE(rr_res_pid);
    }

    dr = jxta_discovery_response_new();
    discovery_response_set_query_id(dr, qid);
    discovery_response_set_discovery_service(dr, (Jxta_discovery_service*) discovery);
    jxta_discovery_response_parse_charbuffer(dr, jstring_get_string(response), jstring_length(response));
    JXTA_OBJECT_RELEASE(response);
    jxta_discovery_response_get_peer_advertisement(dr, (Jxta_advertisement **) JXTA_OBJECT_PPTR(&peerAdv));
    if (NULL != peerAdv) {
        id = jxta_PA_get_PID(peerAdv);
        jxta_id_to_jstring(id, &jID);
        jName = jxta_PA_get_Name(peerAdv);
        sID = jstring_get_string(jID);
        sName = jstring_get_string(jName);
        JXTA_OBJECT_RELEASE(peerAdv);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                        "Discovery rcvd from [%s] %d rsp(s) with qid:%ld\n pid: %s name:%s\n",
                        (rr_res_pid_jstring != NULL ? jstring_get_string(rr_res_pid_jstring) : "Unknown"),
                        jxta_discovery_response_get_count(dr), qid, sID, sName);
        /* prevent loopback */
        if (!strcmp(sID, discovery->pid_str)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Response ignored that originated at this peer\n");
            goto finish;
        }
    }
    status = jxta_discovery_response_get_responses(dr, &responses);
    /* if we have responses and no listeners - publish */
    if (NULL != responses && !process_response_listener(discovery, qid, dr)) {
        Jxta_vector *adv_vec = NULL;
        Jxta_DiscoveryResponseElement *res;
        unsigned int i = 0;

        for (i = 0; i < jxta_vector_size(responses); i++) {
            Jxta_object *tmpadv = NULL;

            res = NULL;
            status = jxta_vector_get_object_at(responses, JXTA_OBJECT_PPTR(&res), i);
            if (res != NULL) {
                radv = jxta_advertisement_new();
                jxta_advertisement_parse_charbuffer(radv, jstring_get_string(res->response), jstring_length(res->response));
                jxta_advertisement_get_advs(radv, &adv_vec);
                jxta_vector_get_object_at(adv_vec, &tmpadv, 0);
                if (tmpadv != NULL) {
                    /* if response expiration == 0 treat as DoS */
                    if (res->expiration > 0) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                                        "Publishing the advertisement time %" APR_INT64_T_FMT "\n", res->expiration);
                        publish_priv((Jxta_discovery_service *) discovery, (Jxta_advertisement *) tmpadv,
                                jxta_discovery_response_get_type(dr), res->expiration, LOCAL_ONLY_EXPIRATION, FALSE);
                    }
                    JXTA_OBJECT_RELEASE(tmpadv);
                }
                JXTA_OBJECT_RELEASE(adv_vec);
                JXTA_OBJECT_RELEASE(radv);
                JXTA_OBJECT_RELEASE(res);
            }
        }
    }

  finish:
    if(responses)
        JXTA_OBJECT_RELEASE(responses);
    if (id)
        JXTA_OBJECT_RELEASE(id);
    if (jID)
        JXTA_OBJECT_RELEASE(jID);
    if (jName)
        JXTA_OBJECT_RELEASE(jName);
    if (rr_res_pid_jstring)
        JXTA_OBJECT_RELEASE(rr_res_pid_jstring);
    /* clean up */
    JXTA_OBJECT_RELEASE(dr);
}

static void *APR_THREAD_FUNC advertisement_handling_func(apr_thread_t * thread, void *arg)
{
    Jxta_status res = 0;
    Jxta_time_diff wait_time=0;
    Jxta_time_diff expired_cycle=0;
    Jxta_time_diff delta_cycle=0;
    Jxta_time_diff expired_diff;
    Jxta_time_diff delta_diff;
    apr_thread_pool_t * tp;

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;

    /* As long as discovery is running */
    if (!discovery->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Discovery has left the building 2\n");
        return NULL;
    }
    delta_cycle = jxta_discovery_config_get_delta_update_cycle(discovery->config);
    expired_cycle = jxta_discovery_config_get_expired_adv_cycle(discovery->config);
    wait_time = delta_cycle > expired_cycle ? expired_cycle : delta_cycle;

    /* thread is null on initialization */
    if (NULL == thread) {
        goto FINAL_EXIT;
    }

    expired_diff = expired_cycle - discovery->expired_loop;
    delta_diff = delta_cycle - discovery->delta_loop;
    if (wait_time > (expired_diff) || wait_time > (delta_diff)) {
        wait_time = delta_diff > expired_diff ? expired_diff : delta_diff;
    }
    discovery->expired_loop += wait_time;
    discovery->delta_loop += wait_time;


    if (NULL == discovery->cm) {
        peergroup_get_cache_manager(discovery->group, &(discovery->cm));
    }
    if (NULL != discovery->cm) {
        if (discovery->expired_loop >= expired_cycle) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Discovery is removing expired records from the CM\n");
            cm_remove_expired_records(discovery->cm);
            discovery->expired_loop = 0;
        }
        /*
         * push delta if any
        */
        if (discovery->delta_loop >= delta_cycle) {
            res = jxta_discovery_push_srdi(discovery, TRUE, TRUE);
            if (TRUE == jxta_rdv_service_is_rendezvous((Jxta_rdv_service *) discovery->rdv)) {
                cm_set_delta(discovery->cm, TRUE);
            }

            discovery->delta_loop = 0;
        }
    }

FINAL_EXIT:

    tp = jxta_PG_thread_pool_get(discovery->group);

    if (NULL != tp) {
        apr_thread_pool_schedule(tp, advertisement_handling_func, discovery, wait_time * 1000, discovery); 
    }
    return NULL;
}

Jxta_status jxta_discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta)
{
    Jxta_status status;
    Jxta_vector *entries = NULL;
    int i = 0;

    if (delta && ppublish) {
        JString *name = NULL;
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "push SRDI delta %s\n", jstring_get_string(discovery->gid_str));

        for (i = 0; i < 3; i++) {
            Jxta_hashtable *peer_entries = NULL;

            cm_get_delta_entries_for_update(discovery->cm, dirname[i], NULL, &peer_entries);

            if (NULL != peer_entries) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send delta updates \n");
                discovery_service_send_delta_updates(discovery, dirname[i], peer_entries);
                JXTA_OBJECT_RELEASE(peer_entries);
            }
        }
        for (i = 0; i < 3; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = cm_get_srdi_delta_entries(discovery->cm, name);
            if (entries != NULL && jxta_vector_size(entries) > 0) {
                status = discovery_send_srdi((Jxta_discovery_service *) discovery, name, entries);
                if (JXTA_SUCCESS != status || FALSE == discovery->running) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to send send SRDI deltas %i\n", status);
                    break;
                }
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }
            JXTA_OBJECT_RELEASE(name);
            name = NULL;
        }
        if (NULL != name)
            JXTA_OBJECT_RELEASE(name);
    } else if (ppublish) {
        JString *name = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "push all CM SRDI indexes \n");
        for (i = FOLDER_MIN; i < FOLDER_MAX; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = cm_create_srdi_entries(discovery->cm, name, NULL, NULL);
            if (entries != NULL && jxta_vector_size(entries) > 0) {
                status = discovery_send_srdi((Jxta_discovery_service *) discovery, name, entries);
                if (JXTA_SUCCESS != status || FALSE == discovery->running) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to send send SRDI messages %i\n", status);
                    break;
                }
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }
            JXTA_OBJECT_RELEASE(name);
            name = NULL;
        }
    }
    if (NULL != entries)
        JXTA_OBJECT_RELEASE(entries);
    return JXTA_SUCCESS;
}

Jxta_status discovery_send_srdi(Jxta_discovery_service * me, JString * pkey, Jxta_vector * entries)
{
    Jxta_discovery_service_ref * discovery = (Jxta_discovery_service_ref *) me;
    Jxta_SRDIMessage *msg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    const char *cKey = jstring_get_string(pkey);

    if (jxta_vector_size(entries) == 0)
        return JXTA_SUCCESS;

    msg = jxta_srdi_message_new_1(1, discovery->localPeerId, (char *) cKey, entries);
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate jxta_srdi_message_new\n");
        return JXTA_NOMEM;
    }
    if ( config_rendezvous == jxta_rdv_service_config( discovery->rdv ) ) {
        res = jxta_srdi_replicateEntries(discovery->srdi, discovery->resolver, msg, discovery->instanceName);
    } else {
        res = jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->resolver, discovery->instanceName, msg, NULL);
    }
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot send srdi message\n");
    }
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status discovery_service_send_delta_updates(Jxta_discovery_service_ref * discovery, const char * key, Jxta_hashtable * source_entries_update)
{
    Jxta_status status = JXTA_SUCCESS;
    char ** source_peer_ids = NULL;
    char ** source_peer_ids_tmp = NULL;

    source_peer_ids = jxta_hashtable_keys_get(source_entries_update);
    source_peer_ids_tmp = source_peer_ids;

    while (*source_peer_ids_tmp) {
        Jxta_PID * source_id = NULL;
        JString * source_id_j = NULL;
        Jxta_hashtable *entries_update = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "getting entry for source id:%s \n", *source_peer_ids_tmp);

        status = jxta_hashtable_get(source_entries_update, *source_peer_ids_tmp, strlen(*source_peer_ids_tmp) + 1, JXTA_OBJECT_PPTR(&entries_update));
        if (JXTA_SUCCESS == status) {
            source_id_j = jstring_new_2(*source_peer_ids_tmp);
            jxta_id_from_jstring( &source_id, source_id_j);
            discovery_service_send_delta_update_msg(discovery, source_id, key, entries_update);

            JXTA_OBJECT_RELEASE(entries_update);
            JXTA_OBJECT_RELEASE(source_id);
            JXTA_OBJECT_RELEASE(source_id_j);
        }
        free(*(source_peer_ids_tmp++));
    }
    free(source_peer_ids);
    return status;
}

static Jxta_status discovery_service_send_delta_update_msg(Jxta_discovery_service_ref * discovery, Jxta_PID *source_id, const char * key, Jxta_hashtable * entries_update)
{
    Jxta_status status = JXTA_SUCCESS;
    char ** peer_ids = NULL;
    char ** peer_ids_tmp = NULL;
    JString *source_peerid_j = NULL;

    peer_ids = jxta_hashtable_keys_get(entries_update);
    peer_ids_tmp = peer_ids;
    while (*peer_ids_tmp) {
        Jxta_SRDIMessage *msg = NULL;
        Jxta_vector * entries = NULL;
        Jxta_id * peerid = NULL;
        JString *messageString = NULL;
        ResolverSrdi *srdi = NULL;
        JString *peerid_j = NULL;

        jxta_hashtable_get(entries_update, *peer_ids_tmp, strlen(*peer_ids_tmp) + 1, JXTA_OBJECT_PPTR(&entries));
        if (NULL == entries) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No entries were available for peerid - %s\n", *peer_ids_tmp);
            free(*peer_ids_tmp++);
            continue;
        }

        msg = jxta_srdi_message_new_2(1, discovery->localPeerId, jxta_id_equals(discovery->localPeerId, source_id) ? NULL:source_id, (char *) key, entries);
        if (msg == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot allocate jxta_srdi_message_new\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        if (strcmp(*peer_ids_tmp, "NoPeer")) {
            jxta_id_from_cstr(&peerid, *peer_ids_tmp);
        }
        status = jxta_srdi_message_get_xml(msg, &messageString);
        JXTA_OBJECT_RELEASE(msg);
        msg = NULL;

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot serialize srdi message\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        srdi = jxta_resolver_srdi_new_1(discovery->instanceName, messageString, NULL);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "%s\n", jstring_get_string(messageString));

        JXTA_OBJECT_RELEASE(messageString);
        messageString = NULL;

        if (srdi == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot allocate a resolver srdi message\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        status = jxta_resolver_service_sendSrdi(discovery->resolver, srdi, peerid);
        JXTA_OBJECT_RELEASE(srdi);
        srdi = NULL;

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot send srdi message to %s\n", *peer_ids_tmp);
            free(*(peer_ids_tmp++));
            continue;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got %d delta sequence updates for peer %s\n", jxta_vector_size(entries), *peer_ids_tmp);

        peerid_j = jstring_new_2(*peer_ids_tmp);
        jxta_id_to_jstring(source_id, &source_peerid_j);

        cm_update_delta_entries(discovery->cm, peerid_j, source_peerid_j, discovery->instanceName, entries, 0);

        JXTA_OBJECT_RELEASE(peerid_j);
        JXTA_OBJECT_RELEASE(entries);
        JXTA_OBJECT_RELEASE(source_peerid_j);
        if (peerid)
            JXTA_OBJECT_RELEASE(peerid);
        free(*(peer_ids_tmp++));
    }

FINAL_EXIT:

    if (peer_ids)
        free(peer_ids);
    return status;
}


/* vi: set ts=4 sw=4 tw=130 et: */
