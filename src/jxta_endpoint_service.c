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

static const char *__log_cat = "ENDPOINT";

#include <stdlib.h> /* malloc, free */
#include <assert.h>

#include "jxta_apr.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "queue.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_service_private.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_endpoint_service.h"
#include "jxta_endpoint_address.h"
#include "jxta_router_service.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_stdpg_private.h"
#include "jxta_routea.h"
#include "jxta_log.h"
#include "jxta_apr.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_traffic_shaping_priv.h"
#include "jxta_util_priv.h"
#include "jxta_flow_control_msg.h"

/* negative cache entry */
typedef struct _nc_entry {
    char *addr;
    Jxta_time_diff timeout;
    Jxta_time expiration;
    Jxta_message **rq;
    int rq_head;
    int rq_tail;
    int rq_capacity;
} Nc_entry;

/* outgoing message task for thread pool */
typedef struct _msg_task {
    Jxta_endpoint_service *ep_svc;
    Jxta_message *msg;
    struct _msg_task *recycle;
} Msg_task;

/* recipient callback */
typedef struct _cb_elt {
    Jxta_endpoint_service *ep_svc;
    Jxta_callback_func func;
    void *arg;
    char *recipient;
    struct _cb_elt *recycle;
} Cb_elt;

/* transport connection */
typedef struct tc_elt {
    apr_pollfd_t fd;
    Jxta_callback_fn fn;
    void * arg;
} Tc_elt;

typedef struct peer_route_elt {
    char * ta;
    Jxta_ep_flow_control *ep_fc;
    JxtaEndpointMessenger * msgr;
} Peer_route_elt;

struct jxta_endpoint_service {
    Extends(Jxta_service);

    apr_thread_mutex_t *mutex;

    Jxta_PG *my_group;
    JString *my_groupid;
    Jxta_advertisement *my_impl_adv;
    Jxta_EndPointConfigAdvertisement *config;
    Jxta_RouteAdvertisement *myRoute;
    char *relay_addr;
    char *relay_proto;
    Jxta_transport *router_transport;
    Jxta_hashtable *transport_table;
    void *ep_msg_cookie;
    void *fc_msg_cookie;

    /* Current messengers */
    /* Each messenger could have two entries, one for peer ID, the other with transport addr */
    apr_hash_t *messengers;

    /* Outgoing messages */
    Msg_task *recycled_tasks;
    volatile apr_uint32_t msg_task_cnt;

    /* Negatice cache */
    apr_thread_mutex_t *nc_wlock; /* write lock for nc, use mutex for read lock */
    apr_hash_t *nc;             /* negative cache */

    /* Endpoint demux */
    apr_thread_mutex_t *demux_mutex;
    apr_hash_t *cb_table;       /* callback table for recipients */
    Cb_elt *recycled_cbs;
    Dlist *filter_list;
    apr_hash_t *listener_table;

    /* Nonblocking I/O */
    volatile apr_size_t pollfd_cnt;
    apr_pollset_t *pollset;

    /* Endpoint service gets its own threadpool */
    apr_pool_t *mypool;
    apr_thread_pool_t *thd_pool;

    Jxta_traffic_shaping *ts;
};

struct _jxta_ep_flow_control {
    JXTA_OBJECT_HANDLE;
    Jxta_PG *group;
    Jxta_endpoint_address *ea;
    Jxta_traffic_shaping *ts;
    char *msg_type;
    Jxta_boolean inbound;
    Jxta_boolean outbound;
    int rate;
    int rate_window;
    Jxta_time expect_delay;
    apr_int64_t msg_size;
};

static Jxta_listener *lookup_listener(Jxta_endpoint_service * me, Jxta_endpoint_address * addr);
static void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * thread, void *arg);
static Jxta_status outgoing_message_process(Jxta_endpoint_service * me, Jxta_message * msg);
static Jxta_status send_message(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest);

/* negative cache table operations */
static Nc_entry *nc_add_peer(Jxta_endpoint_service * me, const char *addr, Jxta_message * msg);
static void nc_remove_peer(Jxta_endpoint_service * me, Nc_entry * ptr);
static void nc_destroy(Jxta_endpoint_service * me);
static Jxta_status nc_peer_queue_msg(Jxta_endpoint_service * me, Nc_entry * ptr, Jxta_message * msg);
static void nc_review_all(Jxta_endpoint_service * me);

/* transport connection ops */
static Jxta_status tc_new(Tc_elt ** me, Jxta_callback_fn fn, apr_socket_t * s, void * arg, apr_pool_t * pool);
static Jxta_status tc_destroy(Tc_elt * me);

static void messenger_add(Jxta_endpoint_service * me, Jxta_endpoint_address *ea, JxtaEndpointMessenger * msgr);
static void messenger_remove(Jxta_endpoint_service * me, Jxta_endpoint_address *ea);
static void messengers_destroy(Jxta_endpoint_service * me);
Jxta_status endpoint_messenger_get(Jxta_endpoint_service * me, Jxta_endpoint_address * dest
                            , JxtaEndpointMessenger **msgr);
static Jxta_status get_messenger(Jxta_endpoint_service * me, Jxta_endpoint_address * dest
                            , Jxta_boolean check_msg_size, JxtaEndpointMessenger **msgr);

static Jxta_status JXTA_STDCALL endpoint_message_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL flow_control_message_cb(Jxta_object *obj, void *arg);
static void demux_ep_flow_control_message(Jxta_endpoint_service * me
                                    , Jxta_endpoint_address *dest, Jxta_ep_flow_control_msg * ep_fc_msg);
/* event operations */
static Jxta_status event_new(Jxta_endpoint_event ** me, Jxta_endpoint_service * ep, Jxta_endpoint_event_type type,
                             size_t extra_size);
static void event_free(Jxta_object * me);
static Jxta_status emit_local_route_change_event(Jxta_endpoint_service * me);

static void event_free(Jxta_object * me)
{
    Jxta_endpoint_event *myself = (Jxta_endpoint_event *) me;
    if (myself->extra) {
        free(myself->extra);
    }
    free(myself);
}

static Jxta_status event_new(Jxta_endpoint_event ** me, Jxta_endpoint_service * ep, Jxta_endpoint_event_type type,
                             size_t extra_size)
{
    Jxta_endpoint_event *event;

    event = calloc(1, sizeof(Jxta_endpoint_event));
    if (!event) {
        *me = NULL;
        return JXTA_NOMEM;
    }
    if (extra_size) {
        event->extra = calloc(1, extra_size);
        if (!event->extra) {
            free(event);
            *me = NULL;
            return JXTA_NOMEM;
        }
    }

    event->ep_svc = ep;
    event->type = type;
    JXTA_OBJECT_INIT(event, event_free, NULL);
    *me = event;
    return JXTA_SUCCESS;
}

static Jxta_status emit_local_route_change_event(Jxta_endpoint_service * me)
{
    Jxta_status rv;
    Jxta_endpoint_event *event;

    rv = event_new(&event, me, JXTA_EP_LOCAL_ROUTE_CHANGE, 0);
    if (!event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to allocate endpoint event.\n");
        return rv;
    }
    jxta_service_event_emit((Jxta_service *) me, event);
    JXTA_OBJECT_RELEASE(event);
    return JXTA_SUCCESS;
}

static Nc_entry *nc_add_peer(Jxta_endpoint_service * me, const char *addr, Jxta_message * msg)
{
    Nc_entry *ptr = NULL;

    if ( addr == NULL || me == NULL ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Invalid argument to nc_add_peer\n");
        return NULL;
    }

    apr_thread_mutex_lock(me->nc_wlock);
    ptr = calloc(1, sizeof(*ptr));
    if (NULL == ptr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to allocate negative cache entry, out of memory?\n");
        apr_thread_mutex_unlock(me->nc_wlock);
        return NULL;
    }
    ptr->addr = strdup(addr);
    if (NULL == ptr->addr) {
        free(ptr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to duplicate address, out of memory?\n");
        apr_thread_mutex_unlock(me->nc_wlock);
        return NULL;
    }
    ptr->timeout = jxta_epcfg_get_nc_timeout_init(me->config);
    ptr->expiration = jpr_time_now() + ptr->timeout;
    ptr->rq_capacity = jxta_epcfg_get_ncrq_size(me->config);
    if (0 != ptr->rq_capacity) {
        ptr->rq = calloc(ptr->rq_capacity, sizeof(*ptr->rq));
        if (NULL == ptr->rq) {
            free(ptr->addr);
            free(ptr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to allocate retransmit queue, out of memory?\n");
            apr_thread_mutex_unlock(me->nc_wlock);
            return NULL;
        }
        ptr->rq[0] = JXTA_OBJECT_SHARE(msg);
        ptr->rq_tail = 1;
    }
    apr_thread_mutex_lock(me->mutex);
    /* apr_hash_set(me->nc, ptr->addr, APR_HASH_KEY_STRING, ptr); */
    free(ptr->addr);
    free(ptr->rq);
    free(ptr);
    apr_thread_mutex_unlock(me->mutex);
    apr_thread_mutex_unlock(me->nc_wlock);
    return ptr;
}

static void nc_remove_peer(Jxta_endpoint_service * me, Nc_entry * ptr)
{
    if (NULL == ptr->rq || -1 == ptr->rq_tail) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "trying to remove peer from the nc but the queue is null or tail is unset [%pp]\n", ptr);
        return;
    }

    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    apr_hash_set(me->nc, ptr->addr, APR_HASH_KEY_STRING, NULL);
    apr_thread_mutex_unlock(me->mutex);
    apr_thread_mutex_unlock(me->nc_wlock);
    free(ptr->addr);
    free(ptr->rq);
    free(ptr);
}

static void nc_peer_drop_retransmit_queue(Nc_entry * me)
{
    if (NULL == me->rq) {
        return;
    }

    if (-1 != me->rq_tail) {
        do {
            Jxta_message *msg = NULL;

            assert(me->rq_capacity > me->rq_head);
            msg = me->rq[me->rq_head++];
            assert(NULL != msg);
            JXTA_OBJECT_RELEASE(msg);
            if (me->rq_head == me->rq_capacity) {
                me->rq_head = 0;
            }
        } while (me->rq_head != me->rq_tail);
    }

    free(me->rq);
    me->rq = NULL;
}

static void nc_destroy(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Nc_entry *ptr = NULL;

    for (hi = apr_hash_first(NULL, me->nc); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, NULL, NULL, (void **) ((void *)&ptr));
        if (NULL != ptr)
        {
        nc_peer_drop_retransmit_queue(ptr);
        free(ptr->addr);
        free(ptr);
    }
        else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "nc entry already null for destroy call [%pp]\n", ptr);
        }
    }
}

/**
 * Review the negative cache entry.
 * Try to send messages in the retransmit queue after timeout
 * In case failed, double the timeout period. If the timeout period is already maximum, drop the queue completely
 * @return JXTA_SUCCESS if the peer is out of timeout period and no msg remains in the queue. JXTA_UNREACHABLE_DEST if the peer
 * still is unreachable.
 */
static Jxta_status nc_peer_process_queue(Jxta_endpoint_service * me, Nc_entry * ptr)
{
    Jxta_message *msg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    int sent = 0;

    if (jpr_time_now() < ptr->expiration) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still in timeout for another %" APR_INT64_T_FMT "ms.\n",
                        ptr->addr, ptr->expiration - jpr_time_now());
        return JXTA_UNREACHABLE_DEST;
    }

    if (!ptr->rq || -1 == ptr->rq_tail) {
        return JXTA_SUCCESS;
    }

    do {

        msg = ptr->rq[ptr->rq_head];
        res = send_message(me, msg, NULL);
        if (JXTA_SUCCESS != res) {
            break;
        }
        JXTA_OBJECT_RELEASE(msg);
        ++sent;
        if (++ptr->rq_head >= ptr->rq_capacity) {
            ptr->rq_head = 0;
        }
    } while (ptr->rq_head != ptr->rq_tail);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is now reachable after timeout %" APR_INT64_T_FMT
                        "ms, retransmit queue is flushed.\n", ptr->addr, ptr->timeout);
        ptr->rq_tail = -1;
    } else if (!sent && JXTA_UNREACHABLE_DEST == res) {
        Jxta_time_diff timeout_max;

        timeout_max = jxta_epcfg_get_nc_timeout_max(me->config);
        if (ptr->timeout >= timeout_max) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still unreachable after timeout %" APR_INT64_T_FMT
                            "ms over maximum %" APR_INT64_T_FMT "ms, drop retransmit queue.\n", ptr->addr, ptr->timeout,
                            timeout_max);
            nc_peer_drop_retransmit_queue(ptr);
            ptr->timeout = timeout_max;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is still unreachable after timeout %" APR_INT64_T_FMT
                            "ms, double it(limit to a maximum of %" APR_INT64_T_FMT "ms).\n", ptr->addr, ptr->timeout,
                            timeout_max);
            ptr->timeout *= 2;
            if (ptr->timeout > timeout_max) {
                ptr->timeout = timeout_max;
            }
        }
        ptr->expiration = jpr_time_now() + ptr->timeout;
    }
    return res;
}

