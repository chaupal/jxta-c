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
 * $Id: jxta_srdi_service.c,v 1.12 2006/08/19 16:50:57 mmx2005 Exp $
 */

#include "jxta_srdi_service_private.h"

/**
 * The base srdi service ctor (not public: the only public way to make a
 * new pg is to instantiate one of the derived types).
 */
void jxta_srdi_service_construct(Jxta_srdi_service * service, Jxta_srdi_service_methods * methods)
{

    PTValid(methods, Jxta_srdi_service_methods);
    jxta_service_construct((Jxta_service *) service, (Jxta_service_methods *) methods);
    service->thisType = "Jxta_srdi_service";
}

/**
 * The base resolver service dtor (Not public, not virtual. Only called by
 * subclassers). We just pass it along.
 */
void jxta_srdi_service_destruct(Jxta_srdi_service * service)
{
    jxta_service_destruct((Jxta_service *) service);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_srdi_service_methods*) JXTA_MODULE_VTBL(service))


JXTA_DECLARE(Jxta_status) jxta_srdi_replicateEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                     Jxta_SRDIMessage * srdiMsg, JString * queueName)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->replicateEntries(service, resolver, srdiMsg, queueName);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_pushSrdi(Jxta_srdi_service * service, Jxta_resolver_service * resolver, JString * instance,
                                             ResolverSrdi * srdi, Jxta_id * peer)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->pushSrdi(service, resolver, instance, srdi, peer);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_pushSrdi_msg(Jxta_srdi_service * service, Jxta_resolver_service * resolver, JString * instance,
                                             Jxta_SRDIMessage * msg, Jxta_id * peer)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->pushSrdi_msg(service, resolver, instance, msg, peer);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_peer(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                      Jxta_id * peer, ResolverQuery * query)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->forwardQuery_peer(service, resolver, peer, query);
}


JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_peers(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                       Jxta_vector * peers, ResolverQuery * query)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->forwardQuery_peers(service, resolver, peers, query);
}


JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_threshold(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                           Jxta_vector * peers, ResolverQuery * query, int threshold)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->forwardQuery_threshold(service, resolver, peers, query, threshold);
}

JXTA_DECLARE(Jxta_peer *) jxta_srdi_getReplicaPeer(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                   Jxta_peerview * peerview, const char *expression)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->getReplicaPeer(service, resolver, peerview, expression);
}

JXTA_DECLARE(Jxta_peer *) jxta_srdi_getNumericReplica(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                      Jxta_peerview * peerview, Jxta_range * rge, const char *value)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->getNumericReplica(service, resolver, peerview, rge, value);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                       Jxta_peer * peer,
                                                       Jxta_id * srcPid,
                                                       const char *primaryKey, const char *secondarykey, const char *value,
                                                       Jxta_expiration_time expiration)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->forwardSrdiMessage(service, resolver, peer, srcPid, primaryKey, secondarykey, value, expiration);
}

JXTA_DECLARE(Jxta_vector *) jxta_srdi_searchSrdi(Jxta_srdi_service * service, const char *handler, const char *ns,
                                                 const char *attr, const char *val)
{
    PTValid(service, Jxta_srdi_service);
    return VTBL->searchSrdi(service, handler, ns, attr, val);
}
