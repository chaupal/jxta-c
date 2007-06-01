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
 * $Id: jxta_endpoint_service.c,v 1.94 2005/09/15 18:41:04 slowhog Exp $
 */

static const char *__log_cat = "ENDPOINT";

#include <stdlib.h> /* malloc, free */

#include "jxtaapr.h"
#include <apr_hash.h>

#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "queue.h"
#include "jstring.h"
#include "jxta_hashtable.h"
#include "jxta_service_private.h"
#include "jxta_endpoint_service.h"
#include "jxta_endpoint_address.h"
#include "jxta_router_service.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_stdpg_private.h"
#include "jxta_routea.h"
#include "jxta_log.h"
#include "jxtaapr.h"
#include "jxta_endpoint_service_priv.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
const Jxta_time_diff JXTA_ENDPOINT_DEST_UNREACHABLE_TIMEOUT = ((Jxta_time_diff) 5L) * 60 * 1000;    /* 10 minutes */

/**
 ** 3/04/2002 MEMORY LEAK WARNING lomax@jxta.org
 **
 ** Because of the implementation of apr_hash, the key that is passed must remain
 ** valid memory (apr_hash_set does not copy the memory, just the pointer).
 ** However, there is no mean to free that memory (the assumption must be that the
 ** memory the key comes from is the same pool as the pool that has created the hashtable).
 ** Recomendation is to use Jxta_hashtable instead of apr_hash_t.
 **/
struct _jxta_endpoint_service {
    Extends(Jxta_service);

    apr_pool_t *pool;

    volatile Jxta_boolean running;
    apr_thread_t *thread;
    apr_thread_mutex_t *mutex;
    apr_thread_mutex_t *filter_list_mutex;
    apr_thread_mutex_t *listener_table_mutex;

    Jxta_PG *my_group;
    JString *my_groupid;
    Jxta_id *my_assigned_id;
    Jxta_advertisement *my_impl_adv;

    Jxta_RouteAdvertisement *myRoute;
    char *relay_addr;
    char *relay_proto;
    Jxta_transport *router_transport;
    Dlist *filter_list;
    apr_hash_t *listener_table;
    Jxta_hashtable *transport_table;
    Queue *outgoing_queue;
    Jxta_hashtable *unreachable_addresses;
};

static Jxta_listener *jxta_endpoint_service_lookup_listener(Jxta_endpoint_service * service, Jxta_endpoint_address * addr);
static void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * t, void *arg);

static void outgoing_message_process(Jxta_endpoint_service * service, Jxta_message * msg);

/*
 * Implementations for module methods
 */
static Jxta_status endpoint_init(Jxta_module * it, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_id *gid;

    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Initializing ...\n");

    /* store our assigned id */
    if (assigned_id != NULL) {
        JXTA_OBJECT_SHARE(assigned_id);
        self->my_assigned_id = assigned_id;
    }

    /* advs and groups are jxta_objects that we share */
    if (impl_adv != NULL) {
        JXTA_OBJECT_SHARE(impl_adv);
    }

    self->my_impl_adv = impl_adv;
    self->my_group = group;
    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &(self->my_groupid));
    JXTA_OBJECT_RELEASE(gid);
    self->relay_addr = NULL;
    self->relay_proto = NULL;
    self->myRoute = NULL;

    self->running = TRUE;
    apr_thread_create(&self->thread, NULL, outgoing_message_thread, (void *) self, self->pool);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");
    return JXTA_SUCCESS;
}

static Jxta_status endpoint_start(Jxta_module * self, const char *args[])
{
    /* construct + init have done everything already */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Starting ...\n");

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started\n");
    return JXTA_SUCCESS;
}

static void endpoint_stop(Jxta_module * self)
{
    Jxta_endpoint_service *me = PTValid(self, Jxta_endpoint_service);
    apr_status_t status;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Stopping outgoing message thread ...\n");
    /* stop the thrad that processes outgoing messages */
    me->running = FALSE;
    queue_enqueue(me->outgoing_queue, NULL);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Waiting outgoing message thread shutdown completely ...\n");
    apr_thread_join(&status, me->thread);

    if (NULL != me->router_transport) {
        JXTA_OBJECT_RELEASE(me->router_transport);
        me->router_transport = NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}


/*
 * implementations for service methods
 */
static void endpoint_get_MIA(Jxta_service * it, Jxta_advertisement ** mia)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    *mia = JXTA_OBJECT_SHARE(self->my_impl_adv);
}

