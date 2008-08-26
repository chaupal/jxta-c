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
static const char *__log_cat = "PG";

#include <assert.h>

#include "jpr/jpr_excep.h"

#include "jxta_apr.h"
#include "jxta_defloader_private.h"
#include "jxta_types.h"

/*
 * For now, jxta_peergroup.h fakes many of the types we use here.
 * Need to include appropriate headers in the future.
 */

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_id.h"
#include "jxta_peergroup.h"
#include "jxta_peergroup_private.h"
#include "jxta_peerview_priv.h"
#include "jxta_objecthashtable.h"
#include "jxta_endpoint_service_priv.h"
#include "jxta_discovery_service.h"
#include "jxta_rdv_service.h"
#include "jxta_util_priv.h"
#include "jxta_log.h"
#include "jxta_dr.h"

#ifndef JXTA_SINGLE_THREAD_POOL
#define JXTA_SINGLE_THREAD_POOL 1
#endif

/*
 * This implementation supports only one instance of each group; the peer ID
 * is the same in each group that a given instance of Jxta joins.
 * To enforce that we need a global registry. This is probably the only
 * "global" (as in "not provided by a group") API in Jxta-C besides new_netpg.
 */
static Jxta_objecthashtable *groups_global_registry;

/*
 * Thread private data initializer control.
 */

static void registry_init(void)
{
    groups_global_registry = jxta_objecthashtable_new(4,
                                                      (Jxta_object_hash_func) jxta_id_hashcode,
                                                      (Jxta_object_equals_func) jxta_id_equals);
}

/*
 * To be clean, groups should register before they complete
 * initialization, but they should have their get_interface routine
 * block until intialization is complete. That way we avoid letting
 * two redundant instances init in parallel and then have one to abort late.
 * If a group attempts to register and gets kicked-out, it should
 * abort initialization and return JXTA_ITEM_EXISTS.
 */
JXTA_DECLARE(Jxta_status) jxta_register_group_instance(Jxta_id * gid, Jxta_PG * pg)
{
    return jxta_objecthashtable_putnoreplace(groups_global_registry, (Jxta_object *) gid, (Jxta_object *) pg);
}

/*
 * Will remove the given GID/group pair from the global group registry
 * if such a pair is found.
 * In the future, unprivileged applications may obtain an interface
 * object to each group that is private and distinct from the group
 * object itself. Checking the object being removed will prevent such
 * applications from removing groups from the registry.
 */
JXTA_DECLARE(Jxta_status) jxta_unregister_group_instance(Jxta_id * gid, Jxta_PG * pg)
{
    return jxta_objecthashtable_delcheck(groups_global_registry, (Jxta_object *) gid, (Jxta_object *) pg);
}

/*
 * Lookup a group by ID.
 */
JXTA_DECLARE(Jxta_status) jxta_lookup_group_instance(Jxta_id * gid, Jxta_PG ** pg)
{
    Jxta_service *tmp;
    Jxta_status res;

    res = jxta_objecthashtable_get(groups_global_registry, (Jxta_object *) gid, JXTA_OBJECT_PPTR(&tmp));

    if (res != JXTA_SUCCESS)
        return res;
    jxta_service_get_interface(tmp, (Jxta_service **) pg);
    JXTA_OBJECT_RELEASE(tmp);

    return res;
}

JXTA_DECLARE(Jxta_vector *) jxta_get_registered_groups()
{
    if (NULL == groups_global_registry) return NULL;
    return jxta_objecthashtable_values_get(groups_global_registry);
}
/**
 * Well known module class identifier: peer group
 */
static Jxta_MCID *_peergroup_classid;

/**
 * Well known module class identifier: resolver service
 */
static Jxta_MCID *_resolver_classid;

/**
 * Well known module class identifier: discovery service
 */
static Jxta_MCID *_discovery_classid;

/**
 * Well known module class identifier: pipe service
 */
static Jxta_MCID *_pipe_classid;

/**
 * Well known module class identifier: membership service
 */
static Jxta_MCID *_membership_classid;

/**
 * Well known module class identifier: rendezvous service
 */
static Jxta_MCID *_rendezvous_classid;

/**
 * Well known module class identifier: peerinfo service
 */
static Jxta_MCID *_peerinfo_classid;

/**
 * Well known module class identifier: endpoint service
 */
static Jxta_MCID *_endpoint_classid;

/**
 * Well known module class identifier: srdi service
 */
static Jxta_MCID *_srdi_classid;

/**
 * Well known module class identifier: cache manager
 */
static Jxta_MCID *_cache_classid;
/*
 * fixme: endpointprotocols should probably all be of the same class
 * and of different specs and roles... but we'll take a shortcut for now.
 */

/**
 * Well known module class identifier: tcp protocol
 */
static Jxta_MCID *_tcpproto_classid;

/**
 * Well known module class identifier: http protocol
 */
static Jxta_MCID *_httpproto_classid;

/**
 * Well known module class identifier: router protocol
 */
static Jxta_MCID *_routerproto_classid;

/**
 * Well known module class identifier: relay protocol
 */
static Jxta_MCID *_relayproto_classid;

/**
 * Well known module class identifier: application
 */
static Jxta_MCID *_application_classid;

/**
 * Well known module class identifier: tlsprotocol
 */
static Jxta_MCID *_tlsproto_classid;

/**
 * Well known group specification identifier: the platform
 */
