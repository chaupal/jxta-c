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


#ifndef JXTA_RESOLVER_SERVICE_PRIVATE_H
#define JXTA_RESOLVER_SERVICE_PRIVATE_H

#include "jxta_resolver_service.h"
#include "jxta_service_private.h"


/****************************************************************
 **
 ** This is the resolver Service API for the implementations of
 ** this service
 **
 ****************************************************************/

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

struct _jxta_resolver_service {
    Extends(Jxta_service);

};

typedef struct _jxta_resolver_service_methods Jxta_resolver_service_methods;

struct _jxta_resolver_service_methods {

    Extends(Jxta_service_methods);

    /* resolver methods */
    Jxta_status(*registerQueryHandler) (Jxta_resolver_service * service, JString * name, Jxta_callback * handler);

    Jxta_status(*unregisterQueryHandler) (Jxta_resolver_service * service, JString * name);

    Jxta_status(*registerSrdiHandler) (Jxta_resolver_service * service, JString * name, Jxta_listener * handler);

    Jxta_status(*unregisterSrdiHandler) (Jxta_resolver_service * service, JString * name);

    Jxta_status(*registerResponseHandler) (Jxta_resolver_service * service, JString * name, Jxta_listener * handler);

    Jxta_status(*unregisterResponseHandler) (Jxta_resolver_service * service, JString * name);

    Jxta_status(*sendQuery) (Jxta_resolver_service * resolver, ResolverQuery * query, Jxta_id * peerid);

    Jxta_status(*sendResponse) (Jxta_resolver_service * resolver, ResolverResponse * response, Jxta_id * peerid, apr_int64_t *max_length);

    Jxta_status(*sendSrdi) (Jxta_resolver_service * resolver, ResolverSrdi * message, Jxta_id * peerid, Jxta_boolean sync);

    Jxta_status(*create_query) (Jxta_resolver_service * me, JString * handlername, JString * query, Jxta_resolver_query ** rq);

    void(*setAlwaysPropagate) (Jxta_resolver_service * service, Jxta_boolean propagate);

    Jxta_boolean(*alwaysPropagate) (Jxta_resolver_service * service);
};

/**
 * The base resolver service ctor (not public: the only public way to make a
 * new pg is to instantiate one of the derived types).
 */
extern void jxta_resolver_service_construct(Jxta_resolver_service * service, Jxta_resolver_service_methods * methods);

/**
 * The base rsesolver service dtor (Not public, not virtual. Only called by
 * subclassers). We just pass it along.
 */
extern void jxta_resolver_service_destruct(Jxta_resolver_service * service);

Jxta_status resolver_query_create(JString * handlername, JString * qdoc, Jxta_id * src_peerid, Jxta_resolver_query ** rq);
                                                        

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_RESOLVER_SERVICE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
