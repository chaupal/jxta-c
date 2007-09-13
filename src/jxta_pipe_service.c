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

static const char *__log_cat = "PIPE";

#include "jxta_apr.h"
#include "jpr/jpr_excep_proto.h"

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jdlist.h"
#include "jxta_id.h"
#include "jxta_service.h"
#include "jxta_peergroup.h"
#include "jxta_service_private.h"
#include "jxta_pipe_service.h"

struct _jxta_pipe_service {
    Extends(Jxta_service);

    Jxta_PG *group;
    Dlist *impls;
    Jxta_id *assigned_id;
    Jxta_advertisement *impl_adv;
    Jxta_pipe_resolver *pipe_resolver;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
};


struct _jxta_pipe_connect_event {
    JXTA_OBJECT_HANDLE;
    int event;
    Jxta_pipe *pipe;
} _jxta_pipe_connect_event;


struct _jxta_inputpipe_event {
    JXTA_OBJECT_HANDLE;
    int event;
    Jxta_object *arg;
};

struct _jxta_outputpipe_event {
    JXTA_OBJECT_HANDLE;
    int event;
    Jxta_object *arg;
};

static Jxta_boolean add_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl);
static Jxta_boolean remove_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl);
static Jxta_pipe_service_impl *get_impl(Jxta_pipe_service * self, const char *name);
extern Jxta_pipe_service_impl *jxta_wire_service_new_instance(Jxta_pipe_service *, Jxta_PG *);
extern Jxta_pipe_service_impl *jxta_unipipe_service_new_instance(Jxta_pipe_service *, Jxta_PG *);
extern Jxta_pipe_service_impl *jxta_securepipe_service_new_instance(Jxta_pipe_service * pipe_service, Jxta_PG * group);
extern Jxta_pipe_resolver *jxta_piperesolver_new(Jxta_PG * group);

static Jxta_status
pipe_service_timed_connect(Jxta_pipe_service * self,
                           Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_timed_connect(impl, adv, timeout, peers, pipe);
}

static Jxta_status
pipe_service_connect(Jxta_pipe_service * self,
                     Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_connect(impl, adv, timeout, peers, listener);
}

static Jxta_status pipe_service_timed_accept(Jxta_pipe_service * self, Jxta_pipe_adv * adv, Jxta_time_diff timeout,
                                             Jxta_pipe ** pipe)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_timed_accept(impl, adv, timeout, pipe);
}

static Jxta_status pipe_service_deny(Jxta_pipe_service * self, Jxta_pipe_adv * adv)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_deny(impl, adv);
}


static Jxta_status pipe_service_add_accept_listener(Jxta_pipe_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_add_accept_listener(impl, adv, listener);
}

static Jxta_status pipe_service_remove_accept_listener(Jxta_pipe_service * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{
    Jxta_pipe_service_impl *impl = NULL;

    impl = get_impl_by_adv(self, adv);
    if (impl == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    return jxta_pipe_service_impl_remove_accept_listener(impl, adv, listener);
}


static Jxta_status pipe_service_lookup_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl ** impl)
{
    Jxta_pipe_service_impl *tmp = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Pipe Service lookup for [%s]\n", name);
    apr_thread_mutex_lock(self->mutex);

    tmp = get_impl(self, name);
    if (tmp != NULL) {
        JXTA_OBJECT_SHARE(tmp);
        *impl = tmp;
    }
    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}


static Jxta_status pipe_service_add_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    Jxta_pipe_service_impl *tmp = NULL;
    Jxta_status res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "add_impl [%s]\n", name);
    apr_thread_mutex_lock(self->mutex);

    tmp = get_impl(self, name);
    if (tmp != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Existing impl already for [%s]\n", name);
        res = JXTA_INVALID_ARGUMENT;
    } else {
        if (add_impl(self, name, impl)) {
            JXTA_OBJECT_SHARE(impl);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "[%s] is added\n", name);
            res = JXTA_SUCCESS;
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot add [%s]\n", name);
            res = JXTA_BUSY;
        }
    }
    apr_thread_mutex_unlock(self->mutex);

    return res;
}