static void endpoint_get_interface(Jxta_service * it, Jxta_service ** serv)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    JXTA_OBJECT_SHARE(self);
    *serv = (Jxta_service *) self;
}

/*
 * Endpoint service is not virtual yet : there is no way to provide
 * more than one implementation. Therefore its methods table is just
 * the same than Jxta_service's.
 */
typedef Jxta_service_methods Jxta_endpoint_service_methods;

static const Jxta_endpoint_service_methods jxta_endpoint_service_methods = {
    {
     "Jxta_module_methods",
     endpoint_init,
     jxta_module_init_e_impl,
     endpoint_start,
     endpoint_stop},
    "Jxta_service_methods",
    endpoint_get_MIA,
    endpoint_get_interface
};


typedef struct _Filter Filter;

struct _Filter {
    char *str;
    JxtaEndpointFilter func;
    void *arg;
};

void jxta_endpoint_service_destruct(Jxta_endpoint_service * service)
{
    PTValid(service, Jxta_endpoint_service);

    /* delete tables and stuff */
    JXTA_OBJECT_RELEASE(service->unreachable_addresses);
    JXTA_OBJECT_RELEASE(service->transport_table);

    if (service->relay_addr)
        free(service->relay_addr);
    if (service->relay_proto)
        free(service->relay_proto);
    dl_free(service->filter_list, free);
    queue_free(service->outgoing_queue);

    apr_thread_mutex_destroy(service->filter_list_mutex);
    apr_thread_mutex_destroy(service->listener_table_mutex);
    apr_thread_mutex_destroy(service->mutex);
    apr_pool_destroy(service->pool);

    /* un-ref our impl_adv, and delete assigned_id */
    service->my_group = NULL;

    if (service->my_groupid) {
        JXTA_OBJECT_RELEASE(service->my_groupid);
        service->my_groupid = NULL;
    }
    if (service->my_impl_adv) {
        JXTA_OBJECT_RELEASE(service->my_impl_adv);
        service->my_impl_adv = NULL;
    }
    if (service->my_assigned_id != 0) {
        JXTA_OBJECT_RELEASE(service->my_assigned_id);
        service->my_assigned_id = NULL;
    }

    if (service->myRoute != NULL) {
        JXTA_OBJECT_RELEASE(service->myRoute);
        service->myRoute = NULL;
    }

    service->thisType = NULL;

    jxta_service_destruct((Jxta_service *) service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}

Jxta_endpoint_service *jxta_endpoint_service_construct(Jxta_endpoint_service * service,
                                                       const Jxta_endpoint_service_methods * methods)
{
    apr_status_t status;

    PTValid(methods, Jxta_service_methods);

    status = apr_pool_create(&service->pool, NULL);
    if (status != APR_SUCCESS)
        return NULL;

    jxta_service_construct((Jxta_service *) service, (const Jxta_service_methods *) methods);
    service->thisType = "Jxta_endpoint_service";

    service->filter_list = dl_make();
    service->listener_table = apr_hash_make(service->pool);
    service->transport_table = jxta_hashtable_new(2);
    service->unreachable_addresses = jxta_hashtable_new(5);

    apr_thread_mutex_create(&service->mutex, APR_THREAD_MUTEX_NESTED, service->pool);
    apr_thread_mutex_create(&service->filter_list_mutex, APR_THREAD_MUTEX_NESTED, service->pool);
    apr_thread_mutex_create(&service->listener_table_mutex, APR_THREAD_MUTEX_NESTED, service->pool);

    service->outgoing_queue = queue_new(service->pool);

    service->my_assigned_id = NULL;
    service->my_impl_adv = NULL;
    service->my_group = NULL;
    service->my_groupid = NULL;
    service->router_transport = NULL;

    return service;
}

static void jxta_endpoint_service_free(Jxta_object * obj)
{
    jxta_endpoint_service_destruct((Jxta_endpoint_service *) obj);

    memset((void *) obj, 0xdd, sizeof(Jxta_endpoint_service));
    free((void *) obj);
}

Jxta_endpoint_service *jxta_endpoint_service_new_instance(void)
{
    Jxta_endpoint_service *service = (Jxta_endpoint_service *) calloc(1, sizeof(Jxta_endpoint_service));

    JXTA_OBJECT_INIT(service, jxta_endpoint_service_free, 0);

    if (NULL == jxta_endpoint_service_construct(service, &jxta_endpoint_service_methods)) {
        free(service);
        service = NULL;
    }

    return service;
}


JXTA_DECLARE(void) jxta_endpoint_service_demux(Jxta_endpoint_service * service, Jxta_message * msg)
{
    Dlist *cur;
    Filter *cur_filter;
    Jxta_endpoint_address *dest;
    Jxta_vector *vector = NULL;
    char *destStr;
    Jxta_listener *listener;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Endpoint Demux of message [%p]\n", msg);

  /**
   * FIXME 4/11/2002 lomax@jxta.org
   *
   * The following code needs to be revisited:
   *    - a same message can go several times through demux. Filters must
   *      be invoked only once.
   **/

    /* pass message through appropriate filters */
    apr_thread_mutex_lock(service->filter_list_mutex);

    dl_traverse(cur, service->filter_list) {
        Jxta_message_element *el = NULL;
        cur_filter = cur->val;

        if ((vector = jxta_message_get_elements_of_namespace(msg, cur_filter->str)) != NULL
            || (JXTA_SUCCESS == jxta_message_get_element_1(msg, cur_filter->str, &el))) {

            JXTA_OBJECT_RELEASE(el);
            /* discard the message if the filter returned false */
            if (!cur_filter->func(msg, cur_filter->arg)) {
                apr_thread_mutex_unlock(service->filter_list_mutex);
                return;
            }

        }
        if (vector != NULL) {
            JXTA_OBJECT_RELEASE(vector);
        }
    }

    apr_thread_mutex_unlock(service->filter_list_mutex);

    dest = jxta_message_get_destination(msg);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Destination of message [%s] is NULL\n", msg);
        return;
    }

    JXTA_OBJECT_CHECK_VALID(dest);

    destStr = jxta_endpoint_address_to_string(dest);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Looking up listener for %s\n", destStr);

    listener = jxta_endpoint_service_lookup_listener(service, dest);

    if (listener != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: Calling listener for %s\n", destStr);
        jxta_listener_schedule_object(listener, (Jxta_object *) msg);

        JXTA_OBJECT_CHECK_VALID(msg);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: No demux listener for %s\n", destStr);
    }

    JXTA_OBJECT_RELEASE(dest);
    free(destStr);
}

