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
 * $Id: jxta_peergroup.h,v 1.21 2007/04/24 21:51:33 slowhog Exp $
 */

#ifndef JXTA_PEERGROUP_H
#define JXTA_PEERGROUP_H

#include "jxta_service.h"

#include "jxta_id.h"

#include "jxta_mia.h"
#include "jxta_msa.h"
#include "jxta_mca.h"
#include "jxta_pa.h"
#include "jxta_pga.h"

#include "jxta_endpoint_service.h"
#include "jxta_membership_service.h"
#include "jxta_rdv_service.h"
#include "jxta_resolver_service.h"
#include "jxta_discovery_service.h"
#include "jxta_peerinfo_service.h"
#include "jxta_pipe_service.h"
#include "jxta_rdv.h"
#include "jxta_srdi.h"
#include "jxta_srdi_service.h"
#include "jxta_cm.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_loader Jxta_loader;

/*
 * End of temporary
 */

    /**
     * Look for needed ModuleImplAdvertisement in this group.
     */
#define JXTA_MIA_LOOKUP_HERE 0

    /**
     * Look for needed ModuleImplAdvertisement in the parent group of this group.
     */
#define JXTA_MIA_LOOKUP_PARENT 1

    /**
     * Look for needed ModuleImplAdvertisement in all the resource groups of this
     * group (may include its parent group).
     */
#define JXTA_MIA_LOOKUP_RESOURCE 2

    /**
     * Look for needed ModuleImplAdvertisement in all known possible places.
     */
#define JXTA_MIA_LOOKUP_ALL 3

/*
 * Functions to get the well known IDs
 */

JXTA_DECLARE(Jxta_MCID *) jxta_peergroup_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_resolver_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_discovery_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_srdi_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_cache_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_pipe_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_membership_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_rendezvous_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_peerinfo_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_endpoint_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_tcpproto_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_httpproto_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_routerproto_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_relayproto_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_application_classid_get(void);
JXTA_DECLARE(Jxta_MCID *) jxta_tlsproto_classid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_platform_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_netpeergroup_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_resolver_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_discovery_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_pipe_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_membership_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_rendezvous_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_peerinfo_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_endpoint_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_tcpProto_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_httpProto_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_routerProto_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_rlsProto_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_startnetpeergroup_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_ref_shell_specid_get(void);
JXTA_DECLARE(Jxta_MSID *) jxta_genericpeergroup_specid_get(void);

/*
 * Macros to lighten a bit the syntax required to get the well-known IDs.
 */
#define jxta_peergroup_classid            jxta_peergroup_classid_get           ()
#define jxta_resolver_classid             jxta_resolver_classid_get            ()
#define jxta_discovery_classid            jxta_discovery_classid_get           ()
#define jxta_srdi_classid                 jxta_srdi_classid_get                ()
#define jxta_cache_classid                jxta_cache_classid_get               ()
#define jxta_pipe_classid                 jxta_pipe_classid_get                ()
#define jxta_membership_classid           jxta_membership_classid_get          ()
#define jxta_rendezvous_classid           jxta_rendezvous_classid_get          ()
#define jxta_peerinfo_classid             jxta_peerinfo_classid_get            ()
#define jxta_endpoint_classid             jxta_endpoint_classid_get            ()
#define jxta_tcpproto_classid             jxta_tcpproto_classid_get            ()
#define jxta_tlsproto_classid             jxta_tlsproto_classid_get            ()
#define jxta_httpproto_classid            jxta_httpproto_classid_get           ()
#define jxta_routerproto_classid          jxta_routerproto_classid_get         ()
#define jxta_relayproto_classid           jxta_relayproto_classid_get          ()
#define jxta_application_classid          jxta_application_classid_get         ()
#define jxta_tlsproto_classid             jxta_tlsproto_classid_get            ()
#define jxta_ref_platform_specid          jxta_ref_platform_specid_get         ()
#define jxta_ref_netpeergroup_specid      jxta_ref_netpeergroup_specid_get     ()
#define jxta_ref_resolver_specid          jxta_ref_resolver_specid_get         ()
#define jxta_ref_discovery_specid         jxta_ref_discovery_specid_get        ()
#define jxta_ref_srdi_specid              jxta_ref_srdi_specid_get             ()
#define jxta_ref_pipe_specid              jxta_ref_pipe_specid_get             ()
#define jxta_ref_membership_specid        jxta_ref_membership_specid_get       ()
#define jxta_ref_rendezvous_specid        jxta_ref_rendezvous_specid_get       ()
#define jxta_ref_peerinfo_specid          jxta_ref_peerinfo_specid_get         ()
#define jxta_ref_endpoint_specid          jxta_ref_endpoint_specid_get         ()
#define jxta_ref_tcpproto_specid          jxta_ref_tcpproto_specid_get         ()
#define jxta_ref_httpproto_specid         jxta_ref_httpproto_specid_get        ()
#define jxta_ref_routerproto_specid       jxta_ref_routerproto_specid_get      ()
#define jxta_ref_tlsproto_specid          jxta_ref_tlsproto_specid_get         ()
#define jxta_ref_startnetpeergroup_specid jxta_ref_startnetpeergroup_specid_get()
#define jxta_ref_shell_specid             jxta_ref_shell_specid_get            ()
#define jxta_genericpeergroup_specid      jxta_genericpeergroup_specid_get     ()

