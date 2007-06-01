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
 * $Id: jxta_rdv_service_server.c,v 1.39 2005/12/15 03:40:21 slowhog Exp $
 **/


/**
 **
 ** This is the Server Rendezvous Service implementation
 **
 ** A server accepts connections from clients and distributes propagated messages
 ** and queries.
 **/

static const char *__log_cat = "RdvServer";

#include <limits.h>
#include <assert.h>

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
#include "jxta_walk_msg.h"
#include "jxta_apr.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
static const char *LIMITEDRANGEWALKELEMENT = "LimitedRangeRdvMessage";

/**
 * Interval between background thread check iterations. Measured in relative microseconds.
 **/
static const apr_interval_time_t PERIODIC_THREAD_NAP_TIME_NORMAL = ((apr_interval_time_t) 60) * 1000 * 1000;    /* 1 Minute */

/**
*    Default max number of clients
**/
static const int DEFAULT_MAX_CLIENTS = 200;

/**
*    Default duration of leases measured in relative milliseconds.
**/
static const Jxta_time_diff DEFAULT_LEASE_DURATION = (((Jxta_time_diff) 20) * 60 * 1000L);  /* 20 Minutes */

/**
*    Public typedef for the rdv server provider.
**/
typedef struct Jxta_rdv_service_provider Jxta_rdv_service_server;

/**
 * Rendezvous Service Server internal object definition.
 * It extends _jxta_rdv_service_provider.
 *
 * This base structure is a Jxta_object that needs to be initialized.
 **/
struct _jxta_rdv_service_server {
    Extends(_jxta_rdv_service_provider);

    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;

    volatile Jxta_boolean running;
    apr_thread_cond_t *periodicCond;
    apr_thread_mutex_t *periodicMutex;
    volatile apr_thread_t *periodicThread;

    Jxta_listener *listener_service;
    JString *walkerSvc;
    JString *walkerParam;
    Jxta_listener *listener_walker;
    Jxta_discovery_service *discovery;

    /* configuration */
    char *groupiduniq;
    Jxta_PID *localPeerId;
    JString *localPeerIdJString;

    unsigned int MAX_CLIENTS;
    Jxta_time_diff OFFERED_LEASE_DURATION;

    /* state */

    Jxta_PA *localPeerAdv;

    Jxta_hashtable *clients;
};

/**
*    Private typedef for the rdv server provider.
**/
typedef struct _jxta_rdv_service_server _jxta_rdv_service_server;

/**
 ** Internal structure of a peer used by this service.
 **/

struct _jxta_peer_client_entry {
    Extends(_jxta_peer_entry);

    Jxta_time connectTime;
};

typedef struct _jxta_peer_client_entry _jxta_peer_client_entry;

struct _jxta_client_entry_methods {
    Extends(Jxta_Peer_entry_methods);

    /* additional methods */
};

typedef struct _jxta_client_entry_methods _jxta_client_entry_methods;

/**
 * Forward definition to make the compiler happy
 **/

static _jxta_rdv_service_server *jxta_rdv_service_server_construct(_jxta_rdv_service_server * self,
                                                                   const _jxta_rdv_service_provider_methods * methods);
static void rdv_server_delete(Jxta_object * provider);
static void Jxta_rdv_service_server_destruct(_jxta_rdv_service_server * rdv);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam);

static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *self);
static void JXTA_STDCALL server_listener(Jxta_object * msg, void *arg);
static void JXTA_STDCALL walker_listener(Jxta_object * obj, void *me);

static Jxta_status send_connect_reply(_jxta_rdv_service_server * self, _jxta_peer_client_entry * client);
static _jxta_peer_client_entry *get_peer_entry(_jxta_rdv_service_server * self, Jxta_id * peerid, Jxta_boolean create);
static Jxta_boolean check_peer_lease(_jxta_peer_client_entry * peer);

static _jxta_peer_client_entry *client_entry_new(void);
static _jxta_peer_client_entry *client_entry_construct(_jxta_peer_client_entry * self,
                                                       const _jxta_client_entry_methods * methods);
static void client_entry_delete(Jxta_object * addr);
static void client_entry_destruct(_jxta_peer_client_entry * self);

/**
 * This implementation's methods.
 * The typedef could go in Jxta_rdv_service_server_private.h if we wanted
 * to support subclassing of this class.
 **/
static const _jxta_rdv_service_provider_methods JXTA_RDV_SERVICE_SERVER_METHODS = {
    "_jxta_rdv_service_provider_methods",
    init,
    start,
    stop,
    get_peers,
    propagate,
    walk
};

