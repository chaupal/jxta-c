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

static const char *__log_cat = "RSLVR";

#include <assert.h>

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_id.h"
#include "jxta_rr.h"
#include "jxta_rq.h"
#include "jxta_dr.h"
#include "jxta_rq_callback.h"
#include "jxta_rsrdi.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_message.h"

#include "jxta_private.h"
#include "jxta_resolver_service_private.h"
#include "jxta_discovery_service.h"
#include "jxta_routea.h"
#include "jxta_callback.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_peergroup_private.h"
#include "jxta_resolver_config_adv.h"

typedef struct {
    Extends(Jxta_resolver_service);
    Jxta_boolean running;
    Jxta_PG *group;
    Jxta_rdv_service *rendezvous;
    Jxta_endpoint_service *endpoint;
    Jxta_hashtable *queryhandlers;
    Jxta_hashtable *responsehandlers;
    Jxta_hashtable *srdihandlers;
    Jxta_PID *localPeerId;
    Jxta_advertisement *impl_adv;
    char *instanceName;
    char *inque;
    char *outque;
    char *srdique;
    apr_thread_mutex_t *mutex;
    long query_id;
    void *ep_cookies[3];
    size_t mru_capacity;
    size_t mru_size;
    size_t mru_pos;
    Jxta_PID **mru;
    Jxta_boolean alwaysPropagate;
} Jxta_resolver_service_ref;

/* used to make an address out of a peerid */
static Jxta_status JXTA_STDCALL resolver_service_query_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL resolver_service_response_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL resolver_service_srdi_cb(Jxta_object * obj, void *arg);
static long getid(Jxta_resolver_service_ref * resolver);
static Jxta_status learn_route_from_query(Jxta_resolver_service_ref * me, ResolverQuery * rq);

static const char *INQUENAMESHORT = "IRes";
static const char *OUTQUENAMESHORT = "ORes";
static const char *SRDIQUENAMESHORT = "Srdi";
static const char *JXTA_NAMESPACE = "jxta:";

static Jxta_status resolver_build_msg(Jxta_resolver_service_ref * me, JString *doc, const char *queue, const Jxta_qos * qos, Jxta_message **msg);
static Jxta_status get_rr_from_message(Jxta_resolver_service_ref *resolver, Jxta_message *msg, ResolverResponse **rr);

/* Most recently used cache for peers we sent RouteAdv */
static void mru_reset(Jxta_resolver_service_ref * me);
static Jxta_status mru_capacity_set(Jxta_resolver_service_ref * me, size_t capacity);
static void mru_check(Jxta_resolver_service_ref * me, ResolverQuery * query, Jxta_id * peerid);

/* event handlers */
static Jxta_status JXTA_STDCALL endpoint_event_handler(void *param, void *arg);

static Jxta_status JXTA_STDCALL endpoint_event_handler(void *param, void *arg)
{
    Jxta_endpoint_event *event = param;
    Jxta_resolver_service_ref *myself = arg;

    switch (event->type) {
    case JXTA_EP_LOCAL_ROUTE_CHANGE:
        mru_reset(myself);
        break;
    default:
        break;
    }

    return JXTA_SUCCESS;
}

/*
 * module methods
 */

/**
 * Initializes an instance of the Resolver Service.
 * 
 * @param service a pointer to the instance of the Resolver Service.
 * @param group a pointer to the PeerGroup the Resolver Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

Jxta_status
jxta_resolver_service_ref_init(Jxta_module * resolver, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_resolver_service_ref *self = (Jxta_resolver_service_ref *) resolver;
    apr_pool_t *pool;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_ResolverConfigAdvertisement *resolver_config = NULL;

    /* Test arguments first */
    if ((resolver == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_service_init((Jxta_service *) resolver, group, assigned_id, impl_adv);
    self->group = group;
    pool = jxta_PG_pool_get(group);
    jxta_PG_get_endpoint_service(group, &(self->endpoint));
    jxta_PG_get_rendezvous_service(group, &(self->rendezvous));
    jxta_PG_get_PID(group, &(self->localPeerId));
    /* Create the mutex */
    apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,  /* nested */
                            pool);

    self->queryhandlers = jxta_hashtable_new(3);
    self->responsehandlers = jxta_hashtable_new(3);
    self->srdihandlers = jxta_hashtable_new(3);
    if (self->queryhandlers == NULL || self->responsehandlers == NULL || self->srdihandlers == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return JXTA_NOMEM;
    }
    self->query_id = 0;

    /* keep a reference to our impl adv */
    if (impl_adv != 0) {
        JXTA_OBJECT_SHARE(impl_adv);
    }
    self->impl_adv = impl_adv;

    /**
     ** Build the local name of the instance
     **/
    self->instanceName = NULL;
    jxta_id_to_cstr(assigned_id, &self->instanceName, pool);

    {   /* don't release this object */
        JString *grp_str = NULL;
        Jxta_id *gid;

        jxta_PG_get_GID(self->group, &gid);
        jxta_id_get_uniqueportion(gid, &grp_str);
        JXTA_OBJECT_RELEASE(gid);
        self->inque = apr_pstrcat(pool, jstring_get_string(grp_str), INQUENAMESHORT, NULL);
        self->outque = apr_pstrcat(pool, jstring_get_string(grp_str), OUTQUENAMESHORT, NULL);
        self->srdique = apr_pstrcat(pool, jstring_get_string(grp_str), SRDIQUENAMESHORT, NULL);
        JXTA_OBJECT_RELEASE(grp_str);
        if (self->inque == NULL || self->outque == NULL || self->srdique == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            return JXTA_NOMEM;
        }
    }
    
    self->alwaysPropagate = FALSE;
    jxta_PG_get_configadv(self->group, &conf_adv);
    if (!conf_adv) {
        Jxta_PG *parentgroup=NULL;

        jxta_PG_get_parentgroup(self->group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }
    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_resolver_classid, &svc);
        if (NULL != svc) {
            resolver_config = jxta_svc_get_ResolverConfig(svc);
            if (NULL != resolver_config) {
                self->alwaysPropagate = jxta_ResolverConfigAdvertisement_get_always_propagate(resolver_config);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Resolver Service will propagate in addition to "
                    "walk: %s\n", self->alwaysPropagate ? "true" : "false");
                JXTA_OBJECT_RELEASE(resolver_config);
            }
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    self->mru_capacity = 20;
    self->mru = calloc(self->mru_capacity, sizeof(*self->mru));
    if (!self->mru) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to allocate memory for MRU, disable the feature\n");
    }
    self->mru_pos = self->mru_size = 0;
    return status;
}