#ifndef JXTA_PG_FORW
#define JXTA_PG_FORW
typedef struct jxta_PG Jxta_PG;
#endif

/**
 * Returns the loader for this group.
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param loader A location where to return (a ptr to) the loader.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_loader(Jxta_PG * self, Jxta_loader ** loader);


/**
 * Ask a group for its group advertisement
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param pga A location where to return (a ptr to) this Group's advertisement.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_PGA(Jxta_PG * self, Jxta_PGA ** pga);


/**
 * Ask a group for its peer advertisement on this peer.
 *
 * @param self handle of the group object to which the operation is applied.
 * 
 * @param pa A location where to return (a ptr to) this peerXgroup advertisement.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_PA(Jxta_PG * self, Jxta_PA ** pa);


/**
 * Lookup a service by name.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param name the service identifier.
 * @param result an address where to return a pointer to the found service.
 * The object returned is already shared and thus must be released after use.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_lookup_service(Jxta_PG * self, Jxta_id * name, Jxta_service ** result);

/**
 * Lookup for a service by name.
 * Throws to report errors. The object returned is already shared
 * and thus must be released after use.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param name the service identifier.
 * @param service A location where to return (a ptr to) the service registered by
 * that name. *service is not affected if an error is thrown.
 * @deprecated Please use jxta_PG_lookup_service() instead.
 */
JXTA_DECLARE(void) jxta_PG_lookup_service_e(Jxta_PG * self, Jxta_id * name, Jxta_service ** service, Throws);

/**
 * Evaluate if the given compatibility statement make the module
 * that bears it loadable by this group.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param compat (a ptr to) the compatibility statement against which to check.
 *
 * @return Jxta_boolean True if the statement is compatible.
 */
JXTA_DECLARE(Jxta_boolean) jxta_PG_is_compatible(Jxta_PG * self, JString * compat);