JXTA_DECLARE(void) jxta_endpoint_service_get_route_from_PA(Jxta_PA * padv, Jxta_RouteAdvertisement ** route)
{
    Jxta_vector *svcs;
    Jxta_svc *svc = NULL;
    unsigned int sz;
    unsigned int i;

    svcs = jxta_PA_get_Svc(padv);
    sz = jxta_vector_size(svcs);
    for (i = 0; i < sz; i++) {
        Jxta_id *mcid;
        Jxta_svc *tmpsvc = NULL;
        jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
        mcid = jxta_svc_get_MCID(tmpsvc);
        if (jxta_id_equals(mcid, jxta_endpoint_classid_get())) {
            svc = tmpsvc;
            JXTA_OBJECT_RELEASE(mcid);
            break;
        }
        JXTA_OBJECT_RELEASE(mcid);
        JXTA_OBJECT_RELEASE(tmpsvc);
    }

    if (svc != NULL) {
        *route = jxta_svc_get_RouteAdvertisement(svc);
        JXTA_OBJECT_RELEASE(svc);
    }

    JXTA_OBJECT_RELEASE(svcs);
}

JXTA_DECLARE(void) jxta_endpoint_service_add_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    Jxta_PA *pa;
    Jxta_vector *svcs;
    Jxta_RouteAdvertisement *route;
    Jxta_AccessPointAdvertisement *dest;
    Jxta_endpoint_address *address;
    Jxta_vector *addresses;
    Jxta_id *pid = NULL;
    JString *jaddress;
    char *caddress;

    Jxta_svc *svc = NULL;
    unsigned int sz;
    unsigned int i;
    JString *name;

    PTValid(service, Jxta_endpoint_service);

    name = jxta_transport_name_get(transport);

    /*
     * Only add the new transport endpoint address if it is not the HTTP
     * transport as we only support HTTP relay config. There are no values to
     * publish an HTTP relay address that cannot be addressed. This
     * is currently creating a problem in the router where the HTTP
     * address is beleived to be addressable.
     *
     * FIXME 20040908 tra  Will need to revisit when we support 
     * relay capability
     */

    if (strncmp(jstring_get_string(name), "http", 4) != 0) {

        /* Add the public address to the endpoint section of the PA */
        jxta_PG_get_PA(service->my_group, &pa);
        svcs = jxta_PA_get_Svc(pa);
        sz = jxta_vector_size(svcs);
        for (i = 0; i < sz; i++) {
            Jxta_id *mcid;
            Jxta_svc *tmpsvc = NULL;
            jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
            mcid = jxta_svc_get_MCID(tmpsvc);
            if (jxta_id_equals(mcid, service->my_assigned_id)) {
                svc = tmpsvc;
                JXTA_OBJECT_RELEASE(mcid);
                break;
            }
            JXTA_OBJECT_RELEASE(mcid);
            JXTA_OBJECT_RELEASE(tmpsvc);
        }

        if (svc == NULL) {
            svc = jxta_svc_new();
            jxta_svc_set_MCID(svc, service->my_assigned_id);
            jxta_vector_add_object_last(svcs, (Jxta_object *) svc);
        }

        route = jxta_svc_get_RouteAdvertisement(svc);
        if (route == NULL) {
            route = jxta_RouteAdvertisement_new();
            jxta_svc_set_RouteAdvertisement(svc, route);
        }
        address = jxta_transport_publicaddr_get(transport);
        caddress = jxta_endpoint_address_to_string(address);
        jaddress = jstring_new_2(caddress);

        dest = jxta_RouteAdvertisement_get_Dest(route);
        if (dest == NULL) {
            dest = jxta_AccessPointAdvertisement_new();
            jxta_PG_get_PID(service->my_group, &pid);
            jxta_AccessPointAdvertisement_set_PID(dest, pid);
            jxta_RouteAdvertisement_set_Dest(route, dest);
            JXTA_OBJECT_RELEASE(pid);
        }

        addresses = jxta_AccessPointAdvertisement_get_EndpointAddresses(dest);
        if (addresses == NULL) {
            addresses = jxta_vector_new(2);
            jxta_AccessPointAdvertisement_set_EndpointAddresses(dest, addresses);
        }

        jxta_vector_add_object_last(addresses, (Jxta_object *) jaddress);

        if (service->myRoute != NULL)
            JXTA_OBJECT_RELEASE(service->myRoute);

        service->myRoute = route;

        free(caddress);

        JXTA_OBJECT_RELEASE(addresses);
        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(jaddress);
        JXTA_OBJECT_RELEASE(address);
        JXTA_OBJECT_RELEASE(svc);
        JXTA_OBJECT_RELEASE(svcs);
        JXTA_OBJECT_RELEASE(pa);
    }

    /* add the transport to our table */
    /* FIXME: should check allow_overload etc. */
    jxta_hashtable_put(service->transport_table, jstring_get_string(name),  /* string gets copied */
                       jstring_length(name), (Jxta_object *) transport);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_endpoint_service_get_local_route(Jxta_endpoint_service * service)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_SHARE(service->myRoute);
    return service->myRoute;
}

