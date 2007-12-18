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

static const char *__log_cat = "RdvSvc";

#include <assert.h>

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
static const Jxta_time_diff DEFAULT_AUTO_RDV_INTERVAL = ((Jxta_time_diff) 60) * 1000;   /* 1 Minute */

/**
 **/
static const Jxta_time_diff SEEDING_REFRESH_INTERVAL = ((Jxta_time_diff) 20 * 60) * 1000;   /* 20 minutes */

const char* RDV_V3_MCID = "urn:jxta:uuid-DeadBeefDeafBabaFeedBabe0000000605";
const char* RDV_V3_MSID = "urn:jxta:uuid-46484E487F4711DBB5D700E081280BF40106";
const char JXTA_RDV_NS_NAME[] = "jxta";
const char JXTA_RDV_LEASING_SERVICE_NAME[] = "RdvLease";
const char JXTA_RDV_WALKER_SERVICE_NAME[] = "RdvWalker";
const char JXTA_RDV_PROPAGATE_SERVICE_NAME[] = "RdvPropagate";

const char *JXTA_RDV_CONFIG_MODE_NAMES[] = { "unknown", "ad-hoc", "client", "rendezvous" };
const char *JXTA_RDV_STATUS_NAMES[] = { "none", "unknown", "ad-hoc", "edge", "auto-edge", "auto-rendezvous", "rendezvous" };
const char *JXTA_RDV_EVENT_NAMES[] =
    { "INVALID", "rdv connected", "rdv reconnected", "rdv failed", "rdv disconnected", "client connect", "client reconnect",
"INVALID", "client disconnect",
    "became edge", "became rdv"
};

Jxta_rdv_service *jxta_rdv_service_new_instance(void);
static _jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * rdv, _jxta_rdv_service_methods const *methods);
static void rdv_service_delete(Jxta_object * service);
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv);


static Jxta_status init(Jxta_module * rdv, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * module, char const *argv[]);
static void stop(Jxta_module * module);

static Jxta_boolean is_listener_for(_jxta_rdv_service * myself, char const *str);

static void rdv_event_delete(Jxta_object * ptr);
static Jxta_rdv_event *rdv_event_new(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

static void JXTA_STDCALL peerview_event_listener(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL leasing_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL walker_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL prop_cb(Jxta_object * obj, void *arg);
static Jxta_status JXTA_STDCALL call_entry(Jxta_object * obj, void *arg, JString * key_j);

static _jxta_rdv_service_methods const JXTA_RDV_SERVICE_METHODS = {
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

struct _provider_callback_entry {
    Extends(Jxta_object);
    Jxta_callback_func func;
    Jxta_rdv_service_provider *provider;
    void *arg;
    JString *key;
};

typedef struct _provider_callback_entry _provider_callback_entry;

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
    _jxta_rdv_service *myself = (_jxta_rdv_service *) PTValid(service, _jxta_rdv_service);

    if (myself->running) {
        stop((Jxta_module *) service);
    }

    /* call the hierarchy of dtors */
    jxta_rdv_service_destruct(myself);

    /* free the object itself */
    free((void *) service);
}

/**
 * Standard instantiator; goes in the builtin modules table.
 **/
Jxta_rdv_service *jxta_rdv_service_new_instance(void)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service *myself = (_jxta_rdv_service *) calloc(1, sizeof(_jxta_rdv_service));

    if (myself == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, rdv_service_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service *) jxta_rdv_service_construct(myself, &JXTA_RDV_SERVICE_METHODS);
}

static void provider_entry_delete(Jxta_object * obj)
{
    _provider_callback_entry *myself = (_provider_callback_entry *) obj;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Provider entry delete\n");

    if (myself->key)
        JXTA_OBJECT_RELEASE(myself->key);
    if (myself->arg)
        JXTA_OBJECT_RELEASE(myself->arg);
    /* free the object itself */
    free((void *) myself);
}

static _provider_callback_entry *provider_callback_entry_new(void)
{
    /* Allocate an instance of this service */
    _provider_callback_entry *myself = (_provider_callback_entry *) calloc(1, sizeof(_provider_callback_entry));

    if (NULL != myself) {
        /* Initialize the object */
        JXTA_OBJECT_INIT(myself, provider_entry_delete, 0);
    }
    return myself;
}

/**
 * The base rdv service ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types).
 **/
static _jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * myself, _jxta_rdv_service_methods const *methods)
{
    PTValid(methods, _jxta_rdv_service_methods);

    myself = (_jxta_rdv_service *) jxta_service_construct((Jxta_service *) myself, (Jxta_service_methods const *) methods);

    if (NULL != myself) {
        apr_status_t res = APR_SUCCESS;

        myself->thisType = "_jxta_rdv_service";

        /* Create the pool & mutex */
        apr_pool_create(&myself->pool, NULL);
        res = apr_thread_mutex_create(&myself->mutex, APR_THREAD_MUTEX_NESTED,  /* nested */
                                      myself->pool);

        res = apr_thread_mutex_create(&myself->cb_mutex, APR_THREAD_MUTEX_NESTED,  /* nested */
                                      myself->pool);

        myself->group = NULL;
        myself->pid = NULL;
        myself->assigned_id_str = NULL;

        myself->endpoint = NULL;
        myself->provider = NULL;
        myself->current_config = config_unknown;
        myself->config = config_adhoc;
        myself->status = status_none;
        myself->peerview = NULL;
        myself->auto_interval_lock = FALSE;

        myself->rdvConfig = NULL;
        myself->active_seeds = NULL;
        myself->is_demoting = FALSE;
        /*
         * create the RDV event listener table
         */
        myself->evt_listener_table = jxta_hashtable_new(1);
        myself->callback_table = jxta_hashtable_new(1);
        myself->running = FALSE;
        myself->service_start = 0L;
    }

    return myself;
}

