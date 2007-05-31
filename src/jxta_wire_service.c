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
 * $Id: jxta_wire_service.c,v 1.19 2005/03/12 00:51:22 slowhog Exp $
 */

#include "jxtaapr.h"
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

#define WIRE_SERVICE_NAME   "jxta.service.wirepipe"
#define WIRE_SERVICE_PAR    "jxta-NetGroup"
#define WM_NAME             "jxta:JxtaWireHeader"
#define MAX_CACHE_MSGID     200

typedef struct {
    Jxta_pipe_service_impl generic;
    char *localPeerString;
    Jxta_PG *group;
    JString *gid_str;
    Jxta_pipe_resolver *generic_resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_rdv_service *rdv;
    Jxta_hashtable *listeners;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    Jxta_listener *listener;
    Jxta_listener *rdv_listener;
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
static Jxta_status wire_timed_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time, Jxta_vector *, Jxta_pipe **);
static Jxta_status wire_deny(Jxta_pipe_service_impl *, Jxta_pipe_adv *);
static Jxta_status wire_connect(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time, Jxta_vector *, Jxta_listener *);
static Jxta_status wire_timed_accept(Jxta_pipe_service_impl *, Jxta_pipe_adv *, Jxta_time, Jxta_pipe **);
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
static void wire_service_endpoint_listener(Jxta_object * obj, void *arg);
static void wire_service_rdv_listener(Jxta_object * obj, void *arg);

static void jxta_wire_service_free(Jxta_object * obj)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    jxta_rdv_service_remove_event_listener(self->rdv, (char *) JXTA_PROPAGATE_PIPE, (char *) jstring_get_string(self->gid_str));

    if (self->gid_str) {
        JXTA_OBJECT_RELEASE(self->gid_str);
        self->gid_str = NULL;
    }

    if (self->listeners != NULL) {
        JXTA_OBJECT_RELEASE(self->listeners);
        self->listeners = NULL;
    }

    if (self->rdv_listener != NULL) {
        JXTA_OBJECT_RELEASE(self->rdv_listener);
        self->rdv_listener = NULL;
    }
    if (self->group != NULL) {
        JXTA_OBJECT_RELEASE(self->group);
        self->group = NULL;
    }
    if (self->rdv != NULL) {
        JXTA_OBJECT_RELEASE(self->rdv);
        self->rdv = NULL;
    }

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
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);

    if (res != APR_SUCCESS) {
        return NULL;
    }

    self->listeners = jxta_hashtable_new(0);

    jxta_PG_get_rendezvous_service(group, &(self->rdv));
    self->group = group;

    res = jxta_pipe_service_get_resolver(pipe_service, &self->generic_resolver);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Cannot get the generic Pipe Service resolver\n");
        return NULL;
    }

    jxta_PG_get_PID(group, &peerId);
    jxta_id_to_jstring(peerId, &pid);
    JXTA_OBJECT_RELEASE(peerId);
    self->localPeerString = malloc(jstring_length(pid) + 1);
    strcpy(self->localPeerString, jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);
    jxta_PG_get_endpoint_service(group, &(self->endpoint));

    jxta_PG_get_GID(group, &self->gid);
    jxta_id_to_jstring(self->gid, &tmp);
    tmpid = (char *) jstring_get_string(tmp);
    self->groupid = malloc(strlen(tmpid) + 1);

    /*
     * Only extract the unique Group ID string
     * For building endpoint address
     */
    strcpy(self->groupid, &tmpid[9]);
    JXTA_OBJECT_RELEASE(tmp);

    self->listener = jxta_listener_new(wire_service_endpoint_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(self->endpoint, (char *) WIRE_SERVICE_NAME, self->groupid, self->listener);

    jxta_listener_start(self->listener);

    /*
     * register to the RDV service for RDV events
     */

    jxta_PG_get_GID(group, &gid);
    jxta_id_to_jstring(gid, &self->gid_str);
    self->rdv_listener = jxta_listener_new((Jxta_listener_func) wire_service_rdv_listener, self, 1, 1);

    jxta_listener_start(self->rdv_listener);
    jxta_rdv_service_add_event_listener(self->rdv,
                                        (char *) JXTA_PROPAGATE_PIPE,
                                        (char *) jstring_get_string(self->gid_str), self->rdv_listener);
    JXTA_OBJECT_RELEASE(gid);

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
        JXTA_LOG("Invalid argument\n");
        return NULL;
    }

    return (char *) JXTA_PROPAGATE_PIPE;
}

