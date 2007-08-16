/*
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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

static const char *__log_cat = "TCP_CONNECTION";

#include <assert.h>

#include "jxta_apr.h"
#include "jxta_object_type.h"
#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_transport_tcp_private.h"
#include "jxta_tcp_message_packet_header.h"
#include "jxta_transport_welcome_message.h"
#include "jxta_util_priv.h"

#define BUFSIZE		8192    /* BUFSIZ */

static const char * WELCOME_GREETING = "JXTAHELLO";
static const size_t WELCOME_GREETING_LENGTH = 9;

typedef enum e_msg_states {
    HDR_NAME_SIZE = 0,
    HDR_NAME,
    HDR_BODY_SIZE_MSB,
    HDR_BODY_SIZE_LSB,
    HDR_BODY,
    HDR_DONE
} E_msg_state;

typedef struct tcp_msg_ctx {
    apr_pool_t *pool;
    char *msg_type;
    apr_int64_t msg_size;
    char *src_ea;
    Jxta_message *msg;
    /* parsing state */
    E_msg_state state;
    /* number of bytes needed to transit to next state */
    apr_int64_t deficit;
} Tcp_msg_ctx;

typedef int (split_fn) (const char *data, apr_size_t *len, void *arg);

struct _tcp_messenger {
    Extends(JxtaEndpointMessenger);
    Jxta_transport_tcp_connection * conn;
};

struct _jxta_transport_tcp_connection {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
    apr_thread_mutex_t *reading_lock;
    volatile Jxta_boolean dispatched;
    volatile Tcp_connection_state connection_state;

    Jxta_transport_tcp *tp;

    /* Remote address */
    Jxta_endpoint_address *dest_addr;
    char *ipaddr;
    apr_port_t port;
    apr_sockaddr_t *intf_addr;

    apr_socket_t *shared_socket;
    void *poll;
    apr_pollfd_t pollfd;

    Jxta_welcome_message *my_welcome;
    Jxta_welcome_message *its_welcome;
    int use_msg_version;

    apr_bucket_alloc_t *bk_list;
    apr_bucket_brigade *brigade;
    apr_bucket_brigade *buf;

    Tcp_msg_ctx msg_ctx;

    char *data_out_buf;         /* output stream */
    int d_out_index;

    Jxta_endpoint_service *endpoint;

    apr_time_t last_time_used;
    Jxta_boolean inbound;

    TcpMessenger * msgr;
};

typedef struct _jxta_transport_tcp_connection _jxta_transport_tcp_connection;

static Jxta_transport_tcp_connection *tcp_connection_new(Jxta_transport_tcp * tp, Jxta_boolean initiator);
static void tcp_connection_free(Jxta_object * me);

static Jxta_status connect_to_from(_jxta_transport_tcp_connection * _self, const char *local_ipaddr);
static Jxta_status JXTA_STDCALL read_cb(void *param, void *arg);

static Jxta_status tcp_connection_start_socket(_jxta_transport_tcp_connection * _self);
static Jxta_status flush(Jxta_transport_tcp_connection * me, const char *buf, apr_size_t size);
static Jxta_status tcp_connection_flush(_jxta_transport_tcp_connection * _self);

/* Tcp Messenger implementation */
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp_connection * conn);
static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg);
static void tcp_messenger_free(Jxta_object * obj);

static void tcp_messenger_free(Jxta_object * obj)
{
    TcpMessenger *me = (TcpMessenger *) obj;

    assert(me->conn);

    me->conn->msgr = NULL;

    JXTA_OBJECT_RELEASE(me->_super.address);

    if (CONN_DISCONNECTED != tcp_connection_state(me->conn)) {
        jxta_transport_tcp_connection_close(me->conn);
        JXTA_OBJECT_RELEASE(me->conn);
    }

    memset(me, 0xDD, sizeof(TcpMessenger));
    free(me);
}

static TcpMessenger * tcp_messenger_new(Jxta_transport_tcp_connection * conn)
{
    TcpMessenger *me = NULL;

    JXTA_OBJECT_CHECK_VALID(conn);
    assert(conn);
    assert(CONN_CONNECTED == tcp_connection_state(conn));

    /* create the object */
    me = calloc(1, sizeof(TcpMessenger));
    if (me == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE 
                        "Out of memory caused failure to create messenger for TCP connection[%pp].\n", conn);
        return NULL;
    }

    me->conn = JXTA_OBJECT_SHARE(conn);
    me->_super.jxta_send = tcp_messenger_send;
    me->_super.address = JXTA_OBJECT_SHARE(conn->dest_addr);

    /* initialize it */
    JXTA_OBJECT_INIT(me, tcp_messenger_free, NULL);

    return me;
}

