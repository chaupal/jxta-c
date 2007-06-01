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
 * $Id: jxta_peergroup_private.h,v 1.26 2006/09/08 20:56:01 exocetrick Exp $
 */

#ifndef JXTA_PEERGROUP_PRIVATE_H
#define JXTA_PEERGROUP_PRIVATE_H

#include "jpr/jpr_excep_proto.h"
#include "jxta_peergroup.h"
#include "jxta_service_private.h"
#include "jxta_rdv.h"

/*
 * This is an internal header file that defines the interface between
 * the peer group API and the various implementations.
 * An implementation of peer group must define a static Table as below
 * or a derivate of it.
 */


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * A Jxta_PG is a Jxta_service plus a thisType field of "Jxta_peergroup".
 * The set of methods is assigned to the base Jxta_module.
 * The set of methods is a superset of the set of methods of Jxta_service.
 * After allocating such an object, one must apply JXTA_OBJECT_INIT() and
 * jxta_PG_construct to it.
 */
struct jxta_PG {
    Extends(Jxta_service);

    /* Implementors put their stuff after that */
    struct jxta_PG *parent;
    apr_pool_t *pool;
    apr_thread_pool_t *thd_pool;
    char *ep_name;
    void *ep_cookie;
};

/**
 * The set of methods that a Jxta_peergroup must implement.
 * All Jxta_peergroup and derivates have a pointer to such an
 * object or a derivate.
 *
 * Objects of this type are meant to be defined statically, no
 * initializer function is provided.
 *
 * Do not forget to begin the initializer block with a complete initializer
 * block for Jxta_service_methods followed by the litteral string
 * "Jxta_PG_methods".
 * For example:
 *
 * Jxta_peergroup_methods myMethods = {
 *   {
 *     {
 *        "Jxta_module_methods",
 *        my_init,
 *        my_init_e,
 *        my_start,
 *        my_stop
 *     },
 *     "Jxta_service_methods",
 *     my_get_MIA,
 *     my_get_interface
 *   },
 *   "Jxta_PG_methods",
 *   my_get_loader,
 *   my_get_PGA,
 *   ...
 * }
 */
typedef struct _jxta_PG_methods Jxta_PG_methods;

struct _jxta_PG_methods {
    Extends(Jxta_service_methods);

    void (*get_loader) (Jxta_PG * self, Jxta_loader ** loader);
    void (*get_PGA) (Jxta_PG * self, Jxta_PGA ** pga);
    void (*get_PA) (Jxta_PG * self, Jxta_PA ** pa);
     Jxta_status(*lookup_service) (Jxta_PG * self, Jxta_id * name, Jxta_service ** result);
     Jxta_boolean(*is_compatible) (Jxta_PG * self, JString * compat);
     Jxta_status(*loadfromimpl_module) (Jxta_PG * self, Jxta_id * assigned_id, Jxta_advertisement * impl, Jxta_module ** result);
     Jxta_status(*loadfromid_module) (Jxta_PG * self,
                                      Jxta_id * assigned_id, Jxta_MSID * spec_id, int where, Jxta_module ** result);
    void (*set_labels) (Jxta_PG * self, JString * name, JString * description);
     Jxta_status(*newfromadv) (Jxta_PG * self, Jxta_advertisement * pgAdv, Jxta_vector * resource_groups, Jxta_PG ** result);
     Jxta_status(*newfromimpl) (Jxta_PG * self, Jxta_PGID * gid,
                                Jxta_advertisement * impl, JString * name,
                                JString * description, Jxta_vector * resource_groups, Jxta_PG ** result);
     Jxta_status(*newfromid) (Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_groups, Jxta_PG ** result);
    void (*get_rendezvous_service) (Jxta_PG * self, Jxta_rdv_service ** rdv);
    void (*get_endpoint_service) (Jxta_PG * self, Jxta_endpoint_service ** endp);
    void (*get_resolver_service) (Jxta_PG * self, Jxta_resolver_service ** resolver);
    void (*get_discovery_service) (Jxta_PG * self, Jxta_discovery_service ** disco);
    void (*get_peerinfo_service) (Jxta_PG * self, Jxta_peerinfo_service ** peerinfo);
    void (*get_membership_service) (Jxta_PG * self, Jxta_membership_service ** memb);
    void (*get_pipe_service) (Jxta_PG * self, Jxta_pipe_service ** pipe);
    void (*get_GID) (Jxta_PG * self, Jxta_PGID ** gid);
    void (*get_PID) (Jxta_PG * self, Jxta_PID ** pid);
    void (*get_groupname) (Jxta_PG * self, JString ** nm);
    void (*get_peername) (Jxta_PG * self, JString ** nm);
    void (*get_configadv) (Jxta_PG * self, Jxta_PA ** adv);
    void (*get_genericpeergroupMIA) (Jxta_PG * self, Jxta_MIA ** mia);
    void (*set_resourcegroups) (Jxta_PG * self, Jxta_vector * resource_groups);
    void (*get_resourcegroups) (Jxta_PG * self, Jxta_vector ** resource_groups);
    void (*get_parentgroup) (Jxta_PG * self, Jxta_PG ** parent_group);
    void (*get_compatstatement) (Jxta_PG * self, JString ** compat);
     Jxta_status(*add_relay_address) (Jxta_PG * self, Jxta_RdvAdvertisement * relay);
     Jxta_status(*remove_relay_address) (Jxta_PG * self, Jxta_id * relayid);
    void (*get_srdi_service) (Jxta_PG * self, Jxta_srdi_service ** srdi);
    void (*set_cache_manager) (Jxta_PG * self, Jxta_cm * cm);
    void (*get_cache_manager) (Jxta_PG * self, Jxta_cm ** cm);
};

/**
 * The base PG ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types). Derivates call this
 * to initialize the base part. This routine calls Jxta_service_construct.
 *
 * @param self The Jxta_PG_object to initialize.
 * @param methods A pointer to the relevant set of methods.
 * @deprecated This API will be eliminated
 */
void jxta_PG_construct(Jxta_PG * self, Jxta_PG_methods * methods);

Jxta_status peergroup_init(Jxta_PG * me, Jxta_PG * parent);
Jxta_status peergroup_start(Jxta_PG * me);
Jxta_status peergroup_stop(Jxta_PG * me);


/**
 * get the cache manager for this group
 *
 * @param self Peer group object
 * @param cm Location to store the cache manager
*/
void peergroup_get_cache_manager(Jxta_PG * self, Jxta_cm ** cm);

/**
 * set the cache manager for this group
 *
 * @param self Peer group object
 * @param cm Cache manager
*/
void peergroup_set_cache_manager(Jxta_PG * self, Jxta_cm * cm);

/**
 * The base PG dtor (Not public, not virtual. Only called by subclassers).
 */
void jxta_PG_destruct(Jxta_PG * self);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_PEERGROUP_PRIVATE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
