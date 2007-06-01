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
 * $Id: jxta_rdv_service_client.c,v 1.160 2005/11/23 03:12:49 slowhog Exp $
 **/


/**
 *
 * This is the edge Client Rendezvous Service implementation
 *
 * And edge client is capable to connect to rendezvous peers, send and receive
 * propagated messages. An edge client is not capable of acting as a rendezvous
 * nor to re-propagate a received propagated message.
 *
 * An edge client can retrieve the list of rendezvous peers it is connected to,
 * but is not capable of setting them (only configuration or self discovery).
 **/

static const char *__log_cat = "RdvClient";

#include <limits.h>

#include "jxta_apr.h"

#include "jxta_log.h"
#include "jxta_object.h"
#include "jxta_object_type.h"
#include "jstring.h"
#include "jxta_vector.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_pm.h"
#include "jxta_peergroup.h"
#include "jxta_peerview.h"
#include "jxta_id.h"
#include "jxta_peer_private.h"
#include "jxta_peerview_priv.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
/**
 * "Normal" time the background thread sleeps between runs. "Normal" usually
 * means that we are connected to a rdv.
 * Measured in relative microseconds.
                                                                                                                         **/
    static const apr_interval_time_t CONNECT_THREAD_NAP_TIME_NORMAL = ((apr_interval_time_t) 60) * 1000 * 1000;
                                                                                                                        /* 1 Minute */
/**
 * "Fast" time the background thread sleeps between runs. "Fast" usually
 * means that we are not connected to a rdv.
 * Measured in relative microseconds.
 **/
static const apr_interval_time_t CONNECT_THREAD_NAP_TIME_FAST = ((apr_interval_time_t) 2) * 1000 * 1000;        /* 2 Seconds */

/**
 * Minimal number of rendezvous this client will attempt to connect to.
 * If there is fewer connected rendezvous, the Rendezvous Service
 * becomes more aggressive trying to connect to rendezvous peers.
 **/
static const unsigned int MIN_NB_OF_CONNECTED_RDVS = 1;

/**
 * Minimum delay between two connection attempts to the same peer.
 **/
static const Jxta_time_diff MIN_RETRY_DELAY = ((Jxta_time_diff) 15) * 1000;     /* 15 Seconds */
/**
 * Renewal of the leases must be done before the lease expires. The following
 * constant defines the number of microseconds to substract to the expiration
 * time in order to set the time of renewal.
 **/
static const Jxta_time_diff LEASE_RENEWAL_DELAY = ((Jxta_time_diff) 5) * 60 * 1000;     /* 5 Minutes */
/**
 *  Need to delay the RDV connection in case we have a relay
 **/
static const apr_interval_time_t RDV_RELAY_DELAY_CONNECTION = ((apr_interval_time_t) 100) * 1000;       /* 100 ms */
/**
 * Internal structure of a peer used by this service.
 **/ struct _jxta_peer_rdv_entry {
    Extends(_jxta_peer_entry);

/**
    * Time of the last reconnection attempt.
 **/
    Jxta_time lastConnectTry;

/**
    * Interval at which we are performing reconnect attempts.
    * Starts at MIN_RETRY_DELAY and increases at MIN_RETRY_DELAY until
    * MAX_RETRY_DELAY is reached. Usually the peer expires or connection is
    * acheived before this happens.
 **/
    Jxta_time_diff connectInterval;
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

/**
 ** This is the Rendezvous Service Client internal object definition
 ** The first field in the structure MUST be of type _jxta_rdv_service_provider
 ** since that is the generic interface of a Rendezvous Service. 
 ** This base structure is a Jxta_object that needs to be initialized.
 **/
struct _jxta_rdv_service_client {
    Extends(_jxta_rdv_service_provider);

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    volatile Jxta_boolean running;
    apr_thread_cond_t *periodicCond;
    apr_thread_mutex_t *periodicMutex;
    volatile apr_thread_t *periodicThread;

    /* configuration */

    char *groupiduniq;
    char *assigned_idString;
    Jxta_discovery_service *discovery;
    Jxta_PA *localPeerAdv;
    Jxta_PID *localPeerId;
    Jxta_listener *listener_service;

    /* state */

    Jxta_hashtable *rdvs;
};

/**
    Private typedef for the JXTA Rendezvous service client object.
**/
typedef struct _jxta_rdv_service_client _jxta_rdv_service_client;

/**
 ** Forward definition to make the compiler happy
 **/

static _jxta_rdv_service_client *jxta_rdv_service_client_construct(_jxta_rdv_service_client * self,
                                                                   const _jxta_rdv_service_provider_methods * methods);
static void rdv_client_delete(Jxta_object * service);
static void jxta_rdv_service_client_destruct(_jxta_rdv_service_client * self);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam);

