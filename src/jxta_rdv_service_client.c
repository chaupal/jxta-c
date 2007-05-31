/**
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_rdv_service_client.c,v 1.121 2005/04/04 20:58:09 slowhog Exp $
 **/


/************************************************************************************
 **
 ** This is the edge Client Rendezvous Service implementation
 **
 ** And edge client is capable to connect to rendezvous peers,
 ** send and receive propagated messages. 
 ** An edge client is not capable of acting as a rendezvous nor
 ** to re-propagate a received propagated message.
 **
 ** An edge client can retrieve the list of rendezvous peers it is connected
 ** to, but is not capable of setting them (only configuration or self discovery).
 ************************************************************************************/

#include <limits.h>

#include "jxtaapr.h"
#include "jpr/jpr_thread.h"

#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jxta_peer.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_discovery_service.h"
#include "jxta_pa.h"
#include "jxta_svc.h"

#include "jxta_vector.h"
#include "jxta_pm.h"
#include "jstring.h"
#include "jxta_private.h"
#include "jxta_id_priv.h"
#include "jxta_id.h"
#include "jxta_rdv.h"
#include "jxta_endpoint_service.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_peer.h"
#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"

extern void *jxta_endpoint_service_new_instance(void);

/**
 ** Time the background thread that regulary checks the peer will wait
 ** between two iteration.
 **/

#define CONNECT_THREAD_NAP_TIME_NORMAL (60 * 1000 * 1000)       /* 1 Minute */
#define CONNECT_THREAD_NAP_TIME_FAST (2 * 1000 * 1000)  /* 2 Seconds */

/**
 ** Minimal number of rendezvous this client will attempt to connect to.
 ** If there is fewer connected rendezvous, the Rendezvous Service
 ** becomes more aggressive trying to connect to rendezvous peers.
 **/

#define MIN_NB_OF_CONNECTED_RDVS 1

/**
 ** Maximum time between two connection attempts to the same peer.
 **/

#define MAX_RETRY_DELAY ((Jxta_time) (30 * 60 * 1000))  /* 30 Minutes */

/**
 ** Minimum delay between two connection attempts to the same peer.
 **/

#define MIN_RETRY_DELAY ((Jxta_time) (15 * 1000))       /* 15 Seconds */

/**
 ** When the number of connected peer is less than MIN_NB_OF_CONNECTED_RDVS
 ** the Rendezvous Service tries to connect to peers earlier than planned
 ** if there were scheduled for more than LONG_RETRY_DELAY
 **/

#define LONG_RETRY_DELAY (MAX_RETRY_DELAY / 2)

/**
 ** Renewal of the leases must be done before the lease expires. The following
 ** constant defines the number of microseconds to substract to the expiration
 ** time in order to set the time of renewal.
 **/
#define LEASE_RENEWAL_DELAY ((Jxta_time) (5 * 60 * 1000))       /* 5 Minutes */

/**
 **  Need to delay the RDV connection in case we have a relay
 **/
#define RDV_RELAY_DELAY_CONNECTION (100 * 1000) /* 100 ms */

/*********************************************************************
 ** Internal structure of a peer used by this service.
 *********************************************************************/

struct _jxta_peer_rdv_entry {
    Extends(_jxta_peer_entry);

    boolean is_connected;
    boolean try_connect;
    boolean is_rdv;
    Jxta_time connectTime;
    Jxta_time connectRetryDelay;
};

typedef struct _jxta_peer_rdv_entry _jxta_peer_rdv_entry;

struct _jxta_rdv_entry_methods {
    Extends(Jxta_Peer_entry_methods);

    /* additional methods */
};

typedef struct _jxta_rdv_entry_methods _jxta_rdv_entry_methods;

/**
    Public typedef for the JXTA Rendezvous service client object.
**/
typedef Jxta_rdv_service_provider Jxta_rdv_service_client;

/*********************************************************************
 ** This is the Rendezvous Service Client internal object definition
 ** The first field in the structure MUST be of type _jxta_rdv_service_provider
 ** since that is the generic interface of a Rendezvous Service. 
 ** This base structure is a Jxta_object that needs to be initialized.
 *********************************************************************/
struct _jxta_rdv_service_client {
    Extends(_jxta_rdv_service_provider);

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    volatile Jxta_boolean running;
    apr_thread_cond_t *periodicCond;
    apr_thread_mutex_t *periodicMutex;
    volatile apr_thread_t *periodicThread;
 
    char *instanceName;
    char *messageElementName;
    char *groupid;
    char *assigned_idString;
    Jxta_PGID *gid;
    Jxta_discovery_service *discovery;
    Jxta_vector *peers;
    Jxta_vector *seeds;
    Jxta_PA *localPeerAdv;
    Jxta_PID *localPeerId;
    JString *localPeerIdJString;
    Jxta_listener *listener_service;
    Jxta_listener *listener_propagate;
    Jxta_listener *listener_peerview;
};

/**
    Private typedef for the JXTA Rendezvous service client object.
**/
typedef struct _jxta_rdv_service_client _jxta_rdv_service_client;

/**
 ** Forward definition to make the compiler happy
 **/

Jxta_rdv_service_client *jxta_rdv_service_client_new(void);
static _jxta_rdv_service_client *jxta_rdv_service_client_construct(_jxta_rdv_service_client * self,
                                                                   const _jxta_rdv_service_provider_methods * methods);
static void rdv_client_delete(Jxta_object * service);
static void jxta_rdv_service_client_destruct(_jxta_rdv_service_client * self);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status add_peer(Jxta_rdv_service_provider * provider, Jxta_peer * peer);
static Jxta_status add_seed(Jxta_rdv_service_provider * provider, Jxta_peer * peer);
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, char *serviceName, char *serviceParam,
                             int ttl);

static void *APR_THREAD_FUNC connect_thread_main(apr_thread_t * thread, void *self);
static void rdv_service_client_listener(Jxta_object * msg, void *arg);
static void rdv_service_peerview_listener(Jxta_object * msg, void *arg);
static void rdv_service_client_message_listener(Jxta_object * msg, void *arg);
static boolean rdv_peer_connected(_jxta_peer_rdv_entry * peer);
static Jxta_status rdv_peer_set_connected(_jxta_peer_rdv_entry * peer, boolean connected);
static Jxta_status jxta_search_rdv_peergroup(_jxta_rdv_service * rdv);

static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * self, Jxta_endpoint_address * addr);

static _jxta_peer_rdv_entry *rdv_entry_new(void);
static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * self, const _jxta_rdv_entry_methods * methods);
static void rdv_entry_delete(Jxta_object * addr);
static void rdv_entry_destruct(_jxta_peer_rdv_entry * self);

static const _jxta_rdv_service_provider_methods JXTA_RDV_SERVICE_CLIENT_METHODS = {
    "_jxta_rdv_service_provider_methods",
    init,
    start,
    stop,
    add_peer,
    add_seed,
    get_peers,
    propagate
};

static const _jxta_rdv_entry_methods JXTA_RDV_ENTRY_METHODS = {
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
    "_jxta_rdv_entry_methods"
};

#define RDV_ENTRY_VTBL(self) ((_jxta_rdv_entry_methods*) ((_jxta_peer_entry*) (self))->methods)

