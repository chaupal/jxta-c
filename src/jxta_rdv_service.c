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
 * $Id: jxta_rdv_service.c,v 1.34 2005/03/03 02:53:53 slowhog Exp $
 */

/**
 ** This is the implementation of JxtaRdvService struc.
 ** This definition must be set before including jxta.h.
 ** This is why reference to other JXTA types must be declared.
 **
 **/

#include "jxtaapr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_listener.h"
#include "jxta_peergroup.h"
#include "jxta_rdv_service.h"
#include "jxta_peer.h"
#include "jxta_svc.h"
#include "jxta_peer_private.h"
#include "jxta_rdv_service_private.h"
#include "jxta_rdv_service_provider_private.h"

/**
*   define the instantiator method for creating a rdv client. This should come from a header file.
**/
extern Jxta_rdv_service_provider *jxta_rdv_service_client_new(void);

/**
*   define the instantiator method for creating a rdv server. This should come from a header file.
**/
/* extern Jxta_rdv_service_provider *jxta_rdv_service_server_new(void); */

const char *JXTA_RDV_SERVICE_NAME = "urn:jxta:uuid-DEADBEEFDEAFBABAFEEDBABE0000000605";
const char *JXTA_RDV_PROPAGATE_ELEMENT_NAME = "jxta:RendezVousPropagate";
const char *JXTA_RDV_PROPAGATE_SERVICE_NAME = "JxtaPropagate";
const char *JXTA_RDV_CONNECT_REQUEST = "jxta:Connect";
const char *JXTA_RDV_DISCONNECT_REQUEST = "jxta:Disconnect";
const char *JXTA_RDV_CONNECTED_REPLY = "jxta:ConnectedPeer";
const char *JXTA_RDV_LEASE_REPLY = "jxta:ConnectedLease";
const char *JXTA_RDV_RDVADV_REPLY = "jxta:RdvAdvReply";
const char *JXTA_RDV_PING_REQUEST = "jxta:PingRequest";
const char *JXTA_RDV_PING_REPLY = "jxta:PingReply";
const char *JXTA_RDV_ADV_REPLY = "jxta:RdvAdv";
const char *JXTA_RDV_PEERVIEW_NAME = "PeerView";
const char *JXTA_RDV_PEERVIEW_EDGE = "jxta:PeerView.EdgePeer";
const char *JXTA_RDV_PEERVIEW_RESPONSE = "jxta:PeerView.RespWanted";
const char *JXTA_RDV_PEERVIEW_RDV = "jxta:PeerView.PeerAdv.Response";
const char *JXTA_RDV_PEERVIEW_PEERADV = "jxta:PeerView.PeerAdv";

static _jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * rdv, const _jxta_rdv_service_methods * methods);
static void rdv_service_delete(Jxta_object * service);
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv);

static Jxta_status init(Jxta_module * rdv, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * self, char *argv[]);
static void stop(Jxta_module * module);

static Jxta_status add_event_listener(Jxta_rdv_service * service, const char *serviceName, const char *serviceParam,
                                      Jxta_rdv_event_listener * listener);
static Jxta_status remove_event_listener(Jxta_rdv_service * service, const char *serviceName, const char *serviceParam);
static Jxta_status add_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam,
                                          Jxta_listener * listener);
static Jxta_status remove_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam,
                                             Jxta_listener * listener);
static Jxta_status add_peer(Jxta_rdv_service * rdv, Jxta_peer * peer);
static Jxta_status add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer);
static Jxta_status get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist);
static Jxta_status propagate(Jxta_rdv_service * rdv, Jxta_message * msg, char *serviceName, char *serviceParam, int ttl);

static void jxta_rdv_event_delete(Jxta_object * ptr);
static Jxta_rdv_event *jxta_rdv_event_new(Jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id);

static const _jxta_rdv_service_methods JXTA_RDV_SERVICE_METHODS = {
    {
     {
      "Jxta_module_methods",
      init,
      jxta_module_init_e_impl,
      start,
      stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl},
    "_jxta_rdv_service_methods",
    add_peer,
    add_seed,
    get_peers,
    propagate
};

/**
 **
 ** This is the Generic Rendezvous Service
 **
 ** This code only implements the C portion of the generic part
 ** of the Rendezvous Service, in other words, the glue between
 ** the public API and the actual implementation.
 **
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
    _jxta_rdv_service *self = (_jxta_rdv_service *) malloc(sizeof(_jxta_rdv_service));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(_jxta_rdv_service));
    JXTA_OBJECT_INIT(self, rdv_service_delete, 0);

    /* call the hierarchy of ctors and Return the new object */

    return (Jxta_rdv_service *) jxta_rdv_service_construct(self, &JXTA_RDV_SERVICE_METHODS);
}