JXTA_DECLARE(void) jxta_endpoint_service_set_local_route(Jxta_endpoint_service * service, Jxta_RouteAdvertisement * route)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_RELEASE(service->myRoute);

    service->myRoute = route;
    JXTA_OBJECT_SHARE(service->myRoute);
}

JXTA_DECLARE(void) jxta_endpoint_service_remove_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
{
    JString *name = jxta_transport_name_get(transport);
    Jxta_object *rmd = NULL;

    PTValid(service, Jxta_endpoint_service);

    jxta_hashtable_del(service->transport_table, jstring_get_string(name), jstring_length(name), &rmd);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);

    /*
     * FIXME 20040516 tra
     * We should update our route info here removing
     * the corresponding endpoint address
     */

    /* If trsp existed, release it */
    if (rmd != NULL)
        JXTA_OBJECT_RELEASE(rmd);
}

/* FIXME: transport_name should be a JString for consistency */
JXTA_DECLARE(Jxta_transport *) jxta_endpoint_service_lookup_transport(Jxta_endpoint_service * service, const char *transport_name)
{
    Jxta_transport *t = NULL;

    PTValid(service, Jxta_endpoint_service);

    jxta_hashtable_get(service->transport_table, transport_name, strlen(transport_name), (Jxta_object **) & t);

    /*
     * This routine is allowed to return NULL if the requested transport
     * was not found. (Until we convert this code the convention of always
     * returning a status.
     */
    if (t != NULL) {
        JXTA_OBJECT_CHECK_VALID(t);
    }

    return t;
}

