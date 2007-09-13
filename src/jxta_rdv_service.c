/*
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
 * $Id$
 */

/**
 * This is the implementation of JxtaRdvService struc.
 * This definition must be set before including jxta.h.
 * This is why reference to other JXTA types must be declared.
 **/

static const char *__log_cat = "RdvService";

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_rdv_service.h"
#include "jxta_peer.h"
#include "jxta_svc.h"
#include "jxta_peer_private.h"
#include "jxta_peerview.h"
#include "jxta_peerview_priv.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_util_priv.h"
#include "jxta_endpoint_service_priv.h"

/* disable auto_rdv by default for 2.2 release */
/* static const Jxta_time_diff DEFAULT_AUTO_RDV_INTERVAL = ((Jxta_time_diff) 60) * 1000; /* 1 Minute */
static const Jxta_time_diff DEFAULT_AUTO_RDV_INTERVAL = -1;

const char JXTA_RDV_NS_NAME[] = "jxta";
const char JXTA_RDV_SERVICE_NAME[] = "urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000605";
const char JXTA_RDV_PROPAGATE_ELEMENT_NAME[] = "jxta:RendezVousPropagate";
const char JXTA_RDV_PROPAGATE_SERVICE_NAME[] = "JxtaPropagate";
const char JXTA_RDV_CONNECT_REQUEST_ELEMENT_NAME[] = "Connect";
const char JXTA_RDV_DISCONNECT_REQUEST_ELEMENT_NAME[] = "Disconnect";
const char JXTA_RDV_CONNECTED_REPLY_ELEMENT_NAME[] = "ConnectedPeer";
const char JXTA_RDV_LEASE_REPLY_ELEMENT_NAME[] = "ConnectedLease";
const char JXTA_RDV_RDVADV_REPLY_ELEMENT_NAME[] = "RdvAdvReply";
const char JXTA_RDV_ADV_ELEMENT_NAME[] = "RdvAdv";

Jxta_rdv_service *jxta_rdv_service_new_instance(void);
static void rdv_service_delete(Jxta_object * service);
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv);

static Jxta_status init(Jxta_module * rdv, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * self, const char *argv[]);
static void stop(Jxta_module * module);

static Jxta_boolean is_listener_for(_jxta_rdv_service * self, const char *str);

static void *APR_THREAD_FUNC auto_rdv_thread(apr_thread_t * t, void *arg);

static void jxta_rdv_event_delete(Jxta_object * ptr);
static Jxta_rdv_event *jxta_rdv_event_new(Jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

static const _jxta_rdv_service_methods JXTA_RDV_SERVICE_METHODS = {
    {
     {
      "Jxta_module_methods",
      init,
      start,
      stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl,
     service_on_option_set},
    "_jxta_rdv_service_methods"
};

/**
 * This is the Generic Rendezvous Service
 *
 * This code only implements the C portion of the generic part
 * of the Rendezvous Service, in other words, the glue between
 * the public API and the actual implementation.
 **/

/**
 * Frees an instance of the Rendezvous Service.
 * All objects and memory used by the instance must first be released/freed.
 *
 * @param service a pointer to the instance of the Rendezvous Service to free.
 * @return error code.
 **/
static void rdv_service_delete(Jxta_object * service)
{
    _jxta_rdv_service *self = (_jxta_rdv_service *) PTValid(service, _jxta_rdv_service);

    if (self->running) {
        stop((Jxta_module *) service);
    }

    /* call the hierarchy of dtors */
    jxta_rdv_service_destruct(self);

    /* free the object itself */
    free((void *) service);
}

/**
 * Standard instantiator; goes in the builtin modules table.
 **/
Jxta_rdv_service *jxta_rdv_service_new_instance(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service *self = (_jxta_rdv_service *) calloc(1, sizeof(_jxta_rdv_service));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, rdv_service_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service *) jxta_rdv_service_construct(self, &JXTA_RDV_SERVICE_METHODS);
}

/**
 * The base rdv service ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types).
 **/
_jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * self, const _jxta_rdv_service_methods * methods)
{
    PTValid(methods, _jxta_rdv_service_methods);

    self = (_jxta_rdv_service *) jxta_service_construct((Jxta_service *) self, (Jxta_service_methods *) methods);

    if (NULL != self) {
        apr_status_t res = APR_SUCCESS;

        self->thisType = "_jxta_rdv_service";

        /* Create the pool & mutex */
        apr_pool_create(&self->pool, NULL);
        res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                      self->pool);

        self->parentgroup = NULL;

        self->endpoint = NULL;
        self->provider = NULL;
        self->config = config_edge;
        self->status = status_none;
        self->peerview = NULL;
        self->auto_interval_lock = FALSE;
        /*
         * create the RDV event listener table
         */
        self->evt_listener_table = jxta_hashtable_new(1);

        self->running = FALSE;
        res = apr_thread_cond_create(&self->periodicCond, self->pool);
        res = apr_thread_mutex_create(&self->periodicMutex, APR_THREAD_MUTEX_DEFAULT, self->pool);
        self->periodicThread = NULL;
    }

    return self;
}

