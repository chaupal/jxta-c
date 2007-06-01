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
 * $Id: jxta_transport_tcp.c,v 1.41 2005/09/15 02:31:46 bondolo Exp $
 */

static const char *__log_cat = "TCP_TRANSPORT";

#include "jpr/jpr_excep_proto.h"

#include "jstring.h"
#include "jdlist.h"
#include "jxta_types.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_peergroup.h"
#include "jxtaapr.h"

#include "jxta_transport_private.h"
#include "jxta_transport_tcp_connection.h"
#include "jxta_transport_tcp_private.h"
#include "jxta_incoming_unicast_server.h"
#include "jxta_tcp_multicast.h"

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
    Jxta_in_addr intf_addr;
    char *ipaddr;
    apr_port_t port;
    Jxta_boolean allow_multicast;
    Jxta_boolean public_address_only;
    IncomingUnicastServer *unicast_server;
    TcpMulticast *multicast;
    Jxta_endpoint_address *public_addr;
    Jxta_endpoint_service *endpoint;
    char *peerid;

    Jxta_hashtable *tcp_messengers;
};

typedef Jxta_transport_methods Jxta_transport_tcp_methods;

struct _tcp_messenger {
    struct _JxtaEndpointMessenger messenger;

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    Jxta_transport_tcp *tp;
    Jxta_transport_tcp_connection *conn;
    Jxta_boolean incoming;

    Jxta_message_element *src_msg_el;
};

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, const char *argv[]);
static void stop(Jxta_module * module);

static JString *name_get(Jxta_transport * self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * self);
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * t);
static Jxta_boolean allow_routing_p(Jxta_transport * t);
static Jxta_boolean connection_oriented_p(Jxta_transport * t);

Jxta_transport_tcp *jxta_transport_tcp_construct(Jxta_transport_tcp * self, Jxta_transport_tcp_methods const *methods);
void jxta_transport_tcp_destruct(Jxta_transport_tcp * self);
static void tcp_free(Jxta_object * obj);

static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp, Jxta_transport_tcp_connection * conn,
                                       Jxta_endpoint_address * dest);
