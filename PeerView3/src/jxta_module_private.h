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

#ifndef JXTA_MODULE_PRIVATE_H
#define JXTA_MODULE_PRIVATE_H

#include "jpr/jpr_excep_proto.h"

#include "jxta_module.h"
#include "jxta_object_type.h"

/*
 * This is an internal header file that defines the interface between
 * the module API and the various implementations.
 * An implementation of module must define a static Table as below
 * or a derivate of it.
 */

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * The set of methods that a Jxta_module must implement.
 * All Jxta_module and derivates have a pointer to such an
 * object or a derivate.
 *
 * Objects of this type are meant to be defined statically, no
 * initializer function is provided.
 *
 * Do not forget to begin the initializer block with the literal
 * string "Jxta_module_methods".
 */
struct _jxta_module_methods {
    Extends_nothing;            /* could extend Jxta_object but that'd be overkill */

    /* An implementation of Jxta_module_init */
    Jxta_status(*init) (Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv);

    /* An implementation of Jxta_module_start */
     Jxta_status(*start) (Jxta_module * self, const char *args[]);

    /* An implementation of Jxta_module_stop */
    void (*stop) (Jxta_module * self);
};

typedef struct _jxta_module_methods Jxta_module_methods;

/**
 * A Jxta_module is a Jxta_object with a thisType field of "Jxta_module" and
 * pointer to a Jxta_module_methods structure.
 * After allocating such an object, one must apply JXTA_OBJECT_INIT() and
 * jxta_module_construct to it.
 */
struct _jxta_module {
    Extends(Jxta_object);       /* equivalent to JXTA_OBJECT_HANDLE; char* thisType. */
    Jxta_module_methods const *methods; /* Pointer to the module implementation set of methods. */
    Jxta_module_state state;

    /* Implementors put their stuff after that */
};

typedef struct _jxta_module _jxta_module;

/**
 * The base class ctor (not public: the only public way to make a new module
 * is to instantiate one of the derived types). Derivates call this
 * to initialize the base part. It will initialize the thisType field and the
 * object's method set.
 * @param self The module object to construct.
 * @param methods The module's set of methods.
 * @return the constructed object or NULL if the construction failed.
 */
extern _jxta_module *jxta_module_construct(_jxta_module * self, Jxta_module_methods const *methods);

/**
 * The base class dtor (Not public, not virtual. Only called by subclassers).
 */
extern void jxta_module_destruct(_jxta_module * self);

/**
 * Access the vtbl for the module, nomatter how deeply derived,
 * there is a single vtbl ptr and it is declared here in Jxta_module.
 * May be used to initialize the vtbl ptr by the objects creator.
 * Expands as a pointer to the Jxta_module_methods (or derivate thereof)
 * object of the given Jxta_module.
 * @param self pointer to a Jxta_module object.
 * @return pointer to this object's set of methods.
 */
#define JXTA_MODULE_VTBL(self) (((_jxta_module*) (self))->methods)

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_SERVICE_PRIVATE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