/**
 * Initializes an instance of the Resolver Service. (exception variant).
 * 
 * @param service a pointer to the instance of the Resolver Service.
 * @param group a pointer to the PeerGroup the Resolver Service is 
 * initialized for.
 *
 */

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param this a pointer to the instance of the Resolver Service
 * @param argv a vector of string arguments.
 */
static Jxta_status start(Jxta_module * resolver, const char *argv[])
{
    Jxta_status rv = JXTA_SUCCESS;
    Jxta_resolver_service_ref *self = (Jxta_resolver_service_ref *) resolver;

    if (argv) {
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Starting ...\n");

    rv = endpoint_service_add_recipient(self->endpoint, &self->ep_cookies[0], self->instanceName,
                                        self->outque, resolver_service_query_cb, self);
    if (rv != JXTA_SUCCESS) {
        return rv;
    }

    rv = endpoint_service_add_recipient(self->endpoint, &self->ep_cookies[1], self->instanceName,
                                        self->inque, resolver_service_response_cb, self);
    if (rv != JXTA_SUCCESS) {
        return rv;
    }

    rv = endpoint_service_add_recipient(self->endpoint, &self->ep_cookies[2], self->instanceName,
                                        self->srdique, resolver_service_srdi_cb, self);
    if (rv != JXTA_SUCCESS) {
        return rv;
    }

    rv = jxta_service_events_connect((Jxta_service *) self->endpoint, endpoint_event_handler, self);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started\n");
    return rv;
}

/**
 * Stops an instance of the Resolver Service.
 * 
 * @param service a pointer to the instance of the Resolver Service
 * @return Jxta_status
 */
static void stop(Jxta_module * resolver)
{
    Jxta_status rv = JXTA_SUCCESS;
    Jxta_resolver_service_ref *self = (Jxta_resolver_service_ref *) resolver;
    int i;

    rv = jxta_service_events_disconnect((Jxta_service *) self->endpoint, endpoint_event_handler, self);
    for (i = 0; i < 3; i++) {
        endpoint_service_remove_recipient(self->endpoint, self->ep_cookies[i]);
    }

    rv = jxta_endpoint_service_cleanup_pending_msgs(self->endpoint, (Jxta_service *)self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
    /* nothing special to stop */
}

/*
 * service methods
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Resolver Service
 * @return the advertisement.
 */
static void get_mia(Jxta_service * resolver, Jxta_advertisement ** mia)
{
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

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
    Jxta_resolver_service_ref* myself = PTValid(self, Jxta_resolver_service_ref);

    JXTA_OBJECT_SHARE(myself);
    *svc = self;
}


/*
 * Resolver methods proper.
 */

/**
 * Registers a given ResolveHandler.
 * The handler should expect a callback object of type ResolverQueryCallback.
 * The called method should set the fields of the callback object to direct the 
 * resolver in taking future actions
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status registerQueryHandler(Jxta_resolver_service * resolver, JString * name, Jxta_callback * handler)
{
    char const *hashname;
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    if (strlen(hashname) != 0) {
        jxta_hashtable_put(self->queryhandlers, hashname, strlen(hashname), (Jxta_object *) handler);
        return JXTA_SUCCESS;
    } else {
        return JXTA_INVALID_ARGUMENT;
    }
}

/**
  * unregisters a given ResolveHandler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status unregisterQueryHandler(Jxta_resolver_service * resolver, JString * name)
{
    Jxta_object *ignore = 0;
    char const *hashname;
    Jxta_status status;

    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    status = jxta_hashtable_del(self->queryhandlers, hashname, strlen(hashname), &ignore);
    if (ignore != 0)
        JXTA_OBJECT_RELEASE(ignore);
    return status;
}

/**
 * Registers a given ResolveHandler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status registerSrdiHandler(Jxta_resolver_service * resolver, JString * name, Jxta_listener * handler)
{
    char const *hashname;
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    if (strlen(hashname) != 0) {
        jxta_hashtable_put(self->srdihandlers, hashname, strlen(hashname), (Jxta_object *) handler);
        return JXTA_SUCCESS;
    } else {
        return JXTA_INVALID_ARGUMENT;
    }
}

/**
  * unregisters a given ResolveHandler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status unregisterSrdiHandler(Jxta_resolver_service * resolver, JString * name)
{
    Jxta_object *ignore = 0;
    char const *hashname;
    Jxta_status status;
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    status = jxta_hashtable_del(self->srdihandlers, hashname, strlen(hashname), &ignore);
    if (ignore != 0)
        JXTA_OBJECT_RELEASE(ignore);
    return status;
}

/**
 * Registers a given ResolveHandler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 */
Jxta_status registerResponseHandler(Jxta_resolver_service * resolver, JString * name, Jxta_listener * handler)
{
    char const *hashname;
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);
    hashname = jstring_get_string(name);

    if (strlen(hashname) != 0) {
        jxta_hashtable_put(self->responsehandlers, hashname, strlen(hashname), (Jxta_object *) handler);
        return JXTA_SUCCESS;
    } else {
        return JXTA_INVALID_ARGUMENT;
    }
}

/**
  * unregisters a given ResolveHandler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
static Jxta_status unregisterResponseHandler(Jxta_resolver_service * resolver, JString * name)
{
    Jxta_object *ignore = 0;
    char const *hashname;
    Jxta_status status;

    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    hashname = jstring_get_string(name);

    status = jxta_hashtable_del(self->responsehandlers, hashname, strlen(hashname), &ignore);
    if (ignore != 0)
        JXTA_OBJECT_RELEASE(ignore);
    return status;
}

Jxta_status return_resolver_func(Jxta_service *service, Jxta_endpoint_return_parms * ret_parms, Jxta_vector * filter_list, Jxta_vector **ret_v, Jxta_endpoint_service_action action)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *new_entries=NULL;
    Jxta_endpoint_message *ep_msg=NULL;
    Jxta_endpoint_return_parms *caller_parms;
    EndpointReturnFunc return_func;
    Jxta_vector *new_v=NULL;
    int i;
    Jxta_service *me;
    Jxta_resolver_service_ref *myself;
    ResolverResponse *res_rsp=NULL;
    Jxta_message *msg=NULL;

    me = jxta_endpoint_return_parms_service(ret_parms);

    myself = (Jxta_resolver_service_ref *) me;
    jxta_endpoint_return_parms_get_arg(ret_parms, JXTA_OBJECT_PPTR(&caller_parms));

    /* if the service didn't provide a callback just exit- There's nothing the resolver can do with it  */
    if (NULL == caller_parms) {
        goto FINAL_EXIT;
    }
    return_func = jxta_endpoint_return_parms_function(caller_parms);

    jxta_endpoint_return_parms_set_max_length(caller_parms, jxta_endpoint_return_parms_max_length(ret_parms));
    jxta_endpoint_return_parms_get_msg(ret_parms, &msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "parms [%pp] get msg [%pp]\n", ret_parms, msg);
    get_rr_from_message(myself, msg, &res_rsp);
    switch (action) {
        case JXTA_EP_ACTION_REDUCE:
        {
            Jxta_message *caller_msg;

            jxta_endpoint_return_parms_get_msg(caller_parms, &caller_msg);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Resolver return func JXTA_EP_ACTION_REDUCE caller msg [%pp] caller_parms [%pp]\n", caller_msg, caller_parms);
            JXTA_OBJECT_RELEASE(caller_msg);
            jxta_endpoint_return_parms_set_msg(caller_parms, (Jxta_message *) res_rsp);
            res = return_func(jxta_endpoint_return_parms_service(ret_parms), caller_parms, NULL, &new_v, JXTA_EP_ACTION_REDUCE);
            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to reduce res_rsp [%pp]\n", res_rsp);
                goto FINAL_EXIT;
            }

            *ret_v = jxta_vector_new(0);
            for (i=0; NULL != new_v && i<jxta_vector_size(new_v); i++) {
                Jxta_resolver_response *rr=NULL;
                Jxta_message *new_msg=NULL;
                JString *doc=NULL;
                Jxta_endpoint_return_parms *new_ret_parms=NULL;

                jxta_vector_get_object_at(new_v, JXTA_OBJECT_PPTR(&rr), i);
                res = jxta_resolver_response_get_xml(rr, &doc);
                JXTA_OBJECT_RELEASE(rr);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "New Resolver response [%pp] length:%d \n", rr, jstring_length(doc));
                if (res != JXTA_SUCCESS) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to parse the Resolver response\n");
                    continue;
                }
                if (JXTA_SUCCESS != resolver_build_msg(myself, doc, myself->inque, NULL, &new_msg)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to build a Resolver message\n");
                    JXTA_OBJECT_RELEASE(doc);
                    continue;
                }
                jxta_message_set_priority(new_msg, MSG_NORMAL_FLOW);

                /* TODO: If the caller parms are NULL don't provide a callback. */
                /* There's nothing the resolver will do  with a message */
                /* The endpoint needs to support this though */

                new_ret_parms = jxta_endpoint_return_parms_new();
                jxta_endpoint_return_parms_set_service(new_ret_parms, (Jxta_service *) me);
                jxta_endpoint_return_parms_set_function(new_ret_parms, (EndpointReturnFunc) return_resolver_func);
                jxta_endpoint_return_parms_set_msg(new_ret_parms, new_msg);
                jxta_endpoint_return_parms_set_arg(new_ret_parms, (Jxta_object *) caller_parms);

                res = jxta_vector_add_object_last(*ret_v, (Jxta_object *) new_ret_parms);
                JXTA_OBJECT_RELEASE(doc);
                JXTA_OBJECT_RELEASE(new_msg);
                JXTA_OBJECT_RELEASE(new_ret_parms);
                if (JXTA_SUCCESS != res) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to add the Resolver response\n");
                    continue;
                }
            }
            JXTA_OBJECT_RELEASE(new_v);
            break;
        }
        case JXTA_EP_ACTION_FILTER:
        {

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Resolver return func JXTA_EP_ACTION_FILTER\n");
            for (i=0; i<jxta_vector_size(filter_list); i++) {
                Jxta_endpoint_filter_entry *f_entry=NULL;
                Jxta_endpoint_filter_entry *new_entry=NULL;
                Jxta_endpoint_return_parms *f_parms=NULL;
                Jxta_message *f_msg=NULL;
                Jxta_boolean breakout=FALSE;

                jxta_vector_get_object_at(filter_list, JXTA_OBJECT_PPTR(&f_entry), i);
                jxta_endpoint_filter_entry_get_parms(f_entry, &f_parms);
                if (NULL == f_parms) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No parms in the filter entry - nothing to do\n");
                    breakout = TRUE;
                } else {
                    Jxta_endpoint_return_parms *tmp_parms=NULL;

                    tmp_parms = f_parms;
                    f_parms = NULL;
                    jxta_endpoint_return_parms_get_arg(tmp_parms, JXTA_OBJECT_PPTR(&f_parms));
                    if (f_parms != NULL) {
                        jxta_vector_remove_object_at(filter_list, NULL, i--);
                    }
                    else {
                        jxta_endpoint_filter_entry_get_orig_msg(f_entry, &f_msg);
                        new_entry = jxta_endpoint_filter_entry_new(f_msg);
                        jxta_endpoint_filter_entry_set_parms(new_entry, f_parms);
                        jxta_vector_replace_object_at(filter_list, (Jxta_object *) new_entry, i);
                        JXTA_OBJECT_RELEASE(new_entry);
                        JXTA_OBJECT_RELEASE(f_msg);
                    }
                   
                    JXTA_OBJECT_RELEASE(f_parms);
                    JXTA_OBJECT_RELEASE(tmp_parms);
                }
                JXTA_OBJECT_RELEASE(f_entry);
                if (breakout) goto FINAL_EXIT;
            }
            return_func(jxta_endpoint_return_parms_service(ret_parms), caller_parms, filter_list, &new_v, JXTA_EP_ACTION_FILTER);
            break;
        }
    };

