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
 * $Id: jxta_transport_tcp_connection.c,v 1.71 2006/09/25 21:36:41 slowhog Exp $
 */

static const char *__log_cat = "TCP_CONNECTION";

#include "jxta_apr.h"

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_transport_tcp_private.h"
#include "jxta_tcp_message_packet_header.h"
#include "jxta_transport_welcome_message.h"

#define BUFSIZE		8192    /* BUFSIZ */

struct _jxta_transport_tcp_connection {
    JXTA_OBJECT_HANDLE;

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
    apr_thread_t *recv_thread;
    volatile int connection_state;

    Jxta_transport_tcp *tp;

    /* Remote address */
    Jxta_endpoint_address *dest_addr;
    char *ipaddr;
    apr_port_t port;
    apr_sockaddr_t *intf_addr;

    apr_socket_t *shared_socket;

    Jxta_welcome_message *my_welcome;
    Jxta_welcome_message *its_welcome;
    int use_msg_version;
    Jxta_boolean initiator;

    char *data_in_buf;          /* input stream */
    int d_index;
    apr_ssize_t d_size;

    char *data_out_buf;         /* output stream */
    int d_out_index;

    char *header_buf;

    Jxta_endpoint_service *endpoint;
};

typedef struct _jxta_transport_tcp_connection _jxta_transport_tcp_connection;

enum e_tcp_connection_state {
    CONN_INIT = 0,
    CONN_CONNECTED,
    CONN_STOPPING,
    CONN_DISCONNECTED,
    CONN_STOPPED
};

static Jxta_transport_tcp_connection *tcp_connection_new(Jxta_transport_tcp * tp, Jxta_boolean initiator);
static void tcp_connection_free(Jxta_object * me);

static Jxta_status connect_to_from(_jxta_transport_tcp_connection * _self, const char *local_ipaddr);
static void *APR_THREAD_FUNC tcp_connection_thread(apr_thread_t * t, void *arg);

static Jxta_status tcp_connection_start_socket(_jxta_transport_tcp_connection * _self);
static Jxta_status tcp_connection_read(_jxta_transport_tcp_connection * _self, char *buf, apr_size_t * size);
static Jxta_status tcp_connection_read_n(_jxta_transport_tcp_connection * _self, char *buf, apr_size_t size);
static Jxta_status tcp_connection_unread(_jxta_transport_tcp_connection * _self, char *buf, apr_size_t size);
static Jxta_status tcp_connection_write(_jxta_transport_tcp_connection * _self, const char *buf, apr_size_t size,
                                        Jxta_boolean zero_copy);
static Jxta_status tcp_connection_flush(_jxta_transport_tcp_connection * _self);
static JString * tcp_connection_read_welcome(_jxta_transport_tcp_connection * _self);

static void tcp_connection_get_intf_from_jxta_endpoint_address(Jxta_endpoint_address * ea, char *ipaddr, apr_port_t * port);



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
    _self->initiator = initiator;
    _self->shared_socket = NULL;
    _self->intf_addr = NULL;
    _self->ipaddr = NULL;
    _self->port = 0;
    _self->dest_addr = NULL;
    _self->my_welcome = NULL;
    _self->its_welcome = NULL;

    /* input stream */
    _self->data_in_buf = (char *) malloc(BUFSIZE);
    if (_self->data_in_buf == NULL) {
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }
    _self->d_index = 0;
    _self->d_size = 0;

    /* output stream */
    _self->data_out_buf = (char *) malloc(BUFSIZE);
    if (_self->data_out_buf == NULL) {
        free(_self->data_in_buf);
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }
    _self->d_out_index = 0;

    _self->header_buf = malloc(HEADER_BUFSIZE);
    if (_self->header_buf == NULL) {
        free(_self->data_in_buf);
        free(_self->data_out_buf);
        apr_pool_destroy(_self->pool);
        free(_self);
        return NULL;
    }

    /* apr_thread */
    _self->recv_thread = NULL;

    _self->endpoint = jxta_transport_tcp_get_endpoint_service(tp);

    return _self;
}