static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * self, const _jxta_rdv_entry_methods * methods)
{
    self = (_jxta_peer_rdv_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        PTValid(methods, _jxta_rdv_entry_methods);
        PEER_ENTRY_VTBL(self) = (Jxta_Peer_entry_methods *) methods;
        self->thisType = "_jxta_peer_rdv_entry";

        self->is_connected = FALSE;
        self->try_connect = FALSE;
        self->is_rdv = FALSE;
        self->connectTime = 0;
        self->connectRetryDelay = 0;
    }

    return self;
}

static void rdv_entry_destruct(_jxta_peer_rdv_entry * self)
{
    peer_entry_destruct((_jxta_peer_entry *) self);
}


static void rdv_entry_delete(Jxta_object * addr)
{
    _jxta_peer_rdv_entry *self = (_jxta_peer_rdv_entry *) addr;

    rdv_entry_destruct(self);

    free(self);
}

static _jxta_peer_rdv_entry *rdv_entry_new(void)
{
    _jxta_peer_rdv_entry *self = (_jxta_peer_rdv_entry *) malloc(sizeof(_jxta_peer_rdv_entry));

    /* Initialize the object */
    memset(self, 0, sizeof(_jxta_peer_rdv_entry));
    JXTA_OBJECT_INIT(self, rdv_entry_delete, 0);

    return rdv_entry_construct(self, &JXTA_RDV_ENTRY_METHODS);
}

/******
 * Initializes an instance of the Rendezvous Service. (a module method)
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service.
 * @param group a pointer to the PeerGroup the Rendezvous Service is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * @return error code.
 *******/

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    JString *tmp = NULL;
    JString *tmp1 = NULL;
    char *tmpid = NULL;
    JString *string = NULL;
    Jxta_id *assigned_id = jxta_service_get_assigned_ID_priv((_jxta_service *) service);
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) service);

    jxta_rdv_service_provider_init(provider, service);

    jxta_id_to_jstring(assigned_id, &string);

    self->assigned_idString = malloc(strlen(jstring_get_string(string)) + 1);

    strcpy(self->assigned_idString, jstring_get_string(string));

    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_discovery_service(group, &(self->discovery));
    jxta_PG_get_PID(group, &(self->localPeerId));
    jxta_PG_get_PA(group, &(self->localPeerAdv));

    jxta_PG_get_GID(group, &self->gid);
    jxta_id_to_jstring(self->gid, &tmp);
    tmpid = (char *) jstring_get_string(tmp);
    self->groupid = malloc(strlen(tmpid));
    self->instanceName = malloc(strlen(tmpid));

    /*
     * Only extract the unique Group ID string
     * For building endpoint address
     */
    strcpy(self->groupid, &tmpid[9]);
    JXTA_LOG("gid %s\n", self->groupid);

    /**
     ** Build the local name of the instance
     **/
    tmpid = (char *) jstring_get_string(tmp);

    /*
     * remove the prefix jxta:urn
     *
     * FIXME 20040225 tra need to define a macro for this
     */
    strcpy(self->instanceName, &tmpid[9]);
    JXTA_OBJECT_RELEASE(tmp);

    /* Build the JString that contains the string representation of the ID */
    self->localPeerIdJString = NULL;
    jxta_id_to_jstring(self->localPeerId, &self->localPeerIdJString);

    /**
     ** Builds the element name contained into each propagated message.
     **/
    {
        char *pt = malloc(strlen(JXTA_RDV_PROPAGATE_ELEMENT_NAME) + strlen(self->instanceName) + 1);

        sprintf(pt, "%s%s", JXTA_RDV_PROPAGATE_ELEMENT_NAME, self->instanceName);
        self->messageElementName = pt;
    }

    /**
     ** Add the Rendezvous Service Endpoint Listener
     **/

    self->listener_service = jxta_listener_new(rdv_service_client_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                       (char *) JXTA_RDV_SERVICE_NAME, self->instanceName, self->listener_service);

    jxta_listener_start(self->listener_service);

    /**
     ** Add the Rendezvous Service Message receiver listener
     **/

    self->listener_propagate = jxta_listener_new(rdv_service_client_message_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                       (char *) JXTA_RDV_PROPAGATE_SERVICE_NAME, self->instanceName, self->listener_propagate);

    jxta_listener_start(self->listener_propagate);

    /**
     **
     ** Add the Rendezvous peerview listener
     **/

    self->listener_peerview = jxta_listener_new(rdv_service_peerview_listener, (void *) self, 10, 100);

    tmp = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    jstring_append_2(tmp, ":");
    jstring_append_2(tmp, self->instanceName);

    tmp1 = jstring_new_2(JXTA_RDV_PEERVIEW_NAME);
    jstring_append_2(tmp1, "/");
    jstring_append_2(tmp1, self->instanceName);

    jxta_endpoint_service_add_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                       (char *) jstring_get_string(tmp),
                                       (char *) jstring_get_string(tmp1), self->listener_peerview);

    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);

    jxta_listener_start(self->listener_peerview);

    /* process the config params rdv */
    do {
        Jxta_PA *conf_adv = NULL;
        Jxta_svc *svc = NULL;
        Jxta_vector *svcs;
        size_t sz;
        size_t i;
        Jxta_vector *addresses;

        jxta_PG_get_configadv((Jxta_PG *) group, &conf_adv);

        if (conf_adv == NULL)
            break;

        svcs = jxta_PA_get_Svc(conf_adv);
        JXTA_OBJECT_RELEASE(conf_adv);

        sz = jxta_vector_size(svcs);
        for (i = 0; i < sz; i++) {
            Jxta_id *mcid;
            Jxta_svc *tmpsvc = NULL;
            jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
            mcid = jxta_svc_get_MCID(tmpsvc);
            if (jxta_id_equals(mcid, jxta_rendezvous_classid)) {
                svc = tmpsvc;
                JXTA_OBJECT_RELEASE(mcid);
                break;
            }
            JXTA_OBJECT_RELEASE(mcid);
            JXTA_OBJECT_RELEASE(tmpsvc);
        }
        JXTA_OBJECT_RELEASE(svcs);

        if (svc == NULL)
            break;

        addresses = jxta_svc_get_Addr(svc);
        JXTA_OBJECT_RELEASE(svc);

        if (addresses == NULL) {
            break;
        }
        sz = jxta_vector_size(addresses);
        for (i = 0; i < sz; i++) {
            JString *addr_s;
            Jxta_endpoint_address *addr_e;

            jxta_vector_get_object_at(addresses, (Jxta_object **) & addr_s, i);
            addr_e = jxta_endpoint_address_new((char *)
                                               jstring_get_string(addr_s));

            /* FIXME: jice@jxta.org 20020321 - that conversion should be
             * done by jxta_svc itself.
             */
            if (addr_e != NULL) {
                Jxta_peer *peer = (Jxta_peer *) rdv_entry_new();

                jxta_peer_set_address(peer, addr_e);

                /*
                 * Add the peer to the list of seed peer
                 */
                add_seed((Jxta_rdv_service_provider *) self, peer);
                JXTA_OBJECT_RELEASE(peer);
                JXTA_OBJECT_RELEASE(addr_e);
            }

            JXTA_OBJECT_RELEASE(addr_s);
        }
        JXTA_OBJECT_RELEASE(addresses);
    } while (0);

    /*
     * Search for RDV for the groups, if we are not the netpeergroup
     *
     */
    JXTA_LOG("group Id %s\n", self->groupid);
    {
        Jxta_PG *parent = NULL;
        jxta_PG_get_parentgroup(group, &parent);
        if (parent != NULL) {
            JXTA_LOG("Looking for seed RDV for the peergroup\n");
            jxta_search_rdv_peergroup(jxta_rdv_service_provider_get_service_priv(provider));
        } else {
            JXTA_LOG("No search for RDV within jxta-NetGroup\n");
        }
    }

    return JXTA_SUCCESS;
}


