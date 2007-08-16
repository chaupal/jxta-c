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
 * $Id$
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
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_peergroup.h"
#include "jxta_peerview.h"
#include "jxta_lease_request_msg.h"
#include "jxta_lease_response_msg.h"
#include "jxta_rdv_diffusion_msg.h"
#include "jxta_peer_private.h"
#include "jxta_peerview_priv.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"
#include "jxta_util_priv.h"

/**
 * Interval between background thread check iterations. Measured in relative microseconds.
 **/
static const Jxta_time_diff PERIODIC_THREAD_NAP_TIME_NORMAL = ((Jxta_time_diff) 60) * 1000; /* 1 Minute */

/**
*    Default max number of clients
**/
static const int DEFAULT_MAX_CLIENTS = 200;

/**
*    Default duration of leases measured in relative milliseconds.
**/
static const Jxta_time_diff DEFAULT_LEASE_DURATION = (((Jxta_time_diff) 20) * 60 * 1000L);  /* 20 Minutes */

/**
*   The maximum TTL we will allow for propagated messages.
*/
static const int DEFAULT_MAX_TTL = 2;

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

    volatile Jxta_boolean running;
    Jxta_discovery_service *discovery;

    Jxta_inputpipe *seed_input_pipe;
    Jxta_listener *seeding_listener;

    /* configuration */
    Jxta_RdvConfigAdvertisement *rdvConfig;

    unsigned int MAX_CLIENTS;
    Jxta_time_diff OFFERED_LEASE_DURATION;

    /* state */

    void *ep_cookie_leasing;
    void *ep_cookie_walker;
    Jxta_hashtable *clients;
};

/**
*    Private typedef for the rdv server provider.
**/
typedef struct _jxta_rdv_service_server _jxta_rdv_service_server;

/**
 ** Internal structure of a peer used by this provider.
 **/

struct _jxta_peer_client_entry {
    Extends(_jxta_peer_entry);

    Jxta_time creation_time;

    Jxta_vector *options;
};

typedef struct _jxta_peer_client_entry _jxta_peer_client_entry;

/**
 * Forward definition to make the compiler happy
 **/

static _jxta_rdv_service_server *jxta_rdv_service_server_construct(_jxta_rdv_service_server * myself,
                                                                   const _jxta_rdv_service_provider_methods * methods,
                                                                   apr_pool_t * pool);
static void rdv_server_delete(Jxta_object * provider);
static void Jxta_rdv_service_server_destruct(_jxta_rdv_service_server * rdv);

static Jxta_status init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service);
static Jxta_status start(Jxta_rdv_service_provider * provider);
static Jxta_status stop(Jxta_rdv_service_provider * provider);
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer ** peer);
static Jxta_status get_peers(Jxta_rdv_service_provider * provider, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                             const char *serviceParam, int ttl);
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam, const char *target_hash);
static Jxta_status walk_to_view (Jxta_rdv_service_provider * provider, Jxta_vector * view, Jxta_id * sourceId, Jxta_message * msg);

static void *APR_THREAD_FUNC periodic_task(apr_thread_t * thread, void *myself);

static Jxta_status JXTA_STDCALL leasing_cb(Jxta_object * msg, void *arg);
static void JXTA_STDCALL seeding_listener(Jxta_object * obj, void *arg);
static Jxta_status handle_lease_request(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_message_element * el);
static Jxta_status handle_lease_response(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_message_element * el);

static Jxta_status JXTA_STDCALL walker_cb(Jxta_object * obj, void *me);

static Jxta_status JXTA_STDCALL walk_handler(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_rdv_diffusion * header);

static Jxta_status send_connect_reply(_jxta_rdv_service_server * myself, Jxta_id * peerid, Jxta_time_diff lease_offered);
static _jxta_peer_client_entry *get_peer_entry(_jxta_rdv_service_server * myself, Jxta_id * peerid, Jxta_boolean create);
static Jxta_boolean check_peer_lease(_jxta_peer_client_entry * peer);

static _jxta_peer_client_entry *client_entry_new(void);
static _jxta_peer_client_entry *client_entry_construct(_jxta_peer_client_entry * myself);
static void client_entry_delete(Jxta_object * addr);
static void client_entry_destruct(_jxta_peer_client_entry * myself);

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
    get_peer,
    get_peers,
    propagate,
    walk
};

static _jxta_peer_client_entry *client_entry_new(void)
{
    _jxta_peer_client_entry *myself = (_jxta_peer_client_entry *) calloc(1, sizeof(_jxta_peer_client_entry));

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, client_entry_delete, 0);

    return client_entry_construct(myself);
}

static _jxta_peer_client_entry *client_entry_construct(_jxta_peer_client_entry * myself)
{
    myself = (_jxta_peer_client_entry *) peer_entry_construct((_jxta_peer_entry *) myself);

    if (NULL != myself) {
        myself->thisType = "_jxta_peer_client_entry";

        myself->creation_time = jpr_time_now();

        myself->options = NULL;
    }

    return myself;
}

static void client_entry_delete(Jxta_object * addr)
{
    _jxta_peer_client_entry *myself = (_jxta_peer_client_entry *) addr;

    client_entry_destruct(myself);

    free(myself);
}

static void client_entry_destruct(_jxta_peer_client_entry * myself)
{
    if (NULL != myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
    }

    peer_entry_destruct((_jxta_peer_entry *) myself);
}

Jxta_time client_entry_get_creation_time(Jxta_peer * me)
{

    _jxta_peer_client_entry *myself = PTValid(me, _jxta_peer_client_entry);

    return myself->creation_time;
}

Jxta_vector *client_entry_get_options(Jxta_peer * me)
{
    Jxta_vector *options;

    _jxta_peer_client_entry *myself = PTValid(me, _jxta_peer_client_entry);

    options = myself->options;

    if (NULL != options) {
        JXTA_OBJECT_SHARE(options);
    }

    return options;
}

