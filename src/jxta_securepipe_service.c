/*
 * Copyright(c) 2006 Sun Microsystems, Inc.  All rights reserved.
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
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
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
 * $Id: jxta_securepipe_service.c,v 1.1 2007/04/24 21:51:33 slowhog Exp $
 */

#include "jxta_apr.h"
#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_id.h"
#include "jxta_service.h"
#include "jxta_peergroup.h"
#include "jxta_service_private.h"
#include "jxta_pipe_service.h"
#include "jxta_resolver_service.h"
#include "jxta_rdv_service.h"
#include "jxta_message.h"
#include "jxta_log.h"

static const char *__log_cat = "SECUREPIPE_SVC";

typedef struct {

    Jxta_pipe_service_impl generic;
    Jxta_PG *group;
    Jxta_PID *peerid;
    Jxta_cm *cm;
    Jxta_hashtable *accept_sessions;
    Jxta_pipe_resolver *pipe_resolver;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
} Jxta_securepipe_service;

typedef struct {

    Jxta_pipe generic;
    char *id;
    Jxta_pipe_adv *adv;
    Jxta_peer *dst_peer;
    Jxta_securepipe_service *securepipe_service;

} Jxta_securepipe_pipe;

typedef struct {

    Jxta_inputpipe generic;
    Jxta_securepipe_pipe *pipe;
    Jxta_listener *listener;
    Jxta_listener *user_listener;
} Jxta_securepipe_inputpipe;

typedef struct {

    Jxta_outputpipe generic;
    Jxta_securepipe_pipe *pipe;
    Jxta_endpoint_address *addr;
    Jxta_listener *user_listener;
} Jxta_securepipe_outputpipe;

typedef struct {
    Jxta_listener *listener;
    Jxta_securepipe_service *pipe_service;
} ResolveRequest;

static char *securepipe_get_name(Jxta_pipe_service_impl *);
static Jxta_status securepipe_timed_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_vector *,
                                            Jxta_pipe **);
static Jxta_status securepipe_deny(Jxta_pipe_service_impl *, Jxta_pipe_adv *);
static Jxta_status securepipe_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_vector *, Jxta_listener *);
static Jxta_status securepipe_timed_accept(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_pipe **);
static Jxta_status securepipe_add_accept_listener(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_listener *);
static Jxta_status securepipe_remove_accept_listener(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_listener *);
static Jxta_status securepipe_get_pipe_resolver(Jxta_pipe_service_impl *, Jxta_pipe_resolver **);
static Jxta_status securepipe_set_pipe_resolver(Jxta_pipe_service_impl *, Jxta_pipe_resolver *, Jxta_pipe_resolver **);
static Jxta_status securepipe_check_listener(Jxta_pipe_service_impl * service, const char *id);
static Jxta_securepipe_outputpipe *securepipe_outputpipe_new(Jxta_securepipe_pipe *);
static Jxta_securepipe_inputpipe *securepipe_inputpipe_new(Jxta_securepipe_pipe *);
static Jxta_status jxta_securepipe_pipe_connect(Jxta_securepipe_service *, const char *, Jxta_pipe_adv *, Jxta_vector *,
                                                Jxta_time_diff, Jxta_listener *);
static Jxta_status jxta_securepipe_pipe_accept(Jxta_securepipe_service *, const char *, Jxta_pipe_adv *, Jxta_listener *);
static Jxta_status securepipe_remove_accept_listener(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_listener *);

static Jxta_status securepipe_pipe_get_outputpipe(Jxta_pipe *, Jxta_outputpipe **);
static Jxta_status securepipe_pipe_get_inputpipe(Jxta_pipe *, Jxta_inputpipe **);
static Jxta_status securepipe_pipe_get_remote_peers(Jxta_pipe *, Jxta_vector **);

#define SECUREPIPE_ENDPOINT_NAME "PipeService"

static void jxta_securepipe_service_free(Jxta_object * obj)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;

    JXTA_OBJECT_RELEASE(self->cm);

    self->group = NULL;

    if (self->peerid != NULL) {
        JXTA_OBJECT_RELEASE(self->peerid);
        self->peerid = NULL;
    }

    if (self->accept_sessions != NULL) {
        JXTA_OBJECT_RELEASE(self->accept_sessions);
        self->accept_sessions = NULL;
    }

    if (self->pipe_resolver != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe_resolver);
        self->pipe_resolver = NULL;
    }

    if (self->pool != NULL) {
        apr_thread_mutex_destroy(self->mutex);
        self->mutex = NULL;

        apr_pool_destroy(self->pool);
        self->pool = NULL;
    }

    free(self);
}

