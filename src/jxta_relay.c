/*
 * Copyright (c) 2002-2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_relay.c,v 1.54 2006/05/23 17:37:17 slowhog Exp $
 */
#include <stdlib.h> /* for atoi */

#ifndef WIN32
#include <signal.h> /* for sigaction */
#endif

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_types.h"
#include "jxta_log.h"
#include "jxta_errno.h"
#include "jstring.h"
#include "jxta_transport_private.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_relaya.h"
#include "jxta_relay.h"
#include "jxta_rdv.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_vector.h"
#include "jxta_object_type.h"
#include "jxta_peer.h"
#include "jxta_peer_private.h"
#include "jxta_endpoint_service_priv.h"

static const char *__log_cat = "RELAY";

/**
 ** Renewal of the leases must be done before the lease expires. The following
 ** constant defines the number of microseconds to substract to the expiration
 ** time in order to set the time of renewal.
 **/
#define RELAY_RENEWAL_DELAY ((Jxta_time_diff) (5 * 60 * 1000))  /* 5 Minutes */

/**
 ** Time the background thread that regulary checks the peer will wait
 ** between two iteration.
 **/

#define RELAY_THREAD_NAP_TIME_NORMAL (1 * 60 * 1000 * 1000) /* 1 Minute */
#define RELAY_THREAD_NAP_TIME_FAST (2 * 1000 * 1000)    /* 2 Seconds */

/**
 ** Minimal number of relays the peers need to be connected to.
 ** If there is fewer connected relay, the Relay Service
 ** becomes more aggressive trying to connect to a realy.
 **/

#define MIN_NB_OF_CONNECTED_RELAYS 1

/**
 ** Maximum time between two connection attempts to the same peer.
 **/

#define RELAY_MAX_RETRY_DELAY (30 * 60 * 1000)  /* 30 Minutes */

/**
 ** Minimum delay between two connection attempts to the same peer.
 **/

#define RELAY_MIN_RETRY_DELAY (3 * 1000)    /* 3 Seconds */

/**
 ** When the number of connected peer is less than MIN_NB_OF_CONNECTED_PEERS
 ** the Relay Service tries to connect to peers earlier than planned
 ** if there were scheduled for more than LONG_RETRY_DELAY
 **/

#define RELAY_LONG_RETRY_DELAY (RELAY_MAX_RETRY_DELAY / 2)

typedef Jxta_transport Jxta_transport_relay;

/*
 * _jxta_transport_relay Structure  
 */
struct _jxta_transport_relay {
    Extends(Jxta_transport);

    apr_thread_cond_t *stop_cond;
    apr_thread_mutex_t *stop_mutex;
    apr_pool_t *pool;

    Jxta_endpoint_address *address;
    char *assigned_id;
    char *peerid;
    Jxta_PG *group;
    char *groupid;
    Jxta_boolean Is_Client;
    Jxta_boolean Is_Server;
    Jxta_vector *HttpRelays;
    Jxta_vector *TcpRelays;
    Jxta_vector *peers;
    Jxta_endpoint_service *endpoint;
    apr_thread_t *thread;
    volatile Jxta_boolean running;

    void *ep_cookie;
};

typedef struct _jxta_transport_relay _jxta_transport_relay;

/*********************************************************************
 ** Internal structure of a peer used by this service.
 *********************************************************************/

struct _jxta_peer_relay_entry {
    Extends(_jxta_peer_entry);

    Jxta_boolean is_connected;
    Jxta_boolean try_connect;
    Jxta_time connectTime;
    Jxta_time connectRetryDelay;
};

typedef struct _jxta_peer_relay_entry _jxta_peer_relay_entry;

typedef struct Jxta_Relay_entry_methods {
    Extends(Jxta_Peer_entry_methods);

    /* additional methods */
} Jxta_Relay_entry_methods;

/*
 * Relay transport methods
 */
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, const char *argv[]);
static void stop(Jxta_module * module);

static JString *name_get(Jxta_transport * self);
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * self);
static Jxta_boolean ping(Jxta_transport * t, Jxta_endpoint_address * addr);
static void propagate(Jxta_transport * t, Jxta_message * msg, const char *service_name, const char *service_params);
static Jxta_boolean allow_overload_p(Jxta_transport * tr);
static Jxta_boolean allow_routing_p(Jxta_transport * tr);
static Jxta_boolean connection_oriented_p(Jxta_transport * tr);

static Jxta_status JXTA_STDCALL relay_transport_client_cb(Jxta_object * obj, void *arg);
static void *APR_THREAD_FUNC connect_relay_thread(apr_thread_t * thread, void *arg);
static void check_relay_lease(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void connect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void reconnect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer);
static void check_relay_connect(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer, int nbOfConnectedPeers);
static void process_connected_reply(_jxta_transport_relay * self, Jxta_message * msg);
static void process_disconnected_reply(_jxta_transport_relay * self, Jxta_message * msg);
static void update_relay_peer_connection(_jxta_transport_relay * self, Jxta_RdvAdvertisement * relay, Jxta_time_diff lease);
static _jxta_peer_relay_entry *relay_entry_construct(_jxta_peer_relay_entry * self);
static void relay_entry_destruct(_jxta_peer_relay_entry * self);

