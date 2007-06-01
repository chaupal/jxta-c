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
 * $Id: jxta_module.c,v 1.30 2006/09/18 22:58:55 slowhog Exp $
 */

static const char *__log_cat = "MODULE";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jpr/jpr_excep.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_module.h"
#include "jxta_module_private.h"

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

    self->methods = PTValid(methods, Jxta_module_methods);
    
    self->state = JXTA_MODULE_CONSTRUCTED;
    
    self->thisType = "_jxta_module";

    return self;
}

/**
 * The base class dtor (Not public, not virtual. Only called by subclassers).
 */
void jxta_module_destruct(Jxta_module * module)
{
    _jxta_module *self = (_jxta_module *) module;

    /*FIXME: we really want to be more restrictive. don't destruct if not stopped. */
    if (JXTA_MODULE_STOPPED != self->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to destruct a module which is not STOPPED.\n");
    }

    self->thisType = NULL;
    self->state = JXTA_MODULE_UNLOADED;
}

JXTA_DECLARE(Jxta_status) jxta_module_init(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id,
                                           Jxta_advertisement * impl_adv)
{
    Jxta_status res;
    _jxta_module *self = PTValid(module, _jxta_module);

    /*FIXME: we really want to be more restrictive. don't init if not constructed. */
    if (JXTA_MODULE_CONSTRUCTED != self->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to init a module which is not in CONSTRUCTED state!\n");
    }

    self->state = JXTA_MODULE_INITING;
    res = JXTA_MODULE_VTBL(self)->init(self, group, assigned_id, impl_adv);
    self->state = (JXTA_SUCCESS == res) ? JXTA_MODULE_INITED : JXTA_MODULE_STOPPED;
    
    return res;
}

JXTA_DECLARE(void) jxta_module_init_e(Jxta_module * module, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv,
                                      Throws)
{
    Jxta_status res;

    JXTA_DEPRECATED_API();

    res = jxta_module_init(module, group, assigned_id, impl_adv);

    if( JXTA_SUCCESS != res ) {
        Throw(res);
    }
}

JXTA_DECLARE(Jxta_status) jxta_module_start(Jxta_module * module, const char *args[])
{
    Jxta_status res;
    _jxta_module *self = PTValid(module, _jxta_module);
    
    /*FIXME: we really want to be more restrictive. don't start if not inited. */
    if (JXTA_MODULE_INITED != self->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to start a module which is not in INITED state!\n");
    }

    if (! JXTA_MODULE_VTBL(self)->start) {
        self->state = JXTA_MODULE_STARTED;
        return JXTA_SUCCESS;
    }

    self->state = JXTA_MODULE_STARTING;
    res = JXTA_MODULE_VTBL(self)->start(self, args);
    self->state = (JXTA_SUCCESS == res) ? JXTA_MODULE_STARTED : JXTA_MODULE_STOPPED;
    return res;
}

JXTA_DECLARE(void) jxta_module_stop(Jxta_module * module)
{
    _jxta_module *self = PTValid(module, _jxta_module);

    if (JXTA_MODULE_STARTED != self->state) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Trying to stop a module which has not started successfully.\n");
        return;
    }

    if (! JXTA_MODULE_VTBL(self)->stop) {
        self->state = JXTA_MODULE_STOPPED;
        return;
    }

    self->state = JXTA_MODULE_STOPPING;
    JXTA_MODULE_VTBL(self)->stop(self);
    self->state = JXTA_MODULE_STOPPED;
    return;
}

JXTA_DECLARE(Jxta_module_state) jxta_module_state(Jxta_module * module)
{
    _jxta_module *self = PTValid(module, _jxta_module);

    return self->state;
}

/* vim: set ts=4 sw=4 et tw=130: */
