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
 * $Id: jpr_thread.c,v 1.8 2005/03/24 19:35:31 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <apr.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_time.h>

#include "jpr_types.h"
#include "jpr_thread.h"
#include "jpr_threadonce.h"
#include "jpr_priv.h"

static apr_thread_cond_t *_jpr_thread_delay_cond = NULL;
static apr_thread_mutex_t *_jpr_thread_delay_mutex = NULL;

#ifndef JPR_LOG
#define JPR_LOG printf
#endif

/**
 ** Initialize jpr_thread_delay.
 **
 ** this function is called through thread_once; which guarantees in a thread
 ** safe manner that it is called only once.
 **/

apr_status_t jpr_thread_delay_initialize(void)
{
    apr_status_t res;

    res = apr_thread_mutex_create(&_jpr_thread_delay_mutex, APR_THREAD_MUTEX_DEFAULT, _jpr_global_pool);
    if (res != APR_SUCCESS) {
        JPR_LOG("FAILURE: %s:%d failed to create a mutex\n", __FILE__, __LINE__);
        return res;
    }

    res = apr_thread_cond_create(&_jpr_thread_delay_cond, _jpr_global_pool);
    if (res != APR_SUCCESS) {
        JPR_LOG("FAILURE: %s:%d failed to create a cond\n", __FILE__, __LINE__);
        apr_thread_mutex_destroy(_jpr_thread_delay_mutex);
        _jpr_thread_delay_mutex = NULL;
        return res;
    }

    return res;
}

void jpr_thread_delay_terminate(void)
{
    apr_thread_mutex_lock(_jpr_thread_delay_mutex);

    apr_thread_cond_destroy(_jpr_thread_delay_cond);
    _jpr_thread_delay_cond = NULL;

    apr_thread_mutex_destroy(_jpr_thread_delay_mutex);
    _jpr_thread_delay_mutex = NULL;
}

/**************************************************************************
 ** jpr_thread_delay:
 **
 ** Puts the calling thread to sleep for the specified time.
 **
 ** @param timeout time in micro-seconds for which the calling thread must
 **                sleep.
 ** @returns JPR_SUCCESS.
 **************************************************************************/

Jpr_status jpr_thread_delay(Jpr_interval_time timeout)
{
    /* Get the mutex */
    apr_thread_mutex_lock(_jpr_thread_delay_mutex);
    /* Wait for the specified time */
    apr_thread_cond_timedwait(_jpr_thread_delay_cond, _jpr_thread_delay_mutex, (apr_interval_time_t) timeout);
    /* Release the mutex */
    apr_thread_mutex_unlock(_jpr_thread_delay_mutex);
    return JPR_SUCCESS;
}

/* vim: set ts=4 sw=4 tw=130 et: */