static const Jxta_Relay_entry_methods JXTA_RELAY_ENTRY_METHODS = {
    {
     "Jxta_Peer_entry_methods",
     jxta_peer_lock,
     jxta_peer_unlock,
     jxta_peer_get_peerid,
     jxta_peer_set_peerid,
     jxta_peer_get_address,
     jxta_peer_set_address,
     jxta_peer_get_adv,
     jxta_peer_set_adv,
     jxta_peer_get_expires,
     jxta_peer_set_expires},
    "Jxta_Relay_entry_methods"
};

#define RELAY_ENTRY_VTBL(self) ((Jxta_Relay_entry_methods*) ((_jxta_peer_entry*) (self))->methods)

typedef Jxta_transport_methods Jxta_transport_relay_methods;

static const Jxta_transport_relay_methods JXTA_TRANSPORT_RELAY_METHODS = {
    {
     "Jxta_module_methods",
     init,
     jxta_module_init_e_impl,
     start,
     stop},
    "Jxta_transport_methods",
    name_get,
    publicaddr_get,
    NULL,
    ping,
    propagate,
    allow_overload_p,
    allow_routing_p,
    connection_oriented_p
};

static void relay_entry_delete(Jxta_object * addr)
{
    _jxta_peer_relay_entry *self = (_jxta_peer_relay_entry *) addr;

    relay_entry_destruct(self);

    free(self);
}

static _jxta_peer_relay_entry *relay_entry_construct(_jxta_peer_relay_entry * self)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Constructing ...\n");

    self = (_jxta_peer_relay_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        self->thisType = "_jxta_peer_relay_entry";
        self->is_connected = FALSE;
        self->try_connect = FALSE;
        self->connectTime = 0;
        self->connectRetryDelay = 0;
    }

    return self;
}

static void relay_entry_destruct(_jxta_peer_relay_entry * self)
{
    peer_entry_destruct((_jxta_peer_entry *) self);
}

static _jxta_peer_relay_entry *relay_entry_new(void)
{
    _jxta_peer_relay_entry *self = (_jxta_peer_relay_entry *) malloc(sizeof(_jxta_peer_relay_entry));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(_jxta_peer_relay_entry));
    JXTA_OBJECT_INIT(self, relay_entry_delete, 0);
    PEER_ENTRY_VTBL(self) = (Jxta_Peer_entry_methods *) & JXTA_RELAY_ENTRY_METHODS;

    return relay_entry_construct(self);
}

/*
 * Init the Relay service
 */
static Jxta_status init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    apr_status_t res;
    char *address_str;
    Jxta_id *id;
    JString *uniquePid;
    Jxta_id *pgid;
    JString *uniquePGid;
    const char *tmp;
    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);
    Jxta_PA *conf_adv = NULL;
    Jxta_svc *svc = NULL;
    JString *val = NULL;
    Jxta_vector *relays = NULL;
    JString *relay = NULL;
    JString *mid = NULL;

    size_t sz;
    size_t i;
    Jxta_RelayAdvertisement *rla;

#ifndef WIN32
    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