static Jxta_status nc_peer_queue_msg(Jxta_endpoint_service * me, Nc_entry * ptr, Jxta_message * msg)
{
    Jxta_status res;

    apr_thread_mutex_lock(me->nc_wlock);
    res = nc_peer_process_queue(me, ptr);
    if (JXTA_SUCCESS == res) {
        if (JXTA_UNREACHABLE_DEST != send_message(me, msg, NULL)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
            apr_thread_mutex_unlock(me->nc_wlock);
            return res;
        }
    }

    if (NULL == ptr->rq) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is still unreachable with maximum timeout %" APR_INT64_T_FMT
                        "ms, drop message[%pp]\n", ptr->addr, ptr->timeout, msg);
        apr_thread_mutex_unlock(me->nc_wlock);
        return JXTA_UNREACHABLE_DEST;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Queueing msg[%pp] for peer at %s for later retransmission.\n", msg,
                    ptr->addr);
    if (ptr->rq_head == ptr->rq_tail) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                        "Peer at %s has filled up(%d) retransmit queue, drop oldest message[%pp]\n", ptr->addr, ptr->rq_capacity,
                        ptr->rq[ptr->rq_head]);
        JXTA_OBJECT_RELEASE(ptr->rq[ptr->rq_head]);
        ptr->rq[ptr->rq_head++] = JXTA_OBJECT_SHARE(msg);
        if (ptr->rq_head >= ptr->rq_capacity) {
            ptr->rq_head = 0;
        }
    } else {
        if (-1 == ptr->rq_tail) {
            ptr->rq_tail = ptr->rq_head;
        }
        ptr->rq[ptr->rq_tail++] = JXTA_OBJECT_SHARE(msg);
        if (ptr->rq_tail >= ptr->rq_capacity) {
            ptr->rq_tail = 0;
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
    return JXTA_UNREACHABLE_DEST;
}

static void nc_review_all(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Nc_entry *ptr = NULL;
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(me->nc_wlock);
    for (hi = apr_hash_first(NULL, me->nc); hi;) {
        apr_hash_this(hi, NULL, NULL, (void **) ((void *)&ptr));
        /* move index before calling nc_remove_peer which removes entry from cache table and could screw the index */
        hi = apr_hash_next(hi);
        if(NULL != ptr) {
        res = nc_peer_process_queue(me, ptr);
        if (JXTA_SUCCESS == res && NULL != ptr->rq) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
        }
    }
        else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Peer is null in review of nc [%pp]\n", me);
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
}

/* transport connection ops */
static Jxta_status tc_new(Tc_elt ** me, Jxta_callback_fn fn, apr_socket_t * s, void * arg, apr_pool_t * pool)
{
    apr_pool_t * p;

    *me = calloc(1, sizeof(**me));
    if (! *me) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }
    if (APR_SUCCESS != apr_pool_create(&p, pool)) {
        free(*me);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Out of memory\n");
        return JXTA_NOMEM;
    }

    (*me)->fn = fn;
    (*me)->arg = arg;
    (*me)->fd.p = p;
    (*me)->fd.desc.s = s;
    (*me)->fd.desc_type = APR_POLL_SOCKET;
    (*me)->fd.client_data = *me;
    (*me)->fd.reqevents = APR_POLLIN | APR_POLLPRI;
    return JXTA_SUCCESS;
}

static Jxta_status tc_destroy(Tc_elt * me)
{
    apr_pool_destroy(me->fd.p);
    free(me);
    return JXTA_SUCCESS;
}

static int number_of_connected_peers(Jxta_endpoint_service * service)
{
    int count=0;

    jxta_endpoint_service_get_connection_peers(service, NULL, NULL, NULL, &count);

    return count;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_fc(Jxta_endpoint_service * me
                                , Jxta_endpoint_address *ea, Jxta_boolean normal)
{
    Jxta_status res=JXTA_FAILED;
    Jxta_message *msg;
    Jxta_message_element *msg_elem;
    JString *ep_fc_msg_xml=NULL;
    Jxta_ep_flow_control_msg *ep_fc_msg;
    Jxta_endpoint_address *send_ea=NULL;
    JxtaEndpointMessenger *msgr=NULL;
    Jxta_id *pid;
    apr_int64_t num_bytes;
    int frame_seconds;
    int connected_peers;
    int time;
    char *ea_string;

    send_ea = jxta_endpoint_address_new_4(ea, "Endpoint", "FlowControl");
    frame_seconds = traffic_shaping_frame(me->ts);
    num_bytes =  traffic_shaping_size(me->ts);

    time = traffic_shaping_time(me->ts);
    endpoint_messenger_get(me, send_ea, &msgr);
    connected_peers = number_of_connected_peers(me);
    if (!normal) {
        num_bytes = ((float) 40 * num_bytes/100);
    }

    if (NULL == msgr) {
        goto FINAL_EXIT;
    }
    if (msgr->fc_frame_seconds == frame_seconds
                  && msgr->fc_num_bytes == num_bytes) {
        if (msgr->fc_msgs_sent++ > 3) {
            goto FINAL_EXIT;
        }
    } else {
        msgr->fc_msgs_sent = 0;
    }
    msg = jxta_message_new();


    ep_fc_msg = jxta_ep_flow_control_msg_new_1(time, num_bytes, 1, frame_seconds, 0, 5, TS_MAX_OPTION_FRAME);
    jxta_PG_get_PID(me->my_group, &pid);

    jxta_ep_flow_control_msg_set_peerid(ep_fc_msg, pid);
    jxta_ep_flow_control_msg_get_xml(ep_fc_msg, &ep_fc_msg_xml);

    msg_elem = jxta_message_element_new_2(JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_FLOWCONTROL_MSG_ELEMENT_NAME, "text/xml", jstring_get_string(ep_fc_msg_xml), jstring_length(ep_fc_msg_xml), NULL);

    jxta_message_add_element(msg, msg_elem);

    ea_string = jxta_endpoint_address_to_string(send_ea);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE
                        , "Send FC - %s bytes:%" APR_INT64_T_FMT " to:%s \n%s\n"
                        , normal == TRUE ? "normal":"reduced"
                        , num_bytes, ea_string, jstring_get_string(ep_fc_msg_xml));
    free(ea_string);

    res = jxta_endpoint_service_send_ex(me, msg, send_ea, JXTA_TRUE, NULL);
    if (JXTA_SUCCESS == res) {
        msgr->fc_num_bytes = num_bytes;
        msgr->fc_frame_seconds = frame_seconds;
    }
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(msg_elem);
    JXTA_OBJECT_RELEASE(ep_fc_msg);
    JXTA_OBJECT_RELEASE(pid);

FINAL_EXIT:
    if (ep_fc_msg_xml)
        JXTA_OBJECT_RELEASE(ep_fc_msg_xml);
    if (msgr)
        JXTA_OBJECT_RELEASE(msgr);
    JXTA_OBJECT_RELEASE(send_ea);
    return res;
}

static void messenger_add(Jxta_endpoint_service * me, Jxta_endpoint_address *ea, JxtaEndpointMessenger *msgr)
{
    char * ta;
    Peer_route_elt *ptr;

    ta = jxta_endpoint_address_get_transport_addr(ea);

    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);

    if (ptr) {
        /* hardcode TCP preference */
        if (0 == strncasecmp("tcp://", ptr->ta, 6) && strcasecmp("tcp", jxta_endpoint_address_get_protocol_name(ea))) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE,
                            "Keep current messenger[%pp] which is preferred than new one[%pp] for peer at %s.\n", 
                            ptr->msgr, msgr, ta);
        } else {

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE 
                            "The old messenger[%pp] will be replaced with new one[%pp] for peer at %s.\n", 
                            ptr->msgr, msgr, ta);

            JXTA_OBJECT_RELEASE(ptr->msgr);
            ptr->msgr = JXTA_OBJECT_SHARE(msgr);
        }
        free(ta);
        return;
    }

    ptr = calloc(1, sizeof(*ptr));
    if (!ptr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        free(ta);
        return;
    }

    apr_hash_set(me->messengers, ta, APR_HASH_KEY_STRING, ptr);

    ptr->ta = ta;
    ptr->msgr = JXTA_OBJECT_SHARE(msgr);
    ptr->ep_fc = jxta_ep_flow_control_new();
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Add messenger [%pp]: %s\n", ptr->msgr, ta);

}

static void messenger_remove(Jxta_endpoint_service * me, Jxta_endpoint_address *ea)
{
    char * ta;
    Peer_route_elt *ptr;
#ifdef UNUSED_VWF
    apr_ssize_t len;
#endif
    ta = jxta_endpoint_address_get_transport_addr(ea);
    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);
    free(ta);
    if (ptr) {
        apr_hash_set(me->messengers, ptr->ta, APR_HASH_KEY_STRING, NULL);
        free(ptr->ta);
        JXTA_OBJECT_RELEASE(ptr->msgr);
        JXTA_OBJECT_RELEASE(ptr->ep_fc);
        free(ptr);
    }
}

static void messengers_destroy(Jxta_endpoint_service * me)
{
    apr_hash_index_t *hi = NULL;
    Peer_route_elt *ptr;
    const char *ta;

    for (hi = apr_hash_first(NULL, me->messengers); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, (const void **) ((void *)&ta), NULL, (void **) ((void *)&ptr));
        if( ptr != NULL && ta == ptr->ta) {
        apr_hash_set(me->messengers, ptr->ta, APR_HASH_KEY_STRING, NULL);

        free(ptr->ta);
        JXTA_OBJECT_RELEASE(ptr->ep_fc);
        JXTA_OBJECT_RELEASE(ptr->msgr);
        free(ptr);
    }
        else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "messenger already null before destroy of messengers [%pp]\n", me);
        }   
    }
}

/*
 * Implementations for module methods
 */