FINAL_EXIT:
    if (res_rsp)
        JXTA_OBJECT_RELEASE(res_rsp);
    if (caller_parms)
        JXTA_OBJECT_RELEASE(caller_parms);
    if (ep_msg)
        JXTA_OBJECT_RELEASE(ep_msg);
    if (new_entries)
        JXTA_OBJECT_RELEASE(new_entries);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    return res;
}

static Jxta_status resolver_build_msg(Jxta_resolver_service_ref * me, JString *doc, const char *queue, const Jxta_qos * qos, Jxta_message **msg)
{
    Jxta_message_element *msgElem=NULL;
    char *tmp=NULL;
    JString *el_name=NULL;


    tmp = malloc(jstring_length(doc) + 1);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return JXTA_NOMEM;
    }
    tmp[jstring_length(doc)] = 0;
    memcpy(tmp, jstring_get_string(doc), jstring_length(doc));

    *msg = jxta_message_new();
    if (*msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        free(tmp);
        return JXTA_NOMEM;
    }
    if (qos) {
        jxta_message_attach_qos(*msg, qos);
    }

    el_name = jstring_new_2(JXTA_NAMESPACE);
    if (el_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(*msg);
        free(tmp);
        return JXTA_NOMEM;
    }
    jstring_append_2(el_name, queue);
    msgElem = jxta_message_element_new_1(jstring_get_string(el_name), "text/xml", tmp, strlen(tmp), NULL);
    JXTA_OBJECT_RELEASE(el_name);
    free(tmp);
    if (msgElem == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to create message element.\n");
        JXTA_OBJECT_RELEASE(*msg);
        return JXTA_INVALID_ARGUMENT;
    }
    jxta_message_add_element(*msg, msgElem);

    JXTA_OBJECT_RELEASE(msgElem);
    return JXTA_SUCCESS;
}