static const _jxta_client_entry_methods JXTA_CLIENT_ENTRY_METHODS = {
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
    "_jxta_client_entry_methods"
};

#define CLIENT_ENTRY_VTBL(self) ((Jxta_Client_entry_methods*) ((_jxta_peer_entry*) (self))->methods)

static _jxta_peer_client_entry *client_entry_new(void)
{
    _jxta_peer_client_entry *self = (_jxta_peer_client_entry *) calloc(1, sizeof(_jxta_peer_client_entry));

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, client_entry_delete, 0);

    return client_entry_construct(self, &JXTA_CLIENT_ENTRY_METHODS);
}

static _jxta_peer_client_entry *client_entry_construct(_jxta_peer_client_entry * self, const _jxta_client_entry_methods * methods)
{
    PEER_ENTRY_VTBL(self) = (const Jxta_Peer_entry_methods *) PTValid(methods, _jxta_client_entry_methods);
    self = (_jxta_peer_client_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        self->thisType = "_jxta_peer_client_entry";

        self->connectTime = 0;
    }

    return self;
}

static void client_entry_delete(Jxta_object * addr)
{
    _jxta_peer_client_entry *self = (_jxta_peer_client_entry *) addr;

    client_entry_destruct(self);

    free(self);
}

static void client_entry_destruct(_jxta_peer_client_entry * self)
{
    peer_entry_destruct((_jxta_peer_entry *) self);
}

/**
 * Standard instantiator
 */
Jxta_rdv_service_provider *jxta_rdv_service_server_new(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_server *self = (_jxta_rdv_service_server *) calloc(1, sizeof(_jxta_rdv_service_server));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, rdv_server_delete, 0);

    /* call the hierarchy of ctors. */
    return (Jxta_rdv_service_provider *) jxta_rdv_service_server_construct(self, &JXTA_RDV_SERVICE_SERVER_METHODS);
}

/*
 * Make sure we have a ctor that subclassers can call.
 */
static _jxta_rdv_service_server *jxta_rdv_service_server_construct(_jxta_rdv_service_server * self,
                                                                   const _jxta_rdv_service_provider_methods * methods)
{
    apr_status_t res = APR_SUCCESS;

    self = (_jxta_rdv_service_server *) jxta_rdv_service_provider_construct((_jxta_rdv_service_provider *) self, methods);

    if (NULL != self) {
        /* Set our rt type checking string */
        self->thisType = "_jxta_rdv_service_server";

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

        /** Allocate a table for storing the list of peers **/
        self->clients = jxta_hashtable_new_0(DEFAULT_MAX_CLIENTS, TRUE);
        self->running = FALSE;
    }

    return self;
}

/**
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
static void Jxta_rdv_service_server_destruct(_jxta_rdv_service_server * rdv)
{
    _jxta_rdv_service_server *self = PTValid(rdv, _jxta_rdv_service_server);

    /* Free the table of peers */
    JXTA_OBJECT_RELEASE(self->clients);

    /* Release the services object this instance was using */
    if (self->discovery != NULL)
        JXTA_OBJECT_RELEASE(self->discovery);

    /* Release the local peer adv and local peer id */
    JXTA_OBJECT_RELEASE(self->localPeerAdv);
    JXTA_OBJECT_RELEASE(self->localPeerId);
    JXTA_OBJECT_RELEASE(self->localPeerIdJString);

    JXTA_OBJECT_RELEASE(self->walkerSvc);
    JXTA_OBJECT_RELEASE(self->walkerParam);

    /* Release our reference to the group and impl_adv. del our id */

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
 * Frees an instance of the Rendezvous Server.
 * All objects and memory used by the instance must first be released/freed.
 *
 * @param service a pointer to the instance of the Rendezvous Service to free.
 * @return error code.
 **/
static void rdv_server_delete(Jxta_object * provider)
{
    /* call the hierarchy of dtors */
    Jxta_rdv_service_server_destruct((_jxta_rdv_service_server *) provider);

    /* free the object itself */
    free((void *) provider);
}

/**
 * Initializes an instance of the Rendezvous Server. (providers method)
 * 
 * @param provider this instance.
 * @param servicer The rendezvous service instance we are working for.
 * @return error code.
 **/