static Jxta_MSID *_ref_platform_specid;

/**
 * Well known group specification identifier: the network peer group
 */
static Jxta_MSID *_ref_netpeergroup_specid;

/**
 * Well known service specification identifier: the standard resolver
 */
static Jxta_MSID *_ref_resolver_specid;

/**
 * Well known service specification identifier: the standard discovery
 */
static Jxta_MSID *_ref_discovery_specid;

/**
 * Well known service specification identifier: the standard pipe
 */
static Jxta_MSID *_ref_pipe_specid;

/**
 * Well known service specification identifier: the standard membership
 */
static Jxta_MSID *_ref_membership_specid;

/**
 * Well known service specification identifier: the standard rendezvous
 */
static Jxta_MSID *_ref_rendezvous_specid;

/**
 * Well known service specification identifier: the standard peerinfo
 */
static Jxta_MSID *_ref_peerinfo_specid;

/**
 * Well known service specification identifier: the standard endpoint
 */
static Jxta_MSID *_ref_endpoint_specid;

/**
 * Well known TCP/IP protocol specification identifier: the standard
 * tcp endpoint protocol
 */
static Jxta_MSID *_ref_tcpproto_specid;

/**
 * Well known HTTP protocol specification identifier: the standard
 * http endpoint protocol
 */
static Jxta_MSID *_ref_httpproto_specid;

/**
 * Well known router protocol specification identifier: the standard
 * router
 */
static Jxta_MSID *_ref_routerproto_specid;

/**
 * Well known relay protocol specification identifier: the standard
 * router
 */
static Jxta_MSID *_ref_relayproto_specid;

/**
 * Well known tls protocol specification identifier: the standard
 * tls endpoint protocol
 */
static Jxta_MSID *_ref_tlsproto_specid;

/**
 * Well known group specification identifier: an all purpose peer group
 * specification. the java reference implementation implements it with
 * the stdpeergroup class and all the standard platform services and no
 * endpoint protocols.
 */
static Jxta_MSID *_genericpeergroup_specid;

/**
 * Well known main application of the platform: startnetpeergroup.
 */
static Jxta_MSID *_ref_startnetpeergroup_specid;

/**
 * Well known application: the shell
 */
static Jxta_MSID *_ref_shell_specid;

/**
 * Used to create static well-known identifiers.
 */
static Jxta_id *buildWellKnownID(const char *s)
{
    Jxta_id *result = NULL;
    JString *tmp = jstring_new_2("urn:jxta:uuid-");
    jstring_append_2(tmp, s);
    jxta_id_from_jstring(&result, tmp);
    JXTA_OBJECT_RELEASE(tmp);
    return result;
}

/**
 * Well known classes for the basic services.
 * FIXME: we should make a "well-known ID" format implementation that
 * has its own little name space of human readable names...later.
 * To keep their string representation shorter, we put our small spec
 * or role pseudo unique ID at the front of the second UUID string.
 * Base classes do not need an explicit second UUID string because it is
 * all 0.
 * The type is always the last two characters, nomatter the total length.
 */

