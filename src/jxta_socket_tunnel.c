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
 * $Id: jxta_socket_tunnel.c,v 1.18 2005/08/03 05:51:19 slowhog Exp $
 */

#include <ctype.h>
#include "apr_network_io.h"
#include "apr_thread_proc.h"
#include "apr_thread_mutex.h"
#include "jpr/jpr_apr_wrapper.h"
#include "jxta_socket_tunnel.h"
#include "jxta_bidipipe.h"
#include "jxta_log.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0                           /* to cheat indent */
}
#endif
#define JXTA_SOCKET_TUNNEL_LOG "SocketTunnel"
#define DEFAULT_TUNNEL_BUFFER_SIZE (8 * 1024)
enum protocols {
    TCP = 0,
    UDP,
    NUMBER_OF_PROTOCOLS
};

enum modes {
    ACCEPTING = 0,
    ESTABLISHING
};

static const char *protocols[NUMBER_OF_PROTOCOLS] = { "tcp", "udp" };
static const int proto_params[NUMBER_OF_PROTOCOLS][3] = {
    {APR_INET, SOCK_STREAM, APR_PROTO_TCP},
    {APR_INET, SOCK_DGRAM, APR_PROTO_UDP}
};

struct Jxta_socket_tunnel {
    Jxta_pipe_adv *pipe_adv;
    Jxta_bidipipe *pipe;
    int protocol;
    apr_socket_t *socket;
    apr_sockaddr_t *sockaddr;
    apr_socket_t *data_socket;
    apr_thread_t *thread;
    Jxta_listener *listener;
    apr_pool_t *pool;
    char *buf;
    size_t buf_size;

    char *o_buf;
    size_t o_buf_size;
    apr_thread_mutex_t *mutex;
    Jxta_boolean stop;
    Jxta_boolean eof;
    enum modes mode;
};

static Jxta_status decode_addr_spec(Jxta_socket_tunnel * self, const char *addr_spec)
{
    Jxta_status rv;
    const char *pb, *pe;
    char *p;
    int len;
    int i;

    pb = addr_spec;
    pe = addr_spec + strlen(addr_spec);

    while (pb < pe && isspace(*pb))
        ++pb;
    if (pe == pb) {
        return JXTA_INVALID_ARGUMENT;
    }

    --pe;
    while (pe > pb && isspace(*pe))
        --pe;

    len = pe - pb + 1;
    /* a://b[:c] is at least 5 characters */
    if (len < 5) {
        return JXTA_INVALID_ARGUMENT;
    }

    p = (char *) malloc(sizeof(char) * (len + 1));
    strncpy(p, pb, len);
    p[len] = '\0';
    pb = p;

    while ('\0' != *p && ':' != *p)
        ++p;
    if ('\0' == *p) {
        free((void *) pb);
        return JXTA_INVALID_ARGUMENT;
    }
    p[0] = '\0';

    if ('\0' == p[1] || '/' != p[1] || '/' != p[2]) {
        free((void *) pb);
        return JXTA_INVALID_ARGUMENT;
    }
    p += 3;
    pe = p;

    for (i = 0; i < NUMBER_OF_PROTOCOLS; i++) {
        if (0 == strcmp(protocols[i], pb)) {
            break;
        }
    }
    if (NUMBER_OF_PROTOCOLS <= i) {
        free((void *) pb);
        return JXTA_INVALID_ARGUMENT;
    }
    self->protocol = i;

    while ('\0' != *p && ':' != *p)
        ++p;

    if ('\0' == *p) {   /* port number is not specified, use any available port */
        len = 0;
    } else {
        *p = '\0';
        p++;
        len = strtol(p, NULL, 10);
    }

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_DEBUG, "Tunnel protocol %s at address %s with port %ld(%s)\n",
                    pb, pe, len, *p ? p : "not specified");

    rv = apr_sockaddr_info_get(&self->sockaddr, pe, proto_params[i][0], len, 0, self->pool);
    free((void *) pb);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Socket address create failed with error %ld\n", rv);
        return rv;
    }