static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * me, Jxta_message * msg)
{
    Jxta_status status;
    TcpMessenger *myself = (TcpMessenger *) me;

    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(msg);

    assert(myself->conn);

    JXTA_OBJECT_CHECK_VALID(myself->conn);

    JXTA_OBJECT_SHARE(msg);
    /* message must be set due to JSE implementation bug */
    jxta_message_set_source(msg, myself->_super.address);

    status = jxta_transport_tcp_connection_send_message(myself->conn, msg);
    JXTA_OBJECT_RELEASE(msg);

    return status;
}

static void msg_ctx_reset(Tcp_msg_ctx * ctx)
{
    if (ctx->pool) {
        apr_pool_destroy(ctx->pool);
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = HDR_NAME_SIZE;
}

static Jxta_status emit_event(Jxta_transport_tcp_connection * me, Jxta_transport_event_type ev_type)
{
    Jxta_transport_event *e = NULL;
    Jxta_status rv;

    e = jxta_transport_event_new(ev_type);
    if (NULL == e) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE 
                        "Out of memory caused failure to create event for connection[%pp].\n", me);
        return JXTA_NOMEM; 
    }

    e->dest_addr = jxta_transport_tcp_connection_get_destaddr(me);
    e->peer_id = jxta_transport_tcp_connection_get_destination_peerid(me);

    switch (ev_type) {
        case JXTA_TRANSPORT_INBOUND_CONNECTED: 
        case JXTA_TRANSPORT_OUTBOUND_CONNECTED:
            rv = tcp_connection_get_messenger(me, apr_time_from_sec(1), (JxtaEndpointMessenger**)&e->msgr);
            assert(JXTA_TIMEOUT != rv);
            break;
        case JXTA_TRANSPORT_CONNECTION_CLOSED:
        default:
            break;
    }

    jxta_endpoint_service_transport_event(me->endpoint, e);
    JXTA_OBJECT_RELEASE(e);
    return JXTA_SUCCESS;
}

static Jxta_transport_tcp_connection *tcp_connection_new(Jxta_transport_tcp * tp, Jxta_boolean initiator)
{
    _jxta_transport_tcp_connection *_self;
    apr_status_t status;

    /* create object */
    _self = (Jxta_transport_tcp_connection *) calloc(1, sizeof(Jxta_transport_tcp_connection));
    if (_self == NULL)
        return NULL;

    /* initialize it */
    JXTA_OBJECT_INIT(_self, tcp_connection_free, NULL);

    /* apr setting */
    status = apr_pool_create(&_self->pool, NULL);
    if (APR_SUCCESS != status) {
        /* Free the memory that has been already allocated */
        free(_self);
        return NULL;
    }

    /* apr_mutex */
    status = apr_thread_mutex_create(&_self->mutex, APR_THREAD_MUTEX_NESTED, _self->pool);
    if (APR_SUCCESS != status) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }

    _self->connection_state = CONN_INIT;

    /* setting */
    _self->tp = tp;
    _self->shared_socket = NULL;
    _self->intf_addr = NULL;
    _self->ipaddr = NULL;
    _self->port = 0;
    _self->dest_addr = NULL;
    _self->my_welcome = NULL;
    _self->its_welcome = NULL;

    _self->bk_list = apr_bucket_alloc_create(_self->pool);
    _self->brigade = apr_brigade_create(_self->pool, _self->bk_list);
    _self->buf = apr_brigade_create(_self->pool, _self->bk_list);
    _self->dispatched = FALSE;

    status = apr_thread_mutex_create(&_self->reading_lock, APR_THREAD_MUTEX_NESTED, _self->pool);
    if (APR_SUCCESS != status) {
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }

    /* output stream */
    _self->data_out_buf = (char *) malloc(BUFSIZE);
    if (_self->data_out_buf == NULL) {
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }
    _self->d_out_index = 0;

    _self->endpoint = jxta_transport_tcp_get_endpoint_service(tp);

    _self->last_time_used = apr_time_now();
    _self->inbound = FALSE;

    return _self;
}

static void tcp_connection_free(Jxta_object * me)
{
    _jxta_transport_tcp_connection *myself = (_jxta_transport_tcp_connection *) me;
    apr_thread_pool_t *tp;

    jxta_endpoint_service_get_thread_pool(myself->endpoint, &tp);
    apr_thread_pool_tasks_cancel(tp, myself);
    /* myself->ipaddr is allocated in myself->pool so we need not to free */
    if (myself->endpoint) {
        JXTA_OBJECT_RELEASE(myself->endpoint);
    }

    if (myself->data_out_buf)
        free(myself->data_out_buf);

    if (myself->my_welcome) {
        JXTA_OBJECT_RELEASE(myself->my_welcome);
    }
    if (myself->its_welcome) {
        JXTA_OBJECT_RELEASE(myself->its_welcome);
    }
    if (myself->dest_addr)
        JXTA_OBJECT_RELEASE(myself->dest_addr);

    if (myself->mutex) {
        apr_thread_mutex_destroy(myself->mutex);
    }

    if (myself->reading_lock) {
        apr_thread_mutex_destroy(myself->reading_lock);
    }

    if (myself->pool)
        apr_pool_destroy(myself->pool);

    free(myself);
}