static Jxta_status do_send(Jxta_resolver_service_ref * me, Jxta_id * peerid, JString * doc, const char *queue, const Jxta_qos * qos,  Jxta_endpoint_return_parms * caller_parms)
{

    Jxta_status status = JXTA_SUCCESS;
    Jxta_message *msg=NULL;
    Jxta_endpoint_address *address;
    char *tmp;


    /* TODO: Needs to be different when the endpoint changes its technique */
    if (JXTA_SUCCESS != resolver_build_msg(me, doc, queue, qos, &msg)) {
        goto FINAL_EXIT;
    }

    if (NULL == peerid) {
        status = jxta_rdv_service_walk(me->rendezvous, msg, me->instanceName, queue);
        if (me->alwaysPropagate || JXTA_SUCCESS != status) {
            status = endpoint_service_propagate_by_group(me->group, msg, me->instanceName, queue);
            if (JXTA_SUCCESS != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service propagation status= %d\n", status);
                goto FINAL_EXIT;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Resolver message sent by propagation\n");
        }
        else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Resolver message sent by rendezvous walk\n");
        }

    } else {
        Jxta_endpoint_return_parms *ret_parms=NULL;

        /* TODO: If the caller parms are NULL don't provide a callback. */
        /* There's nothing the resolver will do  with a message */
        /* The endpoint needs to support this though */

        ret_parms = jxta_endpoint_return_parms_new();
        jxta_endpoint_return_parms_set_service(ret_parms, (Jxta_service *) me);
        jxta_endpoint_return_parms_set_function(ret_parms, (EndpointReturnFunc) return_resolver_func);
        jxta_endpoint_return_parms_set_msg(ret_parms, msg);
        jxta_endpoint_return_parms_set_arg(ret_parms, (Jxta_object *) caller_parms);
        address = jxta_endpoint_address_new_3(peerid, me->instanceName, queue);
        if (address == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }

        if (NULL != ret_parms) {
            jxta_message_set_priority(msg, MSG_NORMAL_FLOW);
        }

        status = jxta_endpoint_service_send_async(me->group, me->endpoint, msg, address, ret_parms);

        if (status != JXTA_SUCCESS) {
            JString *peerid_j = NULL;

            jxta_id_to_jstring(peerid, &peerid_j);
            if (JXTA_BUSY != status && JXTA_LENGTH_EXCEEDED != status) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING
                            , "Failed to send resolver message to %s status:%d\n"
                            , jstring_get_string(peerid_j), status);
            }
            JXTA_OBJECT_RELEASE(peerid_j);
        } else {
            tmp = jxta_endpoint_address_to_string(address);
            if (tmp) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Resolver message sent to %s res:%d\n", tmp, status);
                free(tmp);
            }
        }
        JXTA_OBJECT_RELEASE(address);
        JXTA_OBJECT_RELEASE(ret_parms);
    }

  FINAL_EXIT:
    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return status;
}

static void mru_reset(Jxta_resolver_service_ref * me)
{
    size_t i;

    if (!me->mru) {
        return;
    }

    apr_thread_mutex_lock(me->mutex);
    for (i = 0; i < me->mru_size; i++) {
        JXTA_OBJECT_RELEASE(me->mru[i]);
    }
    me->mru_size = me->mru_pos = 0;
    apr_thread_mutex_unlock(me->mutex);
}

static Jxta_status mru_capacity_set(Jxta_resolver_service_ref * me, size_t capacity)
{
    Jxta_PID **new_mru = NULL;
    size_t pos;
    size_t i;
    size_t cnt;
    Jxta_status rv = JXTA_SUCCESS;

    apr_thread_mutex_lock(me->mutex);

    if (capacity == me->mru_capacity) {
        goto FINAL_EXIT;
    }

    /* mru_capacity could be non-0 while mru is NULL when system is out of memory */
    if (0 == capacity) {
        if (me->mru) {
            mru_reset(me);
            free(me->mru);
            me->mru = NULL;
            me->mru_size = me->mru_pos = 0;
        }
        goto FINAL_EXIT;
    }

    new_mru = calloc(capacity, sizeof(*new_mru));
    if (!new_mru) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to allocate memory for MRU, keep original MRU\n");
        rv = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    if (!me->mru) {
        assert(0 == me->mru_size);
        assert(0 == me->mru_pos);
        me->mru_capacity = capacity;
        me->mru = new_mru;
        goto FINAL_EXIT;
    }

    pos = (me->mru_size < me->mru_capacity) ? 0 : me->mru_pos;
    cnt = (me->mru_size < capacity) ? me->mru_size : capacity;
    /* remove extra entries over the new capacity */
    if (capacity < me->mru_size) {
        for (i = capacity; i < me->mru_size; ++i) {
            JXTA_OBJECT_RELEASE(me->mru[pos++]);
            if (pos >= me->mru_capacity) {
                pos = 0;
            }
        }
    }

    for (i = 0; i < cnt; i++) {
        new_mru[i] = me->mru[pos++];
        if (pos >= me->mru_capacity) {
            pos = 0;
        }
    }

    free(me->mru);
    me->mru = new_mru;
    me->mru_pos = (i == capacity) ? 0 : i;
    me->mru_size = i;
    me->mru_capacity = capacity;

  FINAL_EXIT:
    apr_thread_mutex_unlock(me->mutex);
    return rv;
}