/******
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param this a pointer to the instance of the Rendezvous Service
 * @param argv a vector of string arguments.
 *******/
static Jxta_status start(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);

    jxta_rdv_service_provider_lock_priv(provider);

    self->running = TRUE;

    /**
     ** Start a thread that will process lease requests to rendezvous peers.
     ** Since we are giving the instance of this service to a background thread,
     ** we need to share the object.
     **/
    JXTA_OBJECT_SHARE(self);

    apr_thread_mutex_lock(self->periodicMutex);

    apr_thread_create(&self->periodicThread, NULL,      /* no attr */
                      connect_thread_main, (void *) self, jxta_rdv_service_provider_get_pool_priv(provider));

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_rdv_service_provider_unlock_priv(provider);



    return JXTA_SUCCESS;
}

/******
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @return error code.
 *******/
static Jxta_status stop(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    JString *tmp;
    JString *tmp1;

    /* We need to tell the background thread that it has to die. */
    self->running = FALSE;

    apr_thread_mutex_lock(self->periodicMutex);
    apr_thread_cond_signal(self->periodicCond);
    apr_thread_mutex_unlock(self->periodicMutex);

    tmp = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    jstring_append_2(tmp, ":");
    jstring_append_2(tmp, self->instanceName);

    tmp1 = jstring_new_2(JXTA_RDV_PEERVIEW_NAME);
    jstring_append_2(tmp1, "/");
    jstring_append_2(tmp1, self->instanceName);

    jxta_rdv_service_provider_lock_priv(provider);

    jxta_endpoint_service_remove_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                          (char *) JXTA_RDV_SERVICE_NAME, self->instanceName);

    jxta_endpoint_service_remove_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                          (char *) JXTA_RDV_PROPAGATE_SERVICE_NAME, self->instanceName);


    jxta_endpoint_service_remove_listener(jxta_rdv_service_provider_get_service_priv(provider)->endpoint,
                                          (char *) jstring_get_string(tmp), (char *) jstring_get_string(tmp1));

    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);

	if (self->listener_service) {
        JXTA_OBJECT_RELEASE(self->listener_service);
        self->listener_service = NULL;
    }

    if (self->listener_propagate) {
        JXTA_OBJECT_RELEASE(self->listener_propagate);
        self->listener_propagate = NULL;
    }

    if (self->listener_peerview) {
        JXTA_OBJECT_RELEASE(self->listener_peerview);
        self->listener_peerview = NULL;
    }

    /* Release the services object this instance was using */
    if (self->discovery != NULL)
        JXTA_OBJECT_RELEASE(self->discovery);

    /* kick our thread out of its loop and join it. */
    /*
     * FIXME: jice@jxta.org 20020225 - we need an equivalent of
     * thread.interrupt to force the thread to unblock from whatever it may
     * be blocked on. Otherwise we could wait here forever.
     *  apr_thread_join (&status, self->thread);
     */
    JXTA_OBJECT_RELEASE(self->gid);
    JXTA_LOG("Stopped.\n");

    jxta_rdv_service_provider_unlock_priv(provider);

    return JXTA_SUCCESS;
}

static Jxta_status add_peer(Jxta_rdv_service_provider * provider, Jxta_peer * peer)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    Jxta_status res;

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    /* Add it into the vector */
    res = jxta_vector_add_object_first(self->peers, (Jxta_object *) peer);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Failed inserting _jxta_peer_entry into peers vector.\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* The Rendezvous Client Service does not implement this method */
    return JXTA_SUCCESS;
}

static Jxta_status add_seed(Jxta_rdv_service_provider * provider, Jxta_peer * peer)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    Jxta_status res;

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* Add it into the vector */
    res = jxta_vector_add_object_first(self->seeds, (Jxta_object *) peer);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Failed inserting _jxta_peer_entry into seeds vector.\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return JXTA_SUCCESS;
}


/******
 ** Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 *******/
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    Jxta_status res;

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    /**
     ** Allocate a new vector and stores it the peers from
     ** our own vector: we don't want to share the our vector itself
     **/
    res = jxta_vector_clone(self->peers, peerlist, 0, INT_MAX);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Cannot allocate a new jxta_vector\n");
        return res;
    }
    return JXTA_SUCCESS;
}

