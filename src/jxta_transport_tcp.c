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
 * $Id: jxta_transport_tcp.c,v 1.10 2005/02/22 21:10:57 bondolo Exp $
 */

#include "jstring.h"
#include "jdlist.h"
#include "jxta_types.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_peergroup.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_transport_private.h"
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

    JString *server_name;
    JString *protocol_name;
    char *ipaddr;
    apr_port_t port;
    Jxta_boolean allow_multicast;
    Jxta_boolean public_address_only;
    Jxta_boolean connected;
    IncomingUnicastServer *unicast_server;
    TcpMulticast *multicast;
    Jxta_endpoint_address *public_addr;
    Jxta_endpoint_service *endpoint;
    char *peerid;
    Jxta_vector *tcp_messengers;
    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
};

#define DEFAULT_PORT		9701

struct _tcp_messenger {
    struct _JxtaEndpointMessenger messenger;
    char *name;
    Jxta_endpoint_address *src_addr;
    Jxta_message_element *src_msg_el;
    Jxta_endpoint_address *dest_addr;
    Jxta_message_element *dest_msg_el;
    Jxta_boolean connected;
    Jxta_boolean incoming;
    Jxta_transport_tcp *tp;
    Jxta_transport_tcp_connection *conn;
    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
};

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, char *argv[]);
static void stop(Jxta_module * module);

static JString *name_get(Jxta_transport * self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * self);
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * t);
static Jxta_boolean allow_routing_p(Jxta_transport * t);
static Jxta_boolean connection_oriented_p(Jxta_transport * t);

/********************************************************************************/
/*                                                                              */
/********************************************************************************/
typedef Jxta_transport_methods Jxta_transport_tcp_methods;
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

void jxta_transport_tcp_construct(Jxta_transport_tcp * self, Jxta_transport_tcp_methods * methods);
void jxta_transport_tcp_destruct(Jxta_transport_tcp * self);
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp, Jxta_transport_tcp_connection * conn,
                                       Jxta_endpoint_address * dest, char *ipaddr, apr_port_t port);