static Jxta_status endpoint_init(Jxta_module * it, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_id *gid=NULL;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    apr_pool_t *pool;
    Jxta_PG *parentgroup;
    Jxta_traffic_shaping *ts=NULL;
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing ...\n");

    jxta_service_init((Jxta_service *) self, group, assigned_id, impl_adv);
    pool = jxta_PG_pool_get(group);
    self->listener_table = apr_hash_make(pool);
    self->nc = apr_hash_make(pool);
    self->cb_table = apr_hash_make(pool);
    self->messengers = apr_hash_make(pool);
    self->filter_list = dl_make();
    self->transport_table = jxta_hashtable_new(2);
    if (self->transport_table == NULL || self->filter_list == NULL) {
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, pool);
    apr_thread_mutex_create(&self->nc_wlock, APR_THREAD_MUTEX_NESTED, pool);
    apr_thread_mutex_create(&self->demux_mutex, APR_THREAD_MUTEX_NESTED, pool);

    self->msg_task_cnt = 0;

    /* advs and groups are jxta_objects that we share */
    if (impl_adv != NULL) {
        JXTA_OBJECT_SHARE(impl_adv);
    }

    self->my_impl_adv = impl_adv;
    self->my_group = group;
    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &(self->my_groupid));
    self->relay_addr = NULL;
    self->relay_proto = NULL;
    self->myRoute = NULL;
    char *new_param;
    jxta_PG_get_configadv(group, &conf_adv);

    if (!conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        jxta_PA_get_Svc_with_id(conf_adv, jxta_endpoint_classid, &svc);
        if (NULL != svc) {
            self->config = jxta_svc_get_EndPointConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == self->config) {
        self->config = jxta_EndPointConfigAdvertisement_new();
        if (self->config == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
    }


    jxta_epcfg_get_traffic_shaping(self->config, &ts);
    traffic_shaping_init(ts);

    self->ts = JXTA_OBJECT_SHARE(ts);
    new_param = get_service_key("EndpointService:%groupid%", NULL);
    endpoint_service_add_recipient(self, &self->ep_msg_cookie, new_param, NULL
                                         , endpoint_message_cb, self);
    free(new_param);

    endpoint_service_add_recipient(self, &self->fc_msg_cookie, "Endpoint", "FlowControl"
                                         , flow_control_message_cb, self);


    apr_pollset_create(&self->pollset, 100, pool, 0);
    self->pollfd_cnt = 0;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");

FINAL_EXIT:

    if (gid)
        JXTA_OBJECT_RELEASE(gid);
    if (ts)
        JXTA_OBJECT_RELEASE(ts);
    return res;
}

static void* APR_THREAD_FUNC do_poll(apr_thread_t * thd, void * arg)
{
    Jxta_endpoint_service * me = arg;
    Jxta_status rv;
    Jxta_module_state running;

    /* Use 1 second timeout instead of blocking because:
     * a. poll won't return when close local listening socket as 9/1/2006
     * b. give this thread a chance to serve other tasks in thread pool
     */
    rv = endpoint_service_poll(me, 1000000L);
    running = jxta_module_state((Jxta_module*) me);

    if ((JXTA_MODULE_STARTED == running || JXTA_MODULE_STARTING == running) && me->pollfd_cnt) {
        apr_thread_pool_push(me->thd_pool, do_poll, me, APR_THREAD_TASK_PRIORITY_HIGHEST, me);
    }


    return JXTA_SUCCESS;
}

static Jxta_status endpoint_start(Jxta_module * me, const char *args[])
{
    /* construct + init have done everything already */
    Jxta_endpoint_service * myself = PTValid(me, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting ...\n");

    /* Endpoint service has own threadpool */
    /* TODO set num threads here when we have platformconfig setup */
    int init_threads = endpoint_config_threads_init(myself->config);
    int max_threads = endpoint_config_threads_maximum(myself->config);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing DONE THREADS\n");

    if (NULL == myself->mypool) {
        apr_pool_create(&myself->mypool, NULL);
        if (myself->mypool == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot create pool for threadpools.");
        }
    }

    if (NULL == myself->thd_pool) {
        apr_thread_pool_create(&myself->thd_pool, init_threads, max_threads, myself->mypool);
        if (myself->thd_pool == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot create threadpool.");
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing DONE POOL, init_threads=%d, max_threads=%d\n",init_threads, max_threads);

    if (myself->pollfd_cnt) {
        apr_thread_pool_push(myself->thd_pool, do_poll, myself, APR_THREAD_TASK_PRIORITY_HIGHEST, myself);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started\n");

    return JXTA_SUCCESS;
}

static void endpoint_stop(Jxta_module * self)
{
    Jxta_endpoint_service *me = PTValid(self, Jxta_endpoint_service);

    /* stop the thread that processes outgoing messages */
    apr_thread_pool_tasks_cancel(me->thd_pool, me);
    endpoint_service_remove_recipient(me, me->ep_msg_cookie);
    endpoint_service_remove_recipient(me, me->fc_msg_cookie);
    if(me->thd_pool) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "apr_thread_pool_destroy(service->thd_pool)[%pp];\n", me->thd_pool );
        apr_thread_pool_destroy(me->thd_pool);
        me->thd_pool = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "finished apr_thread_pool_destroy(service->thd_pool);\n");
    }

    if (NULL != me->router_transport) {
        JXTA_OBJECT_RELEASE(me->router_transport);
        me->router_transport = NULL;
    }
    
    /* delete tables and stuff */
    nc_destroy(me);
    messengers_destroy(me);
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

/*
 * implementations for service methods
 */
static void endpoint_get_MIA(Jxta_service * it, Jxta_advertisement ** mia)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    *mia = JXTA_OBJECT_SHARE(self->my_impl_adv);
}

static void endpoint_get_interface(Jxta_service * it, Jxta_service ** serv)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    JXTA_OBJECT_SHARE(self);
    *serv = (Jxta_service *) self;
}

/*
 * Endpoint service is not virtual yet : there is no way to provide
 * more than one implementation. Therefore its methods table is just
 * the same than Jxta_service's.
 */
typedef Jxta_service_methods Jxta_endpoint_service_methods;

static const Jxta_endpoint_service_methods jxta_endpoint_service_methods = {
    {
     "Jxta_module_methods",
     endpoint_init,
     endpoint_start,
     endpoint_stop},
    "Jxta_service_methods",
    endpoint_get_MIA,
    endpoint_get_interface,
    service_on_option_set
};

typedef struct _Filter Filter;

struct _Filter {
    char *str;
    JxtaEndpointFilter func;
    void *arg;
};

void jxta_endpoint_service_destruct(Jxta_endpoint_service * service)
{
    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    JXTA_OBJECT_RELEASE(endpoint_service->transport_table);

    if (endpoint_service->relay_addr)
        free(endpoint_service->relay_addr);
    if (endpoint_service->relay_proto)
        free(endpoint_service->relay_proto);
    dl_free(endpoint_service->filter_list, free);

    apr_thread_mutex_destroy(endpoint_service->nc_wlock);
    apr_thread_mutex_destroy(endpoint_service->demux_mutex);
    apr_thread_mutex_destroy(endpoint_service->mutex);

    /* un-ref our impl_adv, and delete assigned_id */
    endpoint_service->my_group = NULL;

    if (endpoint_service->my_groupid) {
        JXTA_OBJECT_RELEASE(endpoint_service->my_groupid);
        endpoint_service->my_groupid = NULL;
    }
    if (endpoint_service->my_impl_adv) {
        JXTA_OBJECT_RELEASE(endpoint_service->my_impl_adv);
        endpoint_service->my_impl_adv = NULL;
    }

    if (endpoint_service->myRoute != NULL) {
        JXTA_OBJECT_RELEASE(endpoint_service->myRoute);
        endpoint_service->myRoute = NULL;
    }

    if (endpoint_service->config != NULL) {
        JXTA_OBJECT_RELEASE(endpoint_service->config);
        endpoint_service->config = NULL;
    }

    if (endpoint_service->ts != NULL) {
        JXTA_OBJECT_RELEASE(endpoint_service->ts);
        endpoint_service->ts = NULL;
    }

    endpoint_service->thisType = NULL;

    jxta_service_destruct((Jxta_service *) endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}

Jxta_endpoint_service *jxta_endpoint_service_construct(Jxta_endpoint_service * service,
                                                       const Jxta_endpoint_service_methods * methods)
{
    Jxta_service_methods* service_methods = PTValid(methods, Jxta_service_methods);

    jxta_service_construct((Jxta_service *) service, (const Jxta_service_methods *) service_methods);
    service->thisType = "Jxta_endpoint_service";

    service->my_impl_adv = NULL;
    service->my_group = NULL;
    service->my_groupid = NULL;
    service->router_transport = NULL;
    service->thd_pool = NULL;
    service->mypool = NULL;

    return service;
}

static void jxta_endpoint_service_free(Jxta_object * obj)
{
    jxta_endpoint_service_destruct((Jxta_endpoint_service *) obj);

    memset((void *) obj, 0xdd, sizeof(Jxta_endpoint_service));
    free((void *) obj);
}

Jxta_endpoint_service *jxta_endpoint_service_new_instance(void)
{
    Jxta_endpoint_service *service = (Jxta_endpoint_service *) calloc(1, sizeof(Jxta_endpoint_service));
    if (service == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }
    JXTA_OBJECT_INIT(service, jxta_endpoint_service_free, 0);

    if (NULL == jxta_endpoint_service_construct(service, &jxta_endpoint_service_methods)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        free(service);
        service = NULL;
    }

    return service;
}

Jxta_status endpoint_service_demux(Jxta_endpoint_service * me, const char *name, const char *param, Jxta_message * msg)
{
    char *key; 
    Jxta_status rv;
    Cb_elt *cb;

    key = get_service_key(name, param);
    if (NULL == key) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No key found for name:%s param:%s \n", name, param);
        rv = JXTA_ITEM_NOTFOUND;
        goto FINAL_EXIT;
    }
    apr_thread_mutex_lock(me->demux_mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Looking up callback for %s\n", key);

    cb = apr_hash_get(me->cb_table, key, APR_HASH_KEY_STRING);
    if (!cb && param) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Looking up callback for fallback %s\n", name);
        cb = apr_hash_get(me->cb_table, name, APR_HASH_KEY_STRING);
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    if (cb) {
        rv = cb->func((Jxta_object *) msg, cb->arg);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux: No callback found for %s.\n", key);
        rv = JXTA_ITEM_NOTFOUND;
    }

FINAL_EXIT:
    if (NULL != key)
        free(key);
    return rv;
}

static void demux_ep_flow_control_message(Jxta_endpoint_service * me
                                    , Jxta_endpoint_address *dest, Jxta_ep_flow_control_msg * ep_fc_msg)
{

    JxtaEndpointMessenger *msgr=NULL;
    Jxta_id *peer_id=NULL;
    Jxta_endpoint_address *fc_ea=NULL;
    JString *peerid_j=NULL;


    jxta_ep_flow_control_msg_get_peerid(ep_fc_msg, &peer_id);
    if (NULL == peer_id) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received a fc msg [%pp] without a peer id\n", ep_fc_msg);
        goto FINAL_EXIT;
    }
    jxta_id_to_jstring(peer_id, &peerid_j);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux flow control message [%pp] from %s\n"
                    , ep_fc_msg, jstring_get_string(peerid_j));

    fc_ea = jxta_endpoint_address_new_3(peer_id, NULL, NULL);
    if (JXTA_SUCCESS == get_messenger(me, fc_ea, FALSE, &msgr)) {
        Jxta_traffic_shaping *ts;
        char *ep_str=NULL;

        ep_str = jxta_endpoint_address_to_string(fc_ea);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Found messenger [%pp] for %s\n", msgr, ep_str);

        apr_thread_mutex_lock(msgr->mutex);
        if (NULL != msgr->ts) {
            JXTA_OBJECT_RELEASE(msgr->ts);
            msgr->ts = NULL;
        }
        msgr->ts = traffic_shaping_new();
        ts = msgr->ts;
        traffic_shaping_lock(ts);

        traffic_shaping_set_interval(ts, traffic_shaping_interval(me->ts));
        traffic_shaping_set_frame(ts, traffic_shaping_frame(me->ts));

        traffic_shaping_set_size(ts, jxta_ep_flow_control_msg_get_size(ep_fc_msg));
        traffic_shaping_set_time(ts, jxta_ep_flow_control_msg_get_time(ep_fc_msg));
        traffic_shaping_set_look_ahead(ts, jxta_ep_flow_control_msg_get_look_ahead(ep_fc_msg));
        traffic_shaping_set_reserve(ts, jxta_ep_flow_control_msg_get_reserve(ep_fc_msg));
        traffic_shaping_set_max_option(ts, jxta_ep_flow_control_msg_get_max_option(ep_fc_msg));
        traffic_shaping_init(ts);
        traffic_shaping_unlock(ts);
        apr_thread_mutex_unlock(msgr->mutex);
        if (ep_str)
            free(ep_str);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Couldn't find messenger for flow control msg[%pp]\n"
                        , ep_fc_msg);
    }

FINAL_EXIT:
    if (peerid_j)
        JXTA_OBJECT_RELEASE(peerid_j);
    if (fc_ea)
        JXTA_OBJECT_RELEASE(fc_ea);
    if (peer_id)
        JXTA_OBJECT_RELEASE(peer_id);
    if (msgr)
        JXTA_OBJECT_RELEASE(msgr);
}

static Jxta_status JXTA_STDCALL endpoint_message_cb(Jxta_object * obj, void *arg)
 {
    Jxta_status res = JXTA_SUCCESS;
    Jxta_vector *entries=NULL;
    int i;
    Jxta_bytevector *value=NULL;
    JString *string=NULL;
    Jxta_message_element *el_ep=NULL;
    Jxta_endpoint_message *ep_msg=NULL;
    Jxta_endpoint_address *dest=NULL;

    Jxta_endpoint_service *me = PTValid(arg, Jxta_endpoint_service);
    Jxta_message *msg = (Jxta_message *) obj;

    res = jxta_message_get_element_2(msg, JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_ENPOINT_MSG_ELEMENT_NAME, &el_ep);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Call back endpoint message Failed [%pp]\n", msg);
        goto FINAL_EXIT;
    }
    dest = jxta_message_get_destination(msg);
    value = jxta_message_element_get_value(el_ep);
    string = jstring_new_3(value);
    ep_msg = jxta_endpoint_msg_new();
    res = jxta_endpoint_msg_parse_charbuffer(ep_msg, jstring_get_string(string), jstring_length(string));
    jxta_endpoint_msg_get_entries(ep_msg, &entries);

    for (i=0; i<jxta_vector_size(entries); i++) {
        Jxta_hashtable *elem_hash=NULL;
        Jxta_message *new_msg=NULL;
        Jxta_message_element *el=NULL;
        char *dest_string=NULL;
        Jxta_endpoint_msg_entry_element *elem=NULL;
        Jxta_endpoint_address *new_dest=NULL;

        if (JXTA_SUCCESS != jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&elem), i)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to retrieve from [%pp] endpoint entry element at %d\n", entries , i);
            continue;
        }

        jxta_endpoint_msg_entry_get_value(elem, &el);

        new_msg = jxta_message_new();
        jxta_message_add_element(new_msg, el);
        jxta_endpoint_msg_entry_get_attributes(elem, &elem_hash);

        /* get the real destination */
        jxta_endpoint_address_replace_variables(dest, elem_hash, &new_dest);

        jxta_message_set_destination(new_msg, new_dest);

        /* demux the real destination */
        jxta_endpoint_service_demux_addr(me, new_dest, new_msg);

        free(dest_string);
        JXTA_OBJECT_RELEASE(elem_hash);
        JXTA_OBJECT_RELEASE(new_msg);
        JXTA_OBJECT_RELEASE(el);
        JXTA_OBJECT_RELEASE(new_dest);
        JXTA_OBJECT_RELEASE(elem);
    }

