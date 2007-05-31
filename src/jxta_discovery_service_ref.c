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
 * $Id: jxta_discovery_service_ref.c,v 1.76 2005/03/02 22:37:57 slowhog Exp $
 */

#include <limits.h>
#include <stdlib.h>

#include "jpr/jpr_excep.h"
#include "jpr/jpr_thread.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxtaapr.h"
#include "jxta_id.h"
#include "jxta_cm.h"
#include "jxta_rq.h"
#include "jxta_rr.h"
#include "jxta_dq.h"
#include "jxta_dr.h"
#include "jxta_rdv.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_endpoint_service.h"
#include "jxta_routea.h"
#include "jxta_private.h"
#include "jxta_discovery_service_private.h"
#include "jxta_rdv_service.h"
#include "jxta_srdi.h"
#include "jxta_rsrdi.h"

#define HAVE_POOL
#define CM_ADV_RUN_CYCLE         (30 * 1000 * 1000)     /* 30 sec */
#define CM_ADV_WAIT_RDV_CONNECT  (8 * 1000 * 1000)      /* 8 sec */

typedef struct {

    Extends(Jxta_discovery_service);
    JString *instanceName;
    volatile boolean running;
    apr_thread_t *thread;
    apr_thread_mutex_t *mutex;
    Jxta_PG *group;
    JString *gid_str;
    Jxta_resolver_service *resolver;
    Jxta_endpoint_service *endpoint;
    Jxta_rdv_service *rdv;
    /* vector to hold generic discovery listeners */
    Jxta_vector *listener_vec;
    /* hashtable to hold specific query listeners */
    Jxta_hashtable *listeners;
    Jxta_PID *localPeerId;
    Jxta_id *assigned_id;
    Jxta_advertisement *impl_adv;
    Jxta_cm *cm;
    boolean cm_on;
    apr_pool_t *pool;
} Jxta_discovery_service_ref;

static void discovery_service_query_listener(Jxta_object * obj, void *arg);
static void discovery_service_response_listener(Jxta_object * obj, void *arg);
static void discovery_service_rdv_listener(Jxta_object * obj, void *arg);
static void *APR_THREAD_FUNC advertisement_handling_thread(apr_thread_t * thread, void *arg);
Jxta_status jxta_discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean delta);
Jxta_status jxta_discovery_send_srdi(Jxta_discovery_service_ref * discovery, JString * pkey, Jxta_vector * entries);

static const char *dirname[] = { "Peers", "Groups", "Adv"
};
static const char *cm_home = ".cm";

/*
 * module methods
 */