static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *self);
static void JXTA_STDCALL client_listener(Jxta_object * msg, void *arg);

static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * self, Jxta_id * peerid, Jxta_boolean create);

static _jxta_peer_rdv_entry *rdv_entry_new(void);
static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * self, const _jxta_rdv_entry_methods * methods);
static void rdv_entry_delete(Jxta_object * addr);
static void rdv_entry_destruct(_jxta_peer_rdv_entry * self);

static const _jxta_rdv_service_provider_methods JXTA_RDV_SERVICE_CLIENT_METHODS = {
    "_jxta_rdv_service_provider_methods",
    init,
    start,
    stop,
    get_peers,
    propagate,
    walk
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

static _jxta_peer_rdv_entry *rdv_entry_construct(_jxta_peer_rdv_entry * self, const _jxta_rdv_entry_methods * methods)
{
    self = (_jxta_peer_rdv_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        PTValid(methods, _jxta_rdv_entry_methods);
        PEER_ENTRY_VTBL(self) = (Jxta_Peer_entry_methods *) methods;
        self->thisType = "_jxta_peer_rdv_entry";

        self->lastConnectTry = 0;
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
    _jxta_peer_rdv_entry *self = (_jxta_peer_rdv_entry *) calloc(1, sizeof(_jxta_peer_rdv_entry));

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, rdv_entry_delete, 0);

    return rdv_entry_construct(self, &JXTA_RDV_ENTRY_METHODS);
}

/**
 * Standard instantiator
 **/
Jxta_rdv_service_provider* jxta_rdv_service_client_new(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) calloc(1, sizeof(_jxta_rdv_service_client));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, rdv_client_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service_provider *) jxta_rdv_service_client_construct(self, &JXTA_RDV_SERVICE_CLIENT_METHODS);
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

        self->pool = ((_jxta_rdv_service_provider *) self)->pool;

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
        self->rdvs = jxta_hashtable_new_0(MIN_NB_OF_CONNECTED_RDVS * 2, TRUE);

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
    /* Free the hashtable of peers */
    JXTA_OBJECT_RELEASE(self->rdvs);

    /* Release the services object this instance was using */
    if (self->discovery != NULL) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    /* Release the local peer adv and local peer id */
    JXTA_OBJECT_RELEASE(self->localPeerAdv);
    JXTA_OBJECT_RELEASE(self->localPeerId);

    /* Release our references */
    if (self->assigned_idString != 0)
        free(self->assigned_idString);

    /* Free the string that contains the name of the instance */
    free(self->groupiduniq);

    apr_thread_mutex_destroy(self->mutex);
    apr_thread_mutex_destroy(self->periodicMutex);
    apr_thread_cond_destroy(self->periodicCond);

    /* call the base classe's dtor. */
    jxta_rdv_service_provider_destruct((_jxta_rdv_service_provider *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}


/**
 * Initializes an instance of the Rendezvous Service. (a module method)
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service.
 * @param group a pointer to the PeerGroup the Rendezvous Service is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * @return error code.
 **/

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);
    JString *tmp = NULL;
    Jxta_PGID *gid;
    JString *string = NULL;
    Jxta_id *assigned_id = jxta_service_get_assigned_ID_priv((_jxta_service *) service);
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) service);

    jxta_rdv_service_provider_init(provider, service);

    jxta_id_to_jstring(assigned_id, &string);

    self->assigned_idString = malloc(jstring_length(string) + 1);

    strcpy(self->assigned_idString, jstring_get_string(string));

    JXTA_OBJECT_RELEASE(string);

    jxta_PG_get_discovery_service(group, &(self->discovery));
    jxta_PG_get_PID(group, &(self->localPeerId));
    jxta_PG_get_PA(group, &(self->localPeerAdv));

    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &tmp);
    JXTA_OBJECT_RELEASE(gid);
    self->groupiduniq = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);

    /**
     ** Add the Rendezvous Service Endpoint Listener
     **/

    self->listener_service = jxta_listener_new(client_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(provider->service->endpoint, JXTA_RDV_SERVICE_NAME, self->groupiduniq,
                                       self->listener_service);

    jxta_listener_start(self->listener_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client initialized for group %s\n", self->groupiduniq);

    return JXTA_SUCCESS;
}


/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically).
 *
 * @param provider The provider base class for which we are working.
 **/
