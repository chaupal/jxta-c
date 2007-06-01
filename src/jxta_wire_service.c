/*
 * Copyright(c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_wire_service.c,v 1.42 2005/12/22 05:02:36 mmx2005 Exp $
 */

static const char *__log_cat = "WireService";

#include "jxta_apr.h"
#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "jxta_id.h"
#include "jxta_service.h"
#include "jxta_peergroup.h"
#include "jxta_service_private.h"
#include "jxta_hashtable.h"
#include "jxta_pipe_service.h"
#include "jxta_resolver_service.h"
#include "jxta_endpoint_service.h"
#include "jxta_rdv_service.h"
#include "jxta_message.h"
#include "jxta_wm.h"
#include "jxta_rdv_service_private.h"

#define WIRE_SERVICE_NAME   "jxta.service.wirepipe"
#define WM_NAME             "jxta:JxtaWireHeader"
#define MAX_CACHE_MSGID     200

typedef struct {
    Jxta_pipe_service_impl generic;
    char *localPeerString;
    Jxta_PG *group;
    Jxta_pipe_resolver *generic_resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_rdv_service *rdv;
    Jxta_cm *cm;
    Jxta_hashtable *listeners;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_listener *listener;
    char *groupid;
    Jxta_PGID *gid;
    Jxta_vector *cacheMsgIds;

} Jxta_wire_service;

typedef struct {
    Jxta_pipe generic;
    Jxta_pipe_adv *adv;
    Jxta_PG *group;
    Jxta_wire_service *wire_service;

} Jxta_wire_pipe;

typedef struct {
    Jxta_inputpipe generic;
    Jxta_pipe_adv *adv;
    Jxta_wire_service *wire_service;
    Jxta_listener *listener;
    Jxta_listener *user_listener;
} Jxta_wire_inputpipe;

typedef struct {
    Jxta_outputpipe generic;
    Jxta_pipe_adv *adv;
    Jxta_wire_service *wire_service;
    Jxta_listener *user_listener;
} Jxta_wire_outputpipe;

typedef struct {
    JXTA_OBJECT_HANDLE;
    Jxta_pipe_adv *adv;
    Jxta_listener *listener;
    Jxta_vector *listeners;
} Jxta_wire_session;

static char *wire_get_name(Jxta_pipe_service_impl *);
static Jxta_status wire_timed_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_vector *, Jxta_pipe **);
static Jxta_status wire_deny(Jxta_pipe_service_impl *, Jxta_pipe_adv *);
static Jxta_status wire_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_vector *, Jxta_listener *);
static Jxta_status wire_timed_accept(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time_diff, Jxta_pipe **);
static Jxta_status wire_add_accept_listener(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_listener *);
static Jxta_status wire_remove_accept_listener(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_listener *);
static Jxta_status wire_get_pipe_resolver(Jxta_pipe_service_impl *, Jxta_pipe_resolver **);
static Jxta_status wire_set_pipe_resolver(Jxta_pipe_service_impl *, Jxta_pipe_resolver *, Jxta_pipe_resolver **);
static Jxta_wire_outputpipe *wire_outputpipe_new(Jxta_wire_service *, Jxta_PG *, Jxta_pipe_adv * adv);
static Jxta_wire_inputpipe *wire_inputpipe_new(Jxta_wire_service *, Jxta_PG *, Jxta_pipe_adv * adv);
static Jxta_pipe *jxta_wire_pipe_new_instance(Jxta_wire_service *, Jxta_PG *, Jxta_pipe_adv * adv);
static Jxta_status wire_pipe_get_outputpipe(Jxta_pipe *, Jxta_outputpipe **);
static Jxta_status wire_pipe_get_inputpipe(Jxta_pipe *, Jxta_inputpipe **);
static Jxta_status wire_pipe_get_remote_peers(Jxta_pipe *, Jxta_vector **);
static void JXTA_STDCALL wire_service_endpoint_listener(Jxta_object * obj, void *arg);
static Jxta_status notify_local_listeners(Jxta_wire_service * me, const char * pipe_id, Jxta_message * msg);
static Jxta_status notify_remote_listeners(Jxta_wire_service * me, const char * pipe_id, Jxta_message * msg);