/*
 * check if the peerid is in MRU cache, if it is not, add SrcPeerRoute tag to include RA
 * if the query already has a SrcPeerRoute tag, this is a forwarded query, don't modify the tag
 */
static void mru_check(Jxta_resolver_service_ref * me, ResolverQuery * query, Jxta_id * peerid)
{
    size_t i;
    Jxta_RouteAdvertisement *route;

    route = jxta_resolver_query_get_src_peer_route(query);
    if (route) {
        JXTA_OBJECT_RELEASE(route);
        return;
    }

    if (me->mru) {
        apr_thread_mutex_lock(me->mutex);
        for (i = 0; i < me->mru_size; i++) {
            if (jxta_id_equals(me->mru[i], peerid)) {
                apr_thread_mutex_unlock(me->mutex);
                return;
            }
        }

        JXTA_OBJECT_RELEASE(me->mru[me->mru_pos]);
        me->mru[me->mru_pos++] = JXTA_OBJECT_SHARE(peerid);
        if (me->mru_pos >= me->mru_capacity) {
            me->mru_pos = 0;
        } 
        
        if (me->mru_size < me->mru_capacity) {
            me->mru_size++;
        }

        apr_thread_mutex_unlock(me->mutex);
    }

    route = jxta_endpoint_service_get_local_route(me->endpoint);
    jxta_resolver_query_set_src_peer_route(query, route);
    JXTA_OBJECT_RELEASE(route);
}

