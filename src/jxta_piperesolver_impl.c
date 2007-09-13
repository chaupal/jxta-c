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

#include <limits.h>
#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_cm.h"
#include "jxta_cm_private.h"
#include "jxta_debug.h"
#include "jxta_id.h"
#include "jxta_peergroup_private.h"
#include "jxta_pipe_service.h"
#include "jxta_pipe_service_impl.h"
#include "jxta_hashtable.h"
#include "jxta_vector.h"
#include "jxta_resolver_service.h"
#include "jxta_rq.h"
#include "jxta_rr.h"
#include "jxta_srdi.h"
#include "jxta_srdi_service.h"
#include "jxta_message.h"
#include "jxta_piperesolver_msg.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_routea.h"
#include "jxta_log.h"
#include "jxta_discovery_service_private.h"
#include "jxta_rdv_service_provider_private.h"

static const char *__log_cat = "PIPE_RESOLVER";

typedef struct {
    Jxta_pipe_resolver generic;
    Jxta_PG *group;
    Jxta_id *local_peerid;
    Jxta_resolver_service *resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_pipe_service *pipe_service;
    Jxta_rdv_service *rdv_service;
    Jxta_srdi_service *srdi_service;
    Jxta_cm *cm;
    Jxta_peerview *rpv;
    Jxta_hashtable *pending_requests;
    Jxta_callback *query_callback;
    Jxta_listener *response_listener;
    Jxta_listener *srdi_listener;
    JString *name;

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
} Pipe_resolver;

typedef struct {
    JXTA_OBJECT_HANDLE;

    Jxta_pipe_adv *adv;
    Jxta_time timeout;
    Jxta_peer *dest;
    Jxta_listener *listener;
} Pending_request;


struct _jxta_pipe_resolver_event {
    JXTA_OBJECT_HANDLE;
    int event;
    Jxta_pipe_adv *adv;
    Jxta_vector *peers;
};

#define PIPE_RESOLVER_NAME "JxtaPipeResolver"
#define QUERY_MSG_TYPE     "Query"
#define RESPONSE_MSG_TYPE  "Answer"
#define OPEN_PIPE JPR_INTERVAL_TIME_MAX
#define CLOSE_PIPE 0

static Jxta_status pending_request_add(Pipe_resolver *, Pending_request *);
static Jxta_status pending_request_remove(Pipe_resolver *, Pending_request *);
static Pending_request *pending_request_new(Jxta_pipe_adv *, Jxta_time_diff, Jxta_peer *, Jxta_listener *);

static Jxta_status JXTA_STDCALL query_callback_func(Jxta_object * obj, void *arg);
static void JXTA_STDCALL response_listener(Jxta_object * obj, void *arg);
static void JXTA_STDCALL srdi_listener(Jxta_object * obj, void *arg);
static Jxta_status send_query(Pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_peer * peer);
static Jxta_status send_srdi(Jxta_pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_id * peerid, Jxta_boolean adding);

static Jxta_status jxta_pipe_resolver_walk(Pipe_resolver * me, ResolverQuery * rq, char *expression);
static Jxta_status jxta_pipe_resolver_check_srdi(Pipe_resolver * me, ResolverQuery * rq, Jxta_pipe_adv * adv);
static Jxta_status jxta_pipe_resolver_get_replica_peer(Pipe_resolver * me, ResolverQuery * rq, char *expression);

static Jxta_status local_resolve(Jxta_pipe_resolver * obj, Jxta_pipe_adv * adv, Jxta_vector ** peers)
{
    Pipe_resolver *self = (Pipe_resolver *) obj;
    Jxta_pipe_service_impl * pipe_service_impl;
    
    pipe_service_impl = get_impl_by_adv(self->pipe_service, adv);

    if (pipe_service_impl != NULL)
        return jxta_pipe_service_impl_check_listener(pipe_service_impl, jxta_pipe_adv_get_Id(adv));
    else
        return JXTA_FAILED;
}

static Jxta_status remote_resolve(Jxta_pipe_resolver * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_listener * listener)
{
    Pipe_resolver *self = (Pipe_resolver *) obj;
    Pending_request *req = NULL;
    Jxta_status res = JXTA_SUCCESS;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Resolver resolves pipe %s\n", jxta_pipe_adv_get_Id(adv));
    req = pending_request_new(adv, timeout, dest, listener);

    if (req == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a pending request\n");
        return JXTA_NOMEM;
    }

    res = pending_request_add((Pipe_resolver *) self, req);

    if (res == JXTA_SUCCESS) {
        res = send_query(self, adv, dest);
    }
    JXTA_OBJECT_RELEASE(req);
    JXTA_OBJECT_CHECK_VALID(req);

    return res;
}

static Jxta_status timed_remote_resolve(Jxta_pipe_resolver * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_vector ** peers)
{
    Pipe_resolver *self = (Pipe_resolver *) obj;
    Jxta_status res;
    Jxta_listener *listener = NULL;
    Jxta_pipe_resolver_event *event = NULL;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(adv);

    listener = jxta_listener_new(NULL, NULL, 1, 1);

    if (listener == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a listener\n");
        return JXTA_NOMEM;
    }

    jxta_listener_start(listener);

    res = remote_resolve(obj, adv, timeout, dest, listener);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "timed_remote_resolve: failed on timeout\n");

        JXTA_OBJECT_RELEASE(listener);
        return res;
    }

    res = jxta_listener_wait_for_event(listener, timeout, JXTA_OBJECT_PPTR(&event));

    if (jxta_pipe_resolver_event_get_event(event) != JXTA_PIPE_RESOLVER_RESOLVED) {

        JXTA_OBJECT_RELEASE(listener);
        JXTA_OBJECT_RELEASE(event);

        return JXTA_TIMEOUT;
    }

    res = jxta_pipe_resolver_event_get_peers(event, peers);

    JXTA_OBJECT_RELEASE(listener);
    JXTA_OBJECT_RELEASE(event);

    return res;
}

