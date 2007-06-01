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
 * $Id: jxta_transport_tcp.c,v 1.61 2006/10/11 20:08:21 slowhog Exp $
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

/*************************************************************************
 **
 *************************************************************************/
static const char *MESSAGE_SOURCE_NS = "jxta";
static const char *MESSAGE_DESTINATION_NS = "jxta";
static const char *MESSAGE_SOURCE_NAME = "EndpointSourceAddress";
static const char *MESSAGE_DESTINATION_NAME = "EndpointDestinationAddress";

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
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

    Jxta_hashtable *tcp_messengers;
};

typedef struct _jxta_transport_tcp _jxta_transport_tcp;

typedef Jxta_transport_methods Jxta_transport_tcp_methods;

struct _tcp_messenger {
    struct _JxtaEndpointMessenger messenger;

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    Jxta_transport_tcp *tp;
    Jxta_transport_tcp_connection *conn;

    Jxta_message_element *src_msg_el;

    Jxta_boolean initialized_connection;
};

typedef struct _tcp_messenger _tcp_messenger;

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

static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg);
static void tcp_messenger_free(Jxta_object * obj);

static Jxta_status tcp_demux_cb(void *param, void *arg);
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp);
static Jxta_status tcp_messenger_initialize_connection(TcpMessenger * mes, Jxta_transport_tcp_connection * conn,
                                                       Jxta_endpoint_address * dest);

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
    JString *Pid = NULL;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_TCPTransportAdvertisement *tta;

    char *multicast_addr;
    apr_port_t multicast_port;
    int multicast_size;
    JString *server_name;

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

    _self->tcp_messengers = jxta_hashtable_new(20);
    if (_self->tcp_messengers == NULL) {
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
        _self->public_addr = jxta_endpoint_address_new_2(_self->protocol_name, jstring_get_string(server_name), NULL, NULL);
        if (_self->public_addr == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
            JXTA_OBJECT_RELEASE(server_name);
            return JXTA_NOMEM;
        }
    } else {
        char *proto_addr;
        int len;

        if (0 != strcmp(APR_ANYADDR, _self->ipaddr)) {
            len = strlen(_self->ipaddr) + 20;
            proto_addr = (char *) malloc(len);
            if (proto_addr == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                JXTA_OBJECT_RELEASE(server_name);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }
            apr_snprintf(proto_addr, len, "%s:%d", _self->ipaddr, _self->port);
        } else {
            /* Public Address */
            char *local_host = (char *) malloc(APRMAXHOSTLEN);
            apr_pool_t *pool;
            apr_sockaddr_t *localsa;
            if (local_host == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }

            apr_pool_create(&pool, NULL);

            apr_gethostname(local_host, APRMAXHOSTLEN, pool);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport local hostname = %s\n", local_host);

            apr_sockaddr_info_get(&localsa, local_host, APR_INET, 0, 0, pool);
            free(local_host);
            local_host = NULL;

            apr_sockaddr_ip_get(&local_host, localsa);

            len = strlen(local_host) + 20;
            proto_addr = (char *) malloc(len);
            if (proto_addr == NULL) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
                apr_pool_destroy(pool);
                JXTA_OBJECT_RELEASE(tta);
                return JXTA_NOMEM;
            }
            apr_snprintf(proto_addr, len, "%s:%d", local_host, _self->port);
            apr_pool_destroy(pool);
        }

        _self->public_addr = jxta_endpoint_address_new_2(_self->protocol_name, proto_addr, NULL, NULL);
        free(proto_addr);
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

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP transport inited.\n");

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL accept_cb(void *param, void *arg)
{
    apr_pollfd_t *fd = param;
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

    JXTA_OBJECT_CHECK_VALID(conn);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Incoming connection[%pp] from %s:%d\n", conn,
                    jxta_transport_tcp_connection_ipaddr(conn), jxta_transport_tcp_connection_get_port(conn));

    tcp_got_inbound_connection(me, conn);
    JXTA_OBJECT_RELEASE(conn);
    return JXTA_SUCCESS;
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
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    _jxta_transport_tcp *_self = PTValid(t, _jxta_transport_tcp);

    JxtaEndpointMessenger *messenger = (JxtaEndpointMessenger *) get_tcp_messenger(_self, NULL, dest);

    return messenger;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * me, Jxta_endpoint_address * addr)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

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
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return TRUE;
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_transport_tcp *jxta_transport_tcp_construct(_jxta_transport_tcp * _self, Jxta_transport_tcp_methods const *methods)
{
    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) _self, (Jxta_transport_methods const *) methods);

    _self->thisType = "_jxta_transport_tcp";

    _self->_super.metric = 6;   /* value decided as JSE implemetnation */
    _self->_super.direction = JXTA_BIDIRECTION;

    _self->pool = NULL;
    _self->mutex = NULL;

    _self->protocol_name = NULL;
    _self->peerid = NULL;
    _self->tcp_messengers = NULL;
    _self->be_server = JXTA_FALSE;
    _self->multicast = NULL;
    _self->public_addr = NULL;
    _self->endpoint = NULL;

    _self->srv_poll = NULL;
    _self->srv_socket = NULL;

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

    if (_self->tcp_messengers != NULL) {
        JXTA_OBJECT_RELEASE(_self->tcp_messengers);
    }

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