int client_entry_creation_time_compare(Jxta_peer ** me, Jxta_peer ** target)
{

    _jxta_peer_client_entry *myself = PTValid(*me, _jxta_peer_client_entry);
    _jxta_peer_client_entry *themself = PTValid(*target, _jxta_peer_client_entry);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "client_entry_creation_time_compare\n");

    if (myself->creation_time < themself->creation_time) {
        return -1;
    }

    if (myself->creation_time > themself->creation_time) {
        return 1;
    }

    return 0;
}

/**
 * Standard instantiator
 */
Jxta_rdv_service_provider *jxta_rdv_service_server_new(apr_pool_t * pool)
{
    /* Allocate an instance of this service */
    _jxta_rdv_service_server *myself = (_jxta_rdv_service_server *) calloc(1, sizeof(_jxta_rdv_service_server));

    if (myself == NULL) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(myself, rdv_server_delete, 0);

    /* call the hierarchy of ctors. */
    return (Jxta_rdv_service_provider *) jxta_rdv_service_server_construct(myself, &JXTA_RDV_SERVICE_SERVER_METHODS, pool);
}

/*
 * Make sure we have a ctor that subclassers can call.
 */
static _jxta_rdv_service_server *jxta_rdv_service_server_construct(_jxta_rdv_service_server * myself,
                                                                   const _jxta_rdv_service_provider_methods * methods,
                                                                   apr_pool_t * pool)
{

    myself =
        (_jxta_rdv_service_server *) jxta_rdv_service_provider_construct((_jxta_rdv_service_provider *) myself, methods, pool);

    if (NULL != myself) {
        /* Set our rt type checking string */
        myself->thisType = "_jxta_rdv_service_server";

        myself->pool = ((_jxta_rdv_service_provider *) myself)->pool;

        myself->discovery = NULL;

        myself->seed_input_pipe = NULL;
        myself->seeding_listener = NULL;

        /** Allocate a table for storing the list of peers. We create it twice the size of the expected max number of clients so that it has low hash density. **/
        myself->clients = jxta_hashtable_new_0(DEFAULT_MAX_CLIENTS * 2, TRUE);
        myself->running = FALSE;
    }

    return myself;
}

/**
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
static void Jxta_rdv_service_server_destruct(_jxta_rdv_service_server * myself)
{
    if (myself->running) {
        stop((Jxta_rdv_service_provider *) myself);
    }

    /* Free the table of peers */
    JXTA_OBJECT_RELEASE(myself->clients);

    /* Release the services object this instance was using */
    if (myself->rdvConfig != NULL)
        JXTA_OBJECT_RELEASE(myself->rdvConfig);

    /* call the base classe's dtor. */
    jxta_rdv_service_provider_destruct((_jxta_rdv_service_provider *) myself);

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
    _jxta_rdv_service_server *myself = PTValid(provider, _jxta_rdv_service_server);
    Jxta_RdvConfigAdvertisement *rdvConfig = NULL;
    Jxta_PG *parentgroup = NULL;
    Jxta_PA *conf_adv;
    Jxta_PG *group;

    jxta_rdv_service_provider_init(provider, service);

    group = jxta_service_get_peergroup_priv((Jxta_service *) service);

    /* get the config advertisement */
    jxta_PG_get_configadv(group, &conf_adv);

    if (!conf_adv) {
        jxta_PG_get_parentgroup(group, &parentgroup);
        if (parentgroup) {
            jxta_PG_get_configadv(parentgroup, &conf_adv);
            JXTA_OBJECT_RELEASE(parentgroup);
        }
    }

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;
        jxta_PA_get_Svc_with_id(conf_adv, jxta_rendezvous_classid, &svc);
        if (NULL != svc) {
            rdvConfig = jxta_svc_get_RdvConfig(svc);
            JXTA_OBJECT_RELEASE(svc);
        }
        JXTA_OBJECT_RELEASE(conf_adv);
    }

    if (NULL == rdvConfig) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No RdvConfig Parameters. Loading defaults\n");
        rdvConfig = jxta_RdvConfigAdvertisement_new();
    }
    if (myself->rdvConfig)
        JXTA_OBJECT_RELEASE(myself->rdvConfig);
    myself->rdvConfig = rdvConfig;

    myself->MAX_CLIENTS = DEFAULT_MAX_CLIENTS;
    myself->OFFERED_LEASE_DURATION = DEFAULT_LEASE_DURATION;

    if (-1 != jxta_RdvConfig_get_lease_duration(rdvConfig)) {
        myself->OFFERED_LEASE_DURATION = jxta_RdvConfig_get_lease_duration(rdvConfig);
    }
    if (-1 != jxta_RdvConfig_get_max_clients(rdvConfig)) {
        myself->MAX_CLIENTS = jxta_RdvConfig_get_max_clients(rdvConfig);
    }
    if (-1 == jxta_RdvConfig_get_connect_cycle_normal(myself->rdvConfig)) {
        jxta_RdvConfig_set_connect_cycle_normal(myself->rdvConfig, PERIODIC_THREAD_NAP_TIME_NORMAL);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Inited for %s\n", provider->gid_uniq_str);

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
    _jxta_rdv_service_server *myself = PTValid(provider, _jxta_rdv_service_server);
    Jxta_status res;
    Jxta_PG * pg;

    jxta_rdv_service_provider_lock_priv(provider);

    if (TRUE == myself->running)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV server is already started, ignore start request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        assert(FALSE);
        return APR_SUCCESS;
    }

    pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);
    jxta_PG_get_discovery_service(pg, &myself->discovery);
    jxta_PG_add_recipient(pg, &myself->ep_cookie_leasing, RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME, leasing_cb, myself);
    jxta_PG_add_recipient(pg, &myself->ep_cookie_walker, RDV_V3_MSID, JXTA_RDV_WALKER_SERVICE_NAME, walker_cb, myself);

    res = jxta_rdv_service_provider_start(provider);

    /* Mark the service as running now. */
    myself->running = TRUE;

    apr_thread_pool_push(provider->thread_pool, periodic_task, myself, APR_THREAD_TASK_PRIORITY_HIGH, myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "Started for %s\n", provider->gid_uniq_str);

    jxta_rdv_service_provider_unlock_priv(provider);

    rdv_service_generate_event(provider->service, JXTA_RDV_BECAME_RDV, provider->local_peer_id);

    return JXTA_SUCCESS;
}

