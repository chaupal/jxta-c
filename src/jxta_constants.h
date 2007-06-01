/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_constants.h,v 1.3 2006/06/01 02:41:32 mmx2005 Exp $
 */
#ifndef __JXTA_CONSTANTS_H__
#define __JXTA_CONSTANTS_H__

#include "jxta_types.h"
#include "jxta_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Export all constants, which are defined as a C macro. With these functions 
 * other programming languages like C# have access to the constants.
 */

JXTA_DECLARE(Jxta_status) jxta_get_JXTA_SUCCESS(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_INVALID_ARGUMENT(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_ITEM_NOTFOUND(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_NOMEM(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_TIMEOUT(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_BUSY(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_VIOLATION(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_FAILED(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_CONFIG_NOTFOUND(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_IOERR(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_ITEM_EXISTS(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_NOT_CONFIGURED(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_UNREACHABLE_DEST(void);
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_TTL_EXPIRED(void);

JXTA_DECLARE(int) jxta_get_DISC_PEER(void);
JXTA_DECLARE(int) jxta_get_DISC_GROUP(void);
JXTA_DECLARE(int) jxta_get_DISC_ADV(void);
JXTA_DECLARE(Jxta_time_diff) jxta_get_DEFAULT_LIFETIME(void);
JXTA_DECLARE(Jxta_time_diff) jxta_get_DEFAULT_EXPIRATION(void);

JXTA_DECLARE(unsigned int) jxta_get_config_adhoc(void);
JXTA_DECLARE(unsigned int) jxta_get_config_edge(void);
JXTA_DECLARE(unsigned int) jxta_get_config_rendezvous(void);

JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_INVALID(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_MIN(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_FATAL(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_ERROR(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_WARNING(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_INFO(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_DEBUG(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_TRACE(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_PARANOID(void);
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_MAX(void);

#ifdef __cplusplus
}
#endif

#endif /* __JXTA_CONSTANTS_H__ */