#endif

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Initializing ...\n");

    /* Allocate a pool for our apr needs */
    res = apr_pool_create(&self->pool, NULL);
    if (res != APR_SUCCESS)
        return res;

    /*
     * Create our mutex.
     */
    res = apr_thread_mutex_create(&(self->stop_mutex), APR_THREAD_MUTEX_DEFAULT, self->pool);
    if (res != APR_SUCCESS)
        return res;

    res = apr_thread_cond_create(&(self->stop_cond), self->pool);
    if (res != APR_SUCCESS) {
        return res;
    }

    /*
     * following falls-back on backdoor config if needed only.
     * So it can be just be removed when nolonger usefull.
     */
    self->Is_Client = FALSE;
    self->Is_Server = FALSE;

    /*
     * Extract our configuration for the config adv.
     */
    jxta_PG_get_configadv(group, &conf_adv);
    if (conf_adv == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    jxta_PA_get_Svc_with_id(conf_adv, assigned_id, &svc);
    JXTA_OBJECT_RELEASE(conf_adv);

    if (svc == NULL)
        return JXTA_NOT_CONFIGURED;

    rla = jxta_svc_get_RelayAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);

    if (rla == NULL) {
        return JXTA_CONFIG_NOTFOUND;
    }

    /*
     * FIXME - tra@jxta.org 20040121 : for now, we ignore the following
     * relay config items: 
     *          <ServerMaximumClients>
     *          <ServerLeaseInSeconds>
     *          <ClientMaximumServers>
     *          <ClientLeaseInSeconds>
     *          <ClientQueueSize>
     */

    val = jxta_RelayAdvertisement_get_IsServer(rla);
    if (strcmp(jstring_get_string(val), "true") == 0) {
        self->Is_Server = TRUE;
    } else {
        self->Is_Server = FALSE;
    }
    JXTA_OBJECT_RELEASE(val);

    val = jxta_RelayAdvertisement_get_IsClient(rla);
    if (strcmp(jstring_get_string(val), "true") == 0) {
        self->Is_Client = TRUE;
    } else {
        self->Is_Client = FALSE;
    }
    JXTA_OBJECT_RELEASE(val);

    relays = jxta_RelayAdvertisement_get_HttpRelay(rla);
    if (jxta_vector_clone(relays, &(self->HttpRelays), 0, 20) != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(rla);
        return JXTA_CONFIG_NOTFOUND;
    }
    JXTA_OBJECT_RELEASE(relays);

    relays = jxta_RelayAdvertisement_get_TcpRelay(rla);
    JXTA_OBJECT_RELEASE(rla);
    if (jxta_vector_clone(relays, &(self->TcpRelays), 0, 20) != JXTA_SUCCESS) {
        return JXTA_CONFIG_NOTFOUND;
    }
    JXTA_OBJECT_RELEASE(relays);

    jxta_PG_get_endpoint_service(group, &(self->endpoint));
    self->group = group;

    /*
     * Only the unique portion of the peerid is used by the relay Transport.
     */
    jxta_PG_get_PID(group, &id);
    uniquePid = NULL;
    jxta_id_get_uniqueportion(id, &uniquePid);
    tmp = jstring_get_string(uniquePid);
    self->peerid = malloc(strlen(tmp) + 1);
    if (self->peerid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(id);
        JXTA_OBJECT_RELEASE(uniquePid);
        return JXTA_NOMEM;
    }
    strcpy(self->peerid, tmp);
    JXTA_OBJECT_RELEASE(uniquePid);
    JXTA_OBJECT_RELEASE(id);

    /*
     * get the unique portion of the group id
     */
    jxta_PG_get_GID(group, &pgid);
    jxta_id_get_uniqueportion(pgid, &uniquePGid);
    tmp = jstring_get_string(uniquePGid);
    self->groupid = malloc(strlen(tmp) + 1);
    if (self->groupid == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(uniquePGid);
        JXTA_OBJECT_RELEASE(pgid);
        return JXTA_NOMEM;
    }
    strcpy(self->groupid, tmp);
    JXTA_OBJECT_RELEASE(uniquePGid);
    JXTA_OBJECT_RELEASE(pgid);

    /*
     * Save our assigned service id
     */
    jxta_id_get_uniqueportion(assigned_id, &mid);
    self->assigned_id = malloc(strlen(jstring_get_string(mid)) + 1);
    if (self->assigned_id == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(mid);
        return JXTA_NOMEM;
    }
    strcpy(self->assigned_id, jstring_get_string(mid));
    JXTA_OBJECT_RELEASE(mid);

    address_str = apr_psprintf(self->pool, "relay://%s", self->peerid);

    self->address = jxta_endpoint_address_new(address_str);

    if (self->address == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not generate valid local address");
        return JXTA_NOMEM;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay Service Configuration:\n");
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      Address=%s://%s\n",
                    jxta_endpoint_address_get_protocol_name(self->address),
                    jxta_endpoint_address_get_protocol_address(self->address));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      IsClient=%d\n", self->Is_Client);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "      IsServer=%d\n", self->Is_Server);

    sz = jxta_vector_size(self->HttpRelays);
    for (i = 0; i < sz; ++i) {
        jxta_vector_get_object_at(self->HttpRelays, JXTA_OBJECT_PPTR(&relay), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "http://%s\n", jstring_get_string(relay));
        JXTA_OBJECT_RELEASE(relay);
    }

    sz = jxta_vector_size(self->TcpRelays);
    for (i = 0; i < sz; ++i) {
        jxta_vector_get_object_at(self->TcpRelays, JXTA_OBJECT_PPTR(&relay), i);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "tcp://%s\n", jstring_get_string(relay));
        JXTA_OBJECT_RELEASE(relay);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Initialized\n");
    return JXTA_SUCCESS;
}

/*
 * Start Module
 */
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);
    Jxta_status rv;

    if (self->Is_Server) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Relay server is not supported yet!\n");
    }

    /*
     * Check if the relay service is configured as a client, if not
     * just skip starting the service
     */
    if (!self->Is_Client) {
        return JXTA_SUCCESS;
    }

    jxta_endpoint_service_add_transport(self->endpoint, (Jxta_transport *) self);

    /* 
     *  we need to register the relay endpoint callback
     */
    rv = endpoint_service_add_recipient(self->endpoint, &self->ep_cookie, self->assigned_id, NULL, relay_transport_client_cb,
                                        self);

    /*
     * Go ahead an start the Relay connect thread
     */
    apr_thread_create(&self->thread, NULL,  /* no attr */
                      connect_relay_thread, (void *) self, (apr_pool_t *) self->pool);

    return JXTA_SUCCESS;
}

/*
 * Stop Module         
 */
static void stop(Jxta_module * module)
{
    apr_status_t status;

    _jxta_transport_relay *self = PTValid(module, _jxta_transport_relay);

    if (!self->Is_Client) {
        return;
    }

    endpoint_service_remove_recipient(self->endpoint, self->ep_cookie);
    jxta_endpoint_service_remove_transport(self->endpoint, (Jxta_transport *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Signal thread to exit...\n");
    apr_thread_mutex_lock(self->stop_mutex);
    self->running = FALSE;
    apr_thread_cond_signal(self->stop_cond);
    apr_thread_mutex_unlock(self->stop_mutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Waiting thread to exit...\n");
    apr_thread_join(&status, self->thread);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay stopped.\n");
}

/*
 * Relay address
 */
static JString *name_get(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return jstring_new_2("relay");
}

/*
 * get Address
 */
static Jxta_endpoint_address *publicaddr_get(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    JXTA_OBJECT_SHARE(self->address);
    return self->address;
}

/*
 * Ping
 */
static Jxta_boolean ping(Jxta_transport * relay, Jxta_endpoint_address * addr)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    /* FIXME 20050119 If the address requested is the relay we should return true */

    return FALSE;
}

/*
 * Propagate
 */
static void propagate(Jxta_transport * relay, Jxta_message * msg, const char *service_name, const char *service_params)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    /* this code intentionally left blank */
}

/*
 *  Allow overload 
 */
static Jxta_boolean allow_overload_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return FALSE;
}