static Jxta_status connect_to_from(Jxta_transport_tcp_connection * _self, const char *local_ipaddr)
{
    apr_sockaddr_t *intfaddr;
    apr_status_t status;

    status = apr_sockaddr_info_get(&_self->intf_addr, _self->ipaddr, APR_UNSPEC, _self->port, 0, _self->pool);
    if (APR_SUCCESS != status)
        return status;

    status = apr_sockaddr_info_get(&intfaddr, local_ipaddr, APR_INET, 0, 0, _self->pool);
    if (APR_SUCCESS != status)
        return status;

#if CHECK_APR_VERSION(1, 0, 0)
    status = apr_socket_create(&_self->shared_socket, _self->intf_addr->family, SOCK_STREAM, APR_PROTO_TCP, _self->pool);
#else
    status = apr_socket_create_ex(&_self->shared_socket, _self->intf_addr->family, SOCK_STREAM, APR_PROTO_TCP, _self->pool);
#endif
    if (APR_SUCCESS != status)
        return status;

    status = apr_socket_bind(_self->shared_socket, intfaddr);
    if (APR_SUCCESS != status)
        return status;

    /* connection timeout setting (1 sec) */
    apr_socket_timeout_set(_self->shared_socket, CONNECTION_TIMEOUT);

    status = apr_socket_connect(_self->shared_socket, _self->intf_addr);
    if (APR_SUCCESS != status)
        return status;

    /* connection timeout reset */
    apr_socket_timeout_set(_self->shared_socket, -1);
    apr_socket_opt_set(_self->shared_socket, APR_TCP_NODELAY, 1);

    _self->inbound = FALSE;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_1(Jxta_transport_tcp * tp,
                                                                                  Jxta_endpoint_address * dest)
{
    apr_status_t status;
    Jxta_transport_tcp_connection *_self;
    char *local_ipaddr;
    const char *protocol_address;
    const char *colon;

    _self = tcp_connection_new(tp, TRUE);

    if (_self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create tcp connection\n");
        return NULL;
    }

    protocol_address = jxta_endpoint_address_get_protocol_address(dest);

    colon = strchr(protocol_address, ':');
    if (colon && (*(colon + 1))) {
        _self->port = atoi(colon + 1);
        _self->ipaddr = apr_pstrndup(_self->pool, protocol_address, colon - protocol_address);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not parse endpoint address.\n");
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }

    _self->dest_addr = JXTA_OBJECT_SHARE(dest);
    local_ipaddr = jxta_transport_tcp_get_local_ipaddr(tp);

    status = connect_to_from(_self, local_ipaddr);
    free(local_ipaddr);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed connect attempt : %s\n", msg);
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }

    if (JXTA_SUCCESS != tcp_connection_start_socket(_self)) {
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }

    return _self;
}

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_2(Jxta_transport_tcp * tp,
                                                                                  apr_socket_t * in_socket)
{
    Jxta_transport_tcp_connection *_self = tcp_connection_new(tp, FALSE);
    char *protocol_address;
    int len;

    if (_self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create tcp connection\n");
        return NULL;
    }

    _self->shared_socket = in_socket;
    _self->inbound = TRUE;

    apr_socket_addr_get(&_self->intf_addr, APR_REMOTE, _self->shared_socket);
    /* set to connection address before we can retrieve it from welcome message */
    apr_sockaddr_ip_get(&_self->ipaddr, _self->intf_addr);
    _self->port = _self->intf_addr->port;

    len = strlen(_self->ipaddr) + 10;
    protocol_address = (char *) malloc(len);
    if (protocol_address == NULL) {
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }
    apr_snprintf(protocol_address, len, "%s:%d", _self->ipaddr, _self->port);
    /* used to compose welcome message */
    _self->dest_addr = jxta_endpoint_address_new_2("tcp", protocol_address, NULL, NULL);
    free(protocol_address);
    if (_self->dest_addr == NULL) {
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }

    if (JXTA_SUCCESS != tcp_connection_start_socket(_self)) {
        JXTA_OBJECT_RELEASE(_self);
        return NULL;
    }

    return _self;
}

