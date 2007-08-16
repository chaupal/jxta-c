/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights
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

#include <stdlib.h>
#include <assert.h>

#include "jxta_errno.h"
#include "jxta_object_type.h"
#include "jxta_callback.h"
#include "jxta_listener.h"
#include "jxta_log.h"

/**********************************************************************
 ** This file implements Jxta_callback. Please refer to jxta_callback.h
 ** for detail on the API.
 **********************************************************************/
struct _jxta_callback {
    Extends(Jxta_object);
    Jxta_callback_func func;
    void *arg;
};

static void jxta_callback_free(Jxta_object * me)
{
    assert(NULL != me);
    free(PTValid(me, Jxta_callback));
}

JXTA_DECLARE(Jxta_status) jxta_callback_new(Jxta_callback ** instance, Jxta_callback_func func, void *arg)
{
    Jxta_callback *me = (Jxta_callback *) calloc(1, sizeof(Jxta_callback));

    assert(NULL != instance);
    assert(NULL != func);

    *instance = NULL;
    if (NULL == me) {
        return JXTA_NOMEM;
    }

    JXTA_OBJECT_INIT(me, jxta_callback_free, NULL);
#ifndef NO_RUNTIME_TYPE_CHECK
    me->thisType = "Jxta_callback";
#endif
    me->func = func;
    me->arg = arg;

    *instance = me;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_callback_process_object(Jxta_callback * me, Jxta_object * object)
{
    JXTA_OBJECT_CHECK_VALID(me);
    JXTA_OBJECT_CHECK_VALID(object);

    return me->func(object, me->arg);
}

Jxta_status JXTA_STDCALL listener_callback(Jxta_object * obj, void *arg)
{
    Jxta_listener *l = arg;

    JXTA_DEPRECATED_API();
    jxta_listener_process_object(l, obj);
    return JXTA_SUCCESS;
}

/* vi: set ts=4 sw=4 tw=130 et: */
