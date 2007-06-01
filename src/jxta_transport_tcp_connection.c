/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_transport_tcp_connection.c,v 1.52 2005/11/21 01:19:23 mmx2005 Exp $
 */

static const char *__log_cat = "TCP_CONNECTION";

#include <apr.h>
#include <apr_thread_proc.h>
#include <apr_strings.h>
#include <apr_time.h>
#include <assert.h>

#include "jpr/jpr_apr_wrapper.h"

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_tcp_message_packet_header.h"

#define BUFSIZE		8192    /* BUFSIZ */

#define GREETING	"JXTAHELLO"
#define SPACE		" "
#define CURRENTVERSION	"1.1"
#define CRLF		"\r\n"

typedef struct _welcome_message {
    JXTA_OBJECT_HANDLE;

    Jxta_endpoint_address *dest_addr;
    Jxta_endpoint_address *public_addr;
    char *peer_id;
    Jxta_boolean noprop;

    JString *welcome_str;
    const char *welcome_bytes;
} WelcomeMessage;

struct _jxta_transport_tcp_connection {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
    apr_thread_t *recv_thread;
    volatile int connection_state;

    Jxta_transport_tcp *tp;

    Jxta_endpoint_address *dest_addr;
    apr_socket_t *shared_socket;
    apr_sockaddr_t *intf_addr;
    char *ipaddr;
    apr_port_t port;
    WelcomeMessage *my_welcome;
    WelcomeMessage *its_welcome;
    Jxta_boolean initiator;

    char *data_in_buf;          /* input stream */
    int d_index;
    apr_ssize_t d_size;

    char *data_out_buf;         /* output stream */
    int d_out_index;

    char *header_buf;

    Jxta_endpoint_service *endpoint;
};

enum e_tcp_connection_state {
    CONN_INIT = 0,
    CONN_CONNECTED,
    CONN_STOPPING,
    CONN_DISCONNECTED,
    CONN_STOPPED
};

static Jxta_transport_tcp_connection *tcp_connection_new(Jxta_transport_tcp * tp, Jxta_boolean initiator);
static void tcp_connection_free(Jxta_object * me);
static Jxta_status connect_to_from(Jxta_transport_tcp_connection * me, const char *local_ipaddr);
static Jxta_status tcp_connection_start_socket(Jxta_transport_tcp_connection * me);
static void *APR_THREAD_FUNC tcp_connection_thread(apr_thread_t * t, void *arg);

static WelcomeMessage *welcome_message_new(void);
static WelcomeMessage *welcome_message_new_1(Jxta_endpoint_address * dest_addr, Jxta_endpoint_address * public_addr,
                                             const char *peerid, Jxta_boolean noprop);
static WelcomeMessage *welcome_message_new_2(Jxta_transport_tcp_connection * conn);
static void welcome_message_free(Jxta_object * me);
static Jxta_status welcome_message_parse(WelcomeMessage * me);
static Jxta_endpoint_address *welcome_message_destaddr_clone(Jxta_endpoint_address * dest_addr);

static Jxta_status tcp_connection_read(Jxta_transport_tcp_connection * me, char *buf, apr_size_t * size);
static Jxta_status tcp_connection_read_n(Jxta_transport_tcp_connection * me, char *buf, apr_size_t size);
static Jxta_status tcp_connection_unread(Jxta_transport_tcp_connection * me, char *buf, apr_size_t size);
static Jxta_status tcp_connection_write(Jxta_transport_tcp_connection * me, const char *buf, apr_size_t size,
                                        Jxta_boolean zero_copy);
static Jxta_status tcp_connection_flush(Jxta_transport_tcp_connection * me);

static void tcp_connection_get_intf_from_jxta_endpoint_address(Jxta_endpoint_address * ea, char *ipaddr, apr_port_t * port);

