/*
 * Copyright(c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_bidipipe.c,v 1.19 2006/02/15 01:09:38 slowhog Exp $
 */

#include <stdlib.h>
#include <assert.h>

#include "jxta_log.h"
#include "jxta_bidipipe.h"

#include "jxta_peergroup.h"
#include "jxta_apr.h"

#define JXTA_BIDIPIPE_LOG "JxtaBidiPipe"

#define JXTA_BIDIPIPE_NAMESPACE          "JXTABIP"
#define JXTA_BIDIPIPE_CRED_TAG           "Cred"
#define JXTA_BIDIPIPE_REQPIPE_TAG        "reqPipe"
#define JXTA_BIDIPIPE_REMPEER_TAG        "remPeer"
#define JXTA_BIDIPIPE_REMPIPE_TAG        "remPipe"
#define JXTA_BIDIPIPE_CLOSE_TAG          "close"
#define JXTA_BIDIPIPE_RELIABLE_TAG       "reliable"

#define MIME_TYPE_XMLUTF8                "text/xml;charset=\"UTF8\""
#define MIME_TYPE_TEXT                   "text/plain"

/* Helper macros to track mutext lock */
#if 0
#ifdef jpr_thread_mutex_lock
#undef jpr_thread_mutex_lock
#endif
#define jpr_thread_mutex_lock(x) \
    printf(">> Trying to lock mutex at %s %d, TID: %p, Mutex: %p\n", __FILE__, __LINE__, apr_os_thread_current(), x); \
    apr_thread_mutex_lock(x); \
    printf(">> Mutex locked at %s %d, TID: %p, Mutex: %p\n", __FILE__, __LINE__, apr_os_thread_current(), x)

#ifdef jpr_thread_mutex_unlock
#undef jpr_thread_mutex_unlock
#endif
#define jpr_thread_mutex_unlock(x) \
    printf(">> Trying to unlock mutex at %s %d, TID: %p, Mutex: %p\n", __FILE__, __LINE__, apr_os_thread_current(), x); \
    apr_thread_mutex_unlock(x) ;\
    printf(">> Mutex unlocked at %s %d, TID: %p, Mutex: %p\n", __FILE__, __LINE__, apr_os_thread_current(), x)
#endif

struct Jxta_bidipipe {
    Jxta_PG *group;
    Jxta_endpoint_service *ep_svc;
    Jxta_listener *i_listener;

    Jxta_pipe_adv *i_pipe_adv;
    Jxta_inputpipe *i_pipe;
    Jxta_listener *listener;

    Jxta_endpoint_address *ep_addr;

    jpr_thread_mutex_t *mutex;
    jpr_thread_mutex_t *cond_mutex;
    apr_thread_cond_t *cond;
    apr_pool_t *pool;
    int state;
};

/* To create a pipe adv from another pipe adv */
static Jxta_status construct_local_pipe_adv(Jxta_pipe_adv ** local_adv, Jxta_PG * pg, Jxta_pipe_adv * remote_adv)
{
    Jxta_status rv;
    Jxta_id *pg_id;
    Jxta_id *local_pipe_id;
    JString *tmp;

    jxta_PG_get_GID(pg, &pg_id);

    rv = jxta_id_pipeid_new_1(&local_pipe_id, pg_id);
    JXTA_OBJECT_RELEASE(pg_id);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    rv = jxta_id_to_jstring(local_pipe_id, &tmp);
    JXTA_OBJECT_RELEASE(local_pipe_id);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    *local_adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(*local_adv, jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);

    jxta_pipe_adv_set_Name(*local_adv, jxta_pipe_adv_get_Name(remote_adv));
    jxta_pipe_adv_set_Type(*local_adv, jxta_pipe_adv_get_Type(remote_adv));

    return JXTA_SUCCESS;
}