static Jxta_status open_adv_pipe(Jxta_rdv_service_provider * provider)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_server *myself = PTValid(provider, _jxta_rdv_service_server);

    /* Start listening on the pipe */
    if (NULL != provider->seed_pipe) {
        res = jxta_pipe_get_inputpipe(provider->seed_pipe, &(myself->seed_input_pipe));

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot get inputpipe: %d\n", (int) res);
            return res;
        }

        myself->seeding_listener = jxta_listener_new(seeding_listener, (void *) myself, 1, 25);

        jxta_listener_start(myself->seeding_listener);

        res = jxta_inputpipe_add_listener(myself->seed_input_pipe, myself->seeding_listener);
    }

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
    Jxta_status res;
    Jxta_PG * pg;
    _jxta_rdv_service_server *myself = PTValid(provider, _jxta_rdv_service_server);

    jxta_rdv_service_provider_lock_priv(provider);

    if (TRUE != myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RDV server is not started, ignore stop request.\n");
        jxta_rdv_service_provider_unlock_priv(provider);
        return APR_SUCCESS;
    }

    pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);
    jxta_PG_remove_recipient(pg, myself->ep_cookie_leasing);
    jxta_PG_remove_recipient(pg, myself->ep_cookie_walker);

    if (myself->discovery != NULL) {
        JXTA_OBJECT_RELEASE(myself->discovery);
        myself->discovery = NULL;
    }

    /* We need to tell the background thread that it has to die. */
    myself->running = FALSE;
    jxta_rdv_service_provider_unlock_priv(provider);

    apr_thread_pool_tasks_cancel(provider->thread_pool, myself);

    /* Remove listener on the pipe */
    if (myself->seeding_listener != NULL) {
        if (myself->seed_input_pipe != NULL) {
            jxta_inputpipe_remove_listener(myself->seed_input_pipe, myself->seeding_listener);
            JXTA_OBJECT_RELEASE(myself->seed_input_pipe);
            myself->seed_input_pipe = NULL;
        }

        jxta_listener_stop(myself->seeding_listener);
        JXTA_OBJECT_RELEASE(myself->seeding_listener);
        myself->seeding_listener = NULL;
    }

    rdv_service_generate_event(provider->service, JXTA_RDV_FAILED, provider->local_peer_id);

    /*
     *  FIXME 20050203 bondolo Should send a disconnect.
     */
    /* jxta_hashtable_clear(myself->clients); */

    res = jxta_rdv_service_provider_stop(provider);

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
    _jxta_rdv_service_server *myself = (_jxta_rdv_service_server *) PTValid(provider, _jxta_rdv_service_server);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    *peerlist = jxta_hashtable_values_get(myself->clients);

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
    _jxta_rdv_service_server *myself = (_jxta_rdv_service_server *) PTValid(provider, _jxta_rdv_service_server);
    Jxta_rdv_diffusion *header;

    JXTA_OBJECT_CHECK_VALID(msg);

    msg = jxta_message_clone(msg);

    header = jxta_rdv_diffusion_new();

    if (NULL == header) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }

    jxta_rdv_diffusion_set_src_peer_id(header, provider->local_peer_id);
    jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_MULTICAST);
    jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_GLOBAL);
    jxta_rdv_diffusion_set_ttl(header, ttl > DEFAULT_MAX_TTL ? DEFAULT_MAX_TTL : ttl);
    jxta_rdv_diffusion_set_dest_svc_name(header, serviceName);
    jxta_rdv_diffusion_set_dest_svc_param(header, serviceParam);

    res = jxta_message_remove_element_2(msg, JXTA_RDV_NS_NAME, JXTA_RDV_DIFFUSION_ELEMENT_NAME);

    res = jxta_rdv_service_provider_prop_handler(provider, msg, header);

    JXTA_OBJECT_RELEASE(header);

  FINAL_EXIT:
    JXTA_OBJECT_RELEASE(msg);

    return res;
}

/**
* Walk a message within the PeerGroup for which the instance of the 
 * Rendezvous Service is running in.
 *
* @param rdv a pointer to the instance of the Rendezvous Service
 * @param msg the Jxta_message* to propagate.
* @param serviceName pointer to a string containing the name of the service 
* on which the listener is listening on.
* @param serviceParam pointer to a string containing the parameter associated
* to the serviceName.
* @param target_hash The hash value which is being sought.
 * @return error code.
 **/