static Jxta_status pipe_service_remove_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    Jxta_pipe_service_impl *tmp = NULL;
    Jxta_status res;

    apr_thread_mutex_lock(self->mutex);

    tmp = get_impl(self, name);
    if (tmp == NULL) {
        res = JXTA_INVALID_ARGUMENT;
    } else {
        if (remove_impl(self, name, impl)) {
            JXTA_OBJECT_RELEASE(impl);
            res = JXTA_SUCCESS;
        } else {
            res = JXTA_VIOLATION;
        }
    }
    apr_thread_mutex_unlock(self->mutex);

    return res;
}


static Jxta_status pipe_service_get_resolver(Jxta_pipe_service * self, Jxta_pipe_resolver ** resolver)
{
    if (self->pipe_resolver == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    JXTA_OBJECT_SHARE(self->pipe_resolver);
    *resolver = self->pipe_resolver;

    return JXTA_SUCCESS;
}


static Jxta_status pipe_service_set_resolver(Jxta_pipe_service * self, Jxta_pipe_resolver * new_resolver,
                                             Jxta_pipe_resolver ** old)
{
    return JXTA_NOTIMP;
}

Jxta_pipe_service_impl *get_impl_by_adv(Jxta_pipe_service * self, Jxta_pipe_adv * adv)
{
    const char *name = NULL;

    if (adv == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Advertisement is NULL\n");
        return NULL;
    }

    name = jxta_pipe_adv_get_Type(adv);
    if (name == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "The advertisement does not have a type\n");
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Get Pipe implementation for [%s]\n", name);

    return get_impl(self, name);
}

/**
 ** This implementation relies on Dlist, since it is expected that
 ** the number of pipe implementation to be very small. Initially 2 or 3,
 ** and probably will always be less than 10.
 **/

static Jxta_pipe_service_impl *get_impl(Jxta_pipe_service * self, const char *name)
{
    Dlist *pt = NULL;
    Jxta_pipe_service_impl *impl = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "get_impl for [%s]\n", name);
    apr_thread_mutex_lock(self->mutex);
    if (NULL == self->impls) {
        apr_thread_mutex_unlock(self->mutex);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        "No implementation available, most likely the pipe_service is not started yet!\n");
        return NULL;
    }

    pt = dl_first(self->impls);
    while (pt != self->impls) {
        impl = (Jxta_pipe_service_impl *) pt->val;
        if (impl == NULL) {
            pt = dl_next(pt);
            continue;
        }

        if (strcmp(jxta_pipe_service_impl_get_name(impl), name) == 0) {
            /* This is the good service */
            apr_thread_mutex_unlock(self->mutex);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Found it\n");
            return impl;
        }
        pt = dl_next(pt);
    }
    apr_thread_mutex_unlock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Not found\n");
    return NULL;
}

static Jxta_boolean add_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    apr_thread_mutex_lock(self->mutex);
    if (get_impl(self, name) != NULL) {
        /* there already is an implementation for this name */
        apr_thread_mutex_unlock(self->mutex);
        return FALSE;
    }
    dl_insert_b(self->impls, (void *) impl);

    apr_thread_mutex_unlock(self->mutex);
    return TRUE;
}


static Jxta_boolean remove_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    Dlist *pt = NULL;
    Jxta_pipe_service_impl *tmp = NULL;

    apr_thread_mutex_lock(self->mutex);

    pt = dl_first(self->impls);
    while (pt != self->impls) {
        tmp = (Jxta_pipe_service_impl *) pt->val;
        if (tmp == NULL) {
            pt = dl_next(pt);
            continue;
        }
        if (strcmp(jxta_pipe_service_impl_get_name(tmp), name) == 0) {
            if (tmp == impl) {
                /* This is the good service */
                dl_delete_node(pt);

                apr_thread_mutex_unlock(self->mutex);
                return TRUE;
            }
        }
        pt = dl_next(pt);
    }
    apr_thread_mutex_unlock(self->mutex);
    return FALSE;
}


/**
 **
 ** Jxta_service/module wrapper.
 **
 **/