static void tcp_free(Jxta_object * obj);
static void tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg);
static void tcp_messenger_free(Jxta_object * obj);

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static char *jxta_inet_ntop(Jxta_in_addr addr)
{
    char *str;
    union {
        Jxta_in_addr tmp;
        unsigned char addr[sizeof(Jxta_in_addr)];
    }
    un;
    /*un.tmp = htonl(addr); */
    un.tmp = addr;
    str = (char *) malloc(16);
    sprintf(str, "%u.%u.%u.%u", un.addr[0], un.addr[1], un.addr[2], un.addr[3]);
    return str;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_transport_tcp *self;
    apr_status_t status;
    Jxta_id *id;
    JString *Pid = NULL;
    char *tmp;
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    Jxta_vector *svcs;
    size_t sz, i;
    Jxta_TCPTransportAdvertisement *tta;
    Jxta_in_addr intf_addr;

    char *multicast_addr;
    apr_port_t multicast_port;
    int multicast_size;

#ifndef WIN32
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    self = (Jxta_transport_tcp *) module;
    PTValid(self, Jxta_transport_tcp);

    status = apr_pool_create(&self->pool, NULL);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        JXTA_LOG("Failed to create apr pool\n");
        return JXTA_NOMEM;
    }

    self->tcp_messengers = jxta_vector_new(0);
    if (self->tcp_messengers == NULL)
        return JXTA_NOMEM;

    status = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        JXTA_LOG("Failed to create mutex\n");
        return JXTA_FAILED;
    }

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

    if (svc == NULL)
        return JXTA_NOT_CONFIGURED;

    tta = jxta_svc_get_TCPTransportAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);
    if (tta == NULL)
        return JXTA_CONFIG_NOTFOUND;

    /* Protocol Name */
    self->protocol_name = jxta_TCPTransportAdvertisement_get_Protocol(tta);

    /* Server Name */
    self->server_name = jxta_TCPTransportAdvertisement_get_Server(tta);

    /* port */
    self->port = (apr_port_t) jxta_TCPTransportAdvertisement_get_Port(tta);

    /* Config Mode */
    /* Interface Address */
    if (strcmp(jxta_TCPTransportAdvertisement_get_ConfigMode_string(tta), "auto") == 0) {
        char *local_host = (char *) malloc(APRMAXHOSTLEN);
        apr_pool_t *pool;
        apr_sockaddr_t *localsa;

        self->ipaddr = strdup(APR_ANYADDR);

        apr_pool_create(&pool, NULL);

        apr_gethostname(local_host, APRMAXHOSTLEN, pool);
        JXTA_LOG( "TCP Transport local hostname = %s\n ",local_host );        
        
        apr_sockaddr_info_get(&localsa, local_host, APR_INET, 0, 0, pool);
        free(local_host);
 
        apr_sockaddr_ip_get(&local_host, localsa);
        
        /* Public Address */
        tmp = (char *) malloc(strlen(self->ipaddr) + 20);
        sprintf(tmp, "tcp://%s:%d/", local_host, self->port);
        self->public_addr = jxta_endpoint_address_new(tmp);
        apr_pool_destroy(pool);
        free(tmp);
    } else {                    /* ConfigMode == manual */
        intf_addr = jxta_TCPTransportAdvertisement_get_InterfaceAddress(tta);
        self->ipaddr = jxta_inet_ntop(intf_addr);

        /* Public Address */
        tmp = (char *) malloc(strlen(self->ipaddr) + 20);
        sprintf(tmp, "tcp://%s:%d/", self->ipaddr, self->port);
        self->public_addr = jxta_endpoint_address_new(tmp);
        free(tmp);
    }

    JXTA_LOG( "TCP Transport interface addr = %s\n ", self->ipaddr );
    JXTA_LOG( "TCP Transport public addr = %s://%s\n ",
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

    if (self->peerid == NULL)
        return JXTA_NOMEM;

    JXTA_OBJECT_RELEASE(Pid);
    JXTA_OBJECT_RELEASE(id);

    /* Server Enabled */
    if (!jxta_TCPTransportAdvertisement_get_ServerOff(tta)) {
        self->unicast_server = jxta_incoming_unicast_server_new(self, self->ipaddr, self->port);
        if (self->unicast_server == NULL) {
            JXTA_LOG("Failed new incoming unicast server\n");
            return JXTA_FAILED;
        }
        if (!jxta_incoming_unicast_server_start(self->unicast_server)) {
            JXTA_LOG("Failed to start incoming unicast server\n");
            JXTA_OBJECT_RELEASE(self->unicast_server);
            self->unicast_server = NULL;
            return JXTA_FAILED;
        }
    }

    /* Multicast start */
    if (self->allow_multicast == TRUE) {
        self->multicast = tcp_multicast_new(self, multicast_addr, multicast_port, multicast_size);
        if (self->multicast == NULL) {
            JXTA_LOG("Failed new multicast server\n");
            return JXTA_FAILED;
        }
        if (!tcp_multicast_start(self->multicast)) {
            JXTA_LOG("Failed to start multicast server\n");
            JXTA_OBJECT_RELEASE(self->multicast);
            self->multicast = NULL;
            return JXTA_FAILED;
        }
    }

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    /*
       apr_thread_yield();
     */

    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_status start(Jxta_module * module, char *argv[])
{
    PTValid(module, Jxta_transport_tcp);
    return JXTA_SUCCESS;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void stop(Jxta_module * module)
{
    PTValid(module, Jxta_transport_tcp);
    JXTA_LOG("Don't know how to stop yet.\n");
}


/**************************
 * Tcp Transport methods *
 *************************/

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JString *name_get(Jxta_transport * t)
{
    PTValid(t, Jxta_transport_tcp);

    return jstring_new_2("tcp");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * t)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;
    PTValid(self, Jxta_transport_tcp);

    JXTA_OBJECT_SHARE(self->public_addr);
    return self->public_addr;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static JxtaEndpointMessenger *messenger_get(Jxta_transport * t, Jxta_endpoint_address * dest)
{
    JxtaEndpointMessenger *messenger;
    char *protocol_address;
    char *host;
    char *colon;
    int port = DEFAULT_PORT;

    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;
    PTValid(self, Jxta_transport_tcp);
    protocol_address = jxta_endpoint_address_get_protocol_address(dest);

    colon = strchr(protocol_address, ':');
    if (colon && (*(colon + 1))) {
        port = atoi(colon + 1);
        host = (char *) malloc(colon - protocol_address + 1);
        host[0] = 0;
        strncat(host, protocol_address, colon - protocol_address);
    } else {
        host = strdup(protocol_address);
    }

    JXTA_LOG("tcp_messenger: host=%s, port=%d\n", host, port);

    messenger = (JxtaEndpointMessenger *) get_tcp_messenger(self, NULL, dest, host, port);
    if (messenger != NULL) {
        JXTA_OBJECT_SHARE(dest);
        messenger->address = dest;
    }
    free(host);
    return messenger;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr)
{
    PTValid(t, Jxta_transport_tcp);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;

    if (self->allow_multicast == TRUE) {
        jxta_message_set_source(msg, self->public_addr);
        tcp_multicast_propagate(self->multicast, msg, service_name, service_params);
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_overload_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;
    PTValid(self, Jxta_transport_tcp);
    return FALSE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean allow_routing_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;
    PTValid(self, Jxta_transport_tcp);
    return TRUE;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static Jxta_boolean connection_oriented_p(Jxta_transport * t)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) t;
    PTValid(self, Jxta_transport_tcp);
    return TRUE;
}


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_tcp_construct(Jxta_transport_tcp * tp, Jxta_transport_tcp_methods * methods)
{
    Jxta_transport_tcp *self = tp;

    PTValid(methods, Jxta_transport_methods);
    jxta_transport_construct((Jxta_transport *) self, (Jxta_transport_methods *) methods);

    self->thisType = "Jxta_transport_tcp";
    self->protocol_name = NULL;
    self->server_name = NULL;
    self->peerid = NULL;
    self->tcp_messengers = NULL;
    self->unicast_server = NULL;
    self->multicast = NULL;
    self->public_addr = NULL;
    self->endpoint = NULL;
    self->mutex = NULL;
    self->pool = NULL;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
void jxta_transport_tcp_destruct(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;
    PTValid(self, Jxta_transport_tcp);

    if (self->protocol_name != NULL)
        JXTA_OBJECT_RELEASE(self->protocol_name);
    if (self->peerid != NULL)
        free(self->peerid);
    if (self->server_name != NULL)
        JXTA_OBJECT_RELEASE(self->server_name);
    if (self->ipaddr != NULL)
        free(self->ipaddr);
    if (self->tcp_messengers != NULL)
        JXTA_OBJECT_RELEASE(self->tcp_messengers);
    if (self->public_addr != NULL)
        JXTA_OBJECT_RELEASE(self->public_addr);
    if (self->unicast_server != NULL)
        JXTA_OBJECT_RELEASE(self->unicast_server);
    if (self->multicast != NULL)
        JXTA_OBJECT_RELEASE(self->multicast);
    if (self->endpoint != NULL)
        JXTA_OBJECT_RELEASE(self->endpoint);
    apr_thread_mutex_lock(self->mutex);
    if (self->pool)
        apr_pool_destroy(self->pool);

    jxta_transport_destruct((Jxta_transport *) self);

    JXTA_LOG("Destruction finished\n");
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static void tcp_free(Jxta_object * obj)
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) obj;
    jxta_transport_tcp_destruct(self);
    free(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
Jxta_transport_tcp *jxta_transport_tcp_new_instance()
{
    Jxta_transport_tcp *self = (Jxta_transport_tcp *) malloc(sizeof(Jxta_transport_tcp));
    memset(self, 0, sizeof(Jxta_transport_tcp));
    JXTA_OBJECT_INIT(self, tcp_free, NULL);
    jxta_transport_tcp_construct(self, (Jxta_transport_tcp_methods *) & jxta_transport_tcp_methods);

    return self;
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/


/******************************************************************************/
/*                                                                            */
/******************************************************************************/
static TcpMessenger *tcp_messenger_new(Jxta_transport_tcp * tp, Jxta_transport_tcp_connection * conn,
                                       Jxta_endpoint_address * dest, char *ipaddr, apr_port_t port)
{
    TcpMessenger *self;
    apr_status_t status;
    char *src_addr_buffer;
    char *dest_addr_buffer;

    JXTA_OBJECT_CHECK_VALID(tp);

    /* create the object */
    self = (TcpMessenger *) malloc(sizeof(TcpMessenger));
    if (self == NULL)
        return NULL;

    /* initialize it */
    memset(self, 0, sizeof(TcpMessenger));
    JXTA_OBJECT_INIT(self, tcp_messenger_free, NULL);

    /* setting */
    status = apr_pool_create(&self->pool, NULL);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        JXTA_LOG("Failed to create messenger apr pool\n");
        free(self);
        return NULL;
    }

    status = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);
    if (!APR_STATUS_IS_SUCCESS(status)) {
        JXTA_LOG("Failed to create messenger mutex\n");
        apr_pool_destroy(self->pool);
        free(self);
        return NULL;
    }

    self->tp = tp;
    self->conn = conn;

    if (self->conn == NULL) {
        self->conn = jxta_transport_tcp_connection_new_1(self->tp, dest, ipaddr, port);

        if (self->conn != NULL) {
            JXTA_OBJECT_CHECK_VALID(self->conn);
        } else {
            JXTA_LOG("Connection not established\n");
            apr_pool_destroy(self->pool);
            free(self);
            return NULL;
        }
        self->incoming = FALSE;
    } else {
        self->incoming = TRUE;
    }

    self->src_addr = publicaddr_get((Jxta_transport *) self->tp);
    src_addr_buffer = jxta_endpoint_address_to_string(self->src_addr);
    self->src_msg_el = jxta_message_element_new_2(MESSAGE_SOURCE_NS,
                                                  MESSAGE_SOURCE_NAME,
                                                  "text/plain", src_addr_buffer, strlen(src_addr_buffer), NULL);
    if (self->src_msg_el == NULL) {
        return NULL;
    }
    free(src_addr_buffer);

    self->name = (char *) malloc(strlen(ipaddr) + 32);  /* to enough memory */
    sprintf(self->name, "tcp://%s:%d", ipaddr, port);

    self->dest_addr = jxta_endpoint_address_new(self->name);
    dest_addr_buffer = jxta_endpoint_address_to_string(self->dest_addr);
    self->dest_msg_el = jxta_message_element_new_2(MESSAGE_DESTINATION_NS,
                                                   MESSAGE_DESTINATION_NAME,
                                                   "text/plain", dest_addr_buffer, strlen(dest_addr_buffer), NULL);
    if (self->dest_msg_el == NULL) {
        return NULL;
    }
    free(dest_addr_buffer);

    self->connected = FALSE;
    self->messenger.jxta_send = tcp_messenger_send;

    return self;
}

static void tcp_messenger_free(Jxta_object * obj)
{
    TcpMessenger *self = (TcpMessenger *) obj;

    if (self == NULL)
        return;

    if (self->src_addr != NULL)
        JXTA_OBJECT_RELEASE(self->src_addr);

    if (self->dest_addr != NULL)
        JXTA_OBJECT_RELEASE(self->dest_addr);

    if (self->name != NULL)
        free(self->name);

    if (JXTA_OBJECT_CHECK_VALID(self->messenger.address))
        JXTA_OBJECT_RELEASE(self->messenger.address);

    apr_thread_mutex_lock(self->mutex);

    apr_pool_destroy(self->pool);

    free(self);
}

Jxta_boolean tcp_messenger_start(TcpMessenger * mes)
{
    TcpMessenger *self = mes;
    JXTA_OBJECT_CHECK_VALID(self);
    if (self->conn == NULL)
        return FALSE;

    jxta_transport_tcp_connection_start(self->conn);
    return TRUE;
}

TcpMessenger *get_tcp_messenger(Jxta_transport_tcp * tp, Jxta_transport_tcp_connection * conn, Jxta_endpoint_address * addr,
                                char *ipaddr, apr_port_t port)
{
    TcpMessenger *messenger;
    char *key;
    int i, nbOfMessengers;
    Jxta_boolean found = FALSE;
    Jxta_status res;

    if ((tp == NULL) || (ipaddr == NULL)) {
        return NULL;
    }

    key = (char *) malloc(1024);
    sprintf(key, "tcp://%s:%d", ipaddr, port);
    apr_thread_mutex_lock(tp->mutex);

    nbOfMessengers = jxta_vector_size(tp->tcp_messengers);
    for (i = 0; i < nbOfMessengers; i++) {
        res = jxta_vector_get_object_at(tp->tcp_messengers, (Jxta_object **) & messenger, i);
        if (res != JXTA_SUCCESS)
            continue;
        if (strcmp(messenger->name, key) == 0) {
            found = TRUE;
            break;
        }
        JXTA_OBJECT_RELEASE(messenger);
    }

    if (found == FALSE) {
        messenger = tcp_messenger_new(tp, conn, addr, ipaddr, port);

        if (messenger != NULL) {
            jxta_transport_tcp_connection_start(messenger->conn);
            res = jxta_vector_add_object_first(tp->tcp_messengers, (Jxta_object *) messenger);
            if (res != JXTA_SUCCESS)
                JXTA_LOG("Cannot insert TcpMessenger into vector\n");
        }

    }

    apr_thread_mutex_unlock(tp->mutex);
    free(key);
    return messenger;
}

static void tcp_messenger_send(JxtaEndpointMessenger * mes, Jxta_message * msg)
{
    TcpMessenger *self = (TcpMessenger *) mes;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(self->conn);
    JXTA_OBJECT_CHECK_VALID(msg);

    JXTA_OBJECT_SHARE(msg);

    (void) jxta_message_remove_element_2(msg, MESSAGE_SOURCE_NS, MESSAGE_SOURCE_NAME);
    jxta_message_add_element(msg, self->src_msg_el);

    jxta_transport_tcp_connection_send_message(self->conn, msg);

    JXTA_OBJECT_RELEASE(msg);
}

Jxta_endpoint_service *jxta_transport_tcp_get_endpoint_service(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(self->endpoint);

    return self->endpoint;
}

char *jxta_transport_tcp_get_local_ipaddr(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);

    return strdup(self->ipaddr);
}

apr_port_t jxta_transport_tcp_get_local_port(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);

    return self->port;
}

char *jxta_transport_tcp_get_peerid(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);

    return strdup(self->peerid);
}