Jxta_pipe_service_impl *jxta_securepipe_service_new_instance(Jxta_pipe_service * pipe_service, Jxta_PG * group)
{
    Jxta_securepipe_service *self = NULL;
    Jxta_status res;
    Jxta_id *gid = NULL;

    self = (Jxta_securepipe_service *) malloc(sizeof(Jxta_securepipe_service));
    memset(self, 0, sizeof(Jxta_securepipe_service));
    JXTA_OBJECT_INIT(self, jxta_securepipe_service_free, 0);

    self->accept_sessions = jxta_hashtable_new(0);

    if (self->accept_sessions == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate hashtable for accept sessions\n");
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }

    jxta_PG_get_PID(group, &self->peerid);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);

    if (res != APR_SUCCESS) {
        return NULL;
    }

    self->group = group;

    res = jxta_pipe_service_get_resolver(pipe_service, &self->pipe_resolver);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get the generic Pipe Service resolver\n");
        return NULL;
    }

    /**
     ** Initialize the VTBL.
     **/

    self->generic.get_name = securepipe_get_name;
    self->generic.timed_connect = securepipe_timed_connect;
    self->generic.connect = securepipe_connect;
    self->generic.timed_accept = securepipe_timed_accept;
    self->generic.deny = securepipe_deny;
    self->generic.add_accept_listener = securepipe_add_accept_listener;
    self->generic.remove_accept_listener = securepipe_remove_accept_listener;
    self->generic.get_pipe_resolver = securepipe_get_pipe_resolver;
    self->generic.set_pipe_resolver = securepipe_set_pipe_resolver;
    self->generic.check_listener = securepipe_check_listener;

    jxta_PG_get_cache_manager(group, &self->cm);

    return (Jxta_pipe_service_impl *) self;
}

static char *securepipe_get_name(Jxta_pipe_service_impl * obj)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return NULL;
    }
    return (char *) JXTA_UNICAST_SECURE_PIPE;
}

static Jxta_status securepipe_deny(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /**
     ** 3/23/2002 FIXME lomax@jxta.org
     ** The propagation pipe service does not implement this yet.
     **/
    return JXTA_NOTIMP;
}

static Jxta_status
securepipe_timed_connect(Jxta_pipe_service_impl * obj,
                         Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_listener *listener = NULL;
    Jxta_pipe_connect_event *event = NULL;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "securepipe_timed_connect\n");

    listener = jxta_listener_new(NULL, NULL, 1, 1);
    if (listener == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a listener\n");
        return JXTA_NOMEM;
    }

    jxta_listener_start(listener);

    res = jxta_securepipe_pipe_connect(self, jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv), adv, peers, timeout, listener);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "timed_connect: failed on timeout\n");

        JXTA_OBJECT_RELEASE(listener);
        return res;
    }

    res = jxta_listener_wait_for_event(listener, timeout, JXTA_OBJECT_PPTR(&event));
    JXTA_OBJECT_RELEASE(listener);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Timeout\n");
        return res;
    }

    JXTA_OBJECT_CHECK_VALID(event);

    if (jxta_pipe_connect_event_get_event(event) != JXTA_PIPE_CONNECTED) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "timed_connect: CONNECTION_FAILED event\n");
        JXTA_OBJECT_RELEASE(event);
        return JXTA_TIMEOUT;
    }

    res = jxta_pipe_connect_event_get_pipe(event, pipe);

    JXTA_OBJECT_CHECK_VALID(*pipe);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve pipe from connect event\n");
        JXTA_OBJECT_RELEASE(event);
        return JXTA_TIMEOUT;
    }

    JXTA_OBJECT_RELEASE(event);

    {
        Jxta_endpoint_service *endpoint = NULL;
        Jxta_transport *tls_transport = NULL;
        Jxta_securepipe_outputpipe *o_pipe = NULL;
        JxtaEndpointMessenger *messenger = NULL;

        jxta_PG_get_endpoint_service(self->group, &endpoint);

        JXTA_OBJECT_CHECK_VALID(endpoint);

        tls_transport = jxta_endpoint_service_lookup_transport(endpoint, "jxtatls");

        jxta_pipe_get_outputpipe(*pipe, &o_pipe);

        messenger = jxta_transport_messenger_get(tls_transport, o_pipe->addr);

        res = jxta_transport_tls_connect(tls_transport, messenger->address, timeout);

        JXTA_OBJECT_RELEASE(endpoint);
        JXTA_OBJECT_RELEASE(tls_transport);
        JXTA_OBJECT_RELEASE(o_pipe);
        JXTA_OBJECT_RELEASE(messenger);
    }

    if (res == JXTA_SUCCESS)
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "timed connect complete\n");

    return res;
}

