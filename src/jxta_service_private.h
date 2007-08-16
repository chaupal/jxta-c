/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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

#ifndef JXTA_SERVICE_PRIVATE_H
#define JXTA_SERVICE_PRIVATE_H

#include "jxta_errno.h"
#include "jxta_service.h"
#include "jxta_module_private.h"

/*
 * This is an internal header file that defines the interface between
 * the service API and the various implementations.
 * An implementation of service must define a static table as below
 * or a derivate of it.
 */

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * The methods that a Jxta_service must implement.
 * All Jxta_service and derivates have a pointer to such an
 * object or a derivate.
 *
 * Objects of this type are meant to be defined statically, no
 * initializer function is provided.
 *
 * Do not forget to begin the initializer block with a complete initializer
 * block for Jxta_module_methods followed by the literal string
 * "Jxta_service_methods".
 * For example:
 * <pre>
 * Jxta_service_methods myMethods = {
 *   {
 *      "Jxta_module_methods",
 *      my_init,
 *      my_init_e,
 *      my_start,
 *      my_stop
 *   },
 *   "Jxta_service_methods",
 *   jxta_service_get_MIA_impl,
 *   jxta_service_get_interface_impl
 * }
 * </pre>
 **/
typedef struct jxta_service_methods {
    Extends(Jxta_module_methods);

    /* An implementation of Jxta_service_get_MIA */
    void (*get_MIA) (Jxta_service * svc, Jxta_advertisement ** mia);

    /* An implementation of Jxta_service_get_interface */
    void (*get_interface) (Jxta_service * svc, Jxta_service ** service);

    /* option change */
     Jxta_status(*on_option_set) (Jxta_service * me, const char *ns, const char *key, const void *old_val, const void *new_val);
} Jxta_service_methods;

typedef struct svc_cb {
    struct svc_cb *next;
    Jxta_callback_fn f;
    void *arg;
} Svc_cb;

/**
 * A Jxta_service is a Jxta_module plus a thisType field of "Jxta_service".
 * The set of methods is assigned to the base Jxta_module.
 * The set of methods is a superset of the set of methods of Jxta_module.
 * After allocating such an object, one must apply JXTA_OBJECT_INIT() and
 * jxta_service_construct to it.
 */
struct jxta_service {
    Extends(_jxta_module);

    /**
    *    Group context for this service instance.
    **/
    Jxta_PG *group;

    /**
    *   Assigned ID of this service instance.
    **/
    Jxta_id *assigned_id;

    /**
    *   Impl advertisement for this instance.
    **/
    Jxta_advertisement *impl_adv;

    Svc_cb *event_cbs;
    apr_thread_mutex_t *mutex;
    /* a map from namespace to the hashtable of options, which contains key/value pairs */
    apr_hash_t *opt_tables;
    const Jxta_qos *qos_for_all;
};

/**
 * The base service ctor (not public: the only public way to make a new service
 * is to instantiate one of the derived types). Derivates call this
 * to initialize the base part. This routine calls jxta_module_construct.
 *
 * @param svc The Jxta_service object to construct.
 * @param methods Pointer to the relevant Jxta_service_methods object.
 * @return the constructed object or NULL if the construction failed.
 */
extern Jxta_service *jxta_service_construct(Jxta_service * svc, Jxta_service_methods const *methods);

/**
 * The base service dtor (Not public, not virtual. Only called by subclassers).
 *
 * @param svc The Jxta_service object to destruct.
 */
extern void jxta_service_destruct(Jxta_service * svc);

/**
*   Initialize the service.
*
*   @param svc The service instance being initialized.
*   @param group The peer group context for the service instance.
*   @param assigned_id  The id assigned to this service instance.
*   @param impl_adv The implementation advertisement for this service isntance, if any.S
*   @return The status result.
**/
extern Jxta_status jxta_service_init(Jxta_service * svc, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);

/**
 *  Return this service's interface object
 *
 *  @param service return this service's interface object
 *  @return this service's interface object/
 **/
extern void jxta_service_get_interface_impl(Jxta_service * svc, Jxta_service ** intf);

/**
 *  Return the peer group object associated with this service.
 *
 *  @return the peer group object associated with this service.
 **/
extern void jxta_service_get_peergroup_impl(Jxta_service * svc, Jxta_PG ** pg);

/**
 *  Return the id associated with this service.
 *
 *  @return the id associated with this service.
 **/
extern void jxta_service_get_assigned_ID_impl(Jxta_service * svc, Jxta_id ** assignedID);

/**
 *  Return the module implementation advertisement for this service
 *
 *  @return the module implementation advertisement for this service
 **/
extern void jxta_service_get_MIA_impl(Jxta_service * svc, Jxta_advertisement ** mia);

/**
 *  Gets the peer group object associated with this service without sharing it, dispatching or checking the object type.
 *    
 *  @return the peergroup object
**/
extern Jxta_PG *jxta_service_get_peergroup_priv(Jxta_service * svc);

/**
 *  Gets the assigned id associated with this service without sharing it, dispatching or checking the object type.
 *    
 *  @return the assigned id object
**/
extern Jxta_id *jxta_service_get_assigned_ID_priv(Jxta_service * svc);

/**
 *  Gets the MIA associated with this service without sharing it, dispatching or checking the object type.
 *    
 *  @return the MIA
**/
extern Jxta_advertisement *jxta_service_get_MIA_priv(Jxta_service * svc);

extern Jxta_status service_on_option_set(Jxta_service * svc, const char *ns, const char *key, const void *old_val,
                                         const void *new_val);

#define JXTA_SERVICE_VTBL(self) ((Jxta_service_methods*)(((_jxta_module*) (self))->methods))

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_SERVICE_PRIVATE_H */

/* vim: set ts=4 sw=4 et tw=130: */