/******
 * Propagates a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param ttl Maximum number of peers the propagated message can go through.
 * @return error code.
 *******/
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, char *serviceName, char *serviceParam,
                             int ttl)
{

    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    Jxta_message_element *el = NULL;
    RendezVousPropagateMessage *pmsg = NULL;
    JString *string = NULL;
    char *pt = NULL;
    Jxta_vector *vector = NULL;
    Jxta_status res;
    JString *messageId;

    JXTA_LOG("Propagate starts\n");

    /* Test arguments first */
    if ((msg == NULL) || (ttl <= 0)) {
        /* Invalid args. */
        JXTA_LOG("Invalid arguments. Message is dropped\n");
        return JXTA_INVALID_ARGUMENT;
    }

    /* Retrieve (if any) the propagate message element */

    res = jxta_message_get_element_1(msg, self->messageElementName, &el);

    if (el) {
        /* Remove the element from the message */
        Jxta_bytevector *bytes = NULL;
        JString *string = NULL;

        jxta_message_remove_element(msg, el);

        JXTA_OBJECT_CHECK_VALID(el);

        pmsg = RendezVousPropagateMessage_new();
        bytes = jxta_message_element_get_value(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        RendezVousPropagateMessage_parse_charbuffer(pmsg, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);
        string = NULL;
        JXTA_OBJECT_RELEASE(el);

    } else {

        message_id_new(self->gid, &messageId);
        JXTA_LOG("propagate message ID %s\n", jstring_get_string(messageId));

        /* Create a new propagate message element */
        pmsg = RendezVousPropagateMessage_new();
        /* Initialize it */
        RendezVousPropagateMessage_set_DestSName(pmsg, serviceName);
        RendezVousPropagateMessage_set_DestSParam(pmsg, serviceParam);
        RendezVousPropagateMessage_set_MessageId(pmsg, (char *) jstring_get_string(messageId));
        RendezVousPropagateMessage_set_TTL(pmsg, ttl);
        JXTA_OBJECT_RELEASE(messageId);
    }


    /* Add ourselves into the path */
    vector = RendezVousPropagateMessage_get_Path(pmsg);

    JXTA_OBJECT_CHECK_VALID(vector);

    if (vector == NULL) {
        vector = jxta_vector_new(1);
    }

    res = jxta_vector_add_object_first(vector, (Jxta_object *) self->localPeerIdJString);

    JXTA_OBJECT_CHECK_VALID(self->localPeerIdJString);

    if (res != JXTA_SUCCESS) {
        /* We just display an error LOG message and keep going. */
        JXTA_LOG("WARNING: could not add the local peer into the path\n");
    }

    RendezVousPropagateMessage_set_Path(pmsg, vector);

    JXTA_OBJECT_RELEASE(vector);

    /* Add pmsg into the message */
    res = RendezVousPropagateMessage_get_xml(pmsg, &string);

    JXTA_OBJECT_CHECK_VALID(string);

    if (res != JXTA_SUCCESS) {
        JXTA_LOG("Cannot retrieve XML from advertisement.\n");
        JXTA_OBJECT_RELEASE(pmsg);
        return JXTA_INVALID_ARGUMENT;
    }

    /**
     ** FIXME 3/8/2002 lomax@jxta.org
     ** Allocate a buffer than can be freed by the message 
     ** This is currentely needed because the message API does not yet
     ** support sharing of data. When that will be completed, we will be
     ** able to avoid this extra copy.
     **/

    pt = malloc(jstring_length(string) + 1);
    pt[jstring_length(string)] = 0;
    memcpy(pt, jstring_get_string(string), jstring_length(string));

    JXTA_OBJECT_RELEASE(string);

    el = jxta_message_element_new_1(self->messageElementName, "text/xml", pt, strlen(pt), NULL);
    free(pt);
    JXTA_OBJECT_CHECK_VALID(el);

    if (el == NULL) {
        JXTA_LOG("Cannot create message element.\n");

        JXTA_OBJECT_RELEASE(pmsg);
        return JXTA_INVALID_ARGUMENT;
    }

    jxta_message_add_element(msg, el);

    JXTA_OBJECT_CHECK_VALID(el);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(pmsg);


    /*
     * propagate via physical transport if any transport
     * is available and support multicast
     */

    res =
        jxta_endpoint_service_propagate(jxta_rdv_service_provider_get_service_priv(provider)->endpoint, msg,
                                        (char *) JXTA_RDV_PROPAGATE_SERVICE_NAME, self->instanceName);
    JXTA_LOG("endpoint service propagation status= %d\n", res);

    /**
     ** This message is now ready to be propagated.
     **/
    vector = NULL;
    res = get_peers((Jxta_rdv_service_provider *) self, &vector);

    JXTA_OBJECT_CHECK_VALID(vector);
    if ((res == JXTA_SUCCESS) && (vector != NULL)) {

        int i;

        for (i = 0; i < jxta_vector_size(vector); ++i) {
            _jxta_peer_rdv_entry *peer = NULL;

            res = jxta_vector_get_object_at(vector, (Jxta_object **) & peer, i);

            PTValid(peer, _jxta_peer_rdv_entry);

            if (res != JXTA_SUCCESS) {
                JXTA_LOG("Cannot retrieve Jxta_peer from vector.\n");
                continue;
            }
            if (rdv_peer_connected(peer)) {
                /* This is a connected peer, send the message to this peer */
                /* Duplicate the message */

                Jxta_message *newMsg = jxta_message_clone(msg);
                Jxta_endpoint_address *destAddr = NULL;

                if (newMsg == NULL) {
                    JXTA_LOG("Cannot duplicate message\n");
                    continue;
                }

                /**
                 ** Set the destination address of the message.
                 **/
                destAddr =
                    jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                               jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address),
                                               (char *) JXTA_RDV_PROPAGATE_SERVICE_NAME, self->instanceName);

                /* Send the message */
                res =
                    jxta_endpoint_service_send(jxta_service_get_peergroup_priv
                                               ((_jxta_service *) jxta_rdv_service_provider_get_service_priv(provider)),
                                               jxta_rdv_service_provider_get_service_priv(provider)->endpoint, newMsg, destAddr);
                if (res != JXTA_SUCCESS) {
                    JXTA_LOG("failed to propagate message mark the rdv down %s\n",
                             jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));
                    peer->is_connected = FALSE;
                    peer->try_connect = FALSE;
                    peer->connectTime = 0;
                    peer->connectRetryDelay = MIN_RETRY_DELAY;
                }

                JXTA_OBJECT_RELEASE(newMsg);
                JXTA_OBJECT_RELEASE(destAddr);
            }
            JXTA_OBJECT_RELEASE(peer);
        }
        JXTA_OBJECT_RELEASE(vector);
    }

    JXTA_LOG("Propagate ends\n");
    return JXTA_SUCCESS;
}

/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * Standard instantiator
 **/
Jxta_rdv_service_client *jxta_rdv_service_client_new(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) malloc(sizeof(_jxta_rdv_service_client));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(_jxta_rdv_service_client));
    JXTA_OBJECT_INIT(self, rdv_client_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service_client *) jxta_rdv_service_client_construct(self, &JXTA_RDV_SERVICE_CLIENT_METHODS);
}


/**
 * standard constructor. This is a "final" class so it's static.
 **/
static _jxta_rdv_service_client *jxta_rdv_service_client_construct(_jxta_rdv_service_client * self,
                                                                   const _jxta_rdv_service_provider_methods * methods)
{
    Jxta_status res;
    self = (_jxta_rdv_service_client *) jxta_rdv_service_provider_construct((_jxta_rdv_service_provider *) self, methods);

    if (NULL != self) {
        /* Set our rt type checking string */
        self->thisType = "_jxta_rdv_service_client";

        apr_pool_create(&self->pool, NULL);

        res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);

        if (res != APR_SUCCESS) {
            return NULL;
        }
        res = apr_thread_cond_create(&self->periodicCond, self->pool);

        if (res != APR_SUCCESS) {
            return NULL;
        }

        res = apr_thread_mutex_create(&self->periodicMutex, APR_THREAD_MUTEX_DEFAULT, self->pool);

        /** The following will be updated with initialized **/
        self->discovery = NULL;
        self->assigned_idString = NULL;

        /** Allocate a vector for storing the list of peers (rendezvous) **/
        self->peers = jxta_vector_new(1);
        self->seeds = jxta_vector_new(1);
        self->running = FALSE;
    }

    return (_jxta_rdv_service_client *) self;
}

/**
 * Frees an instance of the Rendezvous Service.
 * All objects and memory used by the instance must first be released/freed.
 *
 * @param service a pointer to the instance of the Rendezvous Service to free.
 * @return error code.
 **/
static void rdv_client_delete(Jxta_object * service)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(service, _jxta_rdv_service_client);

    /* call the hierarchy of dtors */
    jxta_rdv_service_client_destruct(self);

    /* free the object itself */
    free((void *) service);
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
static void jxta_rdv_service_client_destruct(_jxta_rdv_service_client * self)
{
    /* First, free the vector of peers */
    JXTA_OBJECT_RELEASE(self->peers);

    /* First, free the vector of peers */
    JXTA_OBJECT_RELEASE(self->seeds);

    /* Release the local peer adv and local peer id */
    JXTA_OBJECT_RELEASE(self->localPeerAdv);
    JXTA_OBJECT_RELEASE(self->localPeerId);

    if (self->localPeerIdJString) {
        JXTA_OBJECT_RELEASE(self->localPeerIdJString);
    }

    if (self->gid) {
        JXTA_OBJECT_RELEASE(self->gid);
    }

    /* Release our references */

    if (self->groupid) {
        free(self->groupid);
    }

    if (self->assigned_idString != 0)
        free(self->assigned_idString);

    if (self->messageElementName)
        free(self->messageElementName);

    /* Free the string that contains the name of the instance */
    if (self->instanceName) {
        free(self->instanceName);
    }
    
    apr_thread_mutex_destroy(self->mutex);
    apr_thread_mutex_destroy(self->periodicMutex);
    apr_thread_cond_destroy(self->periodicCond);

    apr_pool_destroy(self->pool);   

    /* call the base classe's dtor. */
    jxta_rdv_service_provider_destruct((_jxta_rdv_service_provider *) self);

    JXTA_LOG("Destruction finished\n");
}