static Jxta_status send_srdi(Jxta_pipe_resolver * me, Jxta_pipe_adv * adv, Jxta_id * peerid, Jxta_boolean adding)
{
    Pipe_resolver *me2 = (Pipe_resolver *) me;
    Jxta_status res = JXTA_SUCCESS;
    ResolverSrdi *srdi = NULL;
    JString *messageString = NULL;
    JString *pkey = NULL;
    JString *skey = NULL;
    JString *value = NULL;
    JString *nSpace = NULL;
    Jxta_SRDIMessage *msg = NULL;
    Jxta_SRDIEntryElement *entry;
    Jxta_vector *entries;

    nSpace = jstring_new_2(jxta_advertisement_get_document_name((Jxta_advertisement *) adv));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send pipe SRDI message\n");

    skey = jstring_new_2("Id");
    value = jstring_new_2(jxta_pipe_adv_get_Id(adv));
    if (adding) {
        entry = jxta_srdi_new_element_1(skey, value, nSpace, (Jxta_expiration_time) OPEN_PIPE);
    } else {
        entry = jxta_srdi_new_element_1(skey, value, nSpace, (Jxta_expiration_time) CLOSE_PIPE);
    }
    JXTA_OBJECT_RELEASE(nSpace);
    JXTA_OBJECT_RELEASE(skey);
    JXTA_OBJECT_RELEASE(value);
    if (entry == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate memory for an srdi entry\n");
        return JXTA_NOMEM;
    }

    entries = jxta_vector_new(1);
    if (entries == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate memory for the entries vector\n");
        JXTA_OBJECT_RELEASE(entry);
        return JXTA_NOMEM;
    }
    jxta_vector_add_object_last(entries, (Jxta_object *) entry);
    JXTA_OBJECT_RELEASE(entry);

    pkey = jstring_new_2(jxta_pipe_adv_get_Type(adv));
    msg = jxta_srdi_message_new_1(1, me2->local_peerid, (char *) jstring_get_string(pkey), entries);
    JXTA_OBJECT_RELEASE(pkey);
    JXTA_OBJECT_RELEASE(entries);

    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate jxta_srdi_message_new\n");
        return JXTA_NOMEM;
    }

    if (jxta_rdv_service_is_rendezvous(me2->rdv_service)) {
        res = jxta_srdi_replicateEntries(me2->srdi_service, me2->resolver, msg, me2->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot replicate srdi message\n");
        }
    } else {
        res = jxta_srdi_message_get_xml(msg, &messageString);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot serialize srdi message\n");
            return JXTA_NOMEM;
        }

        srdi = jxta_resolver_srdi_new_1(me2->name, messageString, peerid);
        JXTA_OBJECT_RELEASE(messageString);
        if (srdi == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a resolver srdi message\n");
            return JXTA_NOMEM;
        }

        res = jxta_resolver_service_sendSrdi(me2->resolver, srdi, peerid);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot send srdi message\n");
        }
        JXTA_OBJECT_RELEASE(srdi);
    }

    JXTA_OBJECT_RELEASE(msg);
    return res;
}

static void piperesolver_free(Jxta_object * obj)
{
    Pipe_resolver *self = (Pipe_resolver *) obj;
    Jxta_status res;

    if (self->query_callback != NULL) {
        res = jxta_resolver_service_unregisterQueryHandler(self->resolver, self->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot unregister query handler\n");
        }
        JXTA_OBJECT_RELEASE(self->query_callback);
        self->query_callback = NULL;
    }

    if (self->response_listener != NULL) {
        res = jxta_resolver_service_unregisterResHandler(self->resolver, self->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot unregister response handler\n");
        }
        JXTA_OBJECT_RELEASE(self->response_listener);
        self->response_listener = NULL;
    }

    if (self->srdi_listener != NULL) {
        res = jxta_resolver_service_unregisterSrdiHandler(self->resolver, self->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot unregister srdi handler\n");
        }
        JXTA_OBJECT_RELEASE(self->srdi_listener);
        self->srdi_listener = NULL;
    }

    if (self->pending_requests != NULL) {
        JXTA_OBJECT_RELEASE(self->pending_requests);
        self->pending_requests = NULL;
    }

    if (self->resolver != NULL) {
        JXTA_OBJECT_RELEASE(self->resolver);
        self->resolver = NULL;
    }

    if (self->endpoint != NULL) {
        JXTA_OBJECT_RELEASE(self->endpoint);
        self->endpoint = NULL;
    }

    if (self->pipe_service != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe_service);
        self->pipe_service = NULL;
    }

    if (self->rdv_service != NULL) {
        JXTA_OBJECT_RELEASE(self->rdv_service);
        self->rdv_service = NULL;
    }

    if (self->srdi_service != NULL) {
        JXTA_OBJECT_RELEASE(self->srdi_service);
        self->srdi_service = NULL;
    }

    if (self->cm != NULL) {
        JXTA_OBJECT_RELEASE(self->cm);
        self->cm = NULL;
    }

    if (self->name != NULL) {
        JXTA_OBJECT_RELEASE(self->name);
        self->name = NULL;
    }

    if (self->local_peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->local_peerid);
        self->local_peerid = NULL;
    }

    self->group = NULL;

    if (self->pool != NULL) {
        apr_thread_mutex_destroy(self->mutex);
        apr_pool_destroy(self->pool);
        self->pool = NULL;
        self->mutex = NULL;
    }

    free(self);

    return;
}

