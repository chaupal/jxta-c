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

#include <apr_general.h>
#include <apr_pools.h>

#include "jpr_core.h"
#include "jpr_priv.h"

apr_pool_t *_jpr_global_pool = NULL;
static unsigned int _jpr_initialized = 0;
#ifdef WIN32
static int _targc = 0;
static char **_targv = NULL;
#endif

JPR_DECLARE(Jpr_status) jpr_initialize(void)
{
    apr_status_t rv;

    if (_jpr_initialized++) {
        return APR_SUCCESS;
    }

#ifdef WIN32
    if (_targc != __argc)
        _targc = __argc;

    if (_targv != __argv)
        _targv = __argv;

    rv = apr_app_initialize(&_targc, &_targv, NULL);
#else
    rv = apr_initialize();
#endif
    if (APR_SUCCESS == rv) {
        rv = apr_pool_create(&_jpr_global_pool, NULL);
    }

    if (APR_SUCCESS == rv) {
        rv = jpr_excep_initialize();
    }

    if (APR_SUCCESS != rv) {
        _jpr_initialized = 0;
    }

    return rv;
}

JPR_DECLARE(void) jpr_terminate(void)
{
    if (!_jpr_initialized) {
        return;
    }

    _jpr_initialized--;
    if (_jpr_initialized) {
        return;
    }

    jpr_excep_terminate();

    apr_pool_destroy(_jpr_global_pool);
    _jpr_global_pool = NULL;
    apr_terminate();
}

/* vim: set ts=4 sw=4 tw=130 et: */