static Jxta_status
wire_timed_connect(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_vector * peers, Jxta_pipe ** pipe)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_LOG("wire_timed_connect\n");

    *pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (*pipe == NULL) {
        JXTA_LOG("Cannot create a wire pipe\n");
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

static Jxta_status wire_deny(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;
    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this. */
    return JXTA_NOTIMP;
}

static Jxta_status
wire_connect(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_vector * peers, Jxta_listener * listener)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;
    Jxta_pipe *pipe = NULL;
    Jxta_pipe_connect_event *event = NULL;

    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_LOG("wire_connect\n");

    pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (pipe == NULL) {
        JXTA_LOG("Cannot create a wire pipe\n");
        return JXTA_NOMEM;
    }

    event = jxta_pipe_connect_event_new(JXTA_PIPE_CONNECTED, pipe);
    JXTA_OBJECT_RELEASE(pipe);
    jxta_listener_schedule_object(listener, (Jxta_object *) event);
    JXTA_OBJECT_RELEASE(event);
    return JXTA_SUCCESS;
}

static Jxta_status wire_timed_accept(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_time timeout, Jxta_pipe ** pipe)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    JXTA_LOG("wire_timed_accept\n");
    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    *pipe = jxta_wire_pipe_new_instance(self, self->group, adv);
    if (*pipe == NULL) {
        JXTA_LOG("Cannot create a wire pipe\n");
        return JXTA_NOMEM;
    }
    JXTA_LOG("wire_timed_accept return JXTA_SUCCESS\n");
    return JXTA_SUCCESS;
}