#if CHECK_APR_VERSION(1, 0, 0)
    rv = apr_socket_create(&self->socket, proto_params[i][0], proto_params[i][1], proto_params[i][2], self->pool);
#else
    rv = apr_socket_create_ex(&self->socket, proto_params[i][0], proto_params[i][1], proto_params[i][2], self->pool);
#endif

    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING,
                        "Socket create failed with error %ld. family/type/protocol:%d/%d/%d\n", rv, proto_params[i][0],
                        proto_params[i][1], proto_params[i][2]);
        return rv;
    }

    return rv;
}

static Jxta_status bind_socket(apr_socket_t * s, apr_sockaddr_t * addr)
{
    Jxta_status rv;
    char *ip;
    apr_sockaddr_t *sa;

    apr_sockaddr_ip_get(&ip, addr);
    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "Binding to socket %s:%ld\n", ip, addr->port);
    rv = apr_socket_bind(s, addr);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Bind to socket failed with error %ld\n", rv);
        return rv;
    }

    if (0 == addr->port) {
        rv = apr_socket_addr_get(&sa, APR_LOCAL, s);
        if (APR_SUCCESS == rv) {
            addr->port = sa->port;
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "Bind socket to port %ld\n", addr->port);
        }
    }
    return rv;
}

static void listener_func(Jxta_object * obj, void *arg)
{
    Jxta_socket_tunnel *self = (Jxta_socket_tunnel *) arg;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *data;
    size_t data_size;
    size_t size;
    size_t read_size;
    size_t size_to_send;
    Jxta_status rv;

    if (NULL == obj) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "Tunnel closed by remote peer\n");
        apr_thread_mutex_lock(self->mutex);
        self->eof = TRUE;
        if (NULL != self->data_socket) {
            rv = apr_socket_shutdown(self->data_socket, APR_SHUTDOWN_READWRITE);
        }
        /* shutdown socket if UDP tunnel */
        if (SOCK_DGRAM == proto_params[self->protocol][1]) {
            rv = apr_socket_shutdown(self->socket, APR_SHUTDOWN_READWRITE);
        }
        apr_thread_mutex_unlock(self->mutex);
        return;
    }

    rv = jxta_message_get_element_1(msg, "DATA", &el);
    if (JXTA_SUCCESS != rv || NULL == el) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Invalid message discard\n");
        return;
    }

    data = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);

    if (NULL == data) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to retrieve data from message\n");
        return;
    }

    data_size = jxta_bytevector_size(data);
    if (0 == data_size) {
        return;
    }

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "%ld bytes of data received, forward to socket ...\n",
                    data_size);

    if (self->o_buf_size < data_size) {
        if (NULL != self->o_buf)
            free(self->o_buf);
        self->o_buf = (char *) malloc(sizeof(char) * data_size);
        if (NULL == self->o_buf) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Out of memory\n");
            self->o_buf_size = 0;
            JXTA_OBJECT_RELEASE(data);
            return;
        } else {
            self->o_buf_size = data_size;
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_DEBUG, "Forward buffer size grow to %ld\n", self->o_buf_size);
        }
    }

    /* Keep work done, as we expect may need to restrict the max buffer size in the future */
    read_size = 0;
    while (read_size < data_size) {
        size = self->o_buf_size;
        rv = jxta_bytevector_get_bytes_at_1(data, (unsigned char *) self->o_buf, read_size, &size);
        read_size += size;
        size_to_send = size;

        switch (proto_params[self->protocol][1]) {
        case SOCK_STREAM:
            rv = apr_socket_send(self->data_socket, self->o_buf, &size);
            break;
        case SOCK_DGRAM:
            rv = apr_socket_sendto(self->socket, self->sockaddr, 0, self->o_buf, &size);
            break;
        }

        if (APR_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to send %ld bytes of data with error %ld\n",
                            size_to_send, rv);
        } else {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "%ld bytes of data received successfully\n", size);
        }
    }
    JXTA_OBJECT_RELEASE(data);

    return;
}