static void static_id_init(void)
{
    /**
     * Well known module class identifier: peer group
     */
    _peergroup_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000001" "05");
    if (NULL == _peergroup_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: resolver service
     */
    _resolver_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000002" "05");
    if (NULL == _resolver_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: discovery service
     */
    _discovery_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000003" "05");
    if (NULL == _discovery_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: pipe service
     */
    _pipe_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000004" "05");
    if (NULL == _pipe_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: membership service
     */
    _membership_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000005" "05");
    if (NULL == _membership_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: rendezvous service
     */
    _rendezvous_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000006" "05");
    if (NULL == _rendezvous_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: peerinfo service
     */
    _peerinfo_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000007" "05");
    if (NULL == _peerinfo_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: endpoint service
     */
    _endpoint_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000008" "05");
    if (NULL == _endpoint_classid) {
        goto ERROR_EXIT;
    }

    /*
     * FIXME: EndpointProtocols should probably all be of the same class
     * and of different specs and roles... But we'll take a shortcut for now.
     */

    /**
     * Well known module class identifier: tcp protocol
     */
    _tcpproto_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000009" "05");
    if (NULL == _tcpproto_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: http protocol
     */
    _httpproto_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000A" "05");
    if (NULL == _httpproto_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: relay protocol
     */
    _relayproto_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000F" "05");
    if (NULL == _relayproto_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known module class identifier: router protocol
     */
    _routerproto_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000B" "05");
    if (NULL == _routerproto_classid) {
        goto ERROR_EXIT;
    }


    /**
     * Well known module class identifier: application
     */
    _application_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000C" "05");
    if (NULL == _application_classid) {
        goto ERROR_EXIT;
    }


    /**
     * Well known module class identifier: tlsProtocol
     */
    _tlsproto_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000D" "05");
    if (NULL == _tlsproto_classid) {
        goto ERROR_EXIT;
    }

     /**
     * Well known module class identifier: srdi service
     */
    _srdi_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000021" "05");
    if (NULL == _srdi_classid) {
        goto ERROR_EXIT;
    }

     /**
     * Well known module class identifier: cache manager
     */
    _cache_classid = (Jxta_MCID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000022" "05");
    if (NULL == _cache_classid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known group specification identifier: the platform
     */
    _ref_platform_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000001" "01" "06");
    if (NULL == _ref_platform_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known group specification identifier: the Network Peer Group
     */
    _ref_netpeergroup_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000001" "02" "06");
    if (NULL == _ref_netpeergroup_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard resolver
     */
    _ref_resolver_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000002" "01" "06");
    if (NULL == _ref_resolver_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard discovery
     */
    _ref_discovery_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000003" "01" "06");
    if (NULL == _ref_discovery_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard pipe
     */
    _ref_pipe_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000004" "01" "06");
    if (NULL == _ref_pipe_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard membership
     */
    _ref_membership_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000005" "01" "06");
    if (NULL == _ref_membership_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard rendezvous
     */
    _ref_rendezvous_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000006" "01" "06");
    if (NULL == _ref_rendezvous_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard peerinfo
     */
    _ref_peerinfo_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000007" "01" "06");
    if (NULL == _ref_peerinfo_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known service specification identifier: the standard endpoint
     */
    _ref_endpoint_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000008" "01" "06");
    if (NULL == _ref_endpoint_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known endpoint protocol specification identifier: the standard
     * tcp endpoint protocol
     */
    _ref_tcpproto_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000009" "01" "06");
    if (NULL == _ref_tcpproto_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known endpoint protocol specification identifier: the standard
     * http endpoint protocol
     */
    _ref_httpproto_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000A" "01" "06");
    if (NULL == _ref_httpproto_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known endpoint protocol specification identifier: the standard
     * router
     */
    _ref_routerproto_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000B" "01" "06");
    if (NULL == _ref_routerproto_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known endpoint protocol specification identifier: the standard
     * tls endpoint protocol
     */
    _ref_tlsproto_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000D" "01" "06");
    if (NULL == _ref_tlsproto_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known main application of the platform: startNetPeerGroup.
     */
    _ref_startnetpeergroup_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000C" "01" "06");
    if (NULL == _ref_startnetpeergroup_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known application: the shell
     */
    _ref_shell_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe0000000C" "02" "06");
    if (NULL == _ref_shell_specid) {
        goto ERROR_EXIT;
    }

    /**
     * Well known group specification identifier: an all purpose peer group
     * specification. The java reference implementation implements it with
     * the StdPeerGroup class and all the standard platform services and no
     * endpoint protocols.
     */
    _genericpeergroup_specid = (Jxta_MSID *)
        buildWellKnownID("DeadBeefDeafBabaFeedBabe00000001" "03" "06");
    if (NULL == _genericpeergroup_specid) {
        goto ERROR_EXIT;
    }

    return;

  ERROR_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, "Malformed Hardcoded well-known IDs.\n");
    abort();
}

/**
 * get each well known ID.
 */
JXTA_DECLARE(Jxta_MCID *) jxta_peergroup_classid_get(void)
{
    return _peergroup_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_resolver_classid_get(void)
{
    return _resolver_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_discovery_classid_get(void)
{
    return _discovery_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_pipe_classid_get(void)
{
    return _pipe_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_membership_classid_get(void)
{
    return _membership_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_rendezvous_classid_get(void)
{
    return _rendezvous_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_peerinfo_classid_get(void)
{
    return _peerinfo_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_srdi_classid_get(void)
{

    return _srdi_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_cache_classid_get(void)
{

    return _cache_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_endpoint_classid_get(void)
{

    return _endpoint_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_tcpproto_classid_get(void)
{
    return _tcpproto_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_httpproto_classid_get(void)
{
    return _httpproto_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_relayproto_classid_get(void)
{
    return _relayproto_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_routerproto_classid_get(void)
{
    return _routerproto_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_application_classid_get(void)
{
    return _application_classid;
}

JXTA_DECLARE(Jxta_MCID *) jxta_tlsproto_classid_get(void)
{
    return _tlsproto_classid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_platform_specid_get(void)
{
    return _ref_platform_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_netpeergroup_specid_get(void)
{
    return _ref_netpeergroup_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_resolver_specid_get(void)
{
    return _ref_resolver_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_discovery_specid_get(void)
{
    return _ref_discovery_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_pipe_specid_get(void)
{
    return _ref_pipe_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_membership_specid_get(void)
{
    return _ref_membership_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_rendezvous_specid_get(void)
{
    return _ref_rendezvous_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_peerinfo_specid_get(void)
{
    return _ref_peerinfo_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_endpoint_specid_get(void)
{
    return _ref_endpoint_specid;
}

Jxta_MSID *jxta_ref_tcpproto_specid_get(void)
{
    return _ref_tcpproto_specid;
}

Jxta_MSID *jxta_ref_httpproto_specid_get(void)
{
    return _ref_httpproto_specid;
}

Jxta_MSID *jxta_ref_relayproto_specid_get(void)
{
    return _ref_relayproto_specid;
}

Jxta_MSID *jxta_ref_routerproto_specid_get(void)
{
    return _ref_routerproto_specid;
}

Jxta_MSID *jxta_ref_tlsproto_specid_get(void)
{
    return _ref_tlsproto_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_startnetpeergroup_specid_get(void)
{
    return _ref_startnetpeergroup_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_ref_shell_specid_get(void)
{
    return _ref_shell_specid;
}

JXTA_DECLARE(Jxta_MSID *) jxta_genericpeergroup_specid_get(void)
{
    return _genericpeergroup_specid;
}

/**
 * The base PG ctor (not public: the only public way to make a new pg 
 * is to instantiate one of the derived types).
 */
void jxta_PG_construct(Jxta_PG * self, Jxta_PG_methods * methods)
{
    Jxta_PG_methods* pgmethods = PTValid(methods, Jxta_PG_methods);
    jxta_service_construct((Jxta_service *) self, (Jxta_service_methods *) pgmethods);
    self->thisType = "Jxta_PG";
}

Jxta_status peergroup_init(Jxta_PG * me, Jxta_PG * parent)
{
    me->parent = parent;
    apr_pool_create(&me->pool, parent ? parent->pool : NULL);
    /* FIXME: JXTA_SINGLE_THREAD_POOL better to use configuration than defined constant */
    if (parent && JXTA_SINGLE_THREAD_POOL) {
        me->thd_pool = NULL;
    } else {
        apr_thread_pool_create(&me->thd_pool, 3, 5, me->pool);
    }

    return JXTA_SUCCESS;
}

static Jxta_status JXTA_STDCALL peergroup_ep_callback(Jxta_object * obj, void *arg)
{
    Jxta_PG *me = arg;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_endpoint_address *dest;
    char *name, *param;
    Jxta_endpoint_service *ep;
    Jxta_status rv;

    dest = jxta_message_get_destination(msg);
    name = strdup(jxta_endpoint_address_get_service_params(dest));
    JXTA_OBJECT_RELEASE(dest);
    param = strchr(name, '/');
    if (param) {
        *param++ = '\0';
    }

    jxta_PG_get_endpoint_service(me, &ep);
    rv = endpoint_service_demux(ep, name, param, msg);
    free(name);
    JXTA_OBJECT_RELEASE(ep);

    return rv;
}

Jxta_status peergroup_start(Jxta_PG * me)
{
    static const char *prefix = "EndpointService:";
    Jxta_id *gid;
    JString *tmp;
    Jxta_endpoint_service *ep;
    Jxta_status rv;

    jxta_PG_get_GID(me, &gid);
    if (jxta_id_equals(gid, jxta_id_defaultNetPeerGroupID)) {
        tmp = jstring_new_2("jxta-NetGroup");
    } else {
        jxta_id_get_uniqueportion(gid, &tmp);
    }
    me->ep_name = apr_pstrcat(me->pool, prefix, jstring_get_string(tmp), NULL);
    JXTA_OBJECT_RELEASE(gid);
    JXTA_OBJECT_RELEASE(tmp);

    jxta_PG_get_endpoint_service(me, &ep);
    rv = endpoint_service_add_recipient(ep, &me->ep_cookie, me->ep_name, NULL, peergroup_ep_callback, me);
    JXTA_OBJECT_RELEASE(ep);
    return rv;
}

Jxta_status peergroup_stop(Jxta_PG * me)
{
    Jxta_endpoint_service *ep;
    Jxta_status rv;

    jxta_PG_get_endpoint_service(me, &ep);
    rv = endpoint_service_remove_recipient(ep, me->ep_cookie);
    JXTA_OBJECT_RELEASE(ep);
    if (me->thd_pool) {
        apr_thread_pool_destroy(me->thd_pool);
        me->thd_pool = NULL;
    }

    return rv;
}



/**
 * The base PG dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 */
void jxta_PG_destruct(Jxta_PG * me)
{
    if (NULL != me->pool) {
        apr_pool_destroy(me->pool);
        me->pool = NULL;
    }

    jxta_service_destruct((Jxta_service *) me);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_PG_methods*) JXTA_MODULE_VTBL(self))

JXTA_DECLARE(void) jxta_PG_get_loader(Jxta_PG * self, Jxta_loader ** loader)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_loader) (myself, loader);
}

JXTA_DECLARE(void) jxta_PG_get_PGA(Jxta_PG * self, Jxta_PGA ** pga)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_PGA) (myself, pga);
}

JXTA_DECLARE(void) jxta_PG_get_PA(Jxta_PG * self, Jxta_PA ** pa)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_PA) (myself, pa);
}

JXTA_DECLARE(Jxta_status) jxta_PG_lookup_service(Jxta_PG * self, Jxta_id * name, Jxta_service ** result)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->lookup_service) (myself, name, result);
}

JXTA_DECLARE(void) jxta_PG_lookup_service_e(Jxta_PG * self, Jxta_id * name, Jxta_service ** result, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->lookup_service) (myself, name, result);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(Jxta_boolean) jxta_PG_is_compatible(Jxta_PG * self, JString * compat)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->is_compatible) (myself, compat);
}

JXTA_DECLARE(Jxta_status) jxta_PG_loadfromimpl_module(Jxta_PG * self, Jxta_id * assigned_id,
                                                      Jxta_advertisement * impl, Jxta_module ** result)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->loadfromimpl_module) (myself, assigned_id, impl, result);
}

JXTA_DECLARE(void) jxta_PG_loadfromimpl_module_e(Jxta_PG * self, Jxta_id * assigned_id,
                                                 Jxta_advertisement * impl, Jxta_module ** mod, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->loadfromimpl_module) (myself, assigned_id, impl, mod);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(Jxta_status) jxta_PG_loadfromid_module(Jxta_PG * self, Jxta_id * assigned_id,
                                                    Jxta_MSID * spec_id, int where, Jxta_module ** result)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->loadfromid_module) (myself, assigned_id, spec_id, where, result);
}

JXTA_DECLARE(void) jxta_PG_loadfromid_module_e(Jxta_PG * self, Jxta_id * assigned_id,
                                               Jxta_MSID * spec_id, int where, Jxta_module ** mod, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->loadfromid_module) (myself, assigned_id, spec_id, where, mod);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(void) jxta_PG_set_labels(Jxta_PG * self, JString * name, JString * description)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->set_labels) (myself, name, description);
}

JXTA_DECLARE(Jxta_status) jxta_PG_newfromadv(Jxta_PG * self, Jxta_advertisement * pgAdv,
                                             Jxta_vector * resource_groups, Jxta_PG ** pg)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->newfromadv) (myself, pgAdv, resource_groups, pg);
}

JXTA_DECLARE(void) jxta_PG_newfromadv_e(Jxta_PG * self,
                                        Jxta_advertisement * pgAdv, Jxta_vector * resource_groups, Jxta_PG ** pg, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->newfromadv) (myself, pgAdv, resource_groups, pg);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(Jxta_status) jxta_PG_newfromimpl(Jxta_PG * self, Jxta_PGID * gid,
                                              Jxta_advertisement * impl, JString * name,
                                              JString * description, Jxta_vector * resource_groups, Jxta_PG ** result)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->newfromimpl) (myself, gid, impl, name, description, resource_groups, result);
}

JXTA_DECLARE(void) jxta_PG_newfromimpl_e(Jxta_PG * self, Jxta_PGID * gid,
                                         Jxta_advertisement * impl, JString * name,
                                         JString * description, Jxta_vector * resource_groups, Jxta_PG ** pg, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->newfromimpl) (myself, gid, impl, name, description, resource_groups, pg);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(Jxta_status) jxta_PG_newfromid(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_groups, Jxta_PG ** result)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->newfromid) (myself, gid, resource_groups, result);
}

JXTA_DECLARE(void) jxta_PG_newfromid_e(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_groups, Jxta_PG ** result, Throws)
{
    Jxta_status res;
    Jxta_PG* myself = PTValid(self, Jxta_PG);

    JXTA_DEPRECATED_API();

    res = (VTBL->newfromid) (myself, gid, resource_groups, result);

    if (JXTA_SUCCESS != res) {
        Throw(res);
    }
}

JXTA_DECLARE(void) jxta_PG_get_rendezvous_service(Jxta_PG * self, Jxta_rdv_service ** rdv)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_rendezvous_service) (myself, rdv);
}
JXTA_DECLARE(void) jxta_PG_get_endpoint_service(Jxta_PG * self, Jxta_endpoint_service ** endp)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_endpoint_service) (myself, endp);
}

JXTA_DECLARE(void) jxta_PG_get_resolver_service(Jxta_PG * self, Jxta_resolver_service ** res)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_resolver_service) (myself, res);
}

JXTA_DECLARE(void) jxta_PG_get_discovery_service(Jxta_PG * self, Jxta_discovery_service ** disco)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_discovery_service) (myself, disco);
}

JXTA_DECLARE(void) jxta_PG_get_peerinfo_service(Jxta_PG * self, Jxta_peerinfo_service ** peerinfo)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_peerinfo_service) (myself, peerinfo);
}

JXTA_DECLARE(void) jxta_PG_get_membership_service(Jxta_PG * self, Jxta_membership_service ** memb)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_membership_service) (myself, memb);
}

JXTA_DECLARE(void) jxta_PG_get_pipe_service(Jxta_PG * self, Jxta_pipe_service ** pipe)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_pipe_service) (myself, pipe);
}

JXTA_DECLARE(void) jxta_PG_get_cache_manager(Jxta_PG * self, Jxta_cm ** cm)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    JXTA_DEPRECATED_API();
    (VTBL->get_cache_manager) (myself, cm);

}

JXTA_DECLARE(void) jxta_PG_set_cache_manager(Jxta_PG * self, Jxta_cm * cm)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    JXTA_DEPRECATED_API();
    (VTBL->set_cache_manager) (myself, cm);
}

JXTA_DECLARE(void) jxta_PG_get_srdi_service(Jxta_PG * self, Jxta_srdi_service ** srdi)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_srdi_service) (myself, srdi);
}

JXTA_DECLARE(void) jxta_PG_get_GID(Jxta_PG * self, Jxta_PGID ** gid)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_GID) (myself, gid);
}

JXTA_DECLARE(void) jxta_PG_get_PID(Jxta_PG * self, Jxta_PID ** pid)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_PID) (myself, pid);
}

JXTA_DECLARE(void) jxta_PG_get_groupname(Jxta_PG * self, JString ** nm)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_groupname) (myself, nm);
}

JXTA_DECLARE(void) jxta_PG_get_peername(Jxta_PG * self, JString ** nm)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_peername) (myself, nm);
}

JXTA_DECLARE(void) jxta_PG_get_configadv(Jxta_PG * self, Jxta_PA ** adv)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_configadv) (myself, adv);
}