/* END OF STANDARD SERVICE IMPLEMENTATION CODE */

static boolean rdv_peer_connected(_jxta_peer_rdv_entry * peer)
{
    boolean ret = FALSE;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
    ret = peer->is_connected;
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    return ret;
}

static Jxta_status rdv_peer_set_connected(_jxta_peer_rdv_entry * peer, boolean connected)
{
    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
    peer->is_connected = connected;
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    return JXTA_SUCCESS;
}

static Jxta_status rdv_peer_set_is_rdv(_jxta_peer_rdv_entry * peer, boolean val)
{
    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
    peer->is_rdv = val;
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    return JXTA_SUCCESS;
}

/**
 ** Search for Endpoint Addresses of current potential rendezvous peers.
 ** This information can come from the Platform configuration
 ** (typically, for the Network PeerGroup), or by seaching using
 ** the Discovery Service for rendezvous advertisement in the parent
 ** PeerGroup.
 **
 ** FIXME: (lomax@jxta.org) currentely, this code only searches in the
 ** configuration. In other words, it is very likely that this code
 ** will not find any rendezvous in a random PeerGroup. This will be
 ** changed when the Discovery Service will be implemented, and when
 ** this code will be completed. 02/07/2002.
 **/
static Jxta_vector *get_potential_rdvs(_jxta_rdv_service_client * rdv)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(rdv, _jxta_rdv_service_client);

    return self->seeds;
}

/**
 ** Make the union of two vectors.
 ** For each entry in the update vector, check if it exists in the
 ** master, and if it does not, add it.
 **
 ** FIX ME: currentely, this function is thread safe, because it is
 ** invoked only by the thread running connect_thread_main. When
 ** set_rdvs() will be implemented, this code will have to be protected.
 ** lomax@jxta.org
 **/

static void merge_rdv_list(Jxta_vector * master, Jxta_vector * update)
{

    _jxta_peer_rdv_entry *peer_in_master;
    _jxta_peer_rdv_entry *peer_in_update;
    Jxta_status res;
    int i, j;
    boolean found;

    if ((master == NULL) || (update == NULL)) {
        /* nothing to merge */
        return;
    }

    for (i = 0; i < jxta_vector_size(update); ++i) {
        res = jxta_vector_get_object_at(update, (Jxta_object **) & peer_in_update, i);
        if (res != JXTA_SUCCESS) {
            continue;
        }
        found = FALSE;
        for (j = 0; j < jxta_vector_size(master); ++j) {
            res = jxta_vector_get_object_at(master, (Jxta_object **) & peer_in_master, j);
            if (res != JXTA_SUCCESS) {
                /* We need to release the object */
                JXTA_OBJECT_RELEASE(peer_in_master);
                continue;
            }
            if (jxta_endpoint_address_equals
                (((_jxta_peer_entry *) peer_in_master)->address, ((_jxta_peer_entry *) peer_in_update)->address)) {
                /* Master already have this address */
                found = TRUE;
                JXTA_OBJECT_RELEASE(peer_in_master);
                break;
            }
            JXTA_OBJECT_RELEASE(peer_in_master);
        }
        if (found == FALSE) {
            /* This is a new address, save it into master. */
            jxta_vector_add_object_first(master, (Jxta_object *) peer_in_update);
        }
        /* We can now safely release peer_in_update */
        JXTA_OBJECT_RELEASE(peer_in_update);
    }
}


static void connect_to_peer(_jxta_rdv_service_client * self, _jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = (Jxta_time) jpr_time_now();
    Jxta_message *msg;
    Jxta_message_element *msgElem;
    Jxta_endpoint_address *destAddr;
    JString *localPeerAdvStr = NULL;
    JString *tmp;
    JString *tmp1;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *address;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
    if (peer->connectTime > currentTime && peer->is_connected) {
        /* Not time yet to try to connect */
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    peer->connectTime = currentTime + peer->connectRetryDelay;
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    /**
     ** Create a message with a connection request and build the request.
     **/
    msg = jxta_message_new();

    jxta_PA_get_xml(self->localPeerAdv, &localPeerAdvStr);
    msgElem = jxta_message_element_new_1(JXTA_RDV_CONNECT_REQUEST,
                                         "text/xml",
                                         (char *) jstring_get_string(localPeerAdvStr), jstring_length(localPeerAdvStr), NULL);

    JXTA_OBJECT_RELEASE(localPeerAdvStr);
    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);

    /**
     ** Send the message
     **
     ** Since the Endpoint Address object is immutable, we need
     ** to create a new one. Note that this should be done only
     ** once. FIXME 02/10/2002 lomax@jxta.org
     **/

    /**
     ** Set the destination address of the message.
     **/
    tmp = jstring_new_2(self->assigned_idString);
    jstring_append_2(tmp, "/");
    jstring_append_2(tmp, self->groupid);

    tmp1 = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
    jstring_append_2(tmp1, ":");
    jstring_append_2(tmp1, self->groupid);

    /*
     * FIXME 20040621 tra
     * There is a difference in the way the destination address is
     * constructed depending on how the destination physical address is
     * formulated. If it the direct transport address rather than
     * JXTA address we need to bypath the endpoint service demuxing and let
     * the transport demux the message directly to the RDV service.
     */
    address = jxta_peer_get_address_priv((Jxta_peer *) peer);
    if (!peer->try_connect) {
        destAddr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(address),
                                              jxta_endpoint_address_get_protocol_address(address),
                                              (char *) jstring_get_string(tmp1), (char *) jstring_get_string(tmp));
        peer->try_connect = TRUE;
    } else {
        destAddr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(address),
                                              jxta_endpoint_address_get_protocol_address(address),
                                              (char *) jstring_get_string(tmp), NULL);
        peer->try_connect = FALSE;
    }

    JXTA_OBJECT_RELEASE(tmp);
    JXTA_OBJECT_RELEASE(tmp1);

    res = jxta_endpoint_service_send(jxta_service_get_peergroup_priv((_jxta_service *)
                                                                     jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self)), jxta_rdv_service_provider_get_service_priv((Jxta_rdv_service_provider *) self)->endpoint, msg, destAddr);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("failed to send connect message\n");
        peer->connectTime = 0;
    }

    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(msg);
}

static void check_peer_lease(_jxta_rdv_service_client * self, _jxta_peer_rdv_entry * peer)
{

    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    expires = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);

    /* Check if the peer really still has a lease */
    if (expires < currentTime) {
        /* Lease has expired */
        peer->is_connected = FALSE;
        /**
         ** Since we have just lost a connection, it is a good idea to
         ** try to reconnect right away.
         **/
        peer->connectTime = currentTime;
        peer->connectRetryDelay = MIN_RETRY_DELAY;
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    /* We still have a lease. Is it time to renew it ? */
    if ((expires - currentTime) <= LEASE_RENEWAL_DELAY) {
        peer->connectTime = currentTime;
        PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        connect_to_peer(self, peer);
        return;
    }
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
}

