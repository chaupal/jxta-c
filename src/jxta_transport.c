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
 * $Id: jxta_transport.c,v 1.13 2005/08/03 05:51:20 slowhog Exp $
 */

#include "jxta_transport_private.h"

/**
 * The base transport ctor (not public: the only public way to make a new transport
 * is to instantiate one of the derived types).
 */
Jxta_transport *jxta_transport_construct(Jxta_transport * self, Jxta_transport_methods const *methods)
{

    PTValid(methods, Jxta_transport_methods);
    jxta_module_construct(self, (Jxta_module_methods const *) methods);

    self->thisType = "Jxta_transport";

    return self;
}

/**
 * The base transport dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 */
void jxta_transport_destruct(Jxta_transport * self)
{
    jxta_module_destruct((Jxta_module *) self);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_transport_methods*) JXTA_MODULE_VTBL(self))

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(JString *) jxta_transport_name_get(Jxta_transport * self)
{
    PTValid(self, Jxta_transport);
    return VTBL->name_get(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_publicaddr_get(Jxta_transport * self)
{
    PTValid(self, Jxta_transport);
    return VTBL->publicaddr_get(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(JxtaEndpointMessenger *) jxta_transport_messenger_get(Jxta_transport * self, Jxta_endpoint_address * there)
{
    PTValid(self, Jxta_transport);
    return VTBL->messenger_get(self, there);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_ping(Jxta_transport * self, Jxta_endpoint_address * there)
{
    PTValid(self, Jxta_transport);
    return VTBL->ping(self, there);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(void) jxta_transport_propagate(Jxta_transport * self, Jxta_message * msg, const char *service_name, const char *service_params)
{
    PTValid(self, Jxta_transport);
    VTBL->propagate(self, msg, service_name, service_params);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_overload_p(Jxta_transport * self)
{
    PTValid(self, Jxta_transport);
    return VTBL->allow_overload_p(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_routing_p(Jxta_transport * self)
{
    PTValid(self, Jxta_transport);
    return VTBL->allow_routing_p(self);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_connection_oriented_p(Jxta_transport * self)
{
    PTValid(self, Jxta_transport);
    return VTBL->connection_oriented_p(self);
}


/*
 * NB the functions above constitute the interface. There are no default implementations
 * of the interface methods, therefore there is no methods table for the class, and there
 * is no new_instance function either.
 * No one is supposed to create a object of this exact class.
 */