Jxta_pipe_resolver *jxta_piperesolver_new(Jxta_PG * group)
{
    Pipe_resolver *self = NULL;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Default pipe resolver is starting\n");

    self = (Pipe_resolver *) malloc(sizeof(Pipe_resolver));
    memset(self, 0, sizeof(Pipe_resolver));
    JXTA_OBJECT_INIT(self, piperesolver_free, 0);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);

    if (res != APR_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot create mutex\n");
        return NULL;
    }


    self->pending_requests = jxta_hashtable_new(0);

    if (self->pending_requests == NULL) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate hashtable for pending requests\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->group = group;

    jxta_PG_get_resolver_service(self->group, &self->resolver);

    if (self->resolver == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot get resolver service\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    jxta_PG_get_endpoint_service(self->group, &self->endpoint);

    jxta_PG_get_pipe_service(self->group, &self->pipe_service);

    jxta_PG_get_rendezvous_service(self->group, &self->rdv_service);

    jxta_PG_get_srdi_service(self->group, &self->srdi_service);

    peergroup_get_cache_manager(self->group, &self->cm);

    self->rpv = jxta_rdv_service_get_peerview_priv((_jxta_rdv_service *) self->rdv_service);

    self->name = jstring_new_2(PIPE_RESOLVER_NAME);

    jxta_PG_get_PID(self->group, (Jxta_PID **) & self->local_peerid);

    /**
     ** Create the query and response listener and add them to the
     ** resolver service.
     **/

    jxta_callback_new(&self->query_callback, query_callback_func, (void *) self);

    res = jxta_resolver_service_registerQueryHandler(self->resolver, self->name, self->query_callback);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot register Query Handler to the resolver service\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->response_listener = jxta_listener_new(response_listener, (void *) self, 2, 200);

    res = jxta_resolver_service_registerResHandler(self->resolver, self->name, self->response_listener);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot register Res Handler to the resolver service\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    self->srdi_listener = jxta_listener_new(srdi_listener, (void *) self, 2, 200);

    res = jxta_resolver_service_registerSrdiHandler(self->resolver, self->name, self->srdi_listener);

    jxta_listener_start(self->response_listener);
    jxta_listener_start(self->srdi_listener);

    /**
     ** Initialize the VTBL.
     **/

    self->generic.local_resolve = local_resolve;
    self->generic.remote_resolve = remote_resolve;
    self->generic.timed_remote_resolve = timed_remote_resolve;
    self->generic.send_srdi = send_srdi;

    return (Jxta_pipe_resolver *) self;
}

static Jxta_status pending_request_add(Pipe_resolver * self, Pending_request * req)
{
    const char *pipeid = NULL;
    Jxta_vector *vector = NULL;
    Jxta_status res;

    apr_thread_mutex_lock(self->mutex);

    /* Retrieve the pipe id */
    pipeid = jxta_pipe_adv_get_Id(req->adv);

    /* First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, pipeid, strlen(pipeid), JXTA_OBJECT_PPTR(&vector));

    if (res != JXTA_SUCCESS) {
        /* No request found. Create a new vector */
        vector = jxta_vector_new(0);
        if (vector == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a vector. Pipe resolver request will be lost.\n");

            apr_thread_mutex_unlock(self->mutex);
            return JXTA_NOMEM;
        }
        jxta_hashtable_put(self->pending_requests, pipeid, strlen(pipeid), (Jxta_object *) vector);
    }

    res = jxta_vector_add_object_last(vector, (Jxta_object *) req);

    JXTA_OBJECT_RELEASE(vector);

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

static Jxta_status pending_request_remove(Pipe_resolver * self, Pending_request * req)
{
    const char *pipeid = NULL;
    Jxta_vector *vector = NULL;
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);

    /* Retrieve the pipe id */
    pipeid = jxta_pipe_adv_get_Id(req->adv);

    /* First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, pipeid, strlen(pipeid), JXTA_OBJECT_PPTR(&vector));

    if (res != JXTA_SUCCESS) {
        /* No request found. This is an error */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid pipe resolver request to remove.\n");

        apr_thread_mutex_unlock(self->mutex);

        return JXTA_INVALID_ARGUMENT;
    }

	jxta_vector_remove_object(vector, (Jxta_object*)req);

    if (jxta_vector_size(vector) == 0) {
        /* No need to keep an entry for this pipe */
        res = jxta_hashtable_del(self->pending_requests, pipeid, strlen(pipeid), NULL);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot remove vector from hashtable\n");
        }
    }

    JXTA_OBJECT_RELEASE(vector);
    apr_thread_mutex_unlock(self->mutex);
    return res;
}

static void pending_request_free(Jxta_object * obj)
{
    Pending_request *self = (Pending_request *) obj;

    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }

    if (self->dest != NULL) {
        JXTA_OBJECT_RELEASE(self->dest);
        self->dest = NULL;
    }

    if (self->listener != NULL) {
        JXTA_OBJECT_RELEASE(self->listener);
        self->listener = NULL;
    }

    free(self);

    return;
}

static Pending_request *pending_request_new(Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest,
                                            Jxta_listener * listener)
{
    Pending_request *self = NULL;
    Jxta_time currentTime;

    self = (Pending_request *) malloc(sizeof(Pending_request));
    memset(self, 0, sizeof(Pending_request));
    JXTA_OBJECT_INIT(self, pending_request_free, 0);

    JXTA_OBJECT_SHARE(adv);
    self->adv = adv;

    if (dest != NULL) {
        JXTA_OBJECT_SHARE(dest);
    }
    self->dest = dest;

    JXTA_OBJECT_SHARE(listener);
    self->listener = listener;

    currentTime = jpr_time_now();
    self->timeout = currentTime + timeout;

    return self;
}