/******************************************************************************/
/*                                                                            */
/******************************************************************************/


/************************************************************************
* Creates new TCP messenger, does not open a new connection
*
* @param tp TCP transport
* @return new messenger is successful, NULL otherwise
*************************************************************************/
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp)
{
    TcpMessenger *mes = NULL;
    apr_status_t status;

    JXTA_OBJECT_CHECK_VALID(tp);

    /* create the object */
    mes = (TcpMessenger *) calloc(1, sizeof(TcpMessenger));
    if (mes == NULL)
        return NULL;

    /* setting */
    status = apr_pool_create(&mes->pool, NULL);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger apr pool\n");
        free(mes);
        return NULL;
    }

    status = apr_thread_mutex_create(&mes->mutex, APR_THREAD_MUTEX_NESTED, mes->pool);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger mutex\n");
        apr_pool_destroy(mes->pool);
        free(mes);
        return NULL;
    }

    mes->tp = tp;   /* NOTE: We do not share a reference for the transport */

    mes->initialized_connection = FALSE;

    mes->messenger.jxta_send = NULL;

    /* initialize it */
    JXTA_OBJECT_INIT(mes, tcp_messenger_free, NULL);

    return mes;
}

/************************************************************************
* Open a connection for a tcp messenger
*
* @param mes the messenger
* @param conn the connection object if incoming, NULL otherwise
* @param dest destination address
* @return JXTA_SUCCESS if the connection was established
*************************************************************************/
static Jxta_status tcp_messenger_initialize_connection(TcpMessenger * mes, Jxta_transport_tcp_connection * conn,
                                                       Jxta_endpoint_address * dest)
{
    apr_status_t status;
    char *addr_buffer;
    Jxta_endpoint_address *src_addr;

    assert(mes);

    apr_thread_mutex_lock(mes->mutex);

    if (mes->initialized_connection) {
        apr_thread_mutex_unlock(mes->mutex);
        return JXTA_SUCCESS;
    }

    mes->initialized_connection = TRUE;

    if (!conn) {
        mes->conn = jxta_transport_tcp_connection_new_1(mes->tp, dest);
        if (NULL == mes->conn) {
            addr_buffer = jxta_endpoint_address_get_transport_addr(dest);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed opening connection -> %s\n", addr_buffer);
            free(addr_buffer);
            apr_thread_mutex_unlock(mes->mutex);
            return JXTA_FAILED;
        }
    } else {
        mes->conn = JXTA_OBJECT_SHARE(conn);
    }

    status = jxta_transport_tcp_connection_start(mes->conn);
    if (JXTA_SUCCESS != status) {
        addr_buffer = jxta_endpoint_address_get_transport_addr(dest);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start TCP connection with %s\n", addr_buffer);
        free(addr_buffer);
        JXTA_OBJECT_RELEASE(mes->conn);
        mes->conn = NULL;
        apr_thread_mutex_unlock(mes->mutex);
        return JXTA_FAILED;
    }

    src_addr = publicaddr_get((Jxta_transport *) mes->tp);
    addr_buffer = jxta_endpoint_address_to_string(src_addr);
    mes->src_msg_el = jxta_message_element_new_2(MESSAGE_SOURCE_NS,
                                                 MESSAGE_SOURCE_NAME, "text/plain", addr_buffer, strlen(addr_buffer), NULL);
    if (mes->src_msg_el == NULL) {
        apr_thread_mutex_unlock(mes->mutex);
        return JXTA_FAILED;
    }
    free(addr_buffer);
    JXTA_OBJECT_RELEASE(src_addr);

    mes->messenger.address = JXTA_OBJECT_SHARE(dest);
    mes->messenger.jxta_send = tcp_messenger_send;

    apr_thread_mutex_unlock(mes->mutex);
    return JXTA_SUCCESS;
}

static void tcp_messenger_free(Jxta_object * obj)
{
    TcpMessenger *mes = (TcpMessenger *) obj;

    if (mes->src_msg_el != NULL)
        JXTA_OBJECT_RELEASE(mes->src_msg_el);

    if (NULL != mes->messenger.address)
        JXTA_OBJECT_RELEASE(mes->messenger.address);

    if (NULL != mes->conn) {
        jxta_transport_tcp_connection_close(mes->conn);
        JXTA_OBJECT_RELEASE(mes->conn);
    }

    apr_thread_mutex_destroy(mes->mutex);
    apr_pool_destroy(mes->pool);

    memset(mes, 0xDD, sizeof(TcpMessenger));
    free(mes);
}

Jxta_boolean messenger_is_connected(TcpMessenger * mes)
{
    if (NULL == mes->conn) {
        return FALSE;
    }

    return jxta_transport_tcp_connection_is_connected(mes->conn);
}