/**
 * Load a module from a ModuleImplAdv.
 * Compatibility is checked and load is attempted. If compatible and loaded
 * successfuly, the resulting Module is initialized and returned.
 * In most cases the other loadModule() method should be preferred, since
 * unlike this one, it will seek many compatible implementation
 * advertisements and try them all until one works. The home group of the new
 * module (its parent group if the new module is a group) will be this group.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param assigned_id ID to be assigned to that module (usually its ClassID).
 * @param impl An implementation advertisement for that module.
 * @param result An address where to return (a ptr to) the module loaded and 
 * initialized. The object returned is already shared and thus must be
 * released after use.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_loadfromimpl_module(Jxta_PG * self,
                                                      Jxta_id * assigned_id, Jxta_advertisement * impl, Jxta_module ** result);


/**
 * Load a module from a ModuleImplAdv.
 * Compatibility is checked and load is attempted. If compatible and loaded
 * successfuly, the resulting Module is initialized and returned.
 * In most cases the other loadModule() method should be preferred, since
 * unlike this one, it will seek many compatible implementation
 * advertisements and try them all until one works. The home group of the new
 * module (its parent group if the new module is a group) will be this group.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param assigned_id Id to be assigned to that module (usually its ClassID).
 * @param impl An implementation advertisement for that module.
 * @param module A location where to return (a ptr to) the resulting, initialized
 * but not started, module. The object returned is already shared and thus must be
 * released after use. If an error is thrown, *module is not affected.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_loadfromimpl_module_e(Jxta_PG * self,
                                                 Jxta_id * assigned_id, Jxta_advertisement * impl, Jxta_module ** module, Throws);


/**
 * Load a module from a spec id.
 * Advertisement is sought, compatibility is checked on all candidates
 * and load is attempted. The first one that is compatible and loads
 * successfuly is initialized and returned.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param assigned_id Id to be assigned to that module (usually its ClassID).
 * @param spec_id The specID of this module.
 * @param where May be one of: JXTA_MIA_LOOKUP_HERE, JXTA_MIA_LOOKUP_PARENT,
 * JXTA_MIA_LOOKUP_RESOURCE, or JXTA_MIA_LOOKUP_ALL, meaning that the
 * implementation advertisement will be searched in this group, its parent,
 * the groups listed in the resource groups list, or the union or all of these.
 * As a general guideline, the implementation advertisements of a group should be
 * searched in its propsective parent (generaly translates as HERE in code). The
 * implementation advertisements of a group standard services should be searched in
 * the same group than where this group's advertisement was found (that is, PARENT)
 * or in all the resource groups, likewise rendezvous advertisements. Applications
 * may be sought more freely (ALL).
 * @param result address where to return (a ptr to) the the new module. The
 * object returned is already shared and thus must be released after use.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_loadfromid_module(Jxta_PG * self,
                                                    Jxta_id * assigned_id, Jxta_MSID * spec_id, int where, Jxta_module ** result);


/**
 * Load a module from a spec id.
 * Advertisement is sought, compatibility is checked on all candidates
 * and load is attempted. The first one that is compatible and loads
 * successfuly is initialized and returned.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param assigned_id Id to be assigned to that module (usually its ClassID).
 * @param spec_id The specID of this module.
 * @param where May be one of: JXTA_MIA_LOOKUP_HERE, JXTA_MIA_LOOKUP_PARENT,
 * JXTA_MIA_LOOKUP_RESOURCE, or JXTA_MIA_LOOKUP_ALL, meaning that the
 * implementation advertisement will be searched in this group, its parent,
 * the groups listed in the resource groups list, or the union or all of these.
 * As a general guideline, the implementation advertisements of a group should be
 * searched in its propsective parent (generaly translates as HERE in code). The
 * implementation advertisements of a group standard services should be searched in
 * the same group than where this group's advertisement was found (that is, PARENT)
 * or in all the resource groups, likewise rendezvous advertisements. Applications
 * may be sought more freely (ALL).
 * @param module A location where to return (a ptr to) the the new module. The
 * object returned is already shared and thus must be released after use.
 * If an exception is thrown, *module is not affected.
 *
 * @return Jxta_module* (a ptr to) the new module, or throws to report an
 * error. The object returned is already shared and thus must be released
 * after use.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_loadfromid_module_e(Jxta_PG * self,
                                               Jxta_id * assigned_id, Jxta_MSID * spec_id, int where, Jxta_module ** module,
                                               Throws);


/**
 * Assign human readable labels (name and description) to a group.
 * Calling this routine is only usefull if the group is being created
 * from scratch and the PeerGroup advertisement has not been
 * created beforehand. This is how a new group gets named and described
 * when created that way.
 *
 * This routine must be called before the group's init routine.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param name The name of this group.
 * @param description The description of this group.
 */
JXTA_DECLARE(void) jxta_PG_set_labels(Jxta_PG * self, JString * name, JString * description);



/*
 * Valuable application helpers: Various methods to instantiate
 * groups.
 */