/*
 * Allow routing    
 */
static Jxta_boolean allow_routing_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return TRUE;
}

/*
 * Connection oriented
 */
static Jxta_boolean connection_oriented_p(Jxta_transport * relay)
{
    _jxta_transport_relay *self = PTValid(relay, _jxta_transport_relay);

    return TRUE;
}

/*
 * Construct relay service
 */
_jxta_transport_relay *jxta_transport_relay_construct(Jxta_transport * relay, Jxta_transport_relay_methods const *methods)
{
    _jxta_transport_relay *self = NULL;

    PTValid(methods, Jxta_transport_methods);
    self = (_jxta_transport_relay *) jxta_transport_construct(relay, (Jxta_transport_methods const *) methods);
    self->_super.direction = JXTA_OUTBOUND;

    if (NULL != self) {
        self->thisType = "_jxta_transport_relay";
        self->address = NULL;
        self->peerid = NULL;
        self->groupid = NULL;
        self->endpoint = NULL;
        self->group = NULL;
        self->stop_mutex = NULL;
        self->stop_cond = NULL;
        self->pool = NULL;
        self->HttpRelays = NULL;
        self->TcpRelays = NULL;
        self->peers = jxta_vector_new(1);
        if (self->peers == NULL) {
            return NULL;
        }
    }

    return self;
}

/*
 * The destructor is never called explicitly (except by subclass' code).
 * It is called by the free function (installed by the allocator) when the object
 * becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_transport_relay_destruct(_jxta_transport_relay * self)
{
    if (self->HttpRelays != NULL) {
        JXTA_OBJECT_RELEASE(self->HttpRelays);
    }
    if (self->TcpRelays != NULL) {
        JXTA_OBJECT_RELEASE(self->TcpRelays);
    }

    JXTA_OBJECT_RELEASE(self->peers);

    self->group = NULL;

    if (self->address != NULL) {
        JXTA_OBJECT_RELEASE(self->address);
    }
    if (self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }
    if (self->assigned_id) {
        free(self->assigned_id);
    }
    if (self->peerid != NULL) {
        free(self->peerid);
    }
    if (self->groupid != NULL) {
        free(self->groupid);
    }

    apr_thread_cond_destroy(self->stop_cond);
    apr_thread_mutex_destroy(self->stop_mutex);

    if (self->pool) {
        apr_pool_destroy(self->pool);
    }

    jxta_transport_destruct((Jxta_transport *) self);

    self->thisType = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/*
 * De-instantiate an instance of the Relay transport   
 */
static void relay_free(Jxta_object * obj)
{
    jxta_transport_relay_destruct((_jxta_transport_relay *) obj);

    free(obj);
}

/*
 * Instantiate a new instance of the Relay transport
 */
Jxta_transport_relay *jxta_transport_relay_new_instance(void)
{
    _jxta_transport_relay *self = (_jxta_transport_relay *) malloc(sizeof(_jxta_transport_relay));
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }
    memset(self, 0, sizeof(_jxta_transport_relay));
    JXTA_OBJECT_INIT(self, relay_free, NULL);
    if (!jxta_transport_relay_construct((Jxta_transport *) self, &JXTA_TRANSPORT_RELAY_METHODS)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return NULL;
    }

    return (Jxta_transport_relay *) self;
}

/**
 ** This is the generic message handler of the JXTA Relay Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - get the lease from the relay server.
 **   - get a disconnect from the relay server.
 **/
static Jxta_status JXTA_STDCALL relay_transport_client_cb(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_transport_relay *self = PTValid(arg, _jxta_transport_relay);
    Jxta_message_element *el = NULL;
    JString *response = NULL;
    Jxta_bytevector *bytes = NULL;
    char *res;
    _jxta_peer_relay_entry *peer = NULL;
    unsigned int i = 0;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_boolean connected = FALSE;

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(self);

    for (i = 0; i < jxta_vector_size(self->peers); ++i) {
        status = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
        if (status != JXTA_SUCCESS) {
            /* We should print a LOG here. Just continue for the time being */
            continue;
        }

        if (peer->is_connected) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Drop Relay lease response as we already have a Relay connection\n");
            connected = TRUE;
        } else {
            peer->connectTime = 0;
            peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
        }
        JXTA_OBJECT_RELEASE(peer);
    }
    /*
     * we are already connected
     */
    if (connected) {
        return JXTA_SUCCESS;
    }

    jxta_message_get_element_1(msg, RELAY_LEASE_RESPONSE, &el);

    if (el) {
        /*
         * check for the type of responses we got
         */
        bytes = jxta_message_element_get_value(el);
        response = jstring_new_3(bytes);
        if (response == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
            JXTA_OBJECT_RELEASE(el);
            JXTA_OBJECT_RELEASE(bytes);
            return JXTA_NOMEM;
        }
        res = (char *) jstring_get_string(response);

        if (strcmp(res, RELAY_RESPONSE_CONNECTED) == 0) {
            process_connected_reply(self, msg);
        } else if (strcmp(res, RELAY_RESPONSE_DISCONNECTED) == 0) {
            process_disconnected_reply(self, msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Relay response not supported: %s\n", res);
        }

        JXTA_OBJECT_RELEASE(response);
        JXTA_OBJECT_RELEASE(bytes);
        JXTA_OBJECT_RELEASE(el);

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No relay response element\n");
    }

    return JXTA_SUCCESS;
}


