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
 * $Id: jxta_membership_service_null.c,v 1.17 2006/09/08 19:17:54 bondolo Exp $
 */

static const char *__log_cat = "membership_null";

#include <assert.h>
#include "jxta_apr.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_id.h"
#include "jxta_hashtable.h"
#include "jxta_peergroup.h"
#include "jxta_vector.h"

#include "jxta_cred_null.h"

#include "jxta_private.h"
#include "jxta_membership_service_private.h"

typedef struct {

    Extends(Jxta_membership_service);

    Jxta_PG *group;

    Jxta_id *assigned_id;

    Jxta_advertisement *impl_adv;

    Jxta_vector *creds;

} Jxta_membership_service_null;

static Jxta_status resign(Jxta_membership_service * svc);

/*
 * module methods
 */

/**
 * Initializes an instance of the Membership Service.
 * 
 * @param service a pointer to the instance of the Membership Service.
 * @param group a pointer to the PeerGroup the Membership Service is 
 * initialized for.
 * @return Jxta_status
 *
 */

Jxta_status
jxta_membership_service_null_init(Jxta_module * membership, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{

    Jxta_membership_service_null *self = (Jxta_membership_service_null *) membership;

    /* Test arguments first */
    if ((membership == NULL) || (group == NULL)) {
        /* Invalid args. */
        return JXTA_INVALID_ARGUMENT;
    }


    /* store a copy of our assigned id */
    if (assigned_id != NULL) {
        JXTA_OBJECT_SHARE(assigned_id);
        self->assigned_id = assigned_id;
    }

    /* keep a reference to our group and impl adv */

    if (impl_adv != NULL) {
        JXTA_OBJECT_SHARE(impl_adv);
    }
    self->group = group;
    self->impl_adv = impl_adv;

    self->creds = jxta_vector_new(5);

    return JXTA_SUCCESS;
}

/**
 * Starts what init could not start (due to the unavailability of other
 * services, or need for arguments typically). (a module method)
 * Right now every is started by init. When we put all the services
 * together it is unlikely to be so easy.
 *
 * @param this a pointer to the instance of the Membership Service
 * @param argv a vector of string arguments.
 */
static Jxta_status start(Jxta_module * self, const char *argv[])
{
    /* easy as pai */

    resign((Jxta_membership_service *) self);

    return JXTA_SUCCESS;
}


/**
 * Stops an instance of the Membership Service.
 * 
 * @param service a pointer to the instance of the Membership Service
 * @return Jxta_status
 */
static void stop(Jxta_module * membership)
{
    Jxta_membership_service_null *self = (Jxta_membership_service_null *) membership;

    jxta_vector_clear(self->creds);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Stopped\n");
    /* nothing special to stop */
}

/*
 * service methods
 */

/**
 * return this service's own impl adv. (a service method).
 *
 * @param service a pointer to the instance of the Membership Service
 * @return the advertisement.
 */
static void get_mia(Jxta_service * membership, Jxta_advertisement ** mia)
{

    Jxta_membership_service_null *self = (Jxta_membership_service_null *) membership;
    PTValid(self, Jxta_membership_service_null);

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
    PTValid(self, Jxta_membership_service_null);

    JXTA_OBJECT_SHARE(self);
    *svc = self;
}


/*
 * Membership methods proper.
 */

static Jxta_status apply(Jxta_membership_service * self, Jxta_credential * authcred, Jxta_membership_authenticator ** auth)
{
    return JXTA_NOTIMP;

}

static Jxta_status join(Jxta_membership_service * self, Jxta_membership_authenticator * auth, Jxta_credential ** newcred)
{
    return JXTA_NOTIMP;

}

static Jxta_status resign(Jxta_membership_service * svc)
{
    Jxta_membership_service_null *self = (Jxta_membership_service_null *) svc;
    Jxta_id *pg;
    Jxta_id *peer;
    Jxta_object *cred;
    JString *identity = jstring_new_0();

    PTValid(self, Jxta_membership_service_null);

    assert(0 == jxta_vector_size(self->creds));

    jxta_PG_get_GID(self->group, &pg);

    jxta_PG_get_PID(self->group, &peer);

    jstring_append_2(identity, "nobody");

    cred = (Jxta_object *) jxta_cred_null_new((Jxta_service *) self, pg, peer, identity);
    if (NULL != cred) {
        jxta_vector_add_object_last(self->creds, cred);
        JXTA_OBJECT_RELEASE(cred);
    }

    JXTA_OBJECT_RELEASE(pg);
    JXTA_OBJECT_RELEASE(peer);
    JXTA_OBJECT_RELEASE(identity);

    return JXTA_SUCCESS;
}

static Jxta_status currentcreds(Jxta_membership_service * svc, Jxta_vector ** creds)
{

    Jxta_membership_service_null *self = (Jxta_membership_service_null *) svc;

    if (NULL == creds)
        return JXTA_INVALID_ARGUMENT;

    *creds = JXTA_OBJECT_SHARE(self->creds);

    return JXTA_SUCCESS;
}


static Jxta_status makecred(Jxta_membership_service * self, JString * somecred, Jxta_credential ** cred)
{

    return JXTA_NOTIMP;
}

/* BEGINING OF STANDARD SERVICE IMPLEMENTATION CODE */

/**
 * This implementation's methods.
 * The typedef could go in jxta_Membership_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_Membership_service_methods -> Jxta_Membership_service_null_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

typedef Jxta_membership_service_methods Jxta_membership_service_null_methods;

Jxta_membership_service_null_methods jxta_membership_service_null_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_membership_service_null_init,
      start,
      stop},
     "Jxta_service_methods",
     get_mia,
     get_interface,
     service_on_option_set},
    "Jxta_membership_service_methods",
    apply,
    join,
    resign,
    currentcreds,
    makecred
};

void jxta_membership_service_null_construct(Jxta_membership_service_null * self, Jxta_membership_service_null_methods * methods)
{
    /*
     * we do not extend Jxta_Membership_service_methods; so the type string
     * is that of the base table
     */
    PTValid(methods, Jxta_membership_service_methods);

    jxta_membership_service_construct((Jxta_membership_service *) self, (Jxta_membership_service_methods *) methods);

    /* Set our rt type checking string */
    self->thisType = "Jxta_membership_service_null";
}