static void check_peer_connect(_jxta_rdv_service_client * self, _jxta_peer_rdv_entry * peer, int nbOfConnectedRdvs)
{

    Jxta_time currentTime = jpr_time_now();

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    JXTA_LOG("peer->address = %s://%s\n", jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
             jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));
    JXTA_LOG("    try_connect = %d\n", peer->try_connect);
#ifndef WIN32
    JXTA_LOG("peer->connectTime= %lld\n", peer->connectTime);
    JXTA_LOG("     Current Time= %lld\n", currentTime);
    JXTA_LOG("       RetryDelay= %lld\n", peer->connectRetryDelay);
#else
    JXTA_LOG("peer->connectTime= %I64d\n", peer->connectTime);
    JXTA_LOG("     Current Time= %I64d\n", currentTime);
    JXTA_LOG("       RetryDelay= %I64d\n", peer->connectRetryDelay);
#endif

    JXTA_LOG("      Nb connected= %d\n", nbOfConnectedRdvs);

    if (peer->connectTime > currentTime && peer->is_connected) {
        /**
         ** If the number of connected peer is less than MIN_NB_OF_CONNECTED_RDVS
         ** and if peer->connectTime is more than LONG_RETRY_DELAY, then connectTime is
         ** reset, forcing an early attempt to connect.
         ** Otherwise, let it go.
         **/
        if ((nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS) && ((peer->connectTime - currentTime) > LONG_RETRY_DELAY)) {
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
        connect_to_peer(self, peer);
        return;
    }
    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
    return;
}


/**
 ** Process a response of a lease request.
 ** The message contains the following message elements:
 **
 **    -  "jxta:RdvAdvReply": contains the peer advertisement of the
 **        rendezvous peer that has granted the lease.
 **    -  "jxta:ConnectedLease": contains the granted lease in milliseconds
 **    -  "jxta:ConnectedPeer": peer id of the rendezvous peer that has granted
 **                             the lease.
 **/
static void process_connected_reply(_jxta_rdv_service_client * self, Jxta_message * msg)
{
    Jxta_message_element *el = NULL;
    Jxta_endpoint_address *rdvAddr = NULL;
    Jxta_PA *rdvAdv = NULL;
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_id *peerId = NULL;
    Jxta_time lease = 0;
    Jxta_time currentTime = 0;

    JXTA_LOG("Got a RDV service lease Response\n");

    jxta_message_get_element_1(msg, JXTA_RDV_CONNECTED_REPLY, &el);

    if (el) {
        /**
         ** This element contains the peer id of the rendezvous peer
         ** that has granted the lease.
         **/
        JString *tmp = NULL;
        Jxta_bytevector *value = jxta_message_element_get_value(el);

        /* Build the proper endpoint address */
        tmp = jstring_new_3(value);
        JXTA_OBJECT_RELEASE(value);
        value = NULL;

        peerId = NULL;
        jxta_id_from_jstring(&peerId, tmp);
        JXTA_OBJECT_RELEASE(tmp);
        if (peerId == NULL) {
            JXTA_LOG("Cannot create a Jxta_id from [%s]\n", value);
        } else {
            JString *iduniq = NULL;
            jxta_id_get_uniqueportion(peerId, &iduniq);
            tmp = jstring_new_2("jxta://");
            jstring_append_1(tmp, iduniq);
            JXTA_OBJECT_RELEASE(iduniq);
            iduniq = NULL;

            rdvAddr = jxta_endpoint_address_new((char *) jstring_get_string(tmp));
            JXTA_OBJECT_RELEASE(tmp);
        }
        JXTA_OBJECT_RELEASE(el);
    }

    el = NULL;
    jxta_message_get_element_1(msg, JXTA_RDV_RDVADV_REPLY, &el);

    if (el) {
        /**
         ** This element contains the peer id of the rendezvous peer
         ** that has granted the lease.
         **/
        Jxta_bytevector *value = jxta_message_element_get_value(el);

        rdvAdv = jxta_PA_new();
        if (rdvAdv == NULL) {
            JXTA_LOG("Cannot allocate a peer advertisement\n");
        } else {
            JString *string = NULL;

            string = jstring_new_3(value);
            jxta_PA_parse_charbuffer(rdvAdv, jstring_get_string(string), jstring_length(string));
            JXTA_OBJECT_RELEASE(string);
            string = NULL;

            JXTA_LOG("Got connection from [%s]\n", jstring_get_string(jxta_PA_get_Name(rdvAdv)));

            if (self->discovery == NULL) {
                jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((_jxta_service *)
                                                                              jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self)), &(self->discovery));
            }

            if (self->discovery != NULL) {
                discovery_service_publish(self->discovery, (Jxta_advertisement *) rdvAdv, DISC_PEER, (Jxta_expiration_time)
                                          DEFAULT_EXPIRATION, (Jxta_expiration_time)
                                          DEFAULT_EXPIRATION);
            }
        }
        JXTA_OBJECT_RELEASE(value);
        JXTA_OBJECT_RELEASE(el);
    }

    el = NULL;
    jxta_message_get_element_1(msg, JXTA_RDV_LEASE_REPLY, &el);

    if (el) {
        /**
         ** This element contains the lease that has been granted in milli-seconds
         **/
        Jxta_bytevector *value = jxta_message_element_get_value(el);
        int length = jxta_bytevector_size(value);
        char *bytes = malloc(length + 1);

        jxta_bytevector_get_bytes_at(value, (unsigned char *) bytes, 0, length);
        bytes[length] = 0;

        /*
         * convert to (long long) jxta_time
         */

#ifndef WIN32
        lease = atoll(bytes);
        JXTA_LOG("Got lease: %lld milliseconds\n", lease);
#else
        sscanf(bytes, "%I64d", &lease);
        JXTA_LOG("Got lease: %I64d milliseconds\n", lease);
#endif
        free(bytes);
        bytes = NULL;
        JXTA_OBJECT_RELEASE(value);
        value = NULL;
        JXTA_OBJECT_RELEASE(el);
    }

    /**
     ** Get and update the _jxta_peer_rdv_entry of this rendezvous.
     **/
    if (rdvAddr != NULL) {
        Jxta_rdv_service *service = jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self);
        currentTime = jpr_time_now();
        peer = get_peer_entry(self, rdvAddr);
        if (peerId != NULL) {
            jxta_peer_set_peerid((Jxta_peer *) peer, peerId);
        }
        if (rdvAdv != NULL) {
            jxta_peer_set_adv((Jxta_peer *) peer, rdvAdv);
            JXTA_OBJECT_RELEASE(rdvAdv);
        }

        if (lease > 0) {
            rdv_peer_set_connected(peer, TRUE);
            lease = currentTime + lease;
            jxta_peer_set_expires(peer, lease);
            rdv_peer_set_is_rdv(peer, TRUE);

            /**
             ** Connect time is to be set a bit before the expiration.
             **/
            PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);
            peer->connectTime = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);

            /*
             * Reset the try connect flag to the last request
             * state that generated the correct lease renewal.
             * When we are renewing a lease we have to manage
             * two steps depending on if we are just connecting,
             * or we are already connected. The lease connection
             * flip-flop from one state to the other to obtain
             * connections.
             * This enables us to reduce the number of lease
             * requests we send to one in most cases.
             */
            if (peer->try_connect == TRUE) {
                peer->try_connect = FALSE;
            } else {
                peer->try_connect = TRUE;
            }

            PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);
        } else {
            rdv_peer_set_connected(peer, FALSE);
            jxta_peer_set_expires(peer, 0);
        }
    }
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - grant of lease from the rendezvous peer.
 **   - deny of a lease.
 **/