static Jxta_status
securepipe_connect(Jxta_pipe_service_impl * obj,
                   Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    Jxta_status res = JXTA_SUCCESS;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "securepipe_connect\n");

    res = jxta_securepipe_pipe_connect(self, jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv), adv, peers, timeout, listener);

    apr_thread_mutex_unlock(self->mutex);
    return res;
}

static Jxta_status
securepipe_timed_accept(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    Jxta_listener *listener = NULL;
    Jxta_pipe_connect_event *event = NULL;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "securepipe_timed_accept\n");
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);

    res = jxta_hashtable_get(self->accept_sessions,
                             jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv),
                             strlen(jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv)), JXTA_OBJECT_PPTR(&listener));

    if (res == JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "resource is busy\n");
        JXTA_OBJECT_RELEASE(listener);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_BUSY;
    }

    listener = jxta_listener_new(NULL, NULL, 1, 1);

    if (listener == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a listener\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_NOMEM;
    }

    res = jxta_securepipe_pipe_accept(self, jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv), adv, listener);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Wait for connect_event\n");

    res = jxta_listener_wait_for_event(listener, timeout, JXTA_OBJECT_PPTR(&event));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Got connect_event\n");
    JXTA_OBJECT_RELEASE(listener);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot wait for event on the listener\n");
        return res;
    }

    if (jxta_pipe_connect_event_get_event(event) != JXTA_PIPE_CONNECTED) {
        JXTA_OBJECT_RELEASE(event);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Connect timeout\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_TIMEOUT;
    }

    res = jxta_pipe_connect_event_get_pipe(event, pipe);
    JXTA_OBJECT_RELEASE(event);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve pipe from connect event\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_TIMEOUT;
    }

    apr_thread_mutex_unlock(self->mutex);

    return res;
}

static Jxta_status securepipe_add_accept_listener(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "securepipe_timed_accept_listener\n");
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);

    res = jxta_hashtable_get(self->accept_sessions,
                             jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv),
                             strlen(jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv)), JXTA_OBJECT_PPTR(&listener));

    if (res == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(listener);
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_BUSY;
    }

    res = jxta_securepipe_pipe_accept(self, jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv), adv, listener);

    apr_thread_mutex_unlock(self->mutex);
    return res;
}

static Jxta_status securepipe_remove_accept_listener(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /** FIX ME lomax@jxta.org 3/23/2002
     ** The propagation pipe service does not implement this yet.
     **/
    return JXTA_NOTIMP;
}

static Jxta_status securepipe_get_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver ** resolver)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (self->pipe_resolver != NULL) {
        JXTA_OBJECT_SHARE(self->pipe_resolver);
    }
    *resolver = (Jxta_pipe_resolver *) self->pipe_resolver;

    return JXTA_SUCCESS;
}

static Jxta_status securepipe_set_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver * new, Jxta_pipe_resolver ** old)
{

    Jxta_securepipe_service *self = (Jxta_securepipe_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (self->pipe_resolver != NULL) {
        if (*old != NULL) {
            *old = self->pipe_resolver;
        } else {
            JXTA_OBJECT_RELEASE(self->pipe_resolver);
        }
    }
    JXTA_OBJECT_SHARE(new);
    self->pipe_resolver = new;

    return JXTA_SUCCESS;
}

static Jxta_status securepipe_check_listener(Jxta_pipe_service_impl * impl, const char *id)
{
    Jxta_securepipe_service *self = (Jxta_securepipe_service *) impl;
    Jxta_status res;
    Jxta_object *tmp = NULL;

    apr_thread_mutex_lock(self->mutex);

    res = jxta_hashtable_get(self->accept_sessions, id, strlen(id), &tmp);

    apr_thread_mutex_unlock(self->mutex);

    if (res == JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found a listener locally for pipe id %s\n", id);
        JXTA_OBJECT_RELEASE(tmp);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No local listener for pipe id %s\n", id);
    }

    return res;
}

/****************************************************************
 ** Jxta_securepipe_pipe implementation
 ****************************************************************/

static void jxta_securepipe_pipe_free(Jxta_object * obj)
{

    Jxta_securepipe_pipe *self = (Jxta_securepipe_pipe *) obj;

    if (self->securepipe_service != NULL) {
        JXTA_OBJECT_RELEASE(self->securepipe_service);
        self->securepipe_service = NULL;
    }

    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }

    if (self->dst_peer != NULL) {
        JXTA_OBJECT_RELEASE(self->dst_peer);
        self->dst_peer = NULL;
    }

    if (self->id != NULL) {
        free(self->id);
        self->id = NULL;
    }

    free(self);
}