static Jxta_status sendQuery(Jxta_resolver_service * resolver, ResolverQuery * query, Jxta_id * peerid)
{
    JString *doc;
    Jxta_status status;
    const Jxta_qos * qos;
    Jxta_resolver_service_ref *myself = PTValid(resolver, Jxta_resolver_service_ref);

    /* Test arguments first */
    if ((myself == NULL) || (query == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    if (jxta_resolver_query_get_queryid(query) == JXTA_INVALID_QUERY_ID) {
      /** That's a new query, set a new locally unique query id */
        jxta_resolver_query_set_queryid(query, getid(myself));
    }

    mru_check(myself, query, peerid);

    status = jxta_resolver_query_get_xml(query, &doc);
    if (status != JXTA_SUCCESS) {
        return status;
    }

    qos = jxta_resolver_query_qos(query);
    if (!qos) {
        status = jxta_service_default_qos((Jxta_service*) myself, peerid ? peerid : jxta_id_nullID, &qos);
    }
    if (JXTA_ITEM_NOTFOUND == status) {
        jxta_service_default_qos((Jxta_service*) myself, NULL, &qos);
    }

    status = do_send(myself, peerid, doc, myself->outque, qos, NULL);
    JXTA_OBJECT_RELEASE(doc);
    return status;
}

/* XXX fixme hamada@jxta.org breakout the sending part out into static function both
   send query, and response will use 
 */

static Jxta_status sendResponse(Jxta_resolver_service * resolver, ResolverResponse * response, Jxta_id * peerid,  Jxta_endpoint_return_parms * return_parms)
{
    JString *doc;
    Jxta_status status;
    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);

    /* Test arguments first */
    if ((resolver == NULL) || (response == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    status = jxta_resolver_response_get_xml(response, &doc);
    if (status != JXTA_SUCCESS) {
        return status;
    }

    status = do_send(self, peerid, doc, self->inque, jxta_resolver_response_qos(response), return_parms);
    JXTA_OBJECT_RELEASE(doc);
    return status;
}

static Jxta_status sendSrdi(Jxta_resolver_service * resolver, ResolverSrdi * message, Jxta_id * peerid, Jxta_boolean sync,  Jxta_endpoint_return_parms * return_parms)
{
    Jxta_message *msg = NULL;
    Jxta_message_element *msgElem = NULL;
    Jxta_endpoint_address *address = NULL;
    unsigned char *tmp = NULL;
    unsigned char *bytes = NULL;

    JString *doc = NULL;
    Jxta_bytevector *jSend_buf = NULL;
    Jxta_status status;
    JString *jpeerid = NULL;
    JString *el_name = NULL;

    Jxta_resolver_service_ref *self = PTValid(resolver, Jxta_resolver_service_ref);
    if (NULL != peerid) {
        jxta_id_to_jstring(peerid, &jpeerid);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send SRDI resolver message to %s\n",
                         jpeerid == NULL? "all(through propagate)" : jstring_get_string(jpeerid));
    if (jpeerid) {
        JXTA_OBJECT_RELEASE(jpeerid);
    }
    
    /* Test arguments first */
    if ((self == NULL) || (message == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    status = jxta_resolver_srdi_get_xml(message, &doc);
    if (status != JXTA_SUCCESS) {
        return status;
    }

    tmp = (unsigned char *) calloc(1, jstring_length(doc) + 1);
    if (NULL == tmp) {
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    tmp[jstring_length(doc)] = 0;
    memcpy(tmp, jstring_get_string(doc), jstring_length(doc));

    msg = jxta_message_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    el_name = jstring_new_2(JXTA_NAMESPACE);
    if (el_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        status = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    jstring_append_2(el_name, self->srdique);

#ifndef GZIP_ENABLED
    msgElem = jxta_message_element_new_1(jstring_get_string(el_name), "text/xml", (char *) tmp, strlen((char *) tmp), NULL);
#else
    status = jxta_xml_util_compress(tmp, el_name, &msgElem);
#endif
    if (msgElem == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to create message element.\n");
        status = JXTA_FAILED;
        goto FINAL_EXIT;
    }
    jxta_message_add_element(msg, msgElem);

    if (peerid == NULL) {
        jxta_rdv_service_walk(self->rendezvous, msg, self->instanceName, self->srdique);
    } else {
        address = jxta_endpoint_address_new_3(peerid, self->instanceName, self->srdique);
        if (address == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            status = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        if (!sync) {
            status = jxta_endpoint_service_send(self->group, self->endpoint, msg, address, return_parms);
        } else {
            status = jxta_endpoint_service_send_async(self->group, self->endpoint, msg, address, return_parms);
        }
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send srdi message\n");
        }
    }

  FINAL_EXIT:
    if (jSend_buf)
        JXTA_OBJECT_RELEASE(jSend_buf);
    if (el_name)
        JXTA_OBJECT_RELEASE(el_name);
    if (msgElem)
        JXTA_OBJECT_RELEASE(msgElem);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);
    if (doc)
        JXTA_OBJECT_RELEASE(doc);
    if (address)
        JXTA_OBJECT_RELEASE(address);
    if (tmp)
        free(tmp);
    if (bytes)
        free(bytes);
    return status;
}

Jxta_status create_query(Jxta_resolver_service * me, JString * handlername, JString * query, Jxta_resolver_query ** rq)
{
    Jxta_status rv;
    Jxta_resolver_service_ref *myself = (Jxta_resolver_service_ref *) me;

    rv = resolver_query_create(handlername, query, myself->localPeerId, rq);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    jxta_resolver_query_set_queryid(*rq, getid(myself));
    return JXTA_SUCCESS;
}

/**
 * Peergroups that would like the resolver to both walk and propagate a query can use this
 * interface to override the Resolver Service Config provided in the PlatformConfig
 *
 * @param pointer to the Jxta_resolver_service
 * @param propagate flag indicating whether to propagate or not.  TRUE will send both a walk and propagate message.
 */
void setAlwaysPropagate(Jxta_resolver_service * service, Jxta_boolean propagate)
{
    Jxta_resolver_service_ref *myself = (Jxta_resolver_service_ref *) service;
    myself->alwaysPropagate = propagate;
}

/**
 * Returns the always propagate setting for the current group
 *
 * @param service a pointer to the instance of the resolver service
 * @return the boolean indicating the propagation behavior
 */
Jxta_boolean alwaysPropagate(Jxta_resolver_service * service)
{
    Jxta_resolver_service_ref *myself = (Jxta_resolver_service_ref *) service;
    return myself->alwaysPropagate;
}



/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_resolver_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_resolver_service_methods -> Jxta_resolver_service_ref_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_resolver_service_methods Jxta_resolver_service_ref_methods;

Jxta_resolver_service_ref_methods jxta_resolver_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_resolver_service_ref_init,   /* temp long name, for testing */
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface,
     service_on_option_set},
    "Jxta_resolver_service_methods",
    registerQueryHandler,
    unregisterQueryHandler,
    registerSrdiHandler,
    unregisterSrdiHandler,
    registerResponseHandler,
    unregisterResponseHandler,
    sendQuery,
    sendResponse,
    sendSrdi,
    create_query,
    setAlwaysPropagate,
    alwaysPropagate
};

void jxta_resolver_service_ref_construct(Jxta_resolver_service_ref * self, Jxta_resolver_service_ref_methods * methods)
{
    /*
     * we do not extend Jxta_resolver_service_methods; so the type string
     * is that of the base table
     */
    Jxta_resolver_service_methods* service_methods = PTValid(methods, Jxta_resolver_service_methods);

    jxta_resolver_service_construct((Jxta_resolver_service *) self, (Jxta_resolver_service_methods *) service_methods);

    /* Set our rt type checking string */
    self->thisType = "Jxta_resolver_service_ref";
}

void jxta_resolver_service_ref_destruct(Jxta_resolver_service_ref * self)
{
    Jxta_resolver_service_ref* myself = PTValid(self, Jxta_resolver_service_ref);

    /* release/free/destroy our own stuff */
    mru_capacity_set(self, 0);
    if (myself->rendezvous != 0) {
        JXTA_OBJECT_RELEASE(myself->rendezvous);
    }
    if (myself->endpoint != 0) {
        JXTA_OBJECT_RELEASE(myself->endpoint);
    }
    if (myself->localPeerId != 0) {
        JXTA_OBJECT_RELEASE(myself->localPeerId);
    }
    if (myself->queryhandlers != 0) {
        JXTA_OBJECT_RELEASE(myself->queryhandlers);
    }
    if (myself->responsehandlers != 0) {
        JXTA_OBJECT_RELEASE(myself->responsehandlers);
    }
    if (myself->srdihandlers != 0) {
        JXTA_OBJECT_RELEASE(myself->srdihandlers);
    }

    myself->group = NULL;

    if (myself->impl_adv != 0) {
        JXTA_OBJECT_RELEASE(myself->impl_adv);
    }

    apr_thread_mutex_destroy(myself->mutex);
    /* call the base classe's dtor. */
    jxta_resolver_service_destruct((Jxta_resolver_service *) myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/**
 * Frees an instance of the Resolver Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Resolver Service to free.
 */
static void resol_free(void *service)
{
    /* call the hierarchy of dtors */
    jxta_resolver_service_ref_destruct((Jxta_resolver_service_ref *) service);

    /* free the object itself */
    free(service);
}

/**
 * Creates a new instance of the Resolver Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_resolver_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Resolver Service.
 * @return a non initialized instance of the Resolver Service
 */

Jxta_resolver_service_ref *jxta_resolver_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_resolver_service_ref *self = (Jxta_resolver_service_ref *) malloc(sizeof(Jxta_resolver_service_ref));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(Jxta_resolver_service_ref));
    JXTA_OBJECT_INIT(self, (JXTA_OBJECT_FREE_FUNC) resol_free, 0);

    /* call the hierarchy of ctors */
    jxta_resolver_service_ref_construct(self, &jxta_resolver_service_ref_methods);

    /* return the new object */
    return self;
}

/* END OF STANDARD SERVICE IMPLEMENTATION CODE */

static void populate_query_source_route(Jxta_resolver_service_ref * me, ResolverQuery * rq)
{
    Jxta_id * src_pid;
    Jxta_status rv;
    Jxta_RouteAdvertisement *route = NULL;

    src_pid = jxta_resolver_query_get_src_peer_id(rq);
    rv = peergroup_find_peer_RA(me->group, src_pid, 0, &route);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, 
                "Cannot find the route advertisement, forward resolver query without a route advertisement.");
    }
    jxta_resolver_query_set_src_peer_route(rq, route);
    JXTA_OBJECT_RELEASE(src_pid);
    JXTA_OBJECT_RELEASE(route);
}

static Jxta_status learn_route_from_query(Jxta_resolver_service_ref * me, ResolverQuery * rq)
{
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_AccessPointAdvertisement *dest = NULL;
    Jxta_discovery_service *discovery = NULL;
    Jxta_PG *pg = NULL;
    Jxta_PG *npg = NULL;
    Jxta_status rc;

    route = jxta_resolver_query_src_peer_route(rq);
    if (NULL == route) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot extract route to source peer for resolver query[%pp]\n", rq);
        return JXTA_ITEM_NOTFOUND;
    }

    dest = jxta_RouteAdvertisement_get_Dest(route);
    if (NULL == dest) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot extract APA for RA destination from resolver query[%pp]\n", rq);
        return JXTA_INVALID_ARGUMENT;
    }
    JXTA_OBJECT_RELEASE(dest);

    npg = JXTA_OBJECT_SHARE(me->group);
    jxta_PG_get_parentgroup(npg, &pg);
    while (NULL != pg) {
        JXTA_OBJECT_RELEASE(npg);
        npg = pg;
        jxta_PG_get_parentgroup(npg, &pg);
    }

    jxta_PG_get_discovery_service(npg, &discovery);
    JXTA_OBJECT_RELEASE(npg);
    if (NULL == discovery) {
        JXTA_OBJECT_RELEASE(dest);
        return JXTA_FAILED;
    }

    rc = discovery_service_publish(discovery, (Jxta_advertisement *) route, DISC_ADV, ROUTEADV_DEFAULT_LIFETIME,
                                   LOCAL_ONLY_EXPIRATION);
    JXTA_OBJECT_RELEASE(discovery);
    if (JXTA_SUCCESS != rc) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Failed to publish route to source peer for resolver query[%pp], error code %d\n", rq, rc);
    }
    return rc;
}