static void jxta_wire_service_free(Jxta_object * obj)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    jxta_endpoint_service_remove_listener(self->endpoint, WIRE_SERVICE_NAME, self->groupid);
    jxta_listener_stop(self->listener);
    JXTA_OBJECT_RELEASE(self->listener);

    JXTA_OBJECT_RELEASE(self->endpoint);
    self->endpoint = NULL;

    if (self->cm != NULL) {
        JXTA_OBJECT_RELEASE(self->cm);
        self->cm = NULL;
    }

    if (self->listeners != NULL) {
        JXTA_OBJECT_RELEASE(self->listeners);
        self->listeners = NULL;
    }

    self->group = NULL;

    if (self->groupid != NULL) {
        free(self->groupid);
        self->groupid = NULL;
    }

    if (self->gid != NULL) {
        JXTA_OBJECT_RELEASE(self->gid);
        self->gid = NULL;
    }

    if (self->localPeerString != NULL) {
        free(self->localPeerString);
        self->localPeerString = NULL;
    }
    if (self->generic_resolver != NULL) {
        JXTA_OBJECT_RELEASE(self->generic_resolver);
        self->generic_resolver = NULL;
    }

    if (self->pool != NULL) {
        apr_pool_destroy(self->pool);
        self->pool = NULL;
        self->mutex = NULL;
    }

    if (self->cacheMsgIds != NULL) {
        JXTA_OBJECT_RELEASE(self->cacheMsgIds);
    }

    free(self);
}


Jxta_pipe_service_impl *jxta_wire_service_new_instance(Jxta_pipe_service * pipe_service, Jxta_PG * group)
{

    Jxta_wire_service *self = NULL;
    Jxta_status res;
    Jxta_id *peerId = NULL;
    JString *pid = NULL;
    JString *tmp = NULL;
    char *tmpid;
    Jxta_id *gid = NULL;

    self = (Jxta_wire_service *) malloc(sizeof(Jxta_wire_service));
    memset(self, 0, sizeof(Jxta_wire_service));
    JXTA_OBJECT_INIT(self, jxta_wire_service_free, 0);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);

    if (res != APR_SUCCESS) {
        return NULL;
    }

    self->listeners = jxta_hashtable_new(0);

    jxta_PG_get_rendezvous_service(group, &(self->rdv));
    jxta_PG_get_cache_manager(group, &(self->cm));
    self->group = group;

    res = jxta_pipe_service_get_resolver(pipe_service, &self->generic_resolver);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot get the generic Pipe Service resolver\n");
        return NULL;
    }

    jxta_PG_get_PID(group, &peerId);
    jxta_id_to_jstring(peerId, &pid);
    JXTA_OBJECT_RELEASE(peerId);
    self->localPeerString = (char *) malloc(jstring_length(pid) + 1);
    strcpy(self->localPeerString, jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);
    jxta_PG_get_endpoint_service(group, &(self->endpoint));

    jxta_PG_get_GID(group, &self->gid);
    jxta_id_to_jstring(self->gid, &tmp);
    tmpid = (char *) jstring_get_string(tmp);
    self->groupid = (char *) malloc(strlen(tmpid) + 1);

    /*
     * Only extract the unique Group ID string
     * For building endpoint address
     */
    strcpy(self->groupid, &tmpid[9]);
    JXTA_OBJECT_RELEASE(tmp);

    self->listener = jxta_listener_new(wire_service_endpoint_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(self->endpoint, WIRE_SERVICE_NAME, self->groupid, self->listener);

    jxta_listener_start(self->listener);

    /* Initialize the VTBL */

    self->generic.get_name = wire_get_name;
    self->generic.timed_connect = wire_timed_connect;
    self->generic.connect = wire_connect;
    self->generic.timed_accept = wire_timed_accept;
    self->generic.deny = wire_deny;
    self->generic.add_accept_listener = wire_add_accept_listener;
    self->generic.remove_accept_listener = wire_remove_accept_listener;
    self->generic.get_pipe_resolver = wire_get_pipe_resolver;
    self->generic.set_pipe_resolver = wire_set_pipe_resolver;

    self->cacheMsgIds = jxta_vector_new(MAX_CACHE_MSGID);

    return (Jxta_pipe_service_impl *) self;
}

static char *wire_get_name(Jxta_pipe_service_impl * obj)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");       
        return NULL;
    }

    return (char *) JXTA_PROPAGATE_PIPE;
}

