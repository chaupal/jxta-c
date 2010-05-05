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
#include "jxta_rq_callback.h"
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
#include "jxta_peerview.h"
#include "jxta_sql.h"
#include "jxta_srdi.h"
#include "jxta_stdpg_private.h"
#include "jxta_rsrdi.h"
#include "jxta_srdi_service.h"
#include "jxta_test_adv.h"
#include "jxta_apr.h"
#include "jxta_range.h"
#include "jxta_util_priv.h"
#include "jxta_xml_util.h"
#include "jxta_discovery_config_adv.h"

#define HAVE_POOL
#define QUERY_ID_EXPIRATION 30 /* 30 seconds */

const char JXTA_DISCOVERY_SRDI_NAME[] = "Srdi";

enum e_listener_entries {
    LISTENER_RES_RSP_HANDLER = 0,
    LISTENER_RDV_EVENT_HANDLER,
    LISTENER_SRDI_HANDLER,
    LISTENER_MAX
};

typedef struct {
    Extends(Jxta_discovery_service);

    volatile Jxta_boolean running;

    Jxta_DiscoveryConfigAdvertisement *config;
    JString *instanceName;
    Jxta_PG *group;
    JString *gid_str;
    int instance_id;
    int query_expiration;
    Jxta_resolver_service *resolver;
    Jxta_endpoint_service *endpoint;
    void *ep_cookie;
    Jxta_rdv_service *rdv;
    Jxta_srdi_service *srdi;
    /* vector to hold generic discovery listeners */
    Jxta_vector *listener_vec;
    /* hashtable to hold specific query listeners */
    Jxta_hashtable *listeners;
    Jxta_PID *localPeerId;
    Jxta_id *prev_id;
    char *pid_str;
    Jxta_advertisement *impl_adv;
    Jxta_cm *cm;
    Jxta_boolean cm_on;
    Jxta_listener *my_listeners[LISTENER_MAX];
    volatile Jxta_time_diff expired_loop;
    volatile Jxta_time_diff delta_loop;
} Jxta_discovery_service_ref;

typedef struct _DR_thread_struct {
    Jxta_discovery_service_ref  *discovery;
    ResolverQuery *rq;
    Jxta_DiscoveryResponse *dr;
} DR_thread_struct;

        void *APR_THREAD_FUNC discovery_send_response_thread(apr_thread_t *apr_thread, void *arg);

static Jxta_status discovery_service_send_to_replica(Jxta_discovery_service_ref * discovery, ResolverQuery * rq,
                                                     JString * replicaExpression, Jxta_query_context * jContext);
static Jxta_status JXTA_STDCALL discovery_service_query_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_srdi_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_response_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL discovery_service_rdv_listener(Jxta_object * obj, void *arg);
static void *APR_THREAD_FUNC delta_cycle_func(apr_thread_t *thread, void *arg);
static void *APR_THREAD_FUNC cache_handling_func(apr_thread_t * thread, void *arg);
static Jxta_status discovery_service_send_delta_updates(Jxta_discovery_service_ref * discovery, const char * key, Jxta_hashtable * source_entries_update, Jxta_hashtable **ret_msgs);

static Jxta_status discovery_service_send_delta_update_msg(Jxta_discovery_service_ref * discovery, Jxta_PID *source_id, const char * key, Jxta_hashtable * entries_update, Jxta_hashtable **ret_msgs);
static Jxta_status discovery_filter_replicas_forward(Jxta_discovery_service_ref * discovery, Jxta_vector * entries, const char * source_peer, JString *jRepPeerid, Jxta_hashtable ** fwd_entries);
static Jxta_status discovery_get_responses(Jxta_discovery_service_ref *discovery, int threshold, short type, char **keys_parm, Jxta_vector **responses);

static Jxta_status discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync);
static Jxta_status discovery_push_srdi_1(Jxta_discovery_service_ref * discovery, Jxta_hashtable **elements);
static Jxta_status discovery_push_srdi_2(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync);

static Jxta_status discovery_delta_srdi_priv(Jxta_discovery_service_ref * discovery
                                , Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync, Jxta_hashtable **ret_msgs);

static Jxta_status query_replica(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, JString *jDrop, Jxta_vector ** peerids);
static Jxta_status query_srdi(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, JString * jDrop, Jxta_hashtable ** peerids);
static Jxta_status publish(Jxta_discovery_service * service, Jxta_advertisement * adv, short type,
                    Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers);
static JString* build_extended_query(Jxta_discovery_query * dq);
static Jxta_status discovery_notify_peerview_peers(Jxta_discovery_service_ref *discovery, Jxta_id *src_pid, Jxta_id *prev_pid, Jxta_vector *lost_pids);
static Jxta_status discovery_send_discovery_response(Jxta_discovery_service_ref * discovery
                            , ResolverQuery * rq, Jxta_DiscoveryResponse *dr);

static Jxta_status split_discovery_responses(Jxta_DiscoveryResponse *dr, JString * response_j, Jxta_vector **new_responses, apr_int64_t max_length);


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
    int instance;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) module;

    Jxta_PG *parentgroup=NULL;
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

    /* keep a reference to our group and impl adv */
    discovery->group = group;   /* not shared because that would create a circular dependence */
    discovery->impl_adv = impl_adv ? JXTA_OBJECT_SHARE(impl_adv) : NULL;
    jxta_PG_get_parentgroup(discovery->group, &parentgroup);
    jxta_PG_get_configadv(group, &conf_adv);
    if (!conf_adv && NULL != parentgroup) {
        jxta_PG_get_configadv(parentgroup, &conf_adv);
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
    instance = jxta_discovery_config_get_instance(discovery->config);
    if (NULL == parentgroup) {
        instance = ++instance < CHAR_MAX ? instance:1;
        jxta_discovery_config_set_instance(discovery->config, instance);
    }
    discovery->instance_id = instance;
    discovery->query_expiration = jxta_discovery_config_get_query_expiration(discovery->config);
    if (-1 == discovery->query_expiration) {
        discovery->query_expiration = QUERY_ID_EXPIRATION;
        jxta_discovery_config_set_query_expiration(discovery->config, discovery->query_expiration);
    }

    /**
     ** Build the local name of the instance
     **/
    discovery->instanceName = NULL;
    jxta_id_to_jstring(assigned_id, &discovery->instanceName);
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
        discovery->my_listeners[LISTENER_RES_RSP_HANDLER] = listener;
    }

    /*
     * Register to RDV service fro RDV events
     */
    listener = jxta_listener_new((Jxta_listener_func) discovery_service_rdv_listener, discovery, 1, 1);

    jxta_listener_start(listener);

    status = jxta_rdv_service_add_event_listener(discovery->rdv,
                                                 (char *) jstring_get_string(discovery->instanceName),
                                                 (char *) jstring_get_string(discovery->gid_str), listener);
    discovery->my_listeners[LISTENER_RDV_EVENT_HANDLER] = listener;

    /* Create vector and hashtable for the listeners */
    discovery->listener_vec = jxta_vector_new(1);
    discovery->listeners = jxta_hashtable_new(1);


    /* Mark the service as running now. */
    discovery->running = TRUE;
    if (parentgroup)
        JXTA_OBJECT_RELEASE(parentgroup);
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
    Jxta_PG *parent=NULL;
    Jxta_status status;
    Jxta_listener *listener;

    listener = jxta_listener_new((Jxta_listener_func) discovery_service_srdi_listener, discovery, 1, 200);
    jxta_listener_start(listener);

    /* status = jxta_resolver_service_registerSrdiHandler(discovery->resolver, discovery->instanceName, listener); */
    me->my_listeners[LISTENER_SRDI_HANDLER] = listener;

    status = jxta_srdi_registerSrdiListener(me->srdi, me->instanceName, listener);

    jxta_PG_get_PA(me->group, &padv);

    publish((Jxta_discovery_service *) me, (Jxta_advertisement *) padv, DISC_PEER, PA_EXPIRATION_LIFE,
            PA_REMOTE_EXPIRATION);
    JXTA_OBJECT_RELEASE(padv);

    /* start the delta func for the top level group */
    jxta_PG_get_parentgroup(me->group, &parent);
    if (NULL == parent) {
        delta_cycle_func(NULL, me);
        /* register endpoint listener for EndpointMessages */
        /* jxta_PG_add_recipient(me->group, &me->ep_cookie, jstring_get_string(me->instanceName), JXTA_DISCOVERY_SRDI_NAME, discovery_srdi_cb, me); */
    } else {
        JXTA_OBJECT_RELEASE(parent);
    }
    /* start a Cache clean up function */
    cache_handling_func(NULL, me);

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

    for (i = 0; i < LISTENER_MAX; i++) {
        jxta_listener_stop(discovery->my_listeners[i]);
    }
    status = jxta_resolver_service_unregisterQueryHandler(discovery->resolver, discovery->instanceName);
    status = jxta_resolver_service_unregisterResHandler(discovery->resolver, discovery->instanceName);
    
    status = jxta_srdi_unregisterSrdiListener(discovery->srdi, discovery->instanceName);

    jxta_rdv_service_remove_event_listener(discovery->rdv,
                                           (char *) jstring_get_string(discovery->instanceName),
                                           (char *) jstring_get_string(discovery->gid_str));
    for (i = 0; i < LISTENER_MAX; i++) {
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

static Jxta_status discovery_service_forward_from_local(Jxta_discovery_service_ref * discovery, ResolverQuery * res_query, Jxta_discovery_query * query)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_query_context *jContext = NULL;
    JString *replicaExpression = NULL;
    JString *ext_query = NULL;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";

    jxta_discovery_query_get_extended_query(query, &ext_query);
    if (!ext_query) {
        ext_query = build_extended_query(query);
    }

    if (NULL != ext_query) {
        /* create a query context */
        jContext = jxta_query_new(jstring_get_string(ext_query));
        if (jContext == NULL) {
            status = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }
        /* create a compound SQL statement to search the SRDI */
        jxta_query_set_compound_table(jContext, CM_TBL_SRDI_SRC);
        jxta_query_compound_XPath(jContext, xmlString, FALSE);
    }

    replicaExpression = jstring_new_0();
    if (JXTA_SUCCESS == status) {
        Jxta_vector *srdi_results=NULL;
        Jxta_hashtable *peer_results=NULL;

        if (NULL != query) {
            status = query_srdi(discovery, jContext, NULL, &peer_results);
            if (JXTA_ITEM_NOTFOUND == status) {
                status = query_replica(discovery, jContext, NULL, &srdi_results);
            } else {
                char **peerids=NULL;
                char **peerids_save=NULL;

                srdi_results = jxta_vector_new(0);
                peerids = jxta_hashtable_keys_get(peer_results);
                peerids_save = peerids;
                while (NULL != peerids && *peerids) {
                    Jxta_id *peer_id_tmp=NULL;

                    jxta_id_from_cstr(&peer_id_tmp, *peerids);
                    jxta_vector_add_object_last(srdi_results, (Jxta_object *) peer_id_tmp);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding peer %s \n", *peerids);
                    free(*(peerids++));
                    JXTA_OBJECT_RELEASE(peer_id_tmp);
                }
                if (peerids_save)
                    free(peerids_save);

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
            srdi_results = NULL;
        } else {
            status = discovery_service_send_to_replica(discovery, res_query, replicaExpression, jContext);
        }
        if (srdi_results)
            JXTA_OBJECT_RELEASE(srdi_results);
        if (peer_results)
            JXTA_OBJECT_RELEASE(peer_results);
    }
  FINAL_EXIT:
    if (jContext)
        JXTA_OBJECT_RELEASE(jContext);
    if (replicaExpression)
        JXTA_OBJECT_RELEASE(replicaExpression);
    if (ext_query)
        JXTA_OBJECT_RELEASE(ext_query);
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
static long remoteQuery(Jxta_discovery_service * service, Jxta_id * peerid, Jxta_discovery_query *query,
                    Jxta_discovery_listener * listener)
{
    Jxta_status status;
    long qid = 0;
    JString *qdoc = NULL;
    ResolverQuery *rq = NULL;
    char buf[128];              /* More than enough */
    const Jxta_qos * qos;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    jxta_discovery_query_set_instance_id(query, discovery->instance_id);
    memset(buf, 0, sizeof(buf));
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "discovery::remoteQuery send query id: %ld\n", jxta_resolver_query_get_queryid(rq));
        jxta_discovery_query_ext_set_state(query, DEQ_FWD_SRDI);
        jxta_resolver_query_set_query_adv(rq, (Jxta_advertisement *) query);
        status = jxta_resolver_service_sendQuery(discovery->resolver, rq, peerid);
    } else {
        status = discovery_service_forward_from_local(discovery, rq, query);
        if (JXTA_ITEM_NOTFOUND == status) {
            jxta_resolver_query_set_hopcount(rq, 1);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "discovery::remoteQuery send query id: %ld\n", jxta_resolver_query_get_queryid(rq));
            jxta_discovery_query_ext_set_state(query, DEQ_FWD_WALK);
            jxta_resolver_query_set_query_adv(rq, (Jxta_advertisement *) query);
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

static Jxta_status cancelRemoteQuery(Jxta_discovery_service * me, long query_id, Jxta_discovery_listener ** listener)
{
    Jxta_discovery_service_ref *me2 = (Jxta_discovery_service_ref *) me;
    char buf[128];

    apr_snprintf(buf, sizeof(buf), "%ld", query_id);
    return jxta_hashtable_del(me2->listeners, (const void *) buf, strlen(buf), (Jxta_object **) listener);
}

static Jxta_status getLocalAdv(Jxta_discovery_service * service, short type, const char *aattribute, const char *value,
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s elem - %s \n", jstring_get_string(elem->jBoolean),
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
static Jxta_status query_replica(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, JString *jDrop, Jxta_vector ** peerids)
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

static Jxta_status query_srdi(Jxta_discovery_service_ref * self, Jxta_query_context * jContext, JString * jDrop, Jxta_hashtable ** peerids)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) self;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_cache_entry **cache_entries;
    Jxta_cache_entry **scan;
    JString * jDrop_local = NULL;
    JString * sql_cmd = NULL;
    sql_cmd = jstring_new_0();
    jstring_append_1(sql_cmd, jContext->sqlcmd);
    *peerids = NULL;

    cache_entries = cm_sql_query_srdi(discovery->cm, jstring_get_string(jContext->queryNameSpace), sql_cmd, FALSE);
    if (NULL == cache_entries || NULL == *cache_entries) {
        JXTA_OBJECT_RELEASE(sql_cmd);
        return JXTA_ITEM_NOTFOUND;
    }
    if (NULL == jDrop) {
        jDrop_local = jstring_new_0();
    } else {
        jDrop_local = JXTA_OBJECT_SHARE(jDrop);
    }

    scan = cache_entries;
    while (scan != NULL && *scan) {
        Jxta_cache_entry *cache_entry=NULL;
        Jxta_hashtable *peer_entries=NULL;
        JString *dummy=NULL;

        cache_entry = *scan;
        if (!strcmp(jstring_get_string(jDrop_local), jstring_get_string(cache_entry->profid))
            || !strcmp(jstring_get_string(jDrop_local), jstring_get_string(cache_entry->sourceid))) {
                status = JXTA_ITEM_EXISTS;
        } else {
            if (NULL == *peerids) {
                *peerids = jxta_hashtable_new(0);
            }
            if (JXTA_SUCCESS != jxta_hashtable_get(*peerids, jstring_get_string(cache_entry->profid), jstring_length(cache_entry->profid) + 1
                        ,JXTA_OBJECT_PPTR(&peer_entries))) {
                peer_entries = jxta_hashtable_new(0);
                jxta_hashtable_put(*peerids,  jstring_get_string(cache_entry->profid), jstring_length(cache_entry->profid) + 1, (Jxta_object *) peer_entries);
            }
            if (JXTA_SUCCESS != jxta_hashtable_get(peer_entries, jstring_get_string(cache_entry->advid), jstring_length(cache_entry->advid) + 1
                        ,JXTA_OBJECT_PPTR(&dummy))) {
                jxta_hashtable_put(peer_entries, jstring_get_string(cache_entry->advid), jstring_length(cache_entry->advid) + 1, (Jxta_object *) cache_entry->advid);
            }
        }
        JXTA_OBJECT_RELEASE(cache_entry);
        if (dummy)
            JXTA_OBJECT_RELEASE(dummy);
        if (peer_entries)
            JXTA_OBJECT_RELEASE(peer_entries);
        scan++;
    }

    free(cache_entries);
    JXTA_OBJECT_RELEASE(jDrop_local);
    JXTA_OBJECT_RELEASE(sql_cmd);
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

static Jxta_status publish(Jxta_discovery_service * service, Jxta_advertisement * adv, short type,
                    Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers)
{
    return publish_priv(service, adv, type, lifetime, lifetimeForOthers, TRUE);
}

static Jxta_status remotePublish(Jxta_discovery_service * service, Jxta_id * peerid, Jxta_advertisement * adv,
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
        status = jxta_resolver_service_sendResponse(discovery->resolver, res_response, peerid, NULL);
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

static Jxta_status flush(Jxta_discovery_service * service, char *id, short type)
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

static Jxta_status getLifetime(Jxta_discovery_service * service, short type, Jxta_id * advId, Jxta_expiration_time * eexp)
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

static Jxta_status getExpiration(Jxta_discovery_service * service, short type, Jxta_id * advId, Jxta_expiration_time * eexp)
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

static void flush_deltas(Jxta_discovery_service * service)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    discovery_push_srdi(discovery, TRUE, TRUE, TRUE);
}

static Jxta_status add_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    return (jxta_vector_add_object_last(discovery->listener_vec, (Jxta_object *) listener));
}

static Jxta_status remove_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
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
    getExpiration,
    flush_deltas
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

    if (NULL != self->prev_id) {
        JXTA_OBJECT_RELEASE(self->prev_id);
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
                    replicaPeer = jxta_srdi_getReplicaPeer(discovery->srdi, expression, NULL);
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
                        peerHash = jxta_hashtable_new(0);
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
            Jxta_vector *walk_peers=NULL;
            Jxta_peerview *pv = NULL;


            pv = jxta_rdv_service_get_peerview(discovery->rdv);

            peerReplicas = jxta_hashtable_values_get(peerHash);
            JXTA_OBJECT_RELEASE(peerHash);

            jxta_peerview_select_walk_peers(pv, peerReplicas, JXTA_PV_WALK_POLICY_CONFIGURED, &walk_peers);

            for (i = 0; i < jxta_vector_size(peerReplicas); i++) {
                Jxta_id *id = NULL;
                JString *sTemp = NULL;

                status = jxta_vector_get_object_at(peerReplicas, JXTA_OBJECT_PPTR(&id), i);
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to remove id from peerReplicas er:%d\n", status);
                    continue;
                }
                status = jxta_id_to_jstring(id, &sTemp);

                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Invalid Id - Could not create a string er:%d\n", status);
                } else {
                    ResolverQuery * rq_clone = NULL;
                    Jxta_advertisement *query_adv = NULL;

                    jxta_resolver_query_get_query_adv(rq, &query_adv);

                    if (NULL != query_adv) {
                        Jxta_discovery_ext_query_state send_state;

                        send_state = TRUE == jxta_vector_contains(walk_peers, (Jxta_object *) id, (Jxta_object_equals_func) jxta_id_equals) 
                                    ? DEQ_FWD_REPLICA_FWD:DEQ_FWD_REPLICA_STOP;
                        jxta_discovery_query_ext_set_state((Jxta_discovery_query *) query_adv, send_state);
                        if (DEQ_FWD_REPLICA_FWD == send_state) {
                            jxta_peerview_flag_walk_peer_usage(pv, id);
                        }
                    }
                    rq_clone = jxta_resolver_query_clone(rq, query_adv);

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward query id: %ld to replica %s \n", jxta_resolver_query_get_queryid(rq), jstring_get_string(sTemp));
                    if (NULL != rq_clone) {
                        jxta_srdi_forwardQuery_peer(discovery->srdi, discovery->resolver, id, rq_clone);

                        JXTA_OBJECT_RELEASE(rq_clone);
                    } else {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create a resolver query clone - query id: %ld is not sent\n", jxta_resolver_query_get_queryid(rq));
                    }
                    if (query_adv)
                        JXTA_OBJECT_RELEASE(query_adv);
                }
                if (id)
                    JXTA_OBJECT_RELEASE(id);
                if (sTemp)
                    JXTA_OBJECT_RELEASE(sTemp);
            }
            JXTA_OBJECT_RELEASE(peerReplicas);
            if (walk_peers)
                JXTA_OBJECT_RELEASE(walk_peers);
            if (pv)
                JXTA_OBJECT_RELEASE(pv);
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