typedef struct {

    Extends(Jxta_service_methods);

    Jxta_status(*timed_connect) (Jxta_pipe_service * service,
                                 Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe);

    Jxta_status(*connect) (Jxta_pipe_service * service,
                           Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener);

    Jxta_status(*timed_accept) (Jxta_pipe_service * service, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe);


    Jxta_status(*deny) (Jxta_pipe_service * service, Jxta_pipe_adv * adv);


    Jxta_status(*add_accept_listener) (Jxta_pipe_service * service, Jxta_pipe_adv * adv, Jxta_listener * listener);

    Jxta_status(*remove_accept_listener) (Jxta_pipe_service * service, Jxta_pipe_adv * adv, Jxta_listener * listener);

    Jxta_status(*lookup_impl) (Jxta_pipe_service * service, const char *type_name, Jxta_pipe_service_impl ** impl);

    Jxta_status(*add_impl) (Jxta_pipe_service * service, const char *name, Jxta_pipe_service_impl * impl);

    Jxta_status(*remove_impl) (Jxta_pipe_service * service, const char *name, Jxta_pipe_service_impl * impl);

    Jxta_status(*get_resolver) (Jxta_pipe_service * service, Jxta_pipe_resolver ** resolver);

    Jxta_status(*set_resolver) (Jxta_pipe_service * service, Jxta_pipe_resolver * new_resolver, Jxta_pipe_resolver ** old);

} _jxta_pipe_service_methods;


/**
 * Easy access to the vtbl.
 */
#define VTBL(x) ((_jxta_pipe_service_methods*) JXTA_MODULE_VTBL(x))


/*
 * This implementation's methods.
 * The typedef could go in jxta_pipe_service_private.h if we wanted
 * to support subclassing of this class. We don't have to and so the
 * alias Jxta_pipe_service_methods -> Jxta_pipe_service_methods
 * is purely academic, but that'll be less work to do later if someone
 * wants to subclass.
 */

static Jxta_status jxta_pipe_service_init(Jxta_module * module,
                                          Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);
static Jxta_status start(Jxta_module * self, const char *argv[]);
static void stop(Jxta_module * module);