/**
 * The base rdv service ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types).
 */
static _jxta_rdv_service *jxta_rdv_service_construct(_jxta_rdv_service * self, const _jxta_rdv_service_methods * methods)
{
    PTValid(methods, _jxta_rdv_service_methods);

    self = (_jxta_rdv_service *) jxta_service_construct((_jxta_service *) self, (Jxta_service_methods *) methods);

    if (NULL != self) {
        apr_status_t res = APR_SUCCESS;

        self->thisType = "_jxta_rdv_service";

        /* Create the pool & mutex */
        apr_pool_create(&self->pool, NULL);
        res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                      self->pool);

        self->endpoint = NULL;
        self->provider = NULL;

        /*
         * create the RDV event listener table
         */
        self->evt_listener_table = jxta_hashtable_new(1);
    }

    return self;
}

/**
 * The base rdv service dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 */
static void jxta_rdv_service_destruct(_jxta_rdv_service * rdv)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    if (self->provider) {
        JXTA_OBJECT_RELEASE(self->provider);
    }

    if (NULL != self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }

    if (NULL != self->evt_listener_table) {
        JXTA_OBJECT_RELEASE(self->evt_listener_table);
    }

    /* Free the pool used to allocate the thread and mutex */
    apr_pool_destroy(self->pool);

    self->thisType = NULL;

    jxta_service_destruct((_jxta_service *) rdv);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((_jxta_rdv_service_methods*) JXTA_MODULE_VTBL(rdv))
#define PROVIDER_VTBL(provider) ((_jxta_rdv_service_provider_methods*) ((provider)->methods))

/**
 ** Creates a rdv event
 **/
static Jxta_rdv_event *jxta_rdv_event_new(Jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) malloc(sizeof(Jxta_rdv_event));

    if (NULL == rdv_event) {
        JXTA_LOG("Could not malloc rdv event");
        return NULL;
    }

    JXTA_OBJECT_INIT(rdv_event, jxta_rdv_event_delete, NULL);

    rdv_event->source = rdv;
    rdv_event->event = event;
    rdv_event->pid = id;

    JXTA_LOG("EVENT: Create a new rdv event [%p]\n", rdv_event);
    return rdv_event;
}

/**
 ** Deletes a rdv event
 **/
static void jxta_rdv_event_delete(Jxta_object * ptr)
{
    Jxta_rdv_event *rdv_event = (Jxta_rdv_event *) ptr;

    JXTA_LOG("Event [%p] delete \n", ptr);
    JXTA_OBJECT_RELEASE(rdv_event->pid);
    rdv_event->pid = NULL;

    free(rdv_event);
}

void jxta_rdv_generate_event(_jxta_rdv_service * rdv, Jxta_Rendezvous_event_type event, Jxta_id * id)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_rdv_event *rdv_event = jxta_rdv_event_new((Jxta_rdv_service *) rdv, event, id);
    Jxta_listener *listener = NULL;
    Jxta_vector *lis = NULL;
    int i = 0;
    Jxta_status res = JXTA_SUCCESS;

    lis = jxta_hashtable_values_get(self->evt_listener_table);
    if (lis != NULL) {
        for (i = 0; i < jxta_vector_size(lis); i++) {
            res = jxta_vector_get_object_at(lis, (Jxta_object **) & listener, i);
            if (res == JXTA_SUCCESS) {
                JXTA_LOG("calling RDV listener\n");
                jxta_listener_schedule_object(listener, (Jxta_object *)
                                              rdv_event);
                JXTA_OBJECT_RELEASE(listener);
            }
        }
        if (lis != NULL)
            JXTA_OBJECT_RELEASE(lis);
    }

    JXTA_OBJECT_RELEASE(rdv_event);
}

