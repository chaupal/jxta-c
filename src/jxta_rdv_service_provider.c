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
 * $Id: jxta_rdv_service_provider.c,v 1.3 2005/02/02 01:55:05 bondolo Exp $
 */

#include "jxta_rdv_service_provider.h"
#include "jxta_rdv_service_provider_private.h"

#include <stddef.h>

_jxta_rdv_service_provider *jxta_rdv_service_provider_construct(_jxta_rdv_service_provider * self,
                                                                const _jxta_rdv_service_provider_methods * methods)
{
    apr_status_t res = APR_SUCCESS;

    self->methods = PTValid(methods, _jxta_rdv_service_provider_methods);
    self->thisType = "_jxta_rdv_service_provider";

    apr_pool_create(&self->pool, NULL);
    /* Create the mutex */
    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED,        /* nested */
                                  self->pool);

    self->service = NULL;

    return self;
}

void jxta_rdv_service_provider_destruct(_jxta_rdv_service_provider * self)
{
    self->service = NULL;

    /* Free the pool used to allocate the thread and mutex */
    apr_pool_destroy(self->pool);

    self->thisType = NULL;
}

Jxta_status jxta_rdv_service_provider_init(Jxta_rdv_service_provider * provider, _jxta_rdv_service * service)
{
    _jxta_rdv_service_provider *self = PTValid(provider, _jxta_rdv_service_provider);

    self->service = service;

    return JXTA_SUCCESS;
}

apr_pool_t *jxta_rdv_service_provider_get_pool_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    return self->pool;
}

void jxta_rdv_service_provider_lock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_lock(self->mutex);
}

void jxta_rdv_service_provider_unlock_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    apr_thread_mutex_unlock(self->mutex);
}

_jxta_rdv_service *jxta_rdv_service_provider_get_service_priv(Jxta_rdv_service_provider * provider)
{
    _jxta_rdv_service_provider *self = (_jxta_rdv_service_provider *) provider;

    return self->service;
}