static Jxta_status start(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);

    jxta_rdv_service_provider_lock_priv(provider);

    res = jxta_rdv_service_provider_start(provider);

    apr_thread_mutex_lock(self->periodicMutex);

    self->running = TRUE;

    apr_thread_create((apr_thread_t**)&self->periodicThread, NULL,      /* no attr */
                      periodic_thread_main, (void *) JXTA_OBJECT_SHARE(self), jxta_rdv_service_provider_get_pool_priv(provider));

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_rdv_generate_event((_jxta_rdv_service *)
                            jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self),
                            JXTA_RDV_BECAME_EDGE, self->localPeerId);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client started for group %s\n", self->groupiduniq);

    return JXTA_SUCCESS;
}

/**
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static Jxta_status stop(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);

    jxta_rdv_service_provider_lock_priv(provider);
    
    if (TRUE != self->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV client is not started, ignore stop request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        return APR_SUCCESS;
    }

    apr_thread_mutex_lock(self->periodicMutex);

    /* We need to tell the background thread that it has to die. */
    self->running = FALSE;
    apr_thread_cond_signal(self->periodicCond);

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_endpoint_service_remove_listener(provider->service->endpoint, JXTA_RDV_SERVICE_NAME, self->groupiduniq);

    if (self->listener_service) {
        jxta_listener_stop(self->listener_service);
        JXTA_OBJECT_RELEASE(self->listener_service);
        self->listener_service = NULL;
    }

    /* Release the services object this instance was using */
    if (self->discovery != NULL) {
        JXTA_OBJECT_RELEASE(self->discovery);
        self->discovery = NULL;
    }

    /**
    *  FIXME 20050203 bondolo Should send a disconnect.
    **/
    /* jxta_hashtable_clear(self->rdvs); */

    res = jxta_rdv_service_provider_stop(provider);

    apr_thread_join(&res, (apr_thread_t*)self->periodicThread);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");

    return res;
}


/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 **/
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist)
{
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(provider, _jxta_rdv_service_client);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /*
     * Allocate a new vector and stores it the peers from our own vector: we 
     * don't want to share the our vector itself.
     */
    *peerlist = jxta_hashtable_values_get(self->rdvs);

    if (NULL == *peerlist) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot allocate a new jxta_vector\n");
        return JXTA_NOMEM;
    }

    return JXTA_SUCCESS;
}

/**
 * Propagates a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param ttl Maximum number of peers the propagated message can go through.
 * @return error code.
 **/
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl)
{
    Jxta_status res;

    res = jxta_rdv_service_provider_update_prophdr(provider, msg, serviceName, serviceParam, ttl);

    if (JXTA_SUCCESS == res) {
        /* Send the message to ourself */
        jxta_rdv_service_provider_prop_listener( (Jxta_object*)msg, provider );
    
        res = jxta_rdv_service_provider_prop_to_peers(provider, msg, TRUE);
    }

    return res;
}

/**
 * Walks a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @return error code.
 **/
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam)
{
    Jxta_status res;

    res = jxta_rdv_service_provider_update_prophdr(provider, msg, serviceName, serviceParam, 2);

    if (JXTA_SUCCESS == res) {
        res = jxta_rdv_service_provider_prop_to_peers(provider, msg, FALSE);
    }

    return res;
}

/**
*   Send a connect request message to the specified peer.
 *
*   @param self Our "this" pointer.
*   @param peer The peer to which we will attemt to connect.
 **/