static Jxta_status discovery_re_replicate_srdi_entries(Jxta_discovery_service_ref * discovery, char **peerids_parm, Jxta_hashtable *peer_results)
{
    Jxta_status res=JXTA_SUCCESS;
    char **peerids;
    char **idx_keys=NULL;
    peerids = peerids_parm;

    /* get the entries for each peer */
    while (NULL != peerids && *peerids) {
        int i,x = 0;
        Jxta_hashtable *peer_advids_h=NULL;
        Jxta_vector *peer_advids=NULL;
        Jxta_hashtable *srdi_entries=NULL;
        JString *peer_j=NULL;

        if (JXTA_SUCCESS != jxta_hashtable_get(peer_results, *peerids, strlen(*peerids) + 1 , JXTA_OBJECT_PPTR(&peer_advids_h))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Didn't get %s from the peer results\n", *peerids);
            peerids++;
            continue;
        }
        peer_advids = jxta_hashtable_values_get(peer_advids_h);
        JXTA_OBJECT_RELEASE(peer_advids_h);
        peer_j = jstring_new_2(*peerids);
        /* get the advids for this peer */
        for (i=0; i < jxta_vector_size(peer_advids); i++) {
            Jxta_hashtable *idx_entries_h=NULL;
            JString *peer_advid_j=NULL;
            Jxta_SRDIEntryElement *adv_element=NULL;

            jxta_vector_get_object_at(peer_advids, JXTA_OBJECT_PPTR(&peer_advid_j), i);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Get entry a:%s p:%s\n"
                    , jstring_get_string(peer_advid_j)
                    , jstring_get_string(peer_j));

            /* get the index entries for this advid */
            res = cm_get_srdi_index_entries_1(discovery->cm, peer_j, peer_advid_j, &idx_entries_h);
            /* get the srdi message types */
            idx_keys = jxta_hashtable_keys_get(idx_entries_h);
            x = 0;
            /* for each srdi message type create SRDI message entries */
            while (NULL != idx_keys && idx_keys[x]) {
                int j=0;
                Jxta_vector *entries=NULL;

                if (JXTA_SUCCESS != jxta_hashtable_get(idx_entries_h, idx_keys[x], strlen(idx_keys[x]) + 1, JXTA_OBJECT_PPTR(&entries))) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Didn't get %s from the srdi idx entries\n", idx_keys[x]);
                    free(idx_keys[x++]);
                    continue;
                }
                /* create an SRDI entry with the sequence number update */
                for (j = 0; j < jxta_vector_size(entries); j++) {
                    Jxta_srdi_idx_entry *entry=NULL;
                    char tmp[64];

                    jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), j);

                    if (NULL == adv_element) {
                        adv_element = jxta_srdi_new_element();
                        adv_element->advId = JXTA_OBJECT_SHARE(peer_advid_j);
                        adv_element->sn_cs_values = jstring_new_0();
                        adv_element->expiration = entry->expiration;
                        adv_element->replicate = entry->replicate;
                        adv_element->re_replicate = TRUE;
                    }
                    if (jstring_length(adv_element->sn_cs_values) > 0) {
                        jstring_append_2(adv_element->sn_cs_values,",");
                    }
                    memset(tmp, 0, sizeof(tmp));
                    apr_snprintf(tmp, sizeof(tmp), JXTA_SEQUENCE_NUMBER_FMT , entry->seq_number);
                    jstring_append_2(adv_element->sn_cs_values, tmp);

                    JXTA_OBJECT_RELEASE(entry);
                }
                if (adv_element) {
                    Jxta_vector *idx_entries=NULL;

                    if (NULL == srdi_entries) {
                        srdi_entries = jxta_hashtable_new(0);
                    }

                    if (JXTA_SUCCESS != jxta_hashtable_get(srdi_entries, idx_keys[x], strlen(idx_keys[x]) + 1, JXTA_OBJECT_PPTR(&idx_entries))) {
                        idx_entries = jxta_vector_new(0);
                        jxta_hashtable_put(srdi_entries, idx_keys[x], strlen(idx_keys[x]) + 1, (Jxta_object *) idx_entries);
                    }
                    jxta_vector_add_object_last(idx_entries, (Jxta_object *) adv_element);
                    JXTA_OBJECT_RELEASE(idx_entries);
                    JXTA_OBJECT_RELEASE(adv_element);
                    adv_element = NULL;
                }
                JXTA_OBJECT_RELEASE(entries);
                free(idx_keys[x++]);
            }
            if (adv_element)
                JXTA_OBJECT_RELEASE(adv_element);
            JXTA_OBJECT_RELEASE(peer_advid_j);
            if (idx_entries_h)
                JXTA_OBJECT_RELEASE(idx_entries_h);
            free(idx_keys);
        }
        if (srdi_entries) {
            x = 0;
            char **types=NULL;

            types = jxta_hashtable_keys_get(srdi_entries);

            /* replicate the entries for this type */
            while (NULL != types && types[x]) {
                Jxta_SRDIMessage *srdi_msg=NULL;
                Jxta_id *peer_id=NULL;
                JString *messageString=NULL;
                Jxta_vector *entries_v = NULL;
                Jxta_vector *entries_new_v = NULL;

                if (JXTA_SUCCESS != jxta_hashtable_get(srdi_entries, types[x], strlen(types[x]) + 1, JXTA_OBJECT_PPTR(&entries_v))) {
                    free(types[x++]);
                    continue;
                }
                jxta_id_from_cstr(&peer_id, *peerids);
                res = cm_expand_delta_entries(discovery->cm, entries_v, peer_j, peer_j, &entries_new_v, TRUE);

                srdi_msg = jxta_srdi_message_new_1(1, peer_id, types[x], entries_new_v);
                jxta_srdi_message_get_xml(srdi_msg, &messageString);

                jxta_srdi_replicateEntries(discovery->srdi, discovery->resolver, srdi_msg, discovery->instanceName, NULL);

                JXTA_OBJECT_RELEASE(messageString);
                JXTA_OBJECT_RELEASE(srdi_msg);
                JXTA_OBJECT_RELEASE(peer_id);
                JXTA_OBJECT_RELEASE(entries_v);
                JXTA_OBJECT_RELEASE(entries_new_v);
                free(types[x++]);
            }
            if (types)
                free(types);
            JXTA_OBJECT_RELEASE(srdi_entries);
        }
        JXTA_OBJECT_RELEASE(peer_advids);
        JXTA_OBJECT_RELEASE(peer_j);

        peerids++;
    }

    return res;
}