/**
 * The base rdv service dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 **/
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    if (NULL != self->parentgroup) {
        JXTA_OBJECT_RELEASE(self->parentgroup);
    }

    if (NULL != self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }

    if (NULL != self->evt_listener_table) {
        JXTA_OBJECT_RELEASE(self->evt_listener_table);
    }

    apr_thread_mutex_destroy(self->mutex);
    apr_thread_mutex_destroy(self->periodicMutex);
    apr_thread_cond_destroy(self->periodicCond);

    /* Free the pool used to allocate the thread and mutex */
    apr_pool_destroy(self->pool);

    self->thisType = NULL;

    jxta_service_destruct((Jxta_service *) rdv);
}


/**
 * Creates a rendezvous event
 **/
static Jxta_rdv_event *jxta_rdv_event_new(Jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    Jxta_rdv_event *rdv_event;

    if (NULL == id) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "NULL event id");
        return NULL;
    }

    rdv_event = (Jxta_rdv_event *) calloc(1, sizeof(Jxta_rdv_event));

    if (NULL == rdv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not alloc rdv event");
        return NULL;
    }

    JXTA_OBJECT_INIT(rdv_event, jxta_rdv_event_delete, NULL);

    rdv_event->source = rdv;
    rdv_event->event = event;
    rdv_event->pid = JXTA_OBJECT_SHARE(id);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "EVENT: Create a new rdv event [%pp]\n", rdv_event);
    return rdv_event;
}

/**
 * Deletes a rendezvous event
 **/
static void jxta_rdv_event_delete(Jxta_object * ptr)
{
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) ptr;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "Event [%pp] delete \n", ptr);
    JXTA_OBJECT_RELEASE(rdv_event->pid);
    rdv_event->pid = NULL;

    free(rdv_event);
}

/**
 * Initializes an instance of the Rendezvous Service. (a module method)
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service.
 * @param group a pointer to the PeerGroup the Rendezvous Service is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * 
 **/