/**
 * Initializes an instance of the Discovery Service.
 * 
 * @param service a pointer to the instance of the Discovery Service.
 * @param group a pointer to the PeerGroup the Discovery Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

Jxta_status
jxta_discovery_service_ref_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_listener *listener = NULL;
    Jxta_status status = JXTA_SUCCESS;
    Jxta_PGID *gid = NULL;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) module;
    char *home = NULL;
    const char *p_keywords[][2] = {
        {"Name", NULL},
        {"PID", NULL},
        {"Desc", NULL},
        {NULL, NULL}
    };
    const char *g_keywords[][2] = {
        {"Name", NULL},
        {"GID", NULL},
        {"Desc", NULL},
        {"MSID", NULL},
        {NULL, NULL}
    };
    const char *a_keywords[][2] = {
        {"Name", NULL},
        {"Name", "NameAttribute"},
        {"Id", NULL},
        {"MSID", NULL},
        {"RdvGroupId", NULL},
        {"DstPID", NULL},
        {NULL, NULL}
    };

    /* Test arguments first */
    if ((discovery == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    apr_pool_create(&discovery->pool, NULL);
    /* Create the mutex */
    apr_thread_mutex_create(&discovery->mutex, APR_THREAD_MUTEX_NESTED, /* nested */
                            discovery->pool);

    /* store a copy of our assigned id */
    if (assigned_id != 0) {
        JXTA_OBJECT_SHARE(assigned_id);
        discovery->assigned_id = assigned_id;
    }

    /* keep a reference to our group and impl adv */
    if (group != 0)
        JXTA_OBJECT_SHARE(group);
    if (impl_adv != 0)
        JXTA_OBJECT_SHARE(impl_adv);
    discovery->group = group;
    discovery->impl_adv = impl_adv;
    /**
     ** Build the local name of the instance
     **/
    discovery->instanceName = NULL;
    jxta_id_to_jstring(discovery->assigned_id, &discovery->instanceName);
    jxta_PG_get_resolver_service(group, &(discovery->resolver));
    jxta_PG_get_endpoint_service(group, &(discovery->endpoint));
    jxta_PG_get_rendezvous_service(group, &(discovery->rdv));
    jxta_PG_get_PID(group, &(discovery->localPeerId));
    jxta_PG_get_GID(group, &gid);
    jxta_id_to_jstring(gid, &discovery->gid_str);
    JXTA_OBJECT_SHARE(discovery->gid_str);
    JXTA_OBJECT_RELEASE(gid);

    discovery->cm_on = FALSE;

    /**
     * FIXME 
     * Register all the adevertisement type we're to deal with
     * typically this would be during boot rather in discovery
     */
    jxta_advertisement_register_global_handler("jxta:PA", (JxtaAdvertisementNewFunc) jxta_PA_new);
    jxta_advertisement_register_global_handler("jxta:PGA", (JxtaAdvertisementNewFunc) jxta_PGA_new);
    jxta_advertisement_register_global_handler("jxta:MIA", (JxtaAdvertisementNewFunc) jxta_MIA_new);
    jxta_advertisement_register_global_handler("jxta:RdvAdvertisement", (JxtaAdvertisementNewFunc) jxta_RdvAdvertisement_new);
    jxta_advertisement_register_global_handler("jxta:PipeAdvertisement", (JxtaAdvertisementNewFunc) jxta_pipe_adv_new);
    jxta_advertisement_register_global_handler("jxta:RA", (JxtaAdvertisementNewFunc) jxta_RouteAdvertisement_new);
    jxta_advertisement_register_global_handler("jxta:APA", (JxtaAdvertisementNewFunc) jxta_AccessPointAdvertisement_new);

    home = getenv("JXTA_HOME");
    if (home != NULL) {
        discovery->cm = jxta_cm_new(home, gid);
    } else {
        discovery->cm = jxta_cm_new((char *) cm_home, gid);
    }

    if (discovery->cm == NULL) {
        JXTA_LOG("could not create Cache Manager\n");
        discovery->cm_on = FALSE;
    } else {
        discovery->cm_on = TRUE;
    }

    if (discovery->cm_on) {

        /* create the folders for the group and add the indexes from the typical advertisements */
        Jxta_vector *jv = NULL;

        jxta_cm_create_folder(discovery->cm, (char *) dirname[DISC_PEER], p_keywords);
        jv = jxta_PA_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_PEER], jv);
        JXTA_OBJECT_RELEASE(jv);

        jxta_cm_create_folder(discovery->cm, (char *) dirname[DISC_GROUP], g_keywords);
        jv = jxta_PGA_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_GROUP], jv);
        JXTA_OBJECT_RELEASE(jv);

        jxta_cm_create_folder(discovery->cm, (char *) dirname[DISC_ADV], a_keywords);

        jv = jxta_RendezvousAdvertisement_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_ADV], jv);
        JXTA_OBJECT_RELEASE(jv);

        jv = jxta_MIA_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_ADV], jv);
        JXTA_OBJECT_RELEASE(jv);

        jv = jxta_pipe_adv_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_ADV], jv);
        JXTA_OBJECT_RELEASE(jv);

        jv = jxta_RouteAdvertisement_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_ADV], jv);
        JXTA_OBJECT_RELEASE(jv);

        jv = jxta_AccessPointAdvertisement_get_indexes();
        jxta_cm_create_adv_indexes(discovery->cm, (char *) dirname[DISC_ADV], jv);
        JXTA_OBJECT_RELEASE(jv);

    }

    listener = jxta_listener_new((Jxta_listener_func) discovery_service_query_listener, discovery, 1, 200);
    jxta_listener_start(listener);

    status = jxta_resolver_service_registerQueryHandler(discovery->resolver, discovery->instanceName, listener);
    JXTA_OBJECT_RELEASE(listener);

    if (status == JXTA_SUCCESS) {
        listener = jxta_listener_new((Jxta_listener_func) discovery_service_response_listener, discovery, 1, 200);
        jxta_listener_start(listener);
        status = jxta_resolver_service_registerResHandler(discovery->resolver, discovery->instanceName, listener);
        JXTA_OBJECT_RELEASE(listener);
    }

    /*
     * Register to RDV service fro RDV events
     */
    listener = jxta_listener_new((Jxta_listener_func) discovery_service_rdv_listener, discovery, 1, 1);

    jxta_listener_start(listener);

    status = jxta_rdv_service_add_event_listener(discovery->rdv,
                                                 (char *) jstring_get_string(discovery->instanceName),
                                                 (char *) jstring_get_string(discovery->gid_str), listener);
    JXTA_OBJECT_RELEASE(listener);

    /* Create vector and hashtable for the listeners */
    discovery->listener_vec = jxta_vector_new(1);
    discovery->listeners = jxta_hashtable_new(1);

    /* create a thread to be initially used to send SRDI index
     * updates and the later purge expired advertisements
     */

    apr_thread_create(&discovery->thread,
                      NULL, advertisement_handling_thread, (void *) discovery, (apr_pool_t *) discovery->pool);

    return status;
}

/**
 * Initializes an instance of the Discovery Service. (exception variant).
 * 
 * @param service a pointer to the instance of the Discovery Service.
 * @param group a pointer to the PeerGroup the Discovery Service is 
 * initialized for.
 *
 */

