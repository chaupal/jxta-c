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


#ifndef JXTA_SRDI_SERVICE_PRIVATE_H
#define JXTA_SRDI_SERVICE_PRIVATE_H

#include "jxta_types.h"
#include "jxta_object_type.h"
#include "jxta_id.h"
#include "jxta_peer.h"
#include "jxta_peerview_priv.h"
#include "jxta_srdi_service.h"
#include "jxta_service_private.h"
#include "jxta_range.h"

/****************************************************************
 **
 ** This is the discovery Service API for the implementations of
 ** this service
 **
 ****************************************************************/

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
struct _jxta_srdi_service {
    Extends(Jxta_service);
};

typedef struct _jxta_srdi_service_methods Jxta_srdi_service_methods;

struct _jxta_srdi_service_methods {

    Extends(Jxta_service_methods);

    Jxta_status(*replicateEntries) (Jxta_srdi_service * self,
                                    Jxta_resolver_service * resolver, Jxta_SRDIMessage * srdiMsg, JString * queueName);

    Jxta_status(*pushSrdi) (Jxta_srdi_service * self, Jxta_resolver_service * res, JString * instance, ResolverSrdi * srdi, Jxta_id * peer);

    Jxta_status(*pushSrdi_msg) (Jxta_srdi_service * self, Jxta_resolver_service * res, JString * instance, Jxta_SRDIMessage * srdi, Jxta_id * peer);

    Jxta_status(*forwardQuery_peer) (Jxta_srdi_service * self,
                                     Jxta_resolver_service * resolver, Jxta_id * peer, ResolverQuery * query);

    Jxta_status(*forwardQuery_peers) (Jxta_srdi_service * self,
                                      Jxta_resolver_service * resolver, Jxta_vector * peers, ResolverQuery * query);

    Jxta_status(*forwardQuery_threshold) (Jxta_srdi_service * self,
                                          Jxta_resolver_service * resolver,
                                          Jxta_vector * peers, ResolverQuery * query, int threshold);

    Jxta_peer *(*getReplicaPeer) (Jxta_srdi_service * self, const char *expression, Jxta_peer **alt_peer);

    Jxta_peer *(*getNumericReplica) (Jxta_srdi_service * self, Jxta_range *rge, const char * value);
    
    Jxta_status(*forwardSrdiMessage) (Jxta_srdi_service * self, Jxta_resolver_service * resolver, Jxta_peer * peer,
				      Jxta_id * srcPid, const char *primaryKey, const char *secondarykey, const char *value,
				      Jxta_expiration_time expiration);

    Jxta_vector * (*searchSrdi)(Jxta_srdi_service *me, const char * handler, const char *ns, const char *attr, const char *val);
};

/**
 * The base discovery service ctor (not public: the only public way to make a
 * new pg is to instantiate one of the derived types).
 */
extern void jxta_srdi_service_construct(Jxta_srdi_service * service, Jxta_srdi_service_methods const * methods);

/**
 * The base resolver service dtor (Not public, not virtual. Only called by
 * subclassers). We just pass it along.
 */
extern void jxta_srdi_service_destruct(Jxta_srdi_service * service);

#if 0
{
#endif
#ifdef __cplusplus
}
#endif

#endif /* JXTA_DISCOVERY_SERVICE_PRIVATE_H */