Jxta_status jxta_rdv_service_add_peer(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((rdv == NULL) || (peer == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    return VTBL->add_peer(rdv, peer);
}

Jxta_status jxta_rdv_service_add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (peer == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    return VTBL->add_seed(rdv, peer);
}


/**
 ** Get the list of peers used by the Rendezvous Service with their status.
 * 
 * @param service a pointer to the instance of the Rendezvous Service
 * @param pAdv a pointer to a pointer that contains the list of peers
 * @return error code.
 **/
Jxta_status jxta_rdv_service_get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (peerlist == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    return VTBL->get_peers(rdv, peerlist);

}

/**
 * Adds an Event listener that will receive rendezvous Service event, such
 * as connection, disconnection, lease request, lease grant events.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param listener a pointer to an event listener. 
 * @return error code.
 **/
Jxta_status jxta_rdv_service_add_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam, void *listener)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (listener == NULL) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    return add_event_listener(rdv, serviceName, serviceParam, listener);
}

/**
 * Removes an Event listener
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param listener a pointer to an event listener to remove.
 * @return error code.
 **/
Jxta_status jxta_rdv_service_remove_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((rdv == NULL) || (serviceName == NULL) || (serviceParam == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    return remove_event_listener(rdv, serviceName, serviceParam);
}

/**
 * Test if the peer is a rendezvous peer for the given instance of rendezvous Service.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
boolean jxta_rdv_service_is_rendezvous(Jxta_rdv_service * rdv)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (rdv == NULL) {
        /* Invalid args. We can not return an error, so return FALSE */
        return FALSE;
    }

    return FALSE;
}

/**
 * Test if the peer is connected. For now existing and not expired is what we use to decide.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
boolean jxta_rdv_service_peer_is_connected(Jxta_rdv_service * rdv, Jxta_peer * peer)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((rdv == NULL) || (NULL == peer)) {
        /* Invalid args. We can not return an error, so return FALSE */
        return FALSE;
    }

    return jpr_time_now() <= PEER_ENTRY_VTBL(peer)->jxta_peer_get_expires(peer) ? TRUE : FALSE;
}

/**
 * Test if the peer is connected. For now existing and not expired is what we use to decide.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @return TRUE if the peer is a rendezvous, FALSE otherwise.
 **/