static Jxta_status send_to_pipe(Jxta_bidipipe * pipe, const char *buf, size_t size)
{
    Jxta_message *msg;
    Jxta_message_element *el;
    Jxta_status rv;

    if (0 == size) {
        return JXTA_SUCCESS;
    }

    if (NULL == pipe || NULL == buf || 0 == size) {
        return JXTA_INVALID_ARGUMENT;
    }

    msg = jxta_message_new();
    if (NULL == msg) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create message\n");
        return JXTA_FAILED;
    }

    el = jxta_message_element_new_1("DATA", NULL, buf, size, NULL);
    if (NULL == msg) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create message element\n");
        return JXTA_FAILED;
    }

    jxta_message_add_element(msg, el);
    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Sending message ...\n");
    rv = jxta_bidipipe_send(pipe, msg);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_ERROR, "Sending message failed with error %ld\n", rv);
    }

    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(msg);

    return rv;
}

static Jxta_status pump_datagram_socket(Jxta_socket_tunnel * self)
{
    Jxta_status rv = JXTA_SUCCESS;
    apr_sockaddr_t from;
    apr_size_t size;

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Start pumping datagrams\n");
    while (!self->eof) {
        size = self->buf_size;
        from.salen = sizeof(from.sa);
        rv = apr_socket_recvfrom(&from, self->socket, 0, self->buf, &size);
        if (APR_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Recvfrom socket with error %ld\n", rv);
            /* FIXME: should we wait to retry or quit? */
            continue;
        }

        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "%ld bytes was received from socket\n", size);
        rv = send_to_pipe(self->pipe, self->buf, size);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to send data to pipe with error %ld\n", rv);
            /* FIXME: should we wait to retry or quit? */
        }
    }
    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Stop pumping datagrams\n");
    return rv;
}

static Jxta_status pump_stream_socket(Jxta_socket_tunnel * self)
{
    Jxta_status rv = JXTA_SUCCESS;
    apr_size_t size;
    apr_status_t recv_status = APR_SUCCESS;

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Start pumping stream data\n");
    while (!self->eof && recv_status != APR_EOF) {
        size = self->buf_size;
        recv_status = apr_socket_recv(self->data_socket, self->buf, &size);
        if (APR_EOF == recv_status) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "Socket close detected\n");
        } else if (APR_SUCCESS != recv_status) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Receiving from socket with error %ld\n",
                            recv_status);
            /* FIXME: should we wait to retry or quit? */
        }
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "%ld bytes was received from socket\n", size);
        if (0 != size) {
            rv = send_to_pipe(self->pipe, self->buf, size);
            if (JXTA_SUCCESS != rv) {
                jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to send data to pipe with error %ld\n", rv);
                /* FIXME: should we wait to retry or quit? */
            }
        }
    }
    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Stop pumping stream data\n");
    return rv;
}

static void *APR_THREAD_FUNC pipe_accept_func(apr_thread_t * thread, void *arg)
{
    Jxta_status rv = JXTA_SUCCESS;
    Jxta_socket_tunnel *self = (Jxta_socket_tunnel *) arg;

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "pipe accept thread started\n");
    jxta_listener_start(self->listener);
    while (!self->stop) {
        self->eof = FALSE;
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Wait for bidipipe connection ...\n");
        rv = jxta_bidipipe_accept(self->pipe, self->pipe_adv, self->listener);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING,
                            "Fail to accept connection from bidipipe with error %ld\n", rv);
            continue;
        }

        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "bidipipe connection request accepted\n");
        /* pipe closed, must because teardown tunneling */
        if (JXTA_BIDIPIPE_CONNECTED != jxta_bidipipe_get_state(self->pipe)) {
            continue;
        }

        switch (proto_params[self->protocol][1]) {
        case SOCK_STREAM:
#if CHECK_APR_VERSION(1, 0, 0)
            rv = apr_socket_create(&self->data_socket, proto_params[self->protocol][0], proto_params[self->protocol][1],
                                   proto_params[self->protocol][2], self->pool);
#else
            rv = apr_socket_create_ex(&self->data_socket, proto_params[self->protocol][0], proto_params[self->protocol][1],
                                      proto_params[self->protocol][2], self->pool);
#endif
            if (APR_SUCCESS != rv) {
                jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING,
                                "Data socket create failed with error %ld. family/type/protocol:%d/%d/%d\n", rv,
                                proto_params[self->protocol][0], proto_params[self->protocol][1],
                                proto_params[self->protocol][2]);
                break;
            }
            rv = apr_socket_connect(self->data_socket, self->sockaddr);
            if (APR_SUCCESS == rv) {
                rv = pump_stream_socket(self);
            } else {
                jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to connect server with  error %ld\n", rv);
            }
            jxta_bidipipe_close(self->pipe);
            apr_socket_close(self->data_socket);
            self->data_socket = NULL;
            break;
        case SOCK_DGRAM:
            rv = pump_datagram_socket(self);
            break;
        }
    }
    jxta_listener_stop(self->listener);

    apr_thread_exit(thread, rv);
    return NULL;
}