static void init_e(Jxta_module * discovery, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws)
{

    Jxta_status s = jxta_discovery_service_ref_init(discovery,
                                                    group,
                                                    assigned_id,
                                                    impl_adv);
    if (s != JXTA_SUCCESS) {
        Throw(s);
    }
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param discovery a pointer to the instance of the Discovery Service
 * @param argv a vector of string arguments.
 */
static Jxta_status start(Jxta_module * discovery, char *argv[])
{
    /* no startup sequence dependancy, otherwise there would be something here */
    return JXTA_SUCCESS;
}


/**
 * Stops an instance of the Discovery Service.
 * 
 * @param service a pointer to the instance of the Discovery Service
 * @return Jxta_status
 */
static void stop(Jxta_module * module)
{
    apr_status_t status;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) module;
    PTValid(discovery, Jxta_discovery_service_ref);

    /* Test arguments first */
    if (discovery == NULL) {
        /* Invalid args. */
        return;
    }
    /* We need to tell the background thread that it has to die. */
    apr_thread_mutex_lock(discovery->mutex);
    discovery->running = FALSE;

    /*apr_thread_join(&status, discovery->thread); */

    status = jxta_resolver_service_unregisterQueryHandler(discovery->resolver, discovery->instanceName);
    status = jxta_resolver_service_unregisterResHandler(discovery->resolver, discovery->instanceName);

    jxta_rdv_service_remove_event_listener(discovery->rdv,
                                           (char *) jstring_get_string(discovery->instanceName),
                                           (char *) jstring_get_string(discovery->gid_str));

    /* XXX FIXME to be removed when group stop is fully functional */
    jxta_cm_close(discovery->cm);
    apr_thread_mutex_unlock(discovery->mutex);
    JXTA_LOG("Stopped.\n");
}

/*
 * service methods
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Discovery Service
 * @return the advertisement.
 */
static void get_mia(Jxta_service * discovery, Jxta_advertisement ** mia)
{

    Jxta_discovery_service_ref *self = (Jxta_discovery_service_ref *) discovery;
    PTValid(self, Jxta_discovery_service_ref);

    JXTA_OBJECT_SHARE(self->impl_adv);
    *mia = self->impl_adv;
}

/**
 * return an object that proxies this service. (a service method).
 *
 * @return this service object itself.
 */
static void get_interface(Jxta_service * self, Jxta_service ** svc)
{
    PTValid(self, Jxta_discovery_service_ref);

    JXTA_OBJECT_RELEASE(svc);
    *svc = self;
}

/*
 * Discovery methods proper.
 */

long
getRemoteAdv(Jxta_discovery_service * service,
             Jxta_id * peerid,
             short type, const char *attribute, const char *value, int threshold, Jxta_discovery_listener * listener)
{
    Jxta_status status;
    JString *peerdoc = NULL;
    JString *qdoc = NULL;
    Jxta_DiscoveryQuery *query = NULL;
    ResolverQuery *res_query = NULL;
    Jxta_PA *padv;
    Jxta_RouteAdvertisement *route;

    long qid = 0;
    char *buf = malloc(128);    /* More than enough */

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;

    jxta_PG_get_PA(discovery->group, &padv);

    status = jxta_PA_get_xml(padv, &peerdoc);
    JXTA_OBJECT_RELEASE(padv);

    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "discovery::getRemoteAdv failed to retrieve MyPeer Advertisement\n");
        return -1;
    }
    query = jxta_discovery_query_new_1(type, attribute, value, threshold, peerdoc);

    JXTA_OBJECT_RELEASE(peerdoc);

    status = jxta_discovery_query_get_xml((Jxta_advertisement *)
                                          query, &qdoc);
    JXTA_OBJECT_RELEASE(query);

    if (status == JXTA_SUCCESS) {
        route = jxta_endpoint_service_get_local_route(discovery->endpoint);
        res_query = jxta_resolver_query_new_1(discovery->instanceName, qdoc, discovery->localPeerId, route);

        JXTA_OBJECT_RELEASE(qdoc);

        if (route != NULL)
            JXTA_OBJECT_RELEASE(route);

        status = jxta_resolver_service_sendQuery(discovery->resolver, res_query, peerid);
    } else {
        fprintf(stderr, "discovery::getRemoteAdv failed to construct query: %ld\n", status);
        return -1;
    }
    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "discovery::getRemoteAdv failed to send query: %ld\n", status);
        return -1;
    }

    qid = jxta_resolver_query_get_queryid(res_query);
    JXTA_OBJECT_RELEASE(res_query);

    /* make sure we have a listener */
    if (listener != NULL) {
        sprintf(buf, "%ld", qid);
        jxta_hashtable_put(discovery->listeners, (const void *) buf, strlen(buf), (Jxta_object *) listener);
    }

    free(buf);
    return qid;
}