Jxta_boolean is_contain_address(Jxta_endpoint_address * addr, Jxta_RdvAdvertisement * relay)
{
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_AccessPointAdvertisement *apa = NULL;
    Jxta_vector *dest = NULL;
    Jxta_boolean res = FALSE;
    unsigned int i = 0;
    JString *js_addr = NULL;
    char *saddr = NULL;

    route = jxta_RdvAdvertisement_get_Route(relay);
    apa = jxta_RouteAdvertisement_get_Dest(route);
    dest = jxta_AccessPointAdvertisement_get_EndpointAddresses(apa);
    saddr = jxta_endpoint_address_to_string(addr);
    if (saddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory");
        JXTA_OBJECT_RELEASE(route);
        JXTA_OBJECT_RELEASE(apa);
        JXTA_OBJECT_RELEASE(dest);
        return res;
    }

    for (i = 0; i < jxta_vector_size(dest); i++) {
        jxta_vector_get_object_at(dest, JXTA_OBJECT_PPTR(&js_addr), i);
        if (strncmp(saddr, jstring_get_string(js_addr), (strlen(saddr) - 2)) == 0) {
            res = TRUE;
            JXTA_OBJECT_RELEASE(js_addr);
            break;
        }

        JXTA_OBJECT_RELEASE(js_addr);
    }

    JXTA_OBJECT_RELEASE(apa);
    JXTA_OBJECT_RELEASE(route);
    JXTA_OBJECT_RELEASE(dest);
    free(saddr);
    return res;
}

/*
 * update relay connection state
 */
static void update_relay_peer_connection(_jxta_transport_relay * self, Jxta_RdvAdvertisement * relay, Jxta_time_diff lease)
{
    unsigned int i;
    Jxta_status res;
    _jxta_peer_relay_entry *peer = NULL;

    for (i = 0; i < jxta_vector_size(self->peers); i++) {

        res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get peer from vector peers\n");
            break;
        }

        /*
         * check if this entry correspond to our entry
         */
        if (is_contain_address(((_jxta_peer_entry *) peer)->address, relay)) {

            /*
             *  Mark the peer entry has connected
             */
            PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

            /*
             * Reset the flag to our last succeeded lease renewal state
             */
            peer->is_connected = TRUE;
            PEER_ENTRY_VTBL(peer)->jxta_peer_set_expires((Jxta_peer *) peer, jpr_time_now() + lease);
            peer->connectTime = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer) - RELAY_LEASE_RENEWAL_DELAY;

            jxta_endpoint_service_set_relay(self->endpoint,
                                            jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                            jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

            PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
            JXTA_OBJECT_RELEASE(peer);
            return;
        }

        /* We are done with this peer. Let release it */
        JXTA_OBJECT_RELEASE(peer);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "failed to find relay peer entry\n");
}

/*
 * extract Relay advertisement (or RDV advertisement this is the same)
 */
Jxta_RdvAdvertisement *extract_relay_adv(Jxta_message * msg)
{
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes = NULL;
    JString *value = NULL;
    Jxta_RdvAdvertisement *relayAdv = NULL;

    if (jxta_message_get_element_1(msg, RELAY_RESPONSE_ADV, &el) == JXTA_SUCCESS) {
        if (el) {
            bytes = jxta_message_element_get_value(el);
            value = jstring_new_3(bytes);
            relayAdv = jxta_RdvAdvertisement_new();
            if (value == NULL || relayAdv == NULL) {
                JXTA_OBJECT_RELEASE(el);
                JXTA_OBJECT_RELEASE(bytes);
                JXTA_OBJECT_RELEASE(value);
                return NULL;
            }
            jxta_RdvAdvertisement_parse_charbuffer(relayAdv, jstring_get_string(value), jstring_length(value));

            JXTA_OBJECT_RELEASE(el);
            JXTA_OBJECT_RELEASE(bytes);
            JXTA_OBJECT_RELEASE(value);
        }
    }
    return relayAdv;
}

/**
 * extract Relay lease
 */
