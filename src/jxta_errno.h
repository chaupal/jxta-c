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

#ifndef JXTA_ERRNO_H
#define JXTA_ERRNO_H

/* WE DO NOT EXPOSE APR TO JXTA APPS AND WE DO NOT WANT TO GO THROUGH A
 * CONVERSION FUNCTION FOR THAT. AS A RESULT, THIS MUST BE KEPT IN SYNC
 * WITH APR BY HAND
 * We compile src/jpr/jpr_ckcompat.c for the sole purpose of detecting
 * out-of-sync-ness.
 */

#include "jpr/jpr_errno.h"
#include "jxta_types.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif /* __cplusplus */

#define JXTA_SUCCESS ((Jxta_status) JPR_SUCCESS)
#define JXTA_START_ERROR  ((Jxta_status) JPR_START_ERROR)
#define JXTA_START_USEERR ((Jxta_status) JPR_START_USEERR)
#define JXTA_END_USEERR ((Jxta_status) JPR_END_USEERR)
/*
 * The very first error number we need, of course :-)
 */
#define JXTA_NOTIMP ((Jxta_status) (JXTA_START_ERROR + 0))
/*
 * And then litany of usual suspects.
 */
#define JXTA_INVALID_ARGUMENT ((Jxta_status) (JXTA_START_ERROR + 1))
#define JXTA_ITEM_NOTFOUND ((Jxta_status) (JXTA_START_ERROR + 2))
#define JXTA_NOMEM ((Jxta_status) (JXTA_START_ERROR + 3))
#define JXTA_TIMEOUT ((Jxta_status) (JXTA_START_ERROR + 4))
#define JXTA_BUSY ((Jxta_status) (JXTA_START_ERROR + 5))
#define JXTA_VIOLATION ((Jxta_status) (JXTA_START_ERROR + 6))
#define JXTA_FAILED ((Jxta_status) (JXTA_START_ERROR + 7))
#define JXTA_CONFIG_NOTFOUND ((Jxta_status) (JXTA_START_ERROR + 8))
#define JXTA_IOERR ((Jxta_status) (JXTA_START_ERROR + 9))
#define JXTA_ITEM_EXISTS ((Jxta_status) (JXTA_START_ERROR + 10))
#define JXTA_NOT_CONFIGURED ((Jxta_status) (JXTA_START_ERROR + 11))
#define JXTA_UNREACHABLE_DEST ((Jxta_status) (JXTA_START_ERROR + 12))
#define JXTA_TTL_EXPIRED ((Jxta_status) (JXTA_START_ERROR + 13))

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* ! JXTA_ERRNO_H */

/* vi: set ts=4 sw=4 tw=130 et: */