JXTA_DECLARE(void) jxta_PG_get_genericpeergroupMIA(Jxta_PG * self, Jxta_MIA ** mia)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_genericpeergroupMIA) (myself, mia);
}

JXTA_DECLARE(void) jxta_PG_set_resourcegroups(Jxta_PG * self, Jxta_vector * resource_groups)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->set_resourcegroups) (myself, resource_groups);
}

JXTA_DECLARE(void) jxta_PG_get_resourcegroups(Jxta_PG * self, Jxta_vector ** resource_groups)
{
   Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_resourcegroups) (myself, resource_groups);
}

JXTA_DECLARE(void) jxta_PG_get_parentgroup(Jxta_PG * self, Jxta_PG ** parent_group)
{
   Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->get_parentgroup) (myself, parent_group);
}

JXTA_DECLARE(void) jxta_PG_get_compatstatement(Jxta_PG * self, JString ** compat)
{
    self = PTValid(self, Jxta_PG);
    (VTBL->get_compatstatement) (self, compat);
}

JXTA_DECLARE(Jxta_status) jxta_PG_new_netpg(Jxta_PG ** new_netpg)
{
    return jxta_PG_new_custom_netpg(new_netpg, NULL);
}

JXTA_DECLARE(Jxta_status) jxta_PG_new_netpg_1(Jxta_PG ** new_netpg)
{
    Jxta_module *npg = NULL;
    Jxta_status res = JXTA_SUCCESS;

    res = jxta_defloader_instantiate("builtin:netpg", &npg);

    if (JXTA_SUCCESS == res)
        *new_netpg = (Jxta_PG *) npg;

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_PG_new_custom_netpg(Jxta_PG ** new_netpg, Jxta_MIA * mia)
{
    Jxta_module *npg = NULL;
    Jxta_status res = JXTA_SUCCESS;
    const char *noargs[] = { NULL };

    res = jxta_defloader_instantiate("builtin:netpg", &npg);

    if (JXTA_SUCCESS != res) {
        goto FINAL_EXIT;
    }

    res = jxta_module_init(npg, (Jxta_PG *) NULL, NULL, (Jxta_advertisement *) mia);

    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(npg);
        goto FINAL_EXIT;
    }

    res = jxta_module_start((Jxta_module *) npg, noargs);

    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(npg);
        goto FINAL_EXIT;
    }

    *new_netpg = (Jxta_PG *) npg;

  FINAL_EXIT:

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_PG_add_relay_address(Jxta_PG * self, Jxta_RdvAdvertisement * relay)
{
   Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->add_relay_address) (myself, relay);
}

