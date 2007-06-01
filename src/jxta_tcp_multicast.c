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
 * $Id: jxta_tcp_multicast.c,v 1.14.2.3 2005/05/25 01:36:14 slowhog Exp $
 */

#define BUFSIZE		8192

#include <apr.h>
#include <apr_network_io.h>
#include <apr_thread_proc.h>

#include "jpr/jpr_types.h"
#include "jpr/jpr_thread.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_tcp_multicast.h"
#include "jxta_endpoint_address.h"
#include "jxta_tcp_message_packet_header.h"
#include "jxta_log.h"

#ifdef WIN32
#include <winsock.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

typedef struct _stream {
    char *data_buf;
    int d_index;
    apr_size_t d_len;
    apr_size_t d_size;
} STREAM;

struct _tcp_multicast {
    JXTA_OBJECT_HANDLE;
    Jxta_transport_tcp *tp;
    char *multicast_ipaddr;
    apr_port_t multicast_port;
    int multicast_packet_size;
    Jxta_boolean run;
    char *received_src_addr;
    Jxta_boolean allow_multicast;

    apr_thread_t *thread;
    apr_thread_mutex_t *mutex;

    apr_socket_t *recv_sock;
    apr_sockaddr_t *recv_intf;
    apr_socket_t *send_sock;
    apr_sockaddr_t *send_intf;

    apr_pool_t *pool;

    STREAM *input_stream;
    STREAM *output_stream;
    STREAM *buf_stream;

    Jxta_endpoint_service *endpoint;
};

static const char *__log_cat = "TCP_MULTICAST";

static int apr_socketdes_get(apr_socket_t * sock);

static STREAM *stream_new(apr_size_t size);
static void stream_init(STREAM * stream);
static void stream_free(STREAM * stream);
static void stream_print(STREAM * stream);

static void tcp_multicast_free(Jxta_object * arg);

static Jxta_status read_from_tcp_multicast_stream(void *stream, char *buf, apr_size_t len);
static void *APR_THREAD_FUNC tcp_multicast_body(apr_thread_t * t, void *arg);
static void tcp_multicast_process(TcpMulticast * tm, STREAM * stream);

static Jxta_status msg_wireformat_size(void *arg, const char *buf, apr_size_t len);
static Jxta_status write_to_tcp_multicast_stream(void *stream, const char *buf, apr_size_t len);

static Jxta_status tcp_multicast_read(TcpMulticast * tm, char *buf, apr_size_t * size);
/*
static Jxta_status tcp_multicast_unread(TcpMulticast *tm, char *buf, apr_size_t size);
*/
static Jxta_status tcp_multicast_read_stream_n(STREAM * stream, char *buf, apr_size_t size);
static Jxta_status tcp_multicast_write_stream(STREAM * stream, const char *buf, apr_size_t size);
static Jxta_status tcp_multicast_write(TcpMulticast * tm, const char *buf, apr_size_t size);

static int apr_socketdes_get(apr_socket_t * sock)
{
    struct __dummy {
        apr_pool_t *dummypool;
        int sockfd;
    } *tmp;

    tmp = (struct __dummy *) sock;
    return tmp->sockfd;
}

static void stream_init(STREAM * stream)
{
    memset(stream->data_buf, 0, stream->d_size);
    stream->d_index = 0;
    stream->d_len = 0;
}

static STREAM *stream_new(apr_size_t size)
{
    STREAM *stream;

    stream = (STREAM *) malloc(sizeof(STREAM));
    if (stream != NULL) {
        stream->data_buf = (char *) malloc(size);
        stream->d_size = size;
        if (stream->data_buf == NULL)
            return NULL;
        stream_init(stream);
    }

    return stream;
}

static void stream_free(STREAM * stream)
{
    if (stream->data_buf)
        free(stream->data_buf);
    free(stream);
}

