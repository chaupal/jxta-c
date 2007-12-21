/*
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights
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

static const char *__log_cat = "TCP_TRANSPORT";

#include <assert.h>

#include "jpr/jpr_excep_proto.h"

#include "jstring.h"
#include "jdlist.h"
#include "jxta_types.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_peergroup.h"
#include "jxta_apr.h"

#include "jxta_module_private.h"
#include "jxta_transport_private.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_transport_tcp_private.h"
#include "jxta_tcp_multicast.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_util_priv.h"

#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_tta.h"

/* Constants specifying when a messenger should be removed
 * and connection closed */
/* 5 minutes */
#define MESSENGER_TIMEOUT (5*60)
#define CLOSE_CONNECTION_AFTER_TIMEOUT_JPR (5*60*1000)
#define CONSERVATIVE_MESSENGER_REMOVAL

/* 1 minute */
#define INTERVAL_CHECK_FOR_UNUSED_MESSENGERS_APR (60*1000000L)

/*************************************************************************
 **
 *************************************************************************/
#ifdef UNUSED_VWF
static const char *MESSAGE_SOURCE_NS = "jxta";
static const char *MESSAGE_DESTINATION_NS = "jxta";
static const char *MESSAGE_SOURCE_NAME = "EndpointSourceAddress";
static const char *MESSAGE_DESTINATION_NAME = "EndpointDestinationAddress";
#endif

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
typedef struct tcp_conn_elt
{
    APR_RING_ENTRY(tcp_conn_elt) link;
    Jxta_transport_tcp_connection * conn;
} Tcp_conn_elt;

typedef APR_RING_HEAD(tcp_conn_list, tcp_conn_elt) Tcp_connections;

struct _jxta_transport_tcp {
    Extends(Jxta_transport);

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    char *protocol_name;
    char *ipaddr;
    apr_port_t port;
    Jxta_boolean allow_multicast;
    Jxta_boolean be_server;
    apr_socket_t *srv_socket;
    void *srv_poll;

    TcpMulticast *multicast;
    Jxta_endpoint_address *public_addr;
    Jxta_endpoint_service *endpoint;
    JString *peerid;

    Tcp_connections * inbounds;
    Tcp_connections * outbounds;

    Jxta_PG *group;
};

typedef struct _jxta_transport_tcp _jxta_transport_tcp;

typedef Jxta_transport_methods Jxta_transport_tcp_methods;

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, const char *argv[]);
static void stop(Jxta_module * module);

static JString *name_get(Jxta_transport * _self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * _self);
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * t);
static Jxta_boolean allow_routing_p(Jxta_transport * t);
static Jxta_boolean connection_oriented_p(Jxta_transport * t);

static Jxta_transport_tcp *jxta_transport_tcp_construct(Jxta_transport_tcp * _self, Jxta_transport_tcp_methods const *methods);
void jxta_transport_tcp_destruct(Jxta_transport_tcp * _self);
static void tcp_free(Jxta_object * obj);

static Jxta_status create_outbound_connection(Jxta_transport_tcp * me, Jxta_endpoint_address * dest,
                                              Jxta_transport_tcp_connection ** tc);

static Tcp_conn_elt * tcp_conn_new(Jxta_transport_tcp_connection * conn)
{
    Tcp_conn_elt * me;

    me = calloc(1, sizeof(*me));
    APR_RING_ELEM_INIT(me, link);
    me->conn = JXTA_OBJECT_SHARE(conn);

    return me;
}

static void tcp_conn_free(Tcp_conn_elt * me)
{
    JXTA_OBJECT_RELEASE(me->conn);
    free(me);
}

static apr_status_t tcp_connections_cleanup(void *me)
{
    Tcp_connections * myself = me;
    Tcp_conn_elt * conn;

    while (!APR_RING_EMPTY(myself, tcp_conn_elt, link)) {
        conn = APR_RING_FIRST(myself);
        APR_RING_REMOVE(conn, link);
        tcp_conn_free(conn);
    }

    return APR_SUCCESS;
}