static Jxta_transport_tcp_connection *tcp_connection_new(Jxta_transport_tcp * tp, Jxta_boolean initiator)
{
    Jxta_transport_tcp_connection *me;
    apr_status_t status;

    /* create object */
    me = (Jxta_transport_tcp_connection *) calloc(1, sizeof(Jxta_transport_tcp_connection));
    if (me == NULL)
        return NULL;

    /* initialize it */
    JXTA_OBJECT_INIT(me, tcp_connection_free, NULL);

    /* apr setting */
    status = apr_pool_create(&me->pool, NULL);
    if (APR_SUCCESS != status) {
        /* Free the memory that has been already allocated */
        free(me);
        return NULL;
    }

    /* apr_mutex */
    status = apr_thread_mutex_create(&me->mutex, APR_THREAD_MUTEX_NESTED, me->pool);
    if (APR_SUCCESS != status) {
        /* Free the memory that has been already allocated */
        apr_pool_destroy(me->pool);
        free(me);
        return NULL;
    }

    me->connection_state = CONN_INIT;

    /* setting */
    me->tp = tp;
    me->initiator = initiator;
    me->shared_socket = NULL;
    me->intf_addr = NULL;
    me->ipaddr = NULL;
    me->port = 0;
    me->dest_addr = NULL;
    me->my_welcome = NULL;
    me->its_welcome = NULL;

    /* input stream */
    me->data_in_buf = (char *) malloc(BUFSIZE);
    if (me->data_in_buf == NULL) {
        apr_pool_destroy(me->pool);
        free(me);
        return NULL;
    }
    me->d_index = 0;
    me->d_size = 0;

    /* output stream */
    me->data_out_buf = (char *) malloc(BUFSIZE);
    if (me->data_out_buf == NULL) {
        free(me->data_in_buf);
        apr_pool_destroy(me->pool);
        free(me);
        return NULL;
    }
    me->d_out_index = 0;

    me->header_buf = malloc(HEADER_BUFSIZE);
    if (me->header_buf == NULL) {
        free(me->data_in_buf);
        free(me->data_out_buf);
        apr_pool_destroy(me->pool);
        free(me);
        return NULL;
    }

    /* apr_thread */
    me->recv_thread = NULL;

    me->endpoint = jxta_transport_tcp_get_endpoint_service(tp);

    return me;
}

static void tcp_connection_free(Jxta_object * me)
{
    Jxta_transport_tcp_connection *_self = (Jxta_transport_tcp_connection *) me;

    /* _self->ipaddr is allocated in _self->pool so we need not to free */
    if (_self->endpoint) {
        JXTA_OBJECT_RELEASE(_self->endpoint);
    }

    if (_self->data_in_buf)
        free(_self->data_in_buf);

    if (_self->data_out_buf)
        free(_self->data_out_buf);

    if (_self->header_buf)
        free(_self->header_buf);

    if (_self->my_welcome) {
        JXTA_OBJECT_RELEASE(_self->my_welcome);
    }
    if (_self->its_welcome) {
        JXTA_OBJECT_RELEASE(_self->its_welcome);
    }
    if (_self->dest_addr)
        JXTA_OBJECT_RELEASE(_self->dest_addr);

    if (_self->mutex) {
        apr_thread_mutex_destroy(_self->mutex);
    }

    if (_self->pool)
        apr_pool_destroy(_self->pool);

    free((void *) _self);
}