/**
 * The base rdv service dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 **/
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);


    if (NULL != myself->provider) {
        PROVIDER_VTBL(myself->provider)->stop(myself->provider);
        JXTA_OBJECT_RELEASE(myself->provider);
        myself->provider = NULL;
    }

    if (NULL != myself->endpoint) {
        JXTA_OBJECT_RELEASE(myself->endpoint);
    }

    if (NULL != myself->rdvConfig) {
        JXTA_OBJECT_RELEASE(myself->rdvConfig);
    }

    if (NULL != myself->evt_listener_table) {
        JXTA_OBJECT_RELEASE(myself->evt_listener_table);
    }

    if (NULL != myself->callback_table) {
        JXTA_OBJECT_RELEASE(myself->callback_table);
        myself->callback_table = NULL;
    }

    if (NULL != myself->pid) {
        JXTA_OBJECT_RELEASE(myself->pid);
    }

    if (NULL != myself->assigned_id_str) {
        free(myself->assigned_id_str);
    }

    if (NULL != myself->active_seeds) {
        JXTA_OBJECT_RELEASE(myself->active_seeds);
    }
    if (NULL != myself->lease_key_j) {
        JXTA_OBJECT_RELEASE(myself->lease_key_j);
    }
    if (NULL != myself->walker_key_j) {
        JXTA_OBJECT_RELEASE(myself->walker_key_j);
    }
    if (NULL != myself->prop_key_j) {
        JXTA_OBJECT_RELEASE(myself->prop_key_j);
    }

    apr_thread_mutex_destroy(myself->mutex);
    apr_thread_mutex_destroy(myself->cb_mutex);

    /* Free the pool used to allocate the thread and mutex */
    apr_pool_destroy(myself->pool);

    myself->thisType = NULL;

    jxta_service_destruct((Jxta_service *) rdv);
}

/**
 * Creates a rendezvous event
 **/
static Jxta_rdv_event *rdv_event_new(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    Jxta_rdv_event *rdv_event;

    if (NULL == id) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "NULL event id\n");
        return NULL;
    }

    rdv_event = (Jxta_rdv_event *) calloc(1, sizeof(Jxta_rdv_event));

    if (NULL == rdv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not alloc rdv event\n");
        return NULL;
    }

    JXTA_OBJECT_INIT(rdv_event, rdv_event_delete, NULL);

    rdv_event->source = (Jxta_rdv_service *) rdv;
    rdv_event->event = event;
    rdv_event->pid = JXTA_OBJECT_SHARE(id);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "EVENT: Create a new rdv event [%pp]\n", rdv_event);
    return rdv_event;
}

/**
 * Deletes a rendezvous event
 **/