static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_server *self = PTValid(provider, _jxta_rdv_service_server);
    Jxta_PGID *gid;
    Jxta_id *assignedID;
    JString *string = NULL;
    JString *gidString = NULL;
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) service);
    Jxta_PA *conf_adv = NULL;
    Jxta_RdvConfigAdvertisement *rdvConfig = NULL;

    jxta_rdv_service_provider_init(provider, service);

    jxta_PG_get_discovery_service(group, &self->discovery);
    jxta_PG_get_PID(group, &self->localPeerId);
    jxta_PG_get_PA(group, &self->localPeerAdv);

    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &string);
    jxta_id_to_jstring(gid, &gidString);
    JXTA_OBJECT_RELEASE(gid);
    self->groupiduniq = strdup(jstring_get_string(string));
    JXTA_OBJECT_RELEASE(string);

    /* Build the JString that contains the string representation of the ID */
    jxta_id_to_jstring(self->localPeerId, &self->localPeerIdJString);

    /*
     * Register Endpoint Listener for Rendezvous Client-Server protocol.
     */
    self->listener_service = jxta_listener_new(server_listener, (void *) self, 10, 50);

    jxta_endpoint_service_add_listener(service->endpoint, JXTA_RDV_SERVICE_NAME, self->groupiduniq, self->listener_service);

    jxta_listener_start(self->listener_service);

    /*
     * Register Endpoint Listener for Rendezvous Client-Server protocol.
     */
    self->listener_walker = jxta_listener_new(walker_listener, (void *) self, 10, 50);

    self->walkerSvc = jstring_new_0();
    self->walkerParam = NULL;

    jstring_append_2(self->walkerSvc, "LR-Greeter");
    jstring_append_1(self->walkerSvc, gidString);
    JXTA_OBJECT_RELEASE(gidString);

    assignedID = jxta_service_get_assigned_ID_priv((Jxta_service *) service);
    jxta_id_to_jstring(assignedID, &self->walkerParam);
    jstring_append_2(self->walkerParam, self->groupiduniq);

    jxta_endpoint_service_add_listener(service->endpoint, jstring_get_string(self->walkerSvc),
                                       jstring_get_string(self->walkerParam), self->listener_walker);

    jxta_listener_start(self->listener_walker);

    self->MAX_CLIENTS = DEFAULT_MAX_CLIENTS;
    self->OFFERED_LEASE_DURATION = DEFAULT_LEASE_DURATION;

    jxta_PG_get_configadv(group, &conf_adv);

    if (conf_adv != NULL) {
        Jxta_vector *svcs = jxta_PA_get_Svc(conf_adv);
        Jxta_svc *svc = NULL;
        unsigned int sz;
        unsigned int i;

        JXTA_OBJECT_RELEASE(conf_adv);

        sz = jxta_vector_size(svcs);
        for (i = 0; i < sz; i++) {
            Jxta_id *mcid;
            Jxta_svc *tmpsvc = NULL;
            jxta_vector_get_object_at(svcs, JXTA_OBJECT_PPTR(&tmpsvc), i);
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

        if (svc != NULL) {
            rdvConfig = jxta_svc_get_RdvConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
    }

    if (NULL == rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Loading defaults\n");
        rdvConfig = jxta_RdvConfigAdvertisement_new();
    }

    if (-1 != jxta_RdvConfig_get_lease_duration(rdvConfig)) {
        self->OFFERED_LEASE_DURATION = jxta_RdvConfig_get_lease_duration(rdvConfig);
    }

    if (-1 != jxta_RdvConfig_get_max_clients(rdvConfig)) {
        self->MAX_CLIENTS = jxta_RdvConfig_get_max_clients(rdvConfig);
    }

    JXTA_OBJECT_RELEASE(rdvConfig);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Inited for %s\n", self->groupiduniq);

    return JXTA_SUCCESS;
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a provider method)
 *
 * @param provider a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static Jxta_status start(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_server *self = PTValid(provider, _jxta_rdv_service_server);
    Jxta_status res;

    jxta_rdv_service_provider_lock_priv(provider);

    res = jxta_rdv_service_provider_start(provider);

    res = jxta_peerview_start(((Jxta_rdv_service_provider *) self)->peerview);

    apr_thread_mutex_lock(self->periodicMutex);

    /* Mark the service as running now. */
    self->running = TRUE;

    apr_thread_create((apr_thread_t **) & self->periodicThread, NULL,   /* no attr */
                      (apr_thread_start_t) periodic_thread_main, (void *) JXTA_OBJECT_SHARE(self),
                      jxta_rdv_service_provider_get_pool_priv(provider));

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Started for %s\n", self->groupiduniq);

    jxta_rdv_service_provider_unlock_priv(provider);

    jxta_rdv_generate_event((_jxta_rdv_service *)
                            jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self),
                            JXTA_RDV_BECAME_RDV, self->localPeerId);

    return JXTA_SUCCESS;
}

