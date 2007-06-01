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
 * $Id: jxta.c,v 1.15 2006/03/23 02:26:04 slowhog Exp $
 */

#include "jxta_apr.h"
#include "jpr/jpr_core.h"
#include "jxta_log.h"
#include "jxta_private.h"
#include "jxta_advertisement_priv.h"
#include "jxta_netpg_private.h"
#include "jxta_range.h"

/**
 * Briefly, touching jxta jxta touches apr, which requires a call
 * to apr_initialize() and apr_terminate().  
 */

static unsigned int _jxta_initialized = 0;
int _jxta_return = 1;
#ifdef WIN32
static int _targc = 0;
static char **_targv = NULL;
#endif

/**
 * @todo Add initialization code.
 */
JXTA_DECLARE(void) jxta_initialize(void)
{
    if (_jxta_initialized++) {
        return;
    }
#ifdef WIN32
    if (_targc != __argc)
        _targc = __argc;

    if (_targv != __argv)
        _targv = __argv;

    apr_app_initialize(&_targc, &_targv, NULL);
#else
    apr_initialize();
#endif

    jpr_initialize();
    jxta_object_initialize();
    jxta_log_initialize();
    jxta_advertisement_register_global_handlers();
    jxta_PG_module_initialize();
    netpg_init_methods();
    jxta_range_init();
}


/**
 * @todo Add termination code.
 */
JXTA_DECLARE(void) jxta_terminate(void)
{
    if (!_jxta_initialized) {
        return;
    }

    _jxta_initialized--;
    if (_jxta_initialized) {
        return;
    }

    jxta_range_destroy();
    jxta_PG_module_terminate();
    jxta_advertisement_cleanup();
    jxta_log_terminate();
    jxta_object_terminate();
    jpr_terminate();
    apr_terminate();
}

/* vim: set ts=4 sw=4 tw=130 et: */