static void rdv_event_delete(Jxta_object * ptr)
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
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    JString *assigned_id_str;
    Jxta_PA *conf_adv = NULL;
    Jxta_PG *parentgroup = NULL;

    res = jxta_service_init((Jxta_service *) rdv, group, assigned_id, impl_adv);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    jxta_id_to_jstring(assigned_id, &assigned_id_str);

    myself->assigned_id_str = strdup(jstring_get_string(assigned_id_str));
    JXTA_OBJECT_RELEASE(assigned_id_str);

    jxta_PG_get_PID(group, &myself->pid);

    myself->group = group; /* Not shared to prevent a circular dependency */

    jxta_PG_get_parentgroup(group, &parentgroup);

    jxta_PG_get_configadv(group, &conf_adv);
    if (NULL == conf_adv && NULL != parentgroup) {
        jxta_PG_get_configadv(parentgroup, &conf_adv);
    }
    if (NULL != parentgroup)
        JXTA_OBJECT_RELEASE(parentgroup);

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;

        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);

        JXTA_OBJECT_RELEASE(conf_adv);

        if (svc != NULL) {
            myself->rdvConfig = jxta_svc_get_RdvConfig(svc);

            JXTA_OBJECT_RELEASE(svc);
        }
    }

    if (NULL == myself->rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Using defaults.\n");
        myself->rdvConfig = jxta_RdvConfigAdvertisement_new();

        /* The default is -1 "disabled". We want to enable auto-rdv */
        jxta_RdvConfig_set_auto_rdv_interval(myself->rdvConfig, 0);
    }

    myself->auto_rdv_interval = jxta_RdvConfig_get_auto_rdv_interval(myself->rdvConfig);
    if (0 == myself->auto_rdv_interval) {
        myself->auto_rdv_interval = DEFAULT_AUTO_RDV_INTERVAL;
    }

    myself->config = jxta_RdvConfig_get_config(myself->rdvConfig);

    jxta_PG_get_endpoint_service(group, &(myself->endpoint));


    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service inited : %s \n", myself->assigned_id_str);

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
static Jxta_status start(Jxta_module * module, char const *argv[])
{
    _jxta_rdv_service *myself = PTValid(module, _jxta_rdv_service);

    apr_thread_mutex_lock(myself->mutex);
    if (myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV service had started, ignore start request.\n");
        apr_thread_mutex_unlock(myself->mutex);
        return JXTA_SUCCESS;
    }

    myself->running = TRUE;
    myself->switch_config = FALSE;
    myself->lease_key_j = jstring_new_2(RDV_V3_MSID);
    jstring_append_2(myself->lease_key_j, JXTA_RDV_LEASING_SERVICE_NAME);
    jxta_PG_add_recipient(myself->group, &myself->ep_cookie_leasing, RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME, leasing_cb, myself);

    myself->walker_key_j = jstring_new_2(RDV_V3_MSID);
    jstring_append_2(myself->walker_key_j, JXTA_RDV_WALKER_SERVICE_NAME);
    jxta_PG_add_recipient(myself->group, &myself->ep_cookie_walker, RDV_V3_MSID, JXTA_RDV_WALKER_SERVICE_NAME, walker_cb, myself);

    myself->prop_key_j = jstring_new_2(RDV_V3_MSID);
    jstring_append_2(myself->prop_key_j, JXTA_RDV_PROPAGATE_SERVICE_NAME);
    jxta_PG_add_recipient(myself->group, &myself->ep_cookie_prop, RDV_V3_MSID, JXTA_RDV_PROPAGATE_SERVICE_NAME, prop_cb, myself);

    apr_thread_mutex_unlock(myself->mutex);
    myself->service_start = (Jxta_time) jpr_time_now();

    jxta_rdv_service_set_auto_interval((Jxta_rdv_service *) myself, myself->auto_rdv_interval);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Rendezvous service started.\n");

    return rdv_service_switch_config((Jxta_rdv_service *) myself, myself->config);
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
    _jxta_rdv_service *myself = PTValid(module, _jxta_rdv_service);
    Jxta_rdv_service_provider * provider;
    apr_thread_mutex_lock(myself->mutex);

    if (!myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV service is not started, ignore stop request.\n");
        apr_thread_mutex_unlock(myself->mutex);
        return;
    }

    myself->running = FALSE;

    provider = (Jxta_rdv_service_provider *) myself->provider;
    if (NULL != provider) {
        PROVIDER_VTBL(provider)->stop(provider);
        JXTA_OBJECT_RELEASE(provider);
    }
    myself->provider = NULL;
    if (myself->peerview) {
        jxta_peerview_remove_event_listener(myself->peerview, myself->assigned_id_str, NULL);
        jxta_listener_stop(myself->peerview_listener);
        JXTA_OBJECT_RELEASE(myself->peerview_listener);
        myself->peerview_listener = NULL;
        JXTA_OBJECT_RELEASE(myself->peerview);
    }

    assert(NULL != myself->ep_cookie_leasing);
    jxta_PG_remove_recipient(myself->group, myself->ep_cookie_leasing);

    assert(NULL != myself->ep_cookie_walker);
    jxta_PG_remove_recipient(myself->group, myself->ep_cookie_walker);

    assert(NULL != myself->ep_cookie_prop);
    jxta_PG_remove_recipient(myself->group, myself->ep_cookie_prop);

    apr_thread_mutex_unlock(myself->mutex);

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
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    /* Test arguments first */
    if ((msg == NULL) || (ttl <= 0) || (NULL == serviceName)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->provider) {
        res = PROVIDER_VTBL(myself->provider)->propagate(myself->provider, msg, serviceName, serviceParam, ttl);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_directed_walk(Jxta_rdv_service * rdv, Jxta_message * msg, const char *serviceName,
                                                         const char *serviceParam, const char *target_hash)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    /* Test arguments first */
    if ((msg == NULL) || (NULL == serviceName)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->provider) {
        res = PROVIDER_VTBL(myself->provider)->walk(myself->provider, msg, serviceName, serviceParam, target_hash);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_walk(Jxta_rdv_service * rdv, Jxta_message * msg, const char *serviceName,
                                                const char *serviceParam)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    /* Test arguments first */
    if ((msg == NULL) || (NULL == serviceName)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->provider) {
        res = PROVIDER_VTBL(myself->provider)->walk(myself->provider, msg, serviceName, serviceParam, NULL);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_propagate_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                  const char *serviceParam, Jxta_listener * listener)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    void *cookie;

    JXTA_DEPRECATED_API();

    /* Test arguments first */
    if (listener == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_SHARE(listener);
    return endpoint_service_add_recipient(myself->endpoint, &cookie, serviceName, serviceParam, listener_callback, listener);
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_remove_propagate_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                     const char *serviceParam, Jxta_listener * listener)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_status rv;

    JXTA_DEPRECATED_API();

    /* Test arguments first */
    if (listener == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    rv = endpoint_service_remove_recipient_by_addr(myself->endpoint, serviceName, serviceParam);
    JXTA_OBJECT_RELEASE(listener);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_event_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                              const char *serviceParam, void *listener)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
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

    res = jxta_hashtable_putnoreplace(myself->evt_listener_table, str, strlen(str), (Jxta_object *) listener);

    free(str);
    return res ? JXTA_SUCCESS : JXTA_BUSY;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_remove_event_listener(Jxta_rdv_service * rdv, const char *serviceName,
                                                                 const char *serviceParam)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
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

    res = jxta_hashtable_del(myself->evt_listener_table, str, strlen(str), NULL);

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
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return ((status_rendezvous == myself->status) || (status_auto_rendezvous == myself->status));
}

JXTA_DECLARE(Jxta_boolean) jxta_rdv_service_is_demoting(Jxta_rdv_service * rdv)
{

    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return myself->is_demoting;

}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* Add it into the vector */
    jxta_RdvConfig_add_seed(myself->rdvConfig, jxta_peer_address((Jxta_peer *) peer));

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_seed_address(Jxta_rdv_service * rdv, Jxta_endpoint_address * addr)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (addr == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* Add it into the vector */
    jxta_RdvConfig_add_seed(myself->rdvConfig, addr);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_add_seeding_address(Jxta_rdv_service * rdv, Jxta_endpoint_address * addr)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (addr == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    /* Add it into the vector */
    jxta_RdvConfig_add_seeding(myself->rdvConfig, addr);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_get_peer(Jxta_rdv_service * rdv, Jxta_id * peerid, Jxta_peer ** peer)
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

JXTA_DECLARE(Jxta_status) jxta_rdv_service_get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->provider) {
        res = PROVIDER_VTBL(myself->provider)->get_peers((Jxta_rdv_service_provider *) myself->provider, peerlist);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

JXTA_DECLARE(RdvConfig_configuration) jxta_rdv_service_config(Jxta_rdv_service * rdv)
{

    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return myself->current_config;
}

JXTA_DECLARE(Jxta_time_diff) jxta_rdv_service_get_running_time(Jxta_rdv_service * rdv)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return jpr_time_now() - myself->service_start;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_service_set_config(Jxta_rdv_service * rdv, RdvConfig_configuration config)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    myself->switch_config = TRUE;
    myself->new_config = config;
    return res;
}

JXTA_DECLARE(Jxta_status) rdv_service_switch_config(Jxta_rdv_service * rdv, RdvConfig_configuration config)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);
    Jxta_rdv_service_provider *newProvider;
    Jxta_rdv_service_provider *oldProvider;
    RendezVousStatus new_status = myself->current_config;
    Jxta_boolean locked = FALSE;

    if ((NULL != myself->provider) && (myself->current_config == config)) {
        /* Already in correct config. */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "[%pp] Switching rdv configuration : [%s] -> [%s]\n",
                    myself, JXTA_RDV_CONFIG_MODE_NAMES[myself->current_config], JXTA_RDV_CONFIG_MODE_NAMES[config]);

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;
    myself->service_start = (Jxta_time) jpr_time_now();

    switch (config) {
    case config_adhoc:
        new_status = status_adhoc;
        newProvider = jxta_rdv_service_adhoc_new(myself->pool);
        break;

    case config_edge:
        new_status = (-1 == myself->auto_rdv_interval) ? status_edge : status_auto_edge;
        newProvider = jxta_rdv_service_client_new(myself->pool);
        break;

    case config_rendezvous:
        new_status = (-1 == myself->auto_rdv_interval) ? status_rendezvous : status_auto_rendezvous;
        newProvider = jxta_rdv_service_server_new(myself->pool);
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unsupported configuration!\n");
        new_status = status_adhoc;
        newProvider = jxta_rdv_service_adhoc_new(myself->pool);
    }

    /* Do the switch */

    /* Create a new peerview if necessary */
    switch (new_status) {
    case status_adhoc:
    case status_edge:
        /* do nothing */
        break;

    case status_auto_edge:
    case status_auto_rendezvous:
    case status_rendezvous:
        /* Create a peerview if we are rendezvous or auto_rdv. */
        if (NULL == myself->peerview) {
            myself->peerview = jxta_peerview_new(myself->pool);
            jxta_peerview_set_auto_cycle(myself->peerview, myself->auto_rdv_interval);

            res = peerview_init(myself->peerview, myself->group, jxta_service_get_assigned_ID_priv(rdv));

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to init peerview %d\n", res);
                JXTA_OBJECT_RELEASE(myself->peerview);

                myself->peerview = NULL;
                config = config_adhoc;
                goto FINAL_EXIT;
            }

            /* Add a listener to peerview events */
            myself->peerview_listener = jxta_listener_new((Jxta_listener_func) peerview_event_listener, myself, 1, 1);

            if (myself->peerview_listener != NULL) {
                jxta_listener_start(myself->peerview_listener);
                res =
                    jxta_peerview_add_event_listener(myself->peerview, myself->assigned_id_str, NULL, myself->peerview_listener);

            } else {
                res = JXTA_FAILED;
            }

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                FILEANDLINE "Failed to add an event listener to peerview events\n");
                goto FINAL_EXIT;
            }

            res = peerview_start(myself->peerview);

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to start peerview %d\n", res);
                goto FINAL_EXIT;
            }

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "[%pp] Created new peerview [%pp].\n", myself, myself->peerview);
        }
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unrecognized config %d\n", new_status);
        break;
    }

    myself->current_config = config;
    myself->status = new_status;
    oldProvider = (Jxta_rdv_service_provider *) myself->provider;

    if (NULL != oldProvider) {
        res = PROVIDER_VTBL(oldProvider)->stop(oldProvider);
        JXTA_OBJECT_RELEASE(oldProvider);
        myself->provider = NULL;
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed stopping rdv provider [%pp]\n", myself);
        }
    }

    res = PROVIDER_VTBL(newProvider)->init(newProvider, myself);

    if (JXTA_SUCCESS == res) {
        myself->provider = newProvider;
        myself->is_demoting = FALSE;
        apr_thread_mutex_unlock(myself->mutex);
        res = PROVIDER_VTBL(newProvider)->start(newProvider);
        apr_thread_mutex_lock(myself->mutex);
        locked = TRUE;
    }

    if (JXTA_SUCCESS != res) {
        if (NULL != myself->provider) {
            JXTA_OBJECT_RELEASE(myself->provider);
        }
        myself->provider = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed initing/starting rdv provider [%pp]\n", myself);
    }

    /* Stop the peerview if necessary */
    switch (new_status) {
    case status_adhoc:
    case status_edge:
        if (NULL != myself->peerview) {
            res = jxta_peerview_remove_event_listener(myself->peerview, myself->assigned_id_str, NULL);
            jxta_listener_stop(myself->peerview_listener);
            JXTA_OBJECT_RELEASE(myself->peerview_listener);
            myself->peerview_listener = NULL;

            res = peerview_stop(myself->peerview);

            JXTA_OBJECT_RELEASE(myself->peerview);

            myself->peerview = NULL;
        }
        break;

    case status_auto_edge:
    case status_auto_rendezvous:
    case status_rendezvous:
        /* Adjust the activity mode of the peerview. */
        myself->is_demoting = FALSE;
        jxta_peerview_set_active(myself->peerview, config_rendezvous == config);
        break;

    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Unexcepted status.\n");
    }

  FINAL_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "[%pp] Rendezvous provider status : %s\n", myself,
                    JXTA_RDV_STATUS_NAMES[new_status]);
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);


    return res;
}