static Jxta_status send_query(Pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_peer * peer)
{
    Jxta_status res = JXTA_SUCCESS;
    ResolverQuery *query = NULL;
    Jxta_id *peerid = NULL;
    JString *peeridString = NULL;
    JString *queryString = NULL;
    JString *peerdoc = NULL;
    Jxta_piperesolver_msg *msg = NULL;
    Jxta_PA *padv = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send pipe resolver Query\n");

    msg = jxta_piperesolver_msg_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate jxta_piperesolver_msg\n");
        return JXTA_NOMEM;
    }

    if (peer != NULL) {
        jxta_peer_get_peerid(peer, &peerid);
        if (peerid != NULL) {
            jxta_id_to_jstring(peerid, &peeridString);
        }
    }

    jxta_piperesolver_msg_set_MsgType(msg, (char *) QUERY_MSG_TYPE);
    jxta_piperesolver_msg_set_PipeId(msg, (char *) jxta_pipe_adv_get_Id(adv));
    jxta_piperesolver_msg_set_Type(msg, (char *) jxta_pipe_adv_get_Type(adv));

    /*
     * need to send the peer advertisement
     */
    jxta_PG_get_PA(self->group, &padv);
    res = jxta_PA_get_xml(padv, &peerdoc);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "pipe resolver::sendQuery failed to retrieve MyPeer Advertisement\n");
        JXTA_OBJECT_RELEASE(msg);
        if (padv != NULL)
            JXTA_OBJECT_RELEASE(padv);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        if (peeridString != NULL)
            JXTA_OBJECT_RELEASE(peeridString);
        return JXTA_NOMEM;
    }

    jxta_piperesolver_msg_set_PeerAdv(msg, peerdoc);

    if (peeridString != NULL) {
        jxta_piperesolver_msg_set_Peer(msg, (char *) jstring_get_string(peeridString));
        JXTA_OBJECT_RELEASE(peeridString);
    }
    jxta_piperesolver_msg_set_Cached(msg, FALSE);

    res = jxta_piperesolver_msg_get_xml(msg, &queryString);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot produce XML of pipe resolver query\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(padv);
        JXTA_OBJECT_RELEASE(peerdoc);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    query = jxta_resolver_query_new_1(self->name, queryString, self->local_peerid, NULL);

    if (query == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a resolver query\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(padv);
        JXTA_OBJECT_RELEASE(peerdoc);
        JXTA_OBJECT_RELEASE(queryString);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    /* If we are a rdv peer, send to the replica peer */
    if (jxta_rdv_service_is_rendezvous(self->rdv_service)) {

        char *expression = malloc(6 + strlen(jxta_pipe_adv_get_Id(adv)));

        if (expression == NULL)
            return JXTA_FAILED;
        sprintf(expression, "AdvId%s", jxta_pipe_adv_get_Id(adv));

        /* Algorithm for unicast pipes */
        if (strcmp(jxta_pipe_adv_get_Type(adv), "JxtaUnicast") == 0) {

            /* Nothing in the SRDI, so get the replica peer or walk */
            res = jxta_pipe_resolver_check_srdi(self, query, adv);
            if (res != JXTA_SUCCESS) {
                res = jxta_pipe_resolver_get_replica_peer(self, query, expression);
                if (res != JXTA_SUCCESS) {
                    /* We must start the walk */
                    jxta_resolver_query_set_hopcount(query, 1);
                    res = jxta_pipe_resolver_walk(self, query, expression);
                }
            }
        } else {
            /* Algorithm for propagate pipes */
            /* We don't care about the returned value as we always walk */
            jxta_pipe_resolver_check_srdi(self, query, adv);
            /* Start the walk */
            jxta_resolver_query_set_hopcount(query, 1);
            res = jxta_pipe_resolver_walk(self, query, expression);
        }
        free(expression);
    } else {

        res = jxta_resolver_service_sendQuery(self->resolver, query, peerid);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot send resolver request\n");
        }
    }

    JXTA_OBJECT_RELEASE(padv);
    JXTA_OBJECT_RELEASE(peerdoc);
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(query);
    JXTA_OBJECT_RELEASE(queryString);
    if (peerid != NULL)
        JXTA_OBJECT_RELEASE(peerid);

    return res;
}

static Jxta_status publish_peer_adv(Pipe_resolver * self, Jxta_piperesolver_msg * msg)
{
    Jxta_PA *padv;
    JString *tmp;
    Jxta_discovery_service *discovery;
    Jxta_status rv;

    jxta_piperesolver_msg_get_PeerAdv(msg, &tmp);
    if (NULL == tmp) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No peer advertisement found in pipe resolver message\n");
        return JXTA_SUCCESS;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer adv  = %s\n", jstring_get_string(tmp));
    padv = jxta_PA_new();
    if (NULL == padv) {
        JXTA_OBJECT_RELEASE(tmp);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate peer advertisement\n");
        return JXTA_NOMEM;
    }
    rv = jxta_PA_parse_charbuffer(padv, jstring_get_string(tmp), jstring_length(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    if (JXTA_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(padv);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to parse peer advertisement\n");
        return JXTA_FAILED;
    }

    jxta_PG_get_discovery_service(self->group, &discovery);
    if (NULL == discovery) {
        JXTA_OBJECT_RELEASE(padv);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to get Discovery service\n");
        return JXTA_FAILED;
    }

    rv = discovery_service_publish(discovery, (Jxta_advertisement *) padv, DISC_PEER, DEFAULT_EXPIRATION, LOCAL_ONLY_EXPIRATION);
    JXTA_OBJECT_RELEASE(padv);
    JXTA_OBJECT_RELEASE(discovery);

    return rv;
}

static Jxta_status jxta_pipe_resolver_walk(Pipe_resolver * me, ResolverQuery * rq, char *expression)
{
    Jxta_status res = JXTA_SUCCESS;

    res = jxta_resolver_service_sendQuery(me->resolver, rq, NULL);
    if (res == JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Walk for pipe query %s\n", expression);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to walk for pipe query %s\n", expression);
    }

    return res;
}

static Jxta_status jxta_pipe_resolver_check_srdi(Pipe_resolver * me, ResolverQuery * rq, Jxta_pipe_adv * adv)
{
    Jxta_vector *results;
    Jxta_status status = JXTA_SUCCESS;
    unsigned int i = 0;

    /** First check the SRDI */
    results = jxta_srdi_searchSrdi(me->srdi_service, jstring_get_string(me->name), NULL, "Id", jxta_pipe_adv_get_Id(adv));

    if(results == NULL)
        return JXTA_FAILED;

    if(jxta_vector_size(results) < 1) {
        JXTA_OBJECT_RELEASE(results);
        return JXTA_FAILED;
    }

    for (i = 0; i < jxta_vector_size(results); i++) {
        Jxta_id *peer_id;
        JString *peer_id_jstring;

        jxta_vector_get_object_at(results, JXTA_OBJECT_PPTR(&peer_id), i);
        jxta_id_to_jstring(peer_id, &peer_id_jstring);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Thanks to the SRDI forward the pipe query to %s\n",
                        jstring_get_string(peer_id_jstring));
        
        status = jxta_srdi_forwardQuery_peer(me->srdi_service, me->resolver, peer_id, rq);

        JXTA_OBJECT_RELEASE(peer_id);
        JXTA_OBJECT_RELEASE(peer_id_jstring);
        
        if ( status != JXTA_SUCCESS)
            break;
    }

    JXTA_OBJECT_RELEASE(results);

    return status;
}