/**
    Handle welcome message.
**/
static Jxta_status tcp_connection_start_socket(Jxta_transport_tcp_connection * _self)
{
    Jxta_endpoint_address *public_addr;
    JString *peerid;
    Jxta_status res;
    char *caddress = NULL;
    JString *welcome_str = NULL;
    apr_bucket * e;

    peerid = jxta_transport_tcp_get_peerid(_self->tp);
    public_addr = jxta_transport_tcp_get_public_addr(_self->tp);

    _self->pollfd.p = _self->pool;
    _self->pollfd.desc.s = _self->shared_socket;
    _self->pollfd.desc_type = APR_POLL_SOCKET;
    _self->pollfd.reqevents = APR_POLLOUT;

    apr_socket_opt_set(_self->shared_socket, APR_SO_KEEPALIVE, 1);  /* Keep Alive */
    apr_socket_opt_set(_self->shared_socket, APR_SO_LINGER, LINGER_DELAY);  /* Linger Delay */
    apr_socket_opt_set(_self->shared_socket, APR_TCP_NODELAY, 1);   /* disable nagel's */

    apr_socket_opt_set(_self->shared_socket, APR_SO_NONBLOCK, 1);
    apr_socket_timeout_set(_self->shared_socket, 0);
    
    _self->my_welcome = welcome_message_new_1(_self->dest_addr, public_addr, peerid, TRUE, 1);
    JXTA_OBJECT_RELEASE(peerid);
    if (_self->my_welcome == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to create welcome message\n");
        return JXTA_FAILED;
    }

    /* send my welcome message */
    welcome_str = welcome_message_get_welcome(_self->my_welcome);

    res = flush(_self, jstring_get_string(welcome_str), jstring_length(welcome_str));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Local welcome message sent ..%s\n",
                    (caddress = jxta_endpoint_address_to_string(_self->dest_addr)));
    free(caddress);
    JXTA_OBJECT_RELEASE(welcome_str);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error socket write\n");
        return res;
    }

    _self->connection_state = CONN_CONNECTING;

    assert(APR_BRIGADE_EMPTY(_self->brigade));
    e = apr_bucket_socket_create(_self->shared_socket, _self->bk_list);
    APR_BRIGADE_INSERT_TAIL(_self->brigade, e);
    res = jxta_endpoint_service_add_poll(_self->endpoint, _self->shared_socket, read_cb, _self, &_self->poll);
    if (APR_SUCCESS != res) {
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

static int split_welcome(const char *data, apr_size_t *len, void *arg)
{
    const char *c;
    apr_size_t used;

    if (*len < WELCOME_GREETING_LENGTH) {
        return APR_EAGAIN;
    }

    if (strncmp(WELCOME_GREETING, data, WELCOME_GREETING_LENGTH)) {
        /* return EOF to trigger close connection */
        return APR_EOF;
    }

    if (*len > MAX_WELCOME_MSG_SIZE) {
        *len = MAX_WELCOME_MSG_SIZE;
    }

    used = WELCOME_GREETING_LENGTH + 2; /* space and '\r' */
    for (c = data + used; used < *len; c++, used++) {
        if (*c == '\n' && *(c-1) == '\r') {
            *len = used + 1;
            return TRUE;
        }
    }
    return FALSE;
}

static Jxta_status handle_header(Tcp_msg_ctx * ctx, char *name, apr_size_t name_len, char *val, apr_size_t val_len)
{
    int i;
    char tmp;

    name[name_len] = '\0';
    tmp = val[val_len];
    val[val_len] = '\0';
    switch (name_len) {
    case 5:
        if (0 == strncasecmp("srcEA", name, name_len)) {
            ctx->src_ea = val;
        }
        break;
    case 12:
        if (0 == strncasecmp("content-type", name, name_len)) {
            ctx->msg_type = val;
        }
        break;
    case 14:
        if (0 == strncasecmp("content-length", name, name_len)) {
            if (8 != val_len) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Content-length header should be of length 8, "
                                "got %" APR_INT64_T_FMT ".\n", val_len);
                return JXTA_INVALID_ARGUMENT;
            }
            ctx->msg_size = 0;
            for (i = 0; i < 8; i++) {
                ctx->msg_size <<= 8;
                ctx->msg_size |= (apr_int64_t) (val[i]) & 0xFF;
            }
        }
        break;
    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Ignore unused header: %s:%s.\n", name, val);
        break;
    }
    val[val_len] = tmp;
    return JXTA_SUCCESS;
}

/*
 * parse headers
 * 
 * @param ctx the msg context to keep the headers
 * @param data pointer to the data be parsed
 * @param len input as the number of bytes of data, output to be the number of bytes consumed.
 * @return APR_SUCCESS if we done with header block. I.e, see empty header octet 0x00.
 *         APR_EAGAIN if the more data is needed.
 *         Other error values could be returned.
 */