static Jxta_status
wire_timed_connect(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers,
                   Jxta_pipe ** pipe)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wire_timed_connect\n");    

    *pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (*pipe == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create a wire pipe\n");        
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

static Jxta_status wire_deny(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this. */
    return JXTA_NOTIMP;
}

static Jxta_status
wire_connect(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers,
             Jxta_listener * listener)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;
    Jxta_pipe *pipe = NULL;
    Jxta_pipe_connect_event *event = NULL;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wire_connect\n");
    
    pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (pipe == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create a wire pipe\n");        
        return JXTA_NOMEM;
    }

    event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTED, pipe);
    JXTA_OBJECT_RELEASE(pipe);
    jxta_listener_schedule_object(listener, (Jxta_object *) event);
    JXTA_OBJECT_RELEASE(event);
    return JXTA_SUCCESS;
}

static Jxta_status wire_timed_accept(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "wire_timed_accept\n");    
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    *pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (*pipe == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot create a wire pipe\n");
        return JXTA_NOMEM;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "wire_timed_accept return JXTA_SUCCESS\n");
    
    return JXTA_SUCCESS;
}


static Jxta_status wire_add_accept_listener(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /**
     ** The propagation pipe service does not implement this.
     **/
    return JXTA_NOTIMP;
}

static Jxta_status wire_remove_accept_listener(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_status wire_get_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver ** resolver)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_status wire_set_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver * new_resolver,
                                          Jxta_pipe_resolver ** old)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_boolean wire_msgid_new(Jxta_wire_service * self, char *msgId)
{
    unsigned int i;
    JString *val = NULL;

    /*
     * Check if we can find the entry in our MsgId Cache
     */
    for (i = 0; i < jxta_vector_size(self->cacheMsgIds); i++) {
        jxta_vector_get_object_at(self->cacheMsgIds, JXTA_OBJECT_PPTR(&val), i);
        if (strcmp(msgId, jstring_get_string(val)) == 0) {
            JXTA_OBJECT_RELEASE(val);
            return FALSE;
        }
        JXTA_OBJECT_RELEASE(val);
    }


    /*
     * Check the size of the cache. If cache is bigger than
     * MAX_CACHE_MSGID then remove the last entry. We use a FIFO
     * replacement policy.
     */
    if (jxta_vector_size(self->cacheMsgIds) > MAX_CACHE_MSGID) {
        jxta_vector_remove_object_at(self->cacheMsgIds, NULL, jxta_vector_size(self->cacheMsgIds) - 1);
    }

    /*
     * we are sure to have at least one free position
     */
    val = jstring_new_2(msgId);
    jxta_vector_add_object_first(self->cacheMsgIds, (Jxta_object *) val);
    JXTA_OBJECT_RELEASE(val);
    return TRUE;
}

static Jxta_boolean check_wire_header(Jxta_wire_service * self, Jxta_message * msg, JxtaWire ** wm)
{
    Jxta_PID *peer_id = NULL;
    Jxta_PID *src_pid = NULL;
    Jxta_message_element *el = NULL;
    Jxta_status res = 0;
    Jxta_boolean rc = TRUE;
    char *srcPeer = NULL;
    int ttl = 0;
    JString *string = NULL;
    Jxta_bytevector *bytes = NULL;
    char *msgId = NULL;

    res = jxta_message_get_element_1(msg, WM_NAME, &el);

    if (NULL == el) {
        *wm = NULL;
        return FALSE;
    }

    *wm = JxtaWire_new();

    bytes = jxta_message_element_get_value(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    bytes = NULL;
    JxtaWire_parse_charbuffer(*wm, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);
    string = NULL;

    srcPeer = JxtaWire_get_SrcPeer(*wm);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "SrcPeer = %s\n", srcPeer ? srcPeer : "NULL");
    
    if (NULL != srcPeer) {
        jxta_PG_get_PID(self->group, &peer_id);
        jxta_id_from_cstr(&src_pid, srcPeer);
        if (jxta_id_equals(peer_id, src_pid)) {
            rc = FALSE;
            goto FINAL_EXIT;
        }
    }
    
    ttl = JxtaWire_get_TTL(*wm);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "TTL = %d\n", ttl);

    if (ttl <= 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incoming message has ttl of %d - discard\n", ttl);
        rc = FALSE;
        goto FINAL_EXIT;
    }

    /*
     * check if we already have seen this message
     */
    if (self != NULL) {
        msgId = JxtaWire_get_MsgId(*wm);
        if (!wire_msgid_new(self, msgId)) {
            rc = FALSE;
            goto FINAL_EXIT;
        }
    }
  FINAL_EXIT:
    JXTA_OBJECT_RELEASE(src_pid);
    JXTA_OBJECT_RELEASE(peer_id);
    JXTA_OBJECT_RELEASE(el);
    if (!rc) {
        JXTA_OBJECT_RELEASE(*wm);
        *wm = NULL;
    }
    return rc;
}