JXTA_DECLARE(void) jxta_endpoint_service_add_filter(Jxta_endpoint_service * service, char const *str, JxtaEndpointFilter f,
                                                    void *arg)
{
    Filter *filter = (Filter *) malloc(sizeof(Filter));

    PTValid(service, Jxta_endpoint_service);

    filter->func = f;
    filter->arg = arg;

    /* insert the filter into the list */
    apr_thread_mutex_lock(service->filter_list_mutex);
    dl_insert_b(service->filter_list, filter);
    apr_thread_mutex_unlock(service->filter_list_mutex);
}


JXTA_DECLARE(void) jxta_endpoint_service_remove_filter(Jxta_endpoint_service * service, JxtaEndpointFilter filter)
{
    Dlist *cur;

    PTValid(service, Jxta_endpoint_service);

    apr_thread_mutex_lock(service->filter_list_mutex);

    /* find the filter registered under this handle */

    dl_traverse(cur, service->filter_list) {

        if (((Filter *) cur->val)->func == filter)
            break;

    }

    /* remove this filter from the list */
    if (cur != NULL) {
        free(cur->val);
        dl_delete_node(cur);
    }

    apr_thread_mutex_unlock(service->filter_list_mutex);
}

static Jxta_boolean is_listener_for(Jxta_endpoint_service * service, char *str)
{
    Jxta_boolean res = FALSE;
    Jxta_listener *listener = NULL;

    PTValid(service, Jxta_endpoint_service);

    apr_thread_mutex_lock(service->listener_table_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);
    if (listener != NULL) {
        res = TRUE;

    }
    apr_thread_mutex_unlock(service->listener_table_mutex);

    return res;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_add_listener(Jxta_endpoint_service * service,
                                   char const *serviceName, char const *serviceParam, Jxta_listener * listener)
{
    int str_length = (serviceName != NULL ? strlen(serviceName) : 0) +(serviceParam != NULL ? strlen(serviceParam) : 0) + 1;
    char *str = apr_pcalloc(service->pool, str_length);

    PTValid(service, Jxta_endpoint_service);

    apr_snprintf(str, str_length, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Register listener for %s\n", str);

    apr_thread_mutex_lock(service->listener_table_mutex);

    if (is_listener_for(service, str)) {
        apr_thread_mutex_unlock(service->listener_table_mutex);
        return JXTA_BUSY;
    }

    JXTA_OBJECT_SHARE(listener);
    apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, (void *) listener);

    apr_thread_mutex_unlock(service->listener_table_mutex);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_remove_listener(Jxta_endpoint_service * service, char const *serviceName, char const *serviceParam)
{
    Jxta_listener *listener;
    char str[256];

    PTValid(service, Jxta_endpoint_service);

    apr_snprintf(str, sizeof(str), "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    apr_thread_mutex_lock(service->listener_table_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);
    if (listener != NULL) {
        apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, NULL);
        JXTA_OBJECT_RELEASE(listener);
    }
    apr_thread_mutex_unlock(service->listener_table_mutex);
    return JXTA_SUCCESS;
}

static Jxta_listener *jxta_endpoint_service_lookup_listener(Jxta_endpoint_service * service, Jxta_endpoint_address * addr)
{
    Jxta_listener *listener = NULL;
    int i;
    int index;
    int pt = 0;
    int j;
    int str_length;
    char *str;
    char *str1;
    const char *ea_svc_name;
    const char *ea_svc_params;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(addr);

    ea_svc_name = jxta_endpoint_address_get_service_name(addr);
    ea_svc_params = jxta_endpoint_address_get_service_params(addr);

    str_length = strlen(ea_svc_name) + strlen(ea_svc_params) + 1;
    str = malloc(str_length);

    apr_snprintf(str, str_length, "%s%s", ea_svc_name, ea_svc_params);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Lookup for listener %s-%s\n", ea_svc_name, ea_svc_params);

    apr_thread_mutex_lock(service->listener_table_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);

    /*
     * Note: 20040510 tra
     * If we did not find a listener, we should check if the message
     * if from a cross-group endpoint messenger. Cross-group messengers
     * are registered only with the service param of the address.
     * The service name is just the name of the Endpoint cross-messenger
     * service:
     *    EndpointService:uuid-groupId/"real address"
     * We need to remove the begining of the address
     */
    if (listener == NULL) {

        str1 = malloc(strlen(ea_svc_params) + 1);

        index = strlen(ea_svc_name);

        /* 
         * copy the address from the param position, skipping the service name
         */
        j = 0;
        pt = 0;
        for (i = 0; i < strlen(ea_svc_params); i++) {
            *(str1 + j) = *(str + index);

            /*
             * remove the extra '/'
             */
            if (*(str1 + j) == '/') {
                index++;
                pt = i; /* save location of end of first parameter */
            } else {
                index++;
                j++;
            }
        }
        *(str1 + j) = '\0';

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for subgroup endpoint %s\n", str1);
        listener = (Jxta_listener *) apr_hash_get(service->listener_table, str1, APR_HASH_KEY_STRING);

        /*
         * FIXME: 20040706 tra
         * The relay transport is using an endpoint address that
         * contain the relay server peerId. For the time being
         * just use the service id for demuxing to the relay as the
         * relay just registered with this address.
         * we are dropping the service parameter. This used to be used
         * when multiple relay connections were supported. This should
         * have been removed.
         */
        if (listener == NULL && pt != 0) {
            strncpy(str1, ea_svc_params, pt - 1);
            *(str1 + pt) = '\0';
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "looking for relay endpoint %s\n", str1);
            listener = (Jxta_listener *) apr_hash_get(service->listener_table, str1, APR_HASH_KEY_STRING);
        }

        free(str1);
    }

    apr_thread_mutex_unlock(service->listener_table_mutex);
    free(str);
    return listener;
}

JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_propagate(Jxta_endpoint_service * service,
                                Jxta_message * msg, const char *service_name, const char *service_param)
{
    Jxta_transport *transport = NULL;
    JString *svc = NULL;
    JString *param = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service_name == NULL && service_param == NULL)
        return JXTA_INVALID_ARGUMENT;

    /*
     * lookup for TCP/IP transport as the only transport that
     * support propagation at this stage
     */

    transport = jxta_endpoint_service_lookup_transport(service, "tcp");
    if (transport) {    /* ok we can try to propagate, so the transport propagation
                           may still be disabled MulticastOff */

        /*
         * Correctly set the endpoint address for cross-group
         * demuxing via the NetPeerGroup endpoint service
         */
        svc = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
        jstring_append_2(svc, ":");
        jstring_append_1(svc, service->my_groupid);

        param = jstring_new_2(service_name);
        jstring_append_2(param, "/");
        jstring_append_2(param, service_param);

        jxta_transport_propagate(transport, msg, jstring_get_string(svc), jstring_get_string(param));
        JXTA_OBJECT_RELEASE(transport);
        JXTA_OBJECT_RELEASE(param);
        JXTA_OBJECT_RELEASE(svc);
    } else {
        return JXTA_FAILED;
    }

    return JXTA_SUCCESS;
}


