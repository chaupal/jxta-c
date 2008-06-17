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
 * $Id$
 */

static const char *__log_cat = "TRANSPORT";

#include "jxta_transport_private.h"
#include "jxta_log.h"

static void jxta_transport_event_delete(Jxta_object * me)
{
    Jxta_transport_event *me2 = (Jxta_transport_event *) me;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "EVENT: Transport event [%pp] delete \n", me);

    if (NULL != me2->peer_id) {
        JXTA_OBJECT_RELEASE(me2->peer_id);
    }
    if (NULL != me2->dest_addr) {
        JXTA_OBJECT_RELEASE(me2->dest_addr);
    }
    if (NULL != me2->msgr) {
        JXTA_OBJECT_RELEASE(me2->msgr);
    }

    free(me2);
}

Jxta_transport_event *jxta_transport_event_new(Jxta_transport_event_type event_type)
{
    Jxta_transport_event *me = NULL;

    me = (Jxta_transport_event *) calloc(1, sizeof(Jxta_transport_event));
    if (NULL == me) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not alloc transport event");
        return NULL;
    }

    JXTA_OBJECT_INIT(me, jxta_transport_event_delete, NULL);
    me->type = event_type;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, FILEANDLINE "EVENT: Create a new transport event [%pp] with type %d\n",
                    me, event_type);
    return me;
}

/**
 * The base transport ctor (not public: the only public way to make a new transport
 * is to instantiate one of the derived types).
 */
Jxta_transport *jxta_transport_construct(Jxta_transport * self, Jxta_transport_methods const *methods)
{

    Jxta_transport_methods* transport_methods = PTValid(methods, Jxta_transport_methods);
    jxta_module_construct((Jxta_module *) self, (Jxta_module_methods const *) transport_methods);

    self->thisType = "Jxta_transport";
    self->metric = 0;
    self->direction = JXTA_OUTBOUND;

    return self;
}

Jxta_status jxta_transport_init(Jxta_transport * me, apr_pool_t * p)
{
    me->p = p;
    me->qos = apr_hash_make(me->p);
    if (!me->qos) {
        return JXTA_NOMEM;
    }
    return JXTA_SUCCESS;
}

/**
 * The base transport dtor (Not public, not virtual. Only called by subclassers).
 * We just pass it along.
 */
void jxta_transport_destruct(Jxta_transport * self)
{
    jxta_module_destruct((Jxta_module *) self);
}

int jxta_transport_metric_get(Jxta_transport * me)
{
    return me->metric;
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
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->name_get(myself);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_endpoint_address *) jxta_transport_publicaddr_get(Jxta_transport * self)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->publicaddr_get(myself);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(JxtaEndpointMessenger *) jxta_transport_messenger_get(Jxta_transport * self, Jxta_endpoint_address * there)
{
    char *ea_str = NULL;

    Jxta_transport* myself = PTValid(self, Jxta_transport);
    if (VTBL->messenger_get) {
        return VTBL->messenger_get(myself, there);
    } else {
        ea_str = jxta_endpoint_address_to_string(there);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE
                        ": Calling abstract method jxta_transport_messenger_get for EA: %s\n", ea_str ? ea_str : "NULL");
        if (ea_str)
            free(ea_str);

        return NULL;
    }
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_ping(Jxta_transport * self, Jxta_endpoint_address * there)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->ping(myself, there);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(void) jxta_transport_propagate(Jxta_transport * self, Jxta_message * msg, const char *service_name,
                                            const char *service_params)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    VTBL->propagate(myself, msg, service_name, service_params);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_overload_p(Jxta_transport * self)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->allow_overload_p(myself);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_routing_p(Jxta_transport * self)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->allow_routing_p(myself);
}

/******************************************************************************/
/*                                                                            */
/******************************************************************************/
JXTA_DECLARE(Jxta_boolean) jxta_transport_connection_oriented_p(Jxta_transport * self)
{
    Jxta_transport* myself = PTValid(self, Jxta_transport);
    return VTBL->connection_oriented_p(myself);
}

JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_inbound(Jxta_transport * me)
{
    return (me->direction & JXTA_INBOUND);
}

JXTA_DECLARE(Jxta_boolean) jxta_transport_allow_outbound(Jxta_transport * me)
{
    return (me->direction & JXTA_OUTBOUND);
}

/*
 * NB the functions above constitute the interface. There are no default implementations
 * of the interface methods, therefore there is no methods table for the class, and there
 * is no new_instance function either.
 * No one is supposed to create a object of this exact class.
 */

JXTA_DECLARE(Jxta_status) jxta_transport_set_default_qos(Jxta_transport * me, Jxta_endpoint_address * ea, const Jxta_qos * qos)
{
    char *ea_pstr;

    ea_pstr = jxta_endpoint_address_to_pstr(ea, me->p);
    if (!ea_pstr) {
        return JXTA_FAILED;
    }

    apr_hash_set(me->qos, ea_pstr, APR_HASH_KEY_STRING, qos);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_transport_default_qos(Jxta_transport * me, Jxta_endpoint_address * ea, const Jxta_qos ** qos)
{
    char *ea_cstr;

    ea_cstr = jxta_endpoint_address_to_string(ea);
    *qos = apr_hash_get(me->qos, ea_cstr, APR_HASH_KEY_STRING);
    free(ea_cstr);
    return (*qos) ? JXTA_SUCCESS : JXTA_ITEM_NOTFOUND;
}

/* vim: set ts=4 sw=4 et tw=130: */