Jxta_status
getLocalAdv(Jxta_discovery_service * service, short type, const char *attribute, const char *value, Jxta_vector ** advertisements)
{

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;
    char **keys;
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    int i = 0;

    PTValid(discovery, Jxta_discovery_service_ref);

    if (attribute == NULL || attribute[0] == 0) {
        keys = jxta_cm_get_primary_keys(discovery->cm, (char *) dirname[type]);
    } else {

        keys = jxta_cm_search(discovery->cm, (char *) dirname[type], attribute, value, INT_MAX);
    }

    if ((keys == NULL) || (keys[0] == NULL)) {

        JXTA_LOG("No items found \n");

        /*
         * The convention so far seems to be that returning NULL
         * is the way to say "nothing found. Since we always
         * return JXTA_SUCCESS, we'd better set NULL forcefully,
         * otherwise the reading of the result would always depend on
         * the initial value of the return ptr, which is bad practice.
         */

        *advertisements = NULL;

        free(keys);
        return JXTA_SUCCESS;
    }

    JXTA_LOG("Found some items\n");

    while (keys[i]) {
        JString *res_bytes = NULL;
        Jxta_advertisement *root_adv = NULL;
        Jxta_vector *res_vect = NULL;

        JXTA_LOG("retrieving %s \n", keys[i]);
        status = jxta_cm_restore_bytes(discovery->cm, (char *) dirname[type], keys[i], &res_bytes);
        free(keys[i]);
        if (status != JXTA_SUCCESS) {
            ++i;
            continue;
        }
        root_adv = jxta_advertisement_new();
        jxta_advertisement_parse_charbuffer(root_adv, jstring_get_string(res_bytes), jstring_length(res_bytes));
        JXTA_OBJECT_RELEASE(res_bytes);

        jxta_advertisement_get_advs(root_adv, &res_vect);
        JXTA_OBJECT_RELEASE(root_adv);
        if (res_vect == NULL) {
            ++i;
            continue;
        }
        if (jxta_vector_size(res_vect) != 0) {
            Jxta_object *res_adv;

            /*
             * Since we're supposed to return NULL when we find nothing,
             * we have to create the vector only if we get at least one
             * successfully processed result.
             */
            if (responses == NULL) {
                responses = jxta_vector_new(1);
            }
            jxta_vector_get_object_at(res_vect, &res_adv, 0);
            jxta_vector_add_object_last(responses, res_adv);

            JXTA_OBJECT_RELEASE(res_adv);
        }
        JXTA_OBJECT_RELEASE(res_vect);
        i++;
    }

    free(keys);

    *advertisements = responses;
    return JXTA_SUCCESS;
}

Jxta_status
publish(Jxta_discovery_service * service,
        Jxta_advertisement * adv, short type, Jxta_expiration_time lifetime, Jxta_expiration_time lifetimeForOthers)
{

    int stat = -1;
    Jxta_id *id = NULL;
    JString *id_str = NULL;
    Jxta_status status = JXTA_FAILED;
    Jxta_discovery_service_ref *self = (Jxta_discovery_service_ref *) service;
    PTValid(self, Jxta_discovery_service_ref);

    id = jxta_advertisement_get_id(adv);
    if (id != NULL) {
        status = jxta_id_get_uniqueportion(id, &id_str);
        if (status != JXTA_SUCCESS) {
            /* something went wrong with getting the ID */
            JXTA_OBJECT_RELEASE(id);
            return status;
        }
        JXTA_LOG("Publishing doc id of :%s \n", jstring_get_string(id_str));

        JXTA_OBJECT_SHARE(adv);

        stat = jxta_cm_save(self->cm,
                            (char *) dirname[type], (char *) jstring_get_string(id_str), adv, lifetime, lifetimeForOthers);

        JXTA_OBJECT_RELEASE(id);
        JXTA_OBJECT_RELEASE(adv);
        JXTA_OBJECT_RELEASE(id_str);
        if (stat == JXTA_SUCCESS) {
            return JXTA_SUCCESS;
        }

    } else {
        /* Advertisement does not have an id, hash the document and use the
           hash as a key */
        JString *tmps = NULL;
        char *hashname = malloc(128);
        long hash = 0;
        jxta_advertisement_get_xml((Jxta_advertisement *) adv, &tmps);
        if (tmps == NULL) {
            /* abort nothing to publish */
            JXTA_LOG("Error NULL Advertisement String\n");
            free(hashname);
            return JXTA_FAILED;
        }

        hash = jxta_cm_hash(jstring_get_string(tmps), jstring_length(tmps));
        sprintf(hashname, "%ld", hash);
        stat = jxta_cm_save(self->cm,
                            dirname[type],
                            hashname, adv, (Jxta_expiration_time) lifetime, (Jxta_expiration_time) lifetimeForOthers);
        free(hashname);
        JXTA_OBJECT_RELEASE(tmps);
        if (stat == JXTA_SUCCESS) {
            return JXTA_SUCCESS;
        }
    }

    return JXTA_FAILED;
}