static Jxta_status connect_to_from(Jxta_transport_tcp_connection * me, const char *local_ipaddr)
{
    apr_sockaddr_t *intfaddr;
    apr_status_t status;
    apr_port_t port;

    status = apr_sockaddr_info_get(&me->intf_addr, me->ipaddr, APR_UNSPEC, me->port, 0, me->pool);
    if (APR_SUCCESS != status)
        return status;

    status = apr_sockaddr_info_get(&intfaddr, local_ipaddr, APR_INET, 0, 0, me->pool);
    if (APR_SUCCESS != status)
        return status;

#if CHECK_APR_VERSION(1, 0, 0)
    status = apr_socket_create(&me->shared_socket, me->intf_addr->family, SOCK_STREAM, APR_PROTO_TCP, me->pool);
#else
    status = apr_socket_create_ex(&me->shared_socket, me->intf_addr->family, SOCK_STREAM, APR_PROTO_TCP, me->pool);
#endif
    if (APR_SUCCESS != status)
        return status;

    status = apr_socket_bind(me->shared_socket, intfaddr);
    if (APR_SUCCESS != status)
        return status;

    /* connection timeout setting (1 sec) */
    apr_socket_timeout_set(me->shared_socket, CONNECTION_TIMEOUT);

    status = apr_socket_connect(me->shared_socket, me->intf_addr);
    if (APR_SUCCESS != status)
        return status;

    /* connection timeout reset */
    apr_socket_timeout_set(me->shared_socket, -1);
    apr_socket_opt_set(me->shared_socket, APR_TCP_NODELAY, 1);

    apr_socket_addr_get(&intfaddr, APR_LOCAL, me->shared_socket);
    apr_sockaddr_ip_get(&me->ipaddr, me->intf_addr);
    /* apr_sockaddr_port_get(&port, intfaddr); */
    port = intfaddr->port;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_1(Jxta_transport_tcp * tp,
                                                                                  Jxta_endpoint_address * dest)
{
    apr_status_t status;
    Jxta_transport_tcp_connection *me;
    char *local_ipaddr;
    char *remote_ipaddr;
    const char *protocol_address;
    const char *colon;

    me = tcp_connection_new(tp, TRUE);

    if (me == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create tcp connection\n");
        return NULL;
    }

    protocol_address = jxta_endpoint_address_get_protocol_address(dest);

    colon = strchr(protocol_address, ':');
    if (colon && (*(colon + 1))) {
        me->port = atoi(colon + 1);
        remote_ipaddr = (char *) calloc(colon - protocol_address + 1, sizeof(char));
        me->ipaddr = remote_ipaddr;
        me->ipaddr[0] = 0;
        strncat(me->ipaddr, protocol_address, colon - protocol_address);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not parse endpoint address.\n");
        JXTA_OBJECT_RELEASE(me);
        return NULL;
    }

    me->dest_addr = JXTA_OBJECT_SHARE(dest);
    local_ipaddr = jxta_transport_tcp_get_local_ipaddr(tp);

    status = connect_to_from(me, local_ipaddr);
    free(local_ipaddr);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed connect attempt : %s\n", msg);
        JXTA_OBJECT_RELEASE(me);
        return NULL;
    }

    if (JXTA_SUCCESS != tcp_connection_start_socket(me)) {
        JXTA_OBJECT_RELEASE(me);
        return NULL;
    }

    return me;
}

JXTA_DECLARE(Jxta_transport_tcp_connection *) jxta_transport_tcp_connection_new_2(Jxta_transport_tcp * tp,
                                                                                  apr_socket_t * inc_socket)
{
    Jxta_transport_tcp_connection *me = tcp_connection_new(tp, FALSE);
    char *protocol_address;
    int len;

    if (me == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create tcp connection\n");
        return NULL;
    }

    me->shared_socket = inc_socket;

    apr_socket_addr_get(&me->intf_addr, APR_REMOTE, me->shared_socket);
    apr_sockaddr_ip_get(&me->ipaddr, me->intf_addr);
    /* apr_sockaddr_port_get(&me->port, me->intf_addr); */
    me->port = me->intf_addr->port;

    len = strlen(me->ipaddr) + 10;
    protocol_address = (char *) malloc(len);
    apr_snprintf(protocol_address, len, "%s:%d", me->ipaddr, me->port);
    /* used to compose welcome message */
    me->dest_addr = jxta_endpoint_address_new_2("tcp", protocol_address, NULL, NULL);
    free(protocol_address);

    if (JXTA_SUCCESS == tcp_connection_start_socket(me)) {
        assert(NULL != me->its_welcome);
        JXTA_OBJECT_RELEASE(me->dest_addr);
        me->dest_addr = welcome_message_destaddr_clone(me->its_welcome->public_addr);

        /* 
         * Since me->ipaddr is in apr_pool, so it must not be freed.
         * That area is freed when apr_pool_destroy called.
         */
        tcp_connection_get_intf_from_jxta_endpoint_address(me->dest_addr, me->ipaddr, &me->port);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "New connection -> %s:%d\n", me->ipaddr, me->port);
    } else {
        JXTA_OBJECT_RELEASE(me);
        me = NULL;
    }

    return me;
}