void jxta_membership_service_null_destruct(Jxta_membership_service_null * self)
{
    PTValid(self, Jxta_membership_service_null);

    /* release/free/destroy our own stuff */
    if (NULL != self->creds) {
        JXTA_OBJECT_RELEASE(self->creds);
    }

    self->group = NULL;

    if (self->impl_adv) {
        JXTA_OBJECT_RELEASE(self->impl_adv);
    }

    if (self->assigned_id) {
        JXTA_OBJECT_RELEASE(self->assigned_id);
    }

    /* call the base classe's dtor. */
    jxta_membership_service_destruct((Jxta_membership_service *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Destruction finished\n");
}

/**
 * Frees an instance of the Membership Service.
 * 
 * @param service a pointer to the instance of the Membership Service to free.
 */
static void membership_free(Jxta_object * service)
{
    /* call the hierarchy of dtors */
    jxta_membership_service_null_destruct((Jxta_membership_service_null *) service);

    /* free the object itself */
    free(service);
}


/**
 * Creates a new instance of the Membership Service. by default the Service
 * is not initialized. the service must be initialized by 
 * a call to jxta_Membership_service_init().
 *
 * @return a non initialized instance of the Membership Service
 */

Jxta_membership_service_null *jxta_membership_service_null_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_membership_service_null *self = (Jxta_membership_service_null *)
        malloc(sizeof(Jxta_membership_service_null));

    if (self == NULL) {
        return NULL;
    }

    /* Initialize the object */
    memset(self, 0, sizeof(Jxta_membership_service_null));
    JXTA_OBJECT_INIT(self, membership_free, 0);

    /* call the hierarchy of ctors */
    jxta_membership_service_null_construct(self, &jxta_membership_service_null_methods);

    /* return the new object */
    return self;
}

/* END OF STANDARD SERVICE IMPLEMENTATION CODE */

/* vi: set ts=4 sw=4 et tw=130: */
