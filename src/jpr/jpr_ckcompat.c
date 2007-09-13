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

/*
 * The sole purpose of compiling this file is to generate an error if some
 * of the jxta definitions that must map apr_ ones get out-of-sync.
 * This is needed because jxta apps must not include apr_ header files,
 * even indirectly.
 */


#include <apr_errno.h>
#include <apr.h>
#include <apr_network_io.h>

#include <string.h>

#include "jpr_errno.h"
#include "jpr_types.h"

#if (JPR_SUCCESS != APR_SUCCESS)
#error "JPR_SUCCESS != APR_SUCCESS check that jpr_errno.h is compatible with APR"
#endif

#if (JPR_START_ERROR != APR_OS_START_USEERR)
#error "JPR_START_ERROR != APR_OS_START_USEERR check that jpr_errno.h is compatible with APR"
#endif

#if (JPR_START_USEERR > APR_OS_START_CANONERR)
#error "JPR_START_USEERR > APR_OS_START_CANONERR check that jpr_errno.h is compatible with APR"
#endif

void mustCompile(void)
{
    apr_status_t s1 = JPR_SUCCESS;
    Jpr_status s2 = APR_SUCCESS;
    apr_in_addr_t a1;
    Jpr_in_addr a2 = 0;
    apr_port_t p1 = 0;
    Jpr_port p2 = 0;

    apr_size_t t1 = 1;
    size_t t2 = 1;

    s2 = s1;
    s1 = s2;

    t1 = t2;
    t2 = t1;

    memset(&a1, 0, sizeof(apr_in_addr_t));
    a1.s_addr = a2;
    a2 = a1.s_addr;

    p1 = p2;
    p2 = p1;
}
