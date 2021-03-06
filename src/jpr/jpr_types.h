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

#ifndef JPR_TYPES_H
#define JPR_TYPES_H

#ifndef JPR_DECLARE
#ifdef WIN32
#ifdef JPR_STATIC
#define JPR_DECLARE(type) extern type __stdcall
#define JPR_DECLARE_DATA  extern
#else /* JPR_STATIC */

#ifdef JPR_EXPORTS
#define JPR_DECLARE(type) __declspec(dllexport) type __stdcall
#define JPR_DECLARE_DATA  extern __declspec(dllexport)
#else
#define JPR_DECLARE(type) __declspec(dllimport) type __stdcall
#define JPR_DECLARE_DATA  extern __declspec(dllimport)
#endif

#endif /* JPR_STATIC */

#else /* WIN32 */
#define JPR_DECLARE(type)  extern type
#define JPR_DECLARE_DATA   extern
#endif /* WIN32 */

#endif /* JPR_DECLARE */

#include <apr.h>
#include <apr_errno.h>

#ifdef WIN32
#include <windows.h>
#endif


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


enum Jxta_booleans { JXTA_FALSE = 0, JXTA_TRUE = !JXTA_FALSE };

#if !defined(FALSE) && !defined(TRUE)
typedef enum Jxta_booleans Jpr_boolean;
#else
typedef unsigned int Jpr_boolean;
#endif

/* Remove define after clean up TRUE/FALSE from jxta-c codebase */
#ifndef TRUE
#define TRUE JXTA_TRUE
#endif

#ifndef FALSE
#define FALSE JXTA_FALSE
#endif

typedef unsigned long Jpr_status;

typedef unsigned short Jpr_port;
typedef unsigned long Jpr_in_addr;

/*
* NB : we cannot define these using apr_time_t since we use milliseconds and APR uses microseconds.
*/
typedef apr_int64_t Jpr_interval_time;
typedef apr_int64_t Jpr_absolute_time;

#define JPR_ABS_TIME_FMT  "%" APR_INT64_T_FMT
#define JPR_DIFF_TIME_FMT "%" APR_INT64_T_FMT

typedef Jpr_interval_time Jpr_expiration_time;  /* duration expressed in milliseconds */

JPR_DECLARE_DATA const Jpr_interval_time JPR_INTERVAL_ONE_SECOND;
JPR_DECLARE_DATA const Jpr_interval_time JPR_INTERVAL_TIME_MIN;
JPR_DECLARE_DATA const Jpr_interval_time JPR_INTERVAL_TIME_MAX;
JPR_DECLARE_DATA const Jpr_absolute_time JPR_ABSOLUTE_TIME_MAX;

/*
 * Some opaque types that stand for the corresponding apr
 * ones.
 */
typedef struct Jpr_pool Jpr_pool;
typedef struct Jpr_hash Jpr_hash;

/**
 * Return the absolute time since the epoch in milliseconds.
 * 
 * @return the absolute time in milliseconds since the epoch.
 */
JPR_DECLARE(Jpr_absolute_time) jpr_time_now(void);

/**
 * Define jpr_time_now in milliseconds to be consistent with 
 * Jpr_expiration_time specification and JXTA protocol specification.
 * The apr_time_now() is returning microseconds time. 
 */
#define jpr_time_now() ((Jpr_absolute_time) (apr_time_as_msec(apr_time_now())))

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JPR_TYPES_H */
