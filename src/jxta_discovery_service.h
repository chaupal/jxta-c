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

/**
 * The discovery service provides an asynchronous mechanism for discovering
 *   Peer Advertisements, Group Advertisements, and other general jxta
 *   Advertisements (pipe, service, etc.). The scope of discovery can be
 *   controlled by specifying name and attribute pair, and a threshold. The
 *   threshold is an upper limit the requesting peer specifies for responding
 *   peers not to exceed. Each jxta Peer Group has an instance of a
 *   Jxta_discovery_service the scope of the discovery is limited to the group. for
 *   example <p>
 *   A peer in the soccer group invokes the soccer group's Jxta_discovery_service to
 *   discover pipe advertisements in the group, and is interested in a maximum of
 *   10 Advertisements from each peer: <pre>
 *   status = discovery_service_get_remote_advertisements(discovery,null, DISC_ADV,
 *                                       null, null,10, null);
 *  </pre> <p>
 *   in the above example, peers that are part of the soccer group would respond.
 *   After a getRemoteAdvertisements call is made and the peers respond, a call
 *   to getLocalAdvertisements can be made to retrieve results that have been
 *   found and added to the local group cache. Alternately, a call to
 *   addDiscoveryListener() will provide asynchronous notification of discovered
 *   advertisements. <p>
 *   Jxta_discovery_service also provides a mechanism for publishing advertisements, so
 *   that they may be discovered. The rules to follow when publishing are
 *   <ultype-disc>
 *     <li> <p>
 *     use the current discovery service to publish advertisements private to the
 *     group. <pre>
 *         status = publish(discovery,adv,DISC_ADV);
 *  </pre>
 *     <li> <p>
 *     use the parent's discovery to publish advertisements that public outside
 *     of the group. e.g. peer A would like publish the soccer group in the
 *     NetPeerGroup <pre>
 *         status = publish(parent_discovery, adv,discovery.ADV);
 *  </pre>
 *   </ul>
 *   <pe>
 *   The threshold can be utilized in peer discovery in situations where a peer
 *   is only interested in other peers, and not about additional peers they may
 *   know about. to achieve this effect for peer discovery set the Threshold to 0
 *   <p/>
 *   Advertisements are stored in a persistent local cache. When a peers boots
 *   up the same cache is referenced. this is an area where several optimizations
 *   can take place, and intelligence about discovery patterns, etc. <p/>
 *   Another feature of discovery is automatic discovery, a peer initiates a
 *   discovery message by including it's own Advertisement in the discovery
 *   message, which also can be viewed as a announcement. <p/>
 *   Another feature of discovery is a learning about other rendezvous, i.e. when
 *   discovery comes across a peer advertisement of a rendezvous peer it passes
 *   the information down to the endpoint router. <p/>
 *   Message Format : <p/>
 *   A discovery Query <p/>
 *   <pre>
 *       &lt;?xml version="1.0" ?&gt;
 *        &lt;DiscoveryQuery&gt;
 *         &lt;Type&gt;int&lt;/Type&gt;
 *         &lt;Threshold&gt;int&lt;/Threshold&gt;
 *         &lt;PeerAdv&gt;peeradv&lt;/PeerPdv&gt;
 *         &lt;Attr&gt;attribute&lt;/Attr&gt;
 *         &lt;Value&gt;value&lt;/Value&gt;
 *        &lt;/DiscoveryQuery&gt;
 *  </pre> <p/>
 *   A discovery Response <p/>
 *   <pre>
 *       &lt;?xml version="1.0"?&gt;
 *        &lt;DiscoveryResponse&gt;
 *         &lt;Count&gt;int&lt;Count&gt;
 *         &lt;Type&gt;int&lt;/Type&gt;
 *         &lt;PeerAdv&gt; adv &lt;/PeerAdv&gt;
 *         &lt;Attr&gt; attribute &lt;/Attr&gt;
 *         &lt;Value&gt; value &lt;/Aalue&gt;
 *         &lt;Response Expiration="expiration" &gt;adv&lt;/Response&gt;
 *         ......
 *         &lt;Response Expiration="expiration" &gt;adv&lt;/Response&gt;
 *       &lt;/DiscoveryResponse&gt;
 *  </pre>
 */