static Jxta_securepipe_pipe *jxta_securepipe_pipe_new(const char *id, Jxta_pipe_adv * adv, Jxta_securepipe_service * pipe_service)
{

    Jxta_securepipe_pipe *self = NULL;

    self = (Jxta_securepipe_pipe *) malloc(sizeof(Jxta_securepipe_pipe));
    memset(self, 0, sizeof(Jxta_securepipe_pipe));
    JXTA_OBJECT_INIT(self, jxta_securepipe_pipe_free, 0);


    self->id = malloc(strlen(id) + 1);
    strcpy(self->id, id);

    JXTA_OBJECT_SHARE(pipe_service);
    self->securepipe_service = pipe_service;

    JXTA_OBJECT_SHARE(adv);
    self->adv = adv;

    /**
     ** Initialize the VTBL.
     **/

    self->generic.get_outputpipe = securepipe_pipe_get_outputpipe;
    self->generic.get_inputpipe = securepipe_pipe_get_inputpipe;
    self->generic.get_remote_peers = securepipe_pipe_get_remote_peers;

    return self;
}

static void JXTA_STDCALL resolve_listener(Jxta_object * obj, void *arg)
{

    Jxta_pipe_resolver_event *event = (Jxta_pipe_resolver_event *) obj;
    ResolveRequest *req = (ResolveRequest *) arg;
    Jxta_securepipe_pipe *pipe = NULL;
    Jxta_peer *peer_tmp = NULL;
    Jxta_pipe_connect_event *connect_event = NULL;
    Jxta_vector *peers = NULL;
    Jxta_status res;
    Jxta_pipe_adv *adv = NULL;
    Jxta_listener *tmpListener = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "resolve_listener event= %x req= %x\n", event, req);

    JXTA_OBJECT_CHECK_VALID(event);

    if ((event == NULL) || (req == NULL)) {
        return;
    }

    if (jxta_pipe_resolver_event_get_event(event) != JXTA_PIPE_RESOLVER_RESOLVED) {
        goto error;
    }

    res = jxta_pipe_resolver_event_get_peers(event, &peers);
    if (res != JXTA_SUCCESS) {
        goto error;
    }

    JXTA_OBJECT_CHECK_VALID(peers);

    res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer_tmp), 0);

    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(peers);
        goto error;
    }

    JXTA_OBJECT_CHECK_VALID(peer_tmp);

    res = jxta_pipe_resolver_event_get_adv(event, &adv);

    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(peer_tmp);
        JXTA_OBJECT_RELEASE(peers);

        JXTA_OBJECT_CHECK_VALID(peers);
        goto error;
    }

    pipe = jxta_securepipe_pipe_new(jxta_pipe_adv_get_Id(adv), adv, req->pipe_service);

    if (pipe == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate pipe\n");
        JXTA_OBJECT_RELEASE(peer_tmp);
        JXTA_OBJECT_RELEASE(peers);
        JXTA_OBJECT_RELEASE(adv);

        JXTA_OBJECT_CHECK_VALID(peers);
        goto error;
    }

    pipe->dst_peer = peer_tmp;

    connect_event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTED, (Jxta_pipe *) pipe);

    if (connect_event == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a connect_event\n");
        JXTA_OBJECT_RELEASE(peer_tmp);
        JXTA_OBJECT_RELEASE(peers);
        JXTA_OBJECT_RELEASE(adv);
        JXTA_OBJECT_RELEASE(pipe);

        JXTA_OBJECT_CHECK_VALID(peers);
        goto error;
    }

    JXTA_OBJECT_RELEASE(peers);
    JXTA_OBJECT_RELEASE(adv);
    JXTA_OBJECT_RELEASE(pipe);

  error:
    if (connect_event == NULL) {
        connect_event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTION_FAILED, NULL);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invoking the listener %x\n", req->listener);

    tmpListener = req->listener;

    JXTA_OBJECT_CHECK_VALID(req->listener);
    JXTA_OBJECT_CHECK_VALID(connect_event);

    res = jxta_listener_schedule_object(req->listener, (Jxta_object *) connect_event);

    JXTA_OBJECT_RELEASE(connect_event);
    if (req->listener)
        JXTA_OBJECT_RELEASE(req->listener);

    free(req);
    return;
}