FINAL_EXIT:
    if (ep_msg)
        JXTA_OBJECT_RELEASE(ep_msg);
    if (el_ep)
        JXTA_OBJECT_RELEASE(el_ep);
    if (dest)
        JXTA_OBJECT_RELEASE(dest);
    if (value)
        JXTA_OBJECT_RELEASE(value);
    if (string)
        JXTA_OBJECT_RELEASE(string);
    if (entries)
        JXTA_OBJECT_RELEASE(entries);

    return res;
}

static Jxta_status JXTA_STDCALL flow_control_message_cb(Jxta_object *obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *dest=NULL;
    Jxta_message_element *el;

    Jxta_endpoint_service *me = PTValid(arg, Jxta_endpoint_service);
    Jxta_message *msg = (Jxta_message *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Flow control message call back\n");

    res = jxta_message_get_element_2(msg
        , JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_FLOWCONTROL_MSG_ELEMENT_NAME, &el);

    if (JXTA_SUCCESS == res) {
        Jxta_ep_flow_control_msg *ep_fc_msg=NULL;
        Jxta_bytevector *value;
        JString *string;

        value = jxta_message_element_get_value(el);
        string = jstring_new_3(value);
        ep_fc_msg = jxta_ep_flow_control_msg_new();
        jxta_ep_flow_control_msg_parse_charbuffer(ep_fc_msg
                        , jstring_get_string(string), jstring_length(string));

        demux_ep_flow_control_message(me, dest, ep_fc_msg);

        JXTA_OBJECT_RELEASE(value);
        JXTA_OBJECT_RELEASE(string);
        JXTA_OBJECT_RELEASE(ep_fc_msg);
    }
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(dest);
    return res;
}
JXTA_DECLARE(void) jxta_endpoint_service_demux(Jxta_endpoint_service * service, Jxta_message * msg)
{
    Jxta_endpoint_address *dest=NULL;
    Jxta_message_element  *el=NULL;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);
    dest = jxta_message_get_destination(msg);
    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to get message destination\n");
        return;
    }
    jxta_endpoint_service_demux_addr(endpoint_service, dest, msg);

    if (el)
        JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(dest);
}

JXTA_DECLARE(void) jxta_endpoint_service_demux_addr(Jxta_endpoint_service * service, Jxta_endpoint_address * dest,
                                                    Jxta_message * msg)
{
    Dlist *cur;
    Filter *cur_filter;
    Jxta_vector *vector = NULL;
    char *destStr;
    Jxta_listener *listener;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    if (JXTA_MODULE_STARTED != jxta_module_state((Jxta_module*)endpoint_service)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Endpoint stopped \n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux of message [%pp]\n", msg);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Destination of message [%pp] is NULL\n", msg);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(dest);

  /**
   * FIXME 4/11/2002 lomax@jxta.org
   *
   * The following code needs to be revisited:
   *    - a same message can go several times through demux. Filters must
   *      be invoked only once.
   **/

    /* pass message through appropriate filters */
    apr_thread_mutex_lock(endpoint_service->demux_mutex);

    dl_traverse(cur, endpoint_service->filter_list) {
        Jxta_message_element *el = NULL;
        cur_filter = cur->val;

        if ((vector = jxta_message_get_elements_of_namespace(msg, cur_filter->str)) != NULL
            || (JXTA_SUCCESS == jxta_message_get_element_1(msg, cur_filter->str, &el))) {

            JXTA_OBJECT_RELEASE(el);
            /* discard the message if the filter returned false */
            if (!cur_filter->func(msg, cur_filter->arg)) {
                apr_thread_mutex_unlock(endpoint_service->demux_mutex);
                return;
            }

        }
        if (vector != NULL) {
            JXTA_OBJECT_RELEASE(vector);
        }
    }

    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
    destStr = jxta_endpoint_address_to_string(dest);

    if (endpoint_service_demux(endpoint_service, jxta_endpoint_address_get_service_name(dest),
                               jxta_endpoint_address_get_service_params(dest), msg) != JXTA_ITEM_NOTFOUND) {
        goto FINAL_EXIT;
    }

    /* Todo: remove listener interface once transition to callback completed */
    listener = lookup_listener(endpoint_service, dest);

    if (listener != NULL) {
        jxta_listener_process_object(listener, (Jxta_object *) msg);

        JXTA_OBJECT_CHECK_VALID(msg);
        goto FINAL_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Demux: No demux listener for %s\n", destStr);
    /* end of listener section to be deprecated */
  FINAL_EXIT:
    free(destStr);
}

Jxta_status endpoint_service_poll(Jxta_endpoint_service * me, apr_interval_time_t timeout)
{
    apr_status_t rv;
    const apr_pollfd_t * fds;
    int i;
    apr_int32_t num_sockets;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "before the pollset poll\n");
    if (!(me->pollfd_cnt > 0)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Pollfd cnt was not greater than zero [%pp]\n", me);
        return JXTA_FAILED;
    }
    rv = apr_pollset_poll(me->pollset, timeout, &num_sockets, &fds);

/* Not sure if we can safely ignore when endpoint is stopping. Transport should still get a chance to handle it.
    if (! me->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service is stopped, skip poll.\n");
        return JXTA_SUCCESS;
    }
*/

    if (APR_SUCCESS != rv) {
        jxta_log_append(__log_cat, APR_STATUS_IS_TIMEUP(rv) ? JXTA_LOG_LEVEL_PARANOID : JXTA_LOG_LEVEL_DEBUG, 
                        "polling func return with status %d\n", rv);
        return rv;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "num sockets = %d\n", num_sockets);
    for (i = 0; i < num_sockets; i++) {
        int ev;
        Tc_elt * tc;

        ev = fds->rtnevents;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, 
                        "APR_POLLIN %d APR_POLLOUT %d APR_POLLPRI %d APR_POLLERR %d APR_POLLHUP %d APR_POLLNVAL %d\n", 
                        ev & APR_POLLIN ? 1:0, ev & APR_POLLOUT ? 1:0, ev & APR_POLLPRI ? 1:0, ev & APR_POLLERR  ? 1:0,
                        ev & APR_POLLHUP ? 1:0, ev & APR_POLLNVAL ? 1:0);
        tc = fds->client_data;
        rv = tc->fn(&tc->fd, tc->arg);
        fds++;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_endpoint_service_get_route_from_PA(Jxta_PA * padv, Jxta_RouteAdvertisement ** route)
{
    Jxta_svc *svc = NULL;

    jxta_PA_get_Svc_with_id(padv, jxta_endpoint_classid_get(), &svc);

    if (svc != NULL) {
        *route = jxta_svc_get_RouteAdvertisement(svc);
        JXTA_OBJECT_RELEASE(svc);
    }
}

/**
 * Add a transport to the endpoint service, if the transport supports inbound messages, update the router advertisement to the
 * local peer.
 */
JXTA_DECLARE(void) jxta_endpoint_service_add_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    Jxta_PA *pa;
    Jxta_vector *svcs;
    Jxta_RouteAdvertisement *route;
    Jxta_AccessPointAdvertisement *dest;
    Jxta_endpoint_address *address;
    Jxta_vector *addresses;
    Jxta_id *pid = NULL;
    JString *jaddress;
    char *caddress;
    Jxta_id *assigned_id;

    Jxta_svc *svc = NULL;
    JString *name;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);
    if (jxta_transport_allow_inbound(transport)) {
        /* Add the public address to the endpoint section of the PA */
        jxta_PG_get_PA(endpoint_service->my_group, &pa);
        assigned_id = jxta_service_get_assigned_ID_priv((Jxta_service *) endpoint_service);
        jxta_PA_get_Svc_with_id(pa, assigned_id, &svc);
        if (svc == NULL) {
            svc = jxta_svc_new();
            if (svc == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_svc_set_MCID(svc, assigned_id);
            svcs = jxta_PA_get_Svc(pa);
            jxta_vector_add_object_last(svcs, (Jxta_object *) svc);
            JXTA_OBJECT_RELEASE(svcs);
        }

        route = jxta_svc_get_RouteAdvertisement(svc);
        if (route == NULL) {
            route = jxta_RouteAdvertisement_new();
            if (route == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_svc_set_RouteAdvertisement(svc, route);
        }
        address = jxta_transport_publicaddr_get(transport);
        caddress = jxta_endpoint_address_to_string(address);
        jaddress = jstring_new_2(caddress);
        if (jaddress == NULL || caddress == NULL || address == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            JXTA_OBJECT_RELEASE(address);
            free(caddress);
            JXTA_OBJECT_RELEASE(pa);
            JXTA_OBJECT_RELEASE(svc);
            return;
        }
        dest = jxta_RouteAdvertisement_get_Dest(route);
        if (dest == NULL) {
            dest = jxta_AccessPointAdvertisement_new();
            if (dest == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(address);
                free(caddress);
                JXTA_OBJECT_RELEASE(jaddress);
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_PG_get_PID(endpoint_service->my_group, &pid);
            jxta_AccessPointAdvertisement_set_PID(dest, pid);
            jxta_RouteAdvertisement_set_Dest(route, dest);
            JXTA_OBJECT_RELEASE(pid);
        }

        addresses = jxta_AccessPointAdvertisement_get_EndpointAddresses(dest);
        if (addresses == NULL) {
            addresses = jxta_vector_new(2);
            if (addresses == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
                JXTA_OBJECT_RELEASE(addresses);
                free(caddress);
                JXTA_OBJECT_RELEASE(jaddress);
                JXTA_OBJECT_RELEASE(dest);
                JXTA_OBJECT_RELEASE(pa);
                JXTA_OBJECT_RELEASE(svc);
                return;
            }
            jxta_AccessPointAdvertisement_set_EndpointAddresses(dest, addresses);
        }

        jxta_vector_add_object_last(addresses, (Jxta_object *) jaddress);

        if (endpoint_service->myRoute != NULL)
            JXTA_OBJECT_RELEASE(endpoint_service->myRoute);

        endpoint_service->myRoute = route;
        free(caddress);

        JXTA_OBJECT_RELEASE(addresses);
        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(jaddress);
        JXTA_OBJECT_RELEASE(address);
        JXTA_OBJECT_RELEASE(svc);
        JXTA_OBJECT_RELEASE(pa);
    }

    /* add the transport to our table */
    /* FIXME: should check allow_overload etc. */
    name = jxta_transport_name_get(transport);
    jxta_hashtable_put(service->transport_table, jstring_get_string(name),  /* string gets copied */
                       jstring_length(name), (Jxta_object *) transport);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);
    if (jxta_transport_allow_inbound(transport)) {
        emit_local_route_change_event(endpoint_service);
    }
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_add_poll(Jxta_endpoint_service * me, apr_socket_t * s, Jxta_callback_fn fn,
                                                         void * arg, void ** cookie)
{
    Jxta_status rv;
    Tc_elt * tc;
    Jxta_module_state running;

    rv = tc_new(&tc, fn, s, arg, jxta_PG_pool_get(me->my_group));
    if (rv != JXTA_SUCCESS) {
        *cookie = NULL;
        return rv;
    }

    apr_thread_mutex_lock(me->mutex);
    rv = apr_pollset_add(me->pollset, &tc->fd);
    running = jxta_module_state((Jxta_module*) me);

    if (0 == me->pollfd_cnt++ && (JXTA_MODULE_STARTED == running || JXTA_MODULE_STARTING == running)) {
        apr_thread_pool_push(me->thd_pool, do_poll, me, APR_THREAD_TASK_PRIORITY_HIGHEST, me);
    }
    apr_thread_mutex_unlock(me->mutex);
    if (rv != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to add poll[%pp] with error %d\n", tc, rv);
        *cookie = NULL;
        tc_destroy(tc);
        return rv;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Add poll[%pp] with socket[%pp], total %d sockets to poll\n", tc, s,
                    me->pollfd_cnt);

    *cookie = tc;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_remove_poll(Jxta_endpoint_service * me, void * cookie)
{
    Jxta_status rv;
    Tc_elt *tc = cookie;

    apr_thread_mutex_lock(me->mutex);
    if (!(me->pollfd_cnt > 0)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Pollfd cnt was not greater than zero [%pp]\n", me);
        return JXTA_FAILED;
    }
    rv = apr_pollset_remove (me->pollset, &tc->fd);
    if (APR_SUCCESS == rv) {
        --me->pollfd_cnt;
    }
    apr_thread_mutex_unlock(me->mutex);
    if (APR_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to remove poll[%pp] with error %d\n", tc, rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Removed poll[%pp] with socket[%pp], remaining %d sockets to poll\n", 
                        tc, tc->fd.desc.s, me->pollfd_cnt);
    }
    tc_destroy(tc);
    return (APR_SUCCESS == rv) ? JXTA_SUCCESS : JXTA_FAILED;
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_endpoint_service_get_local_route(Jxta_endpoint_service * service)
{
    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (endpoint_service->myRoute != NULL)
        JXTA_OBJECT_SHARE(endpoint_service->myRoute);
    return endpoint_service->myRoute;
}

JXTA_DECLARE(void) jxta_endpoint_service_set_local_route(Jxta_endpoint_service * service, Jxta_RouteAdvertisement * route)
{
    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (endpoint_service->myRoute != NULL)
        JXTA_OBJECT_RELEASE(endpoint_service->myRoute);

    endpoint_service->myRoute = route;
    JXTA_OBJECT_SHARE(endpoint_service->myRoute);
    emit_local_route_change_event(endpoint_service);
}

JXTA_DECLARE(void) jxta_endpoint_service_remove_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    JString *name = jxta_transport_name_get(transport);
    Jxta_object *rmd = NULL;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);
    jxta_hashtable_del(endpoint_service->transport_table, jstring_get_string(name), jstring_length(name), &rmd);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);

    /*
     * FIXME 20040516 tra
     * We should update our route info here removing
     * the corresponding endpoint address
     */

    /* If trsp existed, release it */
    if (rmd != NULL)
        JXTA_OBJECT_RELEASE(rmd);
}