static Jxta_status JXTA_STDCALL discovery_service_query_listener(Jxta_object * obj, void *arg)
{
    Jxta_status status = JXTA_FAILED;
    Jxta_discovery_service_ref *discovery =  PTValid(arg, Jxta_discovery_service_ref);
    Jxta_status res = JXTA_ITEM_NOTFOUND;
    JString *peeradv = NULL;
    JString *query = NULL;
    Jxta_DiscoveryQuery *dq = NULL;
    JString *attr = NULL;
    JString *val = NULL;
    JString *extendedQuery = NULL;
    JString *replicaExpression = NULL;
    char **keys = NULL;
    Jxta_vector *peers = NULL;
    JString *id_str_j = NULL;
    Jxta_id *id = NULL;
    Jxta_query_context *jContext = NULL;
    Jxta_vector *responses = NULL;
    Jxta_id *rqId = NULL;
    char instance_id=-1;
    long qid = -1;
    unsigned int i=0;
    int threshold = 0;
    ResolverQueryCallback *rqc = (ResolverQueryCallback *) obj;
    ResolverQuery *rq = NULL;
    Jxta_id *src_pid = NULL;
    Jxta_vector *qAdvs = NULL;
    short type = 0;
    const char *xmlString = "<jxta:query xmlns:jxta=\"http://jxta.org\"/>";
    JString *print_ext=NULL;
    Jxta_discovery_ext_query_state rcvd_state;

    rq = jxta_resolver_query_callback_get_rq(rqc);
    qid = jxta_resolver_query_get_queryid(rq);
    jxta_resolver_query_get_query(rq, &query);
    rqId = jxta_resolver_query_get_src_peer_id(rq);
    jxta_id_to_jstring(rqId, &id_str_j);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Discovery rcvd resolver query id: %ld from: %s\n", qid, jstring_get_string(id_str_j));

    if (query == NULL) {
        /*we're done, nothing to proccess */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    dq = jxta_discovery_query_new();
    status = jxta_discovery_query_parse_charbuffer(dq, jstring_get_string(query), jstring_length(query));
    if (JXTA_SUCCESS != status) {
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    threshold = jxta_discovery_query_get_threshold(dq);

    print_ext = jstring_new_0();
    jxta_discovery_query_ext_print_state(dq, print_ext);
    rcvd_state = jxta_discovery_query_ext_get_state(dq);
    instance_id = jxta_discovery_query_get_instance_id(dq);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "state: %s instance:%d qid: %ld\n", jstring_get_string(print_ext), instance_id, qid);
    JXTA_OBJECT_RELEASE(print_ext);


    status = cm_save_query_request(discovery->cm, jstring_get_string(id_str_j), instance_id, qid, discovery->query_expiration * 1000);
    if (rcvd_state != DEQ_FWD_REPLICA_FWD) {
        if (JXTA_ITEM_EXISTS == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Dropping query id: %ld from: %s\n", qid, jstring_get_string(id_str_j));
            res = JXTA_SUCCESS;
            goto FINAL_EXIT;
        } else if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error saving query request info query id: %ld from: %s ret:%d\n"
                            , qid, jstring_get_string(id_str_j), status);
        }
    }

/*     JXTA_OBJECT_RELEASE(query); */

    jxta_discovery_query_get_peeradv(dq, &peeradv);

    if (peeradv && jstring_length(peeradv) > 0) {
        Jxta_PA *padv = NULL;

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
            if (TRUE == match) {
                JXTA_OBJECT_RELEASE(padv);
                goto FINAL_EXIT;
            }
        }
        publish((Jxta_discovery_service *) discovery,
                (Jxta_advertisement *) padv, DISC_PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
        JXTA_OBJECT_RELEASE(padv);
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
            goto FINAL_EXIT;
        }

    } else if (!extendedQuery || jstring_length(extendedQuery) == 0) {
        keys = cm_get_primary_keys(discovery->cm, (char *) dirname[type], TRUE);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Search everything \n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Search query %s\n", jstring_get_string(extendedQuery));
        /* create a query context */
        jContext = jxta_query_new(jstring_get_string(extendedQuery));

        if (jContext == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create context from %s\n", jstring_get_string(extendedQuery));
            goto FINAL_EXIT;
        }

        /* compile the XPath statement */
        status = jxta_query_compound_XPath(jContext, xmlString, FALSE);

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to compile %s\n", jstring_get_string(extendedQuery));
            goto FINAL_EXIT;
        }

        status = queryLocal(discovery, NULL, jContext, &qAdvs, threshold, FALSE);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error retrieving local ads res:%d %s\n", status, jstring_get_string(extendedQuery));
            goto FINAL_EXIT;
        }

        keys = (char **) calloc(1, sizeof(char *) * (jxta_vector_size(qAdvs) + 1));

        unsigned int found_count = 0;
        for (i = 0; i < jxta_vector_size(qAdvs); i++) {
            Jxta_query_result * qres = NULL;
            Jxta_advertisement *foundAdv=NULL;
            JString *found_id_j=NULL;

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
                status = jxta_id_get_uniqueportion(id, &found_id_j);
                JXTA_OBJECT_RELEASE(id);
                if (status != JXTA_SUCCESS) {
                    /* something went wrong with getting the ID */
                    goto FINAL_EXIT;
                }
            } else {
                found_id_j = jstring_new_2(jxta_advertisement_get_local_id_string(foundAdv));
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found Id - %s.\n", jstring_get_string(found_id_j));
            keys[found_count++] = strdup(jstring_get_string(found_id_j));
            JXTA_OBJECT_RELEASE(found_id_j);
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
        Jxta_discovery_ext_query_state state = DEQ_SUPRESS;
        Jxta_boolean walk_it=TRUE;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No keys locally \n");
        if (jxta_rdv_service_is_rendezvous(discovery->rdv) != TRUE) {
            goto FINAL_EXIT;
        }
        if (NULL != jContext && DEQ_REV_PUBLISHER != rcvd_state) {
            size_t found_count = 0;
            Jxta_hashtable *peer_results=NULL;

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "query the SRDI\n");

            status = query_srdi(discovery, jContext, id_str_j, &peer_results);

            if (NULL != peer_results) {
                jxta_hashtable_stats(peer_results, NULL, &found_count, NULL, NULL, NULL);
            }
            if (JXTA_ITEM_EXISTS == status) {
                walk_it = FALSE;
            }
            /* if no entry is found */
            if (JXTA_ITEM_NOTFOUND == status || (JXTA_ITEM_EXISTS == status && 0 == found_count)) {
                if (DEQ_REV_REPLICATING == rcvd_state || DEQ_REV_REPLICATING_WALK == rcvd_state) {
                    /* TODO ExocetRick Remove the replica from the peer that routed this query. It is a stale
                        replica that will not be removed until it expires. In order to implement this the discovery
                        query needs to be modified to include the peer that is routing the query back to this peer
                    */
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received query expecting results in the SRDI and none found - Query dropped\n");
                    res = JXTA_SUCCESS;
                    if (peer_results)
                        JXTA_OBJECT_RELEASE(peer_results);
                    goto FINAL_EXIT;
                }
                status = query_replica(discovery, jContext, id_str_j, &peers);
                if (JXTA_SUCCESS == status) {
                    state = (DEQ_FWD_WALK == rcvd_state) ? DEQ_REV_REPLICATING_WALK:DEQ_REV_REPLICATING;
                } else if (DEQ_FWD_REPLICA_FWD == rcvd_state) {
                    state = DEQ_FWD_WALK;
                } else if (DEQ_FWD_REPLICA_STOP == rcvd_state) {
                    state = DEQ_STOP;
                    walk_it = FALSE;
                }
                found_count = NULL == peers ? 0: jxta_vector_size(peers);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found %d replicas\n", found_count);
            } else if (NULL != peer_results) {
                char **peerids=NULL;
                char **peerids_save=NULL;

                if (JXTA_ITEM_EXISTS == status) {
                    walk_it = FALSE;
                }
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found %d srdi entries\n", found_count);

                peers = jxta_vector_new(0);
                peerids = jxta_hashtable_keys_get(peer_results);
                peerids_save = peerids;
                while (NULL != peerids && *peerids) {
                    Jxta_id *peer_id_tmp=NULL;

                    jxta_id_from_cstr(&peer_id_tmp, *peerids);
                    jxta_vector_add_object_last(peers, (Jxta_object *) peer_id_tmp);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding entry %s \n", *peerids);
                    JXTA_OBJECT_RELEASE(peer_id_tmp);
                    peerids++;
                }

                /* decrease hopcount before forward, so that the hop count remains unchanged.
                    because we know the peer published this index should able to answer the query.
                    This is a hack to fix the hop count could be 4 for complex query as an additional stop at the replacating rdv
                */
                jxta_resolver_query_set_hopcount(rq, jxta_resolver_query_get_hopcount(rq) - 1);

                if (DEQ_REV_REPLICATING == rcvd_state || DEQ_REV_REPLICATING_WALK == rcvd_state) {
                    state = DEQ_REV_PUBLISHER;
                    if (DEQ_REV_REPLICATING_WALK == rcvd_state) {
                        JString *tmp_peerid_j=NULL;

                        jxta_discovery_query_ext_get_peerid(dq, &tmp_peerid_j);
                        if (tmp_peerid_j) {
                            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received a replicating walk discovery query from %s\n", jstring_get_string(tmp_peerid_j));
                            JXTA_OBJECT_RELEASE(tmp_peerid_j);
                        }
                        discovery_re_replicate_srdi_entries(discovery, peerids_save, peer_results);
                    }
                    res = JXTA_SUCCESS;
                } else if (DEQ_FWD_SRDI == rcvd_state) {
                    state = DEQ_REV_REPLICATING;
                }
                if (peerids_save) {
                    peerids = peerids_save;
                    while (*peerids) {
                        free(*(peerids++));
                    }
                    free(peerids_save);
                }
            } else if (JXTA_ITEM_EXISTS == status) {
                walk_it = FALSE;
                res = JXTA_SUCCESS;
            }
            if (peer_results)
                JXTA_OBJECT_RELEASE(peer_results);
        } else if (NULL != jContext) {
            /* TODO ExocetRick ************* remove the entries from the source that forwarded the query. In
                order to implement this the discovery query needs to be modified to include the peer that routed
                the query to this peer. It could be assumed that its the rendezvous but that may not be safe.
             */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "NO local advertisement when this peer is the publisher\n");
            goto FINAL_EXIT;

        } else {
            peers = jxta_srdi_searchSrdi(discovery->srdi, jstring_get_string(discovery->instanceName), NULL,
                                         jstring_get_string(attr), jstring_get_string(val));
        }

        jxta_discovery_query_ext_set_state(dq, state);

        if (DEQ_REV_REPLICATING_WALK == state) {
            JString *peerid_j;

            peerid_j = jstring_new_2(discovery->pid_str);
            jxta_discovery_query_ext_set_peerid(dq, peerid_j);
            JXTA_OBJECT_RELEASE(peerid_j);
        }

        jxta_resolver_query_set_query_adv(rq, (Jxta_advertisement *) dq);

        if (NULL != peers && jxta_vector_size(peers) > 0) {
            int j = 0;
            /* if we're using the replicating walk state */
            if (DEQ_REV_REPLICATING_WALK == state) {
                /* only send it to up-level peers */
                for (j=0; j < jxta_vector_size(peers); j++) {
                    Jxta_id *peer_id=NULL;
                    Jxta_PA * padv = NULL;
                    Jxta_version *peerVersion=NULL;
                    JString *tmp_j=NULL;
                    ResolverQuery * rq_clone = NULL;
                    Jxta_advertisement *query_adv = NULL;
                    status = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer_id), j);
                    if (JXTA_SUCCESS != status) {
                        continue;
                    }
                    jxta_id_to_jstring(peer_id, &tmp_j);
                    peergroup_find_peer_PA(discovery->group, peer_id, 30000, &padv);
                    if(padv == NULL){
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No peer adv found for %s\n", jstring_get_string(tmp_j));
                        peerVersion = jxta_version_new();
                    } else {
                        peerVersion = jxta_PA_get_version(padv);
                        JXTA_OBJECT_RELEASE(padv);
                    }

                    jxta_resolver_query_get_query_adv(rq, &query_adv);

                    if (!jxta_version_compatible_1(DISCOVERY_EXT_QUERY_WALK_STATE, peerVersion)) {
                        jxta_discovery_query_ext_set_state((Jxta_discovery_query *) query_adv, DEQ_REV_REPLICATING);
                    }
                    rq_clone = jxta_resolver_query_clone(rq, query_adv);

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Forward query id: %ld to replica %s \n"
                                            , jxta_resolver_query_get_queryid(rq_clone), jstring_get_string(tmp_j));
                    if (NULL != rq_clone) {
                        jxta_srdi_forwardQuery_peer(discovery->srdi, discovery->resolver, id, rq_clone);

                        JXTA_OBJECT_RELEASE(rq_clone);
                    } else {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create a resolver query clone - query id: %ld is not sent\n", jxta_resolver_query_get_queryid(rq));
                    }
                    if (query_adv)
                        JXTA_OBJECT_RELEASE(query_adv);
                    JXTA_OBJECT_RELEASE(tmp_j);
                    JXTA_OBJECT_RELEASE(peer_id);
                    JXTA_OBJECT_RELEASE(peerVersion);
                }
            } else {
                res = jxta_srdi_forwardQuery_peers(discovery->srdi, discovery->resolver, peers, rq);
            }
        } else if (JXTA_ITEM_EXISTS != status && walk_it) {
           jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "forward the query from replicaExpression %s\n"
                                , jstring_length(replicaExpression) > 0 ? jstring_get_string(replicaExpression):"(none)");

            jxta_resolver_query_callback_set_walk(rqc, TRUE);
            jxta_resolver_query_callback_set_propagate(rqc, FALSE);

            if (DEQ_FWD_SRDI == rcvd_state || DEQ_SUPRESS == rcvd_state) {
                res = discovery_service_send_to_replica(discovery, rq, replicaExpression, jContext);
                if (JXTA_ITEM_NOTFOUND == res) {
                    jxta_discovery_query_ext_set_state(dq, DEQ_FWD_WALK);
                    jxta_resolver_query_set_query_adv(rq, (Jxta_advertisement *) dq);
                }
            } else if (DEQ_FWD_WALK == rcvd_state) {
                res = JXTA_ITEM_NOTFOUND;
            }
        } else if (!walk_it) {
            jxta_resolver_query_callback_set_walk(rqc, FALSE);
            res = JXTA_SUCCESS;
        }
    } else {

        Jxta_DiscoveryResponse *dr = NULL;
        int num_responses=0;

        res = JXTA_SUCCESS;
        status = discovery_get_responses(discovery, threshold, type, keys, &responses);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to retrieve response err:%d\n", status);
            goto FINAL_EXIT;
        }

        if (NULL == responses || 0 == jxta_vector_size(responses)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No responses being sent\n");
            goto FINAL_EXIT;
        }

        num_responses = jxta_vector_size(responses);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Number of responses being sent: %i\n", num_responses);
        dr = jxta_discovery_response_new_1(jxta_discovery_query_get_type(dq)
                            , attr ? jstring_get_string(attr) : NULL
                            , val ? jstring_get_string(val) : NULL, num_responses, NULL, responses);

        jxta_discovery_response_set_timestamp(dr, jpr_time_now());


        DR_thread_struct *dr_thread=(DR_thread_struct *) arg;
        apr_thread_pool_t *tp;

        tp = jxta_PG_thread_pool_get(discovery->group);
        dr_thread = calloc(1, sizeof(DR_thread_struct));
        dr_thread->discovery = JXTA_OBJECT_SHARE(discovery);
        dr_thread->dr = JXTA_OBJECT_SHARE(dr);
        dr_thread->rq = JXTA_OBJECT_SHARE(rq);

        apr_thread_pool_push(tp, discovery_send_response_thread, dr_thread, APR_THREAD_TASK_PRIORITY_HIGHEST, discovery);
        /* res = discovery_send_discovery_response(discovery, rq, dr); */

        if (dr)
            JXTA_OBJECT_RELEASE(dr);
    }

  FINAL_EXIT:

    if (id_str_j)
        JXTA_OBJECT_RELEASE(id_str_j);
    if (rq)
        JXTA_OBJECT_RELEASE(rq);
    if (query)
        JXTA_OBJECT_RELEASE(query);
    if (src_pid)
        JXTA_OBJECT_RELEASE(src_pid);
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
    if (NULL != peeradv)
        JXTA_OBJECT_RELEASE(peeradv);
    if (NULL != qAdvs)
        JXTA_OBJECT_RELEASE(qAdvs);
    if (dq)
        JXTA_OBJECT_RELEASE(dq);
    if (NULL != attr)
        JXTA_OBJECT_RELEASE(attr);
    if (NULL != val)
        JXTA_OBJECT_RELEASE(val);
    if (responses)
        JXTA_OBJECT_RELEASE(responses);
    if (NULL != keys) {
        i = 0;
        while (keys[i]) {
            free(keys[i++]);
        }
        free(keys);
    }
    return res;
}