static JxtaWire *get_new_wire_header(char *srcPeer, Jxta_vector * visitedPeer, int ttl)
{

    JxtaWire *wm = JxtaWire_new();
    Jxta_vector *vector = jxta_vector_new(1);
    JString *string = jstring_new_2(srcPeer);

    jxta_vector_add_object_first(vector, (Jxta_object *) string);

    JxtaWire_set_VisitedPeer(wm, vector);
    JxtaWire_set_SrcPeer(wm, srcPeer);
    JxtaWire_set_TTL(wm, ttl);

    JXTA_OBJECT_RELEASE(string);
    JXTA_OBJECT_RELEASE(vector);
    return wm;
}

static Jxta_status notify_local_listeners(Jxta_wire_service * me, const char * pipe_id, Jxta_message * msg)
{
    Jxta_wire_session *session = NULL;
    Jxta_status res;
    Jxta_listener *listener = NULL;
    unsigned int i;
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "Notify local listeners for pipe Id %s\n", pipe_id);

    res = jxta_hashtable_get(me->listeners, pipe_id, strlen(pipe_id), JXTA_OBJECT_PPTR(&session));
    if (res == JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "Found a listener\n");
        for (i = 0; i < jxta_vector_size(session->listeners); ++i) {
            res = jxta_vector_get_object_at(session->listeners, JXTA_OBJECT_PPTR(&listener), i);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,"failed to get listener at position %u\n", i);
                continue;
            }

            if (listener != NULL) {
                Jxta_message *dup_msg = NULL;
                /* We are calling the input internal listener. We know we can
                   directly invoke it. */
                dup_msg = jxta_message_clone(msg);
                if (dup_msg != NULL) {
                    jxta_listener_process_object(listener, (Jxta_object *) dup_msg);
                    JXTA_OBJECT_RELEASE(dup_msg);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot duplicate message\n");
                }
                JXTA_OBJECT_RELEASE(listener);
            }
        }
        JXTA_OBJECT_RELEASE(session);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot find a listener for wire pipe with Id %s\n", pipe_id);
    }

    return res;
}

/**
 * FIXME: SRDI should support primary key. SRDI does not support primary key now, query only for Pipe ID. 
 */
static Jxta_status notify_remote_listeners(Jxta_wire_service * me, const char * pipe_id, Jxta_message * msg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *peers = NULL;
    Jxta_srdi_service *srdi = NULL;
    unsigned int i;

    jxta_PG_get_srdi_service(me->group, &srdi);
    peers = jxta_srdi_searchSrdi(srdi, "JxtaPipeResolver", NULL, "Id", pipe_id);
    JXTA_OBJECT_RELEASE(srdi);
    if (NULL == peers) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "No remote listeners.\n");
        return JXTA_SUCCESS;
    }

    for (i = 0; i < jxta_vector_size(peers); i++) {
        Jxta_id *peer_id;
        JString *peer_id_jstring;
        Jxta_endpoint_address *destAddr = NULL;

        jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer_id), i);
        jxta_id_to_jstring(peer_id, &peer_id_jstring);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "notify remote peer %s\n", jstring_get_string(peer_id_jstring));

        destAddr = jxta_endpoint_address_new_3(peer_id, WIRE_SERVICE_NAME, me->groupid);
        if (JXTA_SUCCESS != jxta_endpoint_service_send(me->group, me->endpoint, msg, destAddr)) {
            res = JXTA_UNREACHABLE_DEST;
        }

        JXTA_OBJECT_RELEASE(destAddr);
        JXTA_OBJECT_RELEASE(peer_id);
        JXTA_OBJECT_RELEASE(peer_id_jstring);
    }
    JXTA_OBJECT_RELEASE(peers);
    return res;
}

