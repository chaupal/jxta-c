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
 * $Id: jxta_endpoint_service.c,v 1.68 2005/03/14 07:46:25 slowhog Exp $
 */

#include <stdlib.h>     /* malloc, free */

#include <apr.h>
#include <apr_hash.h>
#include <apr_thread_mutex.h>
#include <apr_thread_proc.h>

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

static const char *__log_cat = "ENDPOINT";

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
    Dlist *filter_list;
    apr_hash_t *listener_table;
    Jxta_hashtable *transport_table;
    Queue *outgoing_queue;
    int is_running;
    apr_thread_t *thread;
    apr_thread_mutex_t *filter_list_mutex;
    apr_thread_mutex_t *listener_table_mutex;
    apr_pool_t *pool;
    Jxta_advertisement *my_impl_adv;
    Jxta_PG *my_group;
    JString *my_groupid;
    Jxta_id *my_assigned_id;
    Jxta_transport *transport;
    char *relay_addr;
    char *relay_proto;
    Jxta_hashtable *unreachable_addresses;
    Jxta_RouteAdvertisement *myRoute;
};

/*
 * Implementations for module methods
 */
static Jxta_status endpoint_init(Jxta_module * it, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{

    Jxta_id *gid;

    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);
    /* store our assigned id */
    if (assigned_id != 0) {
        JXTA_OBJECT_SHARE(assigned_id);
        self->my_assigned_id = assigned_id;
    }

    /* advs and groups are jxta_objects that we share */
    if (impl_adv != 0)
        JXTA_OBJECT_SHARE(impl_adv);
    if (group != 0)
        JXTA_OBJECT_SHARE(group);
    self->my_impl_adv = impl_adv;
    self->my_group = group;
    jxta_PG_get_GID(group, &gid);
    jxta_id_get_uniqueportion(gid, &(self->my_groupid));
    JXTA_OBJECT_RELEASE(gid);
    self->relay_addr = NULL;
    self->relay_proto = NULL;
    self->myRoute = NULL;
    return JXTA_SUCCESS;
}

static Jxta_status endpoint_start(Jxta_module * self, char *args[])
{
    /* construct + init have done everything already */
    return JXTA_SUCCESS;
}

static void endpoint_stop(Jxta_module * self)
{
    PTValid(self, Jxta_endpoint_service);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Not implemented, don't know how to stop yet.\n");
}


/*
 * implementations for service methods
 */