static void rdv_service_client_listener(Jxta_object * obj, void *arg)
{

    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);
    Jxta_message_element *el = NULL;
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_id *rdv_peerid = NULL;
    int i = 0;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_boolean connected = FALSE;
    Jxta_time currentTime;

    JXTA_LOG("Rendezvous Service Client received a message\n");

    currentTime = jpr_time_now();
    for (i = 0; i < jxta_vector_size(self->peers); ++i) {
        res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);
        if (res != JXTA_SUCCESS) {
            /* We should print a LOG here. Just continue for the time being */
            continue;
        }

        /*
         * Need to verify if this is a valid lease renewal
         * as we may be getting close to the end of our lease. We need to
         * accept this request, if we are trying to renew our lease.
         */
        if (peer->is_connected
            && (PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer) - currentTime) > LEASE_RENEWAL_DELAY) {
            JXTA_LOG("Drop RDV lease response as we already have a RDV connection\n");
            connected = TRUE;
        } else {
            peer->try_connect = FALSE;
            peer->connectTime = 0;
            peer->connectRetryDelay = MIN_RETRY_DELAY;
        }

        JXTA_OBJECT_RELEASE(peer);
    }

    /*
     * we are already connected
     */
    if (connected) {
        JXTA_OBJECT_RELEASE(msg);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(self);

    jxta_message_get_element_1(msg, JXTA_RDV_CONNECTED_REPLY, &el);

    if (el) {
        Jxta_bytevector *value = jxta_message_element_get_value(el);

        /* Build the proper endpoint address */
        JString *tmp = jstring_new_3(value);
        JXTA_OBJECT_RELEASE(value);
        value = NULL;

        jxta_id_from_jstring(&rdv_peerid, tmp);
        JXTA_OBJECT_RELEASE(tmp);
        if (rdv_peerid == NULL) {
            JXTA_LOG("Cannot create a Jxta_id from [%s]\n", value);
        }

        /**
         ** This is a response to a lease request.
         **/
        process_connected_reply(self, msg);
        JXTA_OBJECT_RELEASE(el);

        /*
         *  notify the RDV listeners about the new RDV connection
         */
        jxta_rdv_generate_event(jxta_rdv_service_provider_get_service_priv(arg), JXTA_RDV_CONNECTED, rdv_peerid);

        /**
	 * Release the message: we are done with it.
	 **/
        JXTA_OBJECT_RELEASE(msg);
    }
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Peerview Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - Receive a peerview response when searching for a subgroup RDV
 **/
static void rdv_service_peerview_listener(Jxta_object * obj, void *arg)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes = NULL;
    JString *string;
    Jxta_RdvAdvertisement *rdv;
    Jxta_RouteAdvertisement *route;
    Jxta_AccessPointAdvertisement *ap;
    Jxta_vector *eas;
    int i;
    JString *jaddr;
    char *addr;
    Jxta_peer *peer = NULL;
    Jxta_endpoint_address *addr_e = NULL;

    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_LOG("Rendezvous Service Peerview received a response\n");

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(self);


    jxta_message_get_element_1(msg, JXTA_RDV_PEERVIEW_RDV, &el);

    if (el) {
      /**
       ** This is a response from a rendezvous
       **/
        bytes = jxta_message_element_get_value(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        rdv = jxta_RdvAdvertisement_new();
        jxta_RdvAdvertisement_parse_charbuffer(rdv, (char *) jstring_get_string(string), jstring_length(string));

        route = jxta_RdvAdvertisement_get_Route(rdv);
        if (route == NULL) {
            JXTA_LOG("Could not find route in RDV peerview response\n");
            JXTA_OBJECT_RELEASE(msg);
            return;
        }

        ap = jxta_RouteAdvertisement_get_Dest(route);
        JXTA_OBJECT_RELEASE(route);

        if (ap == NULL) {
            JXTA_LOG("Could not find endpoint addresses in RDV peerview response\n");
            JXTA_OBJECT_RELEASE(msg);
            return;
        }

        eas = jxta_AccessPointAdvertisement_get_EndpointAddresses(ap);
        JXTA_OBJECT_RELEASE(ap);

        peer = jxta_peer_new();

        for (i = 0; i < jxta_vector_size(eas); i++) {
            jxta_vector_get_object_at(eas, (Jxta_object **) & jaddr, i);
            addr = (char *) jstring_get_string(jaddr);

            if ((strncmp(addr, "http", 4) == 0) || ((strncmp(addr, "tcp", 3) == 0))) {
                JXTA_LOG("RDV address %s\n", addr);
                addr_e = jxta_endpoint_address_new(addr);
                jxta_peer_set_address(peer, addr_e);
                add_seed((Jxta_rdv_service_provider *) self, peer);
                JXTA_OBJECT_RELEASE(peer);
                JXTA_OBJECT_RELEASE(addr_e);
                JXTA_OBJECT_RELEASE(jaddr);
                break;
            } else {
                JXTA_LOG("Not taken RDV address %s\n", addr);
            }

            JXTA_OBJECT_RELEASE(jaddr);
        }

        JXTA_OBJECT_RELEASE(eas);
        JXTA_OBJECT_RELEASE(string);
        string = NULL;
        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }

    /**
     ** Release the message: we are done with it.
     **/
    JXTA_OBJECT_RELEASE(msg);

}

/**
 *  The entry point of the thread that maintains the rendezvous connection on a
 *  periodic basis.
 **/

static void *APR_THREAD_FUNC connect_thread_main(apr_thread_t * thread, void *arg)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);
    _jxta_peer_rdv_entry *peer;
    Jxta_vector *seeds;
    Jxta_status res;
    int i;
    int nbOfConnectedRdvs = 0;
    int sz;
    boolean connect_seed = FALSE;
    Jxta_transport *transport = NULL;
    int delay_count = 0;

    
    /* Mark the service as running now. */
    self->running = TRUE;
    
    apr_thread_mutex_lock(self->periodicMutex);

    /* initial connect to seed rendezvous */
    seeds = get_potential_rdvs(self);
    sz = jxta_vector_size(seeds);

    for (i = 0; i < sz; i++) {
        jxta_vector_get_object_at(seeds, (Jxta_object **) & peer, i);
        check_peer_connect(self, peer, nbOfConnectedRdvs);
        connect_seed = TRUE;
        JXTA_OBJECT_RELEASE(peer);
    }

    /*
     * FIXME 20040607  tra
     * We need to artificially delay the RDV service in case
     * we do have a relay transport. The relay transport should be the
     * first service to initiate the connection so the Relay server
     * can catch the incoming connection and keep it as a relay
     * connection
     */

    transport =
        jxta_endpoint_service_lookup_transport(jxta_rdv_service_provider_get_service_priv(arg)->endpoint, (char *) "jxta");
    if (transport != NULL) {
        jpr_thread_delay(RDV_RELAY_DELAY_CONNECTION);
        JXTA_OBJECT_RELEASE(transport);
    }

    /*
     * exponential backoff delay to avoid constant
     * retry to RDV seed.
     */
    delay_count = 1;
    /* Main loop */
    while (self->running) {

        /**
         ** Do the following for each of the peers:
         **        - check if a renewal of the lease has to be sent (if the peer is connected)
         **        - check if an attempt to connect has to be issued (if the peer is not connected)
         **        - check if the peer has to be disconnected (the lease has not been renewed)
         **/

        nbOfConnectedRdvs = 0;

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }
            if (peer->is_connected) {
                check_peer_lease(self, peer);
            }
            /**
             ** Since check_peer_lease() may have changed the connected state,
             ** we need to re-test the state.
             **/
            if (peer->is_connected) {
                ++nbOfConnectedRdvs;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /**
         ** We need to check the peers in two separated passes
         ** because:
         **          - check_peer_lease() may change the disconnected state
         **          - the total number of really connected peers may change
         **            change the behavior of check_peer_connect()
         **/

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {

            res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);
            if (res != JXTA_SUCCESS) {
                JXTA_LOG("Cannot get peer from vector peers\n");
                continue;
            }

            /*
             * only check if we haven't connected yet
             */
            if (!peer->is_connected && (nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS)) {
                check_peer_connect(self, peer, nbOfConnectedRdvs);
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

        if (nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS) {
            if (connect_seed) {
                res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, CONNECT_THREAD_NAP_TIME_FAST * delay_count++);
            } else {
                res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, CONNECT_THREAD_NAP_TIME_FAST * delay_count++);
            }
        } else {
            /*
             * reset delay count if we got a connection
             */
            delay_count = 1;
            res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, CONNECT_THREAD_NAP_TIME_NORMAL);
        }

        /*
         * Check if we have been awakened for the purpose of cancelation
         */
        if (!self->running)
            break;

        /*
         * Check if we got connected from the time
         * we send the request to the time we have been awakened.
         * we may have just received the lease grant while we
         * where sleeping. This check reduces the number of extra
         * lease requests we have to send.
         */

        nbOfConnectedRdvs = 0;

        for (i = 0; i < jxta_vector_size(self->peers); ++i) {
            res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);
            if (res != JXTA_SUCCESS) {
                continue;
            }
            if (peer->is_connected) {
                ++nbOfConnectedRdvs;
            }
            /* We are done with this peer. Let release it */
            JXTA_OBJECT_RELEASE(peer);
        }

        /*
         * We still did not get any rdv connect. Retry our
         * known seeding peers as that's the only thing we can do
         * at that point
         */
        if (nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS) {
            seeds = get_potential_rdvs(self);
            sz = jxta_vector_size(seeds);
            for (i = 0; i < sz; i++) {
                jxta_vector_get_object_at(seeds, (Jxta_object **) & peer, i);
                check_peer_connect(self, peer, nbOfConnectedRdvs);
                connect_seed = TRUE;
                JXTA_OBJECT_RELEASE(peer);
            }

        }
    }

    JXTA_OBJECT_RELEASE(arg);

    JXTA_LOG("Rendezvous Client worker thread exiting.\n");
    
    apr_thread_mutex_unlock(self->periodicMutex);

    self->periodicThread = NULL;

    apr_thread_exit(thread, JXTA_SUCCESS);
}