Jxta_status
remotePublish(Jxta_discovery_service * service,
              Jxta_id * peerid, Jxta_advertisement * adv, short type, Jxta_expiration_time expirationtime)
{

    Jxta_status status;
    JString *peerdoc = NULL;
    JString *rdoc = NULL;
    Jxta_DiscoveryResponse *response = NULL;
    ResolverResponse *res_response = NULL;
    Jxta_PA *padv = NULL;
    Jxta_vector *vec = jxta_vector_new(1);
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;
    Jxta_DiscoveryResponseElement *anElement = NULL;
    JString *tmpString;

    PTValid(discovery, Jxta_discovery_service_ref);

    JXTA_LOG("Remote publish starts\n");

    JXTA_OBJECT_CHECK_VALID(service);
    if (peerid) {
        JXTA_OBJECT_CHECK_VALID(peerid);
    }
    JXTA_OBJECT_CHECK_VALID(adv);
    jxta_PG_get_PA(discovery->group, &padv);
    status = jxta_PA_get_xml(padv, &peerdoc);

    if (status != JXTA_SUCCESS) {
        fprintf(stderr, "discovery::remotePublish failed to retrieve MyPeer Advertisement\n");
        return -1;
    }

    jxta_advertisement_get_xml((Jxta_advertisement *) adv, &tmpString);
    anElement = jxta_discovery_response_new_element_1(tmpString, expirationtime);

    jxta_vector_add_object_last(vec, (Jxta_object *) anElement);

    JXTA_OBJECT_RELEASE(anElement);
    JXTA_OBJECT_RELEASE(tmpString);

    response = jxta_discovery_response_new_1(type, NULL, NULL, 1, peerdoc, vec);

    status = jxta_discovery_response_get_xml(response, &rdoc);

    if (status == JXTA_SUCCESS) {
        res_response = jxta_resolver_response_new_1(discovery->instanceName, rdoc,
                                                    /* remote publish is always a qid of 0 */
                                                    0);
        JXTA_LOG("Sending remote publish request\n");
        status = jxta_resolver_service_sendResponse(discovery->resolver, res_response, peerid);
    } else {
        JXTA_LOG("discovery::remotePublish failed to construct response: %ld\n", status);
        return JXTA_FAILED;
    }
    if (status != JXTA_SUCCESS) {
        JXTA_LOG("discovery::remotePublish failed to send response: %ld\n", status);
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}

Jxta_status flush(Jxta_discovery_service * service, char *id, short type)
{

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;
    Jxta_status stat = JXTA_FAILED;
    PTValid(discovery, Jxta_discovery_service_ref);
    if (id != NULL) {
        stat = jxta_cm_remove_advertisement(discovery->cm, (char *) dirname[type], id);
    } else {
        int i = 0;
        char **keys = jxta_cm_get_primary_keys(discovery->cm, (char *) dirname[type]);
        while (keys[i]) {
            stat = jxta_cm_remove_advertisement(discovery->cm, (char *) dirname[type], keys[i]);
            free(keys[i]);
            i++;
        }
        free(keys);
    }
    return stat;
}

Jxta_status add_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
{
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) service;
    return (jxta_vector_add_object_last(discovery->listener_vec, (Jxta_object *) listener));
}


Jxta_status remove_listener(Jxta_discovery_service * service, Jxta_discovery_listener * listener)
{
    /* need an equals function first before this can be implemented */
    return JXTA_SUCCESS;
}



/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_discovery_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_discovery_service_methods -> Jxta_discovery_service_ref_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_discovery_service_methods Jxta_discovery_service_ref_methods;

Jxta_discovery_service_ref_methods jxta_discovery_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_discovery_service_ref_init,
      init_e,
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface},
    "Jxta_discovery_service_methods",
    getRemoteAdv,
    getLocalAdv,
    publish,
    remotePublish,
    flush,
    add_listener,
    remove_listener
};

void jxta_discovery_service_ref_construct(Jxta_discovery_service_ref * discovery, Jxta_discovery_service_ref_methods * methods)
{
    /*
     * we do not extend Jxta_discovery_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_discovery_service_methods);

    jxta_discovery_service_construct((Jxta_discovery_service *) discovery, (Jxta_discovery_service_methods *) methods);

    /* Set our rt type checking string */
    discovery->thisType = "Jxta_discovery_service_ref";
}

void jxta_discovery_service_ref_destruct(Jxta_discovery_service_ref * discovery)
{
    PTValid(discovery, Jxta_discovery_service_ref);

    /* release/free/destroy our own stuff */

    if (discovery->resolver != 0)
        JXTA_OBJECT_RELEASE(discovery->resolver);
    if (discovery->endpoint != 0)
        JXTA_OBJECT_RELEASE(discovery->endpoint);
    if (discovery->localPeerId != 0)
        JXTA_OBJECT_RELEASE(discovery->localPeerId);
    if (discovery->instanceName != 0)
        JXTA_OBJECT_RELEASE(discovery->instanceName);
    if (discovery->group != 0)
        JXTA_OBJECT_RELEASE(discovery->group);
    if (discovery->listener_vec != 0)
        JXTA_OBJECT_RELEASE(discovery->listener_vec);
    if (discovery->listeners != 0)
        JXTA_OBJECT_RELEASE(discovery->listeners);
    if (discovery->impl_adv != 0)
        JXTA_OBJECT_RELEASE(discovery->impl_adv);
    if (discovery->assigned_id != 0)
        JXTA_OBJECT_RELEASE(discovery->assigned_id);
    if (discovery->cm != 0)
        JXTA_OBJECT_RELEASE(discovery->cm);
    if (NULL != discovery->rdv)
        JXTA_OBJECT_RELEASE(discovery->rdv);
    if (discovery->pool != 0)
        apr_pool_destroy(discovery->pool);

    /* call the base classe's dtor. */
    jxta_discovery_service_destruct((Jxta_discovery_service *) discovery);

    JXTA_LOG("Destruction finished\n");
}