JXTA_DECLARE(Jxta_status) jxta_PG_remove_relay_address(Jxta_PG * self, Jxta_id * relayid)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    return (VTBL->remove_relay_address) (myself, relayid);
}

void peergroup_get_cache_manager(Jxta_PG * self, Jxta_cm ** cm)
{
    self = PTValid(self, Jxta_PG);
    (VTBL->get_cache_manager) (self, cm);
}

void peergroup_set_cache_manager(Jxta_PG * self, Jxta_cm * cm)
{
    Jxta_PG* myself = PTValid(self, Jxta_PG);
    (VTBL->set_cache_manager) (myself, cm);
}

Jxta_status jxta_PG_module_initialize(void)
{
    registry_init();
    static_id_init();

    return JXTA_SUCCESS;
}

void jxta_PG_module_terminate(void)
{
    JXTA_OBJECT_RELEASE(groups_global_registry);
    groups_global_registry = NULL;

    JXTA_OBJECT_RELEASE(_peergroup_classid);
    JXTA_OBJECT_RELEASE(_resolver_classid);
    JXTA_OBJECT_RELEASE(_discovery_classid);
    JXTA_OBJECT_RELEASE(_pipe_classid);
    JXTA_OBJECT_RELEASE(_membership_classid);
    JXTA_OBJECT_RELEASE(_rendezvous_classid);
    JXTA_OBJECT_RELEASE(_peerinfo_classid);
    JXTA_OBJECT_RELEASE(_endpoint_classid);
    JXTA_OBJECT_RELEASE(_srdi_classid);
    JXTA_OBJECT_RELEASE(_cache_classid);
    JXTA_OBJECT_RELEASE(_tcpproto_classid);
    JXTA_OBJECT_RELEASE(_httpproto_classid);
    JXTA_OBJECT_RELEASE(_relayproto_classid);
    JXTA_OBJECT_RELEASE(_routerproto_classid);
    JXTA_OBJECT_RELEASE(_application_classid);
    JXTA_OBJECT_RELEASE(_tlsproto_classid);
    JXTA_OBJECT_RELEASE(_ref_platform_specid);
    JXTA_OBJECT_RELEASE(_ref_netpeergroup_specid);
    JXTA_OBJECT_RELEASE(_ref_resolver_specid);
    JXTA_OBJECT_RELEASE(_ref_discovery_specid);
    JXTA_OBJECT_RELEASE(_ref_pipe_specid);
    JXTA_OBJECT_RELEASE(_ref_membership_specid);
    JXTA_OBJECT_RELEASE(_ref_rendezvous_specid);
    JXTA_OBJECT_RELEASE(_ref_peerinfo_specid);
    JXTA_OBJECT_RELEASE(_ref_endpoint_specid);
    JXTA_OBJECT_RELEASE(_ref_tcpproto_specid);
    JXTA_OBJECT_RELEASE(_ref_httpproto_specid);
    JXTA_OBJECT_RELEASE(_ref_routerproto_specid);
    JXTA_OBJECT_RELEASE(_ref_tlsproto_specid);
    JXTA_OBJECT_RELEASE(_ref_startnetpeergroup_specid);
    JXTA_OBJECT_RELEASE(_ref_shell_specid);
    JXTA_OBJECT_RELEASE(_genericpeergroup_specid);
}

