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
 * $Id: jxta_piperesolver_impl.c,v 1.17 2005/02/16 19:37:33 slowhog Exp $
 */

#include <limits.h>
#include "jxtaapr.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_id.h"
#include "jxta_peergroup.h"
#include "jxta_pipe_service.h"
#include "jxta_hashtable.h"
#include "jxta_vector.h"
#include "jxta_resolver_service.h"
#include "jxta_rq.h"
#include "jxta_rr.h"
#include "jxta_srdi.h"
#include "jxta_message.h"
#include "jxta_piperesolver_msg.h"
#include "jxta_listener.h"
#include "jxta_endpoint_service.h"
#include "jxta_routea.h"
#include "jxta_log.h"

static const char* __log_cat = "PIPE_RESOLVER";

typedef struct {
    Jxta_pipe_resolver generic;
    Jxta_PG *group;
    Jxta_id *local_peerid;
    Jxta_resolver_service *resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_hashtable *pending_requests;
    Jxta_listener *query_listener;
    Jxta_listener *response_listener;
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
#define OPEN_PIPE LONG_MAX
#define CLOSE_PIPE 0
static Jxta_status pending_request_add(Pipe_resolver *, Pending_request *);
static Jxta_status pending_request_remove(Pipe_resolver *, Pending_request *);
static Pending_request *pending_request_new(Jxta_pipe_adv *, Jxta_time, Jxta_peer *, Jxta_listener *);
static void query_listener(Jxta_object * obj, void *arg);
static void response_listener(Jxta_object * obj, void *arg);
static Jxta_status send_query(Pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_peer * peer);
static Jxta_status send_srdi(Pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_id * peerid, Jxta_boolean adding);


static Jxta_status local_resolve(Jxta_pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_vector ** peers)
{

    return JXTA_NOTIMP;
}


static Jxta_status
remote_resolve(Jxta_pipe_resolver * obj, Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_peer * dest, Jxta_listener * listener)
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

static Jxta_status
timed_remote_resolve(Jxta_pipe_resolver * obj, Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_peer * dest, Jxta_vector ** peers)
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

    res = jxta_listener_wait_for_event(listener, timeout, (Jxta_object **) & event);

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

static Jxta_status send_srdi(Pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_id * peerid, Jxta_boolean adding)
{

    Jxta_status res = JXTA_SUCCESS;
    ResolverSrdi *srdi = NULL;
    JString *messageString = NULL;
    JString *pkey = NULL;
    JString *skey = NULL;
    JString *value = NULL;
    Jxta_SRDIMessage *msg = NULL;
    Jxta_SRDIEntryElement *entry;
    Jxta_vector *entries;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send pipe SRDI message\n");
    pkey = jstring_new_2(jxta_pipe_adv_get_Type(adv));
    skey = jstring_new_2("Id");
    value = jstring_new_2(jxta_pipe_adv_get_Id(adv));
    if (adding) {
        entry = jxta_srdi_new_element_1(skey, value, (Jxta_expiration_time) OPEN_PIPE);
    } else {
        entry = jxta_srdi_new_element_1(skey, value, (Jxta_expiration_time) CLOSE_PIPE);
    }
    if (entry == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate memory for an srdi entry\n");
        if (peerid != NULL) {
            JXTA_OBJECT_RELEASE(peerid);
            JXTA_OBJECT_RELEASE(pkey);
            JXTA_OBJECT_RELEASE(skey);
            JXTA_OBJECT_RELEASE(value);
        }
        return JXTA_NOMEM;
    }

    entries = jxta_vector_new(1);
    if (entries == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate memory for the entries vector\n");
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        JXTA_OBJECT_RELEASE(pkey);
        JXTA_OBJECT_RELEASE(skey);
        JXTA_OBJECT_RELEASE(value);
        return JXTA_NOMEM;
    }
    jxta_vector_add_object_last(entries, (Jxta_object *) entry);
    JXTA_OBJECT_RELEASE(entry);
    msg = jxta_srdi_message_new_1(1, self->local_peerid, (char *) jstring_get_string(pkey), entries);
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate jxta_srdi_message_new\n");
        return JXTA_NOMEM;
    }

    res = jxta_srdi_message_get_xml(msg, &messageString);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot serialize srdi message\n");
        JXTA_OBJECT_RELEASE(msg);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    srdi = jxta_resolver_srdi_new_1(self->name, messageString, peerid);
    if (srdi == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a resolver srdi message\n");
        JXTA_OBJECT_RELEASE(msg);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    res = jxta_resolver_service_sendSrdi(self->resolver, srdi, peerid);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot send srdi message\n");
    }
    JXTA_OBJECT_RELEASE(srdi);
    if (peerid != NULL) {
        JXTA_OBJECT_RELEASE(peerid);
    }
    return res;
}