/**
 * Frees an instance of the Discovery Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Discovery Service to free.
 */
static void discovery_free(void *service)
{
    /* call the hierarchy of dtors */
    jxta_discovery_service_ref_destruct((Jxta_discovery_service_ref *) service);

    /* free the object itself */
    free(service);
}


/**
 * Creates a new instance of the Discovery Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_discovery_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Discovery Service.
 * @return a non initialized instance of the Discovery Service
 */

Jxta_discovery_service_ref *jxta_discovery_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *)
        malloc(sizeof(Jxta_discovery_service_ref));

    if (discovery == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(discovery, 0, sizeof(Jxta_discovery_service_ref));
    JXTA_OBJECT_INIT(discovery, (JXTA_OBJECT_FREE_FUNC) discovery_free, 0);

    /* call the hierarchy of ctors */
    jxta_discovery_service_ref_construct(discovery, &jxta_discovery_service_ref_methods);

    /* return the new object */
    JXTA_LOG("New Discovery\n");
    return discovery;
}


/* END OF STANDARD SERVICE IMPLEMENTATION CODE */

static void discovery_service_query_listener(Jxta_object * obj, void *arg)
{

    JString *peeradv = NULL;
    JString *query = NULL;
    Jxta_DiscoveryQuery *dq = NULL;
    JString *attr = NULL;
    JString *val = NULL;
    char **keys = NULL;
    Jxta_PA *padv = jxta_PA_new();
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    long qid = -1;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    ResolverQuery *rq = (ResolverQuery *) obj;
    Jxta_id *src_pid = NULL;

    PTValid(discovery, Jxta_discovery_service_ref);
    JXTA_LOG("Discovery received a query\n");
    qid = jxta_resolver_query_get_queryid(rq);
    jxta_resolver_query_get_query(rq, &query);
    if (query == NULL) {
        /*we're done, nothing to proccess */
    }
    dq = jxta_discovery_query_new();
    status = jxta_discovery_query_parse_charbuffer(dq, jstring_get_string(query), jstring_length(query));
    JXTA_OBJECT_RELEASE(query);

    if (status != JXTA_SUCCESS) {
        /* we're done go no further */
        JXTA_OBJECT_RELEASE(dq);
        JXTA_OBJECT_RELEASE(rq);
        return;
    }
    jxta_discovery_query_get_attr(dq, &attr);
    jxta_discovery_query_get_value(dq, &val);
    jxta_discovery_query_get_peeradv(dq, &peeradv);

    if (jstring_length(peeradv) > 0) {
        jxta_PA_parse_charbuffer(padv, jstring_get_string(peeradv), jstring_length(peeradv));
        JXTA_LOG("Publishing querying Peer Adv.\n");
        publish((Jxta_discovery_service *) discovery,
                (Jxta_advertisement *) padv, DISC_PEER, DEFAULT_EXPIRATION, DEFAULT_EXPIRATION);
        /* done with padv release it */
        JXTA_OBJECT_RELEASE(peeradv);
        JXTA_OBJECT_RELEASE(padv);

    }

    if (jstring_length(attr) == 0) {    /* unqualified query */
        keys = jxta_cm_get_primary_keys(discovery->cm, (char *) dirname[jxta_discovery_query_get_type(dq)]);
    } else {
        keys = jxta_cm_search(discovery->cm,
                              (char *) dirname[jxta_discovery_query_get_type(dq)],
                              (char *) jstring_get_string(attr),
                              (char *) jstring_get_string(val), jxta_discovery_query_get_threshold(dq));
    }

    JXTA_OBJECT_RELEASE(attr);
    JXTA_OBJECT_RELEASE(val);

    if ((keys == NULL) || (keys[0] == NULL)) {
        /* we're done go no further */
        JXTA_OBJECT_RELEASE(dq);
        JXTA_OBJECT_RELEASE(rq);
        free(keys);
        return;
    }

    /* construct response and send it back */
    /* since a peer adv could change at any given moment we always construct a new one
       and grab it from the group */
    {
        JString *peerdoc = NULL;
        Jxta_DiscoveryResponse *dr = NULL;
        ResolverResponse *res_response = NULL;
        JString *rdoc = NULL;
        int i = 0;
        int num_of_res = 0;
        Jxta_status status;

        responses = jxta_vector_new(1);
        jxta_PG_get_PA(discovery->group, &padv);

        while (keys[i]) {
            JString *res = NULL;
            Jxta_expiration_time expiration;
            Jxta_DiscoveryResponseElement *dre;
            status = jxta_cm_get_expiration_time(discovery->cm,
                                                 (char *) dirname[jxta_discovery_query_get_type(dq)], keys[i], &expiration);

            /*
             * Make sure that the entry did not expire
             */
            if ((status != JXTA_SUCCESS) || (expiration == 0)
                || (expiration < jpr_time_now())) {
                JXTA_LOG("Expired Document :%s :%ld\n", keys[i], expiration);
                ++i;
                continue;
            }

            status = jxta_cm_restore_bytes(discovery->cm, (char *) dirname[jxta_discovery_query_get_type(dq)], keys[i], &res);
            if (status != JXTA_SUCCESS) {
                ++i;
                continue;
            }

            /*
             * Make sure we are passing the delta remaining expiration
             * time not the absolute expiration time
             */
            dre = jxta_discovery_response_new_element_1(res, expiration - jpr_time_now());
            jxta_vector_add_object_last(responses, (Jxta_object *) dre);
            /* The vector counts his reference. Release ours */
            JXTA_OBJECT_RELEASE(dre);

            /* The element counts his reference. Release ours */
            JXTA_OBJECT_RELEASE(res);

            free(keys[i]);
            i++;
        }
        free(keys);

        status = jxta_PA_get_xml(padv, &peerdoc);
        JXTA_OBJECT_RELEASE(padv);
        if (status != JXTA_SUCCESS) {
            fprintf(stderr, "discovery::remotePublish failed to retrieve MyPeer Advertisement\n");
            JXTA_OBJECT_RELEASE(dq);
            JXTA_OBJECT_RELEASE(rq);

            return;
        }

        dr = jxta_discovery_response_new_1(jxta_discovery_query_get_type(dq),
                                           (char *) jstring_get_string(val),
                                           (char *) jstring_get_string(val), num_of_res, peerdoc, responses);

        JXTA_OBJECT_RELEASE(peerdoc);
        status = jxta_discovery_response_get_xml(dr, &rdoc);
        JXTA_OBJECT_RELEASE(dr);

        if (status == JXTA_SUCCESS) {
            res_response = jxta_resolver_response_new_1(discovery->instanceName, rdoc, jxta_resolver_query_get_queryid(rq));
            JXTA_OBJECT_RELEASE(rdoc);

            src_pid = jxta_resolver_query_get_src_peer_id(rq);

            status = jxta_resolver_service_sendResponse(discovery->resolver, res_response, src_pid);
            JXTA_OBJECT_RELEASE(res_response);

        } else {
            JXTA_LOG("discovery::remotePublish failed to construct response: %ld\n", status);
        }
    }
    JXTA_OBJECT_RELEASE(dq);
    JXTA_OBJECT_RELEASE(rq);
    if (src_pid)
        JXTA_OBJECT_RELEASE(src_pid);
}