static void endpoint_get_MIA(Jxta_service * it, Jxta_advertisement ** mia)
{
    Jxta_endpoint_service *self = PTValid(it, Jxta_endpoint_service);

    JXTA_OBJECT_SHARE(self->my_impl_adv);
    *mia = self->my_impl_adv;
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

Jxta_endpoint_service_methods jxta_endpoint_service_methods = {
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

static Jxta_listener *jxta_endpoint_service_lookup_listener(Jxta_endpoint_service * service, Jxta_endpoint_address * addr);
static void *APR_THREAD_FUNC outgoing_message_processor(apr_thread_t * t, void *arg);

static void outgoing_message_process(Jxta_endpoint_service * service, Jxta_message * msg);

void jxta_endpoint_service_destruct(Jxta_endpoint_service * service)
{
    apr_status_t status;

    /*
     * An overkill right now, but one day this
     * routine could be called from the outside.
     */
    PTValid(service, Jxta_endpoint_service);

    /* stop the thrad that processes outgoing messages */
    service->is_running = 0;
    apr_thread_join(&status, service->thread);

    /* delete tables and stuff */
    JXTA_OBJECT_RELEASE(service->unreachable_addresses);
    JXTA_OBJECT_RELEASE(service->transport_table);
    JXTA_OBJECT_RELEASE(service->listener_table);
    apr_pool_destroy(service->pool);

    /* un-ref our impl_adv and group, delete assigned_id */
    if (service->my_group)
        JXTA_OBJECT_RELEASE(service->my_group);
    if (service->my_groupid)
        JXTA_OBJECT_RELEASE(service->my_groupid);
    if (service->my_impl_adv)
        JXTA_OBJECT_RELEASE(service->my_impl_adv);
    if (service->my_assigned_id != 0) {
        JXTA_OBJECT_RELEASE(service->my_assigned_id);
    }

    if (service->myRoute != NULL)
        JXTA_OBJECT_RELEASE(service->myRoute);

    /* proceed to base classe's destruction */
    jxta_service_destruct((Jxta_service *) service);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

Jxta_status jxta_endpoint_service_construct(Jxta_endpoint_service * service, Jxta_endpoint_service_methods * methods)
{
    apr_status_t status;

    /*
     * An overkill right now, but one day this
     * routine could be called from the outside.
     *
     * Since we did not extend Jxta_service_methods, the run-time type
     * field shows as "Jxta_service_methods" so that's what we must check,
     * not "Jxta_endpoint_service_methods".
     */
    PTValid(methods, Jxta_service_methods);

    status = apr_pool_create(&service->pool, NULL);
    if (status != APR_SUCCESS)
        return JXTA_NOMEM;

    jxta_service_construct((Jxta_service *) service, (Jxta_service_methods *) methods);
    service->thisType = "Jxta_endpoint_service";

    service->filter_list = dl_make();
    service->listener_table = apr_hash_make(service->pool);
    service->transport_table = jxta_hashtable_new(2);
    service->unreachable_addresses = jxta_hashtable_new(5);

    apr_thread_mutex_create(&service->filter_list_mutex, APR_THREAD_MUTEX_NESTED, service->pool);
    apr_thread_mutex_create(&service->listener_table_mutex, APR_THREAD_MUTEX_NESTED, service->pool);

    service->outgoing_queue = queue_new(service->pool);

    service->is_running = 1;

    service->my_assigned_id = 0;
    service->my_impl_adv = 0;
    service->my_group = 0;
    service->my_groupid = NULL;
    service->transport = NULL;
    apr_thread_create(&service->thread, NULL,   /* no attr */
                      outgoing_message_processor, (void *) service, service->pool);

    return JXTA_SUCCESS;
}

static void jxta_endpoint_service_free(Jxta_object * obj)
{
    jxta_endpoint_service_destruct((Jxta_endpoint_service *) obj);
    free((void *) obj);
}

Jxta_endpoint_service *jxta_endpoint_service_new_instance(void)
{
    Jxta_status res;

    Jxta_endpoint_service *service = (Jxta_endpoint_service *)
        calloc(1, sizeof(Jxta_endpoint_service));

    JXTA_OBJECT_INIT(service, jxta_endpoint_service_free, 0);


    res = jxta_endpoint_service_construct(service, &jxta_endpoint_service_methods);
    if (res != JXTA_SUCCESS) {
        free(service);
        return NULL;
    }

    return service;
}


void jxta_endpoint_service_demux(Jxta_endpoint_service * service, Jxta_message * msg)
{
    Dlist *cur;
    Filter *cur_filter;
    Jxta_endpoint_address *dest;
    Jxta_vector *vector = NULL;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Endpoint Demux\n");

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
            if (!cur_filter->func(msg, cur_filter->arg))
                return;

        }
        if (vector != NULL) {
            JXTA_OBJECT_RELEASE(vector);
        }
    }

    apr_thread_mutex_unlock(service->filter_list_mutex);

    dest = jxta_message_get_destination(msg);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Demux: Destination of message is null\n");
    } else {
        char *destStr;
        JXTA_OBJECT_CHECK_VALID(dest);

        destStr = jxta_endpoint_address_to_string(dest);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: [%s] looking up listener for %s\n",
                        jstring_get_string(service->my_groupid), destStr);
        free(destStr);
    }

    if (dest != NULL) {
        Jxta_listener *listener = jxta_endpoint_service_lookup_listener(service, dest);

        if (listener != NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Demux: calling listener\n");
            jxta_listener_schedule_object(listener, (Jxta_object *) msg);

            JXTA_OBJECT_CHECK_VALID(msg);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No demux listener\n");
        }

        JXTA_OBJECT_RELEASE(dest);
    }
    JXTA_OBJECT_RELEASE(msg);
}