JXTA_DECLARE(Jxta_status)
    jxta_endpoint_service_send(Jxta_PG * obj, Jxta_endpoint_service * service, Jxta_message * msg, Jxta_endpoint_address * dest_addr)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_stdpg *pg = PTValid(obj, Jxta_stdpg);
    Jxta_bytevector *b_exp = NULL;
    char *baseAddrStr = jxta_endpoint_address_get_transport_addr(dest_addr);

    PTValid(service, Jxta_endpoint_service);

    JXTA_OBJECT_CHECK_VALID(msg);

    /*
     * check if this destination is reachable. We are maintaining
     * a negative cache of unreachable destination
     */
    apr_thread_mutex_lock(service->mutex);

    res = jxta_hashtable_get(service->unreachable_addresses, baseAddrStr, strlen(baseAddrStr), (Jxta_object **) & b_exp);

    if ((JXTA_SUCCESS == res) && (b_exp != NULL)) {
        Jxta_time exp = 0;
        Jxta_time now = 0;

        jxta_bytevector_get_bytes_at(b_exp, &exp, 0, sizeof(Jxta_time));
        JXTA_OBJECT_RELEASE(b_exp);

        now = jpr_time_now();

        if (exp > now) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destination [%s] unreachable for " JPR_DIFF_TIME_FMT "ms\n",
                            baseAddrStr, ((Jxta_time_diff) (exp - now)));
            apr_thread_mutex_unlock(service->mutex);
            res = JXTA_UNREACHABLE_DEST;
            goto Common_Exit;
        } else {
            /* 
             * negative info expired remove the negative cache entry
             */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destination [%s] may now be reachable!\n", baseAddrStr);
            jxta_hashtable_del(service->unreachable_addresses, baseAddrStr, strlen(baseAddrStr), NULL);
        }
    }

    apr_thread_mutex_unlock(service->mutex);

    msg = jxta_message_clone(msg);

    /*
     * Note: 20040510 tra
     * Handle the cross-group messenger addressing where all
     * sub-group messages are demuxed within the parent
     * netpeergroup. This is a specificity of the J2SE
     * implementation. This address rewrites require to
     * add the prefix:
     *  EndpointService:<group-uniqueID>
     * to the address
     *
     * This only need to be done for subgroups, and only if
     * the service did not set the endpoint address yet.
     */
    if (pg->home_group != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PeerGroup cross-messenger need to rewrite dest address\n");

        /*
         * Check if we already have the endpoint service address destination
         * in that case assumed that the right endpoint address was
         * set
         *
         * FIXME 20040510 tra we really should have a better way to 
         * standardize address processing through subgroups
         */
        if (strncmp(jxta_endpoint_address_get_service_name(dest_addr),
                    JXTA_ENDPOINT_SERVICE_NAME, strlen(JXTA_ENDPOINT_SERVICE_NAME)) != 0) {
            char *caddress = NULL;
            JString *cross_service = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
            Jxta_endpoint_address *new_addr;
            Jxta_id *id;
            JString *sid = NULL;

            jstring_append_2(cross_service, ":");

            /*
             * Extract the group id of the messenger destination. This
             * is different from the endpoint group id, as the endpoint
             * service always run within the NetGroup.
             */
            jxta_PG_get_GID((Jxta_PG *) pg, &id);
            jxta_id_get_uniqueportion(id, &sid);
            JXTA_OBJECT_RELEASE(id);

            jstring_append_2(cross_service, jstring_get_string(sid));

            jstring_append_2(cross_service, "/");
            jstring_append_2(cross_service, jxta_endpoint_address_get_service_name(dest_addr));

            new_addr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(dest_addr),
                                                  jxta_endpoint_address_get_protocol_address(dest_addr),
                                                  jstring_get_string(cross_service),
                                                  jxta_endpoint_address_get_service_params(dest_addr));
            JXTA_OBJECT_RELEASE(cross_service);
            JXTA_OBJECT_RELEASE(sid);

            /*
             * set our new address
             */
            caddress = jxta_endpoint_address_to_string(new_addr);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "New Address=%s\n", caddress);
            free(caddress);

            jxta_message_set_destination(msg, new_addr);

            queue_enqueue(service->outgoing_queue, (void *) msg);

            JXTA_OBJECT_RELEASE(new_addr);

            return JXTA_SUCCESS;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "No address rewrite as endpoint service is already set\n");
        }
    }

    jxta_message_set_destination(msg, dest_addr);

    queue_enqueue(service->outgoing_queue, (void *) msg);

    res = JXTA_SUCCESS;

  Common_Exit:
    free(baseAddrStr);

    return res;
}