static void piperesolver_free(Jxta_object * obj)
{

    Pipe_resolver *self = (Pipe_resolver *) obj;
    Jxta_status res;


    if (self->query_listener != NULL) {

        res = jxta_resolver_service_unregisterQueryHandler(self->resolver, self->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot unregister query handler\n");
        }
        JXTA_OBJECT_RELEASE(self->query_listener);
        self->query_listener = NULL;
    }


    if (self->response_listener != NULL) {

        res = jxta_resolver_service_unregisterResHandler(self->resolver, self->name);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot unregister response handler\n");
        }
        JXTA_OBJECT_RELEASE(self->response_listener);
        self->response_listener = NULL;
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

    if (self->name != NULL) {
        JXTA_OBJECT_RELEASE(self->name);
        self->name = NULL;
    }

    if (self->local_peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->local_peerid);
        self->local_peerid = NULL;
    }

    if (self->group != NULL) {
        JXTA_OBJECT_RELEASE(self->group);
        self->group = NULL;
    }

    if (self->pool != NULL) {
        apr_pool_destroy(self->pool);
        self->pool = NULL;
        self->mutex = NULL;
    }

    free(self);

    return;
}



Jxta_pipe_resolver *jxta_piperesolver_impl_new(Jxta_PG * group)
{

    Pipe_resolver *self = NULL;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Default pipe resolver is starting\n");

    self = (Pipe_resolver *) malloc(sizeof(Pipe_resolver));
    memset(self, 0, sizeof(Pipe_resolver));
    JXTA_OBJECT_INIT(self, piperesolver_free, 0);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
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

    JXTA_OBJECT_SHARE(group);
    self->group = group;

    jxta_PG_get_resolver_service(self->group, &self->resolver);

    if (self->resolver == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot get resolver service\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    jxta_PG_get_endpoint_service(self->group, &self->endpoint);

    self->name = jstring_new_2(PIPE_RESOLVER_NAME);

    jxta_PG_get_PID(self->group, (Jxta_PID **) & self->local_peerid);

    /**
     ** Create the query and response listener and add them to the
     ** resolver service.
     **/

    self->query_listener = jxta_listener_new(query_listener, (void *) self, 2, 200);

    res = jxta_resolver_service_registerQueryHandler(self->resolver, self->name, self->query_listener);

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

    jxta_listener_start(self->query_listener);
    jxta_listener_start(self->response_listener);

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

    char *pipeid = NULL;
    Jxta_vector *vector = NULL;
    Jxta_status res;

    apr_thread_mutex_lock(self->mutex);

    /* Retrieve the pipe id */
    pipeid = jxta_pipe_adv_get_Id(req->adv);

    /* First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, (const void *) pipeid, strlen(pipeid), (Jxta_object **) & vector);

    if (res != JXTA_SUCCESS) {
        /* No request found. Create a new vector */
        vector = jxta_vector_new(0);
        if (vector == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a vector. Pipe resolver request will be lost.\n");

            apr_thread_mutex_unlock(self->mutex);
            return JXTA_NOMEM;
        }
        jxta_hashtable_put(self->pending_requests, (const void *) pipeid, strlen(pipeid), (Jxta_object *) vector);
    }

    res = jxta_vector_add_object_last(vector, (Jxta_object *) req);

    JXTA_OBJECT_RELEASE(vector);

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}