static Jxta_status init(Jxta_module * rdv, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_id *gid;
    JString *groupUniqIdString;
    Jxta_PA *conf_adv = NULL;
    Jxta_RdvConfigAdvertisement *rdvConfig = NULL;

    res = jxta_service_init((Jxta_service *) rdv, group, assigned_id, impl_adv);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    jxta_PG_get_parentgroup(group, &self->parentgroup);

    jxta_PG_get_endpoint_service(group, &(self->endpoint));

    self->peerview = jxta_peerview_new();

    jxta_id_to_jstring(assigned_id, &self->peerviewNameString);
    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &groupUniqIdString);
    JXTA_OBJECT_RELEASE(gid);
    jstring_append_1(self->peerviewNameString, groupUniqIdString);
    JXTA_OBJECT_RELEASE(groupUniqIdString);

    res = jxta_peerview_init(self->peerview, group, self->peerviewNameString);

    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != self->parentgroup) {
        jxta_PG_get_configadv(self->parentgroup, &conf_adv);
    }

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;

        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);

        JXTA_OBJECT_RELEASE(conf_adv);

        if (svc != NULL) {
            rdvConfig = jxta_svc_get_RdvConfig(svc);

            JXTA_OBJECT_RELEASE(svc);
        }
    }

    if (NULL == rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Loading defaults\n");
        rdvConfig = jxta_RdvConfigAdvertisement_new();

        /* The default is -1 "disabled". We want to enable auto-rdv */
        jxta_RdvConfig_set_auto_rdv_interval(rdvConfig, 0);
    }

    self->auto_rdv_interval = jxta_RdvConfig_get_auto_rdv_interval(rdvConfig);
    if (0 == self->auto_rdv_interval) {
        self->auto_rdv_interval = DEFAULT_AUTO_RDV_INTERVAL;
    }

    self->config = jxta_RdvConfig_get_config(rdvConfig);

    jxta_peerview_set_happy_size(self->peerview, jxta_RdvConfig_get_min_happy_peerview(rdvConfig));
    JXTA_OBJECT_RELEASE(rdvConfig);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service inited.\n");

    return res;
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param this a pointer to the instance of the Rendezvous Service
 * @param argv a vector of string arguments.
 **/
static Jxta_status start(Jxta_module * module, const char *argv[])
{
    _jxta_rdv_service *self = PTValid(module, _jxta_rdv_service);

    apr_thread_mutex_lock(self->mutex);
    if (self->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV service had started, ignore start request.\n");
        apr_thread_mutex_unlock(self->mutex);
        return APR_SUCCESS;
    }

    self->running = TRUE;
    apr_thread_mutex_unlock(self->mutex);

    jxta_rdv_service_set_auto_interval((Jxta_rdv_service *) self, self->auto_rdv_interval);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service for : %s.\n",
                    jstring_get_string(self->peerviewNameString));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service started.\n");

    return jxta_rdv_service_set_config((Jxta_rdv_service *) self, self->config);
}

/**
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 *
 * @param module a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static void stop(Jxta_module * module)
{
    _jxta_rdv_service *self = PTValid(module, _jxta_rdv_service);
    Jxta_rdv_service_provider *provider;

    apr_thread_mutex_lock(self->mutex);

    if (!self->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV service is not started, ignore stop request.\n");
        apr_thread_mutex_unlock(self->mutex);
        return;
    }

    self->running = FALSE;
    apr_thread_mutex_unlock(self->mutex);

    /* disable auto-rdv */
    jxta_rdv_service_set_auto_interval((Jxta_rdv_service *) self, -1);

    provider = (Jxta_rdv_service_provider *) self->provider;
    if (NULL != provider) {
        PROVIDER_VTBL(provider)->stop(provider);
        JXTA_OBJECT_RELEASE(provider);
    }

    self->provider = NULL;
    JXTA_OBJECT_RELEASE(self->peerviewNameString);
    JXTA_OBJECT_RELEASE(self->peerview);


    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service stopped.\n");
}

/**
 * Propagates a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param ttl Maximum number of peers the propagated message can go through.
 * @return error code.
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_propagate(Jxta_rdv_service * rdv, Jxta_message * msg, const char *serviceName,
                                                     const char *serviceParam, int ttl)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    /* Test arguments first */
    if ((msg == NULL) || (ttl <= 0) || (NULL == serviceName)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->provider) {
        res = PROVIDER_VTBL(self->provider)->propagate((Jxta_rdv_service_provider *) self->provider,
                                                       msg, serviceName, serviceParam, ttl);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(self->mutex);

    return res;
}

/**
 * Walk a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
 * @param addr An EndpointAddress that defines on which address the message is destinated
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @return error code.
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_walk(Jxta_rdv_service * rdv, Jxta_message * msg, const char *serviceName,
                                                const char *serviceParam)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    /* Test arguments first */
    if ((msg == NULL) || (NULL == serviceName)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->provider) {
        res = PROVIDER_VTBL(self->provider)->walk((Jxta_rdv_service_provider *) self->provider, msg, serviceName, serviceParam);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(self->mutex);

    return res;
}