static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg);
static void tcp_messenger_free(Jxta_object * obj);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
const Jxta_transport_tcp_methods jxta_transport_tcp_methods = {
    {
     "Jxta_module_methods",
     init,
     jxta_module_init_e_impl,
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
    apr_snprintf(str, 16, "%u.%u.%u.%u", un.addr[0], un.addr[1], un.addr[2], un.addr[3]);
    return str;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_transport_tcp *self = PTValid(module, Jxta_transport_tcp);
    apr_status_t status;
    Jxta_id *id;
    JString *Pid = NULL;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_vector *svcs;
    unsigned int sz, i;
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

    status = apr_pool_create(&self->pool, NULL);

    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to create apr pool\n");
        return JXTA_NOMEM;
    }

    status = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to create mutex\n");
        return JXTA_FAILED;
    }

    self->tcp_messengers = jxta_hashtable_new(20);
    if (self->tcp_messengers == NULL)
        return JXTA_NOMEM;

    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL)
        return JXTA_CONFIG_NOTFOUND;

    svcs = jxta_PA_get_Svc(conf_adv);
    JXTA_OBJECT_RELEASE(conf_adv);

    sz = jxta_vector_size(svcs);
    for (i = 0; i < sz; i++) {
        Jxta_id *mcid;
        Jxta_svc *tmpsvc = NULL;
        jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
        mcid = jxta_svc_get_MCID(tmpsvc);
        if (jxta_id_equals(mcid, assigned_id)) {
            svc = tmpsvc;
            JXTA_OBJECT_RELEASE(mcid);
            break;
        }
        JXTA_OBJECT_RELEASE(mcid);
        JXTA_OBJECT_RELEASE(tmpsvc);
    }
    JXTA_OBJECT_RELEASE(svcs);

    if (svc == NULL) {
        return JXTA_NOT_CONFIGURED;
    }

    tta = jxta_svc_get_TCPTransportAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);
    if (tta == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    /* Protocol Name */
    self->protocol_name = jxta_TCPTransportAdvertisement_get_Protocol_string(tta);

    /* Server Name */
    server_name = jxta_TCPTransportAdvertisement_get_Server(tta);

    /* port */
    self->port = (apr_port_t) jxta_TCPTransportAdvertisement_get_Port(tta);

    self->intf_addr = jxta_TCPTransportAdvertisement_get_InterfaceAddress(tta);
    self->ipaddr = jxta_inet_ntop(self->intf_addr);

    if (jstring_length(server_name) > 0) {
        /* Set public address from server_name */
        self->public_addr = jxta_endpoint_address_new2(self->protocol_name, jstring_get_string(server_name), NULL, NULL);
    } else {
        char *proto_addr;
        int len;

        if (0 != strcmp(APR_ANYADDR, self->ipaddr)) {
            len = strlen(self->ipaddr) + 20;
            proto_addr = (char *) malloc(len);
            apr_snprintf(proto_addr, len, "%s:%d", self->ipaddr, self->port);
        } else {
            /* Public Address */
            char *local_host = (char *) malloc(APRMAXHOSTLEN);
            apr_pool_t *pool;
            apr_sockaddr_t *localsa;

            apr_pool_create(&pool, NULL);

            apr_gethostname(local_host, APRMAXHOSTLEN, pool);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport local hostname = %s\n", local_host);

            apr_sockaddr_info_get(&localsa, local_host, APR_INET, 0, 0, pool);
            free(local_host);

            apr_sockaddr_ip_get(&local_host, localsa);

            len = strlen(local_host) + 20;
            proto_addr = (char *) malloc(len);
            apr_snprintf(proto_addr, len, "%s:%d", local_host, self->port);
            apr_pool_destroy(pool);
        }

        self->public_addr = jxta_endpoint_address_new2(self->protocol_name, proto_addr, NULL, NULL);
        free(proto_addr);
    }

    JXTA_OBJECT_RELEASE(server_name);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport interface addr = %s\n", self->ipaddr);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TCP Transport public addr = %s://%s\n",
                    jxta_endpoint_address_get_protocol_name(self->public_addr),
                    jxta_endpoint_address_get_protocol_address(self->public_addr));

    /* MulticastOff */
    self->allow_multicast = jxta_TCPTransportAdvertisement_get_MulticastOff(tta) ? FALSE : TRUE;

    /* MulticastAddr */
    multicast_addr = jxta_inet_ntop(jxta_TCPTransportAdvertisement_get_MulticastAddr(tta));

    /* MulticastPort */
    multicast_port = (apr_port_t) jxta_TCPTransportAdvertisement_get_MulticastPort(tta);

    /* MulticastSize */
    multicast_size = jxta_TCPTransportAdvertisement_get_MulticastSize(tta);

    /* PublicAddress Only */
    /* Not implemented Yet */

    /* Peer Id */
    jxta_PG_get_endpoint_service(group, &self->endpoint);
    jxta_PG_get_PID(group, &id);
    /*
       jxta_id_get_uniqueportion(id, &uniquePid);
     */
    jxta_id_to_jstring(id, &Pid);
    self->peerid = strdup(jstring_get_string(Pid));

    if (self->peerid == NULL) {
        JXTA_OBJECT_RELEASE(tta);
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_RELEASE(Pid);
    JXTA_OBJECT_RELEASE(id);

    /* Server Enabled */
    if (!jxta_TCPTransportAdvertisement_get_ServerOff(tta)) {
        self->unicast_server = jxta_incoming_unicast_server_new(self, self->ipaddr, self->port);
        if (self->unicast_server == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed new incoming unicast server\n");
            JXTA_OBJECT_RELEASE(tta);
            return JXTA_FAILED;
        }
    }
    JXTA_OBJECT_RELEASE(tta);

    /* Multicast start */
    if (self->allow_multicast) {
        self->multicast = tcp_multicast_new(self, multicast_addr, multicast_port, multicast_size);
        if (self->multicast == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed new multicast server\n");
            return JXTA_FAILED;
        }
    }

    free(multicast_addr);

    /*
       apr_thread_yield();
     */

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    Jxta_transport_tcp *self = PTValid(module, Jxta_transport_tcp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting TCP transport ...\n");
    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    if (NULL != self->unicast_server) {
        if (!jxta_incoming_unicast_server_start(self->unicast_server)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start incoming unicast server\n");
            JXTA_OBJECT_RELEASE(self->unicast_server);
            self->unicast_server = NULL;
            return JXTA_FAILED;
        }
    }

    if (NULL != self->multicast) {
        if (!tcp_multicast_start(self->multicast)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start multicast server\n");
            JXTA_OBJECT_RELEASE(self->multicast);
            self->multicast = NULL;
            return JXTA_FAILED;
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "TCP transport started\n");
    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void stop(Jxta_module * module)
{
    Jxta_transport_tcp *self = PTValid(module, Jxta_transport_tcp);

    if (NULL != self->unicast_server) {
        jxta_incoming_unicast_server_stop(self->unicast_server);
    }

    if (NULL != self->multicast) {
        tcp_multicast_stop(self->multicast);
    }

    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);

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
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    return jstring_new_2(self->protocol_name);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    return JXTA_OBJECT_SHARE(self->public_addr);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    JxtaEndpointMessenger *messenger = (JxtaEndpointMessenger *) get_tcp_messenger(self, NULL, dest);

    return messenger;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    /* XXX bondolo 20050513 This implementation just makes things worse... */
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;

    if (self->allow_multicast) {
        jxta_message_set_source(msg, self->public_addr);
        tcp_multicast_propagate(self->multicast, msg, service_name, service_params);
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_overload_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = PTValid(t, Jxta_transport_tcp);

    return TRUE;
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_transport_tcp *jxta_transport_tcp_construct(Jxta_transport_tcp * tp, Jxta_transport_tcp_methods const *methods)
{
    Jxta_transport_tcp *self = tp;

    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods const *) methods);

    self->thisType = "Jxta_transport_tcp";

    self->pool = NULL;
    self->mutex = NULL;

    self->protocol_name = NULL;
    self->peerid = NULL;
    self->tcp_messengers = NULL;
    self->unicast_server = NULL;
    self->multicast = NULL;
    self->public_addr = NULL;
    self->endpoint = NULL;

    return self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_tcp_destruct(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = PTValid(tp, Jxta_transport_tcp);

    if (self->mutex) {
        apr_thread_mutex_lock(self->mutex);
    }
    if (self->protocol_name != NULL) {
        free(self->protocol_name);
    }

    if (self->peerid != NULL) {
        free(self->peerid);
    }

    if (self->ipaddr != NULL) {
        free(self->ipaddr);
    }

    if (self->tcp_messengers != NULL) {
        JXTA_OBJECT_RELEASE(self->tcp_messengers);
    }

    if (self->public_addr != NULL) {
        JXTA_OBJECT_RELEASE(self->public_addr);
    }

    if (self->unicast_server != NULL) {
        JXTA_OBJECT_RELEASE(self->unicast_server);
    }

    if (self->multicast != NULL) {
        JXTA_OBJECT_RELEASE(self->multicast);
    }

    if (self->endpoint != NULL) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }

    apr_thread_mutex_destroy(self->mutex);

    if (self->pool) {
        apr_pool_destroy(self->pool);
    }

    self->thisType = NULL;

    jxta_transport_destruct((Jxta_transport *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction of [%p] finished\n", self);
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
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) calloc(1, sizeof(Jxta_transport_tcp));

    JXTA_OBJECT_INIT(self, tcp_free, NULL);

    return jxta_transport_tcp_construct(self, (Jxta_transport_tcp_methods const *) &jxta_transport_tcp_methods);;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp, Jxta_transport_tcp_connection * conn,
                                       Jxta_endpoint_address * dest)
{
    TcpMessenger *self;
    apr_status_t status;
    char *addr_buffer;
    Jxta_endpoint_address *src_addr;

    JXTA_OBJECT_CHECK_VALID(tp);

    /* create the object */
    self = (TcpMessenger *) calloc(1, sizeof(TcpMessenger));
    if (self == NULL)
        return NULL;

    /* setting */
    status = apr_pool_create(&self->pool, NULL);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger apr pool\n");
        free(self);
        return NULL;
    }

    status = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (APR_SUCCESS != status) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to create messenger mutex\n");
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->tp = tp;  /* NOTE: We do not share a reference for the transport */
    self->incoming = (NULL != conn);

    /* initialize it */
    JXTA_OBJECT_INIT(self, tcp_messenger_free, NULL);

    if (! self->incoming) {
        self->conn = jxta_transport_tcp_connection_new_1(tp, dest);
        if (NULL == self->conn) {
            addr_buffer = jxta_endpoint_address_get_transport_addr(dest);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed opening connection -> %s\n", addr_buffer);
            free(addr_buffer);
            apr_pool_destroy(self->pool);
            free(self);
            return NULL;
        }
    } else {
        self->conn = JXTA_OBJECT_SHARE(conn);
    }

    status = jxta_transport_tcp_connection_start(self->conn);
    if (JXTA_SUCCESS != status) {
        addr_buffer = jxta_endpoint_address_get_transport_addr(dest);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to start TCP connection with %s\n", addr_buffer);
        free(addr_buffer);
        JXTA_OBJECT_RELEASE(self->conn);
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    src_addr = publicaddr_get((Jxta_transport *) self->tp);
    addr_buffer = jxta_endpoint_address_to_string(src_addr);
    self->src_msg_el = jxta_message_element_new_2(MESSAGE_SOURCE_NS,
                                                  MESSAGE_SOURCE_NAME,
                                                  "text/plain", addr_buffer, strlen(addr_buffer), NULL);
    if (self->src_msg_el == NULL) {
        JXTA_OBJECT_RELEASE(self);
        return NULL;
    }
    free(addr_buffer);
    JXTA_OBJECT_RELEASE(src_addr);

    self->messenger.address = JXTA_OBJECT_SHARE(dest);
    self->messenger.jxta_send = tcp_messenger_send;

    return self;
}

static void tcp_messenger_free(Jxta_object * obj)
{
    TcpMessenger *self = (TcpMessenger *) obj;

    if (self->src_msg_el != NULL)
        JXTA_OBJECT_RELEASE(self->src_msg_el);

    if (NULL != self->messenger.address)
        JXTA_OBJECT_RELEASE(self->messenger.address);

    if (NULL != self->conn) {
        jxta_transport_tcp_connection_close(self->conn);
        JXTA_OBJECT_RELEASE(self->conn);
    }

    apr_thread_mutex_destroy(self->mutex);
    apr_pool_destroy(self->pool);

    memset(self, 0xDD, sizeof(TcpMessenger));
    free(self);
}

Jxta_boolean messenger_is_connected(TcpMessenger * self)
{
    if (NULL == self->conn) {
        return FALSE;
    }

    return jxta_transport_tcp_connection_is_connected(self->conn);
}

TcpMessenger *get_tcp_messenger(Jxta_transport_tcp * self, Jxta_transport_tcp_connection * conn, Jxta_endpoint_address * addr)
{
    TcpMessenger *messenger = NULL;
    char *key;
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(addr);

    key = jxta_endpoint_address_get_transport_addr(addr);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Get Messenger -> %s\n", key);

    apr_thread_mutex_lock(self->mutex);

    res = jxta_hashtable_get(self->tcp_messengers, key, strlen(key), (Jxta_object **) & messenger);

    if (NULL != messenger) {
        if (FALSE == messenger_is_connected(messenger)) {
            jxta_hashtable_del(self->tcp_messengers, key, strlen(key), NULL);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot remove TcpMessenger into vector\n");
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Invalid TcpMessenger [%p] removed.\n", messenger);
            }
            JXTA_OBJECT_RELEASE(messenger);
            messenger = NULL;
        }
    }

    if ((res != JXTA_SUCCESS) || (NULL == messenger)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Create new messenger -> %s\n", key);
        messenger = tcp_messenger_new(self, conn, addr);

        if (messenger != NULL) {
            jxta_hashtable_put(self->tcp_messengers, key, strlen(key), (Jxta_object *) messenger);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed getting messenger -> %s\n", key);
        }
    }

    apr_thread_mutex_unlock(self->mutex);

    if (NULL != messenger) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got Messenger [%p] -> %s\n", messenger, key);
    }

    free(key);
    return messenger;
}

Jxta_status jxta_transport_tcp_remove_messenger(Jxta_transport_tcp * self, Jxta_endpoint_address * addr)
{
    Jxta_status res;
    char *key;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(addr);

    key = jxta_endpoint_address_to_string(addr);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Remove messenger -> %s\n", key);

    apr_thread_mutex_lock(self->mutex);

    res = jxta_hashtable_del(self->tcp_messengers, key, strlen(key), NULL);

    apr_thread_mutex_unlock(self->mutex);

    free(key);
    return res;
}

static Jxta_status tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{
    Jxta_status status;
    TcpMessenger *self = (TcpMessenger *) mes;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(msg);

    apr_thread_mutex_lock(self->mutex);

    if (NULL == self->conn) {
        return JXTA_FAILED;
    } else {
        JXTA_OBJECT_CHECK_VALID(self->conn);
    }

    apr_thread_mutex_unlock(self->mutex);

    JXTA_OBJECT_SHARE(msg);

    jxta_message_remove_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME);
    jxta_message_add_element(msg, self->src_msg_el);

    status = jxta_transport_tcp_connection_send_message(self->conn, msg);

    JXTA_OBJECT_RELEASE(msg);

    return status;
}