const _jxta_pipe_service_methods jxta_pipe_service_methods = {
    {
     {
      "Jxta_module_methods",
      jxta_pipe_service_init,   /* long name temporarily; it is exported for testing purposes */
      start,
      stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl,
     service_on_option_set},
    "Jxta_pipe_service_methods",
    pipe_service_timed_connect,
    pipe_service_connect,
    pipe_service_timed_accept,
    pipe_service_deny,
    pipe_service_add_accept_listener,
    pipe_service_remove_accept_listener,
    pipe_service_lookup_impl,
    pipe_service_add_impl,
    pipe_service_remove_impl,
    pipe_service_get_resolver,
    pipe_service_set_resolver
};


static void pipe_service_free(Jxta_object * obj)
{
    Jxta_pipe_service *self = PTValid(obj, Jxta_pipe_service);

    self->group = NULL;

    if (self->assigned_id != NULL) {
        JXTA_OBJECT_RELEASE(self->assigned_id);
        self->assigned_id = NULL;
    }
    if (self->impl_adv != NULL) {
        JXTA_OBJECT_RELEASE(self->impl_adv);
        self->impl_adv = NULL;
    }
    if (self->pool != NULL) {
        apr_thread_mutex_destroy(self->mutex);
        apr_pool_destroy(self->pool);
        self->pool = NULL;
        self->mutex = NULL;
    }

    /* call the base classe's dtor. */
    jxta_service_destruct((Jxta_service *) self);

    free(self);
}


Jxta_pipe_service *jxta_pipe_service_new_instance(void)
{
    /* Allocate an instance of this service */
    Jxta_pipe_service *self = (Jxta_pipe_service *) calloc(1, sizeof(Jxta_pipe_service));

    if (NULL != self) {
        /* Initialize the object */
        JXTA_OBJECT_INIT(self, pipe_service_free, 0);

        /* call the hierarchy of ctors. */
        jxta_service_construct((Jxta_service *) self, (Jxta_service_methods const *) &jxta_pipe_service_methods);

        self->thisType = "Jxta_pipe_service";

        self->group = NULL;
        self->impls = NULL;
        self->assigned_id = NULL;
        self->impl_adv = NULL;
        self->pipe_resolver = NULL;
        self->mutex = NULL;
        self->pool = NULL;
    }

    /* Return the new object */
    return self;
}

static Jxta_status jxta_pipe_service_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id,
                                          Jxta_advertisement * impl_adv)
{

    Jxta_pipe_service *self = (Jxta_pipe_service *) module;
    apr_status_t res;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Pipe Service init\n");
    /* Test arguments first */
    if ((module == NULL) || (group == NULL)) {
        /* Invalid args. */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }


    PTValid(self, Jxta_pipe_service);

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,    /* nested */
                                  self->pool);

    /* store our assigned id. */
    if (assigned_id != 0) {

        JXTA_OBJECT_SHARE(assigned_id);
        self->assigned_id = assigned_id;
    }

    /* impl_adv are jxta_objects that we share */

    if (impl_adv != 0) {
        JXTA_OBJECT_SHARE(impl_adv);
    }
    self->group = group;
    self->impl_adv = impl_adv;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Init done\n");
    return JXTA_SUCCESS;
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
    Jxta_status res;
    Jxta_pipe_resolver *pipe_resolver = NULL;
    Jxta_pipe_service *self = (Jxta_pipe_service *) module;

    apr_thread_mutex_lock(self->mutex);

    /**
     ** Create the default pipe resolver.
     **/
    pipe_resolver = jxta_piperesolver_new(self->group);
    if (pipe_resolver == NULL) {
      jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot allocate Pipe Resolver\n");
    } else {
      self->pipe_resolver = pipe_resolver;
    }

   self->impls = dl_make();

    /**
     ** Creates a wire_service instance
     **/
    {
        Jxta_pipe_service_impl *impl = NULL;

        impl = jxta_wire_service_new_instance(self, self->group);
        res = jxta_pipe_service_add_impl(self, jxta_pipe_service_impl_get_name(impl), impl);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot insert pipe impl [%s] into pipe service.\n",
                            jxta_pipe_service_impl_get_name(impl));
        }
        JXTA_OBJECT_RELEASE(impl);
    }

    /**
     ** Creates a unipipe_service instance
     **/
    {
        Jxta_pipe_service_impl *impl = NULL;

        impl = jxta_unipipe_service_new_instance(self, self->group);
        res = jxta_pipe_service_add_impl(self, jxta_pipe_service_impl_get_name(impl), impl);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot insert pipe impl [%s] into pipe service.\n",
                            jxta_pipe_service_impl_get_name(impl));
        }

        JXTA_OBJECT_RELEASE(impl);
    }

    /**
     ** Creates a secure_pipe_service instance
     **/
    {
        Jxta_pipe_service_impl *impl = NULL;

        impl = jxta_securepipe_service_new_instance(self, self->group);
        res = jxta_pipe_service_add_impl(self, jxta_pipe_service_impl_get_name(impl), impl);
        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot insert pipe impl [%s] into pipe service.\n",
                            jxta_pipe_service_impl_get_name(impl));
        }

        JXTA_OBJECT_RELEASE(impl);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started.\n");
    apr_thread_mutex_unlock(self->mutex);
    return JXTA_SUCCESS;
}


/**
 * Stops an instance of the Pipe Service. (a module method)
 * Note that the caller still needs to release the service object
 * in order to really free the instance.
 * 
 * @param module a pointer to the instance of the Rendezvous Service
 * @return error code.
 **/
static void stop(Jxta_module * module)
{

    Jxta_pipe_service *self = (Jxta_pipe_service *) module;

    PTValid(self, Jxta_pipe_service);

    /* Test arguments first */
    if (self == NULL) {
        /* Invalid args. */
        return;
    }

    apr_thread_mutex_lock(self->mutex);

    if (self->impls != NULL) {
        dl_free(self->impls, NULL);
        self->impls = NULL;
    }

    if (self->pipe_resolver != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe_resolver);
        self->pipe_resolver = NULL;
    }

    apr_thread_mutex_unlock(self->mutex);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
    return;
}


/**
 ** Implementation of the Jxta_pipe_service API
 **/