#ifndef JXTA_DISCOVERY_SERVICE_H
#define JXTA_DISCOVERY_SERVICE_H

#include "jxta_service.h"
#include "jxta_listener.h"

#include "jxta_id.h"
#include "jxta_advertisement.h"
#include "jxta_vector.h"
#include "jxta_dq.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define DISC_PEER    0
#define DISC_GROUP   1
#define DISC_ADV     2

#define DEFAULT_LIFETIME ((Jxta_expiration_time) 1000 * 60 * 60 * 24 * 365)
#define DEFAULT_EXPIRATION ((Jxta_expiration_time) 1000 * 60 * 60 * 2)
#define LOCAL_ONLY_EXPIRATION ((Jxta_expiration_time) 0)

typedef struct _jxta_discovery_service Jxta_discovery_service;
typedef struct _DiscoveryEvent DiscoveryEvent;
typedef Jxta_listener Jxta_discovery_listener;

/**
 * This method discovers PeerAdvertisements, GroupAdvertisements and jxta
 * Advertisements. jxta Advertisements are documents that describe pipes,
 * services, etc. The discovery scope can be narrowed down firstly by Name and
 * Value pair where the match is an exact match, secondly by setting a upper
 * limit where the responding peer will not exceed. Jxta_discovery_serviceImpl can
 * be performed in two ways 1. by specifying a null peerid, the discovery
 * message is propagated on the local sub-net utilizing ip multicast. In
 * addition to the multicast it is also propagated to rendezvous points. 2. by
 * passing a peerid, the EndpointRouter will attempt to resolve destination
 * peer's endpoints or route the message to other routers in attempt to reach
 * the peer
 * @param  Jxta_discovery_service the service 
 * @param  peerid peerid of the peer to send the query to. A NULL ID causes a propagation
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  aattribute  attribute name to narrow discovery to
 * @param  value value of attribute to narrow discovery to
 * @param  threshold the upper limit of responses from one peer
 * @param  listener the listener which will be called back when responses are received
 * @return query id
 */
JXTA_DECLARE(long) discovery_service_get_remote_advertisements(Jxta_discovery_service * service,
                                                               Jxta_id * peerid,
                                                               short type,
                                                               const char *aattribute,
                                                               const char *value, int threshold,
                                                               Jxta_discovery_listener * listener);

JXTA_DECLARE(long) discovery_service_remote_query(Jxta_discovery_service * service,
                                                  Jxta_id * peerid, const char *query, int threshold,
                                                  Jxta_discovery_listener * listener);

/**
 * Send a discovery query to remote peers for execution.
 * @param  me the Jxta_discovery_service 
 * @param  peerid peerid of the peer to send the query to. A NULL ID causes a propagation
 * @param  query The query
 * @param  listener the listener which will be called back when responses are received
 * @return query id
 */
JXTA_DECLARE(long) discovery_service_remote_query_1(Jxta_discovery_service * me, Jxta_id * peerid, 
                                                    Jxta_discovery_query * query, Jxta_discovery_listener * listener);

/**
 * Stop process responses for remote queries issued earlier by calling either discovery_service_get_remote_advertisements or
 * discovery_service_remote_query. Remove the listener from discovery service.
 *
 * @param  Jxta_discovery_service the service 
 * @param  query_id the query ID returned by previous call to issue remote query
 * @param  listener a pointer to a Jxta_listener pointer to receive the removed listener. Pass NULL if the value is not needed,
 * and the Jxta_listener object will be released automatically in the case.
 *
 * @return Jxta_status JXTA_ITEM_NOT_FOUND if the query ID cannot be found, JXTA_SUCCESS otherwise
 */
JXTA_DECLARE(Jxta_status) discovery_service_cancel_remote_query(Jxta_discovery_service * me, long query_id,
                                                                Jxta_discovery_listener ** listener);