static void tcp_connection_free(Jxta_object * me)
{
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;

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

static Jxta_status connect_to_from(Jxta_transport_tcp_connection * _self, const char *local_ipaddr)
{
    apr_sockaddr_t *intfaddr;
    apr_status_t status;
    apr_port_t port;

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

    apr_socket_addr_get(&intfaddr, APR_LOCAL, _self->shared_socket);
    apr_sockaddr_ip_get(&_self->ipaddr, _self->intf_addr);
    /* apr_sockaddr_port_get(&port, intfaddr); */
    port = intfaddr->port;
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
                                                                                  apr_socket_t * inc_socket)
{
    Jxta_transport_tcp_connection *_self = tcp_connection_new(tp, FALSE);
    char *protocol_address;
    int len;

    if (_self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create tcp connection\n");
        return NULL;
    }

    _self->shared_socket = inc_socket;

    apr_socket_addr_get(&_self->intf_addr, APR_REMOTE, _self->shared_socket);
    apr_sockaddr_ip_get(&_self->ipaddr, _self->intf_addr);
    /* apr_sockaddr_port_get(&_self->port, _self->intf_addr); */
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
    if (JXTA_SUCCESS == tcp_connection_start_socket(_self)) {
        JXTA_OBJECT_RELEASE(_self->dest_addr);
        _self->dest_addr = welcome_message_get_publicaddr(_self->its_welcome);

        /* 
         * Since _self->ipaddr is in apr_pool, so it must not be freed.
         * That area is freed when apr_pool_destroy called.
         */
        tcp_connection_get_intf_from_jxta_endpoint_address(_self->dest_addr, _self->ipaddr, &_self->port);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "New connection -> %s:%d\n", _self->ipaddr, _self->port);
    } else {
        JXTA_OBJECT_RELEASE(_self);
        _self = NULL;
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
    JString * welcome_str = NULL;
    
    peerid = jxta_transport_tcp_get_peerid(_self->tp);
    public_addr = jxta_transport_tcp_get_public_addr(_self->tp);

    apr_socket_opt_set(_self->shared_socket, APR_SO_KEEPALIVE, 1); /* Keep Alive */
    apr_socket_opt_set(_self->shared_socket, APR_SO_LINGER, LINGER_DELAY); /* Linger Delay */
    apr_socket_opt_set(_self->shared_socket, APR_TCP_NODELAY, 1);  /* disable nagel's */

    _self->my_welcome = welcome_message_new_1(_self->dest_addr, public_addr, peerid, TRUE, 1);
    JXTA_OBJECT_RELEASE(peerid);
    if (_self->my_welcome == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to create welcome message\n");
        return JXTA_FAILED;
    }

    /* send my welcome message */
    welcome_str = welcome_message_get_welcome(_self->my_welcome);
    
    res = tcp_connection_write(_self, jstring_get_string(welcome_str), jstring_length(welcome_str), TRUE);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Local welcome message sent ..%s\n",
                    (caddress = jxta_endpoint_address_to_string(_self->dest_addr)));
    free(caddress);
    JXTA_OBJECT_RELEASE(welcome_str);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error socket write\n");
        return res;
    }

    welcome_str = tcp_connection_read_welcome(_self);
    if (!welcome_str) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error reading welcome message\n");
        return JXTA_FAILED;
    }

    /* recv its welcome message */
    _self->its_welcome = welcome_message_new_2(welcome_str);

    JXTA_OBJECT_RELEASE(welcome_str);

    if (_self->its_welcome == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Remote welcome message is malformed.\n");
        return JXTA_FAILED;
    } else {
        int its_msg_version = welcome_message_get_message_version(_self->its_welcome);
        int my_msg_version = welcome_message_get_message_version(_self->my_welcome);
    
        _self->use_msg_version =  its_msg_version < my_msg_version ? its_msg_version : my_msg_version;
        }

    return JXTA_SUCCESS;
}