/**
 ** Returns the _jxta_peer_rdv_entry associated to a peer. If there is none, create
 ** a new one.
 **/
static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * self, Jxta_endpoint_address * addr)
{
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_status res = 0;
    int i;
    boolean found = FALSE;

    jxta_rdv_service_provider_lock_priv((Jxta_rdv_service_provider *) self);

    for (i = 0; i < jxta_vector_size(self->peers); ++i) {

        res = jxta_vector_get_object_at(self->peers, (Jxta_object **) & peer, i);
        if (res != JXTA_SUCCESS) {
            JXTA_LOG("Cannot get _jxta_peer_rdv_entry\n");
            continue;
        }
        if (jxta_endpoint_address_equals(addr, ((_jxta_peer_entry *) peer)->address)) {
            found = TRUE;
            break;
        }
        /* We are done with this peer. Let release it */
        JXTA_OBJECT_RELEASE(peer);
    }

    if (!found) {
        /* We need to create a new _jxta_peer_rdv_entry */
        peer = rdv_entry_new();
        if (peer == NULL) {
            JXTA_LOG("Cannot create a _jxta_peer_rdv_entry\n");
        } else {
            JXTA_OBJECT_SHARE(addr);
            ((_jxta_peer_entry *) peer)->address = addr;
            /* Add it into the vector */
            res = jxta_vector_add_object_first(self->peers, (Jxta_object *) peer);
            if (res != JXTA_SUCCESS) {
                JXTA_LOG("Cannot insert _jxta_peer_rdv_entry into vector peers\n");
            }
        }
    }

    jxta_rdv_service_provider_unlock_priv((Jxta_rdv_service_provider *) self);
    return peer;
}


/**
 ** This listener is called when a propagated message is received.
 **/
static void rdv_service_client_message_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);

    Jxta_message_element *el = NULL;
    RendezVousPropagateMessage *pmsg = NULL;

    JXTA_LOG("Rendezvous Service Client received a propagated message\n");

    jxta_message_get_element_1(msg, self->messageElementName, &el);

    if (el) {
        char *sName;
        char *sParam;
        char *messageId;
        Jxta_endpoint_address *realDest;
        Jxta_bytevector *bytes = NULL;
        JString *string = NULL;

        pmsg = RendezVousPropagateMessage_new();
        bytes = jxta_message_element_get_value(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        bytes = NULL;
        JXTA_OBJECT_RELEASE(el);

        RendezVousPropagateMessage_parse_charbuffer(pmsg, jstring_get_string(string), jstring_length(string));

        JXTA_OBJECT_RELEASE(string);
        string = NULL;

        sName = RendezVousPropagateMessage_get_DestSName(pmsg);
        sParam = RendezVousPropagateMessage_get_DestSParam(pmsg);
        messageId = RendezVousPropagateMessage_get_MessageId(pmsg);

        JXTA_LOG("     MessageId [%s]\n", messageId);
        JXTA_LOG("   ServiceName [%s]\n", sName);
        JXTA_LOG("  ServiceParam [%s]\n", sParam);

        realDest = jxta_endpoint_address_new2(NULL, NULL, sName, sParam);

        /**
         ** Invoke the endpoint service demux again with the new addresses
         **/

        jxta_message_set_destination(msg, realDest);

        jxta_endpoint_service_demux(jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self)->endpoint,
                                    msg);

        JXTA_OBJECT_RELEASE(pmsg);
        return;

    } else {
        JXTA_LOG("RendezVousPropagateMessage element not in message: drop message.\n");
    }
}

/*
 * Search for RDV within a peergroup using the parent group
 *
 */
Jxta_status jxta_search_rdv_peergroup(_jxta_rdv_service * rdv)
{
    /* 
     * construct the Pipe advertisement
     */
    Jxta_pipe_adv *pipeAdv = jxta_rdv_get_wirepipeadv(rdv);

    if (pipeAdv == NULL)
        return JXTA_FAILED;

    jxta_rdv_send_rdv_request(rdv, pipeAdv);

    JXTA_OBJECT_RELEASE(pipeAdv);

    return JXTA_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