/**
    Handle welcome message.
**/
static Jxta_status tcp_connection_start_socket(Jxta_transport_tcp_connection * me)
{
    Jxta_endpoint_address *public_addr;
    char *peerid;
    Jxta_status res;
    char *caddress = NULL;

    peerid = jxta_transport_tcp_get_peerid(me->tp);
    public_addr = jxta_transport_tcp_get_public_addr(me->tp);

    apr_socket_opt_set(me->shared_socket, APR_SO_KEEPALIVE, 1); /* Keep Alive */
    apr_socket_opt_set(me->shared_socket, APR_SO_LINGER, LINGER_DELAY); /* Linger Delay */
    apr_socket_opt_set(me->shared_socket, APR_TCP_NODELAY, 1);  /* disable nagel's */

    me->my_welcome = welcome_message_new_1(me->dest_addr, public_addr, peerid, FALSE);
    free(peerid);

    /* send my welcome message */
    res = tcp_connection_write(me, me->my_welcome->welcome_bytes, strlen(me->my_welcome->welcome_bytes), TRUE);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "My welcome message sent ..%s\n",
                    (caddress = jxta_endpoint_address_to_string(me->my_welcome->dest_addr)));
    free(caddress);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error socket write\n");
        return res;
    }

    /* recv its welcome message */
    me->its_welcome = welcome_message_new_2(me);

    if (me->its_welcome != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Its welcome message received ..%s\n",
                        (caddress = jxta_endpoint_address_to_string(me->its_welcome->dest_addr)));
        free(caddress);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Its welcome message is malformed.\n");
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_start(Jxta_transport_tcp_connection * me)
{
    apr_status_t status;

    assert( me != NULL);

    if (CONN_INIT != me->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Connection [%p] already started.\n", me);
        return JXTA_FAILED;
    }

    status = apr_thread_create(&me->recv_thread, NULL, tcp_connection_thread, me, me->pool);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to start thread : %s\n", msg);
        apr_thread_mutex_lock(me->mutex);
        me->connection_state = CONN_STOPPED;
        apr_thread_mutex_unlock(me->mutex);
        return JXTA_FAILED;
    }

    apr_thread_mutex_lock(me->mutex);
    me->connection_state = CONN_CONNECTED;
    apr_thread_mutex_unlock(me->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Connection [%p] to %s:%d started.\n", me, me->ipaddr, me->port);

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL read_from_tcp_connection(void *me, char *buf, apr_size_t len)
{
    Jxta_transport_tcp_connection *_self = (Jxta_transport_tcp_connection *) me;
    Jxta_status res;

    res = tcp_connection_read_n(_self, buf, len);
    return res;
}

static void *APR_THREAD_FUNC tcp_connection_thread(apr_thread_t * thread, void *me)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_transport_tcp_connection *_self = (Jxta_transport_tcp_connection *) me;

    JXTA_OBJECT_CHECK_VALID(_self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP incoming message thread for [%p] %s:%d started.\n", _self, _self->ipaddr,
                    _self->port);

    apr_thread_mutex_lock(_self->mutex);
    _self->connection_state = CONN_CONNECTED;
    apr_thread_mutex_unlock(_self->mutex);

    while (CONN_CONNECTED == _self->connection_state) {
        JXTA_LONG_LONG msg_size;

        res = message_packet_header_read(_self->header_buf, read_from_tcp_connection, (void *) _self, &msg_size, FALSE, NULL);

        if (res == JXTA_IOERR) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "I/O Error to read message packet header\n");
            break;
        }

        if (res == JXTA_FAILED) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to read message packet header\n");
            break;
        }

        if (msg_size > 0) {
            Jxta_message *msg = jxta_message_new();
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Reading message [%p] of %" APR_INT64_T_FMT " bytes.\n", msg,
                            msg_size);

            res = jxta_message_read(msg, APP_MSG, read_from_tcp_connection, _self);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to read message [%p]\n", msg);
                JXTA_OBJECT_RELEASE(msg);
                break;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received a new message [%p] from %s:%d\n", msg, _self->ipaddr,
                            _self->port);

            JXTA_OBJECT_CHECK_VALID(_self->endpoint);
            jxta_endpoint_service_demux(_self->endpoint, msg);
            JXTA_OBJECT_RELEASE(msg);
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP incoming message thread for [%p] %s:%d stopping : %d \n", _self,
                    _self->ipaddr, _self->port, res);

    if (CONN_CONNECTED == _self->connection_state) {
        apr_thread_mutex_lock(_self->mutex);
        _self->connection_state = CONN_DISCONNECTED;
        apr_thread_mutex_unlock(_self->mutex);
    }
    apr_thread_exit(thread, res);

    /* NOTREACHED */
    return NULL;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_close(Jxta_transport_tcp_connection * me)
{
    apr_status_t status;

    JXTA_OBJECT_CHECK_VALID(me);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Closing connection [%p] to %s:%d\n", me, me->ipaddr, me->port);

    if (CONN_INIT == me->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Not yet connected\n");
        return JXTA_SUCCESS;
    }

    if (CONN_STOPPED == me->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Already closed\n");
        return JXTA_SUCCESS;
    }

    apr_thread_mutex_lock(me->mutex);
    me->connection_state = CONN_STOPPED;
    apr_thread_mutex_unlock(me->mutex);

    apr_socket_shutdown(me->shared_socket, APR_SHUTDOWN_READWRITE);
    apr_socket_close(me->shared_socket);

    if(me->recv_thread){
        apr_thread_join(&status, me->recv_thread);
        if (status != APR_SUCCESS) {
            char msg[256];
            apr_strerror(status, msg, sizeof(msg));
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error result from incoming message thread : %s\n", msg);
        }
    }

    /* don't remove messenger, transport will do that whithin get_messenger in case connection is closed */
    /*
       res = jxta_transport_tcp_remove_messenger(me->tp, me->dest_addr);
       if (res != JXTA_SUCCESS) {
       jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to remove messenger : %d\n", res);
       }
     */
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL msg_wireformat_size(void *arg, const char *buf, apr_size_t len)
{
    JXTA_LONG_LONG *size = (JXTA_LONG_LONG *) arg;  /* 8 bytes */
    *size += len;
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL write_to_tcp_connection(void *stream, const char *buf, apr_size_t len)
{
    return tcp_connection_write((Jxta_transport_tcp_connection *) stream, buf, len, FALSE);
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_send_message(Jxta_transport_tcp_connection * me, Jxta_message * msg)
{
    JXTA_LONG_LONG msg_size;
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(me);

    if (CONN_CONNECTED != me->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Connection [%p] closed -- cannot send msg [%p].\n", me,
                        msg);
        return JXTA_FAILED;
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_SHARE(msg);

    msg_size = (JXTA_LONG_LONG) 0;
    jxta_message_write(msg, APP_MSG, msg_wireformat_size, &msg_size);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "send_message: msg_size=%" APR_INT64_T_FMT "\n", msg_size);

    apr_thread_mutex_lock(me->mutex);

    /* write message packet header */
    message_packet_header_write(write_to_tcp_connection, (void *) me, msg_size, FALSE, NULL);

    /* write message body */
    res = jxta_message_write(msg, APP_MSG, write_to_tcp_connection, me);
    if (me->d_out_index != 0) {
        res = tcp_connection_flush(me);
    }
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send the message [%p]\n", msg);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%p] sent on [%p]\n", msg, me);
    }

    apr_thread_mutex_unlock(me->mutex);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static WelcomeMessage *welcome_message_new(void)
{
    WelcomeMessage *me;

    me = (WelcomeMessage *) calloc(1, sizeof(WelcomeMessage));
    if (me == NULL)
        return NULL;

    JXTA_OBJECT_INIT(me, welcome_message_free, NULL);

    me->dest_addr = NULL;
    me->public_addr = NULL;
    me->peer_id = NULL;
    me->noprop = FALSE;
    me->welcome_str = NULL;
    me->welcome_bytes = NULL;

    return me;
}

static WelcomeMessage *welcome_message_new_1(Jxta_endpoint_address * dest_addr, Jxta_endpoint_address * public_addr,
                                             const char *peerid, Jxta_boolean noprop)
{
    WelcomeMessage *me;
    char *caddress = NULL;

    me = welcome_message_new();
    if (me != NULL) {
        JXTA_OBJECT_CHECK_VALID(dest_addr);
        JXTA_OBJECT_SHARE(dest_addr);
        JXTA_OBJECT_SHARE(public_addr);

        me->dest_addr = dest_addr;
        me->public_addr = public_addr;
        me->peer_id = strdup(peerid);
        me->noprop = noprop;

        me->welcome_str = jstring_new_0();
        jstring_append_2(me->welcome_str, GREETING);
        jstring_append_2(me->welcome_str, SPACE);
        jstring_append_2(me->welcome_str, (caddress = jxta_endpoint_address_to_string(me->dest_addr)));
        free(caddress);
        jstring_append_2(me->welcome_str, "");
        jstring_append_2(me->welcome_str, SPACE);
        jstring_append_2(me->welcome_str, (caddress = jxta_endpoint_address_to_string(me->public_addr)));
        free(caddress);
        jstring_append_2(me->welcome_str, SPACE);
        jstring_append_2(me->welcome_str, peerid);
        jstring_append_2(me->welcome_str, SPACE);
        jstring_append_2(me->welcome_str, me->noprop ? "0" : "1");
        jstring_append_2(me->welcome_str, SPACE);
        jstring_append_2(me->welcome_str, CURRENTVERSION);
        jstring_append_2(me->welcome_str, "\r");
        jstring_append_2(me->welcome_str, "\n");

        me->welcome_bytes = strdup(jstring_get_string(me->welcome_str));
    }
    return me;
}

static WelcomeMessage *welcome_message_new_2(Jxta_transport_tcp_connection * conn)
{
    WelcomeMessage *me;
    char *buf, *line;
    apr_size_t len, max_msg_size = 4096;
    unsigned int i, j;

    Jxta_boolean saw_CR, saw_CRLF;

    saw_CR = saw_CRLF = FALSE;

    buf = (char *) malloc(BUFSIZE * sizeof(char));
    if (buf == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "buf malloc error\n");
        return NULL;
    }
    line = (char *) malloc(max_msg_size * sizeof(char));
    if (line == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "line malloc error\n");
        free(buf);
        return NULL;
    }

    me = welcome_message_new();
    if (me == NULL) {
        free(buf);
        free(line);
        return NULL;
    }

    i = 0;
    while (!saw_CRLF) {
        len = BUFSIZE;
        if (JXTA_SUCCESS != tcp_connection_read(conn, buf, &len)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to read from tcp connection\n");
            free(buf);
            free(line);
            JXTA_OBJECT_RELEASE(me);
            return NULL;
        }
        for (j = 0; j < len && !saw_CRLF; j++) {
            switch (buf[j]) {
            case '\r':
                if (saw_CR)
                    line[i++] = buf[j];
                saw_CR = TRUE;
                break;
            case '\n':
                if (saw_CR)
                    saw_CRLF = TRUE;
                else
                    line[i++] = buf[j];
                break;
            default:
                line[i++] = buf[j];
                saw_CR = FALSE;
            }
        }
        if (i >= max_msg_size) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Welcome message too long.\n");
            free(buf);
            free(line);
            JXTA_OBJECT_RELEASE(me);
            return NULL;
        }
    }
    len -= j;
    tcp_connection_unread(conn, &buf[j], len);
    line[i] = 0;

    me->welcome_bytes = strdup(line);
    free(buf);
    free(line);

    me->welcome_str = jstring_new_2(me->welcome_bytes);
    if (welcome_message_parse(me) != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(me);
        return NULL;
    }

    return me;
}