void jxta_endpoint_service_get_route_from_PA(Jxta_PA * padv, Jxta_RouteAdvertisement ** route)
{

    Jxta_vector *svcs;
    Jxta_svc *svc = NULL;
    size_t sz;
    size_t i;

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

void jxta_endpoint_service_add_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
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
    size_t sz;
    size_t i;
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
        JXTA_OBJECT_SHARE(service->myRoute);

        free(caddress);

        JXTA_OBJECT_RELEASE(addresses);
        JXTA_OBJECT_RELEASE(dest);
        JXTA_OBJECT_RELEASE(route);
        JXTA_OBJECT_RELEASE(jaddress);
        JXTA_OBJECT_RELEASE(address);
        JXTA_OBJECT_RELEASE(svc);
        JXTA_OBJECT_RELEASE(svcs);
        JXTA_OBJECT_RELEASE(pa);
    }

    /* add the transport to our table */
    /* FIXME: should check allow_overload etc. */
    jxta_hashtable_put(service->transport_table, jstring_get_string(name),      /* string gets copied */
                       jstring_length(name), (Jxta_object *) transport);

    /* String copied. Not needed anymore. */
    JXTA_OBJECT_RELEASE(name);
}

Jxta_RouteAdvertisement *jxta_endpoint_service_get_local_route(Jxta_endpoint_service * service)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_SHARE(service->myRoute);
    return service->myRoute;
}

void jxta_endpoint_service_set_local_route(Jxta_endpoint_service * service, Jxta_RouteAdvertisement * route)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->myRoute != NULL)
        JXTA_OBJECT_RELEASE(service->myRoute);

    service->myRoute = route;
    JXTA_OBJECT_SHARE(service->myRoute);
}

void jxta_endpoint_service_remove_transport(Jxta_endpoint_service * service, Jxta_transport * transport)
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
Jxta_transport *jxta_endpoint_service_lookup_transport(Jxta_endpoint_service * service, const char *transport_name)
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

extern void jxta_endpoint_service_add_filter(Jxta_endpoint_service * service, char const *str, JxtaEndpointFilter f, void *arg)
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


void jxta_endpoint_service_remove_filter(Jxta_endpoint_service * service, JxtaEndpointFilter filter)
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