static Jxta_status construct_open_message(Jxta_bidipipe * self, Jxta_message ** msg, Jxta_pipe_adv * local_adv)
{
    Jxta_status rv;
    Jxta_PA *peer_adv;
    Jxta_message_element *e;
    JString *tmp;

    jxta_PG_get_configadv(self->group, &peer_adv);
    if (NULL == peer_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Peer advertisement is not configured\n");
        return JXTA_NOT_CONFIGURED;
    }

    *msg = jxta_message_new();
    if (NULL == *msg) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(peer_adv);
        return JXTA_NOMEM;
    }

    rv = jxta_pipe_adv_get_xml(local_adv, &tmp);

    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REQPIPE_TAG, MIME_TYPE_XMLUTF8, jstring_get_string(tmp),
                                   jstring_length(tmp), NULL);
    jxta_message_add_element(*msg, e);
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(e);

    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_RELIABLE_TAG, MIME_TYPE_TEXT, "false", 5, NULL);
    jxta_message_add_element(*msg, e);
    JXTA_OBJECT_RELEASE(e);

    rv = jxta_PA_get_xml(peer_adv, &tmp);
    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPEER_TAG, MIME_TYPE_XMLUTF8, jstring_get_string(tmp),
                                   jstring_length(tmp), NULL);
    jxta_message_add_element(*msg, e);
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(e);

    JXTA_OBJECT_RELEASE(peer_adv);

    return JXTA_SUCCESS;
}

static Jxta_status handle_open_message(Jxta_bidipipe * self, Jxta_message * msg)
{
    Jxta_status rv;
    Jxta_message_element *e;
    Jxta_bytevector *val;
    JString *s;
    Jxta_PA *peer_adv;
    Jxta_pipe_adv *pipe_adv;
    Jxta_id *peer_id;

    rv = jxta_message_get_element_2(msg, JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REQPIPE_TAG, &e);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Unexpected message received, discard message\n");
        return rv;
    }

    val = jxta_message_element_get_value(e);
    JXTA_OBJECT_RELEASE(e);
    s = jstring_new_3(val);
    JXTA_OBJECT_RELEASE(val);

    if (NULL == s) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }

    pipe_adv = jxta_pipe_adv_new();
    if (NULL == pipe_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(s);
        return JXTA_NOMEM;
    }

    jxta_pipe_adv_parse_charbuffer(pipe_adv, jstring_get_string(s), jstring_length(s));
    JXTA_OBJECT_RELEASE(s);

    rv = jxta_message_get_element_2(msg, JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPEER_TAG, &e);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Invalid response message received, discard message\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        return rv;
    }

    val = jxta_message_element_get_value(e);
    JXTA_OBJECT_RELEASE(e);
    s = jstring_new_3(val);
    JXTA_OBJECT_RELEASE(val);

    if (NULL == s) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        return JXTA_NOMEM;
    }

    peer_adv = jxta_PA_new();
    if (NULL == peer_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        JXTA_OBJECT_RELEASE(s);
        return JXTA_NOMEM;
    }

    jxta_PA_parse_charbuffer(peer_adv, jstring_get_string(s), jstring_length(s));
    JXTA_OBJECT_RELEASE(s);

    peer_id = jxta_PA_get_PID(peer_adv);
    jxta_id_get_uniqueportion(peer_id, &s);
    self->ep_addr = jxta_endpoint_address_new_2("jxta", jstring_get_string(s), "PipeService", jxta_pipe_adv_get_Id(pipe_adv));

    JXTA_OBJECT_RELEASE(peer_adv);
    JXTA_OBJECT_RELEASE(peer_id);
    JXTA_OBJECT_RELEASE(s);
    JXTA_OBJECT_RELEASE(pipe_adv);

    return JXTA_SUCCESS;
}

static Jxta_status construct_response_message(Jxta_bidipipe * self, Jxta_message ** msg, Jxta_pipe_adv * local_adv)
{
    Jxta_status rv;
    Jxta_PA *peer_adv;
    Jxta_message_element *e;
    JString *tmp;

    jxta_PG_get_configadv(self->group, &peer_adv);
    if (NULL == peer_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Peer advertisement is not configured\n");
        return JXTA_NOT_CONFIGURED;
    }

    *msg = jxta_message_new();
    if (NULL == *msg) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(peer_adv);
        return JXTA_NOMEM;
    }

    rv = jxta_pipe_adv_get_xml(local_adv, &tmp);

    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPIPE_TAG, MIME_TYPE_XMLUTF8, jstring_get_string(tmp),
                                   jstring_length(tmp), NULL);
    jxta_message_add_element(*msg, e);
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(e);

    rv = jxta_PA_get_xml(peer_adv, &tmp);
    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPEER_TAG, MIME_TYPE_XMLUTF8, jstring_get_string(tmp),
                                   jstring_length(tmp), NULL);
    jxta_message_add_element(*msg, e);
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(e);

    JXTA_OBJECT_RELEASE(peer_adv);

    return JXTA_SUCCESS;
}