JXTA_DECLARE(Jxta_status)
    jxta_pipe_service_timed_connect(Jxta_pipe_service * self,
                                Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->timed_connect(self, adv, timeout, peers, pipe);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_service_deny(Jxta_pipe_service * self, Jxta_pipe_adv * adv)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->deny(self, adv);
}


JXTA_DECLARE(Jxta_status)
    jxta_pipe_service_connect(Jxta_pipe_service * self,
                          Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->connect(self, adv, timeout, peers, listener);
}


JXTA_DECLARE(Jxta_status) jxta_pipe_service_timed_accept(Jxta_pipe_service * self, Jxta_pipe_adv * adv, Jxta_time_diff timeout,
                                                         Jxta_pipe ** pipe)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->timed_accept(self, adv, timeout, pipe);
}


JXTA_DECLARE(Jxta_status) jxta_pipe_service_add_accept_listener(Jxta_pipe_service * self, Jxta_pipe_adv * adv,
                                                                Jxta_listener * listener)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->add_accept_listener(self, adv, listener);
}



JXTA_DECLARE(Jxta_status) jxta_pipe_service_remove_accept_listener(Jxta_pipe_service * self, Jxta_pipe_adv * adv,
                                                                   Jxta_listener * listener)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->remove_accept_listener(self, adv, listener);
}



JXTA_DECLARE(Jxta_status) jxta_pipe_service_get_resolver(Jxta_pipe_service * self, Jxta_pipe_resolver ** resolver)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->get_resolver(self, resolver);
}


JXTA_DECLARE(Jxta_status) jxta_pipe_service_set_resolver(Jxta_pipe_service * self,
                                                         Jxta_pipe_resolver * new_resolver, Jxta_pipe_resolver ** old)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->set_resolver(self, new_resolver, old);
}


JXTA_DECLARE(Jxta_status) jxta_pipe_service_lookup_impl(Jxta_pipe_service * self, const char *name,
                                                        Jxta_pipe_service_impl ** impl)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->lookup_impl(self, name, impl);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_service_add_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->add_impl(self, name, impl);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_service_remove_impl(Jxta_pipe_service * self, const char *name, Jxta_pipe_service_impl * impl)
{
    PTValid(self, Jxta_pipe_service);

    return VTBL(self)->remove_impl(self, name, impl);
}



/**
 ** Implementation of the Jxta_pipe_service_impl API
 **/

char *jxta_pipe_service_impl_get_name(Jxta_pipe_service_impl * self)
{
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return NULL;
    }

    return self->get_name(self);
}


Jxta_status
jxta_pipe_service_impl_timed_connect(Jxta_pipe_service_impl * self,
                                     Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_pipe ** pipe)
{
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->timed_connect(self, adv, timeout, peers, pipe);
}

Jxta_status jxta_pipe_service_impl_deny(Jxta_pipe_service_impl * self, Jxta_pipe_adv * adv)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->deny(self, adv);
}


Jxta_status
jxta_pipe_service_impl_connect(Jxta_pipe_service_impl * self,
                               Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_vector * peers, Jxta_listener * listener)
{


    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->connect(self, adv, timeout, peers, listener);
}


Jxta_status
jxta_pipe_service_impl_timed_accept(Jxta_pipe_service_impl * self, Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_pipe ** pipe)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->timed_accept(self, adv, timeout, pipe);
}


Jxta_status
jxta_pipe_service_impl_add_accept_listener(Jxta_pipe_service_impl * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->add_accept_listener(self, adv, listener);
}



Jxta_status
jxta_pipe_service_impl_remove_accept_listener(Jxta_pipe_service_impl * self, Jxta_pipe_adv * adv, Jxta_listener * listener)
{
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->remove_accept_listener(self, adv, listener);
}



Jxta_status jxta_pipe_service_impl_get_pipe_resolver(Jxta_pipe_service_impl * self, Jxta_pipe_resolver ** resolver)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->get_pipe_resolver(self, resolver);
}


Jxta_status
jxta_pipe_service_impl_set_pipe_resolver(Jxta_pipe_service_impl * self, Jxta_pipe_resolver * new_resolver,
                                         Jxta_pipe_resolver ** old)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }
    /**
     ** FIXME 3/3/2002 lomax@jxta.org
     ** Not implemented yet.
     **/
    return JXTA_NOTIMP;
}