/**
 * Instantiate a group from its given advertisement
 * Use this when a published implementation advertisement for the group
 * sub-class can be discovered. The peer group advertisement itself may be all
 * new and unpublished. Therefore, the two typical uses of this routine are:
 *
 * - Creating an all new group with a new ID while using an existing
 *   and published implementation. (Possibly a new one previously published
 *   for that purpose). The information should first be gathered in a new
 *   peer group advertisement which is then passed to this method.
 *
 * - Instantiating a group which advertisement has already been discovered
 *   (therefore there is no need to find it by groupID again).
 *
 * To create a group from a known implementation advertisement, use
 * jxta_PG_new_fromimpl(group, gid, impladv, name, description);
 *
 * @param self handle of the group object to which the operation is applied.
 * @param pgAdv The advertisement of that group.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param result An address where to store (a ptr to) the initialized
 * (but not started) peergroup. The object returned is already shared
 * and thus must be released after use.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_newfromadv(Jxta_PG * self, Jxta_advertisement * pgAdv,
                                             Jxta_vector * resource_groups, Jxta_PG ** result);

/**
 * Instantiate a group from its given advertisement
 * Use this when a published implementation advertisement for the group
 * sub-class can be discovered. The peer group advertisement itself may be
 * all new and unpublished. Therefore, the two typical uses of this routine
 * are:
 *
 * - Creating an all new group with a new ID while using an existing
 *   and published implementation. (Possibly a new one previously published
 *   for that purpose). The information should first be gathered in a new
 *   peer group advertisement which is then passed to this method.
 *
 * - Instantiating a group which advertisement has already been discovered
 *   (therefore there is no need to find it by groupID again).
 *
 * To create a group from a known implementation adv, use
 * jxta_PG_newfromimpl(group, gid, impladv, name, description);
 *
 * If the operation cannot be performed, an exceptin is thrown.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param pg_adv The advertisement of that group.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param pg A location where to return (a ptr to) the resulting peer group
 * object, initialized but not started. The object returned is already
 * shared and thus must be released after use. If an exception is thown,
 * *pg is not affected.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_newfromadv_e(Jxta_PG * self,
                                        Jxta_advertisement * pg_adv, Jxta_vector * resource_groups, Jxta_PG ** pg, Throws);


/**
 * Convenience method, instantiate a group from its elementary pieces
 * and publish the corresponding peer group advertisement.
 *
 * The pieces are: the groups implementation advertisement, the group id,
 * the name and description.
 *
 * The typical use of this routine is creating a whole new group based
 * on a newly created and possibly unpublished implementation advertisement.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param gid The ID of that group.
 * @param impl The advertisement of the implementation to be used.
 * @param name The name of the group.
 * @param description A description of this group.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param result An address where to store (a ptr to) the initialized
 * (but not started) peergroup. The object returned is already shared
 * and thus must be released after use.
 *
 * @return Jxta_status JXTA_STATUS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_newfromimpl(Jxta_PG * self, Jxta_PGID * gid,
                                              Jxta_advertisement * impl, JString * name,
                                              JString * description, Jxta_vector * resource_groups, Jxta_PG ** result);


/**
 * Convenience method, instantiate a group from its elementary pieces
 * and publish the corresponding peer group advertisement.
 * The pieces are: the group implementation adv, the group id,
 * the name and description.
 *
 * The typical use of this routine is creating a whole new group based
 * on a newly created and possibly unpublished implementation advertisement.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param gid The ID of that group.
 * @param impl The advertisement of the implementation to be used.
 * @param name The name of the group.
 * @param description A description of this group.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param pg A location where to return (a ptr to) the resulting peer group
 * object, initialized but not started. The object returned is already
 * shared and thus must be released after use. If an exception is thown,
 * *pg is not affected.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_newfromimpl_e(Jxta_PG * self, Jxta_PGID * gid,
                                         Jxta_advertisement * impl, JString * name,
                                         JString * description, Jxta_vector * resource_groups, Jxta_PG ** pg, Throws);


/**
 * Instantiate a group from its group ID only.
 * Use this when using a group that has already been published and
 * which ID has been discovered.
 *
 * The typical uses of this routine is therefore:
 *
 * - instantiating a group which is assumed to exist and which GID is
 *   known.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param gid the groupID.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param result An address where to store (a ptr to) the initialized
 * (but not started) peergroup. The object returned is already shared
 * and thus must be released after use.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was performed, an error
 * otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_newfromid(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_groups, Jxta_PG ** result);


/**
 * Instantiate a group from its group ID only.
 * Use this when using a group that has already been published and
 * which ID has been discovered.
 *
 * The typical uses of this routine is therefore:
 *
 * - instantiating a group which is assumed to exist and which GID is
 *   known.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param gid the groupID.
 * @param resource_groups A set of additional groups the services of which
 * the new group can use for its own needs. (See set_resourcegroups()).
 * @param pg A location where to return (a ptr to) the resulting peer group
 * object, initialized but not started. The object returned is already
 * shared and thus must be released after use. If an exception is thown,
 * *pg is not affected.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_newfromid_e(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_groups, Jxta_PG ** pg, Throws);

/* FIXME: New API to be implemented
JXTA_DECLARE(Jxta_status) jxta_PG_create(Jxta_PG **me, Jxta_PG *parent, Jxta_MIA *mia);
JXTA_DECLARE(Jxta_status) jxta_PG_destroy(Jxta_PG *me);

*/