static void connect_to_peer(_jxta_rdv_service_client * self, _jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = (Jxta_time) jpr_time_now();
    Jxta_message *msg;
    Jxta_message_element *msgElem;
    Jxta_endpoint_address *destAddr;
    JString *localPeerAdvStr = NULL;
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *address;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Requesting Rdv lease from %s://%s\n",
                    jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

    jxta_peer_lock((Jxta_peer *) peer);

    if ((peer->lastConnectTry + peer->connectInterval) > currentTime) {
        /* Not time yet to try to connect */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Too soon for another connect.\n");
        jxta_peer_unlock((Jxta_peer *) peer);
        return;
    }

    peer->lastConnectTry = currentTime;
    peer->connectInterval += MIN_RETRY_DELAY;

    jxta_peer_unlock((Jxta_peer *) peer);

    /*
     * Create a message with a connection request and build the request.
     */
    msg = jxta_message_new();

    jxta_PA_get_xml(self->localPeerAdv, &localPeerAdvStr);
    msgElem = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_RDV_CONNECT_REQUEST_ELEMENT_NAME,
                                         "text/xml", jstring_get_string(localPeerAdvStr), jstring_length(localPeerAdvStr), NULL);

    JXTA_OBJECT_RELEASE(localPeerAdvStr);
    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);

    /*
     * Send the message
     *
     */

    /*
     * Set the destination address of the message.
     */
    address = jxta_peer_get_address_priv((Jxta_peer *) peer);
    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(address),
                                           jxta_endpoint_address_get_protocol_address(address),
                                           self->assigned_idString, self->groupiduniq);


    res = jxta_endpoint_service_send(jxta_service_get_peergroup_priv((_jxta_service *)
                                                                     ((_jxta_rdv_service_provider *) self)->service),
                                     ((_jxta_rdv_service_provider *) self)->service->endpoint, msg, destAddr);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send connect message\n");
        jxta_peer_set_expires((Jxta_peer *) peer, jpr_time_now());
    }

    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(msg);
}

/**
* @return TRUE if there is remaining time left in the lease otherwise FALSE.
**/
static Jxta_boolean check_peer_lease(_jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    Jxta_time_diff remaining;

    jxta_peer_lock((Jxta_peer *) peer);

    expires = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);

    jxta_peer_unlock((Jxta_peer *) peer);

    remaining = (Jxta_time_diff) expires - currentTime;

    /* any lease remaining ? */
    return (remaining > 0);
}