static Jxta_status read_headers(Tcp_msg_ctx * ctx, char *data, apr_size_t * len)
{
    apr_size_t name_len;
    apr_size_t val_len;
    apr_size_t sz, used;

    used = sz = 0;
    while (used < *len) {
        name_len = (apr_size_t) (*data) & 0xFF;
        if (0 == name_len) {
            ++used;
            *len = used;
            return JXTA_SUCCESS;
        } else {
            /* NULL terminate previous header_body */
            *data = '\0';
        }
        sz = name_len + 1;
        if (used + sz + 2 > *len) {
            break;
        }
        val_len = *(apr_int16_t *) (data + sz) & 0xFFFF;
        val_len = ntohs(val_len);
        sz += val_len + 2;
        if (used + sz > *len) {
            break;
        }
        handle_header(ctx, data + 1, name_len, data + name_len + 3, val_len);
        used += sz;
        data += sz;
    }
    *len = used;
    return APR_EAGAIN;
}

static int split_headers(const char *data, apr_size_t *len, void *arg)
{
    Tcp_msg_ctx *ctx = arg;
    apr_size_t used;

    used = 0;
    while (HDR_DONE != ctx->state && used < *len) {
        switch (ctx->state) {
            case HDR_NAME_SIZE:
                ctx->deficit = data[used++] & 0xFF;
                ctx->state = ctx->deficit ? HDR_NAME : HDR_DONE;
                break;
            case HDR_NAME:
            case HDR_BODY:
                if (used + ctx->deficit > *len) {
                    ctx->deficit -= (*len - used);
                    used = *len;
                } else {
                    used += ctx->deficit;
                    ctx->deficit = 0;
                    ctx->state = (HDR_NAME == ctx->state) ? HDR_BODY_SIZE_MSB : HDR_NAME_SIZE;
                }
                break;
            case HDR_BODY_SIZE_MSB:
                ctx->deficit = data[used++] & 0xFF;
                ctx->state = HDR_BODY_SIZE_LSB;
                break;
            case HDR_BODY_SIZE_LSB:
                ctx->deficit <<= 8;
                ctx->deficit |= data[used++] & 0xFF;
                ctx->state = HDR_BODY;
                break;
            default:
                assert(0);
                break;
        }
    }
    *len = used;
    return (HDR_DONE == ctx->state);
}

static int split_message(const char *data, apr_size_t *len, void *arg)
{
    Tcp_msg_ctx *ctx = arg;

    if (ctx->deficit < *len) {
        *len = ctx->deficit;
        ctx->deficit = 0;
    } else {
        ctx->deficit -= *len;
    }
    return (0 == ctx->deficit);
}

static Jxta_status split_brigade(apr_bucket_brigade * in, apr_bucket_brigade * out, split_fn fn, void *arg)
{
    Jxta_status rv;
    apr_bucket *e;
    const char *data;
    apr_size_t len;
    apr_size_t used;
    int done = 0;

    while (!done && !APR_BRIGADE_EMPTY(in)) {
        e = APR_BRIGADE_FIRST(in);
        rv = apr_bucket_read(e, &data, &len, APR_BLOCK_READ);
        if (APR_SUCCESS != rv) {
            return rv;
        }
        /* detect EOF from socket_bucket */
        if (0 == len && APR_BUCKET_IS_IMMORTAL(e)) {
            return APR_EOF;
        }

        used = len;
        done = fn(data, &used, arg);
        if (done && used < len) {
            apr_bucket_split(e, used);
        }

        APR_BUCKET_REMOVE(e);
        APR_BRIGADE_INSERT_TAIL(out, e);
    }
    return done ? APR_SUCCESS : APR_EAGAIN;
}

static Jxta_status handle_reading_error(Jxta_transport_tcp_connection * me, apr_status_t rv)
{
    if (APR_SUCCESS == rv) {
        return JXTA_SUCCESS;
    } else if (APR_STATUS_IS_EAGAIN(rv)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "TCP connection[%pp] is waiting for more data with status %d.\n", me, rv);
    } else if (APR_STATUS_IS_TIMEUP(rv)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, 
                        "TCP connection[%pp] timeup, socket timeout should set to 0 for nonblocking I/O\n", me);
    } else if (APR_STATUS_IS_EOF(rv)) {
        jxta_transport_tcp_connection_close(me);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP connection[%pp] closed.\n", me, rv);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "TCP connection[%pp] failed to read with status %d.\n", me, rv);
        jxta_transport_tcp_connection_close(me);
    }
    return rv;
}