Jxta_peerview *rdv_service_get_peerview_priv(Jxta_rdv_service * rdv)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return myself->peerview;
}

JXTA_DECLARE(Jxta_peerview *) jxta_rdv_service_get_peerview(Jxta_rdv_service * rdv)
{
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    if (myself->peerview != NULL)
        JXTA_OBJECT_SHARE(myself->peerview);

    return myself->peerview;
}

JXTA_DECLARE(Jxta_time_diff) jxta_rdv_service_auto_interval(Jxta_rdv_service * rdv)
{

    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    return myself->auto_rdv_interval;
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
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    apr_thread_mutex_lock(myself->mutex);
    myself->auto_rdv_interval = interval;

    if (myself->peerview == NULL) {
        apr_thread_mutex_unlock(myself->mutex);
        return;
    }
    jxta_peerview_set_auto_cycle(myself->peerview, interval);

    apr_thread_mutex_unlock(myself->mutex);
}


static Jxta_boolean is_listener_for(_jxta_rdv_service * myself, char const *str)
{
    Jxta_boolean res = FALSE;
    Jxta_listener *listener = NULL;

    jxta_hashtable_get(myself->evt_listener_table, str, strlen(str), JXTA_OBJECT_PPTR(&listener));
    if (listener != NULL) {
        res = TRUE;
        JXTA_OBJECT_RELEASE(listener);
    }

    return res;
}