static Jxta_status
jxta_securepipe_pipe_connect(Jxta_securepipe_service * pipe_service, const char *id, Jxta_pipe_adv * adv,
                             Jxta_vector * peers, Jxta_time_diff timeout, Jxta_listener * listener)
{

    Jxta_securepipe_pipe *self = NULL;
    Jxta_status res;
    Jxta_peer *peer = NULL;
    Jxta_listener *myListener = NULL;
    ResolveRequest *req = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "pipe_connect\n");
    if ((id == NULL) || (listener == NULL) || (pipe_service == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "pipe_connect failed\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /**
     ** Resolve the pipe. If a peer has been added into "peers", the
     ** the resolve request must be sent there. Otherwise, first look
     ** locally, if no pipe is resolved, then use the generic pipe serive
     ** pipe resolver to resolve remotely.
     **/

    if (peers != NULL) {
        if (jxta_vector_size(peers) != 1) {
            /* This implementation only support one peer in the list */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Number of peers in peer list must be one\n");
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "pipe_connect failed\n");
            return JXTA_INVALID_ARGUMENT;
        }
        res = jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), 0);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retrieve peer from vector\n");
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "pipe_connect failed\n");
            return JXTA_INVALID_ARGUMENT;
        }
    }

    if (peer == NULL) {
        Jxta_object *tmp = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Start to localize\n");
        /* First try to resolve locally */
        apr_thread_mutex_lock(pipe_service->mutex);

        res = jxta_hashtable_get(pipe_service->accept_sessions, id, strlen(id), &tmp);

        apr_thread_mutex_unlock(pipe_service->mutex);

        if (res == JXTA_SUCCESS) {
            Jxta_peer *peer_tmp = NULL;
            Jxta_pipe_connect_event *event = NULL;

            JXTA_OBJECT_RELEASE(tmp);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Local listener\n");
            /* There is a local listener */
            peer_tmp = jxta_peer_new();
            if (peer_tmp == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a jxta_peer\n");
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "pipe_connect failed\n");
                return JXTA_NOMEM;
            }

            jxta_peer_set_peerid(peer_tmp, pipe_service->peerid);
            self = jxta_securepipe_pipe_new(id, adv, pipe_service);
            if (self == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate pipe\n");
                return JXTA_NOMEM;
            }
            self->dst_peer = peer_tmp;

            event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTED, (Jxta_pipe *) self);
            JXTA_OBJECT_RELEASE(self);

            if (event == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a connect_event\n");
                return JXTA_NOMEM;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invoking the listener\n");
            res = jxta_listener_schedule_object(listener, (Jxta_object *) event);

            JXTA_OBJECT_RELEASE(event);

            return JXTA_SUCCESS;
        }
    }

    /**
     ** Try remote
     **/

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Start remote localize\n");

    req = (ResolveRequest *) malloc(sizeof(ResolveRequest));
    req->listener = JXTA_OBJECT_SHARE(listener);
    req->pipe_service = pipe_service;

    myListener = jxta_listener_new(resolve_listener, (void *) req, 0, 0);
    jxta_listener_start(myListener);

    res = jxta_pipe_resolver_remote_resolve(pipe_service->pipe_resolver, adv, timeout, peer, myListener);
    /**
     ** We release the oject right now: the listener will be destroyed
     ** when it will be called(by the pipe resolver itself).
     **/
    JXTA_OBJECT_RELEASE(myListener);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "pipe_connect failed\n");
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remote resolve\n");
        return res;
    }

    return JXTA_SUCCESS;
}

static Jxta_status
jxta_securepipe_pipe_accept(Jxta_securepipe_service * pipe_service, const char *id, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_securepipe_pipe *self = NULL;
    Jxta_pipe_connect_event *event = NULL;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "pipe_accept starts\n");
    if ((id == NULL) || (listener == NULL) || (pipe_service == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid arguments\n");
        return JXTA_INVALID_ARGUMENT;
    }

    self = jxta_securepipe_pipe_new(id, adv, pipe_service);
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate pipe\n");
        return JXTA_NOMEM;
    }

    jxta_listener_start(listener);

    event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTED, (Jxta_pipe *) self);

    if (event == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a connect_event\n");
        JXTA_OBJECT_RELEASE(self);
        return JXTA_NOMEM;
    }

    res = jxta_listener_schedule_object(listener, (Jxta_object *) event);
    JXTA_OBJECT_RELEASE(event);
    return res;
}