void *APR_THREAD_FUNC discovery_send_response_thread(apr_thread_t *apr_thread, void *arg)
{
    DR_thread_struct *thread;

    thread = (DR_thread_struct *) arg;
    if (!thread->discovery->running) {
        free(thread);
        return JXTA_SUCCESS;
    }
    discovery_send_discovery_response(thread->discovery, thread->rq, thread->dr);
    JXTA_OBJECT_RELEASE(thread->discovery);
    JXTA_OBJECT_RELEASE(thread->rq);
    JXTA_OBJECT_RELEASE(thread->dr);

    free(thread);
    return JXTA_SUCCESS;
}

static Jxta_status discovery_adjust_dr_entries(Jxta_DiscoveryResponse * dr
                    , apr_int32_t diff, apr_int32_t prev_diff)
{
    return JXTA_SUCCESS;
}

static Jxta_status discovery_send_discovery_response(Jxta_discovery_service_ref * discovery
                            , ResolverQuery * rq, Jxta_DiscoveryResponse *dr)
{
    Jxta_status status;
    Jxta_status res=JXTA_SUCCESS;
    Jxta_boolean init=TRUE;
    apr_int32_t prev_diff=0;
    Jxta_vector *dr_rsps=NULL;
    int i;
    apr_int32_t rr_length=0;
    int split_already = 0;

    dr_rsps = jxta_vector_new(0);
    jxta_vector_add_object_last(dr_rsps, (Jxta_object *) dr);

    for (i=0; i<jxta_vector_size(dr_rsps); i++) {
        JString *rdoc = NULL;
        Jxta_id *src_pid=NULL;
        JString *rr_j=NULL;
        apr_int64_t max_length=0;
        ResolverResponse *res_response = NULL;
        Jxta_DiscoveryResponse *dr_msg = NULL;
        Jxta_boolean break_it = FALSE;

        apr_int32_t diff=0;
        jxta_vector_get_object_at(dr_rsps, JXTA_OBJECT_PPTR(&dr_msg), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending rsp msg [%pp]\n", dr_msg );

        if (FALSE == init) {
            diff = jpr_time_now() - jxta_discovery_response_timestamp(dr_msg);
            res = discovery_adjust_dr_entries(dr_msg, diff, prev_diff);
        } else {
            ResolverResponse *calc_response=NULL;

            calc_response = jxta_resolver_response_new_2(discovery->instanceName
                                    , NULL, jxta_resolver_query_get_queryid(rq)
                                    , discovery->localPeerId);

            if (NULL != calc_response) {
                jxta_resolver_response_get_xml(calc_response, &rr_j);
                JXTA_OBJECT_RELEASE(calc_response);
                rr_length = jstring_length(rr_j);
                JXTA_OBJECT_RELEASE(rr_j);
                rr_j = NULL;
            } else {
                JXTA_OBJECT_RELEASE(dr_msg);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "unable to create resolver response\n");
                break;
            }
        }
        init = FALSE;
        status = jxta_discovery_response_get_xml(dr_msg, &rdoc);

        res_response = jxta_resolver_response_new_2(discovery->instanceName
                                    , rdoc, jxta_resolver_query_get_queryid(rq)
                                    , discovery->localPeerId);

        JXTA_OBJECT_RELEASE(rdoc);
        jxta_advertisement_get_xml((Jxta_advertisement *) res_response, &rdoc);

        src_pid = jxta_resolver_query_get_src_peer_id(rq);
        jxta_resolver_response_attach_qos(res_response, jxta_resolver_query_qos(rq));

        res = jxta_resolver_service_sendResponse(discovery->resolver, res_response, src_pid, &max_length);

        if (JXTA_LENGTH_EXCEEDED == res && split_already++ <= 2) {
            int j;
            Jxta_vector *new_responses=NULL;

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING
                                    , "discovery:send res_response [%pp] exceeded max:%ld\n"
                                    , res_response, max_length);

            split_discovery_responses(dr, rdoc, &new_responses, max_length - rr_length);

            jxta_vector_remove_object_at(dr_rsps, NULL, i--);

            for (j=0; NULL != new_responses && j<jxta_vector_size(new_responses); j++) {
                Jxta_DiscoveryResponse *new_dr_rsp=NULL;
                JString *new_dr_xml=NULL;

                jxta_vector_get_object_at(new_responses, JXTA_OBJECT_PPTR(&new_dr_rsp), j);
                jxta_discovery_response_set_timestamp(new_dr_rsp, jpr_time_now());

                jxta_discovery_response_get_xml(new_dr_rsp, &new_dr_xml);

                jxta_vector_add_object_last(dr_rsps, (Jxta_object *) new_dr_rsp);

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Add dr rsp [%pp] len:%d\n", new_dr_rsp, jstring_length(new_dr_xml));

                if (new_dr_rsp)
                    JXTA_OBJECT_RELEASE(new_dr_rsp);
                if (new_dr_xml)
                    JXTA_OBJECT_RELEASE(new_dr_xml);
            }
            if (new_responses)
                JXTA_OBJECT_RELEASE(new_responses);
        } else if (JXTA_LENGTH_EXCEEDED == res) {
            break_it = TRUE;
        } else if (JXTA_BUSY == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "rsp [%pp] busy\n", res_response);
            apr_sleep(1000 * 1000);
            if (!discovery->running) {
                break_it = TRUE;
            }
            i--;
            prev_diff = diff;
        } else if (JXTA_SUCCESS == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID
                                    , "remove object at :%d with size:%d\n", i, jxta_vector_size(dr_rsps));
            split_already = 0;
            jxta_vector_remove_object_at(dr_rsps, NULL, i--);
            prev_diff = 0;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "discovery rcvd error on send res: %d\n", res);
            break_it = TRUE;
        }
        if (rr_j)
            JXTA_OBJECT_RELEASE(rr_j);
        if (res_response)
            JXTA_OBJECT_RELEASE(res_response);
        if (rdoc)
            JXTA_OBJECT_RELEASE(rdoc);
        if (src_pid)
            JXTA_OBJECT_RELEASE(src_pid);
        if (dr_msg)
            JXTA_OBJECT_RELEASE(dr_msg);
        if (break_it) break;
    }

    if (dr_rsps)
        JXTA_OBJECT_RELEASE(dr_rsps);
    return res;
}

