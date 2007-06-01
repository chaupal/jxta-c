/*
 * Copyright (c) 2002-2005 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_srdi_service.h,v 1.9 2005/11/23 16:07:03 slowhog Exp $
 */

#ifndef JXTA_SRDI_SERVICE_H
#define JXTA_SRDI_SERVICE_H

#include "jxta_types.h"
#include "jxta_id.h"
#include "jxta_peer.h"
#include "jxta_peerview.h"
#include "jxta_range.h"
#include "jxta_srdi.h"
#include "jxta_resolver_service.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_srdi_service Jxta_srdi_service;

/**
 * Replicates a SRDI message to other rendezvous'
 * entries are replicated by breaking out entries out of the message
 * and sorted out into rdv distribution bins. after which smaller messages
 * are sent to other rdv's
 *
 * @param  srdiMsg srdi message to replicate
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_replicateEntries(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                     Jxta_SRDIMessage * srdiMsg, JString * queueName);

/**
 * Push an SRDI message to a peer
 * ttl is 1, and therefore services receiving this message could
 * choose to replicate this message
 *
 * @param  peer  peer to push message to, if peer is null it is
 *               the message is propagated
 * @param  srdi  SRDI message to send
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_pushSrdi(Jxta_srdi_service * service, Jxta_resolver_service * res, ResolverSrdi * msg,
                                             Jxta_id * peer);

/**
 * Forwards a Query to a specific peer
 * hopCount is incremented to indicate this query is forwarded
 *
 * @param  peer        peerid to forward query to
 * @param  query       The query
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_peer(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                      Jxta_id * peer, ResolverQuery * query);

/**
 * Forwards a Query to a list of peers
 * hopCount is incremented to indicate this query is forwarded
 *
 * @param  peers       The peerids to forward query to
 * @param  query       The query
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_peers(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                       Jxta_vector * peers, ResolverQuery * query);

/**
 * Forwards a Query to a list of peers
 * if the list of peers exceeds threshold, and random threshold is picked
 * from <code>peers</code>
 * hopCount is incremented to indicate this query is forwarded
 * 
 * @param  peers       The peerids to forward query to
 * @param  query       The query
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_forwardQuery_threshold(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                           Jxta_vector * peers, ResolverQuery * query, int threshold);

/**
 * Given an expression return a peer from the list peers in the peerview
 * this function is used to to give a replication point, and entry point
 * to query on a pipe
 *
 * @param  expression  expression to derive the mapping from
 * @return             The replicaPeer value
 */
JXTA_DECLARE(Jxta_peer *) jxta_srdi_getReplicaPeer(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                   Jxta_peerview * pvw, const char *expression);

/**
 * Given an expression return a peer from the list peers in the peerview
 * this function is used to to give a replication point for numeric values
 * given a high and low range for the value.
 *
 * @param Jxta_srdi_service * service - SRDI service for this group
 * @param Jxta_resolver_service * resolver - Resolver service within the group
 * @param Jxta_object * pvw - Peerview contains the list of peers for replication
 * @param Jxta_range * rge - High and low range for the value
 * @param double value - Value to be applied to find the position in the range.
 * @param  expression  expression to derive the mapping from
 * @return             The replicaPeer value
 */
JXTA_DECLARE(Jxta_peer *) jxta_srdi_getNumericReplica(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                      Jxta_peerview * pvw, Jxta_object * rge, const char *value);

/**
 * forward srdi message to another peer
 *
 * @param  peerid      PeerID to forward query to
 * @param  srcPid      The source originator
 * @param  primaryKey  primary key
 * @param  secondarykey secondary key
 * @param  value       value of the entry
 * @param  expiration  expiration in ms
 */
JXTA_DECLARE(Jxta_status) jxta_srdi_forwardSrdiMessage(Jxta_srdi_service * service, Jxta_resolver_service * resolver,
                                                       Jxta_peer * peer,
                                                       Jxta_id * srcPid,
                                                       const char *primaryKey,
                                                       const char *secondarykey, const char *value,
                                                       Jxta_expiration_time expiration);

/**
 * Search in the local srdi cache for the given handler, namespace, attr and value.
 *
 * @param  me          The self pointer to the instance of SRDI service
 * @param  handler     Name of the handler who owns the SRDI entries
 * @param  ns          Namespace for the attribute, NULL for any value
 * @param  attr        Attr looked for 
 * @param  val         Value for the attr looked for
 * @return The vector of matching SRDI entries
 */
JXTA_DECLARE(Jxta_vector *) jxta_srdi_searchSrdi(Jxta_srdi_service * me, const char *handler, const char *ns, 
                                                 const char *attr, const char *val);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
/* vim: set ts=4 sw=4 et tw=130: */