/**
 * Stops an instance of the Rendezvous server. (a provider method)
 * Note that the caller still needs to release the provider object
 * in order to really free the instance.
 * 
 * @param provider a pointer to the instance of the Rendezvous provider
 * @return error code.
 **/
static Jxta_status stop(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_server *self = PTValid(provider, _jxta_rdv_service_server);
    Jxta_status res;

    jxta_rdv_service_provider_lock_priv(provider);

    if (TRUE != self->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV server is not started, ignore stop request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        return APR_SUCCESS;
    }

    /* We need to tell the background thread that it has to die. */
    apr_thread_mutex_lock(self->periodicMutex);
    self->running = FALSE;
    apr_thread_cond_signal(self->periodicCond);
    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_rdv_service_provider_unlock_priv(provider);

    res = jxta_peerview_stop(((Jxta_rdv_service_provider *) self)->peerview);

    jxta_endpoint_service_remove_listener(provider->service->endpoint, jstring_get_string(self->walkerSvc),
                                          jstring_get_string(self->walkerParam));

    jxta_endpoint_service_remove_listener(provider->service->endpoint, JXTA_RDV_SERVICE_NAME, self->groupiduniq);

    jxta_listener_stop(self->listener_service);

    jxta_listener_stop(self->listener_walker);

    if (self->listener_service) {
        JXTA_OBJECT_RELEASE(self->listener_service);
        self->listener_service = NULL;
    }

    if (self->listener_walker) {
        JXTA_OBJECT_RELEASE(self->listener_walker);
        self->listener_walker = NULL;
    }

    /**
    *  FIXME 20050203 bondolo Should send a disconnect.
    **/
    /* jxta_vector_clear(self->clients); */

    res = jxta_rdv_service_provider_stop(provider);

    apr_thread_join(&res, (apr_thread_t *) self->periodicThread);

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
    _jxta_rdv_service_server *self = (_jxta_rdv_service_server *) PTValid(provider, _jxta_rdv_service_server);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    *peerlist = jxta_hashtable_values_get(self->clients);

    if (NULL == *peerlist) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed getting peers list\n");
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

    res = walk(provider, msg, serviceName, serviceParam);

    res = jxta_rdv_service_provider_update_prophdr(provider, msg, serviceName, serviceParam, ttl);

    if (JXTA_SUCCESS == res) {
        /* Send the message to ourself */
        jxta_rdv_service_provider_prop_listener((Jxta_object *) msg, provider);

        res = jxta_rdv_service_provider_prop_to_peers(provider, msg, TRUE);

    }

    return res;
}