static Jxta_status handle_response_message(Jxta_bidipipe * self, Jxta_message * msg)
{
    Jxta_status rv;
    Jxta_message_element *e;
    Jxta_bytevector *val;
    JString *s;
    Jxta_PA *peer_adv;
    Jxta_pipe_adv *pipe_adv;
    Jxta_id *peer_id;

    rv = jxta_message_get_element_2(msg, JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPIPE_TAG, &e);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Unexpected message received, discard message\n");
        return rv;
    }

    val = jxta_message_element_get_value(e);
    JXTA_OBJECT_RELEASE(e);
    s = jstring_new_3(val);
    JXTA_OBJECT_RELEASE(val);

    if (NULL == s) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }

    pipe_adv = jxta_pipe_adv_new();
    if (NULL == pipe_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(s);
        return JXTA_NOMEM;
    }

    jxta_pipe_adv_parse_charbuffer(pipe_adv, jstring_get_string(s), jstring_length(s));
    JXTA_OBJECT_RELEASE(s);

    rv = jxta_message_get_element_2(msg, JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_REMPEER_TAG, &e);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Invalid response message received, discard message\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        return rv;
    }

    val = jxta_message_element_get_value(e);
    JXTA_OBJECT_RELEASE(e);
    s = jstring_new_3(val);
    JXTA_OBJECT_RELEASE(val);

    if (NULL == s) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        return JXTA_NOMEM;
    }

    peer_adv = jxta_PA_new();
    if (NULL == peer_adv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(pipe_adv);
        JXTA_OBJECT_RELEASE(s);
        return JXTA_NOMEM;
    }

    jxta_PA_parse_charbuffer(peer_adv, jstring_get_string(s), jstring_length(s));
    JXTA_OBJECT_RELEASE(s);

    peer_id = jxta_PA_get_PID(peer_adv);
    jxta_id_get_uniqueportion(peer_id, &s);
    self->ep_addr = jxta_endpoint_address_new_2("jxta", jstring_get_string(s), "PipeService", jxta_pipe_adv_get_Id(pipe_adv));

    JXTA_OBJECT_RELEASE(peer_adv);
    JXTA_OBJECT_RELEASE(peer_id);
    JXTA_OBJECT_RELEASE(s);
    JXTA_OBJECT_RELEASE(pipe_adv);

    return JXTA_SUCCESS;
}

static Jxta_status close_pipes(Jxta_bidipipe * self)
{
    if (JXTA_BIDIPIPE_CLOSING != self->state && JXTA_BIDIPIPE_CONNECTING != self->state) {
        return JXTA_VIOLATION;
    }

    jpr_thread_mutex_lock(self->mutex);

    JXTA_OBJECT_RELEASE(self->listener);
    self->listener = NULL;

    jxta_listener_stop(self->i_listener);

    JXTA_OBJECT_RELEASE(self->i_pipe);
    self->i_pipe = NULL;

    JXTA_OBJECT_RELEASE(self->i_pipe_adv);
    self->i_pipe_adv = NULL;

    if (JXTA_BIDIPIPE_CLOSING == self->state) {
        JXTA_OBJECT_RELEASE(self->ep_addr);
        self->ep_addr = NULL;
    }

    self->state = JXTA_BIDIPIPE_CLOSED;

    jpr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

static void JXTA_STDCALL bidipipe_input_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message *open_msg;
    Jxta_bidipipe *self = (Jxta_bidipipe *) arg;
    Jxta_message_element *e;
    Jxta_status rv;

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Message received by BiDi pipe listener\n");

    jpr_thread_mutex_lock(self->mutex);
    switch (self->state) {
    case JXTA_BIDIPIPE_CONNECTED:
        if (JXTA_SUCCESS == jxta_message_get_element_2(msg, JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_CLOSE_TAG, &e)) {
            jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Close request received, closing connection ...\n");
            self->state = JXTA_BIDIPIPE_CLOSING;
            jxta_listener_schedule_object(self->listener, NULL);
            rv = close_pipes(self);
            jpr_thread_mutex_unlock(self->mutex);
            JXTA_OBJECT_RELEASE(e);
            return;
        }
        jpr_thread_mutex_unlock(self->mutex);

        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Schedule regular message to pipe owner\n");
        assert(NULL != self->listener);
        jxta_listener_schedule_object(self->listener, obj);
        break;

    case JXTA_BIDIPIPE_CONNECTING:
        if (JXTA_SUCCESS != handle_response_message(self, msg)) {
            jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Invalid response to open request, close connection\n");
            close_pipes(self);
            JXTA_OBJECT_RELEASE(self->listener);
            self->listener = NULL;
        } else {
            jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE,
                            "Response to open request received, connection established\n");
            self->state = JXTA_BIDIPIPE_CONNECTED;
            jpr_thread_mutex_lock(self->cond_mutex);
            apr_thread_cond_signal(self->cond);
            jpr_thread_mutex_unlock(self->cond_mutex);
        }
        jpr_thread_mutex_unlock(self->mutex);
        break;

    case JXTA_BIDIPIPE_ACCEPTING:
        rv = handle_open_message(self, msg);
        if (JXTA_SUCCESS == rv) {
            rv = construct_response_message(self, &open_msg, self->i_pipe_adv);
        }
        if (JXTA_SUCCESS == rv) {
            rv = jxta_endpoint_service_send(self->group, self->ep_svc, open_msg, self->ep_addr);
            JXTA_OBJECT_RELEASE(open_msg);
        }
        if (JXTA_SUCCESS == rv) {
            self->state = JXTA_BIDIPIPE_CONNECTED;
            jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Response to open request, connection established\n");
            jpr_thread_mutex_lock(self->cond_mutex);
            apr_thread_cond_signal(self->cond);
            jpr_thread_mutex_unlock(self->cond_mutex);
        } else {
            jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Failed to response open request, connection closed\n");
            JXTA_OBJECT_RELEASE(self->ep_addr);
            self->ep_addr = NULL;
        }
        jpr_thread_mutex_unlock(self->mutex);
        break;

    case JXTA_BIDIPIPE_CLOSING:
        jpr_thread_mutex_unlock(self->mutex);
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Message arrived when closing, discard\n");
        break;

    default:
        jpr_thread_mutex_unlock(self->mutex);
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Invalid state, is this a bug?\n");
        break;
    }
}