static Jxta_time extract_lease_value(Jxta_message * msg)
{
    Jxta_time_diff lease = 0;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *value = NULL;
    int length = 0;
    char *bytes = NULL;

    if (jxta_message_get_element_1(msg, RELAY_LEASE_ELEMENT, &el) == JXTA_SUCCESS) {
        if (el) {
            value = jxta_message_element_get_value(el);
            length = jxta_bytevector_size(value);
            bytes = malloc(length + 1);
            if (bytes == NULL) {
                JXTA_OBJECT_RELEASE(value);
                JXTA_OBJECT_RELEASE(el);
                return 0;
            }

            jxta_bytevector_get_bytes_at(value, (unsigned char *) bytes, 0, length);
            bytes[length] = 0;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "did not find lease element in Lease Response \n");
            return 0;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed when trying to find lease element in Lease Response \n");
        return 0;
    }

    /*
     * convert to (long long) jxta_time
     */
#ifndef WIN32
    lease = atoll(bytes);
#else
    sscanf(bytes, "%I64d", &lease);
#endif

    /*
     * Convert the lease in micro-seconds
     */
    if (lease < 0) {
        lease = 0;
    }

    free(bytes);
    bytes = NULL;
    JXTA_OBJECT_RELEASE(value);
    value = NULL;
    JXTA_OBJECT_RELEASE(el);

    return lease;
}


/**
 ** Process an incoming relay request response
 **/
static void process_connected_reply(_jxta_transport_relay * self, Jxta_message * msg)
{
    Jxta_time_diff lease = 0;
    Jxta_RdvAdvertisement *relayadv = NULL;

    lease = extract_lease_value(msg);

    if (0 == lease) {
        return;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "got relay lease: %" APR_INT64_T_FMT " milliseconds\n", lease);
    relayadv = extract_relay_adv(msg);

    if (relayadv == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No RdvAdvertisement in relay response\n");
        return;
    }

    jxta_PG_add_relay_address(self->group, relayadv);
    update_relay_peer_connection(self, relayadv, lease);
    JXTA_OBJECT_RELEASE(relayadv);
}

/**
 ** Process a relay disconnect request
 **/
static void process_disconnected_reply(_jxta_transport_relay * self, Jxta_message * ms)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Receive Relay disconnect msg not implemented\n");
}

/**
 ** connect_thread_main is the entry point of the thread that regulary tries to
 ** connect to a relay.
 **/