static Jxta_status securepipe_pipe_get_outputpipe(Jxta_pipe * obj, Jxta_outputpipe ** op)
{

    Jxta_securepipe_pipe *self = (Jxta_securepipe_pipe *) obj;
    Jxta_outputpipe *tmp;

    if ((self == NULL) || (op == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    tmp = (Jxta_outputpipe *) securepipe_outputpipe_new(self);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed\n");
        return JXTA_NOMEM;
    }

    *op = tmp;
    return JXTA_SUCCESS;
}

static Jxta_status securepipe_pipe_get_inputpipe(Jxta_pipe * obj, Jxta_inputpipe ** ip)
{

    Jxta_securepipe_pipe *self = (Jxta_securepipe_pipe *) obj;
    Jxta_inputpipe *tmp;

    if ((self == NULL) || (ip == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    tmp = (Jxta_inputpipe *) securepipe_inputpipe_new(self);

    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed\n");
        return JXTA_NOMEM;
    }

    *ip = tmp;
    return JXTA_SUCCESS;
}

static Jxta_status securepipe_pipe_get_remote_peers(Jxta_pipe * self, Jxta_vector ** vector)
{
    /**
     ** The propagated pipes(securepipe pipe) does no implement this
     ** feature.
     **/
    return JXTA_NOTIMP;
}

/****************************************************************
 ** Jxta_securepipe_inputpipe implementation
 ****************************************************************/

static Jxta_status inputpipe_timed_receive(Jxta_inputpipe * obj, Jxta_time_diff timeout, Jxta_message ** msg)
{

    Jxta_securepipe_inputpipe *self = (Jxta_securepipe_inputpipe *) obj;
    Jxta_listener *listener = NULL;
    Jxta_endpoint_service *endpoint = NULL;
    Jxta_status res;

    if (self->user_listener != NULL) {
        return JXTA_BUSY;
    }

    if (self->listener == NULL) {
        /**
         ** Create our listener and connect it to the securepipe service.
         **/
        listener = jxta_listener_new(NULL, (void *) self, 0, 200);
        if (listener == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create listener\n");
            return JXTA_NOMEM;
        }

        self->listener = listener;

        jxta_PG_get_endpoint_service(self->pipe->securepipe_service->group, &endpoint);

        res = jxta_endpoint_service_add_listener(endpoint, (char *) SECUREPIPE_ENDPOINT_NAME, self->pipe->id, listener);
        JXTA_OBJECT_RELEASE(endpoint);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot add endpoint listener\n");
            JXTA_OBJECT_RELEASE(listener);
            self->listener = NULL;
            return res;
        }

        jxta_listener_start(listener);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting for incoming message\n");
    return jxta_listener_wait_for_event(self->listener, timeout, (Jxta_object **) msg);
}

static Jxta_status inputpipe_add_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_securepipe_inputpipe *self = (Jxta_securepipe_inputpipe *) obj;
    Jxta_status res;
    Jxta_endpoint_service *endpoint = NULL;
    Jxta_securepipe_service *pipe_service = self->pipe->securepipe_service;
    Jxta_pipe_adv *pipe_adv = self->pipe->adv;
    Jxta_pipe_resolver *pipe_resolver = self->pipe->securepipe_service->pipe_resolver;
    Jxta_object *tmp = NULL;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(listener);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "add listener starts\n");
    if ((self->user_listener != NULL) || (self->listener != NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "busy\n");
        return JXTA_BUSY;
    }

    apr_thread_mutex_lock(pipe_service->mutex);

    res = jxta_hashtable_get(pipe_service->accept_sessions, self->pipe->id, strlen(self->pipe->id), JXTA_OBJECT_PPTR(&tmp));

    if (res == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(tmp);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "inputpipe is busy\n");
        apr_thread_mutex_unlock(pipe_service->mutex);
        return JXTA_BUSY;
    }

    JXTA_OBJECT_SHARE(listener);
    self->user_listener = listener;

    jxta_PG_get_endpoint_service(pipe_service->group, &endpoint);

    res = jxta_endpoint_service_add_listener(endpoint, (char *) SECUREPIPE_ENDPOINT_NAME, self->pipe->id, listener);
    JXTA_OBJECT_RELEASE(endpoint);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot add endpoint listener\n");
        JXTA_OBJECT_RELEASE(listener);
        self->user_listener = NULL;
        apr_thread_mutex_unlock(pipe_service->mutex);
        return res;
    }

    jxta_hashtable_put(pipe_service->accept_sessions, self->pipe->id, strlen(self->pipe->id), (Jxta_object *) listener);

    res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, pipe_adv, NULL, TRUE);
    apr_thread_mutex_unlock(pipe_service->mutex);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send Pipe SrdiMessage\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "inputpipe_add_listener: Pipe SrdiMessage Sent\n");
    }
    return JXTA_SUCCESS;
}