static Jxta_status replace_rq_element(Jxta_resolver_service_ref *me, Jxta_message * msg, ResolverQuery *rq)
{
    Jxta_message_element *el = NULL;
    JString *doc = NULL;
    Jxta_status rv;

    rv = jxta_resolver_query_get_xml(rq, &doc); 
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    el = jxta_message_element_new_2("jxta", me->outque, "text/xml", jstring_get_string(doc), jstring_length(doc), NULL);
    if (NULL == el) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to create message element.\n");
        JXTA_OBJECT_RELEASE(doc);
        return JXTA_FAILED;
    }
    JXTA_OBJECT_RELEASE(doc);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Remove element resolver [%pp] \n", me->outque);

    jxta_message_remove_element_2(msg, "jxta", me->outque);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    return JXTA_SUCCESS;
}

/*
 * When receiving a resolver query, resolver should call the appropriate handler.
 * Handler will return JXTA_SUCCESS to stop the query, otherwise, the query will be walked.
 *
 * This function also try to publish the router advertisement embedded in the query to reduce the need to resolve the router for
 * returning the response if the query is satisfied locally.
 */
static Jxta_status JXTA_STDCALL resolver_service_query_cb(Jxta_object * obj, void *me)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *element = NULL;
    Jxta_object *handler = NULL;
    ResolverQuery *rq = NULL;
    JString *handlername = NULL;
#ifdef UNUSED_VWF
    Jxta_boolean drop_query = FALSE;
#endif
    Jxta_status status;
    Jxta_resolver_service_ref *_self = (Jxta_resolver_service_ref *) me;
    JString *pid = NULL;
    JString *el_name;
    Jxta_id *src_pid = NULL;
    Jxta_bytevector *bytes = NULL;
    JString *string = NULL;
    int rq_missing_ra = 0;
    ResolverQueryCallback * rqc = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Resolver Query Listener, query received\n");

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(_self);

    el_name = jstring_new_2(JXTA_NAMESPACE);
    if (el_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return JXTA_NOMEM;
    }
    jstring_append_2(el_name, _self->outque);
    status = jxta_message_get_element_1(msg, (char *)
                                        jstring_get_string(el_name), &element);

    if (!element) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Resolver Query not for me %s\n", jstring_get_string(el_name));
        JXTA_OBJECT_RELEASE(el_name);
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_RELEASE(el_name);

    rq = jxta_resolver_query_new();
    if (rq == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(element);
        return JXTA_NOMEM;
    }
    bytes = jxta_message_element_get_value(element);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    if (string == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(rq);
        JXTA_OBJECT_RELEASE(element);
        return JXTA_NOMEM;
    }
    bytes = NULL;
    jxta_resolver_query_parse_charbuffer(rq, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(element);
    jxta_resolver_query_attach_qos(rq, jxta_message_qos(msg));

    src_pid = jxta_resolver_query_get_src_peer_id(rq);
    jxta_id_to_jstring(src_pid, &pid);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Resolver query from peer %s hc:%ld\n", jstring_get_string(pid), jxta_resolver_query_get_hopcount(rq));
    JXTA_OBJECT_RELEASE(string);
    string = NULL;
    JXTA_OBJECT_RELEASE(pid);
    if (jxta_id_equals(src_pid, _self->localPeerId)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Ignore query that originated at this peer\n");
        /* give it a chance to continue walk if this peer became a RDV */
        status = JXTA_ITEM_NOTFOUND;
    } else {
        /* ensure we have a route info as part of the query to respond */
        rq_missing_ra = (JXTA_ITEM_NOTFOUND == learn_route_from_query(_self, rq));
        if (rq_missing_ra) {
            populate_query_source_route(_self, rq);
        }

        jxta_resolver_query_get_handlername(rq, &handlername);

        if (handlername != NULL) {
            status = jxta_hashtable_get(_self->queryhandlers, jstring_get_string(handlername), jstring_length(handlername), 
                                        &handler);
            /* call the query handler with the query */
            if (handler != NULL) {
                rqc = jxta_resolver_query_callback_new_1(rq, FALSE, TRUE);
                status = jxta_callback_process_object((Jxta_callback *) handler, (Jxta_object *) rqc);
                JXTA_OBJECT_RELEASE(handler);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Failed to get handler from hashtable\n");
                status = JXTA_FAILED;
            }
            JXTA_OBJECT_RELEASE(handlername);
        } else {
            status = JXTA_ITEM_NOTFOUND;
        }
    }

    if (JXTA_SUCCESS != status) {
        if (!jxta_rdv_service_is_rendezvous(_self->rendezvous)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot solve query at edge, done deal\n");
            status = JXTA_SUCCESS;
        } else {
            /* this will unconditionally replace the resolver query. */
            /* This was added for the discovery state changes because the discovery query changes. */
            replace_rq_element(_self, msg, rq);
            if (NULL != rqc && TRUE == jxta_resolver_query_callback_walk(rqc)) {
                if (TRUE == jxta_resolver_query_callback_propagate(rqc)) {
                    status = jxta_rdv_service_walk(_self->rendezvous, msg, _self->instanceName, _self->outque);
                } else {
                    status = jxta_rdv_service_walk_no_propagate(_self->rendezvous, msg, _self->instanceName, _self->outque);
                }
            }
            else {
                status = jxta_rdv_service_walk(_self->rendezvous, msg, _self->instanceName, _self->outque);
            }
        }
    }
    JXTA_OBJECT_RELEASE(src_pid);
    JXTA_OBJECT_RELEASE(rq);
    if (NULL != rqc)
        JXTA_OBJECT_RELEASE(rqc);
    return status;
}