/**
* Create a new rendezvous event and send it to all of the registered listeners.
**/
void rdv_service_generate_event(_jxta_rdv_service * myself, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_event *rdv_event = rdv_event_new(myself, event, id);
    Jxta_vector *lis = NULL;
    JString *pid_str;

    if (NULL == rdv_event) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not create rdv event object.\n");
        return;
    }

    if (NULL != id) {
        jxta_id_to_jstring(id, &pid_str);
    } else {
        pid_str = jstring_new_2("[NULL]");
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "[%pp] Rendezvous Event : \"%s\" for %s\n", myself,
                    JXTA_RDV_EVENT_NAMES[event], jstring_get_string(pid_str));
    JXTA_OBJECT_RELEASE(pid_str);

    lis = jxta_hashtable_values_get(myself->evt_listener_table);

    if (lis != NULL) {
        unsigned int i = 0;

        for (i = 0; i < jxta_vector_size(lis); i++) {
            Jxta_listener *listener = NULL;
            res = jxta_vector_get_object_at(lis, JXTA_OBJECT_PPTR(&listener), i);
            if (res == JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Calling RDV listener [%pp]\n", listener);
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

Jxta_status rdv_service_get_seeds(Jxta_rdv_service * me, Jxta_vector ** seeds)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(me, _jxta_rdv_service);
    Jxta_time currentTime = (Jxta_time) jpr_time_now();

    /* XXX 20060918 Narrow this mutex */
    apr_thread_mutex_lock(myself->mutex);

    if ((NULL == myself->active_seeds) || ((myself->last_seeding_update + SEEDING_REFRESH_INTERVAL) < currentTime)) {
        Jxta_vector *new_seeds;
        Jxta_vector *old_seeds;
        Jxta_vector *seed_uris;
        Jxta_vector *seeding_uris;
        unsigned int all_uris;
        unsigned int each_uri;

        /* Update timeout immediately so that only one thread does the update. */
        myself->last_seeding_update = currentTime;

        /* Start with the explict seeds */
        seed_uris = jxta_RdvConfig_get_seeds(myself->rdvConfig);

        new_seeds = jxta_vector_new(jxta_vector_size(seed_uris));

        all_uris = jxta_vector_size(seed_uris);
        for (each_uri = 0; each_uri < all_uris; each_uri++) {
            Jxta_endpoint_address *addr;

            res = jxta_vector_remove_object_at(seed_uris, JXTA_OBJECT_PPTR(&addr), 0);

            if (JXTA_SUCCESS == res) {
                Jxta_peer *seed = jxta_peer_new();

                jxta_peer_set_address(seed, addr);
                jxta_peer_set_expires(seed, JPR_ABSOLUTE_TIME_MAX);
                jxta_vector_add_object_last(new_seeds, (Jxta_object *) seed);

                JXTA_OBJECT_RELEASE(addr);
                JXTA_OBJECT_RELEASE(seed);
            }
        }
        JXTA_OBJECT_RELEASE(seed_uris);

        /* Shuffle them */
        jxta_vector_shuffle(new_seeds);

        seeding_uris = jxta_RdvConfig_get_seeding(myself->rdvConfig);
        jxta_vector_shuffle(seeding_uris);
        all_uris = jxta_vector_size(seeding_uris);

        for (each_uri = 0; each_uri < all_uris; each_uri++) {
            Jxta_endpoint_address *addr;
            Jxta_vector *more_addresses = NULL;

            res = jxta_vector_remove_object_at(seeding_uris, JXTA_OBJECT_PPTR(&addr), 0);

            if (JXTA_SUCCESS != res) {
                continue;
            }

            /* FIXME 20060813 bondolo Handle seeding URIs here */
            if (0 == strcmp(jxta_endpoint_address_get_protocol_name(addr), "http")) {
                /* FIXME 20060815 bondolo Handle http seeding URIs here */
            } else if (0 == strcmp(jxta_endpoint_address_get_protocol_name(addr), "file")) {
                /* FIXME 20060815 bondolo Handle file seeding URIs here */
            } else {
                /* XXX 20060819 bondolo Move Peerview pipe seeding to here */
                char *uri = jxta_endpoint_address_to_string(addr);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Unhandled seeding URI : %s\n", uri);
                free(uri);
            }

            if (NULL != more_addresses) {
                unsigned int all_additional_seed = jxta_vector_size(more_addresses);
                unsigned int each_additional_seed;

                for (each_additional_seed = all_additional_seed - 1; each_additional_seed < all_additional_seed;
                     each_additional_seed--) {
                    Jxta_endpoint_address *addr;

                    res = jxta_vector_remove_object_at(more_addresses, JXTA_OBJECT_PPTR(&addr), 0);

                    if (JXTA_SUCCESS == res) {
                        Jxta_peer *seed = jxta_peer_new();

                        jxta_peer_set_address(seed, addr);
                        jxta_peer_set_expires(seed, jpr_time_now() + SEEDING_REFRESH_INTERVAL);

                        jxta_vector_add_object_first(new_seeds, (Jxta_object *) seed);
                        JXTA_OBJECT_RELEASE(addr);
                        JXTA_OBJECT_RELEASE(seed);
                    }
                }

                JXTA_OBJECT_RELEASE(more_addresses);
                more_addresses = NULL;
            }

            JXTA_OBJECT_RELEASE(addr);
        }

        JXTA_OBJECT_RELEASE(seeding_uris);

        old_seeds = myself->active_seeds;
        myself->active_seeds = new_seeds;

        if (NULL != old_seeds) {
            JXTA_OBJECT_RELEASE(old_seeds);
        }

        /* Update again because fetch may have taken some time. */
        myself->last_seeding_update = (Jxta_time) jpr_time_now();
    }

    if (NULL == myself->active_seeds) {
        *seeds = jxta_vector_new(1);
        res = JXTA_SUCCESS;
    } else {
        res = jxta_vector_clone(myself->active_seeds, seeds, 0, INT_MAX);
    }

    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

Jxta_status rdv_service_add_referral_seed(Jxta_rdv_service * me, Jxta_peer * seed)
{

    _jxta_rdv_service *myself = PTValid(me, _jxta_rdv_service);

    apr_thread_mutex_lock(myself->mutex);

    if (NULL == myself->active_seeds) {
        myself->active_seeds = jxta_vector_new(0);
    }

    jxta_vector_add_object_first(myself->active_seeds, (Jxta_object *) seed);

    apr_thread_mutex_unlock(myself->mutex);

    return JXTA_SUCCESS;
}


Jxta_status rdv_service_send_seed_request(Jxta_rdv_service * rdv)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(rdv, _jxta_rdv_service);

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->provider) {
        res = provider_send_seed_request(myself->provider);
    } else {
        res = JXTA_BUSY;
    }
    apr_thread_mutex_unlock(myself->mutex);

    return res;
}

static void JXTA_STDCALL peerview_event_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = 0;
    Jxta_boolean locked = FALSE;
    Jxta_peerview_event *event = (Jxta_peerview_event *) obj;
    _jxta_rdv_service *myself = PTValid(arg, _jxta_rdv_service);

    if (!myself->running) {
        /* We don't want to churn while things are shutting down. */
        goto FINAL_EXIT;
    }

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    if (!jxta_id_equals(event->pid, myself->pid)) {

        if (JXTA_PEERVIEW_ADD == event->event) {
            Jxta_peer *peer = NULL;
            /* if this peer has a lease revoke it */
            res = jxta_rdv_service_get_peer((Jxta_rdv_service *) myself, event->pid, &peer);
            if (JXTA_SUCCESS == res) {
                jxta_peer_set_expires(peer, 0L);
                JXTA_OBJECT_RELEASE(peer);
            }
        }
        goto FINAL_EXIT;
    }

    switch (event->event) {
    case JXTA_PEERVIEW_ADD:
        myself->is_demoting = FALSE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "ADD EVENT FROM PEERVIEW \n");
        /* Joined peerview, we need to switch to rdv mode. */
        res = jxta_rdv_service_set_config((Jxta_rdv_service *) myself, config_rendezvous);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to switch to RDV config. [%pp]\n", myself);
        }

        break;

    case JXTA_PEERVIEW_REMOVE:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "REMOVE EVENT FROM PEERVIEW \n");

        if (status_auto_rendezvous == myself->status) {
            /* Left peerview, we need to switch to edge mode. */
            res = jxta_rdv_service_set_config((Jxta_rdv_service *) myself, config_edge);

            if (JXTA_SUCCESS != res) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed to switch to EDGE config. [%pp]\n", myself);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Ignoring PEERVIEW_REMOVE. Current state : %s [%pp]\n",
                            JXTA_RDV_STATUS_NAMES[myself->status], myself);
        }
        break;

    case JXTA_PEERVIEW_DEMOTE:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "DEMOTE EVENT FROM PEERVIEW \n");
        if (status_rendezvous == myself->status) {
            myself->is_demoting = TRUE;
        } 
        break;
    default:
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Catch All for event %d ignored\n", event->event);
        break;
    }

  FINAL_EXIT:
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);
}