static Jxta_status wire_service_propagate(Jxta_wire_service * self, char *id, Jxta_message * msg)
{

    Jxta_status res = JXTA_SUCCESS;
    JString *xml = NULL;
    Jxta_message_element *el = NULL;
    JString *msgid;
    JxtaWire *wm;

    JXTA_OBJECT_CHECK_VALID(msg);

    wm = get_new_wire_header(self->localPeerString, NULL, 200);

    if (wm == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot allocate a wire header\n");
        return JXTA_NOMEM;
    }

    /*
     * set the Pipe Id and msg id
     */
    JxtaWire_set_PipeId(wm, id);

    msgid = message_id_new();
    JxtaWire_set_MsgId(wm, jstring_get_string(msgid));
    JXTA_OBJECT_RELEASE(msgid);

    res = JxtaWire_get_xml(wm, &xml);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot build XML of wire header\n");
        JXTA_OBJECT_RELEASE(wm);
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_CHECK_VALID(xml);
    JXTA_OBJECT_RELEASE(wm);

    el = jxta_message_element_new_1(WM_NAME, "text/xml", jstring_get_string(xml), jstring_length(xml), NULL);

    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(xml);

    JXTA_OBJECT_CHECK_VALID(self->rdv);
    JXTA_OBJECT_CHECK_VALID(msg);

    notify_local_listeners(self, id, msg);
    if (jxta_rdv_service_is_rendezvous(self->rdv)) {
        notify_remote_listeners(self, id, msg);
    }
    res = jxta_rdv_service_walk(self->rdv, msg, WIRE_SERVICE_NAME, self->groupid);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot progate message\n");
    }

    return res;
}

static void JXTA_STDCALL wire_service_message_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_wire_session *session = (Jxta_wire_session *) arg;
    Jxta_listener *listener = NULL;
    Jxta_status res;
    JxtaWire *wm;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "wire_service got incoming message\n");
    JXTA_OBJECT_CHECK_VALID(msg);

    if (check_wire_header(NULL, msg, &wm)) {
        unsigned int i;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Accepting incoming message\n");
        /* We can process this message */
        for (i = 0; i < jxta_vector_size(session->listeners); ++i) {
            res = jxta_vector_get_object_at(session->listeners, JXTA_OBJECT_PPTR(&listener), i);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "failed\n");
                continue;
            }

            if (listener != NULL) {
                Jxta_message *dup_msg = NULL;
                /* We are calling the input internal listener. We know we can
                   directly invoke it. */
                dup_msg = jxta_message_clone(msg);
                if (dup_msg != NULL) {
                    jxta_listener_process_object(listener, (Jxta_object *) dup_msg);
                    JXTA_OBJECT_RELEASE(dup_msg);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "Cannot duplicate message\n");
                }
                JXTA_OBJECT_RELEASE(listener);
            }
        }
        JXTA_OBJECT_RELEASE(wm);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG , "No Propagation header - discard,\n");
    }
}

static void JXTA_STDCALL wire_service_endpoint_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_wire_service *self = (Jxta_wire_service *) arg;
    JxtaWire *wm;
    char *id;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wire_service got incoming message\n");
    JXTA_OBJECT_CHECK_VALID(msg);

    if (check_wire_header(self, msg, &wm)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Accepting incoming message from endpoint\n");
        /* We can process this message */

        id = JxtaWire_get_PipeId(wm);
        if (id != NULL) {
            notify_local_listeners(self, id, msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Could not find a pipe Id \n");
        }
        if (jxta_rdv_service_is_rendezvous(self->rdv)) {
            notify_remote_listeners(self, id, msg);
            jxta_rdv_service_walk(self->rdv, msg, WIRE_SERVICE_NAME, self->groupid);
        }
        JXTA_OBJECT_RELEASE(wm);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No Propagation endpoint header - discard,\n");
    }
}

static void wire_session_free(Jxta_object * obj)
{
    Jxta_wire_session *self = (Jxta_wire_session *) obj;

    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }
    if (self->listeners != NULL) {
        JXTA_OBJECT_RELEASE(self->listeners);
        self->listeners = NULL;
    }
    free(self);
}

static Jxta_wire_session *wire_session_new(void)
{
    Jxta_wire_session *self = (Jxta_wire_session *) calloc(1, sizeof(Jxta_wire_session));
    JXTA_OBJECT_INIT(self, wire_session_free, NULL);
    return self;
}