static apr_status_t process_welcome(Jxta_transport_tcp_connection *me)
{
    apr_status_t rv;
    apr_size_t len;
    apr_off_t sz;
    char *data;
    JString *wlcm_str;
    int its_msg_version;
    int my_msg_version;
    const char *colon;
    const char *remote_addr;

    rv = split_brigade(me->brigade, me->buf, split_welcome, NULL);
    if (APR_SUCCESS != rv) {
        return rv;
    }

    rv = apr_brigade_length(me->buf, 1, &sz);
    assert(APR_SUCCESS == rv);
    len = sz;
    data = calloc(len, sizeof(char));
    rv = apr_brigade_flatten(me->buf, data, &len);
    apr_brigade_cleanup(me->buf);
    assert(sz == len);

    wlcm_str = jstring_new_1(len + 1);
    jstring_append_0(wlcm_str, data, len);
    me->its_welcome = welcome_message_new_2(wlcm_str);
    JXTA_OBJECT_RELEASE(wlcm_str);

    if (me->its_welcome == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Remote welcome message is malformed.\n");
        return APR_EOF;
    }

    its_msg_version = welcome_message_get_message_version(me->its_welcome);
    my_msg_version = welcome_message_get_message_version(me->my_welcome);

    me->use_msg_version = its_msg_version < my_msg_version ? its_msg_version : my_msg_version;

    JXTA_OBJECT_RELEASE(me->dest_addr);
    me->dest_addr = welcome_message_get_publicaddr(me->its_welcome);

    remote_addr = jxta_endpoint_address_get_protocol_address(me->dest_addr);
    colon = strchr(remote_addr, ':');
    if (colon && (*(colon + 1))) {
        me->port = atoi(colon + 1);
        me->ipaddr = apr_pstrndup(me->pool, remote_addr, colon - remote_addr);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "New connection with remote address %s:%d in welcome messsage.\n", 
                    me->ipaddr, me->port);

    return APR_SUCCESS;
}

static void* APR_THREAD_FUNC process_data(apr_thread_t * thd, void * arg)
{
    Jxta_transport_tcp_connection *me = arg;
    Tcp_msg_ctx *ctx = &me->msg_ctx;
    Jxta_status rv = APR_EOF;
    apr_size_t sz, len;
    char *data;

    apr_thread_mutex_lock(me->reading_lock);
    assert(me->dispatched);
    do {
        if (NULL == me->its_welcome) {
            rv = process_welcome(me);
            if (APR_SUCCESS != rv) {
                handle_reading_error(me, rv);
                break;
            }
            apr_thread_mutex_lock(me->mutex);
            me->connection_state = CONN_CONNECTED;
            apr_thread_mutex_unlock(me->mutex);
            /* unlock first, endpoint may deal with negative cache */
            apr_thread_mutex_unlock(me->reading_lock);
            emit_event(me, me->inbound ? JXTA_TRANSPORT_INBOUND_CONNECTED : JXTA_TRANSPORT_OUTBOUND_CONNECTED);
            apr_thread_mutex_lock(me->reading_lock);
        }

        /* start a new iteration ? */
        if (NULL == ctx->pool) {
            rv = apr_pool_create(&ctx->pool, me->pool);
            if (APR_SUCCESS != rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "TCP connection[%pp] failed to create pool.\n", me);
                break;
            }
        }

        if (NULL == ctx->msg) {
            rv = split_brigade(me->brigade, me->buf, split_headers, ctx);
            if (APR_SUCCESS != rv) {
                handle_reading_error(me, rv);
                break;
            }
            rv = apr_brigade_pflatten(me->buf, &data, &len, ctx->pool);
            assert(APR_SUCCESS == rv);
            apr_brigade_cleanup(me->buf);
            sz = len;
            rv = read_headers(ctx, data, &sz);
            assert(JXTA_SUCCESS == rv && sz == len);

            rv = jxta_message_create(&ctx->msg, ctx->pool);
            if (APR_SUCCESS != rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "TCP connection[%pp] failed to allocate for message.\n", me);
                break;
            }
            ctx->deficit = ctx->msg_size;
        }

        rv = split_brigade(me->brigade, me->buf, split_message, ctx);
        if (APR_SUCCESS != rv) {
            handle_reading_error(me, rv);
            break;
        }

        apr_thread_mutex_unlock(me->reading_lock);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Reading message[%pp] of %" APR_OFF_T_FMT " bytes.\n", ctx->msg,
                        ctx->msg_size);
        rv = jxta_message_read(ctx->msg, APP_MSG, brigade_read, me->buf);
        if (rv != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to read message[%pp] with status %d\n",
                            ctx->msg, rv);
            jxta_transport_tcp_connection_close(me);
            me->dispatched = FALSE;
            return (void*) rv;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received a new message[%pp] from %s:%d\n", ctx->msg, me->ipaddr,
                        me->port);
        JXTA_OBJECT_CHECK_VALID(me->endpoint);
        jxta_endpoint_service_demux(me->endpoint, ctx->msg);
        msg_ctx_reset(ctx);
        apr_brigade_cleanup(me->buf);

        apr_thread_mutex_lock(me->reading_lock);
    } while (1);    /* while loop until no more data to read */

    me->dispatched = FALSE;
    apr_thread_mutex_unlock(me->reading_lock);
    return (void*) rv;
}