/**
 * Propagates a message within the PeerGroup for which the instance of the 
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
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_server *self = PTValid(provider, _jxta_rdv_service_server);
    unsigned int local_view_size = jxta_peerview_get_localview_size(provider->peerview);
    Jxta_message_element *el = NULL;
    LimitedRangeRdvMessage *header = LimitedRangeRdvMessage_new();
    Walk_direction direction;
    unsigned int useTTL;

    JXTA_OBJECT_CHECK_VALID(msg);

    msg = jxta_message_clone(msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, LIMITEDRANGEWALKELEMENT, &el);

    if ((JXTA_SUCCESS != res) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initiate a walk for msg[%p]\n", msg);
        LimitedRangeRdvMessage_set_TTL(header, local_view_size);
        LimitedRangeRdvMessage_set_direction(header, WALK_BOTH);
        LimitedRangeRdvMessage_set_SrcPeerID(header, jstring_get_string(self->localPeerIdJString));
        LimitedRangeRdvMessage_set_SrcSvcName(header, serviceName);
        LimitedRangeRdvMessage_set_SrcSvcParams(header, serviceParam);
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, "RdvWalkSvcName", "text/plain", serviceName, strlen(serviceName), NULL);
        jxta_message_add_element(msg, el);
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, "RdvWalkSvcParam", "text/plain", serviceParam, strlen(serviceParam),
                                        NULL);
        jxta_message_add_element(msg, el);
    } else {
        JString *string;
        Jxta_bytevector *bytes = jxta_message_element_get_value(el);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Continue to walk for msg[%p]\n", msg);
        jxta_message_remove_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);

        res = LimitedRangeRdvMessage_parse_charbuffer(header, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not parse walk header [%p]\n", msg);
            goto Common_exit;
        }
    }

    /* reduce the TTL */
    useTTL = LimitedRangeRdvMessage_get_TTL(header) - 1;

    /* limit it to *our* localview size */
    useTTL = ((local_view_size - 1) < useTTL) ? (local_view_size - 1) : useTTL;

    LimitedRangeRdvMessage_set_TTL(header, useTTL);

    if (useTTL < 1) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Walk msg[%p] stopped: TTL %d expired with local view size %d\n", msg,
                        useTTL, local_view_size);
        goto Common_exit;
    }

    direction = LimitedRangeRdvMessage_get_direction(header);

    if ((WALK_BOTH == direction) || (WALK_UP == direction)) {
        JString *header_doc;
        Jxta_endpoint_address *destAddr;
        Jxta_message *newmsg = jxta_message_clone(msg);
        Jxta_peer *peer;

        LimitedRangeRdvMessage_set_direction(header, WALK_UP);

        res = LimitedRangeRdvMessage_get_xml(header, &header_doc);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to build LimitedRangeRdvMessage XML\n");
            goto Common_exit;
        }

        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, LIMITEDRANGEWALKELEMENT,
                                        "text/xml", jstring_get_string(header_doc), jstring_length(header_doc), NULL);
        JXTA_OBJECT_RELEASE(header_doc);
        jxta_message_add_element(newmsg, el);
        JXTA_OBJECT_RELEASE(el);

        res = jxta_peerview_get_up_peer(provider->peerview, &peer);

        if (NULL != peer) {
            JString *pidString;
            jxta_id_to_jstring(jxta_peer_get_peerid_priv(peer), &pidString);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Walk the query UP %s\n", jstring_get_string(pidString));
            destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                                   jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->
                                                                                              address),
                                                   jstring_get_string(self->walkerSvc), jstring_get_string(self->walkerParam));
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(pidString);

            /* Send the message */
            res =
                jxta_endpoint_service_send(jxta_service_get_peergroup_priv
                                           ((_jxta_service *) provider->service), provider->service->endpoint, newmsg, destAddr);
            JXTA_OBJECT_RELEASE(destAddr);
        }

        JXTA_OBJECT_RELEASE(newmsg);
    }

    if ((WALK_BOTH == direction) || (WALK_DOWN == direction)) {
        JString *header_doc;
        Jxta_endpoint_address *destAddr;
        Jxta_message *newmsg = jxta_message_clone(msg);
        Jxta_peer *peer;

        LimitedRangeRdvMessage_set_direction(header, WALK_DOWN);

        res = LimitedRangeRdvMessage_get_xml(header, &header_doc);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to build LimitedRangeRdvMessage XML\n");
            goto Common_exit;
        }

        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, LIMITEDRANGEWALKELEMENT,
                                        "text/xml", jstring_get_string(header_doc), jstring_length(header_doc), NULL);
        JXTA_OBJECT_RELEASE(header_doc);
        jxta_message_add_element(newmsg, el);
        JXTA_OBJECT_RELEASE(el);

        res = jxta_peerview_get_down_peer(provider->peerview, &peer);

        if (NULL != peer) {
            JString *pidString;
            jxta_id_to_jstring(jxta_peer_get_peerid_priv(peer), &pidString);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Walk the query DOWN %s\n", jstring_get_string(pidString));
            destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) peer)->address),
                                                   jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) peer)->
                                                                                              address),
                                                   jstring_get_string(self->walkerSvc), jstring_get_string(self->walkerParam));
            JXTA_OBJECT_RELEASE(peer);
            JXTA_OBJECT_RELEASE(pidString);
            /* Send the message */
            res =
                jxta_endpoint_service_send(jxta_service_get_peergroup_priv
                                           ((_jxta_service *) provider->service), provider->service->endpoint, newmsg, destAddr);
            JXTA_OBJECT_RELEASE(destAddr);
        }

        JXTA_OBJECT_RELEASE(newmsg);
    }

  Common_exit:
    JXTA_OBJECT_RELEASE(header);
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

/**
    Message Handler for the client lease protocol.
    
    Listener for :
        <assignedID>/<group-unique>
        
    @param obj The message object.
    @param arg The rdv service server object

 **/