/* FIXME: transport_name should be a JString for consistency */
JXTA_DECLARE(Jxta_transport *) jxta_endpoint_service_lookup_transport(Jxta_endpoint_service * service, const char *transport_name)
{
    Jxta_transport *t = NULL;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    jxta_hashtable_get(endpoint_service->transport_table, transport_name, strlen(transport_name), JXTA_OBJECT_PPTR(&t));

    /*
     * This routine is allowed to return NULL if the requested transport
     * was not found. (Until we convert this code the convention of always
     * returning a status.
     */
    if (t != NULL) {
        JXTA_OBJECT_CHECK_VALID(t);
    }

    return t;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_connection_peers(Jxta_endpoint_service * service, const char * protocol, const char * address, Jxta_vector **endpoints, int *unique)
{
    apr_hash_index_t *hi = NULL;
    Peer_route_elt *ptr;
    const char *ta;
    Jxta_status res = JXTA_SUCCESS;
    unsigned int i;
    JxtaEndpointMessenger **found_entries=NULL;

    i = apr_hash_count(service->messengers);
    found_entries = calloc(i+1, sizeof(char *));
    found_entries[i] = NULL;
    endpoints != NULL ? *endpoints = jxta_vector_new(0):NULL;
    if (NULL != unique) {
        *unique = 0;
    }

    i = 0;
    apr_thread_mutex_lock(service->mutex);
    for (hi = apr_hash_first(NULL, service->messengers); hi; hi = apr_hash_next(hi)) {
        JxtaEndpointMessenger * msgr;
        Jxta_endpoint_address * addr;
        const char *p_prot;
        const char *p_addr;

        apr_hash_this(hi, (const void **)(void *)&ta, NULL,  (void **)(void *)&ptr);
        assert(NULL != ptr);
        assert(ta == ptr->ta);
        msgr = ptr->msgr;
        addr = msgr->address;

        p_prot = jxta_endpoint_address_get_protocol_name(addr);
        p_addr = jxta_endpoint_address_get_protocol_address(addr);

        if (NULL != unique) {
            int j=0;
            Jxta_boolean match=FALSE;

            while (found_entries[j]) {
                JxtaEndpointMessenger *check_msgr;
                Jxta_endpoint_address *check_addr;

                check_msgr = found_entries[j];
                check_addr = check_msgr->address;

                if (0 == strcmp(p_prot, jxta_endpoint_address_get_protocol_name(check_addr)) &&
                    0 == strcmp(p_addr, jxta_endpoint_address_get_protocol_address(check_addr))) {
                    match = TRUE;
                    break;

                }
                j++;
            }
            if (!match) {
                found_entries[j] = msgr;
                (*unique)++;
            }
        }

        if (NULL != protocol && NULL != address && !strcmp(p_prot, protocol) && !strcmp(p_addr, address)) {
            /* this is an alternate messenger when it doesn't match */
            if (0 != strncasecmp(p_prot, ptr->ta, strlen(p_prot))) {
                JString *peerid_j=NULL;
                const char *peerid_c;
                Jxta_id *peerid=NULL;

                peerid_j = jstring_new_2("urn:jxta:");
                peerid_c = ((char *)ptr->ta) + 7;
                jstring_append_2(peerid_j, peerid_c);

                if (JXTA_SUCCESS != jxta_id_from_jstring(&peerid, peerid_j)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unable to create peerid from %s\n", jstring_get_string(peerid_j));
                } else {
                    jxta_vector_add_object_last(*endpoints, (Jxta_object *) peerid);
                }
                if (peerid)
                    JXTA_OBJECT_RELEASE(peerid);
                if (peerid_j)
                    JXTA_OBJECT_RELEASE(peerid_j);
            }
        }
    }
    free(found_entries);
    apr_thread_mutex_unlock(service->mutex);
    return res;
}

JXTA_DECLARE(void) jxta_endpoint_service_add_filter(Jxta_endpoint_service * service, char const *str, JxtaEndpointFilter f,
                                                    void *arg)
{
    Filter *filter = (Filter *) malloc(sizeof(Filter));
    if (filter == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return;
    }
    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    filter->func = f;
    filter->arg = arg;

    /* insert the filter into the list */
    apr_thread_mutex_lock(endpoint_service->demux_mutex);
    dl_insert_b(endpoint_service->filter_list, filter);
    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
}

JXTA_DECLARE(void) jxta_endpoint_service_remove_filter(Jxta_endpoint_service * service, JxtaEndpointFilter filter)
{
    Dlist *cur;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    apr_thread_mutex_lock(endpoint_service->demux_mutex);

    /* find the filter registered under this handle */

    dl_traverse(cur, endpoint_service->filter_list) {

        if (((Filter *) cur->val)->func == filter)
            break;

    }

    /* remove this filter from the list */
    if (cur != NULL) {
        free(cur->val);
        dl_delete_node(cur);
    }

    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
}

static apr_status_t cb_elt_cleanup(void *me)
{
    Cb_elt *_self = me;

    if (_self->recipient) {
        free(_self->recipient);
    }

    return APR_SUCCESS;
}

static Cb_elt *cb_elt_create(Jxta_endpoint_service * me, const char *name, const char *param, Jxta_callback_func f, void *arg)
{
    Cb_elt *cb;
    apr_pool_t *pool;

    pool = jxta_PG_pool_get(me->my_group);
    apr_thread_mutex_lock(me->mutex);
    if (NULL == me->recycled_cbs) {
        cb = apr_pcalloc(pool, sizeof(*cb));
        cb->ep_svc = me;
        apr_pool_cleanup_register(pool, cb, cb_elt_cleanup, apr_pool_cleanup_null);
    } else {
        cb = me->recycled_cbs;
        if (cb->recipient) {
            free(cb->recipient);
        }
        me->recycled_cbs = cb->recycle;
    }
    apr_thread_mutex_unlock(me->mutex);
    cb->func = f;
    cb->arg = arg;
    cb->recipient = get_service_key(name, param);

    return cb;
}

static void cb_elt_recycle(Cb_elt * me)
{
    Jxta_endpoint_service *ep = me->ep_svc;

    apr_thread_mutex_lock(ep->mutex);
    me->recycle = ep->recycled_cbs;
    ep->recycled_cbs = me;
    apr_thread_mutex_unlock(ep->mutex);
}

Jxta_status endpoint_service_add_recipient(Jxta_endpoint_service * me, void **cookie, const char *name                                  , const char *param, Jxta_callback_func f, void *arg)
{
    Cb_elt *cb, *old_cb;
    Jxta_status rv;

    cb = cb_elt_create(me, name, param, f, arg);
    apr_thread_mutex_lock(me->demux_mutex);
    old_cb = apr_hash_get(me->cb_table, cb->recipient, APR_HASH_KEY_STRING);
    if (old_cb) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "There is a recipient for %s already, ignor request.\n",
                        cb->recipient);
        cb_elt_recycle(cb);
        *cookie = NULL;
        rv = JXTA_BUSY;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding recipient[%pp] for %s.\n", cb->func, cb->recipient);
        apr_hash_set(me->cb_table, cb->recipient, APR_HASH_KEY_STRING, cb);
        *cookie = cb->recipient;
        rv = JXTA_SUCCESS;
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    return rv;
}

Jxta_status endpoint_service_remove_recipient(Jxta_endpoint_service * me, void *cookie)
{
    Jxta_status rv;
    Cb_elt *old_cb;

    apr_thread_mutex_lock(me->demux_mutex);
    old_cb = apr_hash_get(me->cb_table, cookie, APR_HASH_KEY_STRING);
    if (!old_cb) {
        rv = JXTA_ITEM_NOTFOUND;
    } else {
        apr_hash_set(me->cb_table, cookie, APR_HASH_KEY_STRING, NULL);
        cb_elt_recycle(old_cb);
        rv = JXTA_SUCCESS;
    }
    apr_thread_mutex_unlock(me->demux_mutex);
    return rv;
}