static Jxta_status split_discovery_responses(Jxta_DiscoveryResponse *dr, JString * response_j, Jxta_vector **new_responses, apr_int64_t max_length)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_DiscoveryResponse *dr_tmp=NULL;
    apr_int64_t total_length=0;
    Jxta_vector *all_entries;
    Jxta_vector *elem_v;
    Jxta_vector *responses=NULL;
    JString *tmp_j=NULL;
    int msg_length=0;
    int i;

    *new_responses = jxta_vector_new(0);

    dr_tmp = jxta_discovery_response_new();
    jxta_discovery_response_parse_charbuffer(dr_tmp, jstring_get_string(response_j), jstring_length(response_j));
    jxta_discovery_response_set_responses(dr_tmp, NULL);

    jxta_discovery_response_get_xml(dr_tmp, &tmp_j);
    msg_length = jstring_length(tmp_j);
    total_length = msg_length;
 
    JXTA_OBJECT_RELEASE(dr_tmp);
    JXTA_OBJECT_RELEASE(tmp_j);
    tmp_j = NULL;

    all_entries = jxta_vector_new(0);
    elem_v = jxta_vector_new(0);
    res = jxta_discovery_response_get_responses(dr, &responses);
    if (NULL == responses) {
        goto FINAL_EXIT;
    }

    for (i = 0; i < jxta_vector_size(responses); i++) {
        Jxta_DiscoveryResponseElement *rsp_element=NULL;
        Jxta_DiscoveryResponse *dr_new=NULL;
        JString *tmp_j_1=NULL;
        tmp_j=NULL;

        res = jxta_vector_get_object_at(responses, JXTA_OBJECT_PPTR(&rsp_element), i);

        jxta_xml_util_encode_jstring(rsp_element->response, &tmp_j);
        jxta_xml_util_encode_jstring(tmp_j, &tmp_j_1);

        total_length += (jstring_length(tmp_j_1) + DR_TAG_SIZE);

        if (total_length >= max_length) {
            if (dr_new) {
                JXTA_OBJECT_RELEASE(dr_new);
                dr_new = NULL;
            }
            dr_new = jxta_discovery_response_new();
            jxta_discovery_response_parse_charbuffer(dr_new, jstring_get_string(response_j), jstring_length(response_j));
            jxta_discovery_response_set_responses(dr_new, all_entries);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "discovery:split add new rsp [%pp]\n", dr_new);
            jxta_vector_add_object_last(*new_responses, (Jxta_object *) dr_new);
            JXTA_OBJECT_RELEASE(all_entries);
            all_entries = jxta_vector_new(0);
            total_length = (jstring_length(tmp_j) + DR_TAG_SIZE);
            total_length += (msg_length);
            if (total_length < max_length) {
                jxta_vector_add_object_last(all_entries, (Jxta_object *) rsp_element);
            }

        } else {
            if ((jstring_length(rsp_element->response) + DR_TAG_SIZE + total_length) < max_length) {
                jxta_vector_add_object_last(all_entries, (Jxta_object *) rsp_element);
            }
        }
        if (rsp_element)
            JXTA_OBJECT_RELEASE(rsp_element);
        if (dr_new)
            JXTA_OBJECT_RELEASE(dr_new);
        if (tmp_j)
            JXTA_OBJECT_RELEASE(tmp_j);
        if (tmp_j_1)
            JXTA_OBJECT_RELEASE(tmp_j_1);

    }
    if (jxta_vector_size(all_entries) > 0) {
        Jxta_DiscoveryResponse *dr_add=NULL;

        dr_add = jxta_discovery_response_new();
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "discovery:split dr_add [%pp]\n", dr_add);

        jxta_discovery_response_parse_charbuffer(dr_add, jstring_get_string(response_j), jstring_length(response_j));
        jxta_discovery_response_set_responses(dr_add, all_entries);
        jxta_vector_add_object_last(*new_responses, (Jxta_object *) dr_add);

        JXTA_OBJECT_RELEASE(dr_add);
    }


FINAL_EXIT:
    if (elem_v)
        JXTA_OBJECT_RELEASE(elem_v);
    if (all_entries)
        JXTA_OBJECT_RELEASE(all_entries);
    if (responses)
        JXTA_OBJECT_RELEASE(responses);
    return res;
}
    /* construct response and send it back */

static Jxta_status discovery_get_responses(Jxta_discovery_service_ref *discovery, int threshold, short type, char **keys_parm, Jxta_vector **responses)
{
    Jxta_status status=JXTA_SUCCESS;
    char **keys;
    int i = 0;

    keys = keys_parm;
    *responses = jxta_vector_new(1);
    while (NULL != keys[i]) {
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
            i++;
            continue;
        }
        status = cm_restore_bytes(discovery->cm, (char *) dirname[type], keys[i], &responseString);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error restoring bytes of advid %s\n", keys[i]);
            i++;
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
            i++;
            JXTA_OBJECT_RELEASE(responseString);
            continue;
        }

        if ((lifetime - (Jxta_expiration_time) jpr_time_now()) < expiration)
            responseTime = lifetime - jpr_time_now();
        else
            responseTime = expiration;

        dre = jxta_discovery_response_new_element_1(responseString, responseTime);
        jxta_vector_add_object_last(*responses, (Jxta_object *) dre);

        JXTA_OBJECT_RELEASE(dre);
        JXTA_OBJECT_RELEASE(responseString);


        i++;
        if ((i + 1) > (unsigned int) threshold) {
            break;
        }
    }
    return status;
}

Jxta_status discovery_send_srdi_msg(Jxta_discovery_service_ref *discovery, Jxta_id * to_peerid, Jxta_id * peerid, Jxta_id *source_peerid, JString *jPrimaryKey, Jxta_vector * entries)
{
    Jxta_SRDIMessage *msg = NULL;
    Jxta_status res=JXTA_SUCCESS;

    msg = jxta_srdi_message_new_3(1, peerid, source_peerid, (char *) jstring_get_string(jPrimaryKey), entries);

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate jxta_srdi_message_new\n");
        goto FINAL_EXIT;
    }
    jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->instanceName, msg, to_peerid, FALSE, FALSE, NULL);

FINAL_EXIT:

    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status discovery_filter_advid_duplicates(Jxta_discovery_service_ref *discovery, Jxta_PID * from_peer_id, Jxta_vector * entries, Jxta_vector ** dupEntries)
{
    Jxta_status res = JXTA_SUCCESS;
    int i;

    assert(NULL != dupEntries);
    assert(NULL != entries);

    *dupEntries = NULL;
    for (i=0; i<jxta_vector_size(entries); i++) {
        Jxta_SRDIEntryElement * entry;
        Jxta_peer *advPeer = NULL;
        Jxta_peer *alt_advPeer = NULL;

        res = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve srdi element res: %d\n", res);
            goto FINAL_EXIT;
        }
        advPeer = jxta_srdi_getReplicaPeer(discovery->srdi, jstring_get_string(entry->advId), &alt_advPeer);
        if (NULL != advPeer) {
            if (entry->dup_peerid) {
                JXTA_OBJECT_RELEASE(entry->dup_peerid);
            }
            if (jxta_id_equals(jxta_peer_peerid(advPeer), from_peer_id) && NULL != alt_advPeer) {

                JXTA_OBJECT_RELEASE(advPeer);
                advPeer = alt_advPeer;
                alt_advPeer = NULL;
                if (entry->duplicate) {
                    JString *alt_peer_j=NULL;
                    jxta_id_to_jstring(from_peer_id, &alt_peer_j);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received an alternate %" APR_INT64_T_FMT " duplicate from %s\n",entry->seqNumber , jstring_get_string(alt_peer_j));
                    JXTA_OBJECT_RELEASE(alt_peer_j);
                }
            }
            jxta_id_to_jstring(jxta_peer_peerid(advPeer), &entry->dup_peerid);
        }
        /* if this is a duplicate entry and this peer is the duplicate */
        if (entry->duplicate) {

            if (NULL != advPeer && jxta_id_equals(discovery->localPeerId, jxta_peer_peerid(advPeer))) {
                if (NULL == *dupEntries) {
                    *dupEntries = jxta_vector_new(0);
                }
                jxta_vector_add_object_last(*dupEntries, (Jxta_object *) entry);
                /* remove it from the entries */
                jxta_vector_remove_object_at(entries, NULL, i--);
            }
        }
        JXTA_OBJECT_RELEASE(entry);
        if (advPeer)
            JXTA_OBJECT_RELEASE(advPeer);
        if (alt_advPeer)
            JXTA_OBJECT_RELEASE(alt_advPeer);
    }

FINAL_EXIT:

    return res;
}

static Jxta_status discovery_notify_peerview_peers(Jxta_discovery_service_ref *discovery, Jxta_id *src_pid, Jxta_id *prev_pid, Jxta_vector *lost_pids)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_peerview *pv=NULL;
    int i;
    JString *prev_pid_j=NULL;
    JString *src_pid_j=NULL;
    Jxta_vector *peers=NULL;
    Jxta_boolean pv_done = FALSE;

    pv = jxta_rdv_service_get_peerview(discovery->rdv);
    if (NULL == pv) {
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    if (NULL != prev_pid && NULL != src_pid) {
        jxta_id_to_jstring(prev_pid, &prev_pid_j);
        jxta_id_to_jstring(src_pid, &src_pid_j);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Notify targets that %s was attached to %s \n"
                        , jstring_get_string(src_pid_j), jstring_get_string(prev_pid_j));
    }
    /* send to partners */
    jxta_peerview_get_localview(pv, &peers);

    while (TRUE) {
        /* broadcast to the peerview */
        for (i=0; i<jxta_vector_size(peers); i++) {
            Jxta_peer *peer=NULL;
            Jxta_SRDIMessage *smsg=NULL;

            res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (JXTA_SUCCESS != res) {
                continue;

            }
            if (jxta_id_equals(jxta_peer_peerid(peer), discovery->localPeerId)) {
                JXTA_OBJECT_RELEASE(peer);
                continue;
            }
            smsg = jxta_srdi_message_new();
            jxta_srdi_message_set_SrcPID(smsg, src_pid);
            jxta_srdi_message_set_PrevPID(smsg, prev_pid);
            jxta_srdi_message_set_LostPIDs(smsg, lost_pids);
            jxta_srdi_message_set_peerID(smsg, discovery->localPeerId);
            jxta_srdi_message_set_ttl(smsg, 1);
            jxta_srdi_message_set_entries(smsg, NULL);

            res = jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->instanceName, smsg, jxta_peer_peerid(peer), TRUE, FALSE, NULL);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error sending notify message [%pp] res:%d\n", smsg, res);
            } else {
                JString *peer_j=NULL;

                jxta_id_to_jstring(jxta_peer_peerid(peer), &peer_j);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sent notify message [%pp] to %s\n", smsg, jstring_get_string(peer_j));

                JXTA_OBJECT_RELEASE(peer_j);
            }
            if (smsg)
                JXTA_OBJECT_RELEASE(smsg);
            if (peer)
                JXTA_OBJECT_RELEASE(peer);
            }
        if (peers)
            JXTA_OBJECT_RELEASE(peers);
        peers = NULL;
        if (TRUE == pv_done) break;
        jxta_peerview_get_globalview(pv, &peers);
        pv_done = TRUE;
        }
FINAL_EXIT:
    if (peers)
        JXTA_OBJECT_RELEASE(peers);
    if (pv)
        JXTA_OBJECT_RELEASE(pv);
    if (prev_pid_j)
        JXTA_OBJECT_RELEASE(prev_pid_j);
    if (src_pid_j)
        JXTA_OBJECT_RELEASE(src_pid_j);
    return res;
}