static void welcome_message_free(Jxta_object * me)
{
    WelcomeMessage *_self = (WelcomeMessage *) me;

    if (_self->welcome_bytes != NULL)
        free((void *) _self->welcome_bytes);

    if (_self->dest_addr)
        JXTA_OBJECT_RELEASE(_self->dest_addr);

    if (_self->public_addr)
        JXTA_OBJECT_RELEASE(_self->public_addr);

    if (_self->peer_id)
        free(_self->peer_id);

    if (_self->welcome_str)
        JXTA_OBJECT_RELEASE(_self->welcome_str);

    free(_self);
}

static Jxta_status welcome_message_parse(WelcomeMessage * me)
{
    char *line = strdup(me->welcome_bytes);
    char *state, *res;
    char *delim;

    delim = strdup(SPACE);

    /* <GREETING> */
    if ((res = apr_strtok(line, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    if (strcmp(res, GREETING) != 0) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }

    /* <WELCOMEDEST> */
    if ((res = apr_strtok(NULL, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    me->dest_addr = jxta_endpoint_address_new(res);
    JXTA_OBJECT_CHECK_VALID(me->dest_addr);

    /* <WELCOMESRC> */
    if ((res = apr_strtok(NULL, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    me->public_addr = jxta_endpoint_address_new(res);
    JXTA_OBJECT_CHECK_VALID(me->public_addr);

    /* <WELCOMEPEER> */
    if ((res = apr_strtok(NULL, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    me->peer_id = strdup(res);

    /* <NOPROP> */
    if ((res = apr_strtok(NULL, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    if (atoi(res) == 1)
        me->noprop = TRUE;
    else if (atoi(res) == 0)
        me->noprop = FALSE;
    else {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }

    /* <VERSION> */
    if ((res = apr_strtok(NULL, delim, &state)) == NULL) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }
    if (strcmp(res, CURRENTVERSION) != 0) {
        free(line);
        free(delim);
        return JXTA_FAILED;
    }

    free(line);
    free(delim);
    return JXTA_SUCCESS;
}

static Jxta_endpoint_address *welcome_message_destaddr_clone(Jxta_endpoint_address * dest_addr)
{
    char *caddress = NULL;
    Jxta_endpoint_address *addr = NULL;

    JXTA_OBJECT_CHECK_VALID(dest_addr);

    caddress = jxta_endpoint_address_to_string(dest_addr);
    addr = jxta_endpoint_address_new(caddress);
    free(caddress);
    return addr;
}

static Jxta_status tcp_connection_read(Jxta_transport_tcp_connection * me, char *buf, apr_size_t * size)
{
    apr_status_t status;
    size_t read = 0, to_read = 0;

    if (buf == NULL || *size == 0)
        return JXTA_FAILED;

    if (me->d_size > 0) {
        read = ((signed) *size > me->d_size) ? me->d_size : *size;
        to_read = *size - read;

        memcpy(buf, &me->data_in_buf[me->d_index], read);

        *size = read;
        me->d_size -= read;
        if (me->d_size == 0)
            me->d_index = 0;
        else
            me->d_index += read;
    } else {
        to_read = *size;
        *size = 0;
    }

    if (to_read > 0) {
        status = apr_socket_recv(me->shared_socket, &buf[read], &to_read);
        if (APR_SUCCESS != status) {
            return JXTA_FAILED;
        }
        *size += to_read;
    }
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_read_n(Jxta_transport_tcp_connection * me, char *buf, apr_size_t size)
{
    Jxta_status res;
    int offset;
    apr_size_t bytesread;

    for (bytesread = size, offset = 0; size > 0;) {
        res = tcp_connection_read(me, buf + offset, &bytesread);
        if (res != JXTA_SUCCESS) {
            return JXTA_FAILED;
        }
        offset += bytesread;
        size -= bytesread;
        bytesread = size;
    }

    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_unread(Jxta_transport_tcp_connection * me, char *buf, apr_size_t size)
{
    if ((signed) size <= 0)
        return JXTA_FAILED;

    if (me->d_index >= (signed) size)
        me->d_index -= size;
    else
        memcpy(&me->data_in_buf[me->d_index + me->d_size], buf, size);

    me->d_size += size;
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_write(Jxta_transport_tcp_connection * me, const char *buf, apr_size_t size,
                                        Jxta_boolean zero_copy)
{
    Jxta_status jxta_status = JXTA_SUCCESS;
    apr_status_t status = APR_SUCCESS;
    apr_size_t written = 0;
    apr_size_t left = BUFSIZE - me->d_out_index;

    if (zero_copy)
        goto Direct_buf;

  /** The buffer won't be filled by this buf */
    if (left > size) {
        memcpy(&me->data_out_buf[me->d_out_index], buf, size);
        me->d_out_index += size;
        size -= size;
    } else {
    /** The buffer will be filled, so flush it ... */
        jxta_status = tcp_connection_flush(me);
        if (jxta_status != JXTA_SUCCESS)
            return jxta_status;

      Direct_buf:
    /** Start to send remaining buff directly */
        while (size > 0) {
            written = size;
            status = apr_socket_send(me->shared_socket, buf, &written);
            if (APR_SUCCESS != status)
                return JXTA_FAILED;
            buf += written;
            size -= written;
        }
    }

    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_flush(Jxta_transport_tcp_connection * me)
{
    apr_status_t status = APR_SUCCESS;
    char *pt = me->data_out_buf;
    apr_size_t written = me->d_out_index;
    apr_size_t total_written = 0;

  /** Start to send the data_out_buf */
    while (written > 0) {
        status = apr_socket_send(me->shared_socket, pt, &written);
        if (APR_SUCCESS != status) {
      /** Drop the all message ... */
            me->d_out_index = 0;
            return JXTA_FAILED;
        }
        total_written += written;
        pt += written;
        written = me->d_out_index - total_written;
    }
  /** The data_out_buf is now empty */
    me->d_out_index = 0;

    return JXTA_SUCCESS;
}

static void tcp_connection_get_intf_from_jxta_endpoint_address(Jxta_endpoint_address * ea, char *ipaddr, apr_port_t * port)
{
    char *addr, *state;
    JXTA_OBJECT_CHECK_VALID(ea);

    addr = strdup(jxta_endpoint_address_get_protocol_address(ea));

    apr_strtok(addr, ":", &state);

    strcpy(ipaddr, addr);
    *port = (apr_port_t) atoi(state);

    free(addr);
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_destaddr(Jxta_transport_tcp_connection * me)
{
    JXTA_OBJECT_CHECK_VALID(me);

    return JXTA_OBJECT_SHARE(me->dest_addr);
}

JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_tcp_connection_get_connaddr(Jxta_transport_tcp_connection * me)
{
    Jxta_transport_tcp_connection *_self = me;
    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(_self->its_welcome);

    return JXTA_OBJECT_SHARE(_self->its_welcome->dest_addr);
}

JXTA_DECLARE(Jxta_id *) jxta_transport_tcp_connection_get_destination_peerid(Jxta_transport_tcp_connection * me)
{
    Jxta_transport_tcp_connection *_self = me;
    Jxta_id *peer_jid;
    JString *tmp;
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(_self->its_welcome);

    tmp = jstring_new_2(_self->its_welcome->peer_id);
    res = jxta_id_from_jstring(&peer_jid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    return (JXTA_SUCCESS == res) ? peer_jid : NULL;
}

JXTA_DECLARE(char *) jxta_transport_tcp_connection_get_ipaddr(Jxta_transport_tcp_connection * me)
{
    Jxta_transport_tcp_connection *_self = me;
    JXTA_OBJECT_CHECK_VALID(_self);
    return strdup(_self->ipaddr);
}

JXTA_DECLARE(apr_port_t) jxta_transport_tcp_connection_get_port(Jxta_transport_tcp_connection * me)
{
    Jxta_transport_tcp_connection *_self = me;
    JXTA_OBJECT_CHECK_VALID(_self);
    return _self->port;
}

JXTA_DECLARE(Jxta_boolean) jxta_transport_tcp_connection_is_connected(Jxta_transport_tcp_connection * me)
{
    return (CONN_CONNECTED == me->connection_state);
}

/* vim: set ts=4 sw=4 tw=130 et: */