static Jxta_boolean check_peer_connect(_jxta_rdv_service_client * self, _jxta_peer_rdv_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    Jxta_time lastConnectTry;
    Jxta_time_diff connectInterval;

    jxta_peer_lock((Jxta_peer *) peer);

    expires = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);
    lastConnectTry = peer->lastConnectTry;
    connectInterval = peer->connectInterval;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer->address = %s://%s\n",
                    jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                    jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->address));

    jxta_peer_unlock((Jxta_peer *) peer);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer->expires= " JPR_DIFF_TIME_FMT " ms\n",
                    (0 == expires) ? 0 : (expires - currentTime));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer->lastConnectTry= " JPR_DIFF_TIME_FMT " ms\n",
                    (0 == lastConnectTry) ? 0 : (currentTime - lastConnectTry));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "peer->connectInterval= " JPR_DIFF_TIME_FMT " ms\n", connectInterval);

    if (((Jxta_time_diff) (expires - currentTime)) < LEASE_RENEWAL_DELAY) {
        /* Time to try a connection */
        connect_to_peer(self, peer);
    }

    return TRUE;
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
static Jxta_status process_connected_reply(_jxta_rdv_service_client * self, Jxta_message * msg)
{
    Jxta_status status = JXTA_SUCCESS;
    Jxta_message_element *el = NULL;
    JString *rdv_peerid_str = NULL;
    Jxta_id *rdv_peerid = NULL;
    Jxta_PA *rdv_PA = NULL;
    JString *rdv_PA_name = NULL;
    Jxta_time_diff lease = 0;
    _jxta_peer_rdv_entry *peer;
    Jxta_Rendezvous_event_type lease_event;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Processing rdv message [%p]\n", msg);

    el = NULL;
    status = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_CONNECTED_REPLY_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS == status) && (NULL != el)) {
        /*
         * This element contains the peer id of the rendezvous peer
         * that has granted the lease.
         */
        Jxta_bytevector *value = jxta_message_element_get_value(el);

        /* Build the proper endpoint address */
        rdv_peerid_str = jstring_new_3(value);
        JXTA_OBJECT_RELEASE(value);
        value = NULL;

        jxta_id_from_jstring(&rdv_peerid, rdv_peerid_str);
        JXTA_OBJECT_RELEASE(el);
    }

    if (NULL == rdv_peerid) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not recover peer id in lease offer\n");
        status = JXTA_FAILED;
        goto Common_exit;
    }

    el = NULL;
    status = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_RDVADV_REPLY_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS == status) && (NULL != el)) {
        /**
         ** This element contains the peer id of the rendezvous peer
         ** that has granted the lease.
         **/
        Jxta_bytevector *value = jxta_message_element_get_value(el);

        rdv_PA = jxta_PA_new();
        if (rdv_PA != NULL) {
            JString *string = jstring_new_3(value);
            jxta_PA_parse_charbuffer(rdv_PA, jstring_get_string(string), jstring_length(string));
            JXTA_OBJECT_RELEASE(string);
            string = NULL;

            if (self->discovery == NULL) {
                apr_thread_mutex_lock(self->mutex);

                jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((_jxta_service *)
                                                                              jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self)), &(self->discovery));
                apr_thread_mutex_unlock(self->mutex);
            }

            if (self->discovery != NULL) {
                discovery_service_publish(self->discovery, (Jxta_advertisement *) rdv_PA, DISC_PEER, (Jxta_expiration_time)
                                          DEFAULT_EXPIRATION, (Jxta_expiration_time) DEFAULT_EXPIRATION);
            }
        }
        JXTA_OBJECT_RELEASE(value);
        JXTA_OBJECT_RELEASE(el);
    }

    if (NULL == rdv_PA) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not recover peer adv in lease offer\n");
        status = JXTA_FAILED;
        goto Common_exit;
    }
    rdv_PA_name = jxta_PA_get_Name(rdv_PA);

    el = NULL;
    status = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_LEASE_REPLY_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS == status) && (NULL != el)) {
        /*
         * This element contains the lease that has been granted in milli-seconds
         */
        Jxta_bytevector *value = jxta_message_element_get_value(el);
        unsigned int length = jxta_bytevector_size(value);
        char *bytes = calloc(length + 1, sizeof(char));

        jxta_bytevector_get_bytes_at(value, (unsigned char *) bytes, 0, length);
        JXTA_OBJECT_RELEASE(value);
        value = NULL;

        bytes[length] = 0;

        /*
         * convert to Jxta_time_diff
         */

        sscanf(bytes, JPR_DIFF_TIME_FMT, &lease);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Got lease: " JPR_DIFF_TIME_FMT " milliseconds\n", lease);
        free(bytes);
        bytes = NULL;
        JXTA_OBJECT_RELEASE(el);
    }

    if (lease <= 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not recover valid lease in lease offer\n");
    }

    peer = get_peer_entry(self, rdv_peerid, (lease > 0));

    if (NULL == peer) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Ignoring lease offer from \"%s\" [%s] \n",
                        jstring_get_string(rdv_PA_name), jstring_get_string(rdv_peerid_str));
        goto Common_exit;
    }

    JXTA_OBJECT_CHECK_VALID(peer);

    jxta_peer_lock((Jxta_peer *) peer);

    if (lease > 0) {
        Jxta_time currentTime = jpr_time_now();

        lease_event =
            jxta_peer_get_expires((Jxta_peer *) peer) != JPR_ABSOLUTE_TIME_MAX ? JXTA_RDV_RECONNECTED : JXTA_RDV_CONNECTED;

        jxta_peer_set_expires((Jxta_peer *) peer, currentTime + lease);
        jxta_peer_set_adv((Jxta_peer *) peer, rdv_PA);
        peer->connectInterval = 0;
        peer->lastConnectTry = currentTime;
    } else {
        jxta_peer_set_expires((Jxta_peer *) peer, 0);
        lease_event = JXTA_RDV_DISCONNECTED;
    }

    jxta_peer_unlock((Jxta_peer *) peer);

    /*
     *  notify the RDV listeners about the lease activity.
     */
    jxta_rdv_generate_event((_jxta_rdv_service *)
                            jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self),
                            lease_event, rdv_peerid);

    JXTA_OBJECT_RELEASE(peer);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous connection with \"%s\" [%s] for " JPR_DIFF_TIME_FMT " ms\n",
                    jstring_get_string(rdv_PA_name), jstring_get_string(rdv_peerid_str), lease);

  Common_exit:
    if (NULL != rdv_peerid) {
        JXTA_OBJECT_RELEASE(rdv_peerid);
    }

    if (NULL != rdv_peerid_str) {
        JXTA_OBJECT_RELEASE(rdv_peerid_str);
    }

    if (NULL != rdv_PA_name) {
        JXTA_OBJECT_RELEASE(rdv_PA_name);
    }

    if (NULL != rdv_PA) {
        JXTA_OBJECT_RELEASE(rdv_PA);
    }

    return status;
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - grant of lease from the rendezvous peer.
 **   - deny of a lease.
 **/