boolean jxta_rdv_service_peer_is_rdv(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if (NULL == peer) {
        /* Invalid args. We can not return an error, so return FALSE */
        return FALSE;
    }

    /* FIXME 20050117 bondolo This return value is entirely made up. I'd prefer to kill this method. */
    return TRUE;
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
Jxta_status jxta_rdv_service_propagate(Jxta_rdv_service * rdv, Jxta_message * msg, char *serviceName, char *serviceParam, int ttl)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((msg == NULL) || (ttl <= 0)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    return VTBL->propagate(rdv, msg, serviceName, serviceParam, ttl);
}

/**
 * NB the functions above constitute the interface. There are no default implementations
 * of the interface methods, therefore there is no methods table for the class, and there
 * is no new_instance function either.
 * No one is supposed to create a object of this exact class.
 */

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

    res = jxta_service_init((Jxta_service *) rdv, group, assigned_id, impl_adv);

    if (res == JXTA_SUCCESS) {
        jxta_PG_get_endpoint_service(group, &(self->endpoint));

        /* XXX bondolo 20050118 Change this to jxta_rdv_service_server_new to instantiate as server. */
        self->provider = jxta_rdv_service_client_new();

        res = PROVIDER_VTBL(self->provider)->init(self->provider, self);
    }

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
static Jxta_status start(Jxta_module * module, char *argv[])
{
    _jxta_rdv_service *self = PTValid(module, _jxta_rdv_service);

    return PROVIDER_VTBL(self->provider)->start(self->provider);
}

/**
 * Stops an instance of the Rendezvous Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param rdv a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static void stop(Jxta_module * module)
{
    _jxta_rdv_service *self = PTValid(module, _jxta_rdv_service);

    PROVIDER_VTBL(self->provider)->stop(self->provider);
}

/**
 * Adds an Propagation listener. The listener is invoked when there is a incoming
 * propagated message for the given EndpointAddress.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was already a listener for that address.
 **/
Jxta_status jxta_rdv_service_add_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam,
                                                    Jxta_listener * listener)
{
    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((rdv == NULL) || (listener == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    return add_propagate_listener(rdv, serviceName, serviceParam, listener);
}

/**
 * Removes a Propagation listener.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was not already a listener for that address.
 **/
Jxta_status jxta_rdv_service_remove_propagate_listener(Jxta_rdv_service * rdv,
                                                       char *serviceName, char *serviceParam, Jxta_listener * listener)
{

    PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((rdv == NULL) || (listener == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }

    return remove_propagate_listener(rdv, serviceName, serviceParam, listener);
}

static boolean is_listener_for(_jxta_rdv_service * self, char *str)
{
    Jxta_boolean res = FALSE;
    Jxta_listener *listener = NULL;

    jxta_hashtable_get(self->evt_listener_table, str, strlen(str), (Jxta_object **) & listener);
    if (listener != NULL) {
        res = TRUE;
        JXTA_OBJECT_RELEASE(listener);
    }

    return res;
}

/**
 * Adds an Event listener that will receive rendezvous Service event, such
 * as connection, disconnection, lease request, lease grant events.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param listener a pointer to an event listener. 
 * @return error code.
 **/
static Jxta_status add_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam,
                                      Jxta_rdv_event_listener * listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);
    Jxta_status res = JXTA_SUCCESS;

    char *str = malloc((serviceName != NULL ? strlen(serviceName) : 0) + (serviceParam != NULL ? strlen(serviceParam) : 0) + 1);

    sprintf(str, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    if (is_listener_for(self, str)) {
        free(str);
        return JXTA_BUSY;
    }

    jxta_hashtable_put(self->evt_listener_table, str, strlen(str), (Jxta_object *) listener);

    free(str);
    return res;
}

/**
 * Removes an Event listener
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param listener a pointer to an event listener to remove.
 * @return error code.
 **/
static Jxta_status remove_event_listener(Jxta_rdv_service * rdv, const char *serviceName, const char *serviceParam)
{
    Jxta_listener *listener;
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);


    char *str = malloc((serviceName != NULL ? strlen(serviceName) : 0) + (serviceParam != NULL ? strlen(serviceParam) : 0) + 1);

    sprintf(str, "%s%s", (serviceName != NULL ? serviceName : ""), (serviceParam != NULL ? serviceParam : ""));

    jxta_hashtable_del(self->evt_listener_table, str, strlen(str), (Jxta_object **) & listener);
    if (listener != NULL) {
        JXTA_OBJECT_RELEASE(listener);
    }

    free(str);
    return JXTA_SUCCESS;
}

/**
 * Adds an Propagation listener. The listener is invoked when there is a incoming
 * propagated message for the given EndpointAddress.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was already a listener for that address.
 **/
static Jxta_status add_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam, Jxta_listener * listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((self == NULL) || (listener == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_endpoint_service_add_listener(self->endpoint, serviceName, serviceParam, listener);
}

/**
 * Removes a Propagation listener.
 *
 * @param service a pointer to the instance of the Rendezvous Service
 * @param addr An EndpointAddress that defines on which address the listener is listening
 * to. Note that only the Service Name and Service Parameter of the EndpointAddress is used.
 * @param listener a pointer to an EndpointListener.
 * @return error code. Fails if there was not already a listener for that address.
 **/
static Jxta_status remove_propagate_listener(Jxta_rdv_service * rdv, char *serviceName, char *serviceParam,
                                             Jxta_listener * listener)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    /* Test arguments first */
    if ((self == NULL) || (listener == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_endpoint_service_remove_listener(self->endpoint, serviceName, serviceParam);
}

Jxta_endpoint_service *jxta_rdv_service_get_endpoint_priv(_jxta_rdv_service * self)
{
    return self->endpoint;
}

/**
 ** Generates a unique number and returns it as a newly malloced string.
 ** The caller is responsible for freeing the string.
 **/
Jxta_status message_id_new(Jxta_id * pgid, JString ** sid)
{
    Jxta_id *id;
    Jxta_status status;

    /*
     * need to generate a unique UUID for msg ID
     *
     * FIXME 20040225 tra we should use a different UUID generator
     * There is no reason to use a codat Id here
     */
    status = jxta_id_codatid_new_1(&id, pgid);
    if (status == JXTA_SUCCESS) {
        jxta_id_to_jstring(id, sid);
        JXTA_OBJECT_RELEASE(id);
        return JXTA_SUCCESS;
    } else {
        JXTA_LOG("error get a new ID\n");
        return JXTA_FAILED;
    }
}

/**
 * Construct the associated peergroup RDV wire pipe
 *
 * FIXME : Errors will make this leak like a sieve.
 **/
Jxta_pipe_adv *jxta_rdv_get_wirepipeadv(_jxta_rdv_service * self)
{
    Jxta_pipe_adv *adv = NULL;
    Jxta_status status;
    JString *buf;
    JString *pipeId;
    Jxta_id *pid;
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) self);
    Jxta_PG *parent;
    Jxta_PGID *pgid;
    Jxta_PGID *parentpgid;
    JString *pgIdString;

    jxta_PG_get_GID(group, &pgid);
    jxta_id_get_uniqueportion(pgid, &pgIdString);

    buf = jstring_new_2(JXTA_RDV_PEERVIEW_NAME);
    jstring_append_1(buf, pgIdString);

    jxta_PG_get_parentgroup(group, &parent);
    jxta_PG_get_GID(parent, &parentpgid);

    /* create pipe id in the parent group */
    status = jxta_id_pipeid_new_2(&pid, parentpgid, (unsigned char *) jstring_get_string(buf),
                                  (size_t) (strlen(jstring_get_string(buf)) - 1));

    JXTA_OBJECT_RELEASE(parentpgid);
    JXTA_OBJECT_RELEASE(buf);

    if (status != JXTA_SUCCESS) {
        JXTA_LOG("Error creating RDV wire pipe adv\n");
        return NULL;
    }

    jxta_id_to_jstring(pid, &pipeId);
    JXTA_OBJECT_RELEASE(pid);

    adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, jstring_get_string(pipeId));
    JXTA_OBJECT_RELEASE(pipeId);
    jxta_pipe_adv_set_Type(adv, JXTA_PROPAGATE_PIPE);
    jxta_pipe_adv_set_Name(adv, JXTA_RDV_PEERVIEW_NAME);

    return adv;
}