Jxta_endpoint_service *jxta_transport_tcp_get_endpoint_service(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(self->endpoint);

    return JXTA_OBJECT_SHARE(self->endpoint);
}

const char *jxta_transport_tcp_local_ipaddr_cstr(Jxta_transport_tcp * me)
{
    if (NULL == me) {
        return NULL;
    }

    return me->ipaddr;
}

char *jxta_transport_tcp_get_local_ipaddr(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);

    return strdup(self->ipaddr);
}

apr_port_t jxta_transport_tcp_get_local_port(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);

    return self->port;
}

char *jxta_transport_tcp_get_peerid(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);

    return strdup(self->peerid);
}

Jxta_endpoint_address *jxta_transport_tcp_get_public_addr(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(self->public_addr);

    return self->public_addr;
}

Jxta_boolean jxta_transport_tcp_get_allow_multicast(Jxta_transport_tcp * self)
{
    JXTA_OBJECT_CHECK_VALID(self);

    return self->allow_multicast;
}

void jxta_tcp_got_inbound_connection(Jxta_transport_tcp * me, Jxta_transport_tcp_connection * conn, 
                                     Jxta_endpoint_address * addr)
{
    TcpMessenger * m = NULL;
    char * ip = NULL;

    ip = jxta_endpoint_address_get_transport_addr(addr);
    jxta_endpoint_service_transport_event(me->endpoint, JXTA_TRANSPORT_INBOUND_CONNECT, ip);
    free(ip);
    /* Add the connection to messenger table */
    m = get_tcp_messenger(me, conn, addr);
    if (NULL != m) {
        JXTA_OBJECT_RELEASE(m);
    }
}

/* vim: set sw=4 ts=4 tw=130 et: */