static Jxta_status construct_input_pipe(Jxta_bidipipe * self, Jxta_pipe_adv * local_adv)
{
    Jxta_status rv;
    Jxta_pipe_service *ps;
    Jxta_pipe *pipe;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL != self->i_pipe) {
        return JXTA_VIOLATION;
    }

    JXTA_OBJECT_SHARE(local_adv);
    self->i_pipe_adv = local_adv;

    jxta_PG_get_pipe_service(self->group, &ps);
    rv = jxta_pipe_service_timed_accept(ps, local_adv, 1000, &pipe);

    if (JXTA_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(ps);
        return rv;
    }

    jpr_thread_mutex_lock(self->mutex);

    rv = jxta_pipe_get_inputpipe(pipe, &(self->i_pipe));

    jxta_listener_start(self->i_listener);
    jxta_inputpipe_add_listener(self->i_pipe, self->i_listener);

    jpr_thread_mutex_unlock(self->mutex);

    JXTA_OBJECT_RELEASE(pipe);
    JXTA_OBJECT_RELEASE(ps);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_bidipipe *) jxta_bidipipe_new(Jxta_PG * pg)
{
    Jpr_status rv;
    Jxta_bidipipe *self = NULL;

    self = (Jxta_bidipipe *) calloc(1, sizeof(Jxta_bidipipe));

    if (NULL == self) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Out of memory\n");
        return NULL;
    }

    JXTA_OBJECT_SHARE(pg);
    self->group = pg;

    jxta_PG_get_endpoint_service(self->group, &self->ep_svc);

    rv = apr_pool_create(&self->pool, NULL);
    if (rv != APR_SUCCESS) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create pool\n");
        free(self);
        return NULL;
    }

    rv = jpr_thread_mutex_create(&self->mutex, JPR_THREAD_MUTEX_NESTED, self->pool);
    if (JPR_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create mutex\n");
        apr_pool_destroy(self->pool);
        free(self);
        self = NULL;
    }

    rv = jpr_thread_mutex_create(&self->cond_mutex, JPR_THREAD_MUTEX_DEFAULT, self->pool);
    if (JPR_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create cond_mutex\n");
        apr_pool_destroy(self->pool);
        free(self);
        self = NULL;
    }

    rv = apr_thread_cond_create(&self->cond, self->pool);
    if (JPR_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create connect event\n");
        jpr_thread_mutex_destroy(self->mutex);
        apr_pool_destroy(self->pool);
        free(self);
        self = NULL;
    }

    self->i_listener = jxta_listener_new(bidipipe_input_listener, (void *) self, 1, 200);
    self->state = JXTA_BIDIPIPE_CLOSED;

    return self;
}

