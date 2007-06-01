/*
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_module.c,v 1.26 2006/05/26 02:13:54 bondolo Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_module.h"
#include "jxta_module_private.h"
#include "jxta_log.h"

static const char *__log_cat = "MODULE";
/**
 * The base class ctor (not public: the only public way to make a new module
 * is to instantiate one of the derived types).
 * We require the methods tbl to be passed through the ctor to make sure
 * it is not forgotten in the process.
 * We verify that the methods tbl has been properly set-up.
     *  
     * NB : jxta_object_init, not called from here; called directly
     * by the allocator, where the most derived type of the obj is known.
 */
_jxta_module *jxta_module_construct(_jxta_module * module, Jxta_module_methods const *methods)
{
    _jxta_module *self = (_jxta_module *) module;

    self->state = JXTA_MODULE_LOADING;
    self->methods = PTValid(methods, Jxta_module_methods);
    self->thisType = "_jxta_module";

    return self;
}

/**
 * The base class dtor (Not public, not virtual. Only called by subclassers).
 */
void jxta_module_destruct(Jxta_module * module)
{
    _jxta_module *self = (_jxta_module *) module;

    self->thisType = NULL;
    self->state = JXTA_MODULE_UNLOADED;
}

/**
 *  Initialize the module, passing it its peer group and advertisement.
 *
 *  @param group The PeerGroup from which this Module can obtain services.
 *  If this module is a service, this is also the PeerGroup of which this
 *  module is a service.
 *  @param assignedID Identity of Module within group.
 *  modules can use it as a the root of their namespace to create
 *  names that are unique within the group but predictible by the
 *  same module on another peer. This is normaly the ModuleClassID
 *  which is also the name under which the module is known by other
 *  modules. For a group it is the PeerGroupID itself.
 *  The parameters of a service, in the Peer configuration, are indexed
 *  by the assignedID of that service, and a Service must publish its
 *  run-time parameters in the Peer Advertisement under its assigned ID.
 *  @param implAdv The implementation advertisement for this Module.
 *
 *  @exception PeerGroupException This module failed to initialize.
 *
 * @since   JXTA 1.0
 */
JXTA_DECLARE(Jxta_status) jxta_module_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id,
                                           Jxta_advertisement * impl_adv)
{
    Jxta_status res;
    _jxta_module *self = PTValid(module, _jxta_module);

    res = JXTA_MODULE_VTBL(self)->init(self, group, assigned_id, impl_adv);
    self->state = JXTA_MODULE_LOADED;
    return res;
}

/**
 *  Initialize the module, passing it its peer group and advertisement.
 *
 *  @param group The PeerGroup from which this Module can obtain services.
 *  If this module is a service, this is also the PeerGroup of which this
 *  module is a service.
 *  @param assignedID Identity of Module within group.
 *  modules can use it as a the root of their namespace to create
 *  names that are unique within the group but predictible by the
 *  same module on another peer. This is normaly the ModuleClassID
 *  which is also the name under which the module is known by other
 *  modules. For a group it is the PeerGroupID itself.
 *  The parameters of a service, in the Peer configuration, are indexed
 *  by the assignedID of that service, and a Service must publish its
 *  run-time parameters in the Peer Advertisement under its assigned ID.
 *  @param implAdv The implementation advertisement for this Module.
 *
 *  @exception PeerGroupException This module failed to initialize.
 *
 * @since   JXTA 1.0
 */
JXTA_DECLARE(void) jxta_module_init_e(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv,
                                      Throws)
{
    _jxta_module *self = PTValid(module, _jxta_module);

    ThrowThrough();

    JXTA_MODULE_VTBL(self)->init_e(self, group, assigned_id, impl_adv, MayThrow);
    self->state = JXTA_MODULE_LOADED;
}


/**
 *  Some Modules will wait for start() being called, before proceeding
 *  beyond a certain point. That's also the opportunity to supply
 *  arbitrary arguments (mostly to applications).
 *
 *  @param args An array of Strings forming the parameters for this
 *  Module.
 *
 *  @return status indication. By convention 0 means that this Module
 *  started succesfully.
 */
JXTA_DECLARE(Jxta_status) jxta_module_start(Jxta_module * module, const char *args[])
{
    Jxta_status res;
    _jxta_module *self = PTValid(module, _jxta_module);

    self->state = JXTA_MODULE_STARTING;
    res = JXTA_MODULE_VTBL(self)->start(self, args);
    self->state = (JXTA_SUCCESS == res) ? JXTA_MODULE_STARTED : JXTA_MODULE_STOPPED;
    return res;
}


/**
 *  One can ask a Module to stop.
 *  The Module cannot be forced to comply, but in the future
 *  we might be able to deny it access to anything after some timeout.
 */
JXTA_DECLARE(void) jxta_module_stop(Jxta_module * module)
{
    _jxta_module *self = PTValid(module, _jxta_module);

    /*FIXME: we really want to be more restrictive. don't stop if not started. */
    if (JXTA_MODULE_STARTED != self->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to stop a module which has not started successfully.\n");
    }
    self->state = JXTA_MODULE_STOPPING;
    JXTA_MODULE_VTBL(self)->stop(self);
    self->state = JXTA_MODULE_STOPPED;
    return;
}

/*
 * NB the functions above constitute the interface. 
 * No one is supposed to create a object of this exact class so there is no method table and
 * not all implementations are present.
 */

/**
 * Initializes an instance of module. (a module method)
 * Exception throwing variant. This implementation simply calls through to the no-exception init via
 * the vtable.
 * 
 * @param module a pointer to the instance of the module
 * @param group a pointer to the PeerGroup the module is initialized for. IMPORTANT
 * note: this code assumes that the reference to the group that has been given has been
 * shared first.
 * Throws an exception if something fouls up.
 * 
 **/
void jxta_module_init_e_impl(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws)
{
    Jxta_status res = JXTA_MODULE_VTBL(module)->init(module, group, assigned_id, impl_adv);

    if (res != JXTA_SUCCESS)
        Throw(res);

    module->state = JXTA_MODULE_LOADED;
}

Jxta_module_state jxta_module_state(Jxta_module * self)
{
    return self->state;
}

/* vim: set ts=4 sw=4 et tw=130: */