static Jxta_status jxta_pipe_resolver_get_replica_peer(Pipe_resolver * me, ResolverQuery * rq, char *expression)
{
    Jxta_peer *replicaPeer = NULL;
    Jxta_id *repId = NULL;
    Jxta_peerview *rpv ;
    
    rpv= jxta_rdv_service_get_peerview_priv((_jxta_rdv_service *) me->rdv_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Getting a replica peer for %s\n", expression);
    replicaPeer = jxta_srdi_getReplicaPeer(me->srdi_service, me->resolver, rpv, expression);

  /** We have found a replica peer */
    if (NULL != replicaPeer) {
        Jxta_status res = JXTA_SUCCESS;

        jxta_peer_get_peerid(replicaPeer, &repId);
        if (repId == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to get a replica peer id from the replica peer\n");
            return JXTA_FAILED;
        }

    /** It is not us so forward the query */
        if (!jxta_id_equals(me->local_peerid, repId)) {
            JString *repId_jstring;

            jxta_id_to_jstring(repId, &repId_jstring);
            res = jxta_srdi_forwardQuery_peer(me->srdi_service, me->resolver, repId, rq);
            if (res == JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Query %s was forwarded to replica %s\n",
                                expression, jstring_get_string(repId_jstring));
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to forward query %s to replica %s\n",
                                expression, jstring_get_string(repId_jstring));
            }

            JXTA_OBJECT_RELEASE(repId_jstring);
        } else {
      /** It is us, so get replica peer fails **/
            res = JXTA_FAILED;
        }
        JXTA_OBJECT_RELEASE(replicaPeer);
        JXTA_OBJECT_RELEASE(repId);

        return res;
    }
    return JXTA_FAILED;
}

/*
 * Query callback handle's resolver queries for pipe
 * @param: Jxta_object *obj: the ResolverQuery
 * @param: void *me: the self pointer
 * @return: JXTA_SUCCESS if the query is handled and no more operation is needed. JXTA_ITEM_NOTFOUND if the query cannot be
 * fulfilled. Other values could be returned to indicate an error.
 * 
 * The handler will try to solve the pipe locally. If the query is solved, sends a response back to the inquirer, otherwise, based
 * on the role of this peer,
 * For RDV:
 *   Forward to the replica peer if has not done so. Otherwise, return JXTA_ITEM_NOTFOUND.
 * For EDGE:
 *   Return JXTA_SUCCESS to stop the query.
 *
 * By returning a value other than JXTA_SUCCESS, the resolver would try to walk the query.
 */