static Jxta_status walk(Jxta_rdv_service_provider * provider, Jxta_message * msg, const char *serviceName,
                        const char *serviceParam, const char *target_hash)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_server *myself = (_jxta_rdv_service_server *) PTValid(provider, _jxta_rdv_service_server);
    Jxta_rdv_diffusion *header = NULL;
    Jxta_rdv_diffusion_scope scope;
    Jxta_rdv_diffusion_policy policy;
    Jxta_vector *localView = NULL;
    Jxta_vector *globalView = NULL;
    Jxta_boolean new_walk;
    Jxta_id * sourceId = NULL;
    JXTA_OBJECT_CHECK_VALID(msg);

    msg = jxta_message_clone(msg);
    if (NULL == msg) {
        res = JXTA_NOMEM;
        goto FINAL_EXIT;
    }
    if (!jxta_peerview_is_member(provider->peerview)) {
        /* We are not in the peerview. There is no place for us to send the message. */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    res = jxta_rdv_service_provider_get_diffusion_header(msg, &header);

    if (JXTA_ITEM_NOTFOUND == res) {
        header = jxta_rdv_diffusion_new();

        if (NULL == header) {
            res = JXTA_NOMEM;
            goto FINAL_EXIT;
        }
        jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_GLOBAL);
        if (NULL == target_hash) {
            jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_MULTICAST);
        } else {
            jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL);
        }
        jxta_rdv_diffusion_set_src_peer_id(header, provider->local_peer_id);
        jxta_rdv_diffusion_set_ttl(header, 1);
        jxta_rdv_diffusion_set_target_hash(header, target_hash);
        jxta_rdv_diffusion_set_dest_svc_name(header, serviceName);
        jxta_rdv_diffusion_set_dest_svc_param(header, serviceParam);

        new_walk = TRUE;
    } else {
        /* Validate that the addressing has not changed */
        const char *current_param = jxta_rdv_diffusion_get_dest_svc_param(header);

        if (0 != strcmp(serviceName, jxta_rdv_diffusion_get_dest_svc_name(header))) {
            res = JXTA_INVALID_ARGUMENT;
            goto FINAL_EXIT;
        }

        if ((NULL != current_param) && (NULL != serviceParam)) {
            if (0 != strcmp(current_param, serviceParam)) {
                res = JXTA_INVALID_ARGUMENT;
                goto FINAL_EXIT;
            }
        } else {
            if ((NULL != current_param) || (NULL != serviceParam)) {
                res = JXTA_INVALID_ARGUMENT;
                goto FINAL_EXIT;
            }
        }
        sourceId = jxta_rdv_diffusion_get_src_peer_id(header);
        jxta_rdv_diffusion_set_src_peer_id(header, provider->local_peer_id);
        new_walk = FALSE;
    }

    /* Set the target hash for this walk. The target hash *may* change for resend walks. */
    jxta_rdv_diffusion_set_target_hash(header, target_hash);

    policy = jxta_rdv_diffusion_get_policy(header);

    scope = jxta_rdv_diffusion_get_scope(header);

    if (NULL != target_hash
        && JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL == policy
        && scope == JXTA_RDV_DIFFUSION_SCOPE_GLOBAL) {

        Jxta_peer *peer = NULL;
        BIGNUM *bn_target_hash = NULL;
        BN_hex2bn(&bn_target_hash, target_hash);

        res = jxta_peerview_get_peer_for_target_hash(provider->peerview, bn_target_hash, &peer);

        BN_free(bn_target_hash);

        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }

        assert(peer->peerid);
        if (!jxta_id_equals(((_jxta_peer_entry *) peer)->peerid, provider->local_peer_id)) {
            Jxta_PG * pg;
            JString * pid;
            pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);
            jxta_id_to_jstring(peer->peerid, &pid);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send walk message to peer %s\n", jstring_get_string(pid));
            JXTA_OBJECT_RELEASE(pid);
            jxta_PG_async_send(pg, msg, peer->peerid, RDV_V3_MSID, JXTA_RDV_WALKER_SERVICE_NAME);
        } else {
            /* It is a message for us! */
            walk_handler(myself, msg, header);
        }
        JXTA_OBJECT_RELEASE(peer);
    }

    if ((JXTA_RDV_DIFFUSION_POLICY_BROADCAST == policy) || (JXTA_RDV_DIFFUSION_POLICY_MULTICAST == policy)) {
        if (JXTA_RDV_DIFFUSION_SCOPE_GLOBAL == scope) {
            /* message transferred to our associates and partners */
            res = jxta_peerview_get_globalview(provider->peerview, &globalView);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
            res = jxta_peerview_get_localview(provider->peerview, &localView);
        } else if (JXTA_RDV_DIFFUSION_SCOPE_LOCAL == scope) {
            /* messages transferred to our partners */
            res = jxta_peerview_get_localview(provider->peerview, &localView);
        } else if (JXTA_RDV_DIFFUSION_SCOPE_TERMINAL == scope) {
            /* Don't continue if walk is in the terminal state */
            goto FINAL_EXIT;
        }

        if (JXTA_SUCCESS != res) {
            goto FINAL_EXIT;
        }

        if (NULL != globalView && jxta_vector_size(globalView) > 0) {
            res = jxta_rdv_service_provider_set_diffusion_header(msg, header);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
            res = walk_to_view(provider, globalView, sourceId, msg);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
        }
        if (NULL != localView && jxta_vector_size(localView) > 0) {
            if (new_walk) {
                jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_LOCAL);
                res = jxta_rdv_service_provider_set_diffusion_header(msg, header);
            }
            res = walk_to_view(provider, localView, sourceId, msg);
            if (JXTA_SUCCESS != res) {
                goto FINAL_EXIT;
            }
        }
    } else {
        res = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (sourceId)
        JXTA_OBJECT_RELEASE(sourceId);
    if (localView)
        JXTA_OBJECT_RELEASE(localView);
    if (globalView)
        JXTA_OBJECT_RELEASE(globalView);
    if (NULL != header)
        JXTA_OBJECT_RELEASE(header);
    if (msg)
        JXTA_OBJECT_RELEASE(msg);

    return res;
}