TcpMulticast *tcp_multicast_new(Jxta_transport_tcp * tp, char *ipaddr, apr_port_t port, int packet_size)
{
    TcpMulticast *self;
    apr_status_t status;

    /* create object */
    self = (TcpMulticast *) malloc(sizeof(TcpMulticast));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed malloc()\n");
        return NULL;
    }

    /* initialize it */
    memset(self, 0, sizeof(TcpMulticast));
    JXTA_OBJECT_INIT(self, tcp_multicast_free, NULL);

    /* setting */
    self->tp = tp;
    self->multicast_port = port;
    self->multicast_packet_size = packet_size;
    self->run = FALSE;
    self->received_src_addr = NULL;
    self->allow_multicast = jxta_transport_tcp_get_allow_multicast(tp);

    /* stream */
    self->input_stream = stream_new(self->multicast_packet_size);
    self->output_stream = stream_new(self->multicast_packet_size);
    self->buf_stream = stream_new(self->multicast_packet_size);

    /* apr setting */
    status = apr_pool_create(&self->pool, NULL);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        /* Free the memory that has been already allocated */
        free(self);
        return NULL;
    }
    status = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    /* socket info */
    status = apr_sockaddr_info_get(&self->recv_intf, APR_ANYADDR, APR_INET, self->multicast_port, 0, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        apr_pool_destroy(self->pool);
        free(self);
    }

    status = apr_sockaddr_info_get(&self->send_intf, ipaddr, APR_INET, self->multicast_port, 0, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        apr_pool_destroy(self->pool);
        free(self);
    }

    self->multicast_ipaddr = strdup(ipaddr);
    self->thread = NULL;
    self->recv_sock = NULL;
    self->send_sock = NULL;
    self->endpoint = jxta_transport_tcp_get_endpoint_service(tp);

    return self;
}

static void tcp_multicast_free(Jxta_object * obj)
{
    TcpMulticast *self = (TcpMulticast *) obj;

    JXTA_OBJECT_CHECK_VALID(self);

    if (self == NULL)
        return;

    tcp_multicast_stop(self);

    if (self->multicast_ipaddr != NULL)
        free(self->multicast_ipaddr);

    stream_free(self->input_stream);
    stream_free(self->output_stream);
    stream_free(self->buf_stream);
    
    JXTA_OBJECT_RELEASE(self->endpoint);

    apr_pool_destroy(self->pool);

    free(self);
}

Jxta_boolean tcp_multicast_start(TcpMulticast * tm)
{
    TcpMulticast *self = tm;
    apr_status_t status;

    struct ip_mreq mreq;
    struct in_addr tmp;
    int sockfd;
    /*
       int loopback;
     */

    JXTA_OBJECT_CHECK_VALID(self);

    self->run = FALSE;

    /* sending socket */
    status = apr_socket_create(&self->send_sock, APR_INET, SOCK_DGRAM, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);

        return FALSE;
    }

    /* receiving socket */
    status = apr_socket_create(&self->recv_sock, APR_INET, SOCK_DGRAM, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
    }

    /* REUSEADDR */
    status = apr_socket_opt_set(self->recv_sock, APR_SO_REUSEADDR, 1);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        return FALSE;
    }

    status = apr_socket_bind(self->recv_sock, self->recv_intf);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        return FALSE;
    }

    /* multicast setting */
    tmp.s_addr = apr_inet_addr(self->multicast_ipaddr);
    mreq.imr_multiaddr = tmp;
    mreq.imr_interface.s_addr = htonl(apr_inet_addr(APR_ANYADDR));

    /* It is not apr function */
    sockfd = apr_socketdes_get(self->recv_sock);
    /* ADD MEMBER; use raw function */
    setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
    /* No Loopback */
    /*
       sockfd = apr_socketdes_get(self->send_sock);
       loopback = 0;
       setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback));
     */
    /* multicast setting done */


    /* thread */
    status = apr_thread_create(&self->thread, NULL, tcp_multicast_body, (void *) self, self->pool);

    if (!APR_STATUS_IS_SUCCESS(status)) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        self->run = FALSE;
        return FALSE;
    }

    return TRUE;
}