Jxta_endpoint_address *jxta_transport_tcp_get_public_addr(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);
    JXTA_OBJECT_CHECK_VALID(self->public_addr);

    return self->public_addr;
}

Jxta_boolean jxta_transport_tcp_get_allow_multicast(Jxta_transport_tcp * tp)
{
    Jxta_transport_tcp *self = tp;

    JXTA_OBJECT_CHECK_VALID(self);

    return self->allow_multicast;
}

Jxta_status jxta_transport_tcp_remove_messenger(Jxta_transport_tcp * tp, char *ipaddr, apr_port_t port)
{
    Jxta_status res;
    TcpMessenger *messenger;
    char *key;
    int i, nbOfMessengers;
    Jxta_boolean found = FALSE;

    JXTA_OBJECT_CHECK_VALID(tp);
    key = (char *) malloc(1024);

    sprintf(key, "tcp://%s:%d", ipaddr, port);
    apr_thread_mutex_lock(tp->mutex);

    nbOfMessengers = jxta_vector_size(tp->tcp_messengers);
    for (i = 0; i < nbOfMessengers; i++) {
        res = jxta_vector_get_object_at(tp->tcp_messengers, (Jxta_object **) & messenger, i);
        if (res != JXTA_SUCCESS)
            continue;
        if (strcmp(messenger->name, key) == 0) {
            found = TRUE;
            break;
        }
        JXTA_OBJECT_RELEASE(messenger);
    }

    if (found == TRUE) {
        res = jxta_vector_remove_object_at(tp->tcp_messengers, (Jxta_object **) & messenger, i);
        if (res != JXTA_SUCCESS)
            JXTA_LOG("Cannot remove TcpMessenger into vector\n");
        JXTA_OBJECT_RELEASE(messenger);
    }

    apr_thread_mutex_unlock(tp->mutex);

    if (found == FALSE)
        return JXTA_FAILED;

    return res;
}