static Jxta_status walk_to_view (Jxta_rdv_service_provider * provider, Jxta_vector * view, Jxta_id * sourceId, Jxta_message * msg)
{
        Jxta_status res = JXTA_SUCCESS;
        unsigned int each_peer;
        unsigned int view_size = jxta_vector_size(view);
        for (each_peer = 0; each_peer < view_size; each_peer++) {
            Jxta_peer *peer = NULL;
            res = jxta_vector_get_object_at(view, JXTA_OBJECT_PPTR(&peer), each_peer);
            if (JXTA_SUCCESS != res) {
                continue;
            }
            assert(peer->peerid);
            if (!jxta_id_equals(jxta_peer_peerid(peer), provider->local_peer_id)
                    && !jxta_id_equals(jxta_peer_peerid(peer), sourceId)) {
                Jxta_PG * pg;
                JString * pid;
                pg = jxta_service_get_peergroup_priv((Jxta_service*) provider->service);
                jxta_id_to_jstring(peer->peerid, &pid);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Walk the message to peer %s\n", jstring_get_string(pid));
                JXTA_OBJECT_RELEASE(pid);
                jxta_PG_async_send(pg, msg, peer->peerid, RDV_V3_MSID, JXTA_RDV_WALKER_SERVICE_NAME);
            }
            if (peer)
                JXTA_OBJECT_RELEASE(peer);
        }
    return res;
}


/**
    Message Handler for the client lease protocol.
    
    Listener for :
        RDV_V3_MSID/"RdvLease"
        
    @param obj The message object.
    @param arg The rdv service server object

 **/
static Jxta_status JXTA_STDCALL leasing_cb(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_rdv_service_server *myself = PTValid(arg, _jxta_rdv_service_server);
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_message_element *el = NULL;
    JXTA_OBJECT_CHECK_VALID(msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_LEASE_REQUEST_ELEMENT_NAME, &el);

    if (JXTA_SUCCESS == res) {
        res = handle_lease_request(myself, msg, el);
        JXTA_OBJECT_RELEASE(el);
        return res;
    }

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_LEASE_RESPONSE_ELEMENT_NAME, &el);

    if (JXTA_SUCCESS == res) {
        res = handle_lease_response(myself, msg, el);
        JXTA_OBJECT_RELEASE(el);
        return res;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No lease request or response in msg [%pp]\n", msg);

    return JXTA_INVALID_ARGUMENT;
}

static Jxta_status handle_lease_request(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_message_element * el)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_service_provider *provider = PTValid(myself, _jxta_rdv_service_provider);
    Jxta_lease_request_msg *lease_request;
    Jxta_id *client_peerid = NULL;
    JString *client_peerid_str = NULL;
    _jxta_peer_client_entry *peer = NULL;
    Jxta_bytevector *value = jxta_message_element_get_value(el);
    JString *string;
    Jxta_time_diff lease_requested;
    Jxta_Rendezvous_event_type lease_event = 0;
    Jxta_PA *pa = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Processing lease request message [%pp]\n", msg);

    /* Build the proper endpoint address */
    string = jstring_new_3(value);
    JXTA_OBJECT_RELEASE(value);

    lease_request = jxta_lease_request_msg_new();

    if (NULL == lease_request) {
        JXTA_OBJECT_RELEASE(string);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure creating Lease Request Message [%pp]\n", msg);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    res = jxta_lease_request_msg_parse_charbuffer(lease_request, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure parsing Lease Request Message [%pp]\n", msg);
        goto FINAL_EXIT;
    }


    client_peerid = jxta_lease_request_msg_get_client_id(lease_request);

    if (NULL == client_peerid) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No client id in request [%pp]\n", lease_request);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    if (jxta_id_equals(client_peerid, provider->local_peer_id)) {
        /* ignore loopback */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    jxta_id_to_jstring(client_peerid, &client_peerid_str);

    lease_requested = jxta_lease_request_msg_get_requested_lease(lease_request);

    if ((-1L == lease_requested) || (lease_requested > myself->OFFERED_LEASE_DURATION)) {
        /* They didn't request a specific duration, give them the default. */
        /* They requested a specific duration longer than we offer, give them the default. */
        lease_requested = myself->OFFERED_LEASE_DURATION;
    }

    pa = jxta_lease_request_msg_get_client_adv(lease_request);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Lease Request (" JPR_DIFF_TIME_FMT " secs) from : %s\n",
                    (lease_requested / 1000), jstring_get_string(client_peerid_str));

    peer = get_peer_entry(myself, client_peerid, (lease_requested > 0));

    if (NULL == peer) {
        if (lease_requested > 0) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to offer lease to %s.\n",
                            jstring_get_string(client_peerid_str));
            lease_requested = 0;
        } else {
            /* Referral or bogus disconnect */
        }
    } else {
        jxta_peer_lock((Jxta_peer *) peer);

        if (NULL != pa) {
            jxta_peer_set_adv((Jxta_peer *) peer, pa);
        }

        if (lease_requested > 0) {
            Jxta_time_diff current_exp = jxta_peer_get_expires((Jxta_peer *) peer);

            lease_event = (current_exp != JPR_ABSOLUTE_TIME_MAX) ? JXTA_RDV_CLIENT_RECONNECTED : JXTA_RDV_CLIENT_CONNECTED;
            jxta_peer_set_expires((Jxta_peer *) peer, jpr_time_now() + lease_requested);

            /* Copy any options */
            if (NULL != peer->options) {
                JXTA_OBJECT_RELEASE(peer->options);
                peer->options = NULL;
            }

            peer->options = jxta_lease_request_msg_get_options(lease_request);
        } else {
            /* Disconnect */
            jxta_peer_set_expires((Jxta_peer *) peer, 0);
            lease_event = JXTA_RDV_CLIENT_DISCONNECTED;
        }

        jxta_peer_unlock((Jxta_peer *) peer);
    }

    /* publish PA */
    if (NULL != pa) {
        Jxta_time_diff pa_exp = jxta_lease_request_msg_get_client_adv_exp(lease_request);

        if (-1 == pa_exp) {
            pa_exp = DEFAULT_LEASE_DURATION;
        }

        if (myself->discovery == NULL) {
            jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((Jxta_service *) provider->service),
                                          &(myself->discovery));
        }

        if (myself->discovery != NULL) {
            discovery_service_publish(myself->discovery, (Jxta_advertisement *) pa, DISC_PEER,
                                      (Jxta_expiration_time) pa_exp, LOCAL_ONLY_EXPIRATION);
        }
    }

    /*
     * Send a response to a referral or lease request.
     */
    if (((NULL == peer) && (0 == lease_requested)) || ((NULL != peer) && (lease_requested > 0))) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "[%pp] Sending lease offer (" JPR_DIFF_TIME_FMT "secs) to : %s\n",
                        myself, (lease_requested / 1000), jstring_get_string(client_peerid_str));

        res = send_connect_reply(myself, client_peerid, lease_requested);

        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send lease response to %s.\n",
                            jstring_get_string(client_peerid_str));
            goto FINAL_EXIT;
        }
    }

    if (NULL != peer) {
        /*
         *  notify the RDV listeners about the client activity
         */
        assert(lease_event != 0);
        rdv_service_generate_event(provider->service, lease_event, client_peerid);
    }


    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != lease_request) {
        JXTA_OBJECT_RELEASE(lease_request);
    }

    if (NULL != client_peerid) {
        JXTA_OBJECT_RELEASE(client_peerid);
    }

    if (NULL != client_peerid_str) {
        JXTA_OBJECT_RELEASE(client_peerid_str);
    }

    if (NULL != pa) {
        JXTA_OBJECT_RELEASE(pa);
    }

    if (NULL != peer) {
        JXTA_OBJECT_RELEASE(peer);
    }

    return res;
}