void tcp_multicast_stop(TcpMulticast * tm)
{
    TcpMulticast *self = tm;
    apr_status_t status;

    self->run = FALSE;

    if (self->send_sock != NULL) {
        apr_socket_shutdown(self->send_sock, APR_SHUTDOWN_READWRITE);
        apr_socket_close(self->send_sock);
        self->send_sock = NULL;
    }

    if (self->recv_sock != NULL) {
        apr_socket_shutdown(self->recv_sock, APR_SHUTDOWN_READWRITE);
        apr_socket_close(self->recv_sock);
        self->recv_sock = NULL;
    }

    if (self->thread != NULL) {
        apr_thread_join(&status, self->thread);
        self->thread = NULL;
    }
}

static Jxta_status read_from_tcp_multicast_stream(void *stream, char *buf, apr_size_t len)
{
    return tcp_multicast_read_stream_n((STREAM *) stream, buf, len);
}

static void *APR_THREAD_FUNC tcp_multicast_body(apr_thread_t * t, void *arg)
{
    TcpMulticast *self = (TcpMulticast *) arg;
    STREAM *stream = self->input_stream;

    if (self->allow_multicast == FALSE) {
        apr_thread_exit(t, APR_SUCCESS);
        return NULL;
    }

    self->run = TRUE;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "tcp multicast started\n");

    while (self->run) {
        stream_init(stream);
        stream->d_len = stream->d_size;
        tcp_multicast_read(self, stream->data_buf, &stream->d_len);

        /*stream_print(stream); */

        tcp_multicast_process(self, stream);
    }
    apr_thread_exit(t, APR_SUCCESS);
    return NULL;
}

static void tcp_multicast_process(TcpMulticast * tm, STREAM * stream)
{
    TcpMulticast *self = tm;
    Jxta_message *msg;
    JXTA_LONG_LONG msg_size;
    Jxta_status res;


    if (self->allow_multicast == FALSE || self->run == FALSE)
        return;

    if (stream->data_buf[0] != 'J' || stream->data_buf[1] != 'X' || stream->data_buf[2] != 'T' || stream->data_buf[3] != 'A') {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Discard damaged multicast\n");
        return;
    }
    stream->d_index += 4;
    stream->d_len -= 4;

    res = message_packet_header_read(read_from_tcp_multicast_stream, (void *) stream, &msg_size, TRUE, &self->received_src_addr);

    /* We do not have anything with self->received_src_addr */
    /* FIXME: slowhog: what does this mean? do we want to have it but failed because bug of
       message_packet_header_read prototype before? Free the addr for now*/
    free(self->received_src_addr);
    self->received_src_addr = NULL;

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to read message packet header\n");
        return;
    }

    if (msg_size > 0) {
        msg = jxta_message_new();
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received new message\n");

        res = jxta_message_read(msg, APP_MSG, read_from_tcp_multicast_stream, stream);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to read message\n");
        }

        JXTA_OBJECT_CHECK_VALID(self->endpoint);
        jxta_endpoint_service_demux(self->endpoint, msg);
        JXTA_OBJECT_RELEASE(msg);
    }
}

static Jxta_status msg_wireformat_size(void *arg, const char *buf, apr_size_t len)
{
    JXTA_LONG_LONG *size = (JXTA_LONG_LONG *) arg;      /* 8 bytes */
    *size += len;
    return JXTA_SUCCESS;
}

static Jxta_status write_to_tcp_multicast_stream(void *stream, const char *buf, apr_size_t len)
{
    return tcp_multicast_write_stream((STREAM *) stream, buf, len);
}