static boolean is_listener_for(Jxta_endpoint_service * service, char *str)
{
    boolean res = FALSE;
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

Jxta_status
jxta_endpoint_service_add_listener(Jxta_endpoint_service * service,
                                   char const *serviceName, char const *serviceParam, Jxta_listener * listener)
{
    char *str = malloc((serviceName != NULL ? strlen(serviceName) : 0) + (serviceParam != NULL ? strlen(serviceParam) : 0) + 1);

    PTValid(service, Jxta_endpoint_service);

    sprintf(str, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Register listener for %s\n", str);

    apr_thread_mutex_lock(service->listener_table_mutex);

    if (is_listener_for(service, str)) {
        apr_thread_mutex_unlock(service->listener_table_mutex);
        free(str);
        return JXTA_BUSY;
    }

    JXTA_OBJECT_SHARE(listener);
    apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, (void *) listener);

    apr_thread_mutex_unlock(service->listener_table_mutex);
  /**free (str);**/
    return JXTA_SUCCESS;
}

Jxta_status
jxta_endpoint_service_remove_listener(Jxta_endpoint_service * service, char const *serviceName, char const *serviceParam)
{
    Jxta_listener *listener;
    char str[256];

    PTValid(service, Jxta_endpoint_service);

    sprintf(&str[0], "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    apr_thread_mutex_lock(service->listener_table_mutex);
    listener = (Jxta_listener *) apr_hash_get(service->listener_table, str, APR_HASH_KEY_STRING);
    if (listener != NULL) {
        apr_hash_set(service->listener_table, str, APR_HASH_KEY_STRING, NULL);
    }
    JXTA_OBJECT_RELEASE(listener);
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
    char *str;
    char *str1;
    const char *ea_svc_name;
    const char *ea_svc_params;

    PTValid(service, Jxta_endpoint_service);
    JXTA_OBJECT_CHECK_VALID(addr);

    ea_svc_name = jxta_endpoint_address_get_service_name(addr);
    ea_svc_params = jxta_endpoint_address_get_service_params(addr);

    str = malloc(strlen(ea_svc_name) + strlen(ea_svc_params) + 1);

    sprintf(str, "%s%s", ea_svc_name, ea_svc_params);

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

Jxta_status
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


Jxta_status
jxta_endpoint_service_send(Jxta_PG * obj, Jxta_endpoint_service * service, Jxta_message * msg, Jxta_endpoint_address * dest_addr)
{
    Jxta_stdpg *pg = PTValid(obj, Jxta_stdpg);
    Jxta_endpoint_address *new_addr;
    JString *cross_service = NULL;
    Jxta_id *id;
    JString *sid = NULL;
    const char *gname = NULL;
    char *caddress = NULL;
    Jxta_time exp = 0;
    Jxta_time now;
    JString *s_exp = NULL;

    PTValid(service, Jxta_endpoint_service);

    JXTA_OBJECT_CHECK_VALID(msg);

    JXTA_OBJECT_SHARE(msg);

    JXTA_OBJECT_CHECK_VALID(dest_addr);

    /*
     * check if this destination is reachable. We are maintaining
     * a negative cache of unreachable destination
     */

    jxta_hashtable_get(service->unreachable_addresses,
                       jxta_endpoint_address_get_protocol_address(dest_addr),
                       strlen(jxta_endpoint_address_get_protocol_address(dest_addr)), (Jxta_object **) & s_exp);

    if (s_exp != NULL) {
#ifdef WIN32
        sscanf(jstring_get_string(s_exp), "%I64d", &exp);
#else
        exp = atoll(jstring_get_string(s_exp));
#endif
        JXTA_OBJECT_RELEASE(s_exp);

        if (exp != 0) {
            now = jpr_time_now();
            if (exp > now) {
#ifdef WIN32
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destination [%s] marked unreachable for %I64dms\n",
                                jxta_endpoint_address_get_protocol_address(dest_addr), (exp - now));
#else
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destination [%s] marked unreachable for %lldms\n",
                                jxta_endpoint_address_get_protocol_address(dest_addr), (exp - now));
#endif
                return JXTA_UNREACHABLE_DEST;
            } else {

                /* 
                 * negative info expired remove the negative cache entry
                 */
                jxta_hashtable_del(service->unreachable_addresses,
                                   jxta_endpoint_address_get_protocol_address(dest_addr),
                                   strlen(jxta_endpoint_address_get_protocol_address(dest_addr)), (Jxta_object **) & s_exp);
                JXTA_OBJECT_RELEASE(s_exp);
            }
        }
    }

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
         * check if we already have the endpoint service address destination
         * in that case assumed that the right endpoint address was
         * set
         *
         * FIXME 20040510 tra we really should have a better way to 
         * standardize address processing through subgroups
         */
        if (strncmp(jxta_endpoint_address_get_service_name(dest_addr),
                    JXTA_ENDPOINT_SERVICE_NAME, strlen(JXTA_ENDPOINT_SERVICE_NAME)) != 0) {
            cross_service = jstring_new_2(JXTA_ENDPOINT_SERVICE_NAME);
            jstring_append_2(cross_service, ":");

            /*
             * Extract the group id of the messenger destination. This
             * is different from the endpoint group id, as the endpoint
             * service always run within the NetGroup.
             */
            jxta_PG_get_GID((Jxta_PG *) pg, &id);
            jxta_id_get_uniqueportion(id, &sid);
            JXTA_OBJECT_RELEASE(id);

            gname = jstring_get_string(sid);

            jstring_append_2(cross_service, gname);

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
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "New Addr=%s\n", caddress);
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


    return JXTA_SUCCESS;
}

void jxta_endpoint_service_set_relay(Jxta_endpoint_service * service, char *proto, char *host)
{
    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        free(service->relay_addr);
        free(service->relay_proto);
    }

    if (host != NULL) {
        service->relay_addr = malloc(strlen(host) + 1);
        strcpy(service->relay_addr, host);
        service->relay_proto = malloc(strlen(proto) + 1);
        strcpy(service->relay_proto, proto);

    } else {
        service->relay_addr = NULL;
        service->relay_proto = NULL;
    }
}