static Jxta_RdvAdvertisement *jxta_rdv_build_rdva(_jxta_rdv_service * self)
{
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) self);
    Jxta_PG *parent;
    Jxta_PID *peerid;
    Jxta_PGID *pgid;
    JString *name;
    JString *buf;
    JString *pgIdString;
    Jxta_id *pid;
    Jxta_vector *svcs = NULL;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_PA *padv;
    Jxta_RdvAdvertisement *rdva = NULL;
    Jxta_svc *svc = NULL;
    int sz;
    int i;

    jxta_PG_get_GID(group, &pgid);
    jxta_id_to_jstring(pgid, &pgIdString);

    jxta_PG_get_parentgroup(group, &parent);
    jxta_PG_get_PA(parent, &padv);
    JXTA_OBJECT_RELEASE(parent);

    rdva = jxta_RdvAdvertisement_new();

    name = jxta_PA_get_Name(padv);
    jxta_RdvAdvertisement_set_Name(rdva, jstring_get_string(name));
    JXTA_OBJECT_RELEASE(name);

    pid = jxta_service_get_assigned_ID_priv((_jxta_service *) self);
    jxta_id_to_jstring(pid, &buf);
    jstring_append_1(buf, pgIdString);
    JXTA_OBJECT_RELEASE(pgIdString);

    jxta_RdvAdvertisement_set_Service(rdva, jstring_get_string(buf));
    JXTA_OBJECT_RELEASE(buf);

    jxta_RdvAdvertisement_set_RdvGroupId(rdva, pgid);
    JXTA_OBJECT_RELEASE(pgid);

    peerid = jxta_PA_get_PID(padv);
    jxta_RdvAdvertisement_set_RdvPeerId(rdva, peerid);
    JXTA_OBJECT_RELEASE(peerid);

    svcs = jxta_PA_get_Svc(padv);
    sz = jxta_vector_size(svcs);
    for (i = 0; i < sz; i++) {
        Jxta_id *mcid;
        Jxta_svc *tmpsvc = NULL;
        jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
        mcid = jxta_svc_get_MCID(tmpsvc);

        if (jxta_id_equals(mcid, jxta_endpoint_classid)) {
            svc = tmpsvc;
            JXTA_OBJECT_RELEASE(mcid);
            break;
        }
        JXTA_OBJECT_RELEASE(mcid);
        JXTA_OBJECT_RELEASE(tmpsvc);
    }
    JXTA_OBJECT_RELEASE(padv);
    JXTA_OBJECT_RELEASE(svcs);

    if (svc == NULL) {
        JXTA_LOG("could not find the endpoint service\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    route = jxta_svc_get_RouteAdvertisement(svc);
    if (route == NULL) {
        JXTA_LOG("could not find the route advertisement\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    jxta_RdvAdvertisement_set_Route(rdva, route);

    return rdva;
}

Jxta_status jxta_rdv_send_rdv_request(_jxta_rdv_service * self, Jxta_pipe_adv * adv)
{
    Jxta_status status;
    Jxta_PG *group = jxta_service_get_peergroup_priv((_jxta_service *) self);

    Jxta_pipe_service *pipe_service;
    Jxta_pipe *pipe;
    Jxta_outputpipe *op = NULL;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    JString *peerdoc;
    Jxta_RdvAdvertisement *rdva = jxta_rdv_build_rdva(self);
    Jxta_PG *parent;

    status = jxta_RdvAdvertisement_get_xml(rdva, &peerdoc);
    JXTA_OBJECT_RELEASE(rdva);

    if (status != JXTA_SUCCESS) {
        JXTA_LOG("rdv service::Failed to retrieve my RdvAdvertisement\n");
        return status;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_1((char *) JXTA_RDV_PEERVIEW_PEERADV,
                                    (char *) "text/xml", (char *) jstring_get_string(peerdoc), jstring_length(peerdoc), NULL);

    JXTA_OBJECT_RELEASE(peerdoc);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    el = jxta_message_element_new_1((char *) JXTA_RDV_PEERVIEW_EDGE, (char *) "text/plain", "true", strlen("true"), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    el = jxta_message_element_new_1((char *) JXTA_RDV_PEERVIEW_RESPONSE, (char *) "text/plain", "true", strlen("true"), NULL);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    JXTA_OBJECT_CHECK_VALID(msg);

    /*
     * Create the output wirepipe
     */
    jxta_PG_get_parentgroup(group, &parent);
    jxta_PG_get_pipe_service(parent, &pipe_service);
    JXTA_OBJECT_RELEASE(parent);

    JXTA_LOG("Waiting to bind RDV wire pipe\n");
    status = jxta_pipe_service_timed_connect(pipe_service, adv, 15 * 1000, NULL, &pipe);
    JXTA_OBJECT_RELEASE(pipe_service);

    if (status != JXTA_SUCCESS) {
        JXTA_LOG("Cannot connect to output RDV pipe\n");
        return status;
    }

    if (pipe == NULL) {
        JXTA_LOG("Could not bind RDV wire pipe\n");
        return JXTA_FAILED;
    }

    status = jxta_pipe_get_outputpipe(pipe, &op);
    JXTA_OBJECT_RELEASE(pipe);

    if (status != JXTA_SUCCESS) {
        JXTA_LOG("Cannot get output wire RDV pipe\n");
        return status;
    }

    if (op == NULL) {
        JXTA_LOG("Cannot get output pipe\n");
        return JXTA_FAILED;
    }

    status = jxta_outputpipe_send(op, msg);
    if (status != JXTA_SUCCESS) {
        JXTA_LOG("sending RDV wire pipe failed: %d\n", (int) status);
    }
    JXTA_OBJECT_RELEASE(op);
    JXTA_OBJECT_RELEASE(msg);

    return status;
}

static Jxta_status add_peer(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return PROVIDER_VTBL(self->provider)->add_peer(self->provider, peer);
}

static Jxta_status add_seed(Jxta_rdv_service * rdv, Jxta_peer * peer)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return PROVIDER_VTBL(self->provider)->add_seed(self->provider, peer);
}

static Jxta_status get_peers(Jxta_rdv_service * rdv, Jxta_vector ** peerlist)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return PROVIDER_VTBL(self->provider)->get_peers(self->provider, peerlist);
}

static Jxta_status propagate(Jxta_rdv_service * rdv, Jxta_message * msg, char *serviceName, char *serviceParam, int ttl)
{
    _jxta_rdv_service *self = PTValid(rdv, _jxta_rdv_service);

    return PROVIDER_VTBL(self->provider)->propagate(self->provider, msg, serviceName, serviceParam, ttl);
}

/* vim: set ts=4 sw=4 et tw=130: */