Jxta_status tcp_multicast_propagate(TcpMulticast * tm, Jxta_message * msg, const char *service_name, const char *service_params)
{
    TcpMulticast *self = tm;
    Jxta_endpoint_address *m_addr;
    char *src_addr, *dest_addr;
    STREAM *stream = self->output_stream;
    apr_size_t packet_header_size = 0;
    JXTA_LONG_LONG msg_size = (JXTA_LONG_LONG) 0;
    Jxta_status res;
    char *tmp;

    JXTA_OBJECT_CHECK_VALID(self);

    if (self->allow_multicast == FALSE || self->run == FALSE) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Not connected\n");
        return JXTA_FAILED;
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_SHARE(msg);

    apr_thread_mutex_lock(self->mutex);

    stream_init(stream);

    dest_addr = (char *) malloc(strlen(self->multicast_ipaddr) + 20);
    sprintf(dest_addr, "%s:%d", self->multicast_ipaddr, self->multicast_port);
    /* set destination */
    m_addr = jxta_endpoint_address_new2("tcp", dest_addr, service_name, service_params);
    jxta_message_set_destination(msg, m_addr);
    JXTA_OBJECT_RELEASE(m_addr);
    free(dest_addr);

    /* welcome header */

    tcp_multicast_write_stream(stream, "J", 1);
    tcp_multicast_write_stream(stream, "X", 1);
    tcp_multicast_write_stream(stream, "T", 1);
    tcp_multicast_write_stream(stream, "A", 1);

    /* message packet header */
    jxta_message_write(msg, APP_MSG, msg_wireformat_size, &msg_size);

    src_addr = (char *) malloc(128);
    sprintf(src_addr, "tcp://%s:%d", jxta_transport_tcp_local_ipaddr_cstr(self->tp), jxta_transport_tcp_get_local_port(self->tp));

    message_packet_header_write(msg_wireformat_size, (void *) &packet_header_size, msg_size, TRUE, src_addr);
    message_packet_header_write(write_to_tcp_multicast_stream, (void *) stream, msg_size, TRUE, src_addr);

    free(src_addr);
    /* message body */
    res = jxta_message_write(msg, APP_MSG, write_to_tcp_multicast_stream, stream);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed to propagate message\n");
        apr_thread_mutex_unlock(self->mutex);
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_FAILED;
    }

    res = tcp_multicast_write(self, stream->data_buf, 4 + packet_header_size + msg_size);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Failed sendto()\n");
        apr_thread_mutex_unlock(self->mutex);
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_FAILED;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "message propagated\n");
    apr_thread_mutex_unlock(self->mutex);
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status tcp_multicast_read(TcpMulticast * tm, char *buf, apr_size_t * size)
{
    apr_status_t status;

    if (buf == NULL)
        return JXTA_FAILED;

    status = apr_socket_recvfrom(tm->recv_intf, tm->recv_sock, 0, buf, size);
    if (!APR_STATUS_IS_SUCCESS(status))
        return JXTA_FAILED;
    return JXTA_SUCCESS;
}

static Jxta_status tcp_multicast_read_stream_n(STREAM * stream, char *buf, apr_size_t size)
{
    if (stream->d_len < size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Too big request: len=%d, size=%d\n", stream->d_len, size);
        return JXTA_FAILED;
    }

    memcpy(buf, &stream->data_buf[stream->d_index], size);
    stream->d_index += size;
    stream->d_len -= size;

    return JXTA_SUCCESS;
}

static Jxta_status tcp_multicast_write_stream(STREAM * stream, const char *buf, apr_size_t size)
{
    if (stream->d_index + size >= stream->d_size) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Too big message\n");
        return JXTA_FAILED;
    }

    memcpy(&stream->data_buf[stream->d_index], buf, size);
    stream->d_index += size;
    stream->d_len += size;

    return JXTA_SUCCESS;
}

static Jxta_status tcp_multicast_write(TcpMulticast * tm, const char *buf, apr_size_t size)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t written = size;

    while (size > 0) {
        status = apr_socket_sendto(tm->send_sock, tm->send_intf, 0, buf, &written);
        if (!APR_STATUS_IS_SUCCESS(status)) {
            char msg[256];
            apr_strerror(status, msg, sizeof(msg));
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "%s\n", msg);
        }
        if (written > 0) {
            buf += written;
            size -= written;
            written = size;
        } else
            break;
    }

    return APR_STATUS_IS_SUCCESS(status) ? JXTA_SUCCESS : JXTA_FAILED;
}

void stream_print(STREAM * stream)
{
    int i, ch;

    printf("[%lu] ", stream->d_len);
    for (i = 0; i < stream->d_len; i++) {
        ch = stream->data_buf[i];
        if (ch < 32) {
            printf("%02X ", (unsigned char) ch);
        } else {
            printf("%c", ch);
        }
    }
    printf("\n");
}

/* vim: set ts=4 sw=4 et tw=130: */