static void process_response_listener(Jxta_discovery_service_ref * discovery, long qid, Jxta_DiscoveryResponse * res)
{

    char *buf = malloc(128);    /* More than enough */
    Jxta_status status;
    Jxta_listener *listener = NULL;
    int i;

    JXTA_LOG("Discovery: process_response_listener\n");

    sprintf(buf, "%ld", qid);
    status = jxta_hashtable_get(discovery->listeners, (const void *) buf, strlen(buf), (Jxta_object **) & listener);
    if (status == JXTA_SUCCESS && listener != NULL) {
        jxta_listener_schedule_object(listener, (Jxta_object *) res);
    }

    if (listener != NULL)
        JXTA_OBJECT_RELEASE(listener);

    /* call general discovery listeners */
    for (i = 0; i < jxta_vector_size(discovery->listener_vec); i++) {
        status = jxta_vector_get_object_at(discovery->listener_vec, (Jxta_object **) & listener, i);
        if (status == JXTA_SUCCESS && listener != NULL) {
            jxta_listener_schedule_object(listener, (Jxta_object *) res);
            JXTA_OBJECT_RELEASE(listener);
        }
    }
}

static void discovery_service_rdv_listener(Jxta_object * obj, void *arg)
{

    Jxta_status res = JXTA_SUCCESS;

    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    JXTA_LOG("Got a RDV new connect event push SRDI entries\n");

    /*
     * Update SRDI indexes for all advertisements in our cm 
     * to our new RDV
     */
    res = jxta_discovery_push_srdi(discovery, FALSE);
    if (res != JXTA_SUCCESS)
        JXTA_LOG("failed to push SRDI entries to new RDV\n");
}