static Jxta_status JXTA_STDCALL query_callback_func(Jxta_object * obj, void *me)
{
    Pipe_resolver *_self = (Pipe_resolver *) me;
    ResolverQuery *rq = (ResolverQuery *) obj;
    ResolverResponse *rr = NULL;
    JString *queryString = NULL;
    Jxta_piperesolver_msg *msg = NULL;
    Jxta_piperesolver_msg *reply = NULL;
    Jxta_pipe_adv *adv = NULL;
    Jxta_pipe_resolver *pipe_resolver = NULL;
    JString *peer_id_jstring = NULL;
    Jxta_id *peer_id = NULL;
    Jxta_vector *peers = NULL;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_PA *padv;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "********** PIPE RESOLVER QUERY LISTENER *****************\n");
    jxta_resolver_query_get_query(rq, &queryString);
    msg = jxta_piperesolver_msg_new();
    jxta_piperesolver_msg_parse_charbuffer(msg,
                                           (const char *) jstring_get_string(queryString),
                                           strlen(jstring_get_string(queryString)));

    publish_peer_adv(_self, msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Resolver query:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "   msg type = %s\n", jxta_piperesolver_msg_MsgType(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "    pipe id = %s\n", jxta_piperesolver_msg_PipeId(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "  pipe type = %s\n", jxta_piperesolver_msg_Type(msg));
    if (jxta_piperesolver_msg_Peer(msg) != NULL)
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       peer = %s\n", jxta_piperesolver_msg_Peer(msg));

    if (!strcmp(jxta_piperesolver_msg_Type(msg), "JxtaPropagate")) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Get a resolver request for PROPAGATE pipe, ignore it.\n");
        goto FINAL_EXIT;
    }
        
    /**
     ** We need to create a pipe advertisement to describe this pipe
     **/
    adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, jxta_piperesolver_msg_PipeId(msg));
    jxta_pipe_adv_set_Type(adv, jxta_piperesolver_msg_Type(msg));

    peer_id_jstring = jstring_new_2(jxta_piperesolver_msg_Peer(msg));
    if (peer_id_jstring != NULL) {
        jxta_id_from_jstring(&peer_id, peer_id_jstring);
        JXTA_OBJECT_RELEASE(peer_id_jstring);
    }

    if (peer_id != NULL && (!jxta_id_equals(peer_id, _self->local_peerid))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Directed query for pipe %s but not on the correct peer, ignore it.\n",
                        jxta_pipe_adv_get_Id(adv));
        goto FINAL_EXIT;
    }

    if (JXTA_SUCCESS == jxta_pipe_resolver_local_resolve((Jxta_pipe_resolver *)_self, adv, &peers)) {
        JString *peerdoc = NULL;
        JString *tmpString = NULL;

        /*
         * There is a local listener for the requested pipe. Answer.
         */
        jxta_id_to_jstring(_self->local_peerid, &tmpString);
        reply = jxta_piperesolver_msg_new();
        jxta_piperesolver_msg_set_MsgType(reply, (char *) RESPONSE_MSG_TYPE);
        jxta_piperesolver_msg_set_PipeId(reply, (char*)jxta_piperesolver_msg_PipeId(msg));
        jxta_piperesolver_msg_set_Type(reply, (char*)jxta_piperesolver_msg_Type(msg));
        jxta_piperesolver_msg_set_Found(reply, TRUE);
        jxta_piperesolver_msg_set_Peer(reply, (char *) jstring_get_string(tmpString));
        JXTA_OBJECT_RELEASE(tmpString);

        /*
         * need to send the peer advertisement
         */
        jxta_PG_get_PA(_self->group, &padv);
        res = jxta_PA_get_xml(padv, &peerdoc);
        JXTA_OBJECT_RELEASE(padv);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                            "pipe resolver::sendListener failed to retrieve MyPeer Advertisement\n");
        } else {
            Jxta_id *src_peer_id = NULL;
            JString *src_pid = NULL;

            jxta_piperesolver_msg_set_PeerAdv(reply, peerdoc);
            JXTA_OBJECT_RELEASE(peerdoc);
            jxta_piperesolver_msg_get_xml(reply, &tmpString);

            /*
             * Send the message back.
             */
            rr = jxta_resolver_response_new_2(_self->name, tmpString, jxta_resolver_query_get_queryid(rq), _self->local_peerid);
            JXTA_OBJECT_RELEASE(tmpString);

            src_peer_id = jxta_resolver_query_get_src_peer_id(rq);
            jxta_id_to_jstring(src_peer_id, &src_pid);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send pipe resolver response to peer %s\n",
                            jstring_get_string(src_pid));
            res = jxta_resolver_service_sendResponse(_self->resolver, rr, src_peer_id);
            JXTA_OBJECT_RELEASE(src_peer_id);
            JXTA_OBJECT_RELEASE(src_pid);
        }
        /* local resolvable */
        res = JXTA_SUCCESS;
    } else {
        /* We are a rdv peer, look for something .. */
        if (! jxta_rdv_service_is_rendezvous(_self->rdv_service)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Not a rdv peer, nothing more we can do for %s!\n",
                            jxta_pipe_adv_get_Id(adv));
            res = JXTA_SUCCESS;
        } else {
            char *expression = malloc(6 + strlen(jxta_pipe_adv_get_Id(adv)));

            if (expression == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Out of memory?.\n");
                res = JXTA_NOMEM;
                goto FINAL_EXIT;
            }
            sprintf(expression, "AdvId%s", jxta_pipe_adv_get_Id(adv));

            /* look into SRDI */
            res = jxta_pipe_resolver_check_srdi(_self, rq, adv);
            if (res != JXTA_SUCCESS) {
                /* Nothing in the SRDI, so next step if replica peer */
                if (jxta_resolver_query_get_hopcount(rq) == 0) {
                    res = jxta_pipe_resolver_get_replica_peer(_self, rq, expression);
                }
            }
            free(expression);

            if (res != JXTA_SUCCESS) {
                jxta_resolver_query_set_hopcount(rq, jxta_resolver_query_get_hopcount(rq) + 1);
                res = JXTA_ITEM_NOTFOUND;
            }
        }
    }

  FINAL_EXIT:
    if (peer_id != NULL)
        JXTA_OBJECT_RELEASE(peer_id);
    if (reply != NULL)
        JXTA_OBJECT_RELEASE(reply);
    if (rr != NULL)
        JXTA_OBJECT_RELEASE(rr);
    if (adv != NULL)
        JXTA_OBJECT_RELEASE(adv);
    if (peers != NULL)
        JXTA_OBJECT_RELEASE(peers);
    if (pipe_resolver != NULL)
        JXTA_OBJECT_RELEASE(pipe_resolver);
    if (queryString != NULL)
        JXTA_OBJECT_RELEASE(queryString);
    if (msg != NULL)
        JXTA_OBJECT_RELEASE(msg);
    return res;
}