static Jxta_status wire_add_accept_listener(Jxta_pipe_service_impl * obj, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
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
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_status wire_get_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver ** resolver)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_status wire_set_pipe_resolver(Jxta_pipe_service_impl * obj, Jxta_pipe_resolver * new, Jxta_pipe_resolver ** old)
{

    Jxta_wire_service *self = (Jxta_wire_service *) obj;

    if (self == NULL) {
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The propagation pipe service does not implement this */
    return JXTA_NOTIMP;
}

static Jxta_boolean wire_msgid_new(Jxta_wire_service * self, char *msgId)
{
    int i;
    JString *val = NULL;

    /*
     * Check if we can find the entry in our MsgId Cache
     */
    for (i = 0; i < jxta_vector_size(self->cacheMsgIds); i++) {
        jxta_vector_get_object_at(self->cacheMsgIds, (Jxta_object **) & val, i);
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

static boolean check_wire_header(Jxta_wire_service * self, Jxta_message * msg, JxtaWire ** wm)
{

    Jxta_message_element *el = NULL;
    Jxta_status res = 0;

    res = jxta_message_get_element_1(msg, (char *) WM_NAME, &el);

    if (el) {
        Jxta_vector *visitedPeers = NULL;
        char *srcPeer = NULL;
        int ttl = 0;
        JString *string = NULL;
        Jxta_bytevector *bytes = NULL;
        char *msgId = NULL;

        *wm = JxtaWire_new();

        bytes = jxta_message_element_get_value(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        JxtaWire_parse_charbuffer(*wm, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);
        string = NULL;
        srcPeer = JxtaWire_get_SrcPeer(*wm);
        ttl = JxtaWire_get_TTL(*wm);
        visitedPeers = JxtaWire_get_VisitedPeer(*wm);

        JXTA_LOG("SrcPeer = %s\n", srcPeer);
        JXTA_LOG("TTL     = %d\n", ttl);

        JXTA_OBJECT_RELEASE(visitedPeers);
        JXTA_OBJECT_RELEASE(el);

        if (ttl <= 0) {
            JXTA_LOG("Incoming message has ttl of %d - discard\n", ttl);
            return FALSE;
        }

        /*
         * check if we already have seen this message
         */
        if (self != NULL) {
            msgId = JxtaWire_get_MsgId(*wm);
            if (!wire_msgid_new(self, msgId)) {
                return FALSE;
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
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

static Jxta_status wire_service_propagate(Jxta_wire_service * self, char *id, Jxta_message * msg)
{

    Jxta_status res = JXTA_SUCCESS;
    JString *xml = NULL;
    Jxta_message_element *el = NULL;
    char *pt = NULL;
    JString *msgid;
    JxtaWire *wm = get_new_wire_header(self->localPeerString,
                                       NULL,
                                       200);
    /*
     * set the Pipe Id and msg id
     */
    JxtaWire_set_PipeId(wm, id);
    res = message_id_new((Jxta_id *) self->gid, &msgid);
    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(wm);
        return JXTA_NOMEM;
    }

    JxtaWire_set_MsgId(wm, (char *) jstring_get_string(msgid));

    JXTA_OBJECT_CHECK_VALID(msg);

    if (wm == NULL) {
        JXTA_LOG("Cannot allocate a wire header\n");
        JXTA_OBJECT_RELEASE(msgid);
        return JXTA_NOMEM;
    }

    res = JxtaWire_get_xml(wm, &xml);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Cannot build XML of wire header\n");
        JXTA_OBJECT_RELEASE(wm);
        JXTA_OBJECT_RELEASE(msgid);
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_CHECK_VALID(xml);
    JXTA_OBJECT_RELEASE(wm);
    JXTA_OBJECT_RELEASE(msgid);

    pt = malloc(jstring_length(xml));
    memcpy(pt, jstring_get_string(xml), jstring_length(xml));

    JXTA_LOG("Wire header %s\n", jstring_get_string(xml));

    el = jxta_message_element_new_1((char *) WM_NAME, "text/xml", pt, jstring_length(xml), NULL);

    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(xml);
    free(pt);

    JXTA_OBJECT_CHECK_VALID(self->rdv);
    JXTA_OBJECT_CHECK_VALID(msg);

    res = jxta_rdv_service_propagate(self->rdv, msg, (char *) WIRE_SERVICE_NAME, self->groupid, 200);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Cannot progate message\n");
    }

    return res;
}

static void wire_service_message_listener(Jxta_object * obj, void *arg)
{

    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_wire_session *session = (Jxta_wire_session *) arg;
    Jxta_listener *listener = NULL;
    Jxta_status res;
    JxtaWire *wm;

    JXTA_LOG("wire_service got incoming message\n");
    JXTA_OBJECT_CHECK_VALID(msg);

    if (check_wire_header(NULL, msg, &wm)) {
        int i;

        JXTA_LOG("Accepting incoming message\n");
        /* We can process this message */
        for (i = 0; i < jxta_vector_size(session->listeners); ++i) {
            res = jxta_vector_get_object_at(session->listeners, (Jxta_object **) & listener, i);
            if (res != JXTA_SUCCESS) {
                JXTA_LOG("failed\n");
                continue;
            }

            if (listener != NULL) {
                Jxta_message *dup_msg = NULL;
                /* We are calling the inputput internal listener. We know we can
                   directely invoke it. */
                dup_msg = jxta_message_clone(msg);
                if (dup_msg != NULL) {
                    jxta_listener_process_object(listener, (Jxta_object *) dup_msg);
                    JXTA_OBJECT_RELEASE(dup_msg);
                } else {
                    JXTA_LOG("Cannot duplicate message\n");
                }
                JXTA_OBJECT_RELEASE(listener);
            }
        }
    } else {
        JXTA_LOG("No Propagation header - discard,\n");
    }
    JXTA_OBJECT_RELEASE(msg);
}

static void wire_service_endpoint_listener(Jxta_object * obj, void *arg)
{

    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_wire_service *self = (Jxta_wire_service *) arg;
    Jxta_listener *listener = NULL;
    Jxta_status res;
    JxtaWire *wm;
    char *id;
    Jxta_wire_session *session;

    JXTA_LOG("wire_service got incoming message\n");
    JXTA_OBJECT_CHECK_VALID(msg);

    if (check_wire_header(self, msg, &wm)) {
        int i;

        JXTA_LOG("Accepting incoming message from endpoint\n");
        /* We can process this message */

        id = JxtaWire_get_PipeId(wm);
        if (id != NULL) {
            JXTA_LOG("Receiving for pipe Id %s\n", id);

            res = jxta_hashtable_get(self->listeners, id, strlen(id), (Jxta_object **) & session);
            if (res == JXTA_SUCCESS) {

                JXTA_LOG("Found a listener\n");
                for (i = 0; i < jxta_vector_size(session->listeners); ++i) {
                    res = jxta_vector_get_object_at(session->listeners, (Jxta_object **) & listener, i);
                    if (res != JXTA_SUCCESS) {
                        JXTA_LOG("failed\n");
                        continue;
                    }

                    if (listener != NULL) {
                        Jxta_message *dup_msg = NULL;
                        /* We are calling the inputput internal listener. We know we can
                           directely invoke it. */
                        dup_msg = jxta_message_clone(msg);
                        if (dup_msg != NULL) {
                            jxta_listener_process_object(listener, (Jxta_object *) dup_msg);
                        } else {
                            JXTA_LOG("Cannot duplicate message\n");
                        }
                        JXTA_OBJECT_RELEASE(listener);
                    }
                }
            } else {
                JXTA_LOG("No Propagation could not find listener\n");
            }
        } else {
            JXTA_LOG("Could not find a pipe Id \n");
        }
    } else {
        JXTA_LOG("No Propagation endpoint header - discard,\n");
    }
    JXTA_OBJECT_RELEASE(msg);
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
    Jxta_wire_session *self = (Jxta_wire_session *) malloc(sizeof(Jxta_wire_session));
    memset(self, 0, sizeof(Jxta_wire_session));
    JXTA_OBJECT_INIT(self, wire_session_free, NULL);
    return self;
}

static Jxta_status wire_service_add_listener(Jxta_wire_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{
    Jxta_wire_session *session = NULL;
    Jxta_status res;
    Jxta_pipe_resolver *pipe_resolver = self->generic_resolver;

    char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);

    apr_thread_mutex_lock(self->mutex);
    res = jxta_hashtable_get(self->listeners, id, strlen(id), (Jxta_object **) & session);
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
            JXTA_LOG("Cannot create listener\n");
            JXTA_OBJECT_RELEASE(session);
            return JXTA_NOMEM;
        }
        session->listener = tmp;
        res = jxta_rdv_service_add_propagate_listener((Jxta_rdv_service *) self->rdv,
                                                      (char *) WIRE_SERVICE_NAME, (char *) id, tmp);
        if (res != JXTA_SUCCESS) {
            JXTA_LOG("Cannot add propagate listener reason= %d\n", res);
            JXTA_OBJECT_RELEASE(session);
            return res;
        }
        jxta_listener_start(tmp);
        jxta_hashtable_put(self->listeners, id, strlen(id), (Jxta_object *) session);
    }

    /* Add the listener to the vector */
    res = jxta_vector_add_object_first(session->listeners, (Jxta_object *) listener);
    res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, adv, NULL, TRUE);
    JXTA_OBJECT_RELEASE(session);
    apr_thread_mutex_unlock(self->mutex);
    return res;
}


static Jxta_status wire_service_remove_listener(Jxta_wire_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    Jxta_wire_session *session = NULL;
    Jxta_listener *tmp = NULL;
    int i = 0;
    Jxta_status res = JXTA_INVALID_ARGUMENT;
    char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);
    Jxta_pipe_resolver *pipe_resolver = self->generic_resolver;

    apr_thread_mutex_lock(self->mutex);
    res = jxta_hashtable_get(self->listeners, id, strlen(id), (Jxta_object **) & session);
    if (res != JXTA_SUCCESS) {
        /* There is no session for this id. */
        apr_thread_mutex_unlock(self->mutex);
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;

    }

    /* Find the listener in the vector */
    /*FIXME verify the vector length */
    for (i = 0; i <= jxta_vector_size(session->listeners); ++i) {

        res = jxta_vector_get_object_at(session->listeners, (Jxta_object **) & tmp, i);
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
            JXTA_LOG("Cannot remove propagate listener reason= %d\n", res);
        }

        res = jxta_rdv_service_remove_propagate_listener((Jxta_rdv_service *) self->rdv,
                                                         (char *) WIRE_SERVICE_NAME, (char *) id, session->listener);
        if (res != JXTA_SUCCESS) {
            JXTA_LOG("Cannot remove propagate listener reason= %d\n", res);
        }

        res = jxta_hashtable_del(self->listeners, (const void *) id, strlen(id), (Jxta_object **) & session);
        res = jxta_pipe_resolver_send_srdi_message(pipe_resolver, adv, NULL, FALSE);

    }

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
    if (self->group != NULL) {
        JXTA_OBJECT_RELEASE(self->group);
        self->group = NULL;
    }
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

    JXTA_OBJECT_SHARE(group);
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
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    tmp = (Jxta_outputpipe *) wire_outputpipe_new(self->wire_service, self->group, self->adv);
    if (tmp == NULL) {
        JXTA_LOG("failed\n");
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
        JXTA_LOG("Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    tmp = (Jxta_inputpipe *) wire_inputpipe_new(self->wire_service, self->group, self->adv);
    if (tmp == NULL) {
        JXTA_LOG("failed\n");
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

static Jxta_status inputpipe_timed_receive(Jxta_inputpipe * obj, Jxta_time timeout, Jxta_message ** msg)
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
            JXTA_LOG("Cannot create listener\n");
            return JXTA_NOMEM;
        }
        self->listener = listener;
        res = wire_service_add_listener(self->wire_service, self->adv, listener);
        if (res != JXTA_SUCCESS) {
            JXTA_OBJECT_RELEASE(listener);
            self->listener = NULL;
            JXTA_LOG("Cannot add listener\n");
            return JXTA_NOMEM;
        }
        jxta_listener_start(listener);
    }

    JXTA_LOG("Waiting for incoming message\n");
    return jxta_listener_wait_for_event(self->listener, timeout, (Jxta_object **) msg);
}

static Jxta_status inputpipe_add_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(listener);

    JXTA_LOG("add listener starts\n");
    if ((self->user_listener != NULL) || (self->listener != NULL)) {
        JXTA_LOG("busy\n");
        return JXTA_BUSY;
    }
    JXTA_OBJECT_SHARE(listener);
    self->user_listener = listener;

    res = wire_service_add_listener(self->wire_service, self->adv, listener);

    if (res != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(listener);
        self->user_listener = NULL;
        JXTA_LOG("Cannot add listener\n");
        return JXTA_NOMEM;
    }
    return JXTA_SUCCESS;
}

static Jxta_status inputpipe_remove_listener(Jxta_inputpipe * obj, Jxta_listener * listener)
{

    Jxta_wire_inputpipe *self = (Jxta_wire_inputpipe *) obj;
    if (self->user_listener != listener) {
        JXTA_LOG("Invalid argument\n");
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
            JXTA_LOG("Cannot remove listener from wire service.\n");
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
    char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) adv);

    if ((id == NULL) || (group == NULL) || (pipe_service == NULL)) {
        JXTA_LOG("Invalid argument\n");
        return NULL;
    }

    self = (Jxta_wire_inputpipe *) malloc(sizeof(Jxta_wire_inputpipe));
    memset(self, 0, sizeof(Jxta_wire_inputpipe));
    JXTA_OBJECT_INIT(self, wire_inputpipe_free, 0);

    JXTA_LOG("wire_inputpipe_new\n");
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
    char *id = jxta_pipe_adv_get_Id((Jxta_pipe_adv *) self->adv);
    JXTA_OBJECT_CHECK_VALID(obj);
    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_LOG("wire_outputpipe_send\n");
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
        JXTA_LOG("Invalid argument\n");
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

static void wire_service_rdv_listener(Jxta_object * obj, void *arg)
{

    Jxta_status res = JXTA_SUCCESS;
    Jxta_wire_service *self = (Jxta_wire_service *) arg;
    char **ids = NULL;
    int i = 0;
    Jxta_pipe_adv *pipeadv = NULL;

    JXTA_LOG("Got a RDV new connect event push SRDI entries\n");

    /*
     * Update SRDI indexes for advertisement in our cm 
     * to our new RDV
     */
    ids = jxta_hashtable_keys_get(self->listeners);

    if (ids == NULL) {
        JXTA_LOG("no input pipe open\n");
        return;
    }

    while (ids[i]) {
        pipeadv = jxta_pipe_adv_new();
        jxta_pipe_adv_set_Id(pipeadv, ids[i]);
        jxta_pipe_adv_set_Type(pipeadv, (char *) JXTA_PROPAGATE_PIPE);
        JXTA_LOG("push pipe SRDI %s\n", ids[i]);
        res = jxta_pipe_resolver_send_srdi_message(self->generic_resolver, pipeadv, NULL, TRUE);
        if (res != JXTA_SUCCESS)
            JXTA_LOG("failed to push SRDI entries to new RDV\n");
        JXTA_OBJECT_RELEASE(pipeadv);
        i++;
    }
    free(ids);
}