static void *APR_THREAD_FUNC socket_dgram_func(apr_thread_t * thread, void *arg)
{
    Jxta_socket_tunnel *self = (Jxta_socket_tunnel *) arg;
    Jxta_status rv = JXTA_SUCCESS;

    while (!self->stop) {
        self->eof = FALSE;
        apr_thread_mutex_lock(self->mutex);
        jxta_listener_start(self->listener);
        rv = jxta_bidipipe_connect(self->pipe, self->pipe_adv, self->listener, 180L * 1000L * 1000L);
        if (JXTA_SUCCESS != rv) {
            jxta_listener_stop(self->listener);
            apr_thread_mutex_unlock(self->mutex);
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to connect remote pipe with error %ld\n", rv);
            apr_thread_exit(thread, rv);
            return NULL;
        }

        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "establishing mode, not reading from UDP\n");
        apr_thread_mutex_unlock(self->mutex);
        break;

        apr_thread_mutex_unlock(self->mutex);
        pump_datagram_socket(self);
        apr_thread_mutex_lock(self->mutex);

        rv = jxta_bidipipe_close(self->pipe);
        jxta_listener_stop(self->listener);
        apr_thread_mutex_unlock(self->mutex);
    }
    apr_thread_exit(thread, rv);
    return NULL;
}

static void *APR_THREAD_FUNC socket_accept_func(apr_thread_t * thread, void *arg)
{
    Jxta_socket_tunnel *self = (Jxta_socket_tunnel *) arg;
    Jxta_status rv;

    rv = apr_socket_listen(self->socket, 1);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to listen socket with error %ld\n", rv);
        apr_thread_exit(thread, rv);
        return NULL;
    }

    apr_thread_mutex_lock(self->mutex);
    while (!self->stop) {
        self->eof = FALSE;
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Accepting socket connection ...\n");
        rv = apr_socket_accept(&self->data_socket, self->socket, self->pool);
        if (APR_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to accept socket connection with error %ld\n",
                            rv);
            apr_thread_exit(thread, rv);
            return NULL;
        }

        apr_thread_mutex_lock(self->mutex);
        if (self->stop) {
            break;
        }
        jxta_listener_start(self->listener);
        apr_thread_mutex_unlock(self->mutex);

        rv = jxta_bidipipe_connect(self->pipe, self->pipe_adv, self->listener, 180L * 1000L * 1000L);
        if (JXTA_SUCCESS != rv) {
            jxta_listener_stop(self->listener);
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to connect remote pipe with error %ld\n", rv);
            apr_thread_mutex_lock(self->mutex);
            continue;
        }

        pump_stream_socket(self);

        apr_thread_mutex_lock(self->mutex);
        rv = apr_socket_close(self->data_socket);
        self->data_socket = NULL;
        rv = jxta_bidipipe_close(self->pipe);
        jxta_listener_stop(self->listener);
    }

    apr_thread_mutex_unlock(self->mutex);
    apr_thread_exit(thread, rv);
    return NULL;
}

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_create(Jxta_PG * group, const char *addr_spec, Jxta_socket_tunnel ** newobj)
{
    Jxta_status rv;
    Jxta_socket_tunnel *self;

    self = (Jxta_socket_tunnel *) calloc(1, sizeof(Jxta_socket_tunnel));
    if (NULL == self) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Out of memory\n");
        return JXTA_NOMEM;
    }

    self->buf_size = DEFAULT_TUNNEL_BUFFER_SIZE;
    self->buf = (char *) malloc(self->buf_size * sizeof(char));
    if (NULL == self->buf) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Out of memory\n");
        free(self);
        return JXTA_NOMEM;
    }

    rv = apr_pool_create(&self->pool, NULL);
    if (rv != APR_SUCCESS) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create pool\n");
        free(self);
        return rv;
    }

    rv = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create mutex\n");
        free(self);
        return rv;
    }

    rv = decode_addr_spec(self, addr_spec);
    if (rv != JXTA_SUCCESS) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create socket\n");
        free(self);
        return rv;
    }

    self->pipe = jxta_bidipipe_new(group);
    if (NULL == self->pipe) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to create bidipipe\n");
        free(self);
        return JXTA_FAILED;
    }

    *newobj = self;
    return JXTA_SUCCESS;
}