static void JXTA_STDCALL client_listener(Jxta_object * obj, void *arg)
{
    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);
    Jxta_message_element *el = NULL;
    Jxta_status res = JXTA_SUCCESS;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Service Client received a message\n");

    JXTA_OBJECT_CHECK_VALID(msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_CONNECTED_REPLY_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS == res) && (NULL != el)) {
        /**
         ** This is a response to a lease request.
         **/
        process_connected_reply(self, msg);

        JXTA_OBJECT_RELEASE(el);
    }
}

/**
 *  The entry point of the thread that maintains the rendezvous connection on a
 *  periodic basis.
 *
 *  @param thread The thread we are running on.
 *  @param arg Our object.
 **/
static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *arg)
{
    Jxta_rdv_service_provider *provider = PTValid(arg, _jxta_rdv_service_provider);
    _jxta_rdv_service_client *self = (_jxta_rdv_service_client *) PTValid(arg, _jxta_rdv_service_client);
    Jxta_status res;
    Jxta_transport *transport = NULL;
    int delay_count;
    Jxta_vector *seeds = NULL;
    Jxta_vector *rdvAdvs = NULL;
    Jxta_vector *candidates = NULL;
    _jxta_peer_rdv_entry *candidate = NULL;

    apr_thread_mutex_lock(self->periodicMutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client worker thread started.\n");

    /*
     * FIXME 20040607  tra
     * We need to artificially delay the RDV service in case
     * we do have a relay transport. The relay transport should be the
     * first service to initiate the connection so the Relay server
     * can catch the incoming connection and keep it as a relay
     * connection
     */

    transport = jxta_endpoint_service_lookup_transport(provider->service->endpoint, "jxta");
    if (transport != NULL) {
        res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, RDV_RELAY_DELAY_CONNECTION);
        JXTA_OBJECT_RELEASE(transport);
    }

    /*
     * exponential backoff delay to avoid constant
     * retry to RDV seed.
     */
    delay_count = 1;
    /* Main loop */
    while (self->running) {
        unsigned int nbOfConnectedRdvs = 0;
        unsigned int i;
        Jxta_vector *rdvs = jxta_hashtable_values_get(self->rdvs);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Client Periodic RUN.\n");

        for (i = 0; i < jxta_vector_size(rdvs); i++) {
            _jxta_peer_rdv_entry *peer;

            res = jxta_vector_get_object_at(rdvs, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }

            if (check_peer_lease(peer)) {
                ++nbOfConnectedRdvs;
            } else {
                /** No lease left. Get rid of this rdv **/
                char *addrStr = jxta_endpoint_address_to_string(jxta_peer_get_address_priv((Jxta_peer *) peer));

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removing expired RDV : %s\n", addrStr);

                jxta_rdv_service_provider_lock_priv((Jxta_rdv_service_provider *) self);

                res = jxta_hashtable_del(self->rdvs, addrStr, strlen(addrStr) + 1, NULL);

                jxta_rdv_service_provider_unlock_priv((Jxta_rdv_service_provider *) self);

                jxta_rdv_generate_event((_jxta_rdv_service *)
                                        jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self),
                                        JXTA_RDV_FAILED, jxta_peer_get_peerid_priv((Jxta_peer *) peer));
                free(addrStr);
            }

            JXTA_OBJECT_RELEASE(peer);
        }

        JXTA_OBJECT_RELEASE(rdvs);

        /*
         *   The second pass, wherein we try to renew any existing leases as needed via check_peer_connect.
         */
        rdvs = jxta_hashtable_values_get(self->rdvs);

        for (i = 0; i < jxta_vector_size(rdvs); i++) {
            _jxta_peer_rdv_entry *peer;

            res = jxta_vector_get_object_at(rdvs, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }

            check_peer_connect(self, peer);

            JXTA_OBJECT_RELEASE(peer);
        }

        JXTA_OBJECT_RELEASE(rdvs);

        /*
         *   The third leg, wherein we try to locate new rdvs as needed.
         */
        if (nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS) {
            if (NULL != seeds) {

                if (jxta_vector_size(seeds) > 0) {
                    /* send a connect to the first peer in the list */
                    Jxta_peer *peer;
                    Jxta_endpoint_address *addr;

                    jxta_vector_remove_object_at(seeds, JXTA_OBJECT_PPTR(&peer), 0);

                    PTValid(peer, _jxta_peer_entry);

                    addr = jxta_peer_get_address_priv(peer);

                    JXTA_OBJECT_CHECK_VALID(addr);

                    jxta_peerview_send_rdv_probe(provider->peerview, addr);

                    JXTA_OBJECT_RELEASE(peer);
                } else {
                    /* The list is empty, try another strategy */
                    JXTA_OBJECT_RELEASE(seeds);
                    seeds = NULL;
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Looking for seed RDV for the peergroup\n");

                jxta_peerview_send_rdv_request(((Jxta_rdv_service_provider *) self)->peerview, FALSE, FALSE);

                seeds = jxta_peerview_get_seeds(provider->peerview);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Found %d seed RDVs in %s\n", jxta_vector_size(seeds),
                                self->groupiduniq);
                delay_count++;
            }

            if (NULL != rdvAdvs) {
                if (jxta_vector_size(rdvAdvs) > 0) {
                    /* send a connect to the first peer in the list */
                    Jxta_RdvAdvertisement *rdvAdv;
                    char *cstr;

                    jxta_vector_remove_object_at(rdvAdvs, JXTA_OBJECT_PPTR(&rdvAdv), 0);
                    cstr = jxta_RdvAdvertisement_get_Name(rdvAdv);

                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Probing seed RDV %s in %s\n", cstr, self->groupiduniq);
                    free(cstr);
                    jxta_peerview_probe_cached_rdvadv(provider->peerview, rdvAdv);

                    JXTA_OBJECT_RELEASE(rdvAdv);
                } else {
                    /* The list is empty, try another strategy */
                    JXTA_OBJECT_RELEASE(rdvAdvs);
                    rdvAdvs = NULL;
                }
            } else {
                /* Try RdvAdvertisements from Discovery */
                if (self->discovery == NULL) {
                    apr_thread_mutex_lock(self->mutex);

                    jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((_jxta_service *)
                                                                                  jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self)), &(self->discovery));
                    apr_thread_mutex_unlock(self->mutex);
                }

                if (self->discovery != NULL) {
                    /* This is *really* async in that we will just use the results
                       in future passes through this same section. */
                    JString *pv_name = jxta_peerview_get_name(provider->peerview);

                    discovery_service_get_remote_advertisements(self->discovery, NULL, DISC_ADV, "RdvServiceName",
                                                                jstring_get_string(pv_name), 10, NULL);

                    res =
                        discovery_service_get_local_advertisements(self->discovery, DISC_ADV, "RdvServiceName",
                                                                   jstring_get_string(pv_name), &rdvAdvs);

                    if (NULL != rdvAdvs) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Discovered %d for seed RDVs in %s\n",
                                        jxta_vector_size(rdvAdvs), self->groupiduniq);
                    }

                    JXTA_OBJECT_RELEASE(pv_name);
                }
            }
        } else {
            /* Don't need any more candidate rdvs */
            if (NULL != seeds) {
                JXTA_OBJECT_RELEASE(seeds);
                seeds = NULL;
            }

            if (NULL != rdvAdvs) {
                JXTA_OBJECT_RELEASE(rdvAdvs);
                rdvAdvs = NULL;
            }

            jxta_vector_clear(candidates);
        }

        /* Final lap -- try to connect to a candidate rdv */
        if ((NULL == candidates) || jxta_vector_size(candidates) == 0) {
            if (NULL != candidates) {
                JXTA_OBJECT_RELEASE(candidates);
                candidates = NULL;
            }

            res = jxta_peerview_get_localview(provider->peerview, &candidates);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Loaded %d rendezvous candidates\n", jxta_vector_size(candidates));
        }


        if (NULL != candidate) {
            if ((candidate->lastConnectTry + candidate->connectInterval) > jpr_time_now()) {
                JXTA_OBJECT_RELEASE(candidate);
                candidate = NULL;
            }
        }

        if ((NULL == candidate) && (jxta_vector_size(candidates) > 0)) {
            _jxta_peer_entry *peer;

            res = jxta_vector_remove_object_at(candidates, JXTA_OBJECT_PPTR(&peer), 0);

            if (res == JXTA_SUCCESS) {
                PTValid(peer, _jxta_peer_entry);

                candidate = rdv_entry_new();

                jxta_peer_set_peerid((Jxta_peer *) candidate, jxta_peer_get_peerid_priv((Jxta_peer *) peer));
                jxta_peer_set_address((Jxta_peer *) candidate, jxta_peer_get_address_priv((Jxta_peer *) peer));

                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Candidate RDV address : %s://%s\n",
                                jxta_endpoint_address_get_protocol_name(jxta_peer_get_address_priv((Jxta_peer *) peer)),
                                jxta_endpoint_address_get_protocol_address(jxta_peer_get_address_priv((Jxta_peer *) peer)));

                connect_to_peer(self, candidate);
                JXTA_OBJECT_RELEASE(peer);
            }
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Client Periodic RUN DONE.\n");

        if (!self->running) {
            break;
        }

        /*
         * Wait for a while until the next iteration.
         *
         * We want to have a better responsiveness (latency) of the mechanism
         * to connect to rendezvous when there isn't enough rendezvous connected.
         * The time to wait could be as small as we want, but the smaller it is, the
         * more CPU it will use. Having two different periods for checking connection
         * is a compromise for latency versus performance.
         */

        if (nbOfConnectedRdvs < MIN_NB_OF_CONNECTED_RDVS) {
            apr_time_t wait_time =
                ((CONNECT_THREAD_NAP_TIME_FAST * delay_count) <
                 CONNECT_THREAD_NAP_TIME_NORMAL) ? CONNECT_THREAD_NAP_TIME_FAST * delay_count : CONNECT_THREAD_NAP_TIME_NORMAL;
            res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, wait_time);
        } else {
            /*
             * reset delay count if we have a connection
             */
            delay_count = 0;
            res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, CONNECT_THREAD_NAP_TIME_NORMAL);
        }
    }

    if (NULL != seeds) {
        JXTA_OBJECT_RELEASE(seeds);
        seeds = NULL;
    }

    if (NULL != candidates) {
        JXTA_OBJECT_RELEASE(candidates);
        candidates = NULL;
    }

    if (NULL != candidate) {
        JXTA_OBJECT_RELEASE(candidate);
        candidate = NULL;
    }

    apr_thread_mutex_unlock(self->periodicMutex);

    JXTA_OBJECT_RELEASE(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Client worker thread exiting.\n");

    apr_thread_exit(thread, JXTA_SUCCESS);

    /* NOTREACHED */
    return NULL;
}