static void discovery_service_update_srdi_peers(Jxta_discovery_service_ref *discovery, Jxta_id *peerid, Jxta_SRDIMessage *smsg, Jxta_boolean bReplica)
{
    Jxta_vector *lost_pids;
    Jxta_status status;
    int i;
    Jxta_vector *idx_entries=NULL;
    Jxta_srdi_idx_entry *idx_entry=NULL;
    JString *peerid_j=NULL;
    Jxta_id *working_pid=NULL;
    JString *working_pid_j=NULL;

    jxta_id_to_jstring(peerid, &peerid_j);
    jxta_srdi_message_get_PrevPID(smsg, &working_pid);

    if (NULL != working_pid) {
        
        if (!bReplica) {
            /* notify the peerview the replicating rendezvous has changed */
            discovery_notify_peerview_peers(discovery, peerid, working_pid, NULL);
        }

        jxta_id_to_jstring(working_pid, &working_pid_j);

        status = cm_get_srdi_replica_index_entries(discovery->cm, working_pid_j, &idx_entries);

        if (JXTA_SUCCESS != status) {
            goto FINAL_EXIT;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "There are %d idx entries that have %s as its peerid\n"
                                , jxta_vector_size(idx_entries), jstring_get_string(working_pid_j));

        for (i = 0; i < jxta_vector_size(idx_entries); i++) {
            Jxta_id *peer_id_temp=NULL;

            status = jxta_vector_get_object_at(idx_entries, JXTA_OBJECT_PPTR(&idx_entry), i);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to get idx entry status:%d\n", status);
                continue;
            }
            jxta_id_from_jstring(&peer_id_temp, idx_entry->peer_id);

            /* the lost rendezvous peer should be updated locally */
            if (jxta_id_equals(working_pid, peer_id_temp)) {
                if (idx_entry->peer_id) {
                    JXTA_OBJECT_RELEASE(idx_entry->peer_id);
                }
                jxta_id_to_jstring(peerid, &idx_entry->peer_id);
            } else {
                jxta_vector_remove_object_at(idx_entries, NULL, i--);
            }
            JXTA_OBJECT_RELEASE(peer_id_temp);
            JXTA_OBJECT_RELEASE(idx_entry);
            idx_entry = NULL;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Updating %d entries that had %s as its peerid\n"
                                , jxta_vector_size(idx_entries), jstring_get_string(working_pid_j));

        if (jxta_vector_size(idx_entries) > 0) {
            cm_update_replicating_peers(discovery->cm, idx_entries, working_pid_j);
        }
        JXTA_OBJECT_RELEASE(idx_entries);
        idx_entries = NULL;
        JXTA_OBJECT_RELEASE(working_pid_j);
        working_pid_j = NULL;
    }
    if (NULL != working_pid) {
        JXTA_OBJECT_RELEASE(working_pid);
        working_pid = NULL;
    }
    jxta_srdi_message_get_LostPIDs(smsg, &lost_pids);
    for (i = 0; NULL != lost_pids && i < jxta_vector_size(lost_pids); i++) {

        status = jxta_vector_get_object_at(lost_pids, JXTA_OBJECT_PPTR(&working_pid), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to get lost pid status:%d\n", status);
            continue;
        }
        jxta_id_to_jstring(working_pid, &working_pid_j);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Need to update the lost pid %s\n", jstring_get_string(working_pid_j));

        status = cm_flag_index_entries(discovery->cm, peerid_j, working_pid_j, 0, FALSE);

        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to update index entries:%d\n", status);
            goto FINAL_EXIT;
        }
        JXTA_OBJECT_RELEASE(working_pid);
        working_pid = NULL;
        JXTA_OBJECT_RELEASE(working_pid_j);
        working_pid_j = NULL;
    }

FINAL_EXIT:
    if (peerid_j)
        JXTA_OBJECT_RELEASE(peerid_j);
    if (idx_entry)
        JXTA_OBJECT_RELEASE(idx_entry);
    if (idx_entries)
        JXTA_OBJECT_RELEASE(idx_entries);
    if (lost_pids)
        JXTA_OBJECT_RELEASE(lost_pids);
    if (working_pid)
        JXTA_OBJECT_RELEASE(working_pid);
    if (working_pid_j)
        JXTA_OBJECT_RELEASE(working_pid_j);
}

static void JXTA_STDCALL discovery_service_srdi_listener(Jxta_object * obj, void *arg)
{
    Jxta_SRDIMessage *smsg;
    Jxta_vector *entries = NULL;
    Jxta_vector *msg_entries = NULL;
    Jxta_vector *resendDelta=NULL;
    Jxta_hashtable *resend_hash = NULL;
    Jxta_hashtable *reverse_hash = NULL;
    Jxta_status status;
    JString *pid_j = NULL;
    JString *src_pid_j = NULL;
    JString *jPrimaryKey = NULL;
    Jxta_peer *peer;
    Jxta_boolean bReplica = TRUE;
    Jxta_boolean am_rdv = FALSE;
    Jxta_id *peerid = NULL;
    Jxta_id *src_pid = NULL;
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
    jxta_id_to_jstring(peerid, &pid_j);

    /* is this not from an edge consider it a replica */
    if (JXTA_SUCCESS == jxta_rdv_service_get_peer(discovery->rdv, peerid, &peer)) {
        bReplica = FALSE;
        JXTA_OBJECT_RELEASE(peer);
    }

    if (pid_j == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message without a peerid \n");
        goto FINAL_EXIT;
    }

    jxta_srdi_message_get_SrcPID(smsg, &src_pid);
    if (NULL != src_pid) {
        jxta_id_to_jstring(src_pid, &src_pid_j);
    } else {
        src_pid_j = JXTA_OBJECT_SHARE(pid_j);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Received from:%s : msg %s \n"
                        , jstring_get_string(pid_j), jstring_get_string((JString *) obj));


    discovery_service_update_srdi_peers(discovery, peerid, smsg, bReplica);

    jxta_srdi_message_get_primaryKey(smsg, &jPrimaryKey);

    status = jxta_srdi_message_get_resend_entries(smsg, &entries);

    if (!am_rdv && entries == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message without resend entries and i'm not a rendezvous \n");
        goto FINAL_EXIT;
    }

    /* process resend entries */
    if (NULL != entries) {
        JString *peerid_j = NULL;
        char **source_peers = NULL;
        char **source_peers_save = NULL;

        cm_get_resend_delta_entries(discovery->cm, NULL == peerid_j ? pid_j : peerid_j, entries, &resend_hash, &reverse_hash);

        if (NULL != peerid_j)
            JXTA_OBJECT_RELEASE(peerid_j);

        if (NULL != resend_hash) {

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
                discovery_send_srdi_msg(discovery, peerid , discovery->localPeerId, source_peerid, jPrimaryKey, resendDelta);
                JXTA_OBJECT_RELEASE(source_peerid);
                JXTA_OBJECT_RELEASE(resendDelta);
                resendDelta = NULL;
                free(*(source_peers++));
            }
            free(source_peers_save);
            JXTA_OBJECT_RELEASE(resend_hash);
        }
        if (NULL != reverse_hash) {
            source_peers = jxta_hashtable_keys_get(reverse_hash);
            source_peers_save = source_peers;

            /* Need to add source id to message since the same advertisement may have come from multiple sources */
            while (*source_peers) {
                Jxta_id * source_peerid=NULL;

                status = jxta_hashtable_get(reverse_hash, *source_peers, strlen(*source_peers) + 1, JXTA_OBJECT_PPTR(&resendDelta));
                if (JXTA_SUCCESS != status) {
                    free(*(source_peers++));
                    continue;
                }

                jxta_id_from_cstr(&source_peerid, *source_peers);
                discovery_send_srdi_msg(discovery, source_peerid , discovery->localPeerId, src_pid, jPrimaryKey, resendDelta);
                JXTA_OBJECT_RELEASE(source_peerid);
                JXTA_OBJECT_RELEASE(resendDelta);
                resendDelta = NULL;
                free(*(source_peers++));
            }
            free(source_peers_save);
            JXTA_OBJECT_RELEASE(reverse_hash);

        }
        JXTA_OBJECT_RELEASE(entries);
        entries = NULL;
        if (!am_rdv)
            goto FINAL_EXIT;
    }

    /* get the entries within the message */
    status = jxta_srdi_message_get_entries(smsg, &msg_entries);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received %ld srdi msg entries from: %s \n", jxta_vector_size(msg_entries), jstring_get_string(pid_j));


    if (status != JXTA_SUCCESS || NULL == msg_entries) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Something is wrong with the message status:%d resenddelta: %s\n",status, NULL == resendDelta? "NULL":"not NULL");
        goto FINAL_EXIT;
    }

    /* take all delta entries received and store the values in the entries */

    status = cm_expand_delta_entries(discovery->cm, msg_entries, pid_j, src_pid_j, &entries, FALSE);

    /* check if delta sequence numbers weren't found and send resend requests */

    for (i=0; NULL != entries && i < jxta_vector_size(entries); i++) {
        Jxta_SRDIEntryElement * entry;
        status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        if (entry->resend) {
            if (NULL == resendDelta) {
                resendDelta = jxta_vector_new(0);
            }
            /* add to request resend and remove it from further processing */
            jxta_vector_add_object_last(resendDelta, (Jxta_object *) entry);
            jxta_vector_remove_object_at(entries, NULL, i--);
        }
        JXTA_OBJECT_RELEASE(entry);
    }
    if (NULL != resendDelta) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Request %d resend entries \n", jxta_vector_size(resendDelta));

        discovery_send_srdi_msg(discovery, peerid, discovery->localPeerId, src_pid , jPrimaryKey, resendDelta);

        JXTA_OBJECT_RELEASE(resendDelta);
        resendDelta = NULL;
    }

    JXTA_OBJECT_RELEASE(msg_entries);

    /* if we've exhausted the entries we're done */

    if (NULL == entries || 0 == jxta_vector_size(entries)) {
        goto FINAL_EXIT;
    }

    /* set the remaining entries in the message */
    jxta_srdi_message_set_entries(smsg, entries);

    /* process normal srdi entries */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got srdi with peerid -- %s Number of srdi/replica elements %i\n",
                    jstring_get_string(pid_j)
                    , jxta_vector_size(entries));

    if (!bReplica) {
        jxta_srdi_message_set_ttl(smsg, 2);
        if (!jxta_srdi_message_update_only(smsg)) {
            Jxta_status busy_status;
            Jxta_endpoint_address *ea;

            ea = jxta_endpoint_address_new_3(peerid, NULL, NULL);

            busy_status = jxta_srdi_replicateEntries(discovery->srdi, discovery->resolver, smsg, discovery->instanceName, NULL);
            if (JXTA_BUSY == busy_status) {
                Jxta_ep_flow_control *flow_control;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Flow control the remote peer *****************\n");
                jxta_endpoint_service_send_fc(discovery->endpoint, ea, FALSE);
            } else if (JXTA_SUCCESS == busy_status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Continue at normal flow ********\n");
                jxta_endpoint_service_send_fc(discovery->endpoint, ea, TRUE);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error returned from repicateEntries:%ld\n"
                                , busy_status);
            }
            JXTA_OBJECT_RELEASE(ea);
        }
    }

    if (NULL != discovery->cm && jxta_vector_size(entries) > 0) {
        Jxta_vector * resendEntries = NULL;
        Jxta_vector * dupEntries = NULL;
        Jxta_hashtable * fwdEntries = NULL;
        const char *err_msg = NULL;
        char err_msg_tmp[256];
        Jxta_peerview *pv = NULL;


        pv = jxta_rdv_service_get_peerview(discovery->rdv);

        if (bReplica && jxta_peerview_is_associate(pv, peerid)) {

            /* check for a more specialized node */
            status = discovery_filter_replicas_forward(discovery, entries, jstring_get_string(pid_j), pid_j, &fwdEntries);

            if (JXTA_SUCCESS != status) {
                err_msg = "Unable to filter replicas forward";
            } else if (fwdEntries) {
                char **peers;
                char **peers_save;

                peers = jxta_hashtable_keys_get(fwdEntries);
                peers_save = peers;
                while (*peers) {
                    Jxta_vector *peer_entries=NULL;
                    Jxta_id *to_peerid = NULL;
                    Jxta_SRDIMessage *msg=NULL;
                    JString * peerid_j=NULL;

                    jxta_hashtable_get(fwdEntries, *peers, strlen(*peers) + 1, JXTA_OBJECT_PPTR(&peer_entries));

                    /* create a message that uses this peer and source from the received message */
                    msg = jxta_srdi_message_new_2(1, discovery->localPeerId , src_pid, (char *) jstring_get_string(jPrimaryKey), peer_entries);

                    if (msg == NULL) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate jxta_srdi_message_new\n");
                        JXTA_OBJECT_RELEASE(peer_entries);
                        free(*(peers++));
                        continue;
                    }

                    jxta_id_from_cstr(&to_peerid, *peers);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "---------> Got a forward peer %s with %d entries\n", *peers, jxta_vector_size(peer_entries));

                    status = cm_save_replica_fwd_elements(discovery->cm, discovery->instanceName, pid_j, src_pid_j, jPrimaryKey, peer_entries, (Jxta_vector **) &resendEntries);
                    if (JXTA_SUCCESS != status) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to save replica forward indices status:%d\n", status);
                    }

                    status = jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->instanceName, msg, to_peerid, TRUE, FALSE, NULL);
                    if (JXTA_SUCCESS != status) {
                       jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Problem sending message with forwarded entries res:%d\n", status);
                    } else {
                        jxta_id_to_jstring(peerid, &peerid_j);
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Should have %d Forward to : %s peerid:%s\n"
                                 ,jxta_vector_size(peer_entries), *peers, jstring_get_string(peerid_j));
                    }
                    if (msg)
                        JXTA_OBJECT_RELEASE(msg);
                    if (peerid_j)
                        JXTA_OBJECT_RELEASE(peerid_j);
                    JXTA_OBJECT_RELEASE(to_peerid);
                    JXTA_OBJECT_RELEASE(peer_entries);
                    free(*(peers++));
                }
                free(peers_save);
                JXTA_OBJECT_RELEASE(fwdEntries);
            }
            if (JXTA_SUCCESS != status && NULL != err_msg) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, " %s status: %d\n", err_msg, status);
            }
        }
        if (bReplica) {
            /* isolate advid duplicates */
            status = discovery_filter_advid_duplicates(discovery, peerid, entries, &dupEntries);
            if (JXTA_SUCCESS == status) {

                if (dupEntries && jxta_vector_size(dupEntries) > 0) {

                    /* duplicates are stored in the srdi cache */

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "---------> calling cm save srdi with %d duplicates \n",jxta_vector_size(dupEntries));

                    status = cm_save_srdi_elements(discovery->cm, discovery->instanceName, pid_j, src_pid_j, jPrimaryKey, dupEntries, (Jxta_vector **) &resendEntries);
                    if (JXTA_SUCCESS != status) {
                        err_msg = "Unable to save SRDI elements";
                    }
                }
                if (JXTA_SUCCESS == status) {

                    if (JXTA_SUCCESS == status && entries && jxta_vector_size(entries) > 0) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "---------> calling cm save replica with %d entries and peerid %s\n"
                                , jxta_vector_size(entries)
                                , jstring_get_string(pid_j));

                        status = cm_save_replica_elements(discovery->cm, discovery->instanceName, pid_j, src_pid_j, jPrimaryKey, entries, (Jxta_vector **) &resendEntries);
                        if (JXTA_SUCCESS != status) {
                            err_msg = "Unable to save Replica elements";
                        }
                    }
                }
            }
            if (JXTA_SUCCESS == status && resendEntries) {
                status = discovery_send_srdi_msg(discovery, peerid, discovery->localPeerId, src_pid, jPrimaryKey, resendEntries);
            }
            if (JXTA_SUCCESS != status && NULL != err_msg) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, " %s status: %d\n", err_msg, status);
            }
            if (resendEntries) {
                JXTA_OBJECT_RELEASE(resendEntries);
            }
            if (dupEntries) {
                JXTA_OBJECT_RELEASE(dupEntries);
            }

        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "---------> calling cm save srdi with %d elements\n",jxta_vector_size(entries));
            status = cm_save_srdi_elements(discovery->cm, discovery->instanceName, pid_j, src_pid_j, jPrimaryKey, entries, (Jxta_vector **) &resendEntries);
            if (JXTA_SUCCESS != status) {
                apr_snprintf(err_msg_tmp, sizeof(err_msg_tmp), "Unable to save %s in discovery_service_srdi_listener %ld\n",
                                        bReplica == FALSE ? "srdi":"replica", status);
                err_msg = (const char *) &err_msg_tmp;
            }
        }
        if (pv) {
            JXTA_OBJECT_RELEASE(pv);
        }
    }
  FINAL_EXIT:
    if (peerid)
        JXTA_OBJECT_RELEASE(peerid);
    if (src_pid)
        JXTA_OBJECT_RELEASE(src_pid);
    if (pid_j != NULL)
        JXTA_OBJECT_RELEASE(pid_j);
    if (src_pid_j)
        JXTA_OBJECT_RELEASE(src_pid_j);
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
    {    Jxta_vector *lost_pids;

        lost_pids = jxta_vector_new(0);
        /* let the peerview know we lost this peer */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Disconnected to rdv peer in Discovery =%s\n",
                        jstring_get_string(string));
        jxta_vector_add_object_last(lost_pids, (Jxta_object *) rdv_event->pid);

        discovery_notify_peerview_peers(discovery, discovery->localPeerId, NULL, lost_pids);

        JXTA_OBJECT_RELEASE(lost_pids);
        break;
    }
    case JXTA_RDV_CLIENT_EXPIRED:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rdv Client expired - %s \n", jstring_get_string(string));
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

        if (discovery->prev_id)
            JXTA_OBJECT_RELEASE(discovery->prev_id);

        discovery->prev_id = JXTA_OBJECT_SHARE(rdv_event->pid);
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
        res = discovery_push_srdi_2(discovery, republish, delta, FALSE);
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