static void *APR_THREAD_FUNC connect_relay_thread(apr_thread_t * thread, void *arg)
{
    _jxta_transport_relay *self = PTValid(arg, _jxta_transport_relay);
    Jxta_status res;
    Jxta_peer *seed;
    unsigned int i;
    int nbOfConnectedPeers = 0;
    size_t sz;
    Jxta_boolean connect_seed = FALSE;
    JString *addr;
    Jxta_endpoint_address *addr_e;
    JString *addr_a;
    /* Mark the service as running now. */
    self->running = TRUE;

    /* initial connect to relay seed */

    for (i = 0; i < jxta_vector_size(self->TcpRelays); i++) {
        seed = (Jxta_peer *) relay_entry_new();
        jxta_vector_get_object_at(self->TcpRelays, JXTA_OBJECT_PPTR(&addr), i);
        addr_a = jstring_new_2("tcp://");
        jstring_append_1(addr_a, addr);
        addr_e = jxta_endpoint_address_new((char *) jstring_get_string(addr_a));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay seed address %s\n", jstring_get_string(addr_a));
        jxta_peer_set_address(seed, addr_e);
        JXTA_OBJECT_RELEASE(addr);
        JXTA_OBJECT_RELEASE(addr_e);
        JXTA_OBJECT_RELEASE(addr_a);
        jxta_vector_add_object_last(self->peers, (Jxta_object *) seed);
        JXTA_OBJECT_RELEASE(seed);
    }

    for (i = 0; i < jxta_vector_size(self->HttpRelays); i++) {
        seed = (Jxta_peer *) relay_entry_new();
        jxta_vector_get_object_at(self->HttpRelays, JXTA_OBJECT_PPTR(&addr), i);
        addr_a = jstring_new_2("http://");
        jstring_append_1(addr_a, addr);
        addr_e = jxta_endpoint_address_new((char *) jstring_get_string(addr_a));
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay seed address %s\n", (char *) jstring_get_string(addr_a));
        jxta_peer_set_address(seed, addr_e);
        JXTA_OBJECT_RELEASE(addr);
        JXTA_OBJECT_RELEASE(addr_e);
        JXTA_OBJECT_RELEASE(addr_a);
        jxta_vector_add_object_last(self->peers, (Jxta_object *) seed);
        JXTA_OBJECT_RELEASE(seed);
    }

    if (jxta_vector_size(self->peers) == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No valid relay addresses supported by relay service\n");
        apr_thread_exit(thread, JXTA_SUCCESS);
        return NULL;
    }

    jxta_vector_shuffle(self->peers);

    /* Main loop */
    while (self->running) {
        _jxta_peer_relay_entry *peer;

        /**
         ** Do the following for each of the relay peers:
         **        - check if a renewal of the lease has to be sent (if the peer is connected)
         **        - check if an attempt to connect has to be issued (if the peer is not connected)
         **        - check if the peer has to be disconnected (the lease has not been renewed)
         **/
        nbOfConnectedPeers = 0;

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }
            if (peer->is_connected) {
                check_relay_lease(self, peer);
            }
            /**
             ** Since check_peer_lease() may have changed the connected state,
             ** we need to re-test the state.
             **/
            if (peer->is_connected) {
                ++nbOfConnectedPeers;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /**
         ** We need to check the peers in two separated passes
         ** because:
         **          - check_relay_lease() may change the disconnected state
         **          - the total number of relay connected peers may change
         **            change the behavior of check_peer_connect()
         **/

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {

            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot get peer from vector peers\n");
                continue;
            }
            if (!peer->is_connected && (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS)) {
                check_relay_connect(self, peer, nbOfConnectedPeers);
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /**
         ** Wait for a while until the next iteration.
         **
         ** We want to have a better responsiveness (latency) of the mechanism
         ** to connect to rendezvous when there isn't enough rendezvous connected.
         ** The time to wait could be as small as we want, but the smaller it is, the
         ** more CPU it will use. Having two different periods for checking connection
         ** is a compromise for latency versus performance.
         **/

        if (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) {
            if (connect_seed) {
                apr_thread_mutex_lock(self->stop_mutex);
                res = apr_thread_cond_timedwait(self->stop_cond, self->stop_mutex, RELAY_THREAD_NAP_TIME_FAST);
                apr_thread_mutex_unlock(self->stop_mutex);
            }
        } else {
            apr_thread_mutex_lock(self->stop_mutex);
            res = apr_thread_cond_timedwait(self->stop_cond, self->stop_mutex, RELAY_THREAD_NAP_TIME_NORMAL);
            apr_thread_mutex_unlock(self->stop_mutex);
        }

        /*
         * Check if we have been awakened for the purpose of cancelation
         */
        if (!self->running)
            apr_thread_exit(thread, JXTA_SUCCESS);

        /*
         * Check if we receive our lease response while we were
         * sleeping
         */
        nbOfConnectedPeers = 0;
        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }

          /**
           ** Since check_peer_lease() may have changed the connected state,
           ** we need to re-test the state.
           **/
            if (peer->is_connected) {
                ++nbOfConnectedPeers;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /*
         * We still did not get a relay connect. Retry our
         * known seeding peers as that's the only thing we can do
         * at that point
         */
        if (nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) {
            sz = jxta_vector_size(self->peers);
            for (i = 0; i < sz; i++) {
                jxta_vector_get_object_at(self->peers, JXTA_OBJECT_PPTR(&peer), i);
                check_relay_connect(self, peer, nbOfConnectedPeers);
                connect_seed = TRUE;
                JXTA_OBJECT_RELEASE(peer);
            }

        }
    }

    apr_thread_exit(thread, JXTA_SUCCESS);
    return NULL;
}

static void check_relay_lease(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    char *add = NULL;

    PTValid(peer, _jxta_peer_relay_entry);

    /*
     * check if the peer has still a valid relay
     */

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    add = jxta_endpoint_service_get_relay_addr(self->endpoint);
    if (add == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "lost relay connection\n");
        peer->is_connected = FALSE;
        peer->try_connect = FALSE;
        peer->connectTime = 0;
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

        /*
         * remove the relay address from our local route
         *
         * FIXME 20040701 tra we should only remove the hop
         * for the relay we just disconnected with. For now we just 
         * remove all the hops
         */
        jxta_PG_remove_relay_address(self->group, NULL);
        return;
    } else {
        free(add);
    }

    expires = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);

    /* Check if the peer really still has a lease */
    if (expires < currentTime) {
        /* Lease has expired */
        peer->is_connected = FALSE;
        peer->try_connect = FALSE;
        /**
         ** Since we have just lost a connection, it is a good idea to
         ** try to reconnect right away.
         **/
        peer->connectTime = currentTime;
        peer->connectRetryDelay = 0;
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }
    /* We still have a lease. Is it time to renew it ? */
    if ((expires - currentTime) <= RELAY_RENEWAL_DELAY) {
        peer->connectTime = currentTime;
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "relay connection reconnect\n");

        reconnect_to_relay(self, peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "relay connection check ok %s\n",
                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
    return;
}

static void check_relay_connect(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer, int nbOfConnectedPeers)
{

    Jxta_time currentTime = jpr_time_now();

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Relay->connect in= %" APR_INT64_T_FMT "ms\n",
                    (peer->connectTime - currentTime));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "       RetryDelay= %" APR_INT64_T_FMT "ms\n", peer->connectRetryDelay);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "     connected to= %d relays\n", nbOfConnectedPeers);

    if (peer->connectTime > currentTime) {
        /**
         ** If the number of connected peer is less than MIN_NB_OF_CONNECTED_PEERS
         ** and if peer->connectTime is more than LONG_RETRY_DELAY, then connectTime is
         ** reset, forcing an early attempt to connect.
         ** Otherwise, let it go.
         **/
        if ((nbOfConnectedPeers < MIN_NB_OF_CONNECTED_RELAYS) && ((peer->connectTime - currentTime) > RELAY_LONG_RETRY_DELAY)) {
            /**
             ** Reset the connection time. Note that connectRetryDelay is not changed,
             ** because while it might be a good idea to try earlier, it is still very
             ** likely that the peer is still not reachable. 
             **/
            peer->connectTime = currentTime;
            PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
            return;
        }
    } else {
        /* Time to try a connection */
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

        /*
         * NOTE: 20050607 tra
         * check if we already send the try connect for that peer
         * if yes, just send a lease reconnect instead. The
         * relay server has a specific way to handle relay client
         * connection to protect itself against constant failure
         * of a client.
         */
        if (!peer->try_connect)
            connect_to_relay(self, peer);
        else
            reconnect_to_relay(self, peer);
        return;
    }
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
    return;
}