Jxta_status endpoint_service_remove_recipient_by_addr(Jxta_endpoint_service * me, const char *name, const char *param)
{
    char *key;
    Jxta_status rv;

    key = get_service_key(name, param);
    rv = endpoint_service_remove_recipient(me, key);
    free(key);
    return rv;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_add_listener(Jxta_endpoint_service * service,
                                   char const *serviceName, char const *serviceParam, Jxta_listener * listener)
{
    int str_length = (serviceName != NULL ? strlen(serviceName) : 0) + (serviceParam != NULL ? strlen(serviceParam) : 0) + 1;
    apr_pool_t *pool = jxta_PG_pool_get(service->my_group);
    char *str = apr_pcalloc(pool, str_length);

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    JXTA_DEPRECATED_API();
    apr_snprintf(str, str_length, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Register listener for %s\n", str);

    apr_thread_mutex_lock(endpoint_service->demux_mutex);

    if (apr_hash_get(endpoint_service->listener_table, str, APR_HASH_KEY_STRING)) {
        apr_thread_mutex_unlock(endpoint_service->demux_mutex);
        return JXTA_BUSY;
    }

    JXTA_OBJECT_SHARE(listener);
    apr_hash_set(endpoint_service->listener_table, str, APR_HASH_KEY_STRING, (void *) listener);

    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_remove_listener(Jxta_endpoint_service * service, char const *serviceName, char const *serviceParam)
{
    Jxta_listener *listener;
    char str[256];

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    JXTA_DEPRECATED_API();
    apr_snprintf(str, sizeof(str), "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    apr_thread_mutex_lock(endpoint_service->demux_mutex);
    listener = (Jxta_listener *) apr_hash_get(endpoint_service->listener_table, str, APR_HASH_KEY_STRING);
    if (listener != NULL) {
        apr_hash_set(endpoint_service->listener_table, str, APR_HASH_KEY_STRING, NULL);
        JXTA_OBJECT_RELEASE(listener);
    }
    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
    return JXTA_SUCCESS;
}

static Jxta_listener *lookup_listener(Jxta_endpoint_service * me, Jxta_endpoint_address * addr)
{
    Jxta_listener *listener = NULL;
    unsigned int i;
    int iindex;
    int pt = 0;
    int j;
    int str_length;
    char *str;
    char *str1;
    const char *ea_svc_name;
    const char *ea_svc_params;

    Jxta_endpoint_service* endpoint_service = PTValid(me, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(addr);

    ea_svc_name = jxta_endpoint_address_get_service_name(addr);
    ea_svc_params = jxta_endpoint_address_get_service_params(addr);
    if (ea_svc_name == NULL) {
        str = jxta_endpoint_address_to_string(addr);
        if (str == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            return NULL;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No listener associated with this address %s\n", str);

        if (str != NULL)
            free(str);
        return NULL;
    }

    str_length = strlen(ea_svc_name) + (ea_svc_params != NULL ? strlen(ea_svc_params) : 0) + 1;
    str = calloc(str_length, sizeof(char));
    if (str == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return NULL;
    }
  
    strcpy( str, ea_svc_name );

    if( NULL != ea_svc_params ) {
        strcat( str, ea_svc_params );
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Lookup for listener : %s\n", str );

    apr_thread_mutex_lock(endpoint_service->demux_mutex);
    listener = (Jxta_listener *) apr_hash_get(endpoint_service->listener_table, str, APR_HASH_KEY_STRING);

    /*
     * Note: 20040510 tra
     * If we did not find a listener, we should check if the message
     * if from a cross-group endpoint messenger. Cross-group messengers
     * are registered only with the service param of the address.
     * The service name is just the name of the Endpoint cross-messenger
     * service:
     *    EndpointService:uuid-groupId/"real address"
     * We need to remove the beginning of the address
     */
    if (listener == NULL) {

        str1 = calloc(strlen(ea_svc_params) + 1, sizeof(char));
        if (str1 == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            free(str);
            return NULL;
        }
        iindex = strlen(ea_svc_name);

        /* 
         * copy the address from the param position, skipping the service name
         */
        j = 0;
        pt = 0;
        for (i = 0; i < strlen(ea_svc_params); i++) {
            *(str1 + j) = *(str + iindex);

            /*
             * remove the extra '/'
             */
            if (*(str1 + j) == '/') {
                iindex++;
                pt = i; /* save location of end of first parameter */
            } else {
                iindex++;
                j++;
            }
        }
        *(str1 + j) = '\0';

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for subgroup endpoint %s\n", str1);
        listener = (Jxta_listener *) apr_hash_get(endpoint_service->listener_table, str1, APR_HASH_KEY_STRING);

        /*
         * FIXME: 20040706 tra
         * The relay transport is using an endpoint address that
         * contain the relay server peerId. For the time being
         * just use the service id for demuxing to the relay as the
         * relay just registered with this address.
         * we are dropping the service parameter. This used to be used
         * when multiple relay connections were supported. This should
         * have been removed.
         */
        if (listener == NULL && pt != 0) {
            strncpy(str1, ea_svc_params, pt - 1);
            *(str1 + pt) = '\0';
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for relay endpoint %s\n", str1);
            listener = (Jxta_listener *) apr_hash_get(endpoint_service->listener_table, str1, APR_HASH_KEY_STRING);
        }

        free(str1);
    }

    apr_thread_mutex_unlock(endpoint_service->demux_mutex);
    free(str);
    return listener;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_propagate(Jxta_endpoint_service * service,
                                                          Jxta_message * msg, const char *service_name, const char *service_param)
{
    Jxta_vector *tps = NULL;
    Jxta_transport *transport = NULL;
    JString *svc = NULL;
    JString *param = NULL;
    unsigned int idx;
    Jxta_status rv = JXTA_SUCCESS;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (service_name == NULL && service_param == NULL)
        return JXTA_INVALID_ARGUMENT;

    tps = jxta_hashtable_values_get(endpoint_service->transport_table);
    if (!tps) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to enumerate available transports!\n");
        return JXTA_FAILED;
    }

    /*
     * Correctly set the endpoint address for cross-group
     * demuxing via the NetPeerGroup endpoint service
     */
    svc = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (svc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        JXTA_OBJECT_RELEASE(tps);
        return JXTA_NOMEM;
    }
    jstring_append_2(svc, ":");
    jstring_append_1(svc, endpoint_service->my_groupid);

    param = jstring_new_2(service_name);
    jstring_append_2(param, "/");
    jstring_append_2(param, service_param);

    for (idx = 0; idx < jxta_vector_size(tps); ++idx) {
        rv = jxta_vector_get_object_at(tps, JXTA_OBJECT_PPTR(&transport), idx);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to get transport %d of %d!\n", idx, jxta_vector_size(tps));
            break;
        }

        if (!PTValid(transport, Jxta_transport)) {
            JXTA_OBJECT_RELEASE(tps);
            JXTA_OBJECT_RELEASE(param);
            JXTA_OBJECT_RELEASE(svc);
            return JXTA_FAILED;
        }
        jxta_transport_propagate(transport, msg, jstring_get_string(svc), jstring_get_string(param));
        JXTA_OBJECT_RELEASE(transport);
    }

    JXTA_OBJECT_RELEASE(tps);
    JXTA_OBJECT_RELEASE(param);
    JXTA_OBJECT_RELEASE(svc);
    return JXTA_SUCCESS;
}

 Jxta_status endpoint_service_propagate_by_group(Jxta_PG * pg, Jxta_message * msg, const char *service_name, const char *service_param)
 {
     Jxta_vector *tps = NULL;
     Jxta_transport *transport = NULL;
     JString *svc = NULL;
     JString *param = NULL;
     unsigned int idx;
     Jxta_status rv = JXTA_SUCCESS;
 
     if (service_name == NULL && service_param == NULL)
         return JXTA_INVALID_ARGUMENT;
 
     Jxta_endpoint_service* endpoint_service = NULL;
     jxta_PG_get_endpoint_service(pg, &endpoint_service);
     if (NULL == endpoint_service)
         return JXTA_FAILED;
 
     tps = jxta_hashtable_values_get(endpoint_service->transport_table);
     if (!tps) {
         jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to enumerate available transports!\n");
         rv = JXTA_FAILED;
         goto FINAL_EXIT;
     }
 
     /*
      * Correctly set the endpoint address for cross-group
      * demuxing via the NetPeerGroup endpoint service
      */
     svc = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
     if (svc == NULL) {
         jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
         rv = JXTA_NOMEM;
         goto FINAL_EXIT;
     }
 
     Jxta_PGID * gid = NULL;    
     jxta_PG_get_GID(pg, &gid);
     JString *gidString = NULL; 
     jxta_id_get_uniqueportion(gid, &gidString);
     JXTA_OBJECT_RELEASE(gid);
     
     jstring_append_2(svc, ":");
     jstring_append_1(svc, gidString);
 
     param = jstring_new_2(service_name);
     jstring_append_2(param, "/");
     jstring_append_2(param, service_param);
 
     for (idx = 0; idx < jxta_vector_size(tps); ++idx) {
         rv = jxta_vector_get_object_at(tps, JXTA_OBJECT_PPTR(&transport), idx);
         if (JXTA_SUCCESS != rv) {
             jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Fail to get transport %d of %d!\n", idx, jxta_vector_size(tps));
             break;
         }
 
         if(!PTValid(transport, Jxta_transport)) {
            goto FINAL_EXIT;
         }
         jxta_transport_propagate(transport, msg, jstring_get_string(svc), jstring_get_string(param));
         JXTA_OBJECT_RELEASE(transport);
     }
 
 FINAL_EXIT: 
 
     if (gidString) JXTA_OBJECT_RELEASE(gidString);
     if (tps) JXTA_OBJECT_RELEASE(tps);
     if (param) JXTA_OBJECT_RELEASE(param);
     if (svc) JXTA_OBJECT_RELEASE(svc);
     JXTA_OBJECT_RELEASE(endpoint_service);
     return rv;
}

static apr_status_t msg_task_cleanup(void *me)
{
    Msg_task *_self = me;

    if (_self->msg) {
        JXTA_OBJECT_RELEASE(_self->msg);
    }

    return APR_SUCCESS;
}

static Msg_task *msg_task_create(Jxta_endpoint_service * me, Jxta_message * msg)
{
    apr_pool_t *pool;
    Msg_task *task;

    pool = jxta_PG_pool_get(me->my_group);
    apr_thread_mutex_lock(me->mutex);
    if (NULL == me->recycled_tasks) {
        task = apr_pcalloc(pool, sizeof(*task));
        task->ep_svc = me;
        apr_pool_cleanup_register(pool, task, msg_task_cleanup, apr_pool_cleanup_null);
    } else {
        task = me->recycled_tasks;
        me->recycled_tasks = task->recycle;
    }
    apr_thread_mutex_unlock(me->mutex);
    /* FIXME: Currently we clone to ensure msg won't be altered */
    /* this is for backward compatibility, we should deprecate this approach to avoid copy */
    task->msg = jxta_message_clone(msg);

    return task;
}

static void msg_task_recycle(Msg_task * me)
{
    Jxta_endpoint_service *ep = me->ep_svc;

    if (me->msg) {
        JXTA_OBJECT_RELEASE(me->msg);
        me->msg = NULL;
    }
    apr_thread_mutex_lock(ep->mutex);
    me->recycle = ep->recycled_tasks;
    ep->recycled_tasks = me;
    apr_thread_mutex_unlock(ep->mutex);
}

static long task_priority(Jxta_message * msg)
{
    apr_uint16_t pri;
    Jxta_status rv;

    rv = jxta_message_get_priority(msg, &pri);
    if (JXTA_SUCCESS != rv) {
        return APR_THREAD_TASK_PRIORITY_NORMAL;
    }
    return (pri >> 8) & APR_THREAD_TASK_PRIORITY_HIGHEST;
}

static Jxta_status get_messenger(Jxta_endpoint_service * me, Jxta_endpoint_address *dest, Jxta_boolean check_msg_size, JxtaEndpointMessenger **msgr)
{
    Jxta_status res=JXTA_SUCCESS;

    res = endpoint_messenger_get(me, dest, msgr);
    if (JXTA_ITEM_NOTFOUND == res) {
        *msgr = NULL;
        if (me->router_transport == NULL) {
            me->router_transport = jxta_endpoint_service_lookup_transport(me, "jxta");
            if (me->router_transport == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                                FILEANDLINE "Router transport not available.\n");
                return JXTA_BUSY;
            }
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No existing messenger dest [%pp] - try router\n", dest);
        *msgr = jxta_transport_messenger_get(me->router_transport, dest);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Returned msgr:[%pp] - after try router\n"
                                , *msgr);
        if (NULL != *msgr) {
            res = JXTA_SUCCESS;
        }
    }

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ex(Jxta_endpoint_service * me, Jxta_message * msg,
                                                        Jxta_endpoint_address * dest_addr, Jxta_boolean sync, apr_int64_t *max_size)
{
    Jxta_status res = JXTA_SUCCESS;

    Jxta_endpoint_service* myself = PTValid(me, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    if (NULL == jxta_endpoint_address_get_service_name(dest_addr)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        FILEANDLINE "No destination service specified. Discarding message [%pp]\n", msg);
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_message_set_destination(msg, dest_addr); 
    if (NULL != max_size) {
        jxta_message_set_priority(msg, MSG_NORMAL_FLOW);
        res = jxta_endpoint_service_check_msg_length(myself, dest_addr, msg, max_size);
        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }
    }

    if (sync) {
        res = outgoing_message_process(myself, msg);
    } else {
        char *baseAddrStr = NULL;
        Nc_entry *ptr = NULL;
        Msg_task *task;

        task = msg_task_create(myself, msg);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "pushing msg [%pp]\n", msg);

        apr_thread_pool_push(myself->thd_pool, outgoing_message_thread, task, task_priority(msg), myself);
        apr_atomic_inc32(&myself->msg_task_cnt);

        baseAddrStr = jxta_endpoint_address_get_transport_addr(dest_addr);
        apr_thread_mutex_lock(myself->mutex);
        ptr = apr_hash_get(myself->nc, baseAddrStr, APR_HASH_KEY_STRING);
        apr_thread_mutex_unlock(myself->mutex);
        free(baseAddrStr);
        res = (NULL == ptr) ? JXTA_SUCCESS : JXTA_UNREACHABLE_DEST;
    }

FINAL_EXIT:
    return res;
}

/* Deprecated, use jxta_PG_get_recipient_addr to get appropriate rewrited address. */
static Jxta_status do_crossgroup_send(Jxta_PG * obj, Jxta_endpoint_service * me, Jxta_message * msg,
                                      Jxta_endpoint_address * dest_addr, Jxta_boolean sync, apr_int64_t *max_length)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_PG *pg = PTValid(obj, Jxta_PG);
    Jxta_endpoint_address *new_addr = NULL;

    /*
     * Rewrite actually required for NetGroup as well, because JSE has the concept of WPG
     */
    if (strncmp(jxta_endpoint_address_get_service_name(dest_addr), JXTA_ENDPOINT_SERVICE_NAME, 
                strlen(JXTA_ENDPOINT_SERVICE_NAME))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PeerGroup cross-messenger need to rewrite dest address\n");
        res = jxta_PG_get_recipient_addr(pg, jxta_endpoint_address_get_protocol_name(dest_addr),
                                         jxta_endpoint_address_get_protocol_address(dest_addr),
                                         jxta_endpoint_address_get_service_name(dest_addr),
                                         jxta_endpoint_address_get_service_params(dest_addr), &new_addr);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Rewrite message address failed\n");
            return JXTA_FAILED;
        }
        res = jxta_endpoint_service_send_ex(me, msg, new_addr, sync, max_length);
        JXTA_OBJECT_RELEASE(new_addr);
    } else {
        res = jxta_endpoint_service_send_ex(me, msg, dest_addr, sync, max_length);
    }
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_sync(Jxta_PG * obj, Jxta_endpoint_service * service
                    , Jxta_message * msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_length)
{
    return do_crossgroup_send(obj, service, msg, dest_addr, JXTA_TRUE, max_length);
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_async(Jxta_PG * obj, Jxta_endpoint_service * service
                    ,Jxta_message * msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_length)
{
    return do_crossgroup_send(obj, service, msg, dest_addr, JXTA_FALSE, max_length);
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_check_msg_length(Jxta_endpoint_service * service
                                            , Jxta_endpoint_address *addr, Jxta_message * msg, apr_int64_t *max_length)
{
    Jxta_status res=JXTA_SUCCESS;
    apr_int64_t length=0;
    JxtaEndpointMessenger *messenger=NULL;
    Jxta_boolean ep_locked=FALSE;
    Jxta_boolean msgr_locked=FALSE;
    float compression=1;
    float compressed;

    res = get_messenger(service, addr, TRUE, &messenger);
    if (JXTA_ITEM_NOTFOUND == res || JXTA_BUSY == res) {
        res = JXTA_ITEM_NOTFOUND;
        goto FINAL_EXIT;
    }
    if (MSG_EXPEDITED != jxta_message_priority(msg)) {
        if (JXTA_SUCCESS != res) goto FINAL_EXIT;
        if (NULL != messenger->jxta_get_msg_details) {
            res = messenger->jxta_get_msg_details(messenger, msg, &length, &compression);
        }
        if (NULL != max_length && JXTA_SUCCESS == res) {
            Jxta_status msgr_status;
            apr_int64_t max_msgr_length=200;

            traffic_shaping_lock(service->ts);
            ep_locked = TRUE;
            res = traffic_shaping_check_max(service->ts, length, max_length, compression, &compressed);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Checked service with msgr:[%pp] length:%" APR_INT64_T_FMT " TS max_length:%" APR_INT64_T_FMT "\n"
                        , messenger, length, *max_length);

            if (NULL != messenger->ts) {
                traffic_shaping_lock(messenger->ts);
                msgr_locked = TRUE;
                msgr_status = traffic_shaping_check_max(messenger->ts
                                , length, &max_msgr_length, compression, &compressed);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Checked messenger [%pp] length:%" APR_INT64_T_FMT " TS max_length:%" APR_INT64_T_FMT "\n"
                            , messenger, length, max_msgr_length);
                if (max_msgr_length < *max_length) {
                    *max_length = max_msgr_length;
                    res = msgr_status;
                }
            }
        } else {
            res = JXTA_SUCCESS;
        }
    }
FINAL_EXIT:
    if (ep_locked)
        traffic_shaping_unlock(service->ts);
    if (msgr_locked)
        traffic_shaping_unlock(messenger->ts);
    if (messenger)
        JXTA_OBJECT_RELEASE(messenger);
    return res;

}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ep_msg_sync(Jxta_endpoint_service *service
                    , Jxta_endpoint_message * ep_msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_size)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_message *msg=NULL;
    Jxta_message_element *msg_elem;
    JString *ep_msg_xml;

    msg = jxta_message_new();
    jxta_endpoint_msg_get_xml(ep_msg, TRUE, &ep_msg_xml, TRUE);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send ep msg [%pp] async length:%d\n"
                            , ep_msg, jstring_length(ep_msg_xml));

    msg_elem = jxta_message_element_new_2(JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_ENPOINT_MSG_ELEMENT_NAME, "text/xml", jstring_get_string(ep_msg_xml), jstring_length(ep_msg_xml), NULL);

    jxta_message_add_element(msg, msg_elem);
    jxta_message_set_priority(msg, jxta_endpoint_msg_priority(ep_msg));
    if (NULL != max_size) {
        res = jxta_endpoint_service_check_msg_length(service, dest_addr, msg, max_size);
        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }
    }

    res = jxta_endpoint_service_send_ex(service, msg, dest_addr, JXTA_TRUE, max_size);

FINAL_EXIT:
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(msg_elem);
    JXTA_OBJECT_RELEASE(ep_msg_xml);
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_send_ep_msg_async(Jxta_endpoint_service *service
                    , Jxta_endpoint_message * ep_msg, Jxta_endpoint_address * dest_addr, apr_int64_t *max_size)
{
    Jxta_status res=JXTA_SUCCESS;
    Jxta_message *msg=NULL;
    Jxta_message_element *msg_elem;
    JString *ep_msg_xml;

    msg = jxta_message_new();
    jxta_endpoint_msg_get_xml(ep_msg, TRUE, &ep_msg_xml, TRUE);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send ep msg [%pp] async length:%d\n"
                            , ep_msg, jstring_length(ep_msg_xml));

    msg_elem = jxta_message_element_new_2(JXTA_ENDPOINT_MSG_JXTA_NS, JXTA_ENPOINT_MSG_ELEMENT_NAME, "text/xml", jstring_get_string(ep_msg_xml), jstring_length(ep_msg_xml), NULL);

    jxta_message_add_element(msg, msg_elem);
    jxta_message_set_priority(msg, jxta_endpoint_msg_priority(ep_msg));
    if (NULL != max_size) {
        res = jxta_endpoint_service_check_msg_length(service, dest_addr, msg, max_size);
        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }
    }

    res = jxta_endpoint_service_send_ex(service, msg, dest_addr, JXTA_FALSE, max_size);

FINAL_EXIT:
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(msg_elem);
    JXTA_OBJECT_RELEASE(ep_msg_xml);
    return res;
}