static Jxta_status handle_lease_response(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_message_element * el)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_service_provider *provider = PTValid(myself, _jxta_rdv_service_provider);
    Jxta_lease_response_msg *lease_response = NULL;
    Jxta_id *server_peerid = NULL;
    JString *server_peerid_str = NULL;
    Jxta_lease_adv_info *server_info = NULL;
    Jxta_PA *server_adv = NULL;
    apr_uuid_t *server_adv_gen = NULL;
    Jxta_time_diff server_adv_exp = -1L;
    Jxta_bytevector *value = jxta_message_element_get_value(el);
    JString *string;
    Jxta_vector *referrals;
    unsigned int each_referral;
    unsigned int all_referrals;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Processing lease response message [%pp]\n", msg);

    /* Build the proper endpoint address */
    string = jstring_new_3(value);
    JXTA_OBJECT_RELEASE(value);

    lease_response = jxta_lease_response_msg_new();

    if (NULL == lease_response) {
        JXTA_OBJECT_RELEASE(string);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure creating Lease Request Message [%pp]\n", msg);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    res = jxta_lease_response_msg_parse_charbuffer(lease_response, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure parsing Lease Response Message [%pp]\n", msg);
        goto FINAL_EXIT;
    }

    server_peerid = jxta_lease_response_msg_get_server_id(lease_response);

    if (NULL == server_peerid) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No server id in response [%pp]\n", lease_response);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    if (jxta_id_equals(server_peerid, provider->local_peer_id)) {
        /* ignore loopback */
        res = JXTA_SUCCESS;
        goto FINAL_EXIT;
    }

    jxta_id_to_jstring(server_peerid, &server_peerid_str);

    server_info = jxta_lease_response_msg_get_server_adv_info(lease_response);

    if (NULL == server_info) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No server adv info in response [%pp]\n", lease_response);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    server_adv = jxta_lease_adv_info_get_adv(server_info);
    server_adv_gen = jxta_lease_adv_info_get_adv_gen(server_info);
    server_adv_exp = jxta_lease_adv_info_get_adv_exp(server_info);

    if (-1 == server_adv_exp) {
        server_adv_exp = myself->OFFERED_LEASE_DURATION;
    }

    /* Save this response as a referral. */
    if (NULL != server_adv) {
        Jxta_peer *server_seed = jxta_peer_new();

        jxta_rdv_service_provider_lock_priv(provider);

        /* Make sure discovery is available. */
        if (myself->discovery == NULL) {
            jxta_PG_get_discovery_service(jxta_service_get_peergroup_priv((Jxta_service *) provider->service),
                                          &(myself->discovery));
        }

        jxta_rdv_service_provider_unlock_priv(provider);

        jxta_peer_set_adv(server_seed, server_adv);
        jxta_peer_set_peerid(server_seed, server_peerid);
        jxta_peer_set_expires(server_seed, jpr_time_now() + server_adv_exp);

        rdv_service_add_referral_seed((Jxta_rdv_service *) provider->service, server_seed);
        JXTA_OBJECT_RELEASE(server_seed);

        if (myself->discovery != NULL) {
            discovery_service_publish(myself->discovery, (Jxta_advertisement *) server_adv, DISC_PEER,
                                      (Jxta_expiration_time) server_adv_exp, LOCAL_ONLY_EXPIRATION);
        }
    }

    /* Handle referrals! */
    referrals = jxta_lease_response_msg_get_referral_advs(lease_response);
    all_referrals = jxta_vector_size(referrals);
    for (each_referral = 0; each_referral < all_referrals; each_referral++) {
        Jxta_lease_adv_info *referral = NULL;

        res = jxta_vector_get_object_at(referrals, JXTA_OBJECT_PPTR(&referral), each_referral);
        if (JXTA_SUCCESS != res) {
            continue;
        }

        if (jxta_lease_adv_info_get_adv_exp(referral) > 0) {
            Jxta_peer *referral_seed = jxta_peer_new();
            Jxta_PA *referral_adv = jxta_lease_adv_info_get_adv(referral);

            jxta_peer_set_adv(referral_seed, referral_adv);
            jxta_peer_set_peerid(referral_seed, jxta_PA_get_PID(referral_adv));
            JXTA_OBJECT_RELEASE(referral_adv);
            jxta_peer_set_expires(referral_seed, jpr_time_now() + jxta_lease_adv_info_get_adv_exp(referral));

            rdv_service_add_referral_seed((Jxta_rdv_service *) provider->service, referral_seed);
            JXTA_OBJECT_RELEASE(referral_seed);
            JXTA_OBJECT_RELEASE(referral_adv);
        }

        JXTA_OBJECT_RELEASE(referral);
    }
    JXTA_OBJECT_RELEASE(referrals);

    res = JXTA_SUCCESS;

  FINAL_EXIT:
    if (NULL != lease_response) {
        JXTA_OBJECT_RELEASE(lease_response);
    }

    if (NULL != server_peerid) {
        JXTA_OBJECT_RELEASE(server_peerid);
    }

    if (NULL != server_peerid_str) {
        JXTA_OBJECT_RELEASE(server_peerid_str);
    }

    if (NULL != server_info) {
        JXTA_OBJECT_RELEASE(server_info);
    }

    if (NULL != server_adv) {
        JXTA_OBJECT_RELEASE(server_adv);
    }

    if (NULL != server_adv_gen) {
        free(server_adv_gen);
    }

    return res;
}