/**
 * Retrieve Stored Peer, Group, and General JXTA Advertisements
 * @param  Jxta_discovery_service the service 
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  aattribute  attribute name to narrow discovery to
 * @param  value value of attribute to narrow discovery to
 * @param  advertisements pointer to the Jxta_vector to return
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_get_local_advertisements(Jxta_discovery_service * service,
                                                                     short type, const char *aattribute, const char *value,
                                                                     Jxta_vector ** advertisements);

/**
 * Query the local cache with an XPath type query
 * @param service - Jxta_discovery_service the service
 * @param query - an XPath type like query 
 * @param advertisements - pointer to the Jxta_vector for return
 *
 */
JXTA_DECLARE(Jxta_status) discovery_service_local_query(Jxta_discovery_service * service, const char *query,
                                                        Jxta_vector ** advertisements);

/**
 * Publish an advertisement that will expire after a certain time. A node that
 * finds this advertisement will hold it for about <i> lifetimeForOthers</i>
 * milliseconds, or <i>lifetime</i> whichever is smaller
 * @param  Jxta_discovery_service the service 
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  lifetime the amount of time this advertisement will
 * live in my cache specified in milliseconds
 * @param  expriation  the amount of time this advertisement will
 * live in other peer's cache specified in milliseconds
 * @param   Jxta_advertisement to publish
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_publish(Jxta_discovery_service * service,
                                                    Jxta_advertisement * adv,
                                                    short type, Jxta_expiration_time lifetime, Jxta_expiration_time expriation);

/**
 * Remote Publish an advertisement will attempt to remote publish adv on all
 * configured transports, the Advertisement will carry a a expiration of <i>
 * expirationtime</i>
 * @param  Jxta_discovery_service the service 
 * @param   Jxta_advertisement to publish
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  expriation  the amount of time this advertisement will expire on other peer's cache specified in milliseconds
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_remote_publish(Jxta_discovery_service * service,
                                                           Jxta_id * peerid, Jxta_advertisement * adv, short type,
                                                           Jxta_expiration_time expriation);

/**
 * Remote Publish an advertisement will attempt to remote publish adv on all
 * configured transports, the Advertisement will carry a a expiration of <i>
 * expirationtime</i>
 * @param  Jxta_discovery_service the service 
 * @param  Jxta_advertisement to publish
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  expriation  the amount of time this advertisement will expire on other peer's cache specified in milliseconds
 * @param  qos The QoS setting to be used for this publish
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_remote_publish_1(Jxta_discovery_service * service,
                                                             Jxta_id * peerid, Jxta_advertisement * adv, short type,
                                                             Jxta_expiration_time expriation, const Jxta_qos * qos);

/**
 * flush s stored Advertisement
 * @param  Jxta_discovery_service the service
 * @param  id  id of the advertisement to flush
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_flush_advertisements(Jxta_discovery_service * service, char *id, short type);

/**
 * register a discovery listener, to notified on discovery events
 * @param  Jxta_discovery_service the service
 * @param  Jxta_discovery_listener the discovery listener to register
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_add_discovery_listener(Jxta_discovery_service * service,
                                                                   Jxta_discovery_listener * listener);

/**
 * remove a discovery listener
 * @param  Jxta_discovery_service the service
 * @param  Jxta_discovery_listener to remove
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_remove_discovery_listener(Jxta_discovery_service * service,
                                                                      Jxta_discovery_listener * listener);

/**
 * Get an advertisement's lifetime value
 *
 * @param  Jxta_discovery_service the service
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  Advertisement ID
 * @param  Location to store lifetime value
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_get_lifetime(Jxta_discovery_service * service, short type, Jxta_id * advId,
                                                         Jxta_expiration_time * expriation);

/**
 * Get an advertisement's expiration value.
 *
 * @param  Jxta_discovery_service the service
 * @param  type  DISC_PEER, DISC_GROUP, or DISC_ADV
 * @param  Advertisement ID
 * @param  Location to store expiration value
 * @return Jxta_status
 * @see Jxta_status
 */
JXTA_DECLARE(Jxta_status) discovery_service_get_expiration(Jxta_discovery_service * service, short type, Jxta_id * advId,
                                                           Jxta_expiration_time * expriation);

/**
 *
 */
JXTA_DECLARE(void) discovery_service_flush_pending_deltas(Jxta_discovery_service * service);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_DISCOVERY_SERVICE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