JXTA_DECLARE(apr_pool_t *) jxta_PG_pool_get(Jxta_PG * me);
JXTA_DECLARE(apr_thread_pool_t *) jxta_PG_thread_pool_get(Jxta_PG * me);
JXTA_DECLARE(Jxta_PG *) jxta_PG_netpg(Jxta_PG * me);
JXTA_DECLARE(Jxta_PG *) jxta_PG_parent(Jxta_PG * me);

JXTA_DECLARE(Jxta_status) jxta_PG_get_recipient_addr(Jxta_PG * me, const char * proto_name, const char * proto_addr,
                                                     const char *name, const char *param, Jxta_endpoint_address ** ea);
JXTA_DECLARE(Jxta_status) jxta_PG_add_recipient(Jxta_PG * me, void **cookie, const char *name, const char *param,
                                                Jxta_callback_func func, void *arg);
JXTA_DECLARE(Jxta_status) jxta_PG_remove_recipient(Jxta_PG * me, void *cookie);
JXTA_DECLARE(Jxta_status) jxta_PG_sync_send(Jxta_PG * me, Jxta_message * msg, Jxta_id * peer_id, 
                                            const char *svc_name, const char *svc_params);
JXTA_DECLARE(Jxta_status) jxta_PG_async_send(Jxta_PG * me, Jxta_message * msg, Jxta_id * peer_id, 
                                             const char *svc_name, const char *svc_params);

/*
 * shortcuts to the well-known services, in order to avoid calls to lookup.
 * Everything is pre-checked so these methods never fail.
 */

/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param rdv A location where to return (a ptr to) an object implementing the
 * rendezvous service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_rendezvous_service(Jxta_PG * self, Jxta_rdv_service ** rdv);


/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param endpoint A location where to return (a ptr to) an object implementing the
 * endpoint service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_endpoint_service(Jxta_PG * self, Jxta_endpoint_service ** endp);


/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param resolver A location where to return (a ptr to) an object implementing the
 * resolver service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_resolver_service(Jxta_PG * self, Jxta_resolver_service ** resolver);


/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param disco A location where to return (a ptr to) an object implementing the
 * discovery service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_discovery_service(Jxta_PG * self, Jxta_discovery_service ** disco);


/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param peerinfo A location where to return (a ptr to) an object implementing the
 * peerinfo service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_peerinfo_service(Jxta_PG * self, Jxta_peerinfo_service ** peerinfo);


/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param membership A location where to return (a ptr to) an object implementing the
 * membership service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_membership_service(Jxta_PG * self, Jxta_membership_service ** membership);

/**
 * @param self handle of the group object to which the operation is applied.
 *
 * @param pipe A location where to return (a ptr to) an object implementing the
 * pipe service for this group. The object returned is already shared
 * and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_pipe_service(Jxta_PG * self, Jxta_pipe_service ** pipe);

/**
 * Get the cache manager object that is used for this group.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param cm location to store the cache manager object.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_get_cache_manager(Jxta_PG * self, Jxta_cm ** cm);

/**
 * Set the cache manager to be used in this group.
 *
 * @param self handle of the group object to which the operation is applied.
 * @param cm Cache manager to use for this group.
 * @deprecated
 */
JXTA_DECLARE(void) jxta_PG_set_cache_manager(Jxta_PG * self, Jxta_cm * cm);

/*
 * A few convenience methods. This information is available from
 * the peer and peergroup advertisements. These methods do not fail.
 */