Jxta_status jxta_socket_tunnel_delete(Jxta_socket_tunnel * self)
{
    Jxta_status rv = JXTA_SUCCESS;

    if (NULL != self->listener) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_INFO, "Trying to release busy tunnel, teardown ...\n", rv);
        rv = jxta_socket_tunnel_teardown(self);
    }

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Deleting jxta_socket_tunnel ...\n");
    apr_thread_mutex_lock(self->mutex);
    rv = jxta_bidipipe_delete(self->pipe);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to delete bidipipe with error %ld\n", rv);
        return rv;
    }

    rv = apr_socket_close(self->socket);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to close socket with error %ld\n", rv);
        return rv;
    }
    self->socket = NULL;

    apr_thread_mutex_unlock(self->mutex);
    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Failed to detroy pool with error %ld\n", rv);
        return rv;
    }

    free(self);
    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "jxta_socket_tunnel had been deleted\n");
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_establish(Jxta_socket_tunnel * self, Jxta_pipe_adv * remote_adv)
{
    Jxta_status rv;

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->listener) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Try to use a busy tunnel\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_BUSY;
    }

    self->mode = ESTABLISHING;

    self->listener = jxta_listener_new(listener_func, self, 1, 30);
    if (NULL == self->listener) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create jxta listener\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_FAILED;
    }

    self->pipe_adv = remote_adv;
    JXTA_OBJECT_SHARE(self->pipe_adv);

    self->stop = FALSE;

    switch (proto_params[self->protocol][1]) {
    case SOCK_STREAM:
        rv = bind_socket(self->socket, self->sockaddr);
        if (APR_SUCCESS != rv) {
            break;
        }
        rv = apr_thread_create(&self->thread, NULL, socket_accept_func, self, self->pool);
        if (APR_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create socket thread with error %ld\n", rv);
        }
        break;
    case SOCK_DGRAM:
        if (0 == self->sockaddr->port) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_ERROR, "UDP tunnel must assign a port number to send to\n");
            rv = JXTA_INVALID_ARGUMENT;
            break;
        }
        rv = apr_thread_create(&self->thread, NULL, socket_dgram_func, self, self->pool);
        if (APR_SUCCESS != rv) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create socket thread with error %ld\n", rv);
        }
        break;
    default:
        rv = JXTA_INVALID_ARGUMENT;
        break;
    }

    if (APR_SUCCESS != rv) {
        JXTA_OBJECT_RELEASE(self->listener);
        self->listener = NULL;
    }

    apr_thread_mutex_unlock(self->mutex);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_accept(Jxta_socket_tunnel * self, Jxta_pipe_adv * local_adv)
{
    Jxta_status rv = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->listener) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Try to use a busy tunnel\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_BUSY;
    }

    switch (proto_params[self->protocol][1]) {
    case SOCK_STREAM:
        if (0 == self->sockaddr->port) {
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_ERROR, "TCP tunnel must assign a port number to connect to\n");
            rv = JXTA_INVALID_ARGUMENT;
        }
        break;
    case SOCK_DGRAM:
        rv = bind_socket(self->socket, self->sockaddr);
        break;
    default:
        rv = JXTA_INVALID_ARGUMENT;
        break;
    }

    if (JXTA_SUCCESS != rv) {
        apr_thread_mutex_unlock(self->mutex);
        return rv;
    }

    self->mode = ACCEPTING;
    self->listener = jxta_listener_new(listener_func, self, 1, 30);
    if (NULL == self->listener) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create jxta listener\n");
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_FAILED;
    }

    self->pipe_adv = local_adv;
    JXTA_OBJECT_SHARE(self->pipe_adv);

    self->stop = FALSE;

    rv = apr_thread_create(&self->thread, NULL, pipe_accept_func, self, self->pool);
    if (APR_SUCCESS != rv) {
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_WARNING, "Fail to create pipe thread with error %ld\n", rv);
        JXTA_OBJECT_RELEASE(self->listener);
        self->listener = NULL;
    }

    apr_thread_mutex_unlock(self->mutex);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_socket_tunnel_teardown(Jxta_socket_tunnel * self)
{
    Jxta_status rv;
    apr_status_t ts;

    apr_thread_mutex_lock(self->mutex);
    if (NULL == self->listener) {
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Tunnel had been closed or haven't been established\n");
        return JXTA_SUCCESS;
    }

    self->eof = TRUE;
    self->stop = TRUE;
    JXTA_OBJECT_RELEASE(self->pipe_adv);
    self->pipe_adv = NULL;

    jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Tearing down ...\n");
    switch (self->mode) {
    case ACCEPTING:
        rv = jxta_bidipipe_close(self->pipe);
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Close pipe with result %ld\n", rv);
        /* shutdown socket if UDP tunnel */
        if (SOCK_DGRAM == proto_params[self->protocol][1]) {
            rv = apr_socket_shutdown(self->socket, APR_SHUTDOWN_READWRITE);
        }
        break;
    case ESTABLISHING:
        if (SOCK_DGRAM == proto_params[self->protocol][1]) {
            rv = jxta_bidipipe_close(self->pipe);
            jxta_listener_stop(self->listener);
        }
        if (NULL != self->data_socket) {
            rv = apr_socket_shutdown(self->data_socket, APR_SHUTDOWN_READWRITE);
            jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Shutdown data socket with result %ld\n", rv);
        }
        rv = apr_socket_shutdown(self->socket, APR_SHUTDOWN_READWRITE);
        jxta_log_append(JXTA_SOCKET_TUNNEL_LOG, JXTA_LOG_LEVEL_TRACE, "Shutdown socket with result %ld\n", rv);
        break;
    default:
        rv = JXTA_INVALID_ARGUMENT;
        break;
    }
    apr_thread_mutex_unlock(self->mutex);

    if (NULL != self->thread) {
        rv = apr_thread_join(&ts, self->thread);
    }

    apr_thread_mutex_lock(self->mutex);
    JXTA_OBJECT_RELEASE(self->listener);
    self->listener = NULL;
    apr_thread_mutex_unlock(self->mutex);

    return rv;
}

JXTA_DECLARE(Jxta_boolean) jxta_socket_tunnel_is_established(Jxta_socket_tunnel * self)
{
    return (NULL != self->listener);
}

JXTA_DECLARE(char *) jxta_socket_tunnel_addr_get(Jxta_socket_tunnel * self)
{
    size_t len;
    char *ip;
    char *buf;

    apr_sockaddr_ip_get(&ip, self->sockaddr);
    len = 36 + strlen(protocols[self->protocol]) + strlen(ip);  /* 4 'proto://addr:port' + 32 for port */
    buf = (char *) calloc(len + 1, sizeof(char));
    snprintf(buf, len, "%s://%s:%u", protocols[self->protocol], ip, self->sockaddr->port);
    return buf;
}

#if 0   /* to cheat indent */
{
#endif
#ifdef __cplusplus
}   /* extern "C" */
#endif
/* vim: set sw=4 ts=4 et tw=130: */