static Jxta_vector * discovery_get_disc_services(JString *request_group)
{
    Jxta_vector *groups=NULL;
    Jxta_vector *dss=NULL;
    int i;

    dss = jxta_vector_new(0);
    groups = jxta_get_registered_groups();
    for (i=0; NULL != groups && i<jxta_vector_size(groups); i++) {
        Jxta_PG *group;
        JString *group_j;
        Jxta_discovery_service *ds;
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
            jxta_PG_get_discovery_service(group, &ds);

            if (NULL != ds) {
                jxta_vector_add_object_last(dss, (Jxta_object *) ds);
            }
            JXTA_OBJECT_RELEASE(ds);
        }
        JXTA_OBJECT_RELEASE(group);
        if (found && NULL !=request_group) break;
    }

    if (groups)
        JXTA_OBJECT_RELEASE(groups);
    return dss;
}

static void *APR_THREAD_FUNC delta_cycle_func(apr_thread_t *thread, void *arg)
{
    Jxta_status res=JXTA_SUCCESS;
    apr_thread_pool_t * tp;
    Jxta_vector *dss=NULL;
    int i;
    Jxta_hashtable *ret_msgs=NULL;

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "Enter delta cycle func\n");

    jxta_service_lock((Jxta_service*) discovery);

    /* As long as discovery is running */
    if (!discovery->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Discovery [%pp] has left the building\n", discovery);
        jxta_service_unlock((Jxta_service*)discovery);
        return NULL;
    }

    jxta_service_unlock((Jxta_service*)discovery);

    /* thread is null on initialization */
    if (NULL == thread) {
        goto FINAL_EXIT;
    }

    dss = discovery_get_disc_services(NULL);
    for (i=0; NULL != dss && i < jxta_vector_size(dss); i++) {
        Jxta_discovery_service_ref *discovery_ref=NULL;

        res = jxta_vector_get_object_at(dss, JXTA_OBJECT_PPTR(&discovery_ref), i);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unable to retrieve discovery vector entry at %d\n", i);
            continue;
        }
        /*
        * push delta if any
        */
        res = discovery_push_srdi_1(discovery_ref, &ret_msgs);

        if (TRUE == jxta_rdv_service_is_rendezvous((Jxta_rdv_service *) discovery_ref->rdv)) {
            cm_set_delta(discovery->cm, TRUE);
        }

        JXTA_OBJECT_RELEASE(discovery_ref);
    }
    if (NULL != ret_msgs) {
        jxta_srdi_pushSrdi_msgs(discovery->srdi, discovery->instanceName, ret_msgs);
        JXTA_OBJECT_RELEASE(ret_msgs);
    }
    discovery->delta_loop = 0;
  
FINAL_EXIT:
  
    if (dss)
        JXTA_OBJECT_RELEASE(dss);
    tp = jxta_PG_thread_pool_get(discovery->group);

    if (NULL != tp) {
        apr_thread_pool_schedule(tp, delta_cycle_func, discovery, jxta_discovery_config_get_delta_update_cycle(discovery->config) * 1000, discovery);
    }

    return NULL;
}

static void *APR_THREAD_FUNC cache_handling_func(apr_thread_t * thread, void *arg)
{
    apr_thread_pool_t * tp;

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;

    jxta_service_lock((Jxta_service*) discovery);

    /* As long as discovery is running */
    if (!discovery->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Discovery [%pp] has left the building\n", discovery);
        jxta_service_unlock((Jxta_service*) discovery);
        return NULL;
    }

    jxta_service_unlock((Jxta_service*) discovery);

    /* thread is null on initialization */
    if (NULL == thread) {
        goto FINAL_EXIT;
    }

    if (NULL == discovery->cm) {
        peergroup_get_cache_manager(discovery->group, &(discovery->cm));
    }
    if (NULL != discovery->cm) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Discovery is removing expired records from the CM\n");
        cm_remove_expired_records(discovery->cm);
        cm_remove_expired_response_entries(discovery->cm);
    }


FINAL_EXIT:

    tp = jxta_PG_thread_pool_get(discovery->group);

    if (NULL != tp) {
        apr_thread_pool_schedule(tp, cache_handling_func, discovery, jxta_discovery_config_get_expired_adv_cycle(discovery->config) * 1000, discovery);
    }

    return NULL;
}

static Jxta_status discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync)
{
    return discovery_delta_srdi_priv(discovery, ppublish, delta, sync, NULL);
}

static Jxta_status discovery_push_srdi_1(Jxta_discovery_service_ref * discovery, Jxta_hashtable **ret_msgs)
{
    return discovery_delta_srdi_priv(discovery, TRUE, TRUE, FALSE, ret_msgs);

}

static Jxta_status discovery_push_srdi_2(Jxta_discovery_service_ref * discovery, Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync)
{
    Jxta_status res;
    Jxta_hashtable *ret_msgs=NULL;

    res = discovery_delta_srdi_priv(discovery, ppublish, delta, sync, &ret_msgs);
    if (JXTA_SUCCESS == res) {
        if (NULL != ret_msgs) {
            jxta_srdi_pushSrdi_msgs(discovery->srdi, discovery->instanceName, ret_msgs);
        }
    }
    if (ret_msgs)
        JXTA_OBJECT_RELEASE(ret_msgs);
    return res;
}



static Jxta_status discovery_delta_srdi_priv(Jxta_discovery_service_ref * discovery
                                , Jxta_boolean ppublish, Jxta_boolean delta, Jxta_boolean sync, Jxta_hashtable **ret_msgs)
{
    Jxta_status status;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *entries = NULL;
    int i = 0;

    if (delta && ppublish) {
        JString *name = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "push SRDI delta %s\n", jstring_get_string(discovery->gid_str));
        for (i = FOLDER_MIN; i < FOLDER_MAX; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = cm_get_srdi_delta_entries(discovery->cm, name);
            if (entries != NULL && jxta_vector_size(entries) > 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send %d SRDI deltas\n", jxta_vector_size(entries));
                status = discovery_send_srdi((Jxta_discovery_service *) discovery, name, entries, NULL, FALSE, sync, ret_msgs);
                if (JXTA_SUCCESS != status || FALSE == discovery->running) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to send send SRDI deltas %i\n", status);
                    break;
                }
            }
            if (NULL != entries) {
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }
            JXTA_OBJECT_RELEASE(name);
            name = NULL;
        }
        for (i = FOLDER_MIN; i < FOLDER_MAX; i++) {
            Jxta_hashtable *peer_entries = NULL;

            cm_get_delta_entries_for_update(discovery->cm, dirname[i], NULL, &peer_entries);

            if (NULL != peer_entries) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send delta updates \n");
                discovery_service_send_delta_updates(discovery, dirname[i], peer_entries, ret_msgs);
                JXTA_OBJECT_RELEASE(peer_entries);
            }
        }
        if (NULL != entries) {
            JXTA_OBJECT_RELEASE(entries);
            entries = NULL;
        }

        if (NULL != name)
            JXTA_OBJECT_RELEASE(name);

    } else if (ppublish) {
        JString *name = NULL;
        Jxta_id *prev_id=NULL;

        cm_get_previous_delta_pid(discovery->cm, &prev_id);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "push all CM SRDI indexes \n");
        for (i = FOLDER_MIN; i < FOLDER_MAX; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = cm_create_srdi_entries(discovery->cm, name, NULL, NULL);
            if (entries != NULL && jxta_vector_size(entries) > 0) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "push all CM SRDI indexes for %s size:%d\n", dirname[i], jxta_vector_size(entries));
                status = discovery_send_srdi((Jxta_discovery_service *) discovery, name, entries, prev_id, TRUE, sync, ret_msgs);
                if (JXTA_SUCCESS != status || FALSE == discovery->running) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to send send SRDI messages %i\n", status);
                    break;
                }
            }
 
            if (NULL != entries) {
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }

            JXTA_OBJECT_RELEASE(name);
            name = NULL;
        }
        if (discovery->prev_id) {
            JXTA_OBJECT_RELEASE(discovery->prev_id);
            discovery->prev_id = NULL;
        }
        if (name)
            JXTA_OBJECT_RELEASE(name);
        if (prev_id)
            JXTA_OBJECT_RELEASE(prev_id);
    }

    if (NULL != entries)
        JXTA_OBJECT_RELEASE(entries);
    return res;
}

Jxta_status discovery_send_srdi(Jxta_discovery_service * me, JString * pkey, Jxta_vector * entries, Jxta_id *prev_id, Jxta_boolean updates_only, Jxta_boolean sync, Jxta_hashtable **ret_msgs)
{
    Jxta_discovery_service_ref * discovery = (Jxta_discovery_service_ref *) me;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_SRDIMessage *msg=NULL;
    const char *cKey = jstring_get_string(pkey);


    if (jxta_vector_size(entries) == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No entries received discovery_send_srdi\n");
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }
    msg = jxta_srdi_message_new_2(1, discovery->localPeerId, discovery->localPeerId, (char *) cKey, entries);

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "cannot allocate jxta_srdi_message_new\n");
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }


    jxta_srdi_message_set_PrevPID(msg, prev_id);

    if ( config_rendezvous == jxta_rdv_service_config( discovery->rdv ) ) {
        res = jxta_srdi_replicateEntries(discovery->srdi, discovery->resolver, msg, discovery->instanceName, ret_msgs);
    } else {
        jxta_srdi_message_set_ttl(msg, 2);
        jxta_srdi_message_set_update_only(msg, updates_only);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Push SRDI msg with ret_msgs %s\n", NULL == ret_msgs ? "no":"yes" );

        res = jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->instanceName, msg, NULL, TRUE, sync, ret_msgs);

    }