/**
 * Tell the ID of this group.
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param gid A location where to return (a ptr to) this groups ID.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_GID(Jxta_PG * self, Jxta_PGID ** gid);


/**
 * Tell the ID of this peer in this group.
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param pid A location where to return (a ptr to) the ID of this peer in this
 * group. The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_PID(Jxta_PG * self, Jxta_PID ** pid);


/**
 * Tell the Name of this group.
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param gname A location where to return (a ptr to) the name of this group.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_groupname(Jxta_PG * self, JString ** gname);


/**
 * Tell the Name of this peer in this group.
 * @param self handle of the group object to which the operation is applied.
 *
 * @param pname A location where to return (a ptr to) the name of this peer.
 * The object returned is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_peername(Jxta_PG * self, JString ** pname);


/**
 * Returns the configuration advertisment for this peer in this group (if any).
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param confadv A location where to return (a ptr to) the configuration
 * advertisement for this peer in this group. If there is no configuration
 * advertisement, a NULL pointer is returned, otherwise the object returned, is
 * already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_configadv(Jxta_PG * self, Jxta_PA ** confadv);


/**
 * Get an allPurpose peer group implementation advertisement compatible with
 * this group. This advertisement can be used to create any group that relies
 * only on the standard services. It may also be used as a base to derive
 * other implementation advertisements. The advertisement that is returned
 * is a copy which fully belongs to the caller.
 *
 * @param self handle of the group object to which the operation is applied.
 *
 * @param confadv A location where to return (a ptr to) the advertisement.
 * The object returned, is already shared and thus must be released after use.
 */
JXTA_DECLARE(void) jxta_PG_get_genericpeergroupMIA(Jxta_PG * self, Jxta_MIA ** mia);

/*
 * Supplies a group with references to other groups that can be used
 * to publish advertisements or find published advertisement regarding
 * this group. These advertisement will typically be those that are more
 * usefull when published "outside" the group, such as impl advs for
 * the group's services or rdv advertisements.
 * 
 * This is a generalization of the previous practice which consisted on
 * using exclusively the so-called "parent" group for that purpose.
 *
 * This method is to be called before the group's init method. Typically
 * by the code performing the instantiation of that group. The vector
 * is to be supplied by the application code via one of the PG_newfrom
 * methods.
 *
 * @param self Handle to the group to which the operation is applied.
 * @param resource groups. A vector of already instantiated groups. If NULL
 * the group may use its "parent" group as the sole resource group.
 */
JXTA_DECLARE(void) jxta_PG_set_resourcegroups(Jxta_PG * self, Jxta_vector * resource_groups);

/*
 * Returns references to other groups that can be used to publish
 * advertisements or find published advertisement regarding
 * this group. These advertisement will typically be those that are more
 * usefull when published "outside" the group, such as impl advs for
 * the group's services or rdv advertisements.
 * 
 * This is a generalization of the previous practice which consisted on
 * using exclusively the so-called "parent" group for that purpose.
 *
 * This method returns the current set of resource groups, as was
 * initialized by set_resourcegroups and possibly modified afterwards.
 * The group implementation may chose to add the "parent" group
 * to the set.
 *
 * @param self Handle to the group to which the operation is applied.
 * @param resource_groups. A location where to return (a ptr to) a vector of
 * groups.
 */
JXTA_DECLARE(void) jxta_PG_get_resourcegroups(Jxta_PG * self, Jxta_vector ** resource_groups);

/*
 * Returns a reference to the group object that was passed as the "group"
 * parameter to this group's init() method.
 * This is the group of which one of the newfromxxx() method was called
 * in order to instantiate this group.
 * Service of this group may make use of the parent's group service in support
 * of their own implementation. The parent group is also automatically added to
 * the resourcegroups list.
 *
 * @param self Handle to the group to which the operation is applied.
 * @param parent_group. A location where to return (a ptr to) the parent group
 * object. This may be null if this group has no parent or does keep a reference
 * to its parent.
 */
JXTA_DECLARE(void) jxta_PG_get_parentgroup(Jxta_PG * self, Jxta_PG ** parent_group);

/*
 * Returns a copy of the compatibility statement string for this group.
 *
 * @param self Handle to the group to which the operation is applied.
 * @param compat. A location where to return (a ptr to) the compat statement
 * string.
 */
JXTA_DECLARE(void) jxta_PG_get_compatstatement(Jxta_PG * self, JString ** compat);

/*
 * @todo Add documentation.
 */