static void JXTA_STDCALL server_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_rdv_service_provider *provider = PTValid(arg, _jxta_rdv_service_provider);
    _jxta_rdv_service_server *self = PTValid(arg, _jxta_rdv_service_server);
    Jxta_message_element *el = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Service Server received a message\n");

    JXTA_OBJECT_CHECK_VALID(msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_CONNECT_REQUEST_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS == res) && (NULL != el)) {
        Jxta_id *client_peerid;
        _jxta_peer_client_entry *client;
        Jxta_bytevector *bytes = jxta_message_element_get_value(el);
        JString *string = jstring_new_3(bytes);
        Jxta_PA *client_PA = jxta_PA_new();

        /*
           This is a connect request
         */
        JXTA_OBJECT_RELEASE(el);
        JXTA_OBJECT_RELEASE(bytes);
        jxta_PA_parse_charbuffer(client_PA, jstring_get_string(string), jstring_length(string));
        JXTA_OBJECT_RELEASE(string);

        /*
           Publish the client's peer advertisement
         */
        if (self->discovery == NULL) {
            jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((_jxta_service *) provider->service),
                                          &(self->discovery));
        }

        if (self->discovery != NULL) {
            discovery_service_publish(self->discovery, (Jxta_advertisement *) client_PA, DISC_PEER, (Jxta_expiration_time)
                                      DEFAULT_EXPIRATION, (Jxta_expiration_time)
                                      DEFAULT_EXPIRATION);
        }

        client_peerid = jxta_PA_get_PID(client_PA);

        jxta_id_get_uniqueportion(client_peerid, &string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Lease Request from %s.\n", jstring_get_string(string));
        JXTA_OBJECT_RELEASE(string);

        client = get_peer_entry(self, client_peerid, TRUE);

        if (NULL != client) {
            Jxta_Rendezvous_event_type lease_event =
                jxta_peer_get_expires((Jxta_peer *) client) !=
                JPR_ABSOLUTE_TIME_MAX ? JXTA_RDV_CLIENT_RECONNECTED : JXTA_RDV_CLIENT_CONNECTED;
            jxta_peer_set_adv((Jxta_peer *) client, client_PA);
            jxta_peer_set_expires((Jxta_peer *) client, jpr_time_now() + self->OFFERED_LEASE_DURATION);

            /*
             * This is a response to a lease request.
             */
            res = send_connect_reply(self, client);

            /*
             *  notify the RDV listeners about the client activity
             */
            jxta_rdv_generate_event(provider->service, lease_event, client_peerid);

            JXTA_OBJECT_RELEASE(client);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Declining to offer lease to %s.\n", jstring_get_string(string));
        }

        /*
         * Release the message: we are done with it.
         */
        JXTA_OBJECT_RELEASE(client_peerid);
        JXTA_OBJECT_RELEASE(client_PA);
    }
}

static Jxta_status send_connect_reply(_jxta_rdv_service_server * self, _jxta_peer_client_entry * client)
{
    Jxta_rdv_service_provider *provider = PTValid(self, _jxta_rdv_service_provider);
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg = jxta_message_new();
    Jxta_message_element *el = NULL;
    JString *peerAdv = NULL;
    Jxta_endpoint_address *destAddr = NULL;
    char leaseStr[64];

    jxta_PA_get_xml(self->localPeerAdv, &peerAdv);

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_RDV_RDVADV_REPLY_ELEMENT_NAME,
                                    "text/xml", jstring_get_string(peerAdv), jstring_length(peerAdv), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);
    JXTA_OBJECT_RELEASE(peerAdv);

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_RDV_CONNECTED_REPLY_ELEMENT_NAME,
                                    "text/plain", jstring_get_string(self->localPeerIdJString),
                                    jstring_length(self->localPeerIdJString), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    apr_snprintf(leaseStr, sizeof(leaseStr), "%" APR_INT64_T_FMT, self->OFFERED_LEASE_DURATION);

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_RDV_LEASE_REPLY_ELEMENT_NAME,
                                    "text/plain", leaseStr, strlen(leaseStr), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    destAddr = jxta_endpoint_address_new_2(jxta_endpoint_address_get_protocol_name(((_jxta_peer_entry *) client)->address),
                                           jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) client)->address),
                                           JXTA_RDV_SERVICE_NAME, self->groupiduniq);

    res = jxta_endpoint_service_send(jxta_service_get_peergroup_priv((_jxta_service *) provider->service),
                                     provider->service->endpoint, msg, destAddr);

    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Failed to send lease offer message. (%s)\n",
                        jxta_endpoint_address_get_protocol_address(((_jxta_peer_entry *) client)->address));
    }

    JXTA_OBJECT_RELEASE(destAddr);
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

/**
 *  The entry point to the periodic thread which manages peer connections.
 **/