static Jxta_status pending_request_remove(Pipe_resolver * self, Pending_request * req)
{

    char *pipeid = NULL;
    Jxta_vector *vector = NULL;
    int i;
    Jxta_status res = JXTA_SUCCESS;

    Pending_request *tmp = NULL;

    apr_thread_mutex_lock(self->mutex);

    /* Retrieve the pipe id */
    pipeid = jxta_pipe_adv_get_Id(req->adv);

    /* First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, (const void *) pipeid, strlen(pipeid), (Jxta_object **) & vector);

    if (res != JXTA_SUCCESS) {
        /* No request found. This is an error */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid pipe resolver request to remove.\n");

        apr_thread_mutex_unlock(self->mutex);

        return JXTA_INVALID_ARGUMENT;
    }

    for (i = 0; i < jxta_vector_size(vector); ++i) {
        res = jxta_vector_get_object_at(vector, (Jxta_object **) & tmp, i);
        if (res != JXTA_SUCCESS) {
            continue;
        }
        if (tmp == req) {
            /* This is the request to remove */
            res = jxta_vector_remove_object_at(vector, NULL, i);
            break;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }

    if (jxta_vector_size(vector) == 0) {
        /* No need to keep an entry for this pipe */
        res = jxta_hashtable_del(self->pending_requests, (const void *) pipeid, strlen(pipeid), NULL);

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

static Pending_request *pending_request_new(Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_peer * dest, Jxta_listener * listener)
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
    Jxta_RouteAdvertisement *route = NULL;

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
    jxta_piperesolver_msg_set_PipeId(msg, jxta_pipe_adv_get_Id(adv));
    jxta_piperesolver_msg_set_Type(msg, jxta_pipe_adv_get_Type(adv));

    /*
     * need to send the peer advertisement
     */
    jxta_PG_get_PA(self->group, &padv);
    res = jxta_PA_get_xml(padv, &peerdoc);

    if (res != JXTA_SUCCESS) {
        fprintf(stderr, "pipe resolver::sendQuery failed to retrieve MyPeer Advertisement\n");
        return -1;
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
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    route = jxta_endpoint_service_get_local_route(self->endpoint);
    query = jxta_resolver_query_new_1(self->name, queryString, self->local_peerid, route);
    if (route != NULL)
        JXTA_OBJECT_RELEASE(route);

    if (query == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot allocate a resolver query\n");
        JXTA_OBJECT_RELEASE(msg);
        if (peerid != NULL)
            JXTA_OBJECT_RELEASE(peerid);
        return JXTA_NOMEM;
    }

    res = jxta_resolver_service_sendQuery(self->resolver, query, peerid);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot send resolver request\n");
    }

    JXTA_OBJECT_RELEASE(query);
    if (peerid != NULL)
        JXTA_OBJECT_RELEASE(peerid);

    return res;
}

static Jxta_status publish_peer_adv(Pipe_resolver *self, Jxta_piperesolver_msg *msg)
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate peer advertisement\n");
        return JXTA_NOMEM;
    }
    jxta_PA_parse_charbuffer(padv, jstring_get_string(tmp), jstring_length(tmp));
    JXTA_OBJECT_RELEASE(tmp);

    jxta_PG_get_discovery_service(self->group, &discovery);
    /* TODO: What if discovery is NULL? Can this happen? */
    
    rv = discovery_service_publish(discovery, padv, DISC_PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
    JXTA_OBJECT_RELEASE(padv);
    JXTA_OBJECT_RELEASE(discovery);

    return rv;
}