static Jxta_status drain_socket(Jxta_transport_tcp_connection *me)
{
    apr_bucket *e;
    const char *ignore;
    apr_size_t len;
    Jxta_status rv;

    e = APR_BRIGADE_LAST(me->brigade);
    assert(APR_BUCKET_IS_SOCKET(e) || APR_BUCKET_IS_IMMORTAL(e));
    while (1) {
        rv = apr_bucket_read(e, &ignore, &len, APR_BLOCK_READ);
        if (APR_SUCCESS != rv) {
            break;
        }
        /* detect EOF from socket_bucket */
        if (0 == len && APR_BUCKET_IS_IMMORTAL(e)) {
            handle_reading_error(me, APR_EOF);
            return APR_EOF;
        }
        e = APR_BUCKET_NEXT(e);
    }
    return APR_SUCCESS;
}

static Jxta_status JXTA_STDCALL read_cb(void *param, void *arg)
{
    Jxta_transport_tcp_connection *me = arg;
    apr_pollfd_t *fd = param;
    apr_thread_pool_t *tp;
    Jxta_status rv;

    assert(fd->desc.s == me->shared_socket);

    apr_thread_mutex_lock(me->reading_lock);
    me->last_time_used = apr_time_now();
    drain_socket(me);
    if (me->dispatched) {
        apr_thread_mutex_unlock(me->reading_lock);
        rv = APR_SUCCESS;
    } else {
        me->dispatched = TRUE;
        apr_thread_mutex_unlock(me->reading_lock);
        jxta_endpoint_service_get_thread_pool(me->endpoint, &tp);
        rv = apr_thread_pool_push(tp, process_data, me, APR_THREAD_TASK_PRIORITY_HIGHEST, me);
    }
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_close(Jxta_transport_tcp_connection * me)
{
    Jxta_boolean was_connected;

    JXTA_OBJECT_CHECK_VALID(me);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Closing connection [%pp] to %s:%d\n", me, me->ipaddr, me->port);

    apr_thread_mutex_lock(me->mutex);
    if (CONN_INIT == me->connection_state) {
        apr_thread_mutex_unlock(me->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Not yet connected\n");
        return JXTA_SUCCESS;
    }

    if (CONN_CONNECTED != me->connection_state && CONN_CONNECTING != me->connection_state) {
        apr_thread_mutex_unlock(me->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Already closed\n");
        return JXTA_SUCCESS;
    }

    was_connected = (CONN_CONNECTED == me->connection_state);
    me->connection_state = CONN_DISCONNECTING;
    apr_thread_mutex_unlock(me->mutex);

    jxta_endpoint_service_remove_poll(me->endpoint, me->poll);

    apr_thread_mutex_lock(me->mutex);
    apr_socket_shutdown(me->shared_socket, APR_SHUTDOWN_READWRITE);
    apr_socket_close(me->shared_socket);
    apr_brigade_cleanup(me->brigade);
    me->connection_state = CONN_DISCONNECTED;
    if (was_connected) {
        emit_event(me, JXTA_TRANSPORT_CONNECTION_CLOSED);
    }
    apr_thread_mutex_unlock(me->mutex);

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL msg_wireformat_size(void *arg, const char *buf, apr_size_t len)
{
    apr_int64_t *size = arg;    /* 8 bytes */
    *size += len;
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL write_to_tcp_connection(void *stream, const char *buf, apr_size_t size)
{
    Jxta_transport_tcp_connection * myself = stream;
    Jxta_status status = JXTA_SUCCESS;
    apr_size_t left = BUFSIZE - myself->d_out_index;

    /** The buffer won't be filled by this buf */
    if (left > size) {
        memcpy(&myself->data_out_buf[myself->d_out_index], buf, size);
        myself->d_out_index += size;
        return JXTA_SUCCESS;
    }

    /** The buffer will be filled, so flush it ... */
    status = tcp_connection_flush(myself);
    if (status != JXTA_SUCCESS) {
        return status;
    }

    /** Send remaining buff directly */
    return flush(myself, buf, size);
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_send_message(Jxta_transport_tcp_connection * me, Jxta_message * msg)
{
    Jxta_status res;
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    apr_int64_t msg_size;

    JXTA_OBJECT_CHECK_VALID(_self);

    if (CONN_CONNECTED != _self->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Connection [%pp] closed -- cannot send msg [%pp].\n",
                        _self, msg);
        return JXTA_FAILED;
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_SHARE(msg);

    msg_size = 0;
    res = jxta_message_write_1(msg, APP_MSG, _self->use_msg_version, msg_wireformat_size, &msg_size);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to determine message size.\n");
        JXTA_OBJECT_RELEASE(msg);
        return res;
    }

    apr_thread_mutex_lock(_self->mutex);

    /* write message packet header */
    res = message_packet_header_write(write_to_tcp_connection, (void *) _self, msg_size, FALSE, NULL);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to write packet header.\n");
        apr_thread_mutex_unlock(_self->mutex);
        JXTA_OBJECT_RELEASE(msg);
        return res;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "send_message: msg_size=%" APR_INT64_T_FMT "\n", msg_size);

    /* write message body */
    res = jxta_message_write_1(msg, APP_MSG, _self->use_msg_version, write_to_tcp_connection, _self);
    if (_self->d_out_index != 0) {
        res = tcp_connection_flush(_self);
    }
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send the message [%pp]\n", msg);
    } else {
        _self->last_time_used = apr_time_now();
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] sent on [%pp]\n", msg, _self);
    }

    apr_thread_mutex_unlock(_self->mutex);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status flush(Jxta_transport_tcp_connection * me, const char *buf, apr_size_t size)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t written = size;
    apr_size_t total_written = 0;
    apr_int32_t nsds = 1;

    while (written > 0) {
        status = apr_socket_send(me->shared_socket, buf, &written);
        if (APR_STATUS_IS_EAGAIN(status) || APR_STATUS_IS_TIMEUP(status)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "Waiting to write to connection[%pp]\n", me);
            apr_poll(&me->pollfd, 1, &nsds, -1);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "Continue to write to connection[%pp]\n", me);
        } else if (APR_SUCCESS != status) {
            /* Drop the all message ... */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to send data through connection[%pp], "
                            "status %d\n", me, status);
            return JXTA_FAILED;
        }
        total_written += written;
        buf += written;
        written = size - total_written;
    }
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_flush(Jxta_transport_tcp_connection * _self)
{
    Jxta_status status;

    status = flush(_self, _self->data_out_buf, _self->d_out_index);
    _self->d_out_index = 0;

    return status;
}