static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *arg)
{
    Jxta_rdv_service_provider *provider = PTValid(arg, _jxta_rdv_service_provider);
    _jxta_rdv_service_server *self = PTValid(arg, _jxta_rdv_service_server);
    Jxta_status res;
    Jxta_transport *transport = NULL;
    int delay_count = 0;

    apr_thread_mutex_lock(self->periodicMutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Server periodic thread [TID %p] started.\n",
                    apr_os_thread_current());

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
        res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, PERIODIC_THREAD_NAP_TIME_NORMAL);
        JXTA_OBJECT_RELEASE(transport);
    }

    /*
     * exponential backoff delay to avoid constant
     * retry to RDV seed.
     */
    delay_count = 1;
    /* Main loop */
    while (self->running) {
        Jxta_vector *clients = jxta_hashtable_values_get(self->clients);
        unsigned int sz = jxta_vector_size(clients);
        unsigned int i;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Server periodic thread [TID %p] RUN.\n",
                        apr_os_thread_current());

        for (i = 0; i < sz; i++) {
            _jxta_peer_client_entry *peer;

            res = jxta_vector_get_object_at(clients, JXTA_OBJECT_PPTR(&peer), i);
            if (res != JXTA_SUCCESS) {
                /* We should print a LOG here. Just continue for the time being */
                continue;
            }

            if (!check_peer_lease(peer)) {
                /** No lease left. Get rid of this peer **/
                JString *uniq;

                jxta_id_get_uniqueportion(jxta_peer_get_peerid_priv((Jxta_peer *) peer), &uniq);

                res = jxta_hashtable_del(self->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, NULL);

                JXTA_OBJECT_RELEASE(uniq);

                jxta_rdv_generate_event((_jxta_rdv_service *)
                                        jxta_rdv_service_provider_get_service_priv((_jxta_rdv_service_provider *) self),
                                        JXTA_RDV_CLIENT_DISCONNECTED, jxta_peer_get_peerid_priv((Jxta_peer *) peer));
            }

            JXTA_OBJECT_RELEASE(peer);
        }

        JXTA_OBJECT_RELEASE(clients);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Server periodic thread [TID %p] RUN DONE.\n",
                        apr_os_thread_current());

        /*
         * Wait for a while until the next iteration.
         */
        res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, PERIODIC_THREAD_NAP_TIME_NORMAL);
    }

    apr_thread_mutex_unlock(self->periodicMutex);

    JXTA_OBJECT_RELEASE(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous Server periodic thread [TID %p] stopping.\n", thread);

    apr_thread_exit(thread, JXTA_SUCCESS);

    /* NOTREACHED */
    return NULL;
}

/**
*   Check a peer's lease and return TRUE if there is lease remaining.
*
*   @param peer The peer object whp's lease is being checked.
*   @return TRUE if the lease is stil valid otherwise FALSE.
**/
static Jxta_boolean check_peer_lease(_jxta_peer_client_entry * peer)
{
    Jxta_time currentTime = jpr_time_now();
    Jxta_time expires;
    Jxta_time_diff remaining;

    PEER_ENTRY_VTBL(peer)->jxta_peer_lock((Jxta_peer *) peer);

    expires = PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires((Jxta_peer *) peer);

    PEER_ENTRY_VTBL(peer)->jxta_peer_unlock((Jxta_peer *) peer);

    remaining = expires - currentTime;

    /* any lease remaining ? */
    return (remaining > 0);
}

/**
 * Returns the _jxta_peer_rdv_entry associated to a peer or optionally create one
 *
 *  @param self The rendezvous server
 *  @param addr The address of the desired peer.
 *  @param create If TRUE then create a new entry if there is no existing entry 
 **/
