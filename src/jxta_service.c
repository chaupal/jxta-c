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
 * $Id: jxta_service.c,v 1.14 2005/02/04 00:22:08 bondolo Exp $
 */

#include "jxta_service_private.h"

_jxta_service *jxta_service_construct(_jxta_service * svc, Jxta_service_methods * methods)
{
    _jxta_service *self = NULL;

    PTValid(methods, Jxta_service_methods);
    self = (_jxta_service *) jxta_module_construct((_jxta_module *) svc, (Jxta_module_methods *) methods);

    if (NULL != self) {
        self->thisType = "_jxta_service";
        self->group = NULL;
        self->assigned_id = NULL;
        self->impl_adv = NULL;
    }

    return self;
}

void jxta_service_destruct(_jxta_service * svc)
{
    _jxta_service *self = (_jxta_service *) svc;

    if (NULL != self->group) {
        JXTA_OBJECT_RELEASE(self->group);
    }

    if (NULL != self->assigned_id) {
        JXTA_OBJECT_RELEASE(self->assigned_id);
    }

    if (NULL != self->impl_adv) {
        JXTA_OBJECT_RELEASE(self->impl_adv);
    }

    self->thisType = NULL;

    jxta_module_destruct(svc);
}

/**
 * Easy access to the vtbl.
 */
#define VTBL ((Jxta_service_methods*) JXTA_MODULE_VTBL(self))

void jxta_service_get_interface(Jxta_service * svc, Jxta_service ** service)
{
    _jxta_service *self = PTValid(svc, _jxta_service);
    (VTBL->get_interface) (self, service);
}

void jxta_service_get_peergroup(Jxta_service * svc, Jxta_PG ** pg)
{
    _jxta_service *self = PTValid(svc, _jxta_service);
    jxta_service_get_peergroup_impl(self, pg);
}

void jxta_service_get_assigned_ID(Jxta_service * svc, Jxta_id ** assignedID)
{
    _jxta_service *self = PTValid(svc, _jxta_service);
    jxta_service_get_assigned_ID_impl(self, assignedID);
}

void jxta_service_get_MIA(Jxta_service * svc, Jxta_advertisement ** mia)
{
    _jxta_service *self = PTValid(svc, _jxta_service);
    (VTBL->get_MIA) (self, mia);
}

Jxta_status jxta_service_init(Jxta_service * svc, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    _jxta_service *self = PTValid(svc, _jxta_service);

    JXTA_OBJECT_SHARE(group);
    self->group = group;
    JXTA_OBJECT_SHARE(assigned_id);
    self->assigned_id = assigned_id;

    if (NULL != impl_adv) {
        JXTA_OBJECT_SHARE(impl_adv);
        self->impl_adv = impl_adv;
    }

    return JXTA_SUCCESS;
}

/*
 * NB the functions above constitute the interface. There are no default implementations
 * of the interface methods, therefore there is no methods table for the class, and there
 * is no new_instance function either.
 * No one is supposed to create a object of this exact class.
 */

/**
 * return this service's interface object
 *
 * @param service return this service's interface object
 * @return this service's interface object/
 **/
void jxta_service_get_interface_impl(Jxta_service * svc, Jxta_service ** intf)
{
    _jxta_service *self = PTValid(svc, _jxta_service);

    JXTA_OBJECT_SHARE(self);
    *intf = self;
}

/**
 * return the peer group object associated with this service.
 *
 * @return the peer group object associated with this service.
 **/
void jxta_service_get_peergroup_impl(Jxta_service * svc, Jxta_PG ** pg)
{
    _jxta_service *self = PTValid(svc, _jxta_service);

    JXTA_OBJECT_SHARE(self->group);
    *pg = self->group;
}

/**
 * Return the id associated with this service.
 *
 * @return the id associated with this service.
 **/
void jxta_service_get_assigned_ID_impl(Jxta_service * svc, Jxta_id ** assignedID)
{
    _jxta_service *self = PTValid(svc, _jxta_service);

    JXTA_OBJECT_SHARE(self->assigned_id);
    *assignedID = self->assigned_id;
}

/**
 * return the module implementation advertisement for this service
 *
 * @return the module implementation advertisement for this service
 **/
void jxta_service_get_MIA_impl(Jxta_service * svc, Jxta_advertisement ** mia)
{
    _jxta_service *self = PTValid(svc, _jxta_service);

    if (NULL != self->impl_adv) {
        JXTA_OBJECT_SHARE(self->impl_adv);
    }

    *mia = self->impl_adv;
}

Jxta_PG *jxta_service_get_peergroup_priv(_jxta_service * self)
{
    return self->group;
}

Jxta_id *jxta_service_get_assigned_ID_priv(_jxta_service * self)
{
    return self->assigned_id;
}

Jxta_advertisement *jxta_service_get_MIA_priv(_jxta_service * self)
{
    return self->impl_adv;
}