JXTA_DECLARE(void) jxta_PG_get_srdi_service(Jxta_PG * self, Jxta_srdi_service ** srdi);

/*
 * Returns an initialized and ready to use (but not started) instance of
 * the default net peer group.
 *
 * In other words, this is a net peer group factory.
 * The object is already shared and thus must be released
 * after use, unless the invoking application simply terminates
 * instead.
 *
 * @param new_netpg pointer to a memory area where to store a reference
 * to the newly created instance.
 *
 * @return Jxta_status JXTA_SUCCESS if the net peergroup was instantiated as
 * described above. Otherwise, an error code is returned and *new_netpg is not
 * affected.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_new_netpg(Jxta_PG ** new_netpg);

/*
 * Returns an initialized and ready to use (but not started) instance of
 * a custom net peer group. All extra services are loaded
 *
 * In other words, this is a net peer group factory for custom group.
 * The object is already shared and thus must be released
 * after use, unless the invoking application simply terminates
 * instead.
 *
 * WARNING: currently what is suppored is that you only specify your own
 * services in the module implementation advertisement.
 *
 * @param new_custom_netpg pointer to a memory area where to store a reference
 * to the newly created instance.
 *
 * @return Jxta_status JXTA_SUCCESS if the custom net peergroup was instantiated as
 * described above. Otherwise, an error code is returned and *new_custom_netpg is not
 * affected.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_new_custom_netpg(Jxta_PG ** new_custom_netpg, Jxta_MIA * mia);

/*
 * To be clean, groups should register before they complete
 * initialization, but they should have their get_interface routine
 * block until intialization is complete. That way we avoid letting
 * two redundant instances init in parallel and then have one to abort late.
 * If a group attempts to register and gets kicked-out, it should
 * abort initialization and return JXTA_ITEM_EXISTS.
 *
 * @param gid The ID of the group being registered.
 * @param pg the group object; not necessarily fully functional.
 *
 * @return Jxta_status JXTA_SUCCESS if ok, JXTA_ITEM_EXISTS if there is
 * already has a group object associated with th egiven GID.
 */
JXTA_DECLARE(Jxta_status) jxta_register_group_instance(Jxta_id * gid, Jxta_PG * pg);


/*
 * Will remove the given GID/group object pair from the global group registry
 * if such a pair is found.
 * In the future, unprivileged applications may obtain an interface
 * object to each group that is private and distinct from the group
 * object itself. Checking the object being removed will prevent such
 * applications from removing groups from the registry.
 *
 * @param gid The ID of the group to be unregistered.
 * @param pg The group object to be unregistered.
 *
 * @return Jxta_status JXTA_SUCCESS if ok. JXTA_ITEM_NOTFOUND if no such
 * group ID is registered. JXTA_VIOLATION if the given group object does
 * not match the registered one.
 */
JXTA_DECLARE(Jxta_status) jxta_unregister_group_instance(Jxta_id * gid, Jxta_PG * pg);

/*
 * Lookup an already instantiated group by ID.
 *
 * @param gid the ID of the wanted group.
 * @param pg Address where to return (a ptr to) the found group interface
 * object. If the group is not found the data at this address is not affected.
 *
 * @return Jxta_status JXTA_SUCCESS if found, an error otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_lookup_group_instance(Jxta_id * gid, Jxta_PG ** pg);

/*
 * Retrieve all groups that have been registered
 *
 */
JXTA_DECLARE(Jxta_vector *) jxta_get_registered_groups();

/*
 * Add a new relay address to the peergroup advertisement. This
 * is done whenever a new relay address is registered after the
 * peer connected to its relay
 *
 * @param group pointer
 * @param Jxta_RdvAdvertisement relay advertisement that contains
 * relay route.
 *
 * @return Jxta_status JXTA_SUCCESS if found, an error otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_add_relay_address(Jxta_PG * pg, Jxta_RdvAdvertisement * relay);

/*
 * Remove a relay address from the peergroup advertisement after
 * the peer disconnected from the relay
 *
 * @param group pointer
 * @param Jxta_id of the relay to remove.
 *
 * @return Jxta_status JXTA_SUCCESS if found, an error otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_PG_remove_relay_address(Jxta_PG * pg, Jxta_id * relayid);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_PEERGROUP_H */

/* vi: set ts=4 sw=4 tw=130 et: */