Jxta_status 
jxta_pipe_service_impl_check_listener(Jxta_pipe_service_impl * self, const char * id)
{
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->check_listener(self, id);
}

/**
 ** Implementation of the Jxta_pipe API
 **/

JXTA_DECLARE(Jxta_status) jxta_pipe_get_outputpipe(Jxta_pipe * self, Jxta_outputpipe ** op)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->get_outputpipe(self, op);
}


JXTA_DECLARE(Jxta_status) jxta_pipe_get_inputpipe(Jxta_pipe * self, Jxta_inputpipe ** ip)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->get_inputpipe(self, ip);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_get_remote_peers(Jxta_pipe * self, Jxta_vector ** vector)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->get_remote_peers(self, vector);
}

JXTA_DECLARE(Jxta_status) jxta_pipe_get_pipeadv(Jxta_pipe * self, Jxta_pipe_adv ** pipeadv)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->get_pipeadv(self, pipeadv);
}


/**
 ** Implementation of the Jxta_inputpipe API
 **/

JXTA_DECLARE(Jxta_status) jxta_inputpipe_timed_receive(Jxta_inputpipe * self, Jxta_time_diff timeout, Jxta_message ** msg)
{
    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->timed_receive(self, timeout, msg);
}

JXTA_DECLARE(Jxta_status) jxta_inputpipe_add_listener(Jxta_inputpipe * self, Jxta_listener * listener)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->add_listener(self, listener);
}

JXTA_DECLARE(Jxta_status) jxta_inputpipe_remove_listener(Jxta_inputpipe * self, Jxta_listener * listener)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->remove_listener(self, listener);
}


/**
 ** Implementation of the Jxta_ouputpipe API
 **/


JXTA_DECLARE(Jxta_status) jxta_outputpipe_send(Jxta_outputpipe * self, Jxta_message * msg)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->send(self, msg);
}

JXTA_DECLARE(Jxta_status) jxta_outputpipe_add_listener(Jxta_outputpipe * self, Jxta_listener * listener)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->add_listener(self, listener);
}

JXTA_DECLARE(Jxta_status) jxta_outputpipe_remove_listener(Jxta_outputpipe * self, Jxta_listener * listener)
{

    if (self == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Invalid argument\n");
        return JXTA_INVALID_ARGUMENT;
    }

    return self->remove_listener(self, listener);
}


/**
 ** Implementation of the Jxta_pipe_connect_event API
 **/

static void jxta_pipe_connect_event_free(Jxta_object * obj)
{

    Jxta_pipe_connect_event *self = (Jxta_pipe_connect_event *) obj;

    if (self->pipe != NULL) {
        JXTA_OBJECT_RELEASE(self->pipe);
        self->pipe = NULL;
    }

    free((void *) self);
}


JXTA_DECLARE(Jxta_pipe_connect_event *) jxta_pipe_connect_event_new(int ev, Jxta_pipe * pipe)
{

    Jxta_pipe_connect_event *event = (Jxta_pipe_connect_event *) malloc(sizeof(Jxta_pipe_connect_event));
    memset(event, 0, sizeof(Jxta_pipe_connect_event));
    JXTA_OBJECT_INIT(event, jxta_pipe_connect_event_free, NULL);

    event->event = ev;
    if (pipe != NULL) {
        JXTA_OBJECT_SHARE(pipe);
        event->pipe = pipe;
    } else {
        event->pipe = NULL;
    }
    return event;
}

JXTA_DECLARE(int) jxta_pipe_connect_event_get_event(Jxta_pipe_connect_event * self)
{
    return self->event;
}

JXTA_DECLARE(Jxta_status) jxta_pipe_connect_event_get_pipe(Jxta_pipe_connect_event * self, Jxta_pipe ** pipe)
{

    if (self->pipe == NULL) {
        return JXTA_FAILED;
    }
    *pipe = self->pipe;

    JXTA_OBJECT_SHARE(*pipe);

    return JXTA_SUCCESS;
}


/**
 ** Implementation of the Jxta_inputpipe_event API
 **/