/**
 * Returns the _jxta_peer_rdv_entry associated to a peer or optionally create one
 *
 *  @param self The rendezvous server
 *  @param addr The address of the desired peer.
 *  @param create If TRUE then create a new entry if there is no existing entry 
 **/
static _jxta_peer_rdv_entry *get_peer_entry(_jxta_rdv_service_client * self, Jxta_id * peerid, Jxta_boolean create)
{
    _jxta_peer_rdv_entry *peer = NULL;
    Jxta_status res = 0;
    Jxta_boolean found = FALSE;
    JString *uniq;
    size_t currentPeers;

    jxta_id_get_uniqueportion(peerid, &uniq);

    jxta_rdv_service_provider_lock_priv((Jxta_rdv_service_provider *) self);

    res = jxta_hashtable_get(self->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, JXTA_OBJECT_PPTR(&peer));

    found = res == JXTA_SUCCESS;

    jxta_hashtable_stats(self->rdvs, NULL, &currentPeers, NULL, NULL, NULL);

    if (!found && create && (MIN_NB_OF_CONNECTED_RDVS > currentPeers)) {
        /* We need to create a new _jxta_peer_rdv_entry */
        peer = rdv_entry_new();
        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot create a _jxta_peer_rdv_entry\n");
        } else {
            Jxta_endpoint_address *clientAddr = jxta_endpoint_address_new_2("jxta", jstring_get_string(uniq), NULL, NULL);

            jxta_peer_set_address((Jxta_peer *) peer, clientAddr);
            JXTA_OBJECT_RELEASE(clientAddr);
            jxta_peer_set_peerid((Jxta_peer *) peer, peerid);
            jxta_peer_set_expires((Jxta_peer *) peer, JPR_ABSOLUTE_TIME_MAX);

            jxta_hashtable_put(self->rdvs, jstring_get_string(uniq), jstring_length(uniq) + 1, (Jxta_object *) peer);
        }
    }

    jxta_rdv_service_provider_unlock_priv((Jxta_rdv_service_provider *) self);

    JXTA_OBJECT_RELEASE(uniq);

    return peer;
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vim: set ts=4 sw=4 tw=130 et: */