static Jxta_status tcp_connections_create(Tcp_connections **me, apr_pool_t * pool)
{
    *me = apr_palloc(pool, sizeof(Tcp_connections));
    if (NULL == *me) {
        return JXTA_NOMEM;
    }
    apr_pool_cleanup_register(pool, *me, tcp_connections_cleanup, apr_pool_cleanup_null);
    APR_RING_INIT(*me, tcp_conn_elt, link);
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connections_destroy(Tcp_connections * me)
{
    tcp_connections_cleanup(me);
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connections_add(Tcp_connections *me, Jxta_transport_tcp_connection * conn)
{
    Tcp_conn_elt *elt;

    elt = tcp_conn_new(conn);
    if (!elt) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        return JXTA_NOMEM;
    }

    APR_RING_INSERT_TAIL(me, elt, tcp_conn_elt, link);
    return JXTA_SUCCESS;
}

static Jxta_status tcp_connections_lookup(Tcp_connections *me, Jxta_endpoint_address *ea, Jxta_transport_tcp_connection **conn)
{
    Tcp_conn_elt *elt;
    Jxta_endpoint_address *remote_ea;

    for (elt = APR_RING_FIRST(me); elt != APR_RING_SENTINEL(me, tcp_conn_elt, link); elt = APR_RING_NEXT(elt, link)) {
        remote_ea = jxta_transport_tcp_connection_get_destaddr(elt->conn);
        if (jxta_endpoint_address_transport_addr_equals(ea, remote_ea)) {
            *conn = JXTA_OBJECT_SHARE(elt->conn);
            return JXTA_SUCCESS;
        }
    }

    return JXTA_ITEM_NOTFOUND;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
const Jxta_transport_tcp_methods jxta_transport_tcp_methods = {
    {
     "Jxta_module_methods",
     init,
     start,
     stop},
    "Jxta_transport_methods",
    name_get,
    publicaddr_get,
    messenger_get,
    ping,
    propagate,
    allow_overload_p,
    allow_routing_p,
    connection_oriented_p
};

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static char *jxta_inet_ntop(Jxta_in_addr addr)
{
    char *str;
    union {
        Jxta_in_addr tmp;
        unsigned char addr[sizeof(Jxta_in_addr)];
    } un;
    /*un.tmp = htonl(addr); */
    un.tmp = addr;
    str = (char *) malloc(16);
    if (str == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }
    apr_snprintf(str, 16, "%u.%u.%u.%u", un.addr[0], un.addr[1], un.addr[2], un.addr[3]);
    return str;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    _jxta_transport_tcp *_self = PTValid(module, _jxta_transport_tcp);
    apr_status_t status;
    Jxta_id *id;
#ifdef UNUSED_VWF
JString *Pid = NULL;
#endif
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_TCPTransportAdvertisement *tta;

    char *multicast_addr;
    apr_port_t multicast_port;
    int multicast_size;
    JString *server_name;
    char * addr_buffer;
    int len;

#ifndef WIN32
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    status = apr_pool_create(&_self->pool, NULL);

    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to create apr pool\n");
        return JXTA_NOMEM;
    }

    status = apr_thread_mutex_create(&_self->mutex, APR_THREAD_MUTEX_NESTED, _self->pool);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to create mutex\n");
        return JXTA_FAILED;
    }

    if (JXTA_SUCCESS != tcp_connections_create(&_self->inbounds, _self->pool)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        return JXTA_NOMEM;
    }

    if (JXTA_SUCCESS != tcp_connections_create(&_self->outbounds, _self->pool)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        return JXTA_NOMEM;
    }

    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get config adv.\n");
        return JXTA_CONFIG_NOTFOUND;
    }

    jxta_PA_get_Svc_with_id(conf_adv, assigned_id, &svc);
    JXTA_OBJECT_RELEASE(conf_adv);

    if (svc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get svc config adv.\n");
        return JXTA_NOT_CONFIGURED;
    }

    tta = jxta_svc_get_TCPTransportAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);
    if (tta == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get tcp config adv.\n");
        return JXTA_CONFIG_NOTFOUND;
    }

    /* Protocol Name */
    _self->protocol_name = jxta_TCPTransportAdvertisement_get_Protocol_string((Jxta_advertisement *) tta);

    /* Server Name */
    server_name = jxta_TCPTransportAdvertisement_get_Server(tta);

    /* port */
    _self->port = (apr_port_t) jxta_TCPTransportAdvertisement_get_Port(tta);

    _self->ipaddr = strdup(jxta_TCPTransportAdvertisement_InterfaceAddress(tta));

    if (jstring_length(server_name) > 0) {
        /* Set public address from server_name */
        if (strchr(jstring_get_string(server_name), ':')) {
            _self->public_addr = jxta_endpoint_address_new_2(_self->protocol_name, jstring_get_string(server_name), NULL, NULL);
        } else {
            len = jstring_length(server_name) + 20;
            addr_buffer = (char *) malloc(len);
            if (addr_buffer == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                JXTA_OBJECT_RELEASE(server_name);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }
            apr_snprintf(addr_buffer, len, "%s:%d", jstring_get_string(server_name), _self->port);
            _self->public_addr = jxta_endpoint_address_new_2(_self->protocol_name, addr_buffer, NULL, NULL);
            free(addr_buffer);
            addr_buffer = NULL;
        }
        if (_self->public_addr == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            JXTA_OBJECT_RELEASE(server_name);
            JXTA_OBJECT_RELEASE(tta);
            return JXTA_NOMEM;
        }
    } else {
        if (0 != strcmp(APR_ANYADDR, _self->ipaddr)) {
            len = strlen(_self->ipaddr) + 20;
            addr_buffer = (char *) malloc(len);
            if (addr_buffer == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                JXTA_OBJECT_RELEASE(server_name);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }
            apr_snprintf(addr_buffer, len, "%s:%d", _self->ipaddr, _self->port);
        } else {
            /* Public Address */
            char *local_host = (char *) malloc(APRMAXHOSTLEN);
            apr_pool_t *pool;
            apr_sockaddr_t *localsa;
            if (local_host == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                JXTA_OBJECT_RELEASE(server_name);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }

            apr_pool_create(&pool, NULL);

            apr_gethostname(local_host, APRMAXHOSTLEN, pool);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport local hostname = %s\n", local_host);

            apr_sockaddr_info_get(&localsa, local_host, APR_INET, 0, 0, pool);
            free(local_host);
            local_host = NULL;

            if (!localsa) {
                /* failed to resolve the name, use 0.0.0.0 as the last resort */
                local_host = APR_ANYADDR;
            } else {
                apr_sockaddr_ip_get(&local_host, localsa);
            }

            len = strlen(local_host) + 20;
            addr_buffer = (char *) malloc(len);
            if (addr_buffer == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                apr_pool_destroy(pool);
                JXTA_OBJECT_RELEASE(server_name);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }
            apr_snprintf(addr_buffer, len, "%s:%d", local_host, _self->port);
            apr_pool_destroy(pool);
        }

        _self->public_addr = jxta_endpoint_address_new_2(_self->protocol_name, addr_buffer, NULL, NULL);
        free(addr_buffer);
        addr_buffer = NULL;
    }

    JXTA_OBJECT_RELEASE(server_name);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport interface addr = %s\n", _self->ipaddr);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport public addr = %s://%s\n",
                    jxta_endpoint_address_get_protocol_name(_self->public_addr),
                    jxta_endpoint_address_get_protocol_address(_self->public_addr));

    /* MulticastOff */
    _self->allow_multicast = jxta_TCPTransportAdvertisement_get_MulticastOff(tta) ? FALSE : TRUE;

    /* MulticastAddr */
    multicast_addr = jxta_inet_ntop(jxta_TCPTransportAdvertisement_get_MulticastAddr(tta));

    /* MulticastPort */
    multicast_port = (apr_port_t) jxta_TCPTransportAdvertisement_get_MulticastPort(tta);

    /* MulticastSize */
    multicast_size = jxta_TCPTransportAdvertisement_get_MulticastSize(tta);

    /* PublicAddress Only */
    /* Not implemented Yet */

    /* Peer Id */
    jxta_PG_get_endpoint_service(group, &_self->endpoint);
    jxta_PG_get_PID(group, &id);
    jxta_id_to_jstring(id, &(_self->peerid));
    JXTA_OBJECT_RELEASE(id);

    if (_self->peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to get peer id string.\n");
        JXTA_OBJECT_RELEASE(tta);
        return JXTA_NOMEM;
    }

    /* Server Enabled */
    if (!jxta_TCPTransportAdvertisement_get_ServerOff(tta)) {
        _self->be_server = JXTA_TRUE;
    }
    JXTA_OBJECT_RELEASE(tta);

    /* Multicast start */
    if (_self->allow_multicast) {
        _self->multicast = tcp_multicast_new(_self, multicast_addr, multicast_port, multicast_size);
        if (_self->multicast == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed new multicast server\n");
            return JXTA_FAILED;
        }
    }

    free(multicast_addr);

    _self->group = group;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP transport inited.\n");

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL accept_cb(void *param, void *arg)
{
#ifdef UNUSED_VWF
apr_pollfd_t *fd = param;
#endif
    Jxta_transport_tcp *me = arg;
    Jxta_transport_tcp_connection *conn;
    apr_socket_t *input_socket = NULL;
    Jxta_status rv;
    char err[256];

    assert(fd->desc.s == me->srv_socket);

    /* 
       NOTE: With test on 9/1/2006, closing a listening socket does not generate any event for pollset.
       Thus, we won't be able to remove socket from pollset here, do it in endpoint_stop instead and make endpoint not blocking
       infinitely, using 1 second timeout in poll
     */
    /**
    if (JXTA_MODULE_STOPPING == jxta_module_state((Jxta_module*) me)) {
        jxta_endpoint_service_remove_poll(me->endpoint, me->srv_poll);
        me->srv_poll = NULL;
        return JXTA_SUCCESS;
    }

    if (fd->rtnevents & APR_POLLHUP || fd->rtnevents & APR_POLLNVAL) {
        jxta_endpoint_service_remove_poll(me->endpoint, me->srv_poll);
        me->srv_poll = NULL;
        return JXTA_SUCCESS;
    }
    */

    rv = apr_socket_accept(&input_socket, me->srv_socket, me->pool);
    if (APR_SUCCESS != rv) {
        apr_strerror(rv, err, sizeof(err));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Accept failed: %s\n", err);
        return JXTA_FAILED;
    }

    conn = jxta_transport_tcp_connection_new_2(me, input_socket);
    if (conn == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed creating connection. Closing socket.\n");
        apr_socket_shutdown(input_socket, APR_SHUTDOWN_READWRITE);
        apr_socket_close(input_socket);
        return JXTA_FAILED;
    }

    tcp_connections_add(me->outbounds, conn);
    JXTA_OBJECT_RELEASE(conn);

    return JXTA_SUCCESS;
}

