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

#ifndef JXTA_MODULE_H
#define JXTA_MODULE_H

#include "jxta_object.h"

#include "jpr/jpr_excep_proto.h"

#include "jxta_id.h"
#include "jxta_advertisement.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 * Do not include jxta_peergroup header: c/cpp induced circular dependency.
 */
#ifndef JXTA_PG_FORW
#define JXTA_PG_FORW
typedef struct jxta_PG Jxta_PG;
#endif

typedef struct _jxta_module Jxta_module;

enum _jxta_module_states {
    JXTA_MODULE_UNKNOWN = -1,
    JXTA_MODULE_CONSTRUCTED = 0,
    JXTA_MODULE_INITING,
    JXTA_MODULE_INITED,
    JXTA_MODULE_STARTING,
    JXTA_MODULE_STARTED,
    JXTA_MODULE_STOPPING,
    JXTA_MODULE_STOPPED,
    JXTA_MODULE_UNLOADING,
    JXTA_MODULE_UNLOADED
}; 

typedef enum _jxta_module_states Jxta_module_state;


/**
 * Initialize this module, supplying it its peer group assigned id and
 * impl advertisement.
 *
 * If this module is a service, it is registered in the group's list of
 * services and its API made available to other modules. Therefore a
 * service must support its interface being invoked before its start
 * routine has been invoked and executed to completion. However the routine
 * of a service are not required to carry out their task fully under these 
 * circumstances.
 *
 * @param self handle of the Jxta_module object to which the operation is
 * applied.
 *
 * @param group The peer group from which this Module can obtain services.
 * If this module is a service, this is also the peer group of which this
 * module is a service.
 *
 * @param assigned_id Identity of this Module within group. Modules can use it
 * as a the root of their namespace in order to create names that are unique
 * within the group but predictible by the same module on another peer. This
 * is normaly the module class ID which is also the name under which the
 * module is known by other modules. If this module is a group, then its
 * assigned id is its Peer Group ID.
 * The parameters of a service, in the Peer configuration, are indexed
 * by the assigned ID of that service, and a service must publish its
 * run-time parameters in the Peer Advertisement under its assigned ID.
 *
 * @param impl_adv The implementation advertisement for this Module.
 *
 * @return Jxta_status JXTA_SUCCESS if the operation was carried out, an
 * error otherwise.
 */
JXTA_DECLARE(Jxta_status) jxta_module_init(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id,
                                           Jxta_advertisement * impl_adv);


/**
 * Initialize this module, supplying it its peer group assigned id and
 * impl advertisement.
 *
 * @deprecated Please use the non-exception varient.
 *
 * If the operation cannot be carried out, an exception is thrown.
 *
 * @param self handle of the Jxta_module object to which the operation is
 * applied.
 *
 * @param group The peer group from which this Module can obtain services.
 * If this module is a service, this is also the peer group of which this
 * module is a service.
 *
 * If this module is a service, it is registered in the group's list of
 * services and its API made available to other modules. Therefore a
 * service must support its interface being invoked before its start()
 * function has been invoked and executed to completion. However the functions
 * of a service are not required to carry out their task fully under these 
 * circumstances.
 *
 * @param assigned_id Identity of this Module within group. Modules can use it
 * as a the root of their namespace in order to create names that are unique
 * within the group but predictible by the same module on another peer. This
 * is normaly the module class ID which is also the name under which the
 * module is known by other modules. If this module is a group, then its
 * assigned id is its Peer Group ID.
 * The parameters of a service, in the Peer configuration, are indexed
 * by the assigned ID of that service, and a service must publish its
 * run-time parameters in the Peer Advertisement under its assigned ID.
 *
 * @param impl_adv The implementation advertisement for this Module.
 */
JXTA_DECLARE(void) jxta_module_init_e(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv,
                                      Throws);


/**
 * Some Modules will wait for start() to be called, before begining full
 * opperation. This is also an opportunity to supply arbitrary arguments 
 * (mostly for applications).
 *
 * When jxta_module_start is invoked on a module, all services of the
 * module's home group are available and can safely be used. However
 * some of the other services may not yet be fully functional and
 * therefore some operations that would otherwise succed may fail
 * during the group's bootstrap. This specialy true if this module
 * is a service of the group. If this module is an application, it may 
 * assume that all services have completed their initialization.
 *
 * @param self handle of the Jxta_module object to which the operation is
 * applied.
 * @param args An array of Strings forming the parameters for this
 * Module.
 *
 * @return status indication. By convention 0 means that this Module
 * started succesfully.
 */
JXTA_DECLARE(Jxta_status) jxta_module_start(Jxta_module * self, const char *args[]);


/**
 * One can ask a Module to stop.
 * The Module cannot be forced to comply, but in the future
 * we might be able to deny it access to anything after some timeout.
 *
 * By convention after a module has been stopped, it is never re-started;
 * it is released and eventually deallocated. This is guaranteed if the
 * module is a group service and should be kept true by any module that
 * manipulates another module's life cycle.
 *
 * @param self handle of the Jxta_module object to which the operation is
 * applied.
 */
JXTA_DECLARE(void) jxta_module_stop(Jxta_module * self);

/**
*   Returns the current execution state of the module.
*
*   @param module The module who's state is desired.
*   @return The current state of the module.
*/
JXTA_DECLARE(Jxta_module_state) jxta_module_state(Jxta_module * module);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_MODULE_H */

/* vi: set ts=4 sw=4 tw=130 et: */