static void query_listener(Jxta_object * obj, void *arg)
{

    Pipe_resolver *self = (Pipe_resolver *) arg;
    ResolverQuery *rq = (ResolverQuery *) obj;
    ResolverResponse *rr = NULL;
    JString *queryString = NULL;
    Jxta_piperesolver_msg *msg = NULL;
    Jxta_piperesolver_msg *reply = NULL;
    Jxta_pipe_adv *adv = NULL;
    Jxta_pipe_service_impl *pipe_impl = NULL;
    Jxta_pipe_service *pipe_service = NULL;
    Jxta_pipe_resolver *pipe_resolver = NULL;
    Jxta_vector *peers = NULL;
    JString *tmpString = NULL;
    Jxta_status res;
    JString *peerdoc = NULL;
    Jxta_PA *padv;
    JString *pid = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "********** PIPE RESOLVER QUERY LISTENER *****************\n");
    jxta_resolver_query_get_query(rq, &queryString);
    msg = jxta_piperesolver_msg_new();
    jxta_piperesolver_msg_parse_charbuffer(msg,
                                           (const char *) jstring_get_string(queryString),
                                           strlen(jstring_get_string(queryString)));

    publish_peer_adv(self, msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Resolver query:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "   msg type = %s\n", jxta_piperesolver_msg_get_MsgType(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "    pipe id = %s\n", jxta_piperesolver_msg_get_PipeId(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "  pipe type = %s\n", jxta_piperesolver_msg_get_Type(msg));
    if (jxta_piperesolver_msg_get_Peer(msg) != NULL)
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       peer = %s\n", jxta_piperesolver_msg_get_Peer(msg));

    /**
     ** We need to create a pipe advertisement to describe this pipe
     **/
    adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, jxta_piperesolver_msg_get_PipeId(msg));
    jxta_pipe_adv_set_Type(adv, jxta_piperesolver_msg_get_Type(msg));

    /**
     ** resolve locally in the appropriate instance of pipe
     **/
    jxta_PG_get_pipe_service(self->group, &pipe_service);

    res = jxta_pipe_service_lookup_impl(pipe_service, jxta_pipe_adv_get_Type(adv), &pipe_impl);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve pipe implementation for this type [%s]\n", jxta_pipe_adv_get_Type(adv));
        goto error;
    }

    res = jxta_pipe_service_impl_get_pipe_resolver(pipe_impl, &pipe_resolver);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve pipe resolver for this type [%s]\n", jxta_pipe_adv_get_Type(adv));
        goto error;
    }

    res = jxta_pipe_resolver_local_resolve(pipe_resolver, adv, &peers);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Not a local pipe\n");
        goto error;
    }

    /**
     ** This is a local pipe. Answer.
     **/
    jxta_id_to_jstring(self->local_peerid, &tmpString);
    reply = jxta_piperesolver_msg_new();
    jxta_piperesolver_msg_set_MsgType(reply, (char *) RESPONSE_MSG_TYPE);
    jxta_piperesolver_msg_set_PipeId(reply, jxta_piperesolver_msg_get_PipeId(msg));
    jxta_piperesolver_msg_set_Type(reply, jxta_piperesolver_msg_get_Type(msg));
    jxta_piperesolver_msg_set_Found(reply, TRUE);
    jxta_piperesolver_msg_set_Peer(reply, (char *) jstring_get_string(tmpString));

    /*
     * need to send the peer advertisement
     */
    jxta_PG_get_PA(self->group, &padv);
    res = jxta_PA_get_xml(padv, &peerdoc);
    JXTA_OBJECT_RELEASE(padv);

    if (res != JXTA_SUCCESS) {
        fprintf(stderr, "pipe resolver::sendListener failed to retrieve MyPeer Advertisement\n");
        return;
    }

    jxta_piperesolver_msg_set_PeerAdv(reply, peerdoc);
    JXTA_OBJECT_RELEASE(peerdoc);
    JXTA_OBJECT_RELEASE(tmpString);
    jxta_piperesolver_msg_get_xml(reply, &tmpString);

    /**
     ** Send the message back.
     **/
    rr = jxta_resolver_response_new_1(self->name, tmpString, jxta_resolver_query_get_queryid(rq));

    jxta_id_to_jstring(jxta_resolver_query_get_src_peer_id(rq), &pid);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send pipe resolver response to peer %s\n", jstring_get_string(pid));
    res = jxta_resolver_service_sendResponse(self->resolver, rr, jxta_resolver_query_get_src_peer_id(rq));
  error:
    if (reply != NULL)
        JXTA_OBJECT_RELEASE(reply);
    if (rr != NULL)
        JXTA_OBJECT_RELEASE(rr);
    if (tmpString != NULL)
        JXTA_OBJECT_RELEASE(tmpString);
    if (adv != NULL)
        JXTA_OBJECT_RELEASE(adv);
    if (peers != NULL)
        JXTA_OBJECT_RELEASE(peers);
    if (pipe_resolver != NULL)
        JXTA_OBJECT_RELEASE(pipe_resolver);
    if (pipe_impl != NULL)
        JXTA_OBJECT_RELEASE(pipe_impl);
    if (pipe_service != NULL)
        JXTA_OBJECT_RELEASE(pipe_service);
    if (queryString != NULL)
        JXTA_OBJECT_RELEASE(queryString);
    if (msg != NULL)
        JXTA_OBJECT_RELEASE(msg);
    if (rq != NULL)
        JXTA_OBJECT_RELEASE(rq);
}