static void jxta_inputpipe_event_free(Jxta_object * obj)
{

    Jxta_inputpipe_event *self = (Jxta_inputpipe_event *) obj;

    if (self->arg != NULL) {
        JXTA_OBJECT_RELEASE(self->arg);
        self->arg = NULL;
    }

    free((void *) self);
}


JXTA_DECLARE(Jxta_inputpipe_event *) jxta_inputpipe_event_new(int ev, Jxta_object * arg)
{

    Jxta_inputpipe_event *event = (Jxta_inputpipe_event *) malloc(sizeof(Jxta_inputpipe_event));
    memset(event, 0, sizeof(Jxta_inputpipe_event));
    JXTA_OBJECT_INIT(event, jxta_inputpipe_event_free, NULL);

    event->event = ev;
    if (arg != NULL) {
        JXTA_OBJECT_SHARE(arg);
        event->arg = arg;
    } else {
        event->arg = NULL;
    }
    return event;
}

JXTA_DECLARE(int) jxta_inputpipe_event_get_event(Jxta_inputpipe_event * self)
{
    return self->event;
}

JXTA_DECLARE(Jxta_status) jxta_inputpipe_event_get_object(Jxta_inputpipe_event * self, Jxta_object ** arg)
{

    if (self->arg == NULL) {
        return JXTA_FAILED;
    }
    JXTA_OBJECT_SHARE(arg);
    *arg = self->arg;
    return JXTA_SUCCESS;
}

/**
 ** Implementation of the Jxta_outputpipe_event API
 **/

static void jxta_outputpipe_event_free(Jxta_object * obj)
{

    Jxta_outputpipe_event *self = (Jxta_outputpipe_event *) obj;

    if (self->arg != NULL) {
        JXTA_OBJECT_RELEASE(self->arg);
        self->arg = NULL;
    }

    free((void *) self);
}


JXTA_DECLARE(Jxta_outputpipe_event *) jxta_outputpipe_event_new(int ev, Jxta_object * arg)
{

    Jxta_outputpipe_event *event = (Jxta_outputpipe_event *) malloc(sizeof(Jxta_outputpipe_event));
    memset(event, 0, sizeof(Jxta_outputpipe_event));
    JXTA_OBJECT_INIT(event, jxta_outputpipe_event_free, NULL);

    event->event = ev;
    if (arg != NULL) {
        JXTA_OBJECT_SHARE(arg);
        event->arg = arg;
    } else {
        event->arg = NULL;
    }
    return event;
}

JXTA_DECLARE(int) jxta_outputpipe_event_get_event(Jxta_outputpipe_event * self)
{
    return self->event;
}

JXTA_DECLARE(Jxta_status) jxta_outputpipe_event_get_object(Jxta_outputpipe_event * self, Jxta_object ** arg)
{

    if (self->arg == NULL) {
        return JXTA_FAILED;
    }
    JXTA_OBJECT_SHARE(arg);
    *arg = self->arg;
    return JXTA_SUCCESS;
}



/**
 ** Implementation of the Jxta_pipe_resolver API
 **/

Jxta_status jxta_pipe_resolver_local_resolve(Jxta_pipe_resolver * self, Jxta_pipe_adv * adv, Jxta_vector ** peers)
{

    return self->local_resolve(self, adv, peers);
}

Jxta_status
jxta_pipe_resolver_timed_remote_resolve(Jxta_pipe_resolver * self,
                                        Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_vector ** peers)
{

    return self->timed_remote_resolve(self, adv, timeout, dest, peers);
}

Jxta_status jxta_pipe_resolver_send_srdi_message(Jxta_pipe_resolver * self,
                                                 Jxta_pipe_adv * adv, Jxta_id * dest, Jxta_boolean adding)
{

    return self->send_srdi(self, adv, dest, adding);
}


Jxta_status
jxta_pipe_resolver_remote_resolve(Jxta_pipe_resolver * self,
                                  Jxta_pipe_adv * adv, Jxta_time_diff timeout, Jxta_peer * dest, Jxta_listener * listener)
{

    return self->remote_resolve(self, adv, timeout, dest, listener);
}

/* vim: set ts=4 sw=4 tw=130 et: */