static Jxta_status inputpipe_remove_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_securepipe_inputpipe *self = (Jxta_securepipe_inputpipe *) obj;
    Jxta_endpoint_service *endpoint = NULL;
    Jxta_status res;
    Jxta_pipe_adv *pipe_adv = self->pipe->adv;
    Jxta_pipe_resolver *pipe_resolver = self->pipe->securepipe_service->pipe_resolver;

    if (self->user_listener != listener) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_PG_get_endpoint_service(self->pipe->securepipe_service->group, &endpoint);

    apr_thread_mutex_lock(self->pipe->securepipe_service->mutex);
    res = jxta_endpoint_service_remove_listener(endpoint, (char *) SECUREPIPE_ENDPOINT_NAME, self->pipe->id);
    JXTA_OBJECT_RELEASE(endpoint);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remove endpoint listener\n");
    }

    res = jxta_hashtable_del(self->pipe->securepipe_service->accept_sessions, self->pipe->id, strlen(self->pipe->id), NULL);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remove listener from accept_session\n");
    }

    JXTA_OBJECT_RELEASE(self->user_listener);
    self->user_listener = NULL;
    res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, pipe_adv, NULL, FALSE);
    apr_thread_mutex_unlock(self->pipe->securepipe_service->mutex);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send Pipe SrdiMessage\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "inputpipe_remove_listener: Pipe SrdiMessage Sent\n");
    }
    return JXTA_SUCCESS;
}

static void securepipe_inputpipe_free(Jxta_object * obj)
{

    Jxta_securepipe_inputpipe *self = (Jxta_securepipe_inputpipe *) obj;
    Jxta_status res;
    Jxta_endpoint_service *endpoint = NULL;

    jxta_PG_get_endpoint_service(self->pipe->securepipe_service->group, &endpoint);
    res = jxta_endpoint_service_remove_listener(endpoint, (char *) SECUREPIPE_ENDPOINT_NAME, self->pipe->id);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remove endpoint listener\n");
    }
    JXTA_OBJECT_RELEASE(endpoint);

    if (self->listener != NULL) {
        JXTA_OBJECT_RELEASE(self->listener);
        self->listener = NULL;
    }

    if (self->user_listener != NULL) {
        inputpipe_remove_listener((Jxta_inputpipe *) self, self->user_listener);
    }

    if (self->pipe != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe);
        self->pipe = NULL;
    }

    free(self);
}

static Jxta_securepipe_inputpipe *securepipe_inputpipe_new(Jxta_securepipe_pipe * pipe)
{

    Jxta_securepipe_inputpipe *self = NULL;

    self = (Jxta_securepipe_inputpipe *) malloc(sizeof(Jxta_securepipe_inputpipe));
    memset(self, 0, sizeof(Jxta_securepipe_inputpipe));
    JXTA_OBJECT_INIT(self, securepipe_inputpipe_free, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "securepipe_inputpipe_new\n");
    JXTA_OBJECT_SHARE(pipe);
    self->pipe = pipe;
    self->listener = NULL;
    self->user_listener = NULL;

    /**
     ** Initialize the VTBL.
     **/

    self->generic.timed_receive = inputpipe_timed_receive;
    self->generic.add_listener = inputpipe_add_listener;
    self->generic.remove_listener = inputpipe_remove_listener;

    return (Jxta_securepipe_inputpipe *) self;
}

/****************************************************************
 ** Jxta_securepipe_outputpipe implementation
 ****************************************************************/

static Jxta_status outputpipe_add_listener(Jxta_outputpipe * obj, Jxta_listener * listener)
{

    /**
     ** The securepipe service does not implement this. (no meaning).
     **/
    return JXTA_NOTIMP;
}

static Jxta_status outputpipe_remove_listener(Jxta_outputpipe * obj, Jxta_listener * listener)
{

    /**
     ** The securepipe service does not implement this. (no meaning).
     **/
    return JXTA_NOTIMP;
}