static Jxta_status wire_service_add_listener(Jxta_wire_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{
    Jxta_wire_session *session = NULL;
    Jxta_status res;
    Jxta_pipe_resolver *pipe_resolver = self->generic_resolver;

    const char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);

    apr_thread_mutex_lock(self->mutex);
    res = jxta_hashtable_get(self->listeners, id, strlen(id), JXTA_OBJECT_PPTR(&session));
    if (res != JXTA_SUCCESS) {
        Jxta_listener *tmp = NULL;

        /* There is no session yet for this id. Create a new one */
        session = wire_session_new();
        session->listeners = jxta_vector_new(1);
        JXTA_OBJECT_SHARE(adv);
        session->adv = adv;
        /* Create our own listener, and connect it to the Rendezvous Service */
        tmp = jxta_listener_new((Jxta_listener_func) wire_service_message_listener, (void *) session, 1, 10);
        if (tmp == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot create listener\n");
            JXTA_OBJECT_RELEASE(session);
            return JXTA_NOMEM;
        }
        session->listener = tmp;
        res = jxta_rdv_service_add_propagate_listener((Jxta_rdv_service *) self->rdv, WIRE_SERVICE_NAME, id, tmp);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot add propagate listener reason= %d\n", res);
            JXTA_OBJECT_RELEASE(session);
            return res;
        }
        jxta_listener_start(tmp);
        jxta_hashtable_put(self->listeners, id, strlen(id), (Jxta_object *) session);
        JXTA_OBJECT_RELEASE(session);
    }

    /* Add the listener to the vector */
    res = jxta_vector_add_object_first(session->listeners, (Jxta_object *) listener);
    res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, adv, NULL, TRUE);
    apr_thread_mutex_unlock(self->mutex);
    return res;
}


static Jxta_status wire_service_remove_listener(Jxta_wire_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_wire_session *session = NULL;
    Jxta_listener *tmp = NULL;
    unsigned int i = 0;
    Jxta_status res = JXTA_INVALID_ARGUMENT;
    const char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);
    Jxta_pipe_resolver *pipe_resolver = self->generic_resolver;

    apr_thread_mutex_lock(self->mutex);
    res = jxta_hashtable_del(self->listeners, id, strlen(id), JXTA_OBJECT_PPTR(&session));
    if (res != JXTA_SUCCESS) {
        /* There is no session for this id. */
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;

    }

    /* Find the listener in the vector */
    /*FIXME verify the vector length */
    for (i = 0; i <= jxta_vector_size(session->listeners); ++i) {

        res = jxta_vector_get_object_at(session->listeners, JXTA_OBJECT_PPTR(&tmp), i);
        if (res != JXTA_SUCCESS) {
            continue;
        }

        if (tmp == listener) {
            /* That's the listener to remove */
            res = jxta_vector_remove_object_at(session->listeners, NULL, i);
            JXTA_OBJECT_RELEASE(tmp);
            break;
        }
        JXTA_OBJECT_RELEASE(tmp);
    }

    if (jxta_vector_size(session->listeners) == 0) {
        /* No more need to keep the session alive */

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot remove propagate listener reason= %d\n", res);
        }

        res = jxta_rdv_service_remove_propagate_listener((Jxta_rdv_service *) self->rdv,
                                                         WIRE_SERVICE_NAME, id, session->listener);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot remove propagate listener reason= %d\n", res);
        }

	jxta_listener_stop(session->listener);
	JXTA_OBJECT_RELEASE(session->listener);

        res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, adv, NULL, FALSE);

    }

    JXTA_OBJECT_RELEASE(session);

    apr_thread_mutex_unlock(self->mutex);
    return res;
}


/****************************************************************
 ** Jxta_wire_pipe implementation
 ****************************************************************/

static void jxta_wire_pipe_free(Jxta_object * obj)
{

    Jxta_wire_pipe *self = (Jxta_wire_pipe *) obj;
    if (self->wire_service != NULL) {
        JXTA_OBJECT_RELEASE(self->wire_service);
        self->wire_service = NULL;
    }

    self->group = NULL;

    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }
    free(self);
}


