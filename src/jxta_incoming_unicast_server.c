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
 * $Id: jxta_incoming_unicast_server.c,v 1.32 2006/06/12 18:22:57 slowhog Exp $
 */

static const char *__log_cat = "TCP_UNICAST";


#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_incoming_unicast_server.h"

struct _incoming_unicast_server {
    JXTA_OBJECT_HANDLE;
    Jxta_transport_tcp *tp;
    apr_sockaddr_t *srv_interface;
    apr_socket_t *srv_socket;

    char *ipaddr;
    apr_port_t port;

    volatile Jxta_boolean connected;
    apr_pool_t *pool;
    apr_thread_t *tid;
};

static void unicast_free(Jxta_object * obj);
static void *APR_THREAD_FUNC unicast_accept_thread(apr_thread_t * tid, void *arg);

IncomingUnicastServer *jxta_incoming_unicast_server_new(Jxta_transport_tcp * tp, char *ipaddr, apr_port_t port)
{
    IncomingUnicastServer *self = NULL;
    apr_status_t status;

    /* create object */
    self = (IncomingUnicastServer *) calloc(1, sizeof(IncomingUnicastServer));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed malloc()\n");
        return NULL;
    }

    /* initialize it */
    JXTA_OBJECT_INIT(self, unicast_free, NULL);

    /* setting */
    JXTA_OBJECT_CHECK_VALID(tp);

    self->tp = tp;
    self->ipaddr = strdup(ipaddr);
    self->port = port;
    self->connected = FALSE;
    self->srv_socket = NULL;
    self->tid = NULL;

    /* apr setting */
    status = apr_pool_create(&self->pool, NULL);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create apr pool\n");
        /* Free the memory that has been already allocated */
        free(self);
        return NULL;
    }

    /* interface address */
    /*
       status = apr_sockaddr_info_get(&self->srv_interface, self->ipaddr, APR_INET,
       self->port, 0, self->pool);
     */
    status = apr_sockaddr_info_get(&self->srv_interface, APR_ANYADDR, APR_INET, self->port, 0, self->pool);
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Socket addr info failed : %s\n", msg);
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    return self;
}

static void unicast_free(Jxta_object * obj)
{
    IncomingUnicastServer *self = (IncomingUnicastServer *) obj;

    if (self == NULL)
        return;

    jxta_incoming_unicast_server_stop(self);

    if (self->ipaddr != NULL)
        free(self->ipaddr);

    apr_pool_destroy(self->pool);

    free(self);
}

Jxta_boolean jxta_incoming_unicast_server_start(IncomingUnicastServer * ius)
{
    IncomingUnicastServer *self = ius;
    apr_status_t status;

    JXTA_OBJECT_CHECK_VALID(self);
    /* check if already started */
    if (self->srv_socket != NULL || self->connected)
        return FALSE;

    self->connected = FALSE;

    /* server socket create */
#if CHECK_APR_VERSION(1, 0, 0)
    status = apr_socket_create(&self->srv_socket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, self->pool);
#else
    status = apr_socket_create_ex(&self->srv_socket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, self->pool);
#endif
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed create server socket : %s\n", msg);
        return FALSE;
    }

    /* REUSEADDR: this may help in case of the port is in TIME_WAIT */
    status = apr_socket_opt_set(self->srv_socket, APR_SO_REUSEADDR, 1);
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed setting options : %s\n", msg);
        return FALSE;
    }

    /* bind */
    status = apr_socket_bind(self->srv_socket, self->srv_interface);
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to bind socket : %s\n", msg);
        return FALSE;
    }

    /* listen */
    status = apr_socket_listen(self->srv_socket, MAX_ACCEPT_COUNT_BACKLOG);
    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to listen on socket : %s\n", msg);
        return FALSE;
    }

    /* thread create */
    self->connected = TRUE;
    status = apr_thread_create(&self->tid, NULL, unicast_accept_thread, (void *) self, self->pool);

    if (APR_SUCCESS != status) {
        char msg[256];
        apr_strerror(status, msg, sizeof(msg));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to start thread for socket : %s\n", msg);
        self->connected = FALSE;
        return FALSE;
    }
    return TRUE;
}

void jxta_incoming_unicast_server_stop(IncomingUnicastServer * ius)
{
    IncomingUnicastServer *self = ius;
    apr_status_t status;

    self->connected = FALSE;

    if (self->srv_socket != NULL) {
        apr_socket_shutdown(self->srv_socket, APR_SHUTDOWN_READWRITE);
        apr_socket_close(self->srv_socket);
    }

    if (self->tid != NULL) {
        apr_thread_join(&status, self->tid);
        self->srv_socket = NULL;
        self->tid = NULL;
    }
}

static void *APR_THREAD_FUNC unicast_accept_thread(apr_thread_t * tid, void *arg)
{
    IncomingUnicastServer *self = (IncomingUnicastServer *) arg;
    apr_status_t status;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Unicast incoming accept thread started.\n");

    JXTA_OBJECT_CHECK_VALID(self);

    while (self->connected) {
        Jxta_transport_tcp_connection *conn;
        apr_socket_t *input_socket = NULL;
        char *ipaddr = NULL;

        status = apr_socket_accept(&input_socket, self->srv_socket, self->pool);
        if (APR_SUCCESS != status) {
            char msg[256];
            apr_strerror(status, msg, sizeof(msg));
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Accept failed : %s\n", msg);
            input_socket = NULL;
            continue;
        }

        conn = jxta_transport_tcp_connection_new_2(self->tp, input_socket);
        if (conn == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed creating connection. Closing socket.\n");
            apr_socket_shutdown(input_socket, APR_SHUTDOWN_READWRITE);
            apr_socket_close(input_socket);
            continue;
        }

        JXTA_OBJECT_CHECK_VALID(conn);

        ipaddr = jxta_transport_tcp_connection_get_ipaddr(conn);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incoming connection [%pp] from %s:%d\n", conn, ipaddr,
                        jxta_transport_tcp_connection_get_port(conn));
	    if (ipaddr != NULL) 
            free(ipaddr);

        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Start connection [%pp] failed. Closing socket.\n", conn);
            JXTA_OBJECT_RELEASE(conn);
            apr_socket_shutdown(input_socket, APR_SHUTDOWN_READWRITE);
            apr_socket_close(input_socket);
            input_socket = NULL;
            continue;
        }

        jxta_tcp_got_inbound_connection(self->tp, conn);
        JXTA_OBJECT_RELEASE(conn);
        input_socket = NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Unicast incoming accept thread stopped.\n");

    apr_thread_exit(tid, APR_SUCCESS);

    /* NOTREACHED */
    return NULL;
}

apr_port_t jxta_incoming_unicast_server_get_local_port(IncomingUnicastServer * ius)
{
    IncomingUnicastServer *self = ius;

    JXTA_OBJECT_CHECK_VALID(self);

    return self->port;
}

const char *jxta_incoming_unicast_server_get_local_interface(IncomingUnicastServer * ius)
{
    IncomingUnicastServer *self = ius;

    JXTA_OBJECT_CHECK_VALID(self);

    return self->ipaddr;
}

/* vim: set ts=4 sw=4 tw=130 et: */