JXTA_DECLARE(void) jxta_endpoint_service_set_relay(Jxta_endpoint_service * service, const char *proto, const char *host)
{
    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (endpoint_service->relay_addr != NULL) {
        free(endpoint_service->relay_addr);
    }

    if (NULL != endpoint_service->relay_proto) {
        free(endpoint_service->relay_proto);
    }

    if (host != NULL) {
        endpoint_service->relay_addr = strdup(host);
        endpoint_service->relay_proto = strdup(proto);
    } else {
        endpoint_service->relay_addr = NULL;
        endpoint_service->relay_proto = NULL;
    }
}

JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_addr(Jxta_endpoint_service * service)
{
    char *addr = NULL;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (endpoint_service->relay_addr != NULL) {
        addr = strdup(endpoint_service->relay_addr);
    }

    return addr;
}

JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_proto(Jxta_endpoint_service * service)
{
    char *proto = NULL;

    Jxta_endpoint_service* endpoint_service = PTValid(service, Jxta_endpoint_service);

    if (endpoint_service->relay_proto != NULL) {
        proto = strdup(endpoint_service->relay_proto);
        if (proto == NULL)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");

    }

    return proto;
}

static void ep_fc_free(Jxta_object *me)
{
    Jxta_ep_flow_control *ep_fc=NULL;
    ep_fc = (Jxta_ep_flow_control *) me;

    if (ep_fc->group)
        JXTA_OBJECT_RELEASE(ep_fc->group);
    if (ep_fc->ea)
        JXTA_OBJECT_RELEASE(ep_fc->ea);
    free(ep_fc->msg_type);
    free(ep_fc);
}

JXTA_DECLARE(Jxta_ep_flow_control *) jxta_ep_flow_control_new()
{
    Jxta_ep_flow_control *ep_fc=NULL;

    ep_fc = calloc(1, sizeof(Jxta_ep_flow_control));
    JXTA_OBJECT_INIT(ep_fc, ep_fc_free, NULL);

    ep_fc->outbound = TRUE;
    ep_fc->msg_type = strdup("default");
    return ep_fc;
}


JXTA_DECLARE(void) jxta_ep_fc_set_group(Jxta_ep_flow_control *ep_fc, Jxta_PG *group)
{
    if (ep_fc->group) {
        JXTA_OBJECT_RELEASE(ep_fc->group);
    }
    ep_fc->group = NULL != group ? JXTA_OBJECT_SHARE(group):NULL;
}

JXTA_DECLARE(Jxta_status ) jxta_ep_fc_get_group(Jxta_ep_flow_control *ep_fc, Jxta_PG **group)
{
    Jxta_status res=JXTA_SUCCESS;
    if (NULL != ep_fc->group) {
        *group = JXTA_OBJECT_SHARE(ep_fc->group);
    } else {
        res=JXTA_ITEM_NOTFOUND;
    }
    return res;
}

JXTA_DECLARE(void) jxta_ep_fc_set_ea(Jxta_ep_flow_control *ep_fc, Jxta_endpoint_address *ea)
{
    if (ep_fc->ea) {
        JXTA_OBJECT_RELEASE(ep_fc->ea);
    }
    ep_fc->ea = NULL != ea ? JXTA_OBJECT_SHARE(ea):NULL;
}

JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_ea(Jxta_ep_flow_control *ep_fc, Jxta_endpoint_address **ea)
{
    Jxta_status res = JXTA_SUCCESS;
    if (NULL != ep_fc->ea) {
        *ea = JXTA_OBJECT_SHARE(ep_fc->ea);
    } else {
        res=JXTA_ITEM_NOTFOUND;
    }
    return res;
}

JXTA_DECLARE(void) jxta_ep_fc_set_msg_type(Jxta_ep_flow_control *ep_fc, const char *type)
{
    if (ep_fc->msg_type)
        free(ep_fc->msg_type);

    ep_fc->msg_type = strdup(type);
}

JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_msg_type(Jxta_ep_flow_control *ep_fc, char **msg_type)
{
    Jxta_status res=JXTA_SUCCESS;

    if (ep_fc->msg_type) {
        *msg_type = strdup(ep_fc->msg_type);
    } else {
        res = JXTA_ITEM_NOTFOUND;
    }
    return res;
}

JXTA_DECLARE(void) jxta_ep_fc_set_outbound(Jxta_ep_flow_control *ep_fc, Jxta_boolean outbound)
{
    ep_fc->outbound = outbound;
}

JXTA_DECLARE(Jxta_boolean) jxta_ep_fc_outbound(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->outbound;
}

JXTA_DECLARE(void) jxta_ep_fc_set_inbound(Jxta_ep_flow_control *ep_fc, Jxta_boolean inbound)
{
    ep_fc->inbound = inbound;
}

JXTA_DECLARE(Jxta_boolean) jxta_ep_fc_inbound(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->inbound;
}

JXTA_DECLARE(Jxta_status) jxta_ep_fc_set_rate(Jxta_ep_flow_control *ep_fc, int rate)
{
    Jxta_status res=JXTA_SUCCESS;

    ep_fc->rate = rate;
    return res;
}

JXTA_DECLARE(int) jxta_ep_fc_rate(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->rate;
}

JXTA_DECLARE(void) jxta_ep_fc_set_rate_window(Jxta_ep_flow_control *ep_fc, int window)
{
    ep_fc->rate_window = window;
}

JXTA_DECLARE(int) jxta_ep_fc_rate_window(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->rate_window;
}

JXTA_DECLARE(void) jxta_ep_fc_set_expect_delay(Jxta_ep_flow_control *ep_fc, Jxta_time time)
{
    ep_fc->expect_delay = time;
}

JXTA_DECLARE(Jxta_time) jxta_ep_fc_expect_delay(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->expect_delay;
}

JXTA_DECLARE(void) jxta_ep_fc_set_msg_size(Jxta_ep_flow_control *ep_fc, apr_int64_t size)
{
    ep_fc->msg_size = size;
}

JXTA_DECLARE(apr_int64_t) jxta_ep_fc_msg_size(Jxta_ep_flow_control *ep_fc)
{
    return ep_fc->msg_size;
}

JXTA_DECLARE(void) jxta_ep_fc_set_reserve(Jxta_ep_flow_control *ep_fc, int reserve)
{
    if (ep_fc->ts != NULL) {
        traffic_shaping_set_reserve(ep_fc->ts, reserve);
    }
}

JXTA_DECLARE(int) jxta_ep_fc_reserve(Jxta_ep_flow_control *ep_fc)
{
    return NULL != ep_fc->ts ? traffic_shaping_reserve(ep_fc->ts):0;
}