FINAL_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Deleted msg [%pp]\n", msg);
 
    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;

}

static Jxta_status discovery_service_send_delta_updates(Jxta_discovery_service_ref * discovery, const char * key, Jxta_hashtable * source_entries_update, Jxta_hashtable **ret_msgs)
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
            discovery_service_send_delta_update_msg(discovery, source_id, key, entries_update, ret_msgs);

            JXTA_OBJECT_RELEASE(entries_update);
            JXTA_OBJECT_RELEASE(source_id);
            JXTA_OBJECT_RELEASE(source_id_j);
        }
        free(*(source_peer_ids_tmp++));
    }
    free(source_peer_ids);
    return status;
}

static Jxta_status discovery_service_send_delta_update_msg(Jxta_discovery_service_ref * discovery, Jxta_PID *source_id, const char * key, Jxta_hashtable * entries_update, Jxta_hashtable **ret_msgs)
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
        JString *peerid_j = NULL;

        jxta_hashtable_get(entries_update, *peer_ids_tmp, strlen(*peer_ids_tmp) + 1, JXTA_OBJECT_PPTR(&entries));
        if (NULL == entries) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No entries were available for peerid - %s\n", *peer_ids_tmp);
            free(*peer_ids_tmp++);
            continue;
        }

        msg = jxta_srdi_message_new_2(1, discovery->localPeerId, source_id, (char *) key, entries);
        if (msg == NULL) {
            JXTA_OBJECT_RELEASE(entries);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot allocate jxta_srdi_message_new\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        if (strcmp(*peer_ids_tmp, "NoPeer")) {
            jxta_id_from_cstr(&peerid, *peer_ids_tmp);
        }
        jxta_srdi_pushSrdi_msg(discovery->srdi, discovery->instanceName, msg, peerid, FALSE, FALSE, ret_msgs);

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "cannot send srdi message [%pp} to %s\n", msg, *peer_ids_tmp);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got %d delta sequence updates for peer %s\n", jxta_vector_size(entries), *peer_ids_tmp);

            peerid_j = jstring_new_2(*peer_ids_tmp);
            jxta_id_to_jstring(source_id, &source_peerid_j);

            cm_update_delta_entries(discovery->cm, peerid_j, source_peerid_j, discovery->instanceName, entries, 0);
        }

        JXTA_OBJECT_RELEASE(peerid_j);
        JXTA_OBJECT_RELEASE(entries);
        JXTA_OBJECT_RELEASE(source_peerid_j);
        JXTA_OBJECT_RELEASE(msg);
        if (peerid)
            JXTA_OBJECT_RELEASE(peerid);
        free(*(peer_ids_tmp++));
    }

FINAL_EXIT:
    while (*peer_ids_tmp) {
        free(*(peer_ids_tmp++));
    }
    if (peer_ids)
        free(peer_ids);
    return status;
}

static Jxta_status discovery_filter_replicas_forward(Jxta_discovery_service_ref * discovery, Jxta_vector * entries, const char * source_peer, JString *jRepPeerid, Jxta_hashtable ** fwd_entries)
{
    Jxta_status status = JXTA_SUCCESS;
    int i;
    JString *replicaExpression = NULL;

    replicaExpression = jstring_new_0();

    for (i=0; i < jxta_vector_size(entries); i++) {
        Jxta_peer *replicaPeer = NULL;
        Jxta_peer *advPeer = NULL;
        Jxta_peer *alt_advPeer = NULL;
        const char *expression = NULL;
        Jxta_SRDIEntryElement *entry=NULL;

        status = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Couldn't retrieve an entry element when trying to forward\n");
            continue;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Entry %" APR_INT64_T_FMT " is_fwd:%s fwd_this:%s isdup:%s istarget:%s is_dup_fwd:%s\n", entry->seqNumber
                    , TRUE == entry->fwd ? "yes":"no"
                    , TRUE == entry->fwd_this ? "yes":"no"
                    , TRUE == entry->duplicate ? "yes":"no", TRUE == entry->dup_target ? "yes":"no", TRUE == entry->dup_fwd ? "yes":"no");
        /* if this is a forward already and it shouldn't be forwarded cache it locally */
        if ((entry->dup_fwd || entry->fwd) && !entry->fwd_this) {
            entry->cache_this = TRUE;
            JXTA_OBJECT_RELEASE(entry);
            continue;
        }
        advPeer = jxta_srdi_getReplicaPeer(discovery->srdi, jstring_get_string(entry->advId), &alt_advPeer);
        if (NULL != advPeer) {
            JString *adv_peer_j = NULL;

            jxta_id_to_jstring(jxta_peer_peerid(advPeer), &adv_peer_j);

            if (NULL != entry->dup_peerid) {
                JXTA_OBJECT_RELEASE(entry->dup_peerid);
            }

            if (!strcmp(jstring_get_string(adv_peer_j), source_peer) && NULL != alt_advPeer) {
                JXTA_OBJECT_RELEASE(adv_peer_j);
                jxta_id_to_jstring(jxta_peer_peerid(alt_advPeer), &adv_peer_j);
            }
            entry->dup_peerid = adv_peer_j;
        }

        /* if this isn't a forward already find a replica peer */
        if (!entry->fwd_this) {
            jstring_reset(replicaExpression, NULL);
            jstring_append_1(replicaExpression, entry->key);
            if (NULL != entry->value) {
                jstring_append_1(replicaExpression, entry->value);
            }
            expression = jstring_get_string(replicaExpression);
            replicaPeer = jxta_srdi_getReplicaPeer(discovery->srdi, expression, NULL);
        }
        /* if we have a replica peer or this is a duplicate or it needs to be forwarded */
        if (NULL != replicaPeer || entry->duplicate || entry->fwd_this) {
            JString * fwdPeer = NULL;
            Jxta_vector * fwd_peer_entries=NULL;
            Jxta_SRDIEntryElement *new_entry = NULL;
            Jxta_boolean fwd_entry = TRUE;
            Jxta_boolean fwd_duplicate = FALSE;
            Jxta_boolean fwd_is_duplicate = FALSE;

            /* forward the entry if there's a forward peer */
            if (entry->fwd_this && NULL != entry->fwd_peerid) {
                fwd_entry = TRUE;
                fwdPeer = JXTA_OBJECT_SHARE(entry->fwd_peerid);
            }
            /* if we have a replica peer and it shouldn't be forwarded */
            if (NULL != replicaPeer && !entry->fwd_this) {
                jxta_id_to_jstring(jxta_peer_peerid(replicaPeer), &fwdPeer);
                /* if the replica equals the source or this peer is the replica don't forward */
                if (!strcmp(jstring_get_string(fwdPeer), source_peer) || jxta_id_equals(jxta_peer_peerid(replicaPeer), discovery->localPeerId)) {
                    fwd_entry = FALSE;
                    entry->cache_this = TRUE;
                }
            }
            /* if it's a duplicate and not this peer or the peer that sent this --- forward duplicate */
            if (entry->duplicate && NULL != entry->dup_peerid
                    && strcmp(jstring_get_string(entry->dup_peerid), source_peer)
                    && strcmp(jstring_get_string(entry->dup_peerid), discovery->pid_str)) {
                    fwd_duplicate = TRUE;
            }
            /* if it needs to be forwarded */
            if (fwd_entry || fwd_duplicate) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Need to forward %" APR_INT64_T_FMT " to %s  %s fwd_duplicate:%s fwd_duplicate:%s\n", entry->seqNumber
                        , jstring_get_string(fwdPeer), NULL != expression ? expression:""
                        , fwd_duplicate ? "true":"false" , fwd_duplicate ? "true":"false");
                /* if we're forwarding a duplicate and the dup peerid is equal to the forward peerid */
                if (fwd_duplicate) {
                    if (NULL != fwdPeer && (NULL != entry->dup_peerid && !strcmp(jstring_get_string(entry->dup_peerid), jstring_get_string(fwdPeer)))) {
                        fwd_is_duplicate = TRUE;
                    }
                }
                if (NULL == *fwd_entries) {
                    *fwd_entries = jxta_hashtable_new(0);
                }
                /* if we have a forward peer and we are supposed to forward it */
                if (fwdPeer && fwd_entry) {
                    if (JXTA_SUCCESS != jxta_hashtable_get(*fwd_entries, jstring_get_string(fwdPeer), jstring_length(fwdPeer)+ 1, JXTA_OBJECT_PPTR(&fwd_peer_entries))) {
                        fwd_peer_entries = jxta_vector_new(0);
                        jxta_hashtable_put(*fwd_entries, jstring_get_string(fwdPeer), jstring_length(fwdPeer) + 1, (Jxta_object *) fwd_peer_entries);
                    }
                    new_entry = jxta_srdi_element_clone(entry);
                    new_entry->dup_target = fwd_is_duplicate;
                    new_entry->duplicate = fwd_is_duplicate;
                    new_entry->fwd = TRUE;
                    new_entry->fwd_this = TRUE;
                    if (new_entry->fwd_peerid) {
                        JXTA_OBJECT_RELEASE(new_entry->fwd_peerid);
                    }
                    new_entry->fwd_peerid = JXTA_OBJECT_SHARE(fwdPeer);
                    if (new_entry->rep_peerid) {
                        JXTA_OBJECT_RELEASE(new_entry->rep_peerid);
                    }
                    new_entry->rep_peerid = JXTA_OBJECT_SHARE(jRepPeerid);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "forwarding %" APR_INT64_T_FMT " to peerid: %s\n", new_entry->seqNumber, jstring_get_string(fwdPeer));
                    jxta_vector_add_object_last(fwd_peer_entries, (Jxta_object *) new_entry);
                }

                if (fwd_duplicate && !fwd_is_duplicate) {
                    if (fwd_peer_entries)
                        JXTA_OBJECT_RELEASE(fwd_peer_entries);
                    if (JXTA_SUCCESS != jxta_hashtable_get(*fwd_entries, jstring_get_string(entry->dup_peerid), jstring_length(entry->dup_peerid)+ 1, JXTA_OBJECT_PPTR(&fwd_peer_entries))) {
                        fwd_peer_entries = jxta_vector_new(0);
                        jxta_hashtable_put(*fwd_entries, jstring_get_string(entry->dup_peerid), jstring_length(entry->dup_peerid) + 1, (Jxta_object *) fwd_peer_entries);
                    }
                    if (new_entry)
                        JXTA_OBJECT_RELEASE(new_entry);
                    new_entry = jxta_srdi_element_clone(entry);
                    new_entry->duplicate = TRUE;
                    new_entry->dup_target = FALSE;
                    new_entry->dup_fwd = TRUE;
                    new_entry->fwd = TRUE;
                    new_entry->fwd_this = TRUE;
                    new_entry->cache_this = FALSE;
                    if (new_entry->fwd_peerid) {
                        JXTA_OBJECT_RELEASE(new_entry->fwd_peerid);
                    }
                    new_entry->fwd_peerid = JXTA_OBJECT_SHARE(entry->dup_peerid);
                    if (new_entry->rep_peerid) {
                        JXTA_OBJECT_RELEASE(new_entry->rep_peerid);
                    }
                    new_entry->rep_peerid = JXTA_OBJECT_SHARE(jRepPeerid);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "duplicating and fwd %" APR_INT64_T_FMT " to peerid: %s\n"
                                    , new_entry->seqNumber, jstring_get_string(new_entry->dup_peerid));
                    jxta_vector_add_object_last(fwd_peer_entries, (Jxta_object *) new_entry);
                }
                if (!entry->cache_this) {
                    /* remove from further processing */
                    jxta_vector_remove_object_at(entries, NULL, i--);
                }
            } else {
                /* just cache it locally */
                entry->cache_this = TRUE;
            }
            if (fwdPeer)
                JXTA_OBJECT_RELEASE(fwdPeer);
            if (fwd_peer_entries)
                JXTA_OBJECT_RELEASE(fwd_peer_entries);
            if (new_entry)
                JXTA_OBJECT_RELEASE(new_entry);
        }
        JXTA_OBJECT_RELEASE(entry);
        if (replicaPeer)
            JXTA_OBJECT_RELEASE(replicaPeer);
        if (advPeer)
            JXTA_OBJECT_RELEASE(advPeer);
        if (alt_advPeer)
            JXTA_OBJECT_RELEASE(alt_advPeer);
    }
    if (replicaExpression)
        JXTA_OBJECT_RELEASE(replicaExpression);

    return status;
}


/* vi: set ts=4 sw=4 tw=130 et: */
