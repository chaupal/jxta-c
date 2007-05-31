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
 * $Id: jxta_resolver_service.h,v 1.2 2005/01/10 17:19:27 brent Exp $
 */

#ifndef JXTA_RESOLVER_SERVICE_H
#define JXTA_RESOLVER_SERVICE_H

#include "jxta_service.h"

#include "jstring.h"
#include "jxta_rq.h"
#include "jxta_rr.h"
#include "jxta_rsrdi.h"
#include "jxta_id.h"
#include "jxta_listener.h"


/**
 * ResolverService provides a generic mechanism for JXTA Services
 * to send "Queries", and receive "Responses".  It removes the burden for
 * registered handlers in deal with :
 *
 *<ul type-disc>
 *    <li><p>Setting message tags, to ensure uniqueness of tags and
 *     ensures that messages are sent to the correct address, and group
 *    <li><p>Authentication, and Verification of credentials
 *    <li><p>drop rogue messages
 *</ul>
 *
 * <p/>The ResolverService does not proccess the queries, nor does it not compose
 * reponses. Handling of queries, and composition of responses are left up
 * to the registered handlers. Services that wish to handle queries,
 * and generate reponses must implement
 *
 * <p/>Message Format:
 *
 * <ul><li>A Query message:
 *
 * <pre>&lt;?xml version="1.0" standalone='yes'?&gt;
 * &lt;ResolverQuery&gt;
 *   &lt;handlername&gt; name &lt;/handlername&gt;
 *   &lt;credentialServiecUri&gt; uri &lt;/credentialServiecUri&gt;
 *   &lt;credentialToken&gt; token &lt;/credentialToken&gt;
 *   &lt;srcpeerid&gt; srcpeerid &lt;/srcpeerid&gt;
 *   &lt;queryid&gt; id &lt;/queryid&gt;
 *   &lt;query&gt; query &lt;/query&gt;
 * &lt;/ResolverQuery&gt;</pre>
 *
 * <p/>Note: queryid is unique to the originating node only, it can be utilized to
 * match queries to responses.</p></li>
 *
 * <li>A Response Message:
 *
 * <pre>&lt;?xml version="1.0" standalone='yes'?&gt;
 * &lt;ResolverResponse&gt;
 *   &lt;handlername&gt; name &lt;/handlername&gt;
 *   &lt;credentialServiecUri&gt; uri &lt;/credentialServiecUri&gt;
 *   &lt;credentialToken&gt; token &lt;/credentialToken&gt;
 *   &lt;queryid&gt; id &lt;/queryid&gt;
 *   &lt;response&gt; response &lt;/response&gt;
 * &lt;/ResolverResponse&gt;</pre>
 *
 * <p/>Note: queryid is unique to the originating node only, it can be
 * utilized to match queries to responses.</li></ul>
 *
 */

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _jxta_resolver_service  Jxta_resolver_service;

/**
 * Registers a given Resolver Query Handler.
 *
 * @param service pointer to the Jxta_resolver_service
 * @param name the name under which this handler is to be registered, typically
 * jxta services utilize the module id as the name
 * @param Jxta_listener handler the handler.
 * @return Jxta_status
 * @see Jxta_status
 *
 */
Jxta_status
jxta_resolver_service_registerQueryHandler(Jxta_resolver_service * service,
                                           JString* name,
                                           Jxta_listener * handler);

/**
 * unregisters a given Resolver Query Handler.
 *
 * @param service pointer to the Jxta_resolver_service
 * @param name The name of the handler to unregister.
 * @return Jxta_status
 * @see Jxta_status
 *
 */    
Jxta_status
jxta_resolver_service_unregisterQueryHandler(Jxta_resolver_service * service,
                                             JString* name);
/**
 * Registers a given Resolver Response Handler.
 *
 * @param service pointer to the Jxta_resolver_service
 * @param name The name under which this handler is to be registered, typically
 * jxta services utilize the module id as the name
 * @param Jxta_listener handler the handler.
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status
jxta_resolver_service_registerResHandler(Jxta_resolver_service * service,
                                         JString* name,
                                         Jxta_listener * handler);

/**
 * unregisters a given Resolver Response Handler.
 *
 * @param service pointer to the Jxta_resolver_service
 * @param name The name of the handler to unregister.
 * @return Jxta_status
 * @see Jxta_status
 */    
Jxta_status
jxta_resolver_service_unregisterResHandler(Jxta_resolver_service * service,
                                        JString* name);

/**
 * For Services that wish to implement a ResolverService Service they must
 * implement this interface Sends query to the specified address.
 * If address is null the query is propagated 
 *
 * @param service pointer to the Jxta_resolver_service
 * @param query The query 
 * @param addr Peer address (unicast) , or NULL (propagate)
 * @return Jxta_status
 * @see Jxta_status
 */

Jxta_status
jxta_resolver_service_sendQuery(Jxta_resolver_service* service,
                                ResolverQuery* query,
                                Jxta_id* peerid);

/**
 * send a response to a peer
 *
 * @param service pointer to the Jxta_resolver_service
 * @param response is the response to be sent
 * @param peerid  Peer address (unicast) , or NULL (propagate)
 * @return Jxta_status
 * @see Jxta_status
 */
Jxta_status
jxta_resolver_service_sendResponse(Jxta_resolver_service* service,
                                   ResolverResponse* response,
                                   Jxta_id* peerid);
/**
 * For Services that wish to implement a ResolverService Service they must
 * implement this interface Sends query to the specified address.
 * If address is null the query is propagated 
 *
 * @param pointer to the Jxta_resolver_service
 * @param message the Srdi message to be sent
 * @param addr Peer address (unicast) , or NULL (propagate)
 * @return Jxta_status
 * @see Jxta_status
 */

Jxta_status
jxta_resolver_service_sendSrdi(Jxta_resolver_service* service, 
                               ResolverSrdi* message,
                               Jxta_id* peerid);
/**
 *
 * Create a new message id
 *
 * @param group id
 * @param pointer to a Jstring which conatin the new id
 * 
 * @return Jxta_status
 */
Jxta_status message_id_new(Jxta_id* pgid, JString** sid);

#ifdef __cplusplus
}
#endif

#endif /* JXTA_RESOLVER_SERVICE_H */
