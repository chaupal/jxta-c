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
 * $Id: jxta_transport_private.h,v 1.18 2006/09/06 21:45:18 slowhog Exp $
 */

#ifndef JXTA_TRANSPORT_PRIVATE_H
#define JXTA_TRANSPORT_PRIVATE_H

#include "jxta_transport.h"
#include "jxta_module_private.h"
#include "jxta_object_type.h"


/*
 * This is an internal header file that defines the interface between
 * the transport API and the various implementations.
 * An implementation of transport must define a static Table as below
 * or a derivate of it.
 */

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_transport_methods Jxta_transport_methods;

typedef enum Jxta_transport_event_types {
    JXTA_TRANSPORT_INBOUND_CONNECT = 0
} Jxta_transport_event_type;

typedef struct _jxta_transport_event {
    Extends(Jxta_object);
    Jxta_transport_event_type type;
    /* peer id of remote peer */
    Jxta_id *peer_id;
    /* endpoint address in the welcome msg */
    Jxta_endpoint_address *dest_addr;
} Jxta_transport_event;

Jxta_transport_event *jxta_transport_event_new(Jxta_transport_event_type event_type);

struct _jxta_transport_methods {
    Extends(Jxta_module_methods);

    /* Transport specific API */
    JString *(*name_get) (Jxta_transport * self);
    Jxta_endpoint_address *(*publicaddr_get) (Jxta_transport * self);
    JxtaEndpointMessenger *(*messenger_get) (Jxta_transport * self, Jxta_endpoint_address * there);

     Jxta_boolean(*ping) (Jxta_transport * self, Jxta_endpoint_address * there);

    void (*propagate) (Jxta_transport * self, Jxta_message * msg, const char *service_name, const char *service_params);

     Jxta_boolean(*allow_overload_p) (Jxta_transport * self);
     Jxta_boolean(*allow_routing_p) (Jxta_transport * self);
     Jxta_boolean(*connection_oriented_p) (Jxta_transport * self);
};

struct _jxta_transport {
    Extends(Jxta_module);

    /* Implementors put their stuff after self */
    int metric;
    Jxta_direction direction;
    apr_pool_t *p;
    apr_hash_t *qos;
};

typedef struct _jxta_transport _jxta_transport;

/**
 * The base transport ctor (not public: the only public way to make a new transport
 * is to instantiate one of the derived types). Derivates call this
 * to initialize the base part.
 */
extern Jxta_transport *jxta_transport_construct(Jxta_transport * self, Jxta_transport_methods const *methods);

Jxta_status jxta_transport_init(Jxta_transport * me, apr_pool_t * p);

/**
 * The base transport dtor (Not public, not virtual. Only called by subclassers).
 */
extern void jxta_transport_destruct(Jxta_transport * self);

int jxta_transport_metric_get(Jxta_transport * me);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_TRANSPORT_PRIVATE_H */

/* vim: set ts=4 sw=4 et tw=130: */