static JString * tcp_connection_read_welcome(_jxta_transport_tcp_connection * _self)
{
    JString * result = NULL;
    char *line;
    apr_size_t offset;

    Jxta_boolean saw_CR, saw_CRLF;

    saw_CR = saw_CRLF = FALSE;

    line = (char *) calloc(MAX_WELCOME_MSG_SIZE + 1, sizeof(char));
    if (line == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "line calloc error\n");
        return NULL;
    }
   
    offset = 0;
    while ( (!saw_CRLF) && (offset < MAX_WELCOME_MSG_SIZE) )  {
        apr_size_t read_len = 1;
        if (JXTA_SUCCESS != tcp_connection_read(_self, &line[offset], &read_len)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to read from tcp connection\n");
            free(line);
            return NULL;
        }
        
        if( 0 == read_len ) {
            continue;
        }

        switch (line[offset]) {
            case '\r':
                saw_CR = TRUE;
                break;

            case '\n':
                if (saw_CR) {
                    saw_CRLF = TRUE;
                }
                break;

            default:
                saw_CR = FALSE;
        }

        offset++;
    }

    if (!saw_CRLF) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Welcome message too long.\n");
    } else {
        line[offset] = 0;
        result = jstring_new_2(line);
    }
   
    free(line);

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_start(Jxta_transport_tcp_connection * me)
{
    apr_status_t status;
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;

    JXTA_OBJECT_CHECK_VALID(_self);

    if (CONN_INIT != _self->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Connection [%pp] already started.\n", _self);
        return JXTA_FAILED;
    }

    status = apr_thread_create(&_self->recv_thread, NULL, tcp_connection_thread, _self, _self->pool);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to start thread : %s\n", msg);
        apr_thread_mutex_lock(_self->mutex);
        _self->connection_state = CONN_STOPPED;
        apr_thread_mutex_unlock(_self->mutex);
        return JXTA_FAILED;
    }

    apr_thread_mutex_lock(_self->mutex);
    _self->connection_state = CONN_CONNECTED;
    apr_thread_mutex_unlock(_self->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Connection [%pp] to %s:%d started.\n", _self, _self->ipaddr, _self->port);

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL read_from_tcp_connection(void *me, char *buf, apr_size_t len)
{
    Jxta_status res;
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;

    JXTA_OBJECT_CHECK_VALID(_self);

    res = tcp_connection_read_n(_self, buf, len);
    
    return res;
}

static void *APR_THREAD_FUNC tcp_connection_thread(apr_thread_t * thread, void *me)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;

    JXTA_OBJECT_CHECK_VALID(_self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP incoming message thread [%pp] for [%pp] %s:%d started.\n", 
                    _self->recv_thread, _self, _self->ipaddr, _self->port);

    apr_thread_mutex_lock(_self->mutex);
    _self->connection_state = CONN_CONNECTED;
    apr_thread_mutex_unlock(_self->mutex);

    while (CONN_CONNECTED == _self->connection_state) {
        apr_int64_t msg_size;

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
            if (msg == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                break;
            }
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Reading message [%pp] of %" APR_INT64_T_FMT " bytes.\n", msg,
                            msg_size);

            res = jxta_message_read(msg, APP_MSG, read_from_tcp_connection, _self);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to read message [%pp]\n", msg);
                JXTA_OBJECT_RELEASE(msg);
                break;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Received a new message [%pp] from %s:%d\n", msg, _self->ipaddr,
                            _self->port);

            JXTA_OBJECT_CHECK_VALID(_self->endpoint);
            jxta_endpoint_service_demux(_self->endpoint, msg);
            JXTA_OBJECT_RELEASE(msg);
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP incoming message thread for [%pp] %s:%d stopping : %d \n", 
                    _self, _self->ipaddr, _self->port, res);
   
    if(CONN_CONNECTED == _self->connection_state) {    
        apr_thread_mutex_lock(_self->mutex);
        _self->connection_state = CONN_DISCONNECTED;
        apr_thread_mutex_unlock(_self->mutex);
        }
 
    return NULL;
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_close(Jxta_transport_tcp_connection * me)
{
    apr_status_t status;
    _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;

    JXTA_OBJECT_CHECK_VALID(_self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Closing connection [%pp] to %s:%d\n", _self, _self->ipaddr, _self->port);

    if (CONN_INIT == _self->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Not yet connected\n");
        return JXTA_SUCCESS;
    }

    if (CONN_STOPPED == _self->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Already closed\n");
        return JXTA_SUCCESS;
    }

    apr_thread_mutex_lock(_self->mutex);
    _self->connection_state = CONN_STOPPED;
    apr_thread_mutex_unlock(_self->mutex);

    apr_socket_shutdown(_self->shared_socket, APR_SHUTDOWN_READWRITE);
    apr_socket_close(_self->shared_socket);

    if(_self->recv_thread){
        apr_thread_join(&status, _self->recv_thread);
        if (status != APR_SUCCESS) {
            char msg[256];
            apr_strerror(status, msg, sizeof(msg));
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error result from incoming message thread : %s\n",
                            msg);
        }
    }

    /* don't remove messenger, transport will do that within get_messenger in case connection is closed */
    /*
       res = jxta_transport_tcp_remove_messenger(_self->tp, _self->dest_addr);
       if (res != JXTA_SUCCESS) {
       jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to remove messenger : %d\n", res);
       }
     */
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL msg_wireformat_size(void *arg, const char *buf, apr_size_t len)
{
    apr_int64_t *size = arg;  /* 8 bytes */
    *size += len;
    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL write_to_tcp_connection(void *stream, const char *buf, apr_size_t len)
{
    return tcp_connection_write((Jxta_transport_tcp_connection *) stream, buf, len, FALSE);
}

JXTA_DECLARE(Jxta_status) jxta_transport_tcp_connection_send_message(Jxta_transport_tcp_connection * me, Jxta_message * msg)
{
    Jxta_status res;
   _jxta_transport_tcp_connection *_self = (_jxta_transport_tcp_connection *) me;
    apr_int64_t msg_size;

    JXTA_OBJECT_CHECK_VALID(_self);

    if (CONN_CONNECTED != _self->connection_state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Connection [%pp] closed -- cannot send msg [%pp].\n", _self,
                        msg);
        return JXTA_FAILED;
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_SHARE(msg);

    msg_size = 0;
    res =jxta_message_write_1(msg, APP_MSG, _self->use_msg_version, msg_wireformat_size, &msg_size);
    if ( JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to determine message size.\n");
        JXTA_OBJECT_RELEASE(msg);
        return res;
    }

    apr_thread_mutex_lock(_self->mutex);

    /* write message packet header */
    res = message_packet_header_write(write_to_tcp_connection, (void *) _self, msg_size, FALSE, NULL);
    if ( JXTA_SUCCESS != res) {
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Message [%pp] sent on [%pp]\n", msg, _self);
    }

    apr_thread_mutex_unlock(_self->mutex);

    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status tcp_connection_read(_jxta_transport_tcp_connection * _self, char *buf, apr_size_t * size)
{
    apr_status_t status;
    size_t read = 0, to_read = 0;

    if (buf == NULL || *size == 0)
        return JXTA_FAILED;

    if (_self->d_size > 0) {
        read = ((signed) *size > _self->d_size) ? _self->d_size : *size;
        to_read = *size - read;

        memcpy(buf, &_self->data_in_buf[_self->d_index], read);

        *size = read;
        _self->d_size -= read;
        if (_self->d_size == 0)
            _self->d_index = 0;
        else
            _self->d_index += read;
    } else {
        to_read = *size;
        *size = 0;
    }

    if (to_read > 0) {
        status = apr_socket_recv(_self->shared_socket, &buf[read], &to_read);
        if (APR_SUCCESS != status) {
            return JXTA_FAILED;
        }
        *size += to_read;
    }
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_read_n(Jxta_transport_tcp_connection * _self, char *buf, apr_size_t size)
{
    Jxta_status res;
    int offset;
    apr_size_t bytesread;

    for (bytesread = size, offset = 0; size > 0;) {
        res = tcp_connection_read(_self, buf + offset, &bytesread);
        if (res != JXTA_SUCCESS) {
            return JXTA_FAILED;
        }
        offset += bytesread;
        size -= bytesread;
        bytesread = size;
    }

    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_unread(Jxta_transport_tcp_connection * _self, char *buf, apr_size_t size)
{
    if ((signed) size <= 0)
        return JXTA_FAILED;

    if (_self->d_index >= (signed) size)
        _self->d_index -= size;
    else
        memcpy(&_self->data_in_buf[_self->d_index + _self->d_size], buf, size);

    _self->d_size += size;
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_write(Jxta_transport_tcp_connection * _self, const char *buf, apr_size_t size,
                                        Jxta_boolean zero_copy)
{
    Jxta_status jxta_status = JXTA_SUCCESS;
    apr_status_t status = APR_SUCCESS;
    apr_size_t written = 0;
    apr_size_t left = BUFSIZE - _self->d_out_index;

    if (zero_copy)
        goto Direct_buf;

  /** The buffer won't be filled by this buf */
    if (left > size) {
        memcpy(&_self->data_out_buf[_self->d_out_index], buf, size);
        _self->d_out_index += size;
        size -= size;
    } else {
    /** The buffer will be filled, so flush it ... */
        jxta_status = tcp_connection_flush(_self);
        if (jxta_status != JXTA_SUCCESS)
            return jxta_status;

      Direct_buf:
    /** Start to send remaining buff directly */
        while (size > 0) {
            written = size;
            status = apr_socket_send(_self->shared_socket, buf, &written);
            if (APR_SUCCESS != status)
                return JXTA_FAILED;
            buf += written;
            size -= written;
        }
    }

    return JXTA_SUCCESS;
}

static Jxta_status tcp_connection_flush(Jxta_transport_tcp_connection * _self)
{
    apr_status_t status = APR_SUCCESS;
    char *pt = _self->data_out_buf;
    apr_size_t written = _self->d_out_index;
    apr_size_t total_written = 0;

  /** Start to send the data_out_buf */
    while (written > 0) {
        status = apr_socket_send(_self->shared_socket, pt, &written);
        if (APR_SUCCESS != status) {
      /** Drop the all message ... */
            _self->d_out_index = 0;
            return JXTA_FAILED;
        }
        total_written += written;
        pt += written;
        written = _self->d_out_index - total_written;
    }
  /** The data_out_buf is now empty */
    _self->d_out_index = 0;

    return JXTA_SUCCESS;
}

static void tcp_connection_get_intf_from_jxta_endpoint_address(Jxta_endpoint_address * ea, char *ipaddr, apr_port_t * port)
{
    char *addr, *state;
    JXTA_OBJECT_CHECK_VALID(ea);

    addr = strdup(jxta_endpoint_address_get_protocol_address(ea));
    if (addr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return;
    }

    apr_strtok(addr, ":", &state);

    strcpy(ipaddr, addr);
    *port = (apr_port_t) atoi(state);

    free(addr);
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

/* vim: set ts=4 sw=4 tw=130 et: */