static void JXTA_STDCALL response_listener(Jxta_object * obj, void *arg)
{
    Pipe_resolver *self = (Pipe_resolver *) arg;
    ResolverResponse *rr = (ResolverResponse *) obj;
    Jxta_id *rr_res_pid = NULL;
    JString *rr_res_pid_jstring = NULL;
    JString *queryString = NULL;
    Jxta_piperesolver_msg *msg = NULL;
    Jxta_status res;
    const char *pipeid = NULL;
    Pending_request *req = NULL;
    Jxta_id *peerid = NULL;
    Jxta_id *tmpPeerid = NULL;
    Jxta_vector *tmpVector = NULL;
    Jxta_vector *reqs = jxta_vector_new(0);
    Jxta_peer *remote_peer;
    JString *peerString = NULL;
    Jxta_vector *vector = NULL;
    unsigned int i;
    Jxta_pipe_resolver_event *event = NULL;

    JXTA_OBJECT_CHECK_VALID(rr);
    JXTA_OBJECT_CHECK_VALID(self);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "********** PIPE RESOLVER RESPONSE LISTENER *****************\n");

    jxta_resolver_response_get_response(rr, &queryString);
    rr_res_pid = jxta_resolver_response_get_res_peer_id(rr);
    if (rr_res_pid != NULL) {
        jxta_id_to_jstring(rr_res_pid, &rr_res_pid_jstring);
        JXTA_OBJECT_RELEASE(rr_res_pid);
    }

    msg = jxta_piperesolver_msg_new();
    jxta_piperesolver_msg_parse_charbuffer(msg,
                                           jstring_get_string(queryString),
                                           strlen(jstring_get_string(queryString)));

    publish_peer_adv(self, msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Resolver Response:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "from peerid = %s\n",
                    (rr_res_pid_jstring != NULL ? jstring_get_string(rr_res_pid_jstring) : "Unknown"));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "   msg type = %s\n", jxta_piperesolver_msg_MsgType(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "    pipe id = %s\n", jxta_piperesolver_msg_PipeId(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "  pipe type = %s\n", jxta_piperesolver_msg_Type(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       peer = %s\n", jxta_piperesolver_msg_Peer(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "     status = %s\n",
                    jxta_piperesolver_msg_get_Found(msg) == TRUE ? "Found" : "NACK");

    if (rr_res_pid_jstring)
        JXTA_OBJECT_RELEASE(rr_res_pid_jstring);

    pipeid = jxta_piperesolver_msg_PipeId(msg);

    if (pipeid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid response\n");
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);

        return;
    }

    peerString = jstring_new_2((const char *) jxta_piperesolver_msg_Peer(msg));
    res = jxta_id_from_jstring(&peerid, peerString);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot build a jstring from the peerid\n");
        JXTA_OBJECT_RELEASE(peerString);
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);
        return;
    }

    JXTA_OBJECT_RELEASE(peerString);

    remote_peer = jxta_peer_new();
    jxta_peer_set_peerid(remote_peer, peerid);

    /* Invoke listeners */
    apr_thread_mutex_lock(self->mutex);
    /* Retrieve the pipe id
     * First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, pipeid, strlen(pipeid), JXTA_OBJECT_PPTR(&vector));

    if (res != JXTA_SUCCESS) {
        /* No request found. This is an error */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No pending request\n");
        JXTA_OBJECT_RELEASE(peerid);
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);
        apr_thread_mutex_unlock(self->mutex);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(vector);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Starting loop\n");
    for (i = 0; i < jxta_vector_size(vector); ++i) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "in loop %d\n", i);
        res = jxta_vector_get_object_at(vector, JXTA_OBJECT_PPTR(&req), i);
        if (res != JXTA_SUCCESS) {
            continue;
        }

        /* Check if this response is coming from the appropriate peer */
        if (req->dest != NULL) {
            res = jxta_peer_get_peerid(req->dest, &tmpPeerid);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve peerid from peer\n");
            }
            if (jxta_id_equals(tmpPeerid, peerid)) {
                jxta_vector_add_object_first(reqs, (Jxta_object *) req);
                JXTA_OBJECT_CHECK_VALID(req);
            }
            JXTA_OBJECT_RELEASE(tmpPeerid);
        } else {
            jxta_vector_add_object_first(reqs, (Jxta_object *) req);
            JXTA_OBJECT_CHECK_VALID(req);
        }

        JXTA_OBJECT_RELEASE(req);
        JXTA_OBJECT_CHECK_VALID(req);
    }
    JXTA_OBJECT_RELEASE(vector);

    apr_thread_mutex_unlock(self->mutex);

    /* Invoke listeners */
    for (i = 0; i < jxta_vector_size(reqs); ++i) {
        res = jxta_vector_get_object_at(reqs, JXTA_OBJECT_PPTR(&req), i);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve req from vector\n");
            continue;
        }

        JXTA_OBJECT_CHECK_VALID(req);
        pending_request_remove(self, req);
        tmpVector = jxta_vector_new(0);
        res = jxta_vector_add_object_first(tmpVector, (Jxta_object *) remote_peer);
        event = jxta_pipe_resolver_event_new(JXTA_PIPE_RESOLVER_RESOLVED, req->adv, tmpVector);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invoke listener req= %x\n", req);
        JXTA_OBJECT_CHECK_VALID(req->listener);
        jxta_listener_process_object(req->listener, (Jxta_object *) event);

        JXTA_OBJECT_RELEASE(event);
        JXTA_OBJECT_RELEASE(tmpVector);
        JXTA_OBJECT_RELEASE(req);

        JXTA_OBJECT_CHECK_VALID(req);
    }

    JXTA_OBJECT_RELEASE(reqs);
    JXTA_OBJECT_RELEASE(remote_peer);
    JXTA_OBJECT_RELEASE(peerid);
    JXTA_OBJECT_RELEASE(queryString);
    JXTA_OBJECT_RELEASE(msg);
}