/* FIXME: New API to be implemented
JXTA_DECLARE(Jxta_status) jxta_PG_create(Jxta_PG **me, Jxta_PG *parent, Jxta_MIA *mia)
{
    return JXTA_NOTIMP;
}

JXTA_DECLARE(Jxta_status) jxta_PG_destroy(Jxta_PG *me)
{
    return JXTA_NOTIMP;
}
*/

JXTA_DECLARE(apr_pool_t *) jxta_PG_pool_get(Jxta_PG * me)
{
    return me->pool;
}

JXTA_DECLARE(apr_thread_pool_t *) jxta_PG_thread_pool_get(Jxta_PG * me)
{
    if (NULL == me->thd_pool && me->parent) {
        return jxta_PG_thread_pool_get(me->parent);
    }
    return me->thd_pool;
}

JXTA_DECLARE(Jxta_PG *) jxta_PG_netpg(Jxta_PG * me)
{
    Jxta_PG *npg;

    npg = me;
    while (NULL != npg->parent) {
        npg = npg->parent;
    }
    return npg;
}

JXTA_DECLARE(Jxta_PG *) jxta_PG_parent(Jxta_PG * me)
{
    return me->parent;
}

JXTA_DECLARE(Jxta_status) jxta_PG_get_recipient_addr(Jxta_PG * me, const char * proto_name, const char * proto_addr,
                                                     const char *name, const char *param, Jxta_endpoint_address ** ea)
{
    char *new_param;

    new_param = get_service_key(name, param);
    *ea = jxta_endpoint_address_new_2(proto_name, proto_addr, me->ep_name, new_param);
    free(new_param);

    return *ea ? JXTA_SUCCESS : JXTA_FAILED;
}

