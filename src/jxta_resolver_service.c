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

#include "jxta_resolver_service_private.h"

/**
 * The base resolver service ctor (not public: the only public way to make a
 * new pg is to instantiate one of the derived types).
 */
void jxta_resolver_service_construct(Jxta_resolver_service * service, Jxta_resolver_service_methods * methods)
{

    Jxta_resolver_service_methods* service_methods = PTValid(methods, Jxta_resolver_service_methods);
    jxta_service_construct((Jxta_service *) service, (Jxta_service_methods *) service_methods);
    service->thisType = "Jxta_resolver_service";
}

/**
 * The base rsesolver service dtor (Not public, not virtual. Only called by
 * subclassers). We just pass it along.
 */
void jxta_resolver_service_destruct(Jxta_resolver_service * service)
{
    jxta_service_destruct((Jxta_service *) service);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_resolver_service_methods*) JXTA_MODULE_VTBL(service))

/**
 * Registers a given Resolver Query Handler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 *
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_registerQueryHandler(Jxta_resolver_service * service, JString * name,
                                                                     Jxta_callback * handler)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->registerQueryHandler(resolver_service, name, handler);
}

/**
  * unregisters a given Resolver Query Handler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_unregisterQueryHandler(Jxta_resolver_service * service, JString * name)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->unregisterQueryHandler(resolver_service, name);
}

/**
 * Registers a given Resolver Srdi Handler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 *
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_registerSrdiHandler(Jxta_resolver_service * service, JString * name, Jxta_listener * handler)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->registerSrdiHandler(resolver_service, name, handler);
}

/**
  * unregisters a given Resolver Srdi Handler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_unregisterSrdiHandler(Jxta_resolver_service * service, JString * name)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->unregisterSrdiHandler(resolver_service, name);
}

/**
 * Registers a given Resolver Response Handler.
 *
 * @param name The name under which this handler is to be registered.
 * @param handler The handler.
 * @return Jxta_status
 *
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_registerResHandler(Jxta_resolver_service * service, JString * name, Jxta_listener * handler)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->registerResponseHandler(resolver_service, name, handler);
}

/**
  * unregisters a given  Resolver Response Handler.
  *
  * @param name The name of the handler to unregister.
  * @return error code
  *
  */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_unregisterResHandler(Jxta_resolver_service * service, JString * name)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->unregisterResponseHandler(resolver_service, name);
}

/**
 * For Services that wish to implement a ResolverService Service they must
 * implement this interface Sends query to the specified address.
 * If address is null the query is propagated 
 *
 * @param query The query to match
 * @param addr  Peer address (unicast) , or NULL (propagate)
 *
 * @since JXTA 1.0
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_sendQuery(Jxta_resolver_service * service, ResolverQuery * query, Jxta_id * peerid)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->sendQuery(resolver_service, query, peerid);
}

/**
 * send a response to a peer
 * @param response is the response to be sent
 * @param addr  Peer address (unicast) , or NULL (propagate)
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_sendResponse(Jxta_resolver_service * service, ResolverResponse * response, Jxta_id * peerid, apr_int64_t *max_length)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->sendResponse(resolver_service, response, peerid, max_length);
}

/**
 * send an srdi message to a peer
 * @param message is the srdi message to be sent
 * @param addr  Peer address (unicast) , or NULL (propagate)
 * @param sync If TRUE send the message asynchronously
 */
JXTA_DECLARE(Jxta_status) jxta_resolver_service_sendSrdi(Jxta_resolver_service * service, ResolverSrdi * message, Jxta_id * peerid, Jxta_boolean sync, apr_int64_t *max_length)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->sendSrdi(resolver_service, message, peerid, sync, max_length);
}

JXTA_DECLARE(Jxta_status) jxta_resolver_service_create_query(Jxta_resolver_service * service, JString * handlername, 
                                                             JString * query, Jxta_resolver_query ** rq)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->create_query(resolver_service, handlername, query, rq);
}

/**
 * Peergroups that would like the resolver to both walk and propagate a query can use this
 * interface to override the Resolver Service Config provided in the PlatformConfig
 *
 * @param pointer to the Jxta_resolver_service
 * @param propagate flag indicating whether to propagate or not.  TRUE will send both a walk and propagate message.
 */
JXTA_DECLARE(void) jxta_resolver_service_set_always_propagate(Jxta_resolver_service * service,
                                                              Jxta_boolean propagate)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    VTBL->setAlwaysPropagate(resolver_service, propagate);
}

/**
 * Returns the always propagate setting for the current group
 *
 * @param service a pointer to the instance of the resolver service
 * @return the boolean indicating the propagation behavior
 */
JXTA_DECLARE(Jxta_boolean) jxta_resolver_service_always_propagate(Jxta_resolver_service * service)
{
    Jxta_resolver_service* resolver_service = PTValid(service, Jxta_resolver_service);
    return VTBL->alwaysPropagate(resolver_service);
}

/* vim: set ts=4 sw=4 et tw=130: */