Jxta_status rdv_service_remove_cb(Jxta_rdv_service * me, Jxta_object *cookie)
{
    Jxta_vector *entries = NULL;
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(me,_jxta_rdv_service);

    apr_thread_mutex_lock(myself->mutex);
    if (NULL != myself->callback_table) {
        _provider_callback_entry * e_cookie;
        _provider_callback_entry * entry;
        e_cookie = (_provider_callback_entry *) cookie;
        res = jxta_hashtable_get(myself->callback_table, jstring_get_string(e_cookie->key), jstring_length(e_cookie->key), JXTA_OBJECT_PPTR(&entry));
        if (JXTA_ITEM_NOTFOUND != res) {
            if (entry == (_provider_callback_entry *) cookie) {
                jxta_hashtable_del(myself->callback_table, jstring_get_string(entry->key), jstring_length(entry->key), NULL);
            }
            JXTA_OBJECT_RELEASE(entry);
        }
    }
    apr_thread_mutex_unlock(myself->mutex);

    if (entries)
        JXTA_OBJECT_RELEASE(entries);
    return res;
}

Jxta_status rdv_service_add_cb(Jxta_rdv_service * me, Jxta_object ** cookie, const char *name, const char *param,
                                                Jxta_callback_func func, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(me,_jxta_rdv_service);
    _provider_callback_entry *entry=NULL;
    JString *jkey = NULL;
    Jxta_boolean locked = FALSE;

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;

    entry = provider_callback_entry_new();
    if (NULL == entry) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    entry->func = func;
    entry->arg = JXTA_OBJECT_SHARE(arg);
    entry->provider = myself->provider;
    jkey = jstring_new_2(name);

    if (NULL == jkey) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    jstring_append_2(jkey, param);
    entry->key = JXTA_OBJECT_SHARE(jkey);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding entry[%pp]  with key: %s\n",entry, jstring_get_string(entry->key));
    jxta_hashtable_put(myself->callback_table, jstring_get_string(jkey), jstring_length(jkey), (Jxta_object *)entry);
    *cookie = JXTA_OBJECT_SHARE(entry);

FINAL_EXIT:
    if (jkey)
        JXTA_OBJECT_RELEASE(jkey);
    if (entry)
        JXTA_OBJECT_RELEASE(entry);
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);

    return res;
}