static Jxta_status outputpipe_send(Jxta_outputpipe * obj, Jxta_message * msg)
{

    Jxta_securepipe_outputpipe *self = (Jxta_securepipe_outputpipe *) obj;
    Jxta_endpoint_service *endpoint = NULL;
    Jxta_status res = JXTA_SUCCESS;

    Jxta_transport *tls_transport;

    JxtaEndpointMessenger *messenger;

    JXTA_OBJECT_CHECK_VALID(obj);
    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "securepipe_outputpipe_send\n");

    if (self->addr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "outputpipe_send: no destination peer. Message is dropped.\n");
        return JXTA_FAILED;
    }

    JXTA_OBJECT_CHECK_VALID(self->addr);

    jxta_PG_get_endpoint_service(self->pipe->securepipe_service->group, &endpoint);

    JXTA_OBJECT_CHECK_VALID(endpoint);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending unicast pipe message to %s://%s/%s/%s\n",
                    jxta_endpoint_address_get_protocol_name(self->addr),
                    jxta_endpoint_address_get_protocol_address(self->addr),
                    jxta_endpoint_address_get_service_name(self->addr), jxta_endpoint_address_get_service_params(self->addr));

    tls_transport = jxta_endpoint_service_lookup_transport(endpoint, "jxtatls");

    messenger = jxta_transport_messenger_get(tls_transport, self->addr);

    messenger->jxta_send(messenger, msg);

    JXTA_OBJECT_RELEASE(endpoint);
    JXTA_OBJECT_RELEASE(messenger);
    JXTA_OBJECT_RELEASE(tls_transport);

    return res;
}

static void securepipe_outputpipe_free(Jxta_object * obj)
{

    Jxta_securepipe_outputpipe *self = (Jxta_securepipe_outputpipe *) obj;

    if (self->pipe != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe);
        self->pipe = NULL;
    }

    if (self->user_listener != NULL) {
        JXTA_OBJECT_RELEASE(self->user_listener);
        self->user_listener = NULL;
    }

    if (self->addr != NULL) {
        JXTA_OBJECT_RELEASE(self->addr);
        self->addr = NULL;
    }

    free(self);
}

static Jxta_securepipe_outputpipe *securepipe_outputpipe_new(Jxta_securepipe_pipe * pipe)
{

    Jxta_securepipe_outputpipe *self = NULL;
    Jxta_peer *peer = NULL;
    Jxta_id *pid = NULL;
    JString *string = NULL;
    Jxta_endpoint_address *addr = NULL;

    self = (Jxta_securepipe_outputpipe *) calloc(1, sizeof(Jxta_securepipe_outputpipe));

    self->user_listener = NULL;
    JXTA_OBJECT_SHARE(pipe);
    self->pipe = pipe;

    /* Build the destination endpoint address */
    peer = pipe->dst_peer;
    if (peer == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "outputpipe_new: no destination peer.\n");
        JXTA_OBJECT_RELEASE(pipe);
        free(self);
        return NULL;
    }

    jxta_peer_get_peerid(peer, &pid);
    if (pid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot retried peer id from jxta_peer. Message discarded\n");
        JXTA_OBJECT_RELEASE(pipe);
        free(self);
        return NULL;
    }
    jxta_id_get_uniqueportion(pid, &string);
    JXTA_OBJECT_RELEASE(pid);

    /**
     ** The following cast are necessary because jxta_endpoint_address_new_2()
     ** takes char*, even if it's treated as const char*. Until the endpoint_address
     ** API is fixed, the following cast are necessary in order to avoid a compile
     ** warning. It is safe, however.
     **/
    addr = jxta_endpoint_address_new_2((char *) "jxtatls",
                                       (char *) jstring_get_string(string), (char *) SECUREPIPE_ENDPOINT_NAME, pipe->id);
    JXTA_OBJECT_RELEASE(string);

    if (addr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a new endpoint address\n");
        JXTA_OBJECT_RELEASE(pipe);
        free(self);
        return NULL;
    }

    self->addr = addr;

    /**
     ** Initialize the VTBL.
     **/

    self->generic.send = outputpipe_send;
    self->generic.add_listener = outputpipe_add_listener;
    self->generic.remove_listener = outputpipe_remove_listener;

    JXTA_OBJECT_INIT(self, securepipe_outputpipe_free, 0);
    return (Jxta_securepipe_outputpipe *) self;
}

/* vim: set ts=4 sw=4 tw=130 et: */