/**
 * Adds an Propagation listener. The listener is invoked when there is a incoming
 * propagated message for the given EndpointAddress.
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was already a listener for that address.
 *
 * @deprecated
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_propagate_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                  const char *serviceParam, Jxta_listener * listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    void *cookie;

    JXTA_DEPRECATED_API();
    
    /* Test arguments first */
    if (listener == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_SHARE(listener);
    return endpoint_service_add_recipient(self->endpoint, &cookie, serviceName, serviceParam, listener_callback, listener);
}

/**
 * Removes a Propagation listener.
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was not already a listener for that address.
 *
 * @deprecated
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_remove_propagate_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                     const char *serviceParam, Jxta_listener * listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status rv;

    JXTA_DEPRECATED_API();
    
    /* Test arguments first */
    if (listener == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    rv = endpoint_service_remove_recipient_by_addr(self->endpoint, serviceName, serviceParam);
    JXTA_OBJECT_RELEASE(listener);
    return rv;
}

/**
 * Adds an Event listener that will receive rendezvous Service event, such
* as connection, disconnection, lease request, lease grant events. Note that
* the peer object is automatically shared by this method.
 *
* @param rdv a pointer to the instance of the Rendezvous Service
* @param serviceName pointer to a string containing the name of the service on which
* the listener is listening on.
* @param serviceParam pointer to a string containing the parameter associated to the
* serviceName.
 * @param listener a pointer to an event listener. 
 * @return error code.
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_event_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                              const char *serviceParam, void *listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_boolean res;
    char *str = NULL;

    /* Test arguments first */
    if ((listener == NULL) || (serviceName == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    str = get_service_key(serviceName, serviceParam);
    if (NULL == str) {
        return JXTA_FAILED;
    }

    res = jxta_hashtable_putnoreplace(self->evt_listener_table, str, strlen(str), (Jxta_object *) listener);

    free(str);
    return res ? JXTA_SUCCESS : JXTA_BUSY;
}

/**
 * Removes an Event listener
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param serviceName pointer to a string containing the name of the service on which
 * the listener is listening on.
 * @param serviceParam pointer to a string containing the parameter associated to the
 * serviceName.
 * @return error code.
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_remove_event_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                 const char *serviceParam)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    char *str = NULL;
    Jxta_status res;

    /* Test arguments first */
    if (serviceName == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    str = get_service_key(serviceName, serviceParam);
    if (NULL == str) {
        return JXTA_FAILED;
    }

    res = jxta_hashtable_del(self->evt_listener_table, str, strlen(str), NULL);

    free(str);

    return res;
}

/**
 * Test if the peer is a rendezvous peer for the given instance of rendezvous Service.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
JXTA_DECLARE(Jxta_boolean) jxta_rdv_service_is_rendezvous(Jxta_rdv_service * rdv)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return ((status_rendezvous == self->status) || (status_auto_rendezvous == self->status));
}

/**
 * Test if the peer is connected. For now existing and not expired is what we use to decide.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
JXTA_DECLARE(Jxta_boolean) jxta_rdv_service_peer_is_connected(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (NULL == peer) {
        /* Invalid args. We can not return an error, so return FALSE */
        return FALSE;
    }

    return jpr_time_now() <= jxta_peer_get_expires(peer) ? TRUE : FALSE;
}

JXTA_DECLARE(Jxta_time) jxta_rdv_service_peer_get_expires(Jxta_service * rdv, Jxta_peer * p)
{
    JXTA_DEPRECATED_API();
    
    return jxta_peer_get_expires(p);
}

Jxta_status jxta_rdv_service_peer_set_expires(Jxta_service * rdv, Jxta_peer * p, Jxta_time expires)
{
    JXTA_DEPRECATED_API();

    return jxta_peer_set_expires(p, expires);
}