static Jxta_status JXTA_STDCALL leasing_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res;
    _jxta_rdv_service *myself = PTValid(arg, _jxta_rdv_service);
    res = call_entry(obj, arg, myself->lease_key_j);
    return res;
}

static Jxta_status JXTA_STDCALL walker_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res;
    _jxta_rdv_service *myself = PTValid(arg, _jxta_rdv_service);
    res = call_entry(obj, arg, myself->walker_key_j);
    return res;
}

static Jxta_status JXTA_STDCALL prop_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res;
    _jxta_rdv_service *myself = PTValid(arg, _jxta_rdv_service);
    res = call_entry(obj, arg, myself->prop_key_j);
    return res;
}

static Jxta_status JXTA_STDCALL call_entry(Jxta_object * obj, void *arg, JString * key_j)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service *myself = PTValid(arg, _jxta_rdv_service);
    _provider_callback_entry *entry=NULL;
    Jxta_boolean locked = FALSE;

    apr_thread_mutex_lock(myself->mutex);
    locked = TRUE;
    if (NULL == myself->callback_table) goto FINAL_EXIT;

    res = jxta_hashtable_get(myself->callback_table, jstring_get_string(key_j), jstring_length(key_j), JXTA_OBJECT_PPTR(&entry));
    if (JXTA_ITEM_NOTFOUND == res) {
        goto FINAL_EXIT;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "entry_provider[%pp]  myself->provider[%pp]\n",entry->provider, myself->provider);
    if (entry->provider != myself->provider) {
        JXTA_OBJECT_RELEASE(entry);
        res = JXTA_ITEM_NOTFOUND;
        goto FINAL_EXIT;
    }
    Jxta_message *msg = (Jxta_message *) obj;

    JXTA_OBJECT_CHECK_VALID(msg);
    apr_thread_mutex_unlock(myself->mutex);
    locked = FALSE;

    res = entry->func((Jxta_object *) msg, entry->arg);
    JXTA_OBJECT_RELEASE(entry);

    if (myself->switch_config) {
        apr_thread_mutex_lock(myself->cb_mutex);
        rdv_service_switch_config((Jxta_rdv_service *) myself, myself->new_config);
        myself->switch_config = FALSE;
        apr_thread_mutex_unlock(myself->cb_mutex);
    }
FINAL_EXIT:
    if (locked)
        apr_thread_mutex_unlock(myself->mutex);
    return res;
}

/* vim: set ts=4 sw=4 tw=130 et: */
