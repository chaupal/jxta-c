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
 * $Id: jxta_peerinfo_service_ref.c,v 1.8 2005/01/29 19:13:42 bondolo Exp $
 */


#include "jxtaapr.h"

#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_id.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"

#include "jxta_private.h"
#include "jxta_peerinfo_service_private.h"

#define HAVE_POOL

typedef struct {

    Extends(Jxta_peerinfo_service);
    JString *instanceName;
    boolean running;
    Jxta_PG *group;
    Jxta_resolver_service *resolver;
    Jxta_hashtable *listeners;
    Jxta_PID *localPeerId;
    Jxta_id *assigned_id;
    Jxta_advertisement *impl_adv;
    apr_pool_t *pool;
} Jxta_peerinfo_service_ref;


/*
 * module methods
 */

/**
 * Initializes an instance of the Peerinfo Service.
 * 
 * @param service a pointer to the instance of the Peerinfo Service.
 * @param group a pointer to the PeerGroup the Peerinfo Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

Jxta_status
jxta_peerinfo_service_ref_init(Jxta_module * peerinfo, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{

    Jxta_peerinfo_service_ref *self = (Jxta_peerinfo_service_ref *) peerinfo;

    /* Test arguments first */
    if ((peerinfo == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }


    apr_pool_create(&self->pool, NULL);

    /* store a copy of our assigned id */
    if (assigned_id != 0) {
        JXTA_OBJECT_SHARE(assigned_id);
        self->assigned_id = assigned_id;
    }

    /* keep a reference to our group and impl adv */
    if (group != 0)
        JXTA_OBJECT_SHARE(group);
    if (impl_adv != 0)
        JXTA_OBJECT_SHARE(impl_adv);
    self->group = group;
    self->impl_adv = impl_adv;

    jxta_PG_get_resolver_service(group, &(self->resolver));
    jxta_PG_get_PID(group, &(self->localPeerId));

    return JXTA_SUCCESS;
}

/**
 * Initializes an instance of the Peerinfo Service. (exception variant).
 * 
 * @param service a pointer to the instance of the Peerinfo Service.
 * @param group a pointer to the PeerGroup the Peerinfo Service is 
 * initialized for.
 *
 */

static void init_e(Jxta_module * peerinfo, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws)
{
    Jxta_status s = jxta_peerinfo_service_ref_init(peerinfo, group, assigned_id, impl_adv);
    if (s != JXTA_SUCCESS)
        Throw(s);
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param self a pointer to the instance of the Peerinfo Service
 * @param argv a vector of string arguments.
 */
static Jxta_status start(Jxta_module * self, char *argv[])
{
    /* easy as pai */
    return JXTA_SUCCESS;
}


/**
 * Stops an instance of the Peerinfo Service.
 * 
 * @param service a pointer to the instance of the Peerinfo Service
 * @return Jxta_status
 */
static void stop(Jxta_module * peerinfo)
{
    JXTA_LOG("Stopped.\n");
    /* nothing special to stop */
}

/*
 * service methods
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Peerinfo Service
 * @return the advertisement.
 */
static void get_mia(Jxta_service * peerinfo, Jxta_advertisement ** mia)
{

    Jxta_peerinfo_service_ref *self = (Jxta_peerinfo_service_ref *) peerinfo;
    PTValid(self, Jxta_peerinfo_service_ref);

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
    PTValid(self, Jxta_peerinfo_service_ref);

    JXTA_OBJECT_SHARE(self);
    *svc = self;
}


/*
 * Peerinfo methods proper.
 */

Jxta_status getRemotePeerInfo(Jxta_peerinfo_service * self, Jxta_id * peerid, Jxta_peerinfo_listener * listener)
{
    return JXTA_SUCCESS;
}


Jxta_status getLocalPeerInfo(Jxta_peerinfo_service * self, Jxta_id * peerid, Jxta_object ** adv)
{
    return JXTA_SUCCESS;
}


Jxta_status getPeerInfoService(Jxta_peerinfo_service * service, Jxta_object ** adv)
{
    return JXTA_SUCCESS;
}

Jxta_status flushPeerInfo(Jxta_peerinfo_service * self, Jxta_id * peerid)
{
    return JXTA_SUCCESS;
}

/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_Peerinfo_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_Peerinfo_service_methods -> Jxta_Peerinfo_service_ref_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_peerinfo_service_methods Jxta_peerinfo_service_ref_methods;

Jxta_peerinfo_service_ref_methods jxta_peerinfo_service_ref_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_peerinfo_service_ref_init,
      init_e,
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface},
    "Jxta_peerinfo_service_methods",
    getRemotePeerInfo,
    getLocalPeerInfo,
    getPeerInfoService,
    flushPeerInfo
};

void jxta_peerinfo_service_ref_construct(Jxta_peerinfo_service_ref * self, Jxta_peerinfo_service_ref_methods * methods)
{
    /*
     * we do not extend Jxta_Peerinfo_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_peerinfo_service_methods);

    jxta_peerinfo_service_construct((Jxta_peerinfo_service *) self, (Jxta_peerinfo_service_methods *) methods);

    /* Set our rt type checking string */
    self->thisType = "Jxta_peerinfo_service_ref";
}

void jxta_peerinfo_service_ref_destruct(Jxta_peerinfo_service_ref * self)
{
    PTValid(self, Jxta_peerinfo_service_ref);

    /* release/free/destroy our own stuff */

    if (self->resolver != 0)
        JXTA_OBJECT_RELEASE(self->resolver);
    if (self->localPeerId != 0)
        JXTA_OBJECT_RELEASE(self->localPeerId);
    if (self->instanceName != 0)
        JXTA_OBJECT_RELEASE(self->instanceName);
    if (self->group != 0)
        JXTA_OBJECT_RELEASE(self->group);
    if (self->impl_adv != 0)
        JXTA_OBJECT_RELEASE(self->impl_adv);
    if (self->assigned_id != 0)
        JXTA_OBJECT_RELEASE(self->assigned_id);
    if (self->pool != 0)
        apr_pool_destroy(self->pool);

    /* call the base classe's dtor. */
    jxta_peerinfo_service_destruct((Jxta_peerinfo_service *) self);

    JXTA_LOG("Destruction finished\n");
}

/**
 * Frees an instance of the Peerinfo Service.
 * Note: freeing of memory pool is the responsibility of the caller.
 * 
 * @param service a pointer to the instance of the Peerinfo Service to free.
 */
static void peerinfo_free(Jxta_object * service)
{
    /* call the hierarchy of dtors */
    jxta_peerinfo_service_ref_destruct((Jxta_peerinfo_service_ref *) service);

    /* free the object itself */
    free(service);
}


/**
 * Creates a new instance of the Peerinfo Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_Peerinfo_service_init().
 *
 * @param pool a pointer to the pool of memory that needs to be used in order
 * to allocate the Peerinfo Service.
 * @return a non initialized instance of the Peerinfo Service
 */

Jxta_peerinfo_service_ref *jxta_peerinfo_service_ref_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_peerinfo_service_ref *self = (Jxta_peerinfo_service_ref *)
        malloc(sizeof(Jxta_peerinfo_service_ref));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(Jxta_peerinfo_service_ref));
    JXTA_OBJECT_INIT(self, peerinfo_free, 0);

    /* call the hierarchy of ctors */
    jxta_peerinfo_service_ref_construct(self, &jxta_peerinfo_service_ref_methods);

    /* return the new object */
    return self;
}

/* END OF STANDARD SERVICE IMPLEMENTATION CODE */