Jxta_time get_tcp_connection_last_time_used(Jxta_transport_tcp_connection * tcp_connection)
{
    assert(tcp_connection);
    JXTA_OBJECT_CHECK_VALID(tcp_connection);
    return apr_time_as_msec(tcp_connection->last_time_used);
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_destaddr(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);

    return JXTA_OBJECT_SHARE(_self->dest_addr);
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_connaddr(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(_self->its_welcome);

    return welcome_message_get_destaddr(_self->its_welcome);
}

JXTA_DECLARE(Jxta_id *) jxta_transport_tcp_connection_get_destination_peerid(Jxta_transport_tcp_connection * me)
{
    Jxta_transport_tcp_connection *_self = me;
    Jxta_id *peer_jid;
    JString *tmp;
    Jxta_status res;

    assert(_self);
    assert(_self->its_welcome);
    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(_self->its_welcome);

    tmp = welcome_message_get_peerid(_self->its_welcome);
    res = jxta_id_from_jstring(&peer_jid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    return (JXTA_SUCCESS == res) ? peer_jid : NULL;
}

JXTA_DECLARE(char *) jxta_transport_tcp_connection_get_ipaddr(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);

    return strdup(_self->ipaddr);
}

JXTA_DECLARE(const char *) jxta_transport_tcp_connection_ipaddr(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);

    return _self->ipaddr;
}

JXTA_DECLARE(apr_port_t) jxta_transport_tcp_connection_get_port(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);

    return _self->port;
}

JXTA_DECLARE(Jxta_boolean) jxta_transport_tcp_connection_is_connected(Jxta_transport_tcp_connection * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    JXTA_OBJECT_CHECK_VALID(_self);

    return (CONN_CONNECTED == _self->connection_state);
}

Tcp_connection_state tcp_connection_state(Jxta_transport_tcp_connection *me)
{
    return me->connection_state;
}

Jxta_status tcp_connection_get_messenger(Jxta_transport_tcp_connection *me, apr_interval_time_t timeout, TcpMessenger **msgr)
{
    apr_interval_time_t wait;
    apr_time_t elapsed;
    
    wait = 100000; /* 100 ms */
    elapsed = 0;
    while (CONN_CONNECTED != me->connection_state && elapsed < timeout) {
        if (CONN_DISCONNECTED == me->connection_state) {
            *msgr = NULL;
            return JXTA_FAILED;
        }
        apr_sleep(wait);
        elapsed += wait;
        wait <<= 2;
    }

    if (CONN_CONNECTED != me->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE
                        "Tcp connection[%pp] is not ready after %" APR_TIME_T_FMT " microseconds timeout.\n", 
                        me, elapsed);
        *msgr = NULL;
        return JXTA_TIMEOUT;
    }

    /* me->msgr is singleton placeholder */
    if (me->msgr) {
        *msgr = JXTA_OBJECT_SHARE(me->msgr);
    } else {
        *msgr = me->msgr = tcp_messenger_new(me);
    }

    return JXTA_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