static _jxta_peer_client_entry *get_peer_entry(_jxta_rdv_service_server * self, Jxta_id * peerid, Jxta_boolean create)
{
    _jxta_peer_client_entry *peer = NULL;
    Jxta_status res = 0;
    Jxta_boolean found = FALSE;
    JString *uniq;
    size_t currentPeers;

    jxta_id_get_uniqueportion(peerid, &uniq);

    jxta_rdv_service_provider_lock_priv((Jxta_rdv_service_provider *) self);

    res = jxta_hashtable_get(self->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, JXTA_OBJECT_PPTR(&peer));

    found = res == JXTA_SUCCESS;

    jxta_hashtable_stats(self->clients, NULL, &currentPeers, NULL, NULL, NULL);

    if (!found && create && (self->MAX_CLIENTS > currentPeers)) {
        /* We need to create a new _jxta_peer_rdv_entry */
        peer = client_entry_new();
        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot create a _jxta_peer_rdv_entry\n");
        } else {
            Jxta_endpoint_address *clientAddr = jxta_endpoint_address_new_2("jxta", jstring_get_string(uniq), NULL, NULL);

            jxta_peer_set_address((Jxta_peer *) peer, clientAddr);
            jxta_peer_set_peerid((Jxta_peer *) peer, peerid);
            jxta_peer_set_expires((Jxta_peer *) peer, JPR_ABSOLUTE_TIME_MAX);

            jxta_hashtable_put(self->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, (Jxta_object *) peer);
            JXTA_OBJECT_RELEASE(clientAddr);
        }
    }

    jxta_rdv_service_provider_unlock_priv((Jxta_rdv_service_provider *) self);

    JXTA_OBJECT_RELEASE(uniq);

    return peer;
}

/*
    Message Handler for the walker protocol.
    
    Listener for :
        "LR-Greeter"<groupuniq>/<assignedid><groupid>
        
    @param obj The message object.
    @param arg The rdv service server object
    @remarks When a walk message was received, what the listener will do is simply pass it to the handler based on the destination
             of the internal message. The handler then should decide what to do, it can continue the walk by pass the msg with
             walk header back to rdv.
 */
static void JXTA_STDCALL walker_listener(Jxta_object * obj, void *me)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_rdv_service_provider *provider = PTValid(me, _jxta_rdv_service_provider);
    _jxta_rdv_service_server *_self = PTValid(me, _jxta_rdv_service_server);
    LimitedRangeRdvMessage *header = NULL;
    Jxta_message_element *el = NULL;
    Jxta_bytevector *bytes;
    JString *string;
    int ttl;
    Jxta_endpoint_address *realDest = NULL;
    const char *svc_name = NULL;

    assert(NULL != msg);
    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Service walker received a message [%p]\n", msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, LIMITEDRANGEWALKELEMENT, &el);
    if ((JXTA_SUCCESS != res) || (NULL == el)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Walk header missing. Ignoring message [%p]\n", msg);
        goto FINAL_EXIT;
    }

    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    header = LimitedRangeRdvMessage_new();

    res = LimitedRangeRdvMessage_parse_charbuffer(header, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed parsing walker message[%p] %d\n", msg, res);
        goto FINAL_EXIT;
    }

    ttl = LimitedRangeRdvMessage_get_TTL(header);
    if (ttl <= 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Walker message[%p] with nonsense TTL %d, drop it.\n",
                        msg, ttl);
        goto FINAL_EXIT;
    }

    /* work around the JSE walk implementation and previous C implementation */
    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, "RdvWalkSvcName", &el);
    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Walker message[%p] has RdvWalkSvcName element.\n", msg);
        bytes = jxta_message_element_get_value(el);
        JXTA_OBJECT_RELEASE(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        svc_name = strdup(jstring_get_string(string));
        JXTA_OBJECT_RELEASE(string);
        if (NULL == svc_name) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE
                            "Cannot get svc name in walker msg, could be out of memory.\n");
            goto FINAL_EXIT;
        }
        res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, "RdvWalkSvcParam", &el);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE
                            "Cannot get the RdvWalkSvcParam element in an ealier verion of walker msg, discard.\n");
            goto FINAL_EXIT;
        }
        bytes = jxta_message_element_get_value(el);
        JXTA_OBJECT_RELEASE(el);
        string = jstring_new_3(bytes);
        JXTA_OBJECT_RELEASE(bytes);
        realDest = jxta_endpoint_address_new_2("jxta", _self->groupiduniq, svc_name, jstring_get_string(string));
        JXTA_OBJECT_RELEASE(string);
        free(svc_name);
    } else {
        realDest = jxta_endpoint_address_new_2("jxta", _self->groupiduniq, LimitedRangeRdvMessage_get_SrcSvcName(header),
                                               LimitedRangeRdvMessage_get_SrcSvcParams(header));
    }

    if (NULL == realDest) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to extract endpoint address from walker msg[%p]\n",
                        msg);
        goto FINAL_EXIT;
    }

    jxta_endpoint_service_demux_addr(jxta_rdv_service_endpoint_svc(provider->service), realDest, msg);
    JXTA_OBJECT_RELEASE(realDest);

  FINAL_EXIT:
    if (NULL != header) {
        JXTA_OBJECT_RELEASE(header);
    }
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vim: set ts=4 sw=4  tw=130 et: */