/**
 * Test if the peer is connected. For now existing and not expired is what we use to decide.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
JXTA_DECLARE(Jxta_boolean) jxta_rdv_service_peer_is_rdv(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    PTValid(rdv, _jxta_rdv_service);

    JXTA_DEPRECATED_API();

    /* Test arguments first */
    if (NULL == peer) {
        /* Invalid args. We can not return an error, so return FALSE */
        return FALSE;
    }

    /* FIXME 20050117 bondolo This return value is entirely made up. I'd prefer to kill this method. */
    return TRUE;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_peer(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    PTValid(rdv, _jxta_rdv_service);

    JXTA_DEPRECATED_API();

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* DOES NOTHING! */
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res;

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* Add it into the vector */

    res = jxta_peerview_add_seed(self->peerview, peer);
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed inserting _jxta_peer_entry as seed.\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_get_peer(Jxta_rdv_service * rdv, Jxta_id* peerid, Jxta_peer ** peer)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);
    
    if (NULL != self->provider) {
        res = PROVIDER_VTBL(self->provider)->get_peer((Jxta_rdv_service_provider *) self->provider, peerid, peer);
    } else {
        res = JXTA_BUSY;
    }
    
    apr_thread_mutex_unlock(self->mutex);

    return res;
}

/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);
    if (NULL != self->provider) {
        res = PROVIDER_VTBL(self->provider)->get_peers((Jxta_rdv_service_provider *) self->provider, peerlist);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(self->mutex);

    return res;
}

    /**
     * Returns the current rendezvous configuration of this peer within the
     * current peer group.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     **/
JXTA_DECLARE(RdvConfig_configuration) jxta_rdv_service_config(Jxta_rdv_service * rdv)
{

    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return self->config;
}

    /**
     * Adjusts the current rendezvous configuration of this peer within the 
     * current peer group. The configuration change may happen asynchronously.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @param config The desired configuration.
     **/
JXTA_DECLARE(Jxta_status) jxta_rdv_service_set_config(Jxta_rdv_service * rdv, RdvConfig_configuration config)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_rdv_service_provider *newProvider;
    Jxta_rdv_service_provider *oldProvider;
    RendezVousStatus newStatus;

    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(self->mutex);

    if ((NULL != self->provider) && (self->config == config)) {
        apr_thread_mutex_unlock(self->mutex);
        return JXTA_SUCCESS;
    }

    switch (config) {
    case config_adhoc:
        newStatus = status_adhoc;
        newProvider = jxta_rdv_service_adhoc_new();
        break;
        
    case config_edge:
        newStatus = (-1 == self->auto_rdv_interval) ? status_edge : status_auto_edge;
        newProvider = jxta_rdv_service_client_new();
        break;

    case config_rendezvous:
        newStatus = (-1 == self->auto_rdv_interval) ? status_rendezvous : status_auto_rendezvous;
        newProvider = jxta_rdv_service_server_new();
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unsupported configuration!\n");
        newStatus = status_adhoc;
        newProvider = jxta_rdv_service_adhoc_new();
    }

    /* Do the switch */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Switching to mode %d\n", newStatus);


    self->config = config;
    self->status = newStatus;
    oldProvider = (Jxta_rdv_service_provider *) self->provider;
    self->provider = newProvider;

    if (NULL != oldProvider) {
        res = PROVIDER_VTBL(oldProvider)->stop(oldProvider);
        JXTA_OBJECT_RELEASE(oldProvider);
    }

    res = PROVIDER_VTBL(newProvider)->init(newProvider, self);
    res = PROVIDER_VTBL(newProvider)->start(newProvider);

    apr_thread_mutex_unlock(self->mutex);
    return res;
}

    /**
     * Returns the interval at which this peer dynamically reconsiders its'
     * rendezvous configuration within the current group.
     *
     * @param rdv a pointer to the instance of the Rendezvous Service
     * @return The interval expressed as positive milliseconds or -1 if the peer
     * is using a fixed configuration.
     *
     **/
JXTA_DECLARE(Jxta_time_diff) jxta_rdv_service_auto_interval(Jxta_rdv_service * rdv)
{

    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return self->auto_rdv_interval;
}

