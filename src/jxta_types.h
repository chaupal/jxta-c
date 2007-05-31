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
 * $Id: jxta_types.h,v 1.4 2005/03/23 19:31:38 bondolo Exp $
 */

#ifndef JXTA_TYPES_H
#define JXTA_TYPES_H

#include <stddef.h>
#include "jpr/jpr_types.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

/**
 * JXTA public types from the Portable Runtime (JPR)
 *
 * They are renamed here do reflect that they are part of the JXTA API, 
 * although their true definition belongs in the jpr layer. Maybe we'll
 * eventually get rid of one or the other of the names.
 **/
 
typedef Jpr_boolean Jxta_boolean;
typedef Jpr_status Jxta_status;

/**
    @deprecated Unused
**/
typedef Jpr_ssize Jxta_ssize;

typedef Jpr_pool Jxta_pool;
typedef Jpr_hash Jxta_hash;

/**
    @deprecated Unused
**/
typedef Jpr_uint Jxta_uint;

typedef Jpr_interval_time Jxta_time_diff;
typedef Jpr_absolute_time Jxta_time;

/**
    XXX should be redefined as an interval time.
**/
typedef Jpr_expiration_time Jxta_expiration_time;

typedef Jpr_port Jxta_port;
typedef Jpr_in_addr Jxta_in_addr;

 /**
 * A definition of boolean that looks like a fundamental type.
 * Hopefully all boolean definitions are the same.
 *
 * XXX bondolo Defining "boolean" this way is an incredibly bad idea.
 * not only will it cause any OS typedef of boolean to be replaced by
 * Jxta_boolean. There's of course no garauntee that they are AT ALL
 * compatible. I'm only guessing, but I assume it's a #define because 
 * if boolean is typedefed then typedef boolean Jxta_boolean would produce
 * an error. This should have been a clue....
 *
 * @deprecated Will be removed. Soon. DO NOT USE!
 */
#ifndef boolean
#define boolean Jxta_boolean
#endif

/**
  JXTA Standard callbacks
 **/

typedef Jxta_status(*ReadFunc) (void *stream, char *buf, size_t len);

typedef int (*Jxta_read_func) (void *stream, const char *buf, size_t len, Jxta_status * res);

typedef Jxta_status(*PrintFunc) (void *stream, const char *format, ...);

typedef Jxta_status(*WriteFunc) (void *stream, const char *buf, size_t len);

typedef int (*Jxta_write_func) (void *stream, const char *buf, size_t len, Jxta_status * res);

/* Can't "write" to a SOCKET in Win32. */
typedef Jxta_status(*SendFunc) (void *stream, const char *buf, size_t len, unsigned int flag);


#ifdef __cplusplus
}
#endif

#endif /* JXTATYPES_H */