static void response_listener(Jxta_object * obj, void *arg)
{

    Pipe_resolver *self = (Pipe_resolver *) arg;
    ResolverResponse *rr = (ResolverResponse *) obj;
    JString *queryString = NULL;
    Jxta_piperesolver_msg *msg = NULL;
    Jxta_status res;
    char *pipeid = NULL;
    Pending_request *req = NULL;
    Jxta_id *peerid = NULL;
    Jxta_id *tmpPeerid = NULL;
    Jxta_vector *tmpVector = NULL;
    Jxta_vector *reqs = jxta_vector_new(0);
    Jxta_peer *remote_peer;
    JString *peerString = NULL;
    Jxta_vector *vector = NULL;
    int i;
    Jxta_pipe_resolver_event *event = NULL;

    JXTA_OBJECT_CHECK_VALID(rr);
    JXTA_OBJECT_CHECK_VALID(self);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "********** PIPE RESOLVER RESPONSE LISTENER *****************\n");
    jxta_resolver_response_get_response(rr, &queryString);
    msg = jxta_piperesolver_msg_new();
    jxta_piperesolver_msg_parse_charbuffer(msg,
                                           (const char *) jstring_get_string(queryString),
                                           strlen(jstring_get_string(queryString)));

    publish_peer_adv(self, msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Resolver Response:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "   msg type = %s\n", jxta_piperesolver_msg_get_MsgType(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "    pipe id = %s\n", jxta_piperesolver_msg_get_PipeId(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "  pipe type = %s\n", jxta_piperesolver_msg_get_Type(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       peer = %s\n", jxta_piperesolver_msg_get_Peer(msg));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "     status = %s\n", jxta_piperesolver_msg_get_Found(msg) == TRUE ? "Found" : "NACK");

    pipeid = jxta_piperesolver_msg_get_PipeId(msg);

    if (pipeid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid response\n");
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(rr);

        return;
    }

    peerString = jstring_new_2((const char *) jxta_piperesolver_msg_get_Peer(msg));
    res = jxta_id_from_jstring(&peerid, peerString);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot build a jstring from the peerid\n");
        JXTA_OBJECT_RELEASE(peerString);
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(rr);
        return;
    }

    JXTA_OBJECT_RELEASE(peerString);

    remote_peer = jxta_peer_new();
    jxta_peer_set_peerid(remote_peer, peerid);

    /* Invoke listeners */
    apr_thread_mutex_lock(self->mutex);
    /* Retrieve the pipe id
     * First find if there is already a pending request for that pipe */
    res = jxta_hashtable_get(self->pending_requests, (const void *) pipeid, strlen(pipeid), (Jxta_object **) & vector);

    if (res != JXTA_SUCCESS) {
        /* No request found. This is an error */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No pending request\n");
        JXTA_OBJECT_RELEASE(queryString);
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(rr);
        apr_thread_mutex_unlock(self->mutex);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(vector);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Starting loop\n");
    for (i = 0; i < jxta_vector_size(vector); ++i) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "in loop %d\n", i);
        res = jxta_vector_get_object_at(vector, (Jxta_object **) & req, i);
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
                pending_request_remove(self, req);
                JXTA_OBJECT_CHECK_VALID(req);
            }
            JXTA_OBJECT_RELEASE(tmpPeerid);
        } else {
            jxta_vector_add_object_first(reqs, (Jxta_object *) req);
            pending_request_remove(self, req);

            JXTA_OBJECT_CHECK_VALID(req);
        }

        JXTA_OBJECT_RELEASE(req);
        JXTA_OBJECT_CHECK_VALID(req);
    }

    apr_thread_mutex_unlock(self->mutex);

    /* Invoke listeners */
    for (i = 0; i < jxta_vector_size(reqs); ++i) {
        res = jxta_vector_get_object_at(reqs, (Jxta_object **) & req, i);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Cannot retrieve req from vector\n");
            continue;
        }

        JXTA_OBJECT_CHECK_VALID(req);
        tmpVector = jxta_vector_new(0);
        res = jxta_vector_add_object_first(tmpVector, (Jxta_object *) remote_peer);
        event = jxta_pipe_resolver_event_new(JXTA_PIPE_RESOLVER_RESOLVED, req->adv, tmpVector);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invoke listener req= %x\n", req);
        JXTA_OBJECT_CHECK_VALID(req->listener);
        jxta_listener_schedule_object(req->listener, (Jxta_object *) event);

        JXTA_OBJECT_RELEASE(event);
        JXTA_OBJECT_RELEASE(tmpVector);
        JXTA_OBJECT_RELEASE(req);

        JXTA_OBJECT_CHECK_VALID(req);
    }

    JXTA_OBJECT_RELEASE(reqs);
    JXTA_OBJECT_RELEASE(remote_peer);
    JXTA_OBJECT_RELEASE(queryString);
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(rr);
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

    Jxta_pipe_resolver_event *self = (Jxta_pipe_resolver_event *) malloc(sizeof(Jxta_pipe_resolver_event));
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