static Jxta_status get_rr_from_message(Jxta_resolver_service_ref *resolver, Jxta_message *msg, ResolverResponse **rr)
{
    Jxta_status status;
    Jxta_message_element *element = NULL;

    status = jxta_message_get_element_2(msg, "jxta", resolver->inque, &element);
    if (element && status == JXTA_SUCCESS) {
        Jxta_bytevector *bytes = NULL;
        JString *string = NULL;

        *rr = jxta_resolver_response_new();
        if (*rr == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            JXTA_OBJECT_RELEASE(element);
            return JXTA_NOMEM;
        }
        bytes = jxta_message_element_get_value(element);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        if (string == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            JXTA_OBJECT_RELEASE(*rr);
            JXTA_OBJECT_RELEASE(element);
            return JXTA_NOMEM;
        }
        bytes = NULL;
        status = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) *rr, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);
        string = NULL;
        JXTA_OBJECT_RELEASE(element);
        if(status != JXTA_SUCCESS) {
            *rr = NULL;
            return status;
        }

        jxta_resolver_response_attach_qos(*rr, jxta_message_qos(msg));
    }
    return status;
}

static Jxta_status JXTA_STDCALL resolver_service_response_cb(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_object *handler = NULL;
    JString *handlername = NULL;
    ResolverResponse *rr = NULL;
    Jxta_status status = JXTA_SUCCESS;

    Jxta_resolver_service_ref *resolver = (Jxta_resolver_service_ref *) arg;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Resolver Response Listener, response received\n");

    if (JXTA_SUCCESS != get_rr_from_message(resolver, msg, &rr)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve rr from [%pp]\n", msg);
        return JXTA_FAILED;
    }

    jxta_resolver_response_get_handlername(rr, &handlername);

    if (handlername != NULL) {
        status = jxta_hashtable_get(resolver->responsehandlers,
                                    jstring_get_string(handlername), jstring_length(handlername), &handler);

        if (handler != NULL) {
            status = jxta_listener_process_object((Jxta_listener *) handler, (Jxta_object *) rr);
            JXTA_OBJECT_RELEASE(handler);
        }
        JXTA_OBJECT_RELEASE(handlername);
        handlername = NULL;
    }
    if (rr)
        JXTA_OBJECT_RELEASE(rr);
    return status;
}

static Jxta_status JXTA_STDCALL resolver_service_srdi_cb(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *element = NULL;
    Jxta_object *handler = NULL;
    JString *handlername = NULL;
    ResolverSrdi *srdi_msg = NULL;
    Jxta_status status;
    JString *el_name;
    Jxta_resolver_service_ref *resolver = (Jxta_resolver_service_ref *) arg;

    /* jxta_message_print(msg); */
    JXTA_OBJECT_CHECK_VALID(msg);

    el_name = jstring_new_2(JXTA_NAMESPACE);
    if (el_name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return JXTA_NOMEM;
    }
    jstring_append_2(el_name, resolver->srdique);
    status = jxta_message_get_element_1(msg, (char *)
                                        jstring_get_string(el_name), &element);
    JXTA_OBJECT_RELEASE(el_name);
    if (element) {
        const char *mime_type = jxta_message_element_get_mime_type(element);
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
                return JXTA_NOMEM;
            }
            jxta_bytevector_get_bytes_at(jb, bytes, 0, size);
            JXTA_OBJECT_RELEASE(jb);
            if (!strcmp(mime_type, "application/gzip")) {
#ifdef GUNZIP_ENABLED
                status = jxta_xml_util_uncompress(bytes, size, &uncompr, &uncomprLen);
                free(bytes);
#else
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received GZip mime type but GZip is disabled \n");
#endif
            } else {
                uncompr = bytes;
                uncomprLen = size;
            }
            if (NULL != uncompr) {
                srdi_msg = jxta_resolver_srdi_new();
                if (srdi_msg == NULL) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                    JXTA_OBJECT_RELEASE(element);
                    return JXTA_NOMEM;
                }
                jxta_resolver_srdi_parse_charbuffer(srdi_msg, (const char *) uncompr, uncomprLen);
                jxta_resolver_srdi_get_handlername(srdi_msg, &handlername);
                if (handlername != NULL) {
                    status = jxta_hashtable_get(resolver->srdihandlers,
                                                jstring_get_string(handlername), jstring_length(handlername), &handler);
                    JXTA_OBJECT_RELEASE(handlername);
                }
                /* call the handler with the payload */
                if (handler != NULL) {
                    JString *payload = NULL;
                    jxta_resolver_srdi_get_payload(srdi_msg, &payload);
                    if (payload != NULL) {
                        status = jxta_listener_process_object((Jxta_listener *) handler, (Jxta_object *) payload);
                        JXTA_OBJECT_RELEASE(payload);
                    }
                    JXTA_OBJECT_RELEASE(handler);
                }
                JXTA_OBJECT_RELEASE(srdi_msg);
                free(uncompr);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "got a message with a null mime type \n");
            status = JXTA_INVALID_ARGUMENT;
        }
        JXTA_OBJECT_RELEASE(element);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Didn't get the resolver srdi msg expected \n");
        status = JXTA_INVALID_ARGUMENT;
    }
    return status;
}

/**
 * generate a unique id within this instance of this service on this peer
 *
 */
static long getid(Jxta_resolver_service_ref * resolver)
{
    long qid = 0;
    Jxta_resolver_service_ref* res = PTValid(resolver, Jxta_resolver_service_ref);
    apr_thread_mutex_lock(res->mutex);
    qid = ++res->query_id;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "queryid: %ld\n", res->query_id);
    apr_thread_mutex_unlock(res->mutex);
    return qid;
}

/* vim: set ts=4 sw=4 et tw=130: */