static Jxta_pipe *jxta_wire_pipe_new_instance(Jxta_wire_service * pipe_service, Jxta_PG * group, Jxta_pipe_adv * adv)
{

    Jxta_wire_pipe *self = NULL;
    if ((adv == NULL) || (group == NULL) || (pipe_service == NULL)) {
        return NULL;
    }

    self = (Jxta_wire_pipe *) malloc(sizeof(Jxta_wire_pipe));
    memset(self, 0, sizeof(Jxta_wire_pipe));
    JXTA_OBJECT_INIT(self, jxta_wire_pipe_free, 0);

    JXTA_OBJECT_SHARE(adv);
    JXTA_OBJECT_SHARE(pipe_service);

    self->group = group;
    self->adv = adv;
    self->wire_service = pipe_service;

    /* Initialize the VTBL. */
    self->generic.get_outputpipe = wire_pipe_get_outputpipe;
    self->generic.get_inputpipe = wire_pipe_get_inputpipe;
    self->generic.get_remote_peers = wire_pipe_get_remote_peers;
    return (Jxta_pipe *) self;
}


static Jxta_status wire_pipe_get_outputpipe(Jxta_pipe * obj, Jxta_outputpipe ** op)
{

    Jxta_wire_pipe *self = (Jxta_wire_pipe *) obj;
    Jxta_outputpipe *tmp;
    if ((self == NULL) || (op == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    tmp = (Jxta_outputpipe *) wire_outputpipe_new(self->wire_service, self->group, self->adv);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "failed\n");
        return JXTA_NOMEM;
    }

    *op = tmp;
    return JXTA_SUCCESS;
}

static Jxta_status wire_pipe_get_inputpipe(Jxta_pipe * obj, Jxta_inputpipe ** ip)
{

    Jxta_wire_pipe *self = (Jxta_wire_pipe *) obj;
    Jxta_inputpipe *tmp;

    if ((self == NULL) || (ip == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    tmp = (Jxta_inputpipe *) wire_inputpipe_new(self->wire_service, self->group, self->adv);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "failed\n");
        return JXTA_NOMEM;
    }
    *ip = tmp;
    return JXTA_SUCCESS;
}

static Jxta_status wire_pipe_get_remote_peers(Jxta_pipe * self, Jxta_vector ** vector)
{
    /* The propagated pipes(wire pipe) does no implement this */
    return JXTA_NOTIMP;
}


/****************************************************************
 ** Jxta_wire_inputpipe implementation
 ****************************************************************/

static Jxta_status inputpipe_timed_receive(Jxta_inputpipe * obj, Jxta_time_diff timeout, Jxta_message ** msg)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    Jxta_listener *listener = NULL;
    Jxta_status res;

    if (self->user_listener != NULL) {
        return JXTA_BUSY;
    }

    if (self->listener == NULL) {
        /* Create our listener and connect it to the wire service */
        listener = jxta_listener_new(NULL, (void *) self, 0, 200);
        if (listener == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot create listener\n");
            return JXTA_NOMEM;
        }
        self->listener = listener;
        res = wire_service_add_listener(self->wire_service, self->adv, listener);
        if (res != JXTA_SUCCESS) {
            JXTA_OBJECT_RELEASE(listener);
            self->listener = NULL;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot add listener\n");
            return JXTA_NOMEM;
        }
        jxta_listener_start(listener);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting for incoming message\n");
    return jxta_listener_wait_for_event(self->listener, timeout, (Jxta_object **) msg);
}

static Jxta_status inputpipe_add_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(listener);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "add listener starts\n");
    if ((self->user_listener != NULL) || (self->listener != NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "busy\n");
        return JXTA_BUSY;
    }
    JXTA_OBJECT_SHARE(listener);
    self->user_listener = listener;

    res = wire_service_add_listener(self->wire_service, self->adv, listener);

    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(listener);
        self->user_listener = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot add listener\n");
        return JXTA_NOMEM;
    }
    return JXTA_SUCCESS;
}

static Jxta_status inputpipe_remove_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    if (self->user_listener != listener) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    JXTA_OBJECT_RELEASE(listener);
    self->user_listener = NULL;
    return JXTA_SUCCESS;
}