/**
 * Set the interval at which this peer dynamically reconsiders its'
 * rendezvous configuration within the current group.
 *
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @param The interval expressed as positive milliseconds or -1 if the peer
 * should maintain its' current configuration.
 **/
JXTA_DECLARE(void) jxta_rdv_service_set_auto_interval(Jxta_rdv_service * rdv, Jxta_time_diff interval)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    apr_status_t res;

    apr_thread_mutex_lock(self->mutex);

    /* spinlock to prevent re-entrant */
    while (self->auto_interval_lock) {
        apr_sleep(10 * 1000);   /* 10 ms */
    }
    self->auto_interval_lock = TRUE;

    if (0 == interval) {
        interval = DEFAULT_AUTO_RDV_INTERVAL;
    }

    if ((interval > 0) && (interval < 10000)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Suspiciously low auto-rendezvous interval.\n");
    }

    self->auto_rdv_interval = interval;
    apr_thread_mutex_unlock(self->mutex);

    if (self->periodicThread) {
        apr_thread_mutex_lock(self->periodicMutex);
        apr_thread_cond_signal(self->periodicCond);
        apr_thread_mutex_unlock(self->periodicMutex);
        if (self->auto_rdv_interval < 0) {
            apr_thread_join(&res, (apr_thread_t *) self->periodicThread);
            self->periodicThread = NULL;
        }
    } else {
        if (self->auto_rdv_interval > 0) {
            /* Mark the service as running now. */
            apr_thread_create((apr_thread_t **) & self->periodicThread, NULL,   /* no attr */
                              (apr_thread_start_t) auto_rdv_thread, (void *) self, self->pool);
        }
    }

    apr_thread_mutex_lock(self->mutex);
    self->auto_interval_lock = FALSE;
    apr_thread_mutex_unlock(self->mutex);
}


static Jxta_boolean is_listener_for(_jxta_rdv_service * self, const char *str)
{
    Jxta_boolean res = FALSE;
    Jxta_listener *listener = NULL;

    jxta_hashtable_get(self->evt_listener_table, str, strlen(str), JXTA_OBJECT_PPTR(&listener));
    if (listener != NULL) {
        res = TRUE;
        JXTA_OBJECT_RELEASE(listener);
    }

    return res;
}

JXTA_DECLARE(Jxta_peerview *) jxta_rdv_service_get_peerview_priv(_jxta_rdv_service * self)
{
    return self->peerview;
}

JXTA_DECLARE(Jxta_peerview *) jxta_rdv_service_get_peerview(Jxta_rdv_service * rdv)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    if (self->peerview != NULL)
        JXTA_OBJECT_SHARE(self->peerview);

    return self->peerview;
}

Jxta_endpoint_service *jxta_rdv_service_endpoint_svc(_jxta_rdv_service * self)
{
    return self->endpoint;
}

/**
* Create a new rendezvous event and send it to all of the registered listeners.
**/
JXTA_DECLARE(void) jxta_rdv_generate_event(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_rdv_event *rdv_event = jxta_rdv_event_new((Jxta_rdv_service *) rdv, event, id);
    Jxta_vector *lis = NULL;

    if (NULL == rdv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not create rdv event object.\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Event [%d] for [%pp]\n", event, id);

    lis = jxta_hashtable_values_get(self->evt_listener_table);

    if (lis != NULL) {
        unsigned int i = 0;

        for (i = 0; i < jxta_vector_size(lis); i++) {
            Jxta_listener *listener = NULL;
            res = jxta_vector_get_object_at(lis, JXTA_OBJECT_PPTR(&listener), i);
            if (res == JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "Calling RDV listener [%pp]\n", listener);
                jxta_listener_process_object(listener, (Jxta_object *) rdv_event);
                JXTA_OBJECT_RELEASE(listener);
            }
        }

        JXTA_OBJECT_RELEASE(lis);
    }

    JXTA_OBJECT_RELEASE(rdv_event);
}

