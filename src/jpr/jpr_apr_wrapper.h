/*
 * Copyright (c) 2004 Sun Microsystems, Inc.  All rights
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
 * $Id: jpr_apr_wrapper.h,v 1.6 2005/07/21 23:02:22 slowhog Exp $
 */

#ifndef __JPR_APR_WRAPPER_H__
#define __JPR_APR_WRAPPER_H__

#include "apr_version.h"
#include "apr_thread_mutex.h"
#include "apr_general.h"
#include "jpr_types.h"
#include "apr_time.h"





#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif



#define CHECK_APR_VERSION(major, minor, patch) \
	(APR_MAJOR_VERSION > (major) || \
	 (APR_MAJOR_VERSION == (major) && APR_MINOR_VERSION > (minor)) || \
	 (APR_MAJOR_VERSION == (major) && APR_MINOR_VERSION == (minor) && \
	  APR_PATCH_VERSION >= (patch)))

#define JPR_HAS_THREADS APR_HAS_THREADS

#if JPR_HAS_THREADS

typedef struct apr_thread_mutex_t jpr_thread_mutex_t;

#define JPR_THREAD_MUTEX_DEFAULT APR_THREAD_MUTEX_DEFAULT
#define JPR_THREAD_MUTEX_NESTED APR_THREAD_MUTEX_NESTED
#define JPR_THREAD_MUTEX_UNNESTED APR_THREAD_MUTEX_UNNESTED

#define jpr_thread_mutex_create apr_thread_mutex_create
#define jpr_thread_mutex_lock apr_thread_mutex_lock
#define jpr_thread_mutex_trylock apr_thread_mutex_trylock
#define jpr_thread_mutex_unlock apr_thread_mutex_unlock
#define jpr_thread_mutex_destroy apr_thread_mutex_destroy

#endif /* JPR_HAS_THREADS */

JPR_DECLARE(const char *) jpr_inet_ntop(int af, const void *src, char *dst, apr_size_t size);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif



#endif /* __JPR_APR_WRAPPER_H__ */

/* vim: set ts=4 sw=4 et: */