static Jxta_status check_unused_connections(Jxta_transport_tcp * me, Tcp_connections * conns)
{
    Tcp_conn_elt * elt;
    Tcp_conn_elt * next;
    Jxta_transport_tcp_connection * tc;
    int i;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Started TCP messenger removal iteration for messengers[%pp]\n", conns);

    apr_thread_mutex_lock(me->mutex);

    i = 0;
    for (elt = APR_RING_FIRST(conns); elt != APR_RING_SENTINEL(conns, tcp_conn_elt, link); elt = next) {
        if (jxta_module_state((Jxta_module *) me) != JXTA_MODULE_STARTED) {
           break;
        }

        next = APR_RING_NEXT(elt, link);
        tc = elt->conn;

        if (CONN_DISCONNECTED == tcp_connection_state(tc)) {
            APR_RING_REMOVE(elt, link);
            tcp_conn_free(elt);
            i++;
            continue;
        }

        /* check that the connection belonging to the messenger has
         * not been used for a while. By removing it from here, the connection will be once no one hold the messenger. Do we want
         * to close connection no matter what? */
        if ((get_tcp_connection_last_time_used(tc) + CLOSE_CONNECTION_AFTER_TIMEOUT_JPR) < jpr_time_now()) {
#ifndef CONSERVATIVE_MESSENGER_REMOVAL
            jxta_transport_tcp_connection_close(tc);
#endif
            APR_RING_REMOVE(elt, link);
            tcp_conn_free(elt);
            i++;
            continue;
        }
    }

    apr_thread_mutex_unlock(me->mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%d TCP messengers were removed from messengers[%pp]\n", i, conns);
    return JXTA_SUCCESS;
}

/*
 * The entry point for the remove_unused_messengers thread, 
 * the reads watches the hashtable of messengers and removes
 * the messengers which do not belong to anyone and have not been
 * used in a while
 *
 * @param thread the thread
 * @param arg _jxta_transport_tcp
 * @return
 */
static void *APR_THREAD_FUNC remove_unused_connections(apr_thread_t * thread, void *arg)
{
    _jxta_transport_tcp *me = arg;

    assert(PTValid(arg, _jxta_transport_tcp));

    check_unused_connections(me, me->inbounds);
    check_unused_connections(me, me->outbounds);

    apr_thread_mutex_lock(me->mutex);
    if (JXTA_MODULE_STARTED == jxta_module_state((Jxta_module *) me)) {
        apr_thread_pool_schedule(jxta_PG_thread_pool_get(me->group), remove_unused_connections, me,
                                 INTERVAL_CHECK_FOR_UNUSED_MESSENGERS_APR, me);
    }
    apr_thread_mutex_unlock(me->mutex);

    return NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    Jxta_status rv;
    _jxta_transport_tcp *_self = PTValid(module, _jxta_transport_tcp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting TCP transport ...\n");
    jxta_endpoint_service_add_transport(_self->endpoint, (Jxta_transport *) _self);

    if (_self->be_server) {
        rv = ep_tcp_socket_listen(&_self->srv_socket, _self->ipaddr, _self->port, MAX_ACCEPT_COUNT_BACKLOG, _self->pool);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start incoming server\n");
            return JXTA_FAILED;
        }
        jxta_endpoint_service_add_poll(_self->endpoint, _self->srv_socket, accept_cb, _self, &_self->srv_poll);
    }

    if (NULL != _self->multicast) {
        if (!tcp_multicast_start(_self->multicast)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start multicast server\n");
            JXTA_OBJECT_RELEASE(_self->multicast);
            _self->multicast = NULL;
            return JXTA_FAILED;
        }
    }

    /* start the thread for deleting unused messengers */
    apr_thread_pool_schedule(jxta_PG_thread_pool_get(_self->group), remove_unused_connections, _self,
                             INTERVAL_CHECK_FOR_UNUSED_MESSENGERS_APR, _self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP transport started\n");
    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void stop(Jxta_module * me)
{
    _jxta_transport_tcp *myself = PTValid(me, _jxta_transport_tcp);

    if (myself->be_server) {
        if (myself->srv_poll) {
            jxta_endpoint_service_remove_poll(myself->endpoint, myself->srv_poll);
            myself->srv_poll = NULL;
        }
        apr_socket_shutdown(myself->srv_socket, APR_SHUTDOWN_READWRITE);
        apr_socket_close(myself->srv_socket);
    }

    if (NULL != myself->multicast) {
        tcp_multicast_stop(myself->multicast);
    }

    apr_thread_pool_tasks_cancel(jxta_PG_thread_pool_get(myself->group), (void *) myself);

    jxta_endpoint_service_remove_transport(myself->endpoint, (Jxta_transport *) myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}


/**************************
 * Tcp Transport methods *
 *************************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JString *name_get(Jxta_transport * t)
{
    _jxta_transport_tcp *_self = PTValid(t, _jxta_transport_tcp);

    JString *tmp = jstring_new_2(_self->protocol_name);
    if (tmp == NULL)
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
    return tmp;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    _jxta_transport_tcp *_self = PTValid(t, _jxta_transport_tcp);

    return JXTA_OBJECT_SHARE(_self->public_addr);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JxtaEndpointMessenger *messenger_get(Jxta_transport * me, Jxta_endpoint_address * dest)
{
    _jxta_transport_tcp *myself = PTValid(me, _jxta_transport_tcp);
    Jxta_status rv;
    Jxta_transport_tcp_connection *tc;
    TcpMessenger * msgr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Need a messenger for %s://%s.\n",
                    jxta_endpoint_address_get_protocol_name(dest), jxta_endpoint_address_get_protocol_address(dest));

    apr_thread_mutex_lock(myself->mutex);
    /* should no needed, if messenger is available, it would be registered with ep already */
    /* Begin: Drop this section to optimize */
    /* Cannot drop because relay is getting messenger from transport directly, not from endpoint */
    rv = tcp_connections_lookup(myself->inbounds, dest, &tc);
    if (JXTA_SUCCESS != rv) {
        rv = tcp_connections_lookup(myself->outbounds, dest, &tc);
    }
    /* End: Drop this section to optimize */
    if (JXTA_SUCCESS != rv) {
        rv = create_outbound_connection(myself, dest, &tc);
    }
    apr_thread_mutex_unlock(myself->mutex);
    if (JXTA_SUCCESS != rv) {
        return NULL;
    }

    assert(tc);
    rv = tcp_connection_get_messenger(tc, apr_time_from_sec(60), &msgr);
    switch (rv) {
        case JXTA_TIMEOUT:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE 
                            "Tcp connection[%pp] for %s://%s is not ready after 1 minute.\n", tc,
                            jxta_endpoint_address_get_protocol_name(dest), jxta_endpoint_address_get_protocol_address(dest));
            msgr = NULL;
            break;
        case JXTA_SUCCESS:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Get Messenger[%pp] for %s://%s.\n", msgr,
                            jxta_endpoint_address_get_protocol_name(dest), jxta_endpoint_address_get_protocol_address(dest));
            break;
        case JXTA_FAILED:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE 
                            "Tcp connection[%pp] for %s://%s is not ready, more likely it had been closed.\n", tc,
                            jxta_endpoint_address_get_protocol_name(dest), jxta_endpoint_address_get_protocol_address(dest));
            msgr = NULL;
            break;
        default:
            assert(FALSE);
            msgr = NULL;
            break;
    }

    return (JxtaEndpointMessenger*) msgr;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * me, Jxta_endpoint_address * addr)
{
#ifdef UNUSED_VWF
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif
    /* XXX bondolo 20050513 This implementation just makes things worse... */
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void propagate(Jxta_transport * me, Jxta_message * msg, const char *service_name, const char *service_params)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    if (_self->allow_multicast) {
        jxta_message_set_source(msg, _self->public_addr);
        if (tcp_multicast_propagate(_self->multicast, msg, service_name, service_params) != JXTA_SUCCESS)
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Propagate failed\n");
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_overload_p(Jxta_transport * me)
{
#ifdef UNUSED_VWF
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif
    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * me)
{
#ifdef UNUSED_VWF
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * me)
{
#ifdef UNUSED_VWF
_jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif

    return TRUE;
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_transport_tcp *jxta_transport_tcp_construct(_jxta_transport_tcp * _self, Jxta_transport_tcp_methods const *methods)
{
    Jxta_transport_methods* transport_methods = PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) _self, (Jxta_transport_methods const *) transport_methods);

    _self->thisType = "_jxta_transport_tcp";

    _self->_super.metric = 6;   /* value decided as JSE implemetnation */
    _self->_super.direction = JXTA_BIDIRECTION;

    _self->pool = NULL;
    _self->mutex = NULL;

    _self->protocol_name = NULL;
    _self->peerid = NULL;
    _self->inbounds = NULL;
    _self->outbounds = NULL;
    _self->be_server = JXTA_FALSE;
    _self->multicast = NULL;
    _self->public_addr = NULL;
    _self->endpoint = NULL;

    _self->srv_poll = NULL;
    _self->srv_socket = NULL;

    _self->group = NULL;

    return _self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_tcp_destruct(Jxta_transport_tcp * tp)
{
    _jxta_transport_tcp *_self = PTValid(tp, _jxta_transport_tcp);

    if (_self->mutex) {
        apr_thread_mutex_lock(_self->mutex);
    }
    if (_self->protocol_name != NULL) {
        free(_self->protocol_name);
    }

    if (_self->peerid != NULL) {
        JXTA_OBJECT_RELEASE(_self->peerid);
    }

    if (_self->ipaddr != NULL) {
        free(_self->ipaddr);
    }

    tcp_connections_destroy(_self->inbounds);
    tcp_connections_destroy(_self->outbounds);

    if (_self->public_addr != NULL) {
        JXTA_OBJECT_RELEASE(_self->public_addr);
    }

    if (_self->multicast != NULL) {
        JXTA_OBJECT_RELEASE(_self->multicast);
    }

    if (_self->endpoint != NULL) {
        JXTA_OBJECT_RELEASE(_self->endpoint);
    }

    apr_thread_mutex_destroy(_self->mutex);

    if (_self->pool) {
        apr_pool_destroy(_self->pool);
    }

    _self->thisType = NULL;

    jxta_transport_destruct((Jxta_transport *) _self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction of [%pp] finished\n", _self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void tcp_free(Jxta_object * obj)
{
    jxta_transport_tcp_destruct((Jxta_transport_tcp *) obj);

    memset(obj, 0xdd, sizeof(Jxta_transport_tcp));
    free(obj);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_transport_tcp *jxta_transport_tcp_new_instance()
{
    Jxta_transport_tcp *_self = (Jxta_transport_tcp *) calloc(1, sizeof(Jxta_transport_tcp));
    if (_self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(_self, tcp_free, NULL);

    return jxta_transport_tcp_construct(_self, (Jxta_transport_tcp_methods const *) &jxta_transport_tcp_methods);;
}

Jxta_endpoint_service *jxta_transport_tcp_get_endpoint_service(Jxta_transport_tcp * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    JXTA_OBJECT_CHECK_VALID(_self->endpoint);

    return JXTA_OBJECT_SHARE(_self->endpoint);
}

const char *jxta_transport_tcp_local_ipaddr_cstr(Jxta_transport_tcp * me)
{
    if (NULL == me) {
        return NULL;
    }

    return me->ipaddr;
}

char *jxta_transport_tcp_get_local_ipaddr(Jxta_transport_tcp * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return strdup(_self->ipaddr);
}

apr_port_t jxta_transport_tcp_get_local_port(Jxta_transport_tcp * me)
{
#ifdef UNUSED_VWF
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif
    return me->port;
}

JString *jxta_transport_tcp_get_peerid(Jxta_transport_tcp * me)
{
#ifdef UNUSED_VWF
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
#endif
    return JXTA_OBJECT_SHARE(me->peerid);
}

Jxta_endpoint_address *jxta_transport_tcp_get_public_addr(Jxta_transport_tcp * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    JXTA_OBJECT_CHECK_VALID(_self->public_addr);

    return _self->public_addr;
}

Jxta_boolean jxta_transport_tcp_get_allow_multicast(Jxta_transport_tcp * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return _self->allow_multicast;
}

static Jxta_status create_outbound_connection(Jxta_transport_tcp * me, Jxta_endpoint_address * dest, 
                                              Jxta_transport_tcp_connection ** tc)
{
    Jxta_transport_tcp_connection * conn;

    conn = jxta_transport_tcp_connection_new_1(me, dest);
    if (NULL == conn) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed opening connection -> %s://%s\n",
                        jxta_endpoint_address_get_protocol_name(dest), jxta_endpoint_address_get_protocol_address(dest));
        *tc = NULL;
        return JXTA_FAILED;
    }

    tcp_connections_add(me->outbounds, conn);
    *tc = conn;

    return JXTA_SUCCESS;
}

/* vim: set sw=4 ts=4 tw=130 et: */