static void discovery_service_response_listener(Jxta_object * obj, void *arg)
{

    JString *response = NULL;
    Jxta_DiscoveryResponse *dr = NULL;
    Jxta_vector *responses = NULL;
    Jxta_status status = JXTA_FAILED;
    long qid = -1;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    ResolverResponse *rr = (ResolverResponse *) obj;
    Jxta_advertisement *radv = NULL;

    PTValid(discovery, Jxta_discovery_service_ref);
    JXTA_LOG("Discovery received a response\n");

    qid = jxta_resolver_response_get_queryid(rr);
    jxta_resolver_response_get_response(rr, &response);
    if (response == NULL) {
        /* we're done, nothing to proccess */
        JXTA_OBJECT_RELEASE(rr);
        return;
    }
    dr = jxta_discovery_response_new();
    jxta_discovery_response_parse_charbuffer(dr, jstring_get_string(response), jstring_length(response));
    JXTA_LOG("Discovery received %d response(s)\n", jxta_discovery_response_get_count(dr));
    status = jxta_discovery_response_get_responses(dr, &responses);
    if (status == JXTA_SUCCESS) {
        jxta_discovery_response_get_responses(dr, &responses);
        {
            Jxta_vector *adv_vec = NULL;
            Jxta_DiscoveryResponseElement *res;
            int i = 0;
            for (i = 0; i < jxta_vector_size(responses); i++) {
                Jxta_object *tmpadv = NULL;
                res = NULL;
                jxta_vector_get_object_at(responses, (Jxta_object **) & res, i);
                if (res != NULL) {
                    radv = jxta_advertisement_new();
                    jxta_advertisement_parse_charbuffer(radv, jstring_get_string(res->response), jstring_length(res->response));
                    jxta_advertisement_get_advs(radv, &adv_vec);
                    jxta_vector_get_object_at(adv_vec, &tmpadv, 0);
                    if (tmpadv != NULL) {
                        publish((Jxta_discovery_service *) discovery, (Jxta_advertisement *) tmpadv,
                                /* XXX use more deterministic method than what the other end says */
                                jxta_discovery_response_get_type(dr),
                                (Jxta_expiration_time) res->expiration, (Jxta_expiration_time) res->expiration);
                        JXTA_OBJECT_RELEASE(tmpadv);
                    }
                    /* call listener(s) if any */
                    JXTA_OBJECT_RELEASE(res);
                    JXTA_OBJECT_RELEASE(radv);
                    JXTA_OBJECT_RELEASE(adv_vec);
                }
            }
            process_response_listener(discovery, qid, dr);
        }
    }
    /* clean up */
    JXTA_OBJECT_RELEASE(rr);
    JXTA_OBJECT_RELEASE(dr);
    return;
}

static void *APR_THREAD_FUNC advertisement_handling_thread(apr_thread_t * thread, void *arg)
{

    Jxta_status res = 0;
    Jxta_discovery_service_ref *discovery = (Jxta_discovery_service_ref *) arg;
    /* Mark the service as running now. */
    discovery->running = TRUE;

    /* Main loop */
    for (;;) {
        jpr_thread_delay(CM_ADV_RUN_CYCLE);
        jxta_cm_remove_expired_records(discovery->cm);

        /*
         * push delta if any
         */
        res = jxta_discovery_push_srdi(discovery, TRUE);
        if (res != JXTA_SUCCESS) {
            JXTA_LOG("failed to push SRDI delta CM entries\n");
        }
    }

    /*
     * Check if we have been awakened for the purpose of stopping
     */
    if (!discovery->running)
        apr_thread_exit(thread, JXTA_SUCCESS);

    return NULL;
}

Jxta_status jxta_discovery_push_srdi(Jxta_discovery_service_ref * discovery, Jxta_boolean delta)
{
    Jxta_vector *entries = NULL;
    JString *name;
    int i = 0;
    if (delta) {
        JXTA_LOG("push SRDI delta index if any \n");
        for (i = 0; i < 3; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = jxta_cm_get_srdi_delta_entries(discovery->cm, name);
            if (entries != NULL) {
                jxta_discovery_send_srdi(discovery, name, entries);
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }
            JXTA_OBJECT_RELEASE(name);
        }
    } else {
        JXTA_LOG("push all CM SRDI indexes \n");
        for (i = 0; i < 3; i++) {
            name = jstring_new_2((char *) dirname[i]);
            entries = jxta_cm_get_srdi_entries(discovery->cm, name);
            if (entries != NULL) {
                jxta_discovery_send_srdi(discovery, name, entries);
                JXTA_OBJECT_RELEASE(entries);
                entries = NULL;
            }
            JXTA_OBJECT_RELEASE(name);
        }
    }
    return JXTA_SUCCESS;
}

Jxta_status jxta_discovery_send_srdi(Jxta_discovery_service_ref * discovery, JString * pkey, Jxta_vector * entries)
{

    Jxta_SRDIMessage *msg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    JString *messageString = NULL;
    ResolverSrdi *srdi = NULL;

    msg = jxta_srdi_message_new_1(1, discovery->localPeerId, (char *) jstring_get_string(pkey), entries);
    if (msg == NULL) {
        JXTA_LOG("cannot allocate jxta_srdi_message_new\n");
        return JXTA_NOMEM;
    }

    res = jxta_srdi_message_get_xml(msg, &messageString);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("cannot serialize srdi message\n");
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_NOMEM;
    }
    JXTA_OBJECT_RELEASE(msg);

    srdi = jxta_resolver_srdi_new_1(discovery->instanceName, messageString, NULL);
    if (srdi == NULL) {
        JXTA_LOG("cannot allocate a resolver srdi message\n");
        JXTA_OBJECT_RELEASE(messageString);
        return JXTA_NOMEM;
    }
    JXTA_OBJECT_RELEASE(messageString);

    res = jxta_resolver_service_sendSrdi(discovery->resolver, srdi, NULL);
    if (res != JXTA_SUCCESS) {
        JXTA_LOG("cannot send srdi message\n");
    }

    JXTA_OBJECT_RELEASE(srdi);
    return res;
}

/* vim: set ts=4 sw=4 tw=130 et: */