JXTA_DECLARE(void) jxta_endpoint_service_set_relay(Jxta_endpoint_service * service, const char *proto, const char *host)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        free(service->relay_addr);
    }

    if (NULL != service->relay_proto) {
        free(service->relay_proto);
    }

    if (host != NULL) {
        service->relay_addr = strdup(host);
        service->relay_proto = strdup(proto);
    } else {
        service->relay_addr = NULL;
        service->relay_proto = NULL;
    }
}

JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_addr(Jxta_endpoint_service * service)
{
    char *addr = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        addr = strdup(service->relay_addr);
    }

    return addr;
}


JXTA_DECLARE(char *) jxta_endpoint_service_get_relay_proto(Jxta_endpoint_service * service)
{
    char *proto = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_proto != NULL) {
        proto = strdup(service->relay_proto);
    }

    return proto;
}

static
void *APR_THREAD_FUNC outgoing_message_thread(apr_thread_t * thread, void *arg)
{
    Jxta_endpoint_service *service = PTValid(arg, Jxta_endpoint_service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Endpoint outgoing message thread started.\n");

    while (service->running) {
        Jxta_message *msg = (Jxta_message *) queue_dequeue(service->outgoing_queue, 10L * 1000L * 1000L);
        if (msg) {
            JXTA_OBJECT_CHECK_VALID(msg);

            outgoing_message_process(service, msg);
            JXTA_OBJECT_RELEASE(msg);
        }
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Endpoint outgoing message thread stopping.\n");

    apr_thread_exit(thread, JXTA_SUCCESS);

    /* NOTREACHED */
    return NULL;
}


static
void outgoing_message_process(Jxta_endpoint_service * self, Jxta_message * msg)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_endpoint_address *dest;
    JxtaEndpointMessenger *messenger;

    JXTA_OBJECT_CHECK_VALID(msg);

    dest = jxta_message_get_destination(msg);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No destination. Message [%p] is discarded.\n", msg);
        return;
    }

    if (self->router_transport == NULL) {
        self->router_transport = jxta_endpoint_service_lookup_transport(self, "jxta");
    }

    if (self->router_transport == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Router transport not available\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Outgoing message [%p] -> %s://%s/%s/%s\n",
                    msg,
                    jxta_endpoint_address_get_protocol_name(dest),
                    jxta_endpoint_address_get_protocol_address(dest),
                    jxta_endpoint_address_get_service_name(dest), jxta_endpoint_address_get_service_params(dest));


    /*
     * We have the router transport go ahead and demux to it for sending our message
     */
    messenger = jxta_transport_messenger_get(self->router_transport, dest);
    if (messenger != NULL) {
        JXTA_OBJECT_CHECK_VALID(messenger);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending message [%p] via router transport.\n", msg);

        res = messenger->jxta_send(messenger, msg);
        JXTA_OBJECT_RELEASE(messenger);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not get router messenger\n");
        res = JXTA_UNREACHABLE_DEST;
    }

    if (JXTA_SUCCESS != res) {
        Jxta_bytevector *b_exp = jxta_bytevector_new_1(sizeof(Jxta_time));
        Jxta_time until = (Jxta_time) jpr_time_now() + JXTA_ENDPOINT_DEST_UNREACHABLE_TIMEOUT;
        Jxta_endpoint_address *failed = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(dest),
                                                                   jxta_endpoint_address_get_protocol_address(dest),
                                                                   NULL,
                                                                   NULL);
        char *failedStr = jxta_endpoint_address_to_string(failed);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "Failed to send message [%p]. Marking destination %s://%s unreachable until " JPR_ABS_TIME_FMT ".\n", msg,
                        jxta_endpoint_address_get_protocol_name(failed),
                        jxta_endpoint_address_get_protocol_address(failed), until);

        jxta_bytevector_add_bytes_at(b_exp, &until, 0, sizeof(Jxta_time));

        apr_thread_mutex_lock(self->mutex);

        jxta_hashtable_put(self->unreachable_addresses, failedStr, strlen(failedStr), (Jxta_object *) b_exp);
        free(failedStr);
        JXTA_OBJECT_RELEASE(b_exp);

        apr_thread_mutex_unlock(self->mutex);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Message [%p] send successful.\n", msg);
    }

    JXTA_OBJECT_RELEASE(dest);
}

void jxta_endpoint_service_transport_event(Jxta_endpoint_service *me, Jxta_transport_event e, void *data)
{
    const char *addr = NULL;

    switch (e) {
        case JXTA_TRANSPORT_INBOUND_CONNECT:
            addr = (const char *) data;
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Inbound connection from %s.\n", addr);
            jxta_hashtable_del(me->unreachable_addresses, addr, strlen(addr), NULL);
            break;
        default:
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Ignore unknow transport event type %d.\n", e);
            break;
    }
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vim: set ts=4 sw=4 tw=130 et: */