/**
 * Generates a unique number and returns it as a newly allocated string.
 * The caller is responsible for freeing the string.
 **/
JXTA_DECLARE(JString *) message_id_new(void)
{
    apr_uuid_t uuid;
    char uuidStr[APR_UUID_FORMATTED_LENGTH + 1];

    /*  get a new uuid and format it into a string */
    apr_uuid_get(&uuid);
    apr_uuid_format(uuidStr, &uuid);

    return jstring_new_2(uuidStr);
}

static void *APR_THREAD_FUNC auto_rdv_thread(apr_thread_t * thread, void *arg)
{
    _jxta_rdv_service *self = PTValid(arg, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;
    float average_probability = 0.5;
    unsigned int iterations_since_switch = 0;


    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Auto-rendezvous thread started.\n");

    while (self->running && self->auto_rdv_interval > 0) {
        float should_be_rdv_probability;
        float random = rand() / ((float) RAND_MAX);
        float probability_difference;
        RdvConfig_configuration new_config;

        /*
         * Wait for a while until the next iteration.
         */
        apr_thread_mutex_lock(self->periodicMutex);
        res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, self->auto_rdv_interval * 1000);

        apr_thread_mutex_unlock(self->periodicMutex);

        if (self->auto_rdv_interval < 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Auto-rendezvous periodic thread [TID %pp] is stopping.\n",
                            apr_os_thread_current());
            continue;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Auto-rendezvous periodic thread [TID %pp] RUN.\n",
                        apr_os_thread_current());

        /* Initially based upon configuration */
        should_be_rdv_probability = (config_rendezvous == self->config) ? (float) 0.75 : (float) 0.25;

        /* if we have a parent and it's a rdv, we should want to be one. */
        if (NULL != self->parentgroup) {
            Jxta_rdv_service *rdv = NULL;

            jxta_PG_get_rendezvous_service(self->parentgroup, &rdv);
            if (NULL != rdv) {
                if (config_rendezvous == jxta_rdv_service_config(rdv)) {
                    should_be_rdv_probability *= 1.25;
                }
                JXTA_OBJECT_RELEASE(rdv);
            }
        }

        /* If the peerview is happy then we shouldn't want to be a rdv. */
        /* NOTE : "greater-than" comparison to prevent dropping below size by our changing. */
        if (jxta_peerview_get_localview_size(self->peerview) > jxta_peerview_get_happy_size(self->peerview)) {
            should_be_rdv_probability *= (float) (1.0 - (0.01 * iterations_since_switch));
        } else {
            should_be_rdv_probability *= (float) (1.0 + (0.01 * iterations_since_switch));
        }

        /* If we are close to the long term average accentuate that behaviour */
        probability_difference = should_be_rdv_probability - average_probability;
        if ((probability_difference < 0.1) && (probability_difference > -0.1)) {
            should_be_rdv_probability *= (should_be_rdv_probability < 0.5) ? (float) 0.90 : (float) 1.1;
        }

        /* Re-calculate the long term probability */
        average_probability = (float) ((average_probability + should_be_rdv_probability) / 2.0);

        new_config = (should_be_rdv_probability > random) ? config_rendezvous : config_edge;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,
                        "Auto-rendezvous -- probability = %f average = %f random = %f diff = %f : "
                        "since last switch = %d : current = %d new = %d \n",
                        should_be_rdv_probability, average_probability, random, probability_difference, iterations_since_switch,
                        self->config, new_config);

        /* Refuse to switch if we just switched */
        if ((self->config != new_config) && (iterations_since_switch > 2)) {
            jxta_rdv_service_set_config((Jxta_rdv_service *) self, new_config);
            iterations_since_switch = 0;
        } else {
            iterations_since_switch++;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Auto-rendezvous periodic thread RUN DONE.\n");
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Auto-rendezvous thread stopped.\n");
    apr_thread_exit(thread, JXTA_SUCCESS);

    /* NOTREACHED */
    return NULL;
}

/* vim: set ts=4 sw=4 tw=130 et: */