TcpMessenger *get_tcp_messenger(Jxta_transport_tcp * me, Jxta_transport_tcp_connection * conn, Jxta_endpoint_address * addr)
{
    Jxta_status res;
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);
    TcpMessenger *messenger = NULL;
    char *key;

    JXTA_OBJECT_CHECK_VALID(addr);

    key = jxta_endpoint_address_get_transport_addr(addr);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Get Messenger -> %s\n", key);

    apr_thread_mutex_lock(_self->mutex);

    res = jxta_hashtable_get(_self->tcp_messengers, key, strlen(key), JXTA_OBJECT_PPTR(&messenger));

    if (NULL != messenger) {
        if ((FALSE == messenger_is_connected(messenger)) && (messenger->initialized_connection)) {
            jxta_hashtable_del(_self->tcp_messengers, key, strlen(key), NULL);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remove TcpMessenger into vector\n");
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid TcpMessenger [%pp] removed.\n", messenger);
            }
            JXTA_OBJECT_RELEASE(messenger);
            messenger = NULL;
        }
    }

    if ((res != JXTA_SUCCESS) || (NULL == messenger)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Create new messenger -> %s\n", key);
        messenger = tcp_messenger_new(_self);

        if (messenger != NULL) {
            jxta_hashtable_put(_self->tcp_messengers, key, strlen(key), (Jxta_object *) messenger);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed getting messenger -> %s\n", key);
        }
    }
    apr_thread_mutex_unlock(_self->mutex);

    /* try to initialize the messenger */
    if (!messenger->initialized_connection) {
        /* if unable to initialize connection do not return the messenger */
        if (tcp_messenger_initialize_connection(messenger, conn, addr) != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unable to create connection for messenger [%pp] -> %s\n", messenger,
                            key);
            free(key);
            JXTA_OBJECT_RELEASE(messenger);
            return NULL;
        }
    }

    if (NULL != messenger) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got Messenger [%pp] -> %s\n", messenger, key);
    }

    free(key);
    return messenger;
}

Jxta_status jxta_transport_tcp_remove_messenger(Jxta_transport_tcp * _self, Jxta_endpoint_address * addr)
{
    Jxta_status res;
    char *key;

    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(addr);

    key = jxta_endpoint_address_to_string(addr);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Remove messenger -> %s\n", key);

    apr_thread_mutex_lock(_self->mutex);

    res = jxta_hashtable_del(_self->tcp_messengers, key, strlen(key), NULL);

    apr_thread_mutex_unlock(_self->mutex);

    free(key);
    return res;
}

static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{
    Jxta_status status;
    TcpMessenger *_self = (TcpMessenger *) mes;

    JXTA_OBJECT_CHECK_VALID(_self);
    JXTA_OBJECT_CHECK_VALID(msg);

    if (!_self->initialized_connection) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Tried to send on uninitialized TCP messenger\n");
        return JXTA_FAILED;
    }

    apr_thread_mutex_lock(_self->mutex);

    if (NULL == _self->conn) {
        apr_thread_mutex_unlock(_self->mutex);
        return JXTA_FAILED;
    } else {
        JXTA_OBJECT_CHECK_VALID(_self->conn);
    }

    apr_thread_mutex_unlock(_self->mutex);

    JXTA_OBJECT_SHARE(msg);

    jxta_message_remove_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME);
    jxta_message_add_element(msg, _self->src_msg_el);

    status = jxta_transport_tcp_connection_send_message(_self->conn, msg);

    JXTA_OBJECT_RELEASE(msg);

    return status;
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
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

    return me->port;
}

JString *jxta_transport_tcp_get_peerid(Jxta_transport_tcp * me)
{
    _jxta_transport_tcp *_self = PTValid(me, _jxta_transport_tcp);

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

void tcp_got_inbound_connection(Jxta_transport_tcp * me, Jxta_transport_tcp_connection * conn)
{
    TcpMessenger *m = NULL;
    Jxta_transport_event *e = NULL;
    Jxta_endpoint_address *ea;

    ea = jxta_transport_tcp_connection_get_destaddr(conn);
    /* Add the connection to messenger table */
    m = get_tcp_messenger(me, conn, ea);
    if (NULL == m) {
        JXTA_OBJECT_RELEASE(ea);
        jxta_transport_tcp_connection_close(conn);
        return;
    }

    JXTA_OBJECT_RELEASE(m);
    e = jxta_transport_event_new(JXTA_TRANSPORT_INBOUND_CONNECT);
    if (NULL == e) {
        JXTA_OBJECT_RELEASE(ea);
        jxta_transport_tcp_connection_close(conn);
        return;
    }

    e->dest_addr = ea;
    e->peer_id = jxta_transport_tcp_connection_get_destination_peerid(conn);
    jxta_endpoint_service_transport_event(me->endpoint, e);
    JXTA_OBJECT_RELEASE(e);
}

/* vim: set sw=4 ts=4 tw=130 et: */