JXTA_DECLARE(Jxta_status) jxta_PG_add_recipient(Jxta_PG * me, void **cookie, const char *name, const char *param,
                                                Jxta_callback_func func, void *arg)
{
    Jxta_endpoint_service *ep;
    Jxta_status rv;
    char *new_param;

    new_param = get_service_key(name, param);
    jxta_PG_get_endpoint_service(me, &ep);
    rv = endpoint_service_add_recipient(ep, cookie, me->ep_name, new_param, func, arg);
    free(new_param);
    JXTA_OBJECT_RELEASE(ep);

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_PG_remove_recipient(Jxta_PG * me, void *cookie)
{
    Jxta_endpoint_service *ep;
    Jxta_status rv;

    jxta_PG_get_endpoint_service(me, &ep);
    rv = endpoint_service_remove_recipient(ep, cookie);
    JXTA_OBJECT_RELEASE(ep);
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_PG_sync_send(Jxta_PG * me, Jxta_message * msg, Jxta_id * peer_id, 
                                            const char *svc_name, const char *svc_param)
{
    Jxta_endpoint_service *ep;
    Jxta_status rv;
    char *new_param;
    Jxta_endpoint_address * dest;

    new_param = get_service_key(svc_name, svc_param);
    dest = jxta_endpoint_address_new_3(peer_id, me->ep_name, new_param);
    jxta_PG_get_endpoint_service(me, &ep);
    rv = jxta_endpoint_service_send_ex(ep, msg, dest, JXTA_TRUE);
    free(new_param);
    JXTA_OBJECT_RELEASE(ep);
    JXTA_OBJECT_RELEASE(dest);

    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_PG_async_send(Jxta_PG * me, Jxta_message * msg, Jxta_id * peer_id, 
                                             const char *svc_name, const char *svc_param)
{
    Jxta_endpoint_service *ep;
    Jxta_status rv;
    char *new_param;
    Jxta_endpoint_address * dest;

    new_param = get_service_key(svc_name, svc_param);
    dest = jxta_endpoint_address_new_3(peer_id, me->ep_name, new_param);
    jxta_PG_get_endpoint_service(me, &ep);
    rv = jxta_endpoint_service_send_ex(ep, msg, dest, JXTA_FALSE);
    free(new_param);
    JXTA_OBJECT_RELEASE(ep);
    JXTA_OBJECT_RELEASE(dest);

    return rv;
}

static Jxta_status find_peer_PA_remotely(Jxta_discovery_service *ds, JString *pid, Jxta_time_diff timeout, Jxta_PA ** pa)
{
    Jxta_status rv = JXTA_SUCCESS;
    Jxta_listener *l = NULL;
    Jxta_DiscoveryResponse *dr = NULL;
    long qid;

    l = jxta_listener_new(NULL, pa, 0, 1);
    if (NULL == l) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error when createing listener.\n");
        return JXTA_NOMEM;
    }

    qid = discovery_service_get_remote_advertisements(ds, NULL, DISC_PEER, "PID", jstring_get_string(pid), 1, l);

    rv = jxta_listener_wait_for_event(l, timeout, JXTA_OBJECT_PPTR(&dr));
    if (JXTA_SUCCESS == rv) {
        Jxta_vector * res = NULL;
        Jxta_PA *padv = NULL;

        jxta_discovery_response_get_advertisements(dr, &res);
        if (NULL == res) {
            return JXTA_ITEM_NOTFOUND;
        }

        assert(jxta_vector_size(res) > 0);
        jxta_vector_get_object_at(res, JXTA_OBJECT_PPTR(&padv), 0);
        assert(NULL != padv);
        JXTA_OBJECT_RELEASE(res);
        *pa = padv;
        rv = JXTA_SUCCESS;
    }

    discovery_service_cancel_remote_query(ds, qid, NULL);
    JXTA_OBJECT_RELEASE(l);

    return rv;
}

Jxta_status peergroup_find_peer_PA(Jxta_PG * me, Jxta_id * peer_id, Jxta_time_diff timeout, Jxta_PA ** pa)
{
    Jxta_discovery_service *ds = NULL;
    Jxta_rdv_service *rdv = NULL;
    JString *pid=NULL;
    Jxta_vector *res = NULL;
    Jxta_status rv = JXTA_ITEM_NOTFOUND;
    Jxta_vector *peers=NULL;

    jxta_PG_get_rendezvous_service(me, &rdv);

    jxta_id_to_jstring(peer_id, &pid);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Trying to find PA for peerid:%s\n", jstring_get_string(pid) );

    rv = jxta_rdv_service_get_peers(rdv, &peers);
    if (NULL != peers) {
        int i;

        for (i=0; i<jxta_vector_size(peers); i++) {
            Jxta_peer *peer;
            rv = JXTA_ITEM_NOTFOUND;
            jxta_vector_get_object_at(peers, JXTA_OBJECT_PPTR(&peer), i);
            if (jxta_id_equals(peer_id, jxta_peer_peerid(peer))) {
                jxta_peer_get_adv(peer, pa);
                rv = (NULL != *pa) ? JXTA_SUCCESS:JXTA_ITEM_NOTFOUND;
            }
            JXTA_OBJECT_RELEASE(peer);
            if (JXTA_SUCCESS == rv)
                break;
        }
        JXTA_OBJECT_RELEASE(peers);
    }
    /* if this peer is a rendezvous the peerview may have the adv */
    if ( JXTA_ITEM_NOTFOUND == rv && jxta_rdv_service_is_rendezvous(rdv)) {
        Jxta_peer *peer = NULL;
        Jxta_peerview *pv = jxta_rdv_service_get_peerview(rdv);
        if (NULL != pv) {
            rv = peerview_get_peer(pv, peer_id, &peer);
            if (JXTA_SUCCESS == rv) {
                jxta_peer_get_adv(peer, pa);
                rv = (NULL != *pa) ? JXTA_SUCCESS:JXTA_ITEM_NOTFOUND;
            }
        }
    }
    if (JXTA_ITEM_NOTFOUND == rv) {
        jxta_PG_get_discovery_service(me, &ds);
        discovery_service_get_local_advertisements(ds, DISC_PEER, "PID", jstring_get_string(pid), &res);
        if (res != NULL) {
            Jxta_PA *padv = NULL;
            assert(jxta_vector_size(res) > 0);
            jxta_vector_get_object_at(res, JXTA_OBJECT_PPTR(&padv), 0);
            assert(NULL != padv);
            JXTA_OBJECT_RELEASE(res);
            *pa = padv;
            rv = JXTA_SUCCESS;
        }
    }
    if (JXTA_ITEM_NOTFOUND == rv) {
        if (0 == timeout) {
            /* do a remote query without listener, the result will be published locally. */
            discovery_service_get_remote_advertisements(ds, NULL, DISC_PEER, "PID", jstring_get_string(pid), 1, NULL);
            rv = JXTA_TIMEOUT;
        } else {
            rv = find_peer_PA_remotely(ds, pid, timeout, pa);
        }
    }

    if (pid)
        JXTA_OBJECT_RELEASE(pid);
    if (ds)
        JXTA_OBJECT_RELEASE(ds);
    if (rdv)
        JXTA_OBJECT_RELEASE(rdv);
    return rv;
}

Jxta_status peergroup_find_peer_RA(Jxta_PG * me, Jxta_id * peer_id, Jxta_time_diff timeout, Jxta_RouteAdvertisement ** ra)
{
    Jxta_discovery_service *ds = NULL;
    JString *pid;
    Jxta_vector *res = NULL;
    Jxta_status rv = JXTA_ITEM_NOTFOUND;

    jxta_PG_get_discovery_service(me, &ds);
    jxta_id_to_jstring(peer_id, &pid);

    /* find RA in local cm */
    discovery_service_get_local_advertisements(ds, DISC_ADV, "DstPID", jstring_get_string(pid), &res);
    if (res != NULL) {
        Jxta_RouteAdvertisement *route = NULL;

        assert(jxta_vector_size(res) > 0);
        jxta_vector_get_object_at(res, JXTA_OBJECT_PPTR(&route), 0);
        assert(NULL != route);
        JXTA_OBJECT_RELEASE(res);
        *ra = route;
        rv = JXTA_SUCCESS;
    }

    /* Try to find PA, which will have RA. */
    if (JXTA_ITEM_NOTFOUND == rv) {
        Jxta_PA *pa = NULL;

        rv = peergroup_find_peer_PA(me, peer_id, timeout, &pa);
        if (JXTA_SUCCESS == rv) {
            jxta_endpoint_service_get_route_from_PA(pa, ra);
            JXTA_OBJECT_RELEASE(pa);
        }
    }

    JXTA_OBJECT_RELEASE(pid);
    JXTA_OBJECT_RELEASE(ds);

    return rv;
}

/* vim: set ts=4 sw=4 tw=130 et: */