static Jxta_status send_message_to_peer(_jxta_transport_relay * me, Jxta_endpoint_address * destAddr, Jxta_message * msg)
{
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;

    transport = jxta_endpoint_service_lookup_transport(me->endpoint, jxta_endpoint_address_get_protocol_name(destAddr));

    if (!transport) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not find the corresponding transport\n");
        return JXTA_FAILED;
    }

    endpoint_messenger = jxta_transport_messenger_get(transport, destAddr);

    if (endpoint_messenger) {
        JXTA_OBJECT_CHECK_VALID(endpoint_messenger);
        addr = jxta_transport_publicaddr_get(transport);
        jxta_message_set_source(msg, addr);
        jxta_message_set_destination(msg, destAddr);
        endpoint_messenger->jxta_send(endpoint_messenger, msg);
        JXTA_OBJECT_RELEASE(endpoint_messenger);
        JXTA_OBJECT_RELEASE(addr);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not create a messenger\n");
        return JXTA_FAILED;
    }
    JXTA_OBJECT_RELEASE(transport);

    return JXTA_SUCCESS;
}

static void connect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{

    Jxta_time currentTime = jpr_time_now();
    Jxta_message *msg = NULL;
    Jxta_endpoint_address *destAddr = NULL;
    JString *tmp = NULL;
    JString *tmp1 = NULL;
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    if (peer->connectTime > currentTime) {
        /* Not time yet to try to connect */
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send connect request to relay\n");

    /*
     * mark that we send our new connect
     */
    peer->try_connect = TRUE;

    /**
     ** Update the delay before the next retry. Double it until it
     ** reaches MAX_RETRY_DELAY.
     **
     **/
    if (peer->connectRetryDelay > 0) {
        peer->connectRetryDelay = peer->connectRetryDelay * 2;
    } else {
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
    }
    if (peer->connectRetryDelay > RELAY_MAX_RETRY_DELAY) {
        peer->connectRetryDelay = RELAY_MAX_RETRY_DELAY;
    }

    peer->connectTime = currentTime + peer->connectRetryDelay;

    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    /**
     ** Create a message with a connection request and build the request.
     **/
    msg = jxta_message_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return;
    }

    /**
     ** Set the destination address of the message.
     **/

    tmp = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return;
    }
    jstring_append_2(tmp, ":");
    jstring_append_2(tmp, self->groupid);

    tmp1 = jstring_new_2(self->assigned_id);
    if (tmp1 == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        return;
    }
    jstring_append_2(tmp1, "/");
    jstring_append_2(tmp1, (char *) RELAY_CONNECT_REQUEST);
    jstring_append_2(tmp1, ",");
    jstring_append_2(tmp1, (char *) LEASE_REQUEST);
    jstring_append_2(tmp1, ",");
    jstring_append_2(tmp1, (char *) LAZY_CLOSE);

    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                           jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                           jstring_get_string(tmp), jstring_get_string(tmp1));
    if (destAddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        JXTA_OBJECT_RELEASE(tmp1);
        return;
    }

    send_message_to_peer(self, destAddr, msg);

    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);
    JXTA_OBJECT_RELEASE(msg);
}

static void reconnect_to_relay(_jxta_transport_relay * self, _jxta_peer_relay_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_message *msg = NULL;
    Jxta_endpoint_address *destAddr = NULL;
    JString *tmp = NULL;
    JString *tmp1 = NULL;
    Jxta_transport *transport = NULL;
    JxtaEndpointMessenger *endpoint_messenger = NULL;
    Jxta_endpoint_address *addr = NULL;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    if (peer->connectTime > currentTime) {
        /* Not time yet to try to connect */
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "send reconnect request to relay\n");

    /**
     ** Update the delay before the next retry. Double it until it
     ** reaches MAX_RETRY_DELAY.
     **
     **/
    if (peer->connectRetryDelay > 0) {
        peer->connectRetryDelay = peer->connectRetryDelay * 2;
    } else {
        peer->connectRetryDelay = RELAY_MIN_RETRY_DELAY;
    }
    if (peer->connectRetryDelay > RELAY_MAX_RETRY_DELAY) {
        peer->connectRetryDelay = RELAY_MAX_RETRY_DELAY;
    }

    peer->connectTime = currentTime + peer->connectRetryDelay;

    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    /**
     ** Create a message with a connection request and build the request.
     **/
    msg = jxta_message_new();
    if (msg == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        return;
    }

    /**
     ** Set the destination address of the message.
     **/
    tmp = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    if (tmp == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        return;
    }
    jstring_append_2(tmp, ":");
    jstring_append_2(tmp, self->groupid);

    tmp1 = jstring_new_2(self->assigned_id);
    if (tmp1 == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        return;
    }
    jstring_append_2(tmp1, "/");
    jstring_append_2(tmp1, self->peerid);

    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                           jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                           jstring_get_string(tmp), jstring_get_string(tmp1));
    if (destAddr == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "out of memory\n");
        JXTA_OBJECT_RELEASE(msg);
        JXTA_OBJECT_RELEASE(tmp);
        JXTA_OBJECT_RELEASE(tmp1);
        return;
    }

    send_message_to_peer(self, destAddr, msg);

    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);
    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(msg);
}

/* vim: set ts=4 sw=4 tw=130 et: */