Jxta_status jxta_bidipipe_delete(Jxta_bidipipe * self)
{
    Jxta_status rv;

    if (NULL == self) {
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_DEBUG, "freeing Bidipipe[%p] ...\n", self);
    jpr_thread_mutex_lock(self->mutex);

    if (JXTA_BIDIPIPE_CLOSED != self->state) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO,
                        "Bidipipe is still connected when delete, close connection ...\n");
        rv = jxta_bidipipe_close(self);
        if (JXTA_SUCCESS != rv) {
            jpr_thread_mutex_unlock(self->mutex);
            return rv;
        }
    }

    JXTA_OBJECT_RELEASE(self->i_listener);
    self->i_listener = NULL;

    jpr_thread_mutex_unlock(self->mutex);
    apr_thread_cond_destroy(self->cond);
    jpr_thread_mutex_destroy(self->mutex);
    jpr_thread_mutex_destroy(self->cond_mutex);
    apr_pool_destroy(self->pool);

    JXTA_OBJECT_RELEASE(self->ep_svc);
    JXTA_OBJECT_RELEASE(self->group);
    free(self);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_bidipipe_connect(Jxta_bidipipe * self, Jxta_pipe_adv * remote_adv, Jxta_listener * listener,
                                                Jxta_time_diff timeout)
{
    Jxta_status rv;
    Jxta_pipe_service *ps;
    Jxta_pipe_adv *local_adv;
    Jxta_message *msg;
    Jxta_pipe *pipe;
    Jxta_outputpipe *o_pipe;
    Jxta_time now;

    now = apr_time_now();   /* microsec, not millisec was used in pipe_timed_connect as queue_dequeue_1 */
    jpr_thread_mutex_lock(self->mutex);
    if (JXTA_BIDIPIPE_CLOSED != self->state) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_VIOLATION;
    }
    self->state = JXTA_BIDIPIPE_CONNECTING;
    jpr_thread_mutex_unlock(self->mutex);

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Attempt connect to remote pipe ...\n");
    jxta_PG_get_pipe_service(self->group, &ps);
    rv = jxta_pipe_service_timed_connect(ps, remote_adv, timeout, NULL, &pipe);
    JXTA_OBJECT_RELEASE(ps);
    switch (rv) {
    case JXTA_SUCCESS:
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Connect attempt to remote pipe succeeded\n");
        break;
    case JXTA_TIMEOUT:
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_INFO, "Connect attempt to remote pipe timeout\n");
        self->state = JXTA_BIDIPIPE_CLOSED;
        return rv;
    default:
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING,
                        "Connect attempt to remote pipe failed, status code %ld\n", rv);
        self->state = JXTA_BIDIPIPE_CLOSED;
        return rv;
    }

    rv = jxta_pipe_get_outputpipe(pipe, &o_pipe);
    JXTA_OBJECT_RELEASE(pipe);

    if (JXTA_SUCCESS != rv) {
        self->state = JXTA_BIDIPIPE_CLOSED;
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Cannot get output pipe\n");
        return rv;
    }

    rv = construct_local_pipe_adv(&local_adv, self->group, remote_adv);
    if (JXTA_SUCCESS != rv) {
        self->state = JXTA_BIDIPIPE_CLOSED;
        JXTA_OBJECT_RELEASE(o_pipe);
        return rv;
    }

    assert(NULL == self->listener);
    JXTA_OBJECT_SHARE(listener);
    self->listener = listener;

    rv = construct_open_message(self, &msg, local_adv);
    if (JXTA_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(listener);
        self->listener = NULL;
        self->state = JXTA_BIDIPIPE_CLOSED;
        JXTA_OBJECT_RELEASE(o_pipe);
        return rv;
    }

    rv = construct_input_pipe(self, local_adv);
    if (JXTA_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(listener);
        self->listener = NULL;
        self->state = JXTA_BIDIPIPE_CLOSED;
        JXTA_OBJECT_RELEASE(o_pipe);
        JXTA_OBJECT_RELEASE(msg);
        return rv;
    }

    rv = jxta_outputpipe_send(o_pipe, msg);

    JXTA_OBJECT_RELEASE(local_adv);
    JXTA_OBJECT_RELEASE(o_pipe);
    JXTA_OBJECT_RELEASE(msg);

    /* Wait for response message until timeout reached, at least 1 ms */
    timeout -= (apr_time_now() - now);
    if (timeout < 1000)
        timeout = 1000;
    jpr_thread_mutex_lock(self->cond_mutex);
    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_DEBUG, "Waiting confirmation of bidipipe connection ...\n");
    rv = apr_thread_cond_timedwait(self->cond, self->cond_mutex, timeout);
    jpr_thread_mutex_unlock(self->cond_mutex);

    if (APR_TIMEUP == rv)
        rv = JXTA_TIMEOUT;

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_bidipipe_close(Jxta_bidipipe * self)
{
    Jxta_status rv;
    Jxta_message *msg;
    Jxta_message_element *e;

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_DEBUG, "jxta_bidipipe_close is called with %X\n", self);
    jpr_thread_mutex_lock(self->mutex);

    switch (self->state) {
    case JXTA_BIDIPIPE_CLOSED:
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;

    case JXTA_BIDIPIPE_ACCEPTING:
        self->state = JXTA_BIDIPIPE_CLOSING;
        jpr_thread_mutex_lock(self->cond_mutex);
        apr_thread_cond_signal(self->cond);
        jpr_thread_mutex_unlock(self->cond_mutex);
        jpr_thread_mutex_unlock(self->mutex);
        rv = close_pipes(self);
        return rv;

    case JXTA_BIDIPIPE_CONNECTED:
        break;

    default:
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Close pipe in a bad state: %d\n", self->state);
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_VIOLATION;
    }

    self->state = JXTA_BIDIPIPE_CLOSING;
    jpr_thread_mutex_unlock(self->mutex);

    msg = jxta_message_new();
    if (NULL == msg) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }

    e = jxta_message_element_new_2(JXTA_BIDIPIPE_NAMESPACE, JXTA_BIDIPIPE_CLOSE_TAG, MIME_TYPE_TEXT, "close", 5, NULL);
    if (NULL == e) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_NOMEM;
    }

    jxta_message_add_element(msg, e);
    JXTA_OBJECT_RELEASE(e);

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Sending close message ...\n");
    rv = jxta_endpoint_service_send(self->group, self->ep_svc, msg, self->ep_addr);
    JXTA_OBJECT_RELEASE(msg);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to make close request\n");
        return rv;
    }

    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Close pipes of bidipipe[%p] ...\n", self);
    rv = close_pipes(self);
    jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_TRACE, "Pipes of bidipipe[%p] closed\n", self);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_bidipipe_accept(Jxta_bidipipe * self, Jxta_pipe_adv * local_adv, Jxta_listener * listener)
{
    Jxta_status rv;

    jpr_thread_mutex_lock(self->mutex);
    if (JXTA_BIDIPIPE_CLOSED != self->state) {
        jpr_thread_mutex_unlock(self->mutex);
        return JXTA_VIOLATION;
    }
    self->state = JXTA_BIDIPIPE_ACCEPTING;
    jpr_thread_mutex_unlock(self->mutex);

    assert(NULL == self->listener);
    JXTA_OBJECT_SHARE(listener);
    self->listener = listener;

    rv = construct_input_pipe(self, local_adv);
    if (JXTA_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(listener);
        self->listener = NULL;
        self->state = JXTA_BIDIPIPE_CLOSED;
        return rv;
    }

    jpr_thread_mutex_lock(self->cond_mutex);
    apr_thread_cond_wait(self->cond, self->cond_mutex);
    jpr_thread_mutex_unlock(self->cond_mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_bidipipe_send(Jxta_bidipipe * self, Jxta_message * msg)
{
    if (JXTA_BIDIPIPE_CONNECTED != self->state) {
        jxta_log_append(JXTA_BIDIPIPE_LOG, JXTA_LOG_LEVEL_WARNING,
                        "Trying to send message while bidipipe is not connected, current state:%d\n", self->state);
        return JXTA_VIOLATION;
    }

    return jxta_endpoint_service_send(self->group, self->ep_svc, msg, self->ep_addr);
}

JXTA_DECLARE(Jxta_bidipipe_state) jxta_bidipipe_get_state(Jxta_bidipipe * self)
{
    return (Jxta_bidipipe_state) self->state;
}

JXTA_DECLARE(Jxta_pipe_adv *) jxta_bidipipe_get_pipe_adv(Jxta_bidipipe * self)
{
    if (NULL != self->i_pipe_adv) {
        JXTA_OBJECT_SHARE(self->i_pipe_adv);
    }

    return self->i_pipe_adv;
}

/* vi: set sw=4 ts=4 et tw=130: */