char *jxta_endpoint_service_get_relay_addr(Jxta_endpoint_service * service)
{
    char *addr = NULL;;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_addr != NULL) {
        addr = malloc(strlen(service->relay_addr) + 1);
        strcpy(addr, service->relay_addr);
        return addr;
    } else {
        return NULL;
    }
}


char *jxta_endpoint_service_get_relay_proto(Jxta_endpoint_service * service)
{
    char *proto = NULL;

    PTValid(service, Jxta_endpoint_service);

    if (service->relay_proto != NULL) {
        proto = malloc(strlen(service->relay_proto) + 1);
        strcpy(proto, service->relay_proto);
        return proto;
    } else {
        return NULL;
    }
}

static
void *APR_THREAD_FUNC outgoing_message_processor(apr_thread_t * t, void *arg)
{
    Jxta_endpoint_service *service = (Jxta_endpoint_service *) arg;
    Jxta_message *msg;

    JXTA_OBJECT_CHECK_VALID(service);

    while (service->is_running) {
        msg = (Jxta_message *) queue_dequeue(service->outgoing_queue, 1000 * 100);
        if (msg) {
            outgoing_message_process(service, msg);
            JXTA_OBJECT_RELEASE(msg);
        }
    }

    return NULL;
}


static
void outgoing_message_process(Jxta_endpoint_service * service, Jxta_message * msg)
{
    Jxta_endpoint_address *dest;
    JxtaEndpointMessenger *endpoint;
    Jxta_time now;
    char c_val[256];
    JString *s_exp = NULL;

    PTValid(service, Jxta_endpoint_service);

    dest = jxta_message_get_destination(msg);

    JXTA_OBJECT_CHECK_VALID(msg);
    JXTA_OBJECT_CHECK_VALID(dest);

    if (dest == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No destination. Message [%p] is discarded.\n", msg);
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Outgoing message [%p] :"
                    "\n\tProtocol : %s"
                    "\n\tAddress : %s"
                    "\n\tService : %s"
                    "\n\tParams : %s\n",
                    msg,
                    jxta_endpoint_address_get_protocol_name(dest),
                    jxta_endpoint_address_get_protocol_address(dest),
                    jxta_endpoint_address_get_service_name(dest), jxta_endpoint_address_get_service_params(dest));

    if (service->transport == NULL) {
        service->transport = jxta_endpoint_service_lookup_transport(service, "jxta");
    }

    /*
     * We have the router transport go ahead and demux to it for sending our message
     */
    if (service->transport != NULL) {
        endpoint = jxta_transport_messenger_get(service->transport, dest);
        JXTA_OBJECT_CHECK_VALID(endpoint);
        if (endpoint != NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Send message [%p] to router endpoint\n", msg);
            endpoint->jxta_send(endpoint, msg);
            if (endpoint->status != JXTA_SUCCESS) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                                "Failed to send message [%p] update unreachable address cache\n", msg);
                now = (Jxta_time) jpr_time_now();
                now = now + JXTA_ENDPOINT_DEST_UNREACHABLE_TIMEOUT;
#ifdef WIN32
                sprintf(c_val, "%I64d", now);
#else
                sprintf(c_val, "%lld", now);
#endif
                s_exp = jstring_new_2(c_val);
                jxta_hashtable_put(service->unreachable_addresses,
                                   jxta_endpoint_address_get_protocol_address(dest),
                                   strlen(jxta_endpoint_address_get_protocol_address(dest)), (Jxta_object *) s_exp);
                JXTA_OBJECT_RELEASE(s_exp);
            }
            JXTA_OBJECT_RELEASE(endpoint);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not find router endpoint\n");
        }
    }

    JXTA_OBJECT_RELEASE(dest);
}

/* vim: set ts=4 sw=4 tw=130 et: */