static void JXTA_STDCALL srdi_listener(Jxta_object * obj, void *arg)
{
    Jxta_status status;
    Pipe_resolver *self = (Pipe_resolver *) arg;
    Jxta_SRDIMessage *smsg;
    Jxta_vector *entries = NULL;
    JString *jPeerid = NULL;
    JString *jPrimaryKey = NULL;
    unsigned int i;
    Jxta_id *peerid = NULL;
    Jxta_boolean bReplica = FALSE;

    if ( !JXTA_OBJECT_CHECK_VALID(obj) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message with an invalid obj \n");
        return;
    }

    if (!jxta_rdv_service_is_rendezvous(self->rdv_service)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Dropping SRDI pipe message as we are an edge\n");
        return;
    }

    smsg = jxta_srdi_message_new();

    status = jxta_srdi_message_parse_charbuffer(smsg, jstring_get_string((JString *) obj), jstring_length((JString *) obj));
    
    if( JXTA_SUCCESS != status ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not parse SRDI message.\n");
        goto FINAL_EXIT;
    }
    
    jxta_srdi_message_get_peerID(smsg, &peerid);
    jxta_id_to_jstring(peerid, &jPeerid);

    if (jPeerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "ooops - got a srdi message without a peerid \n");
        goto FINAL_EXIT;
    }
    
    jxta_srdi_message_get_primaryKey(smsg, &jPrimaryKey);

    /** Replicates entries only if unicast pipes */
    if (strcmp(jstring_get_string(jPrimaryKey), "JxtaUnicast") == 0) {
        Jxta_vector *localView = NULL;

        status = jxta_peerview_get_localview(self->rpv, &localView);
        if((JXTA_SUCCESS == status) && (NULL != localView)) {
            unsigned int rpv_size = jxta_vector_size(localView);
            for (i = 0; i < rpv_size; i++) {
                Jxta_id *tmpId;
                Jxta_peer *peer;

                status = jxta_vector_get_object_at(localView, JXTA_OBJECT_PPTR(&peer), i);
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "didn't get a peer from the peerview status: %i\n",
                                    status);
                    break;
                }
                status = jxta_peer_get_peerid(peer, &tmpId);
                JXTA_OBJECT_RELEASE(peer);
                if (JXTA_SUCCESS != status) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "didn't get the peerid from pve\n");
                    break;
                } 
                  
                /* Origin peer is in peerview, this is a replica */
                if (jxta_id_equals( tmpId, peerid )) {
                    bReplica = TRUE;
                }
                
                JXTA_OBJECT_RELEASE(tmpId);
                
                if (bReplica)
                    break;
            }
        }
        if (!bReplica) {
            jxta_srdi_replicateEntries(self->srdi_service, self->resolver, smsg, self->name);
        }
        JXTA_OBJECT_RELEASE(localView);
    }
    
    status = jxta_srdi_message_get_entries(smsg, &entries);
    if (status != JXTA_SUCCESS) {
        goto FINAL_EXIT;
    }
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Saving SRDI %sentries -- peerid : %s entries : %d\n",
                    (bReplica ? "replica" : ""), jstring_get_string(jPeerid), jxta_vector_size(entries));

    if (!bReplica) {
        cm_save_srdi_elements(self->cm, self->name, jPeerid, jPrimaryKey, entries, NULL);
    } else {
        cm_save_replica_elements(self->cm, self->name, jPeerid, jPrimaryKey, entries, NULL);
    }

FINAL_EXIT:
    JXTA_OBJECT_RELEASE(smsg);
    JXTA_OBJECT_RELEASE(jPrimaryKey);
    if (peerid != NULL) {
        JXTA_OBJECT_RELEASE(peerid);
    }
    if (jPeerid != NULL) {
        JXTA_OBJECT_RELEASE(jPeerid);
    }
    if (entries != NULL) {
        JXTA_OBJECT_RELEASE(entries);
    }
}

static void pipe_resolver_event_free(Jxta_object * obj)
{
    Jxta_pipe_resolver_event *self = (Jxta_pipe_resolver_event *) obj;

    if (self->peers != NULL) {
        JXTA_OBJECT_CHECK_VALID(self->peers);
        JXTA_OBJECT_RELEASE(self->peers);
        self->peers = NULL;
    }

    if (self->adv != NULL) {
        JXTA_OBJECT_CHECK_VALID(self->adv);
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }

    free(self);
}

Jxta_pipe_resolver_event *jxta_pipe_resolver_event_new(int ev, Jxta_pipe_adv * adv, Jxta_vector * peers)
{
    Jxta_pipe_resolver_event *self ;
    
    self= (Jxta_pipe_resolver_event *) malloc(sizeof(Jxta_pipe_resolver_event));
    memset(self, 0, sizeof(Jxta_pipe_resolver_event));

    JXTA_OBJECT_CHECK_VALID(adv);
    JXTA_OBJECT_CHECK_VALID(peers);

    JXTA_OBJECT_INIT(self, pipe_resolver_event_free, 0);

    self->event = ev;
    if (peers != NULL) {
        JXTA_OBJECT_SHARE(peers);
    }
    self->peers = peers;

    if (adv != NULL) {
        JXTA_OBJECT_SHARE(adv);
    }
    self->adv = adv;

    JXTA_OBJECT_CHECK_VALID(self);

    return self;
}

int jxta_pipe_resolver_event_get_event(Jxta_pipe_resolver_event * self)
{
    JXTA_OBJECT_CHECK_VALID(self);
    return self->event;
}

Jxta_status jxta_pipe_resolver_event_get_peers(Jxta_pipe_resolver_event * self, Jxta_vector ** peers)
{
    JXTA_OBJECT_CHECK_VALID(self);

    if (self->peers != NULL) {
        JXTA_OBJECT_CHECK_VALID(self->peers);
        JXTA_OBJECT_SHARE(self->peers);
        *peers = self->peers;
        return JXTA_SUCCESS;
    }
    return JXTA_FAILED;
}

Jxta_status jxta_pipe_resolver_event_get_adv(Jxta_pipe_resolver_event * self, Jxta_pipe_adv ** adv)
{
    JXTA_OBJECT_CHECK_VALID(self);

    if (self->adv != NULL) {
        JXTA_OBJECT_CHECK_VALID(self->adv);
        JXTA_OBJECT_SHARE(self->adv);
        *adv = self->adv;
        return JXTA_SUCCESS;
    }
    return JXTA_FAILED;
}

/* vim: set ts=4 sw=4 tw=130 et: */