static void wire_inputpipe_free(Jxta_object * obj)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    Jxta_status res;

    if (self->listener != NULL) {
        res = wire_service_remove_listener(self->wire_service, self->adv, self->listener);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Cannot remove listener from wire service.\n");
        }
        JXTA_OBJECT_RELEASE(self->listener);
        self->listener = NULL;
    }
    if (self->user_listener != NULL) {
        JXTA_OBJECT_RELEASE(self->user_listener);
        self->user_listener = NULL;
    }
    if (self->wire_service != NULL) {
        JXTA_OBJECT_RELEASE(self->wire_service);
        self->wire_service = NULL;
    }
    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }
    free(self);
}


static Jxta_wire_inputpipe *wire_inputpipe_new(Jxta_wire_service * pipe_service, Jxta_PG * group, Jxta_pipe_adv * adv)
{

    Jxta_wire_inputpipe *self = NULL;
    char *id = (char *) jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);

    if ((id == NULL) || (group == NULL) || (pipe_service == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return NULL;
    }

    self = (Jxta_wire_inputpipe *) malloc(sizeof(Jxta_wire_inputpipe));
    memset(self, 0, sizeof(Jxta_wire_inputpipe));
    JXTA_OBJECT_INIT(self, wire_inputpipe_free, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wire_inputpipe_new\n");
    JXTA_OBJECT_SHARE(pipe_service);
    self->wire_service = pipe_service;
    JXTA_OBJECT_SHARE(adv);
    self->adv = adv;
    self->listener = NULL;
    self->user_listener = NULL;

    /* Initialize the VTBL */

    self->generic.timed_receive = inputpipe_timed_receive;
    self->generic.add_listener = inputpipe_add_listener;
    self->generic.remove_listener = inputpipe_remove_listener;
    return (Jxta_wire_inputpipe *) self;
}


/****************************************************************
 ** Jxta_wire_outputpipe implementation
 ****************************************************************/

static Jxta_status outputpipe_add_listener(Jxta_outputpipe * obj, Jxta_listener * listener)
{

    /* The wire service does not implement this. (no meaning). */
    return JXTA_NOTIMP;
}

static Jxta_status outputpipe_remove_listener(Jxta_outputpipe * obj, Jxta_listener * listener)
{

    /* The wire service does not implement this. (no meaning) */
    return JXTA_NOTIMP;
}

static Jxta_status outputpipe_send(Jxta_outputpipe * obj, Jxta_message * msg)
{

    Jxta_wire_outputpipe *self = (Jxta_wire_outputpipe *) obj;
    char *id = (char *) jxta_pipe_adv_get_Id((Jxta_pipe_adv *) self->adv);

    JXTA_OBJECT_CHECK_VALID(obj);
    JXTA_OBJECT_CHECK_VALID(msg);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "wire_outputpipe_send\n");
    return wire_service_propagate(self->wire_service, id, msg);
}

static void wire_outputpipe_free(Jxta_object * obj)
{
    Jxta_wire_outputpipe *self = (Jxta_wire_outputpipe *) obj;
    if (self->wire_service != NULL) {
        JXTA_OBJECT_RELEASE(self->wire_service);
        self->wire_service = NULL;
    }
    if (self->user_listener != NULL) {
        JXTA_OBJECT_RELEASE(self->user_listener);
        self->user_listener = NULL;
    }
    if (self->adv != NULL) {
        JXTA_OBJECT_RELEASE(self->adv);
        self->adv = NULL;
    }
    free(self);
}

static Jxta_wire_outputpipe *wire_outputpipe_new(Jxta_wire_service * pipe_service, Jxta_PG * group, Jxta_pipe_adv * adv)
{

    Jxta_wire_outputpipe *self = NULL;

    if ((adv == NULL) || (group == NULL) || (pipe_service == NULL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid argument\n");
        return NULL;
    }

    self = (Jxta_wire_outputpipe *) malloc(sizeof(Jxta_wire_outputpipe));
    memset(self, 0, sizeof(Jxta_wire_outputpipe));
    JXTA_OBJECT_INIT(self, wire_outputpipe_free, 0);

    JXTA_OBJECT_SHARE(adv);
    self->adv = adv;
    JXTA_OBJECT_SHARE(pipe_service);
    self->wire_service = pipe_service;

    /* Initialize the VTBL */
    self->generic.send = outputpipe_send;
    self->generic.add_listener = outputpipe_add_listener;
    self->generic.remove_listener = outputpipe_remove_listener;
    return (Jxta_wire_outputpipe *) self;
}

/* vim: set ts=4 sw=4 tw=130 et: */