JXTA_DECLARE(Jxta_status) jxta_ep_fc_set_traffic_shaping(Jxta_ep_flow_control *ep_fc, Jxta_traffic_shaping *ts)
{
    if (NULL != ep_fc->ts)
        JXTA_OBJECT_RELEASE(ep_fc->ts);
    ep_fc->ts = NULL != ts ? JXTA_OBJECT_SHARE(ts):NULL;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_ep_fc_get_traffic_shaping(Jxta_ep_flow_control *ep_fc, Jxta_traffic_shaping **ts)
{
    *ts = (NULL != ep_fc->ts) ? JXTA_OBJECT_SHARE(ep_fc->ts):NULL;
    return JXTA_SUCCESS;
}

static void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * thread, void *arg)
{
    Jxta_status status;
    Msg_task *task = arg;
    Jxta_endpoint_service *me = PTValid(task->ep_svc, Jxta_endpoint_service);
    Jxta_message *msg;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint outgoing message handler awake.\n");

    if (JXTA_MODULE_STARTED != jxta_module_state((Jxta_module*)me)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint service is stopped, interrupt handler.\n");
        return NULL;
    }

    msg = task->msg;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "processing msg [%pp].\n", msg);

    apr_atomic_dec32(&me->msg_task_cnt);
    JXTA_OBJECT_CHECK_VALID(msg);
    status = outgoing_message_process(me, msg);
    if (JXTA_BUSY == status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Outgoing msg handler can't send message [%pp]\n", msg);
        apr_thread_pool_schedule(me->thd_pool, outgoing_message_thread, task, 2000 * 1000 , me);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint outgoing message handler stopped.\n");
        msg_task_recycle(task);
    }
    return NULL;
}

Jxta_status endpoint_messenger_get(Jxta_endpoint_service * me, Jxta_endpoint_address * dest
                            , JxtaEndpointMessenger **msgr)
{
    Jxta_status res=JXTA_SUCCESS;
    JxtaEndpointMessenger *messenger=NULL;
    char * ta;
    Peer_route_elt *ptr;

    ta = jxta_endpoint_address_get_transport_addr(dest);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->messengers, ta, APR_HASH_KEY_STRING);

    free(ta);
    if (!ptr) {
        apr_thread_mutex_unlock(me->mutex);
        return JXTA_ITEM_NOTFOUND;
    }

    messenger = JXTA_OBJECT_SHARE(ptr->msgr);
    apr_thread_mutex_unlock(me->mutex);

    JXTA_OBJECT_CHECK_VALID(messenger);
    *msgr = messenger;
    return res;
}

static Jxta_status send_with_messenger(Jxta_endpoint_service * me, JxtaEndpointMessenger * messenger
                        , Jxta_message * msg, Jxta_endpoint_address * dest)
{
    Jxta_status res=JXTA_SUCCESS;
    apr_int64_t msg_size=0;
    float compression=1.0;
    apr_int64_t max_size;
    Jxta_boolean ep_locked=FALSE;
    Jxta_boolean msgr_locked=FALSE;

    if (NULL != messenger->jxta_get_msg_details) {
        messenger->jxta_get_msg_details(messenger, msg, &msg_size, &compression);
    }

    if (MSG_NORMAL_FLOW == jxta_message_priority(msg) && msg_size > 0) {
        float compressed;
        Jxta_boolean look_ahead_update = TRUE;

        traffic_shaping_lock(me->ts);
        ep_locked = TRUE;
        res = traffic_shaping_check_max(me->ts, msg_size, &max_size, compression, &compressed);
        if (NULL != messenger->ts && JXTA_SUCCESS == res) {
            Jxta_status msgr_status;
            apr_int64_t max_msgr_length;

            traffic_shaping_lock(messenger->ts);
            msgr_locked = TRUE;
            msgr_status = traffic_shaping_check_max(messenger->ts
                                , msg_size, &max_msgr_length, compression, &compressed);
            res = msgr_status;

            look_ahead_update = FALSE;
            if (traffic_shaping_check_size(messenger->ts, compressed, FALSE, &look_ahead_update)) {
                traffic_shaping_update(messenger->ts, compressed, look_ahead_update);
            } else {
                res = JXTA_BUSY;
            }
        } else if (JXTA_SUCCESS == res && traffic_shaping_check_size(me->ts, compressed, FALSE, &look_ahead_update)) {
                traffic_shaping_update(me->ts,compressed, look_ahead_update);
        } else {
            res = JXTA_BUSY;
        }
    }
    if (JXTA_SUCCESS == res) {
        if (ep_locked) {
            traffic_shaping_unlock(me->ts);
            ep_locked = FALSE;
        }
        if (msgr_locked) {
            traffic_shaping_unlock(messenger->ts);
            msgr_locked = FALSE;
        }
        res = messenger->jxta_send(messenger, msg);

    } else if (JXTA_BUSY == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "busy messenger:[%pp] msg:[%pp]\n"
                                            , messenger, msg);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Removing messenger [%pp] \n", me);
        messenger_remove(me, dest);
    }

    if (ep_locked)
        traffic_shaping_unlock(me->ts);
    if (msgr_locked)
        traffic_shaping_unlock(messenger->ts);
    return res;
}

static Jxta_status send_message(Jxta_endpoint_service * me, Jxta_message * msg, Jxta_endpoint_address * dest)
{
    Jxta_status res;
    JxtaEndpointMessenger *messenger=NULL;
    Jxta_endpoint_address *dest_tmp=NULL;

    if (NULL == dest) {
        dest_tmp = jxta_message_get_destination(msg);
        if (dest_tmp == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "Failed to get destination from message. Message [%pp] is discarded.\n", msg);
            res = JXTA_FAILED;
            goto FINAL_EXIT;
        }
        if(NULL == jxta_endpoint_address_get_service_name(dest_tmp)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to get endpoint address service name [%pp]\n", dest_tmp);
            res = JXTA_FAILED;
            goto FINAL_EXIT;
        }
    } else {
        dest_tmp = JXTA_OBJECT_SHARE(dest);
    }

    res = get_messenger(me, dest_tmp , FALSE, &messenger);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send message to [%pp] status:%ld\n", dest, res);
    if (JXTA_SUCCESS != res) {
        res = JXTA_UNREACHABLE_DEST;
        goto FINAL_EXIT;
    }
    res = send_with_messenger(me, messenger, msg, dest_tmp);

FINAL_EXIT:
    if (dest_tmp)
        JXTA_OBJECT_RELEASE(dest_tmp);
    if (messenger)
        JXTA_OBJECT_RELEASE(messenger);
    return res;
}

static Jxta_status outgoing_message_process(Jxta_endpoint_service * me, Jxta_message * msg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *dest=NULL;
    char *baseAddrStr = NULL;
    Nc_entry *ptr;
    static volatile apr_uint32_t cnt = 0;
    apr_uint16_t ttl;
    time_t lifespan;

    if (me->router_transport == NULL) {
        me->router_transport = jxta_endpoint_service_lookup_transport(me, "jxta");
        if (me->router_transport == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                            FILEANDLINE "Router transport not available. Discarding message [%pp]\n", msg);
            return JXTA_BUSY;
        }
    }

    JXTA_OBJECT_CHECK_VALID(msg);

    if (JXTA_SUCCESS == jxta_message_get_ttl(msg, &ttl) && 0 == ttl) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] is discarded with TTL 0\n", msg);
        return JXTA_SUCCESS;
    }

    if (JXTA_SUCCESS == jxta_message_get_lifespan(msg, &lifespan) && time(NULL) >= lifespan) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] is discarded with lifespan %ld\n", msg, lifespan);
        return JXTA_SUCCESS;
    }

    dest = jxta_message_get_destination(msg);
    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "No destination. Message [%pp] is discarded.\n", msg);
        return JXTA_ITEM_NOTFOUND;
    }
    if (NULL == jxta_endpoint_address_get_service_name(dest)) {
        return JXTA_ITEM_NOTFOUND;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Outgoing message [%pp] -> %s://%s/%s/%s\n",
                    msg,
                    jxta_endpoint_address_get_protocol_name(dest),
                    jxta_endpoint_address_get_protocol_address(dest),
                    jxta_endpoint_address_get_service_name(dest), jxta_endpoint_address_get_service_params(dest));

    baseAddrStr = jxta_endpoint_address_get_transport_addr(dest);
    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->nc, baseAddrStr, APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(me->mutex);
    if (NULL != ptr) {
        res = nc_peer_queue_msg(me, ptr, msg);
        apr_thread_mutex_unlock(me->nc_wlock);
    } else {
        apr_thread_mutex_unlock(me->nc_wlock);
        res = send_message(me, msg, dest);
        if (JXTA_UNREACHABLE_DEST == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer at %s is unreachable, queueing msg[%pp]\n", baseAddrStr, msg);
            ptr = nc_add_peer(me, baseAddrStr, msg);
        } else if (JXTA_LENGTH_EXCEEDED == res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Message [%pp] dropped - Should not exceed max\n", msg);
        }
    }
    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] send successful.\n", msg);
    } else if (JXTA_BUSY != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Message [%pp] send unsuccessful. status:%ld\n", msg, res);
    }

    free(baseAddrStr);
    JXTA_OBJECT_RELEASE(dest);

    if (apr_atomic_inc32(&cnt) >= jxta_epcfg_get_ncrq_retry(me->config) || 0 == apr_atomic_read32(&me->msg_task_cnt)) {
        apr_atomic_set32(&cnt, 0);
        nc_review_all(me);
    }
    return res;
}

static void check_nc_entry(Jxta_endpoint_service *me, const char *ta)
{
    Nc_entry *ptr = NULL;
    Jxta_status status;

    apr_thread_mutex_lock(me->nc_wlock);
    apr_thread_mutex_lock(me->mutex);
    ptr = apr_hash_get(me->nc, ta, APR_HASH_KEY_STRING);
    apr_thread_mutex_unlock(me->mutex);
    if (NULL != ptr) {
        ptr->expiration = 0;
        status = nc_peer_process_queue(me, ptr);
        if (JXTA_SUCCESS == status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peer at %s is back online.\n", ptr->addr);
            nc_remove_peer(me, ptr);
        }
    }
    apr_thread_mutex_unlock(me->nc_wlock);
}

void jxta_endpoint_service_transport_event(Jxta_endpoint_service * me, Jxta_transport_event * e)
{
    char *addr = NULL;
    Jxta_endpoint_address *ea = NULL;
#ifdef UNUSED_VWF
    Jxta_status status;
    Nc_entry *ptr = NULL;
    apr_ssize_t klen;
#endif

    switch (e->type) {
    case JXTA_TRANSPORT_INBOUND_CONNECTED:
        if (e->dest_addr == NULL || e->peer_id == NULL || e->msgr == NULL ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "transport event missing required information [%pp]\n", e);
            break;
        }

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Inbound connection from %s.\n", addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);

        apr_thread_mutex_lock(me->mutex);

        messenger_add(me, e->dest_addr, e->msgr);
        if (ea) {
            messenger_add(me, ea, e->msgr);
        }
        apr_thread_mutex_unlock(me->mutex);

        check_nc_entry(me, addr);
        free(addr);
        if (ea) {
            addr = jxta_endpoint_address_get_transport_addr(ea);
            check_nc_entry(me, addr);
            free(addr);
            JXTA_OBJECT_RELEASE(ea);
        }
        break;

    case JXTA_TRANSPORT_OUTBOUND_CONNECTED:
        if (e->dest_addr == NULL || e->peer_id == NULL || e->msgr == NULL ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "transport event missing required information [%pp]\n", e);
            break;
        }

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Outbound connection to %s.\n", addr);
        free(addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);

        apr_thread_mutex_lock(me->mutex);
        messenger_add(me, e->dest_addr, e->msgr);
        if (ea) {
            messenger_add(me, ea, e->msgr);
            JXTA_OBJECT_RELEASE(ea);
        }
        apr_thread_mutex_unlock(me->mutex);
        break;

    case JXTA_TRANSPORT_CONNECTION_CLOSED:
        if (e->dest_addr == NULL || e->peer_id == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "transport event missing required information [%pp]\n", e);
            break;
        }

        addr = jxta_endpoint_address_get_transport_addr(e->dest_addr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Close connection to %s.\n", addr);
        free(addr);

        ea = jxta_endpoint_address_new_3(e->peer_id, NULL, NULL);
        apr_thread_mutex_lock(me->mutex);
        messenger_remove(me, e->dest_addr);
        if (ea) {
            addr = jxta_endpoint_address_get_transport_addr(ea);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Close connection with %s.\n", addr);
            free(addr);
            messenger_remove(me, ea);
            JXTA_OBJECT_RELEASE(ea);
        }
        apr_thread_mutex_unlock(me->mutex);
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Ignore unknow transport event type %d.\n", e);
        break;
    }
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_service_get_thread_pool(Jxta_endpoint_service *me, apr_thread_pool_t **tp)
{
    *tp = me->thd_pool;
    return JXTA_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