static Jxta_status send_connect_reply(_jxta_rdv_service_server * myself, Jxta_id * pid, Jxta_time_diff lease_offered)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_service_provider *provider = PTValid(myself, _jxta_rdv_service_provider);
    Jxta_lease_response_msg *lease_response;
    JString *lease_response_xml = NULL;
    Jxta_message *msg = NULL;
    Jxta_message_element *msgElem;
    apr_uuid_t adv_gen;

    lease_response = jxta_lease_response_msg_new();

    if (NULL == lease_response) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failure creating Lease Response Message [%pp]\n", msg);
        res = JXTA_FAILED;
        goto FINAL_EXIT;
    }

    jxta_lease_response_msg_set_server_id(lease_response, provider->local_peer_id);
    jxta_lease_response_msg_set_offered_lease(lease_response, lease_offered);

    /* FIXME 20060811 bondolo Use a real advertisement generation */
    apr_uuid_get(&adv_gen);
    jxta_lease_response_msg_set_server_adv_info(lease_response, provider->local_pa, &adv_gen, DEFAULT_LEASE_DURATION * 2);

    /* FIXME 20060812 bondolo Generate referrals. */

    res = jxta_lease_response_msg_get_xml(lease_response, &lease_response_xml);
    JXTA_OBJECT_RELEASE(lease_response);
    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed to generate lease response xml. [%pp]\n", myself);
        goto FINAL_EXIT;
    }

    msgElem = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_LEASE_RESPONSE_ELEMENT_NAME,
                                         "text/xml", jstring_get_string(lease_response_xml), jstring_length(lease_response_xml),
                                         NULL);
    JXTA_OBJECT_RELEASE(lease_response_xml);

    if (NULL == msgElem) {
        goto FINAL_EXIT;
    }

    msg = jxta_message_new();

    if (NULL == msg) {
        JXTA_OBJECT_RELEASE(msgElem);
        goto FINAL_EXIT;
    }

    jxta_message_add_element(msg, msgElem);
    JXTA_OBJECT_RELEASE(msgElem);

    res = jxta_PG_sync_send(jxta_service_get_peergroup_priv((Jxta_service *) provider->service), msg, pid, 
                            RDV_V3_MSID, JXTA_RDV_LEASING_SERVICE_NAME);
    if (JXTA_SUCCESS != res) {
        JString * pid_jstr;

        jxta_id_to_jstring(pid, &pid_jstr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to send lease offer message. (%s)\n", 
                        jstring_get_string(pid_jstr));
        JXTA_OBJECT_RELEASE(pid_jstr);
    }
    JXTA_OBJECT_RELEASE(msg);

  FINAL_EXIT:
    return res;
}

/**
 *  The entry point to the periodic thread which manages peer connections.
 **/
static void *APR_THREAD_FUNC periodic_task(apr_thread_t * thread, void *arg)
{
    Jxta_rdv_service_provider *provider = PTValid(arg, _jxta_rdv_service_provider);
    _jxta_rdv_service_server *myself = PTValid(arg, _jxta_rdv_service_server);
    Jxta_status res;
    Jxta_vector *clients;
    unsigned int sz;
    unsigned int i;

    if (!myself->running) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Server[%pp] is stopped, discard periodic task.\n", myself);
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Server[%pp] periodic task RUN.\n", myself);

    clients = jxta_hashtable_values_get(myself->clients);
    sz = jxta_vector_size(clients);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Server periodic task %d clients.\n", sz);
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

            jxta_id_get_uniqueportion(jxta_peer_peerid((Jxta_peer *) peer), &uniq);
            res = jxta_hashtable_del(myself->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, NULL);
            JXTA_OBJECT_RELEASE(uniq);
            rdv_service_generate_event(provider->service, JXTA_RDV_CLIENT_DISCONNECTED,
                                       jxta_peer_peerid((Jxta_peer *) peer));
        }

        JXTA_OBJECT_RELEASE(peer);
    }

    JXTA_OBJECT_RELEASE(clients);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Server[%pp] periodic task DONE.\n", myself);

    /*
     * TODO: exponential backoff delay to avoid constant * retry to RDV seed.
     */
    jxta_rdv_service_provider_lock_priv(provider);
    if (myself->running) {
        apr_thread_pool_schedule(provider->thread_pool, periodic_task, myself,
                                jxta_RdvConfig_get_connect_cycle_normal(myself->rdvConfig) * 1000, myself);
    }
    jxta_rdv_service_provider_unlock_priv(provider);
                             
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
    Jxta_time expires = jxta_peer_get_expires((Jxta_peer *) peer);
    Jxta_time_diff remaining = (Jxta_time_diff) (expires - currentTime);

    /* any lease remaining ? */
    return (remaining > 0);
}

/**
 * Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param provider a pointer to the instance of the Rendezvous Service
 * @param peerid The peerID of the peer sought.
 * @param peer The result.
 * @return error code.
 **/
static Jxta_status get_peer(Jxta_rdv_service_provider * provider, Jxta_id * peerid, Jxta_peer ** peer)
{
    _jxta_rdv_service_server *myself = (_jxta_rdv_service_server *) PTValid(provider, _jxta_rdv_service_server);

    /* Test arguments first */
    if ((NULL == peerid) || (NULL == peer)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    *peer = (Jxta_peer *) get_peer_entry(myself, peerid, FALSE);

    if (NULL == *peer) {
        return JXTA_ITEM_NOTFOUND;
    }

    return JXTA_SUCCESS;
}

/**
 * Returns the _jxta_peer_rdv_entry associated to a peer or optionally create one
 *
 *  @param myself The rendezvous server
 *  @param addr The address of the desired peer.
 *  @param create If TRUE then create a new entry if there is no existing entry 
 **/
static _jxta_peer_client_entry *get_peer_entry(_jxta_rdv_service_server * myself, Jxta_id * peerid, Jxta_boolean create)
{
    _jxta_peer_client_entry *peer = NULL;
    Jxta_status res = 0;
    Jxta_boolean found = FALSE;
    JString *uniq;
    size_t currentPeers;

    jxta_id_get_uniqueportion(peerid, &uniq);

    jxta_rdv_service_provider_lock_priv((Jxta_rdv_service_provider *) myself);

    res = jxta_hashtable_get(myself->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, JXTA_OBJECT_PPTR(&peer));

    found = res == JXTA_SUCCESS;

    jxta_hashtable_stats(myself->clients, NULL, &currentPeers, NULL, NULL, NULL);

    if (!found && create && (myself->MAX_CLIENTS > currentPeers)) {
        /* We need to create a new _jxta_peer_rdv_entry */
        peer = client_entry_new();
        if (peer == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Cannot create a _jxta_peer_rdv_entry\n");
        } else {
            Jxta_endpoint_address *clientAddr = jxta_endpoint_address_new_3(peerid, NULL, NULL);

            jxta_peer_set_address((Jxta_peer *) peer, clientAddr);
            jxta_peer_set_peerid((Jxta_peer *) peer, peerid);
            jxta_peer_set_expires((Jxta_peer *) peer, JPR_ABSOLUTE_TIME_MAX);

            jxta_hashtable_put(myself->clients, jstring_get_string(uniq), jstring_length(uniq) + 1, (Jxta_object *) peer);
            JXTA_OBJECT_RELEASE(clientAddr);
        }
    }

    jxta_rdv_service_provider_unlock_priv((Jxta_rdv_service_provider *) myself);

    JXTA_OBJECT_RELEASE(uniq);

    return peer;
}

/*
    Message Handler for the walker protocol.
    
    Listener for :
        RDV_V3_MSID/"RdvWalker"
        
    @param obj The message object.
    @param arg The rdv service server object
    @remarks When a walk message was received, what the listener will do is simply pass it to the handler based on the destination
             of the internal message. The handler then should decide what to do, it can continue the walk by pass the msg with
             walk header back to rdv.
 */
static Jxta_status JXTA_STDCALL walker_cb(Jxta_object * obj, void *me)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_message *msg = (Jxta_message *) obj;
    _jxta_rdv_service_server *myself = PTValid(me, _jxta_rdv_service_server);
    Jxta_rdv_diffusion *header = NULL;
    Jxta_rdv_diffusion_scope scope;
    Jxta_rdv_diffusion_policy policy;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Rendezvous Service walker received a message [%pp]\n", msg);

    res = jxta_rdv_service_provider_get_diffusion_header(msg, &header);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    /* Adjust scope */
    scope = jxta_rdv_diffusion_get_scope(header);

    if (JXTA_RDV_DIFFUSION_SCOPE_GLOBAL == scope) {
        jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_LOCAL);
    }
    if (JXTA_RDV_DIFFUSION_SCOPE_LOCAL == scope) {
        /* FIXME 20060817 bondolo We need a better policy for reducing scope local to scope terminal */
        jxta_rdv_diffusion_set_scope(header, JXTA_RDV_DIFFUSION_SCOPE_TERMINAL);
    }

    /* Adjust policy */
    policy = jxta_rdv_diffusion_get_policy(header);

    if (JXTA_RDV_DIFFUSION_POLICY_BROADCAST == policy) {
        jxta_rdv_diffusion_set_policy(header, JXTA_RDV_DIFFUSION_POLICY_MULTICAST);
    }

    /* Call handler */
    res = walk_handler(myself, msg, header);

    if (NULL != header) {
        JXTA_OBJECT_RELEASE(header);
    }
    return res;
}

static Jxta_status JXTA_STDCALL walk_handler(_jxta_rdv_service_server * myself, Jxta_message * msg, Jxta_rdv_diffusion * header)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_rdv_service_provider *provider = PTValid(myself, _jxta_rdv_service_provider);

    Jxta_rdv_diffusion_policy policy = jxta_rdv_diffusion_get_policy(header);

    if (JXTA_RDV_DIFFUSION_POLICY_MULTICAST == policy) {
        /* We are walking a flood message, pass it on to the propagation code */
        res = jxta_rdv_service_provider_prop_handler(provider, msg, header);
    }
    if (JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL == policy) {
        Jxta_endpoint_address *realDest = jxta_endpoint_address_new_2("jxta", provider->gid_uniq_str,
                                                                      jxta_rdv_diffusion_get_dest_svc_name(header),
                                                                      jxta_rdv_diffusion_get_dest_svc_param(header));

        res = jxta_rdv_service_provider_set_diffusion_header(msg, header);
        if (JXTA_SUCCESS == res) {
            jxta_endpoint_service_demux_addr(provider->service->endpoint, realDest, msg);
            JXTA_OBJECT_RELEASE(realDest);
        }
    }
    return res;
}

static void JXTA_STDCALL seeding_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = leasing_cb(obj, arg);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error in listener :%d \n", res);
    }
}

/* vim: set ts=4 sw=4  tw=130 et: */
