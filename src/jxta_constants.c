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
 * $Id$
 */
#include "jxta_constants.h"
#include "jxta_advertisement.h"
#include "jxta_errno.h"
#include "jxta_discovery_service.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_listener.h"
#include "jstring.h"

/*
 * Export all constants, which are defined as a C macro. With these functions 
 * other programming languages like C# have access to the constants.
 */

JXTA_DECLARE(Jxta_status) jxta_get_JXTA_SUCCESS(void) { return JXTA_SUCCESS; }

JXTA_DECLARE(Jxta_status) jxta_get_JXTA_INVALID_ARGUMENT(void) { return JXTA_INVALID_ARGUMENT; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_ITEM_NOTFOUND(void) { return JXTA_ITEM_NOTFOUND; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_NOMEM(void) { return JXTA_NOMEM; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_TIMEOUT(void) { return JXTA_TIMEOUT; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_BUSY(void) { return JXTA_BUSY; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_VIOLATION(void) { return JXTA_VIOLATION; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_FAILED(void) { return JXTA_FAILED; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_CONFIG_NOTFOUND(void) { return JXTA_CONFIG_NOTFOUND; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_IOERR(void) { return JXTA_IOERR; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_ITEM_EXISTS(void) { return JXTA_ITEM_EXISTS; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_NOT_CONFIGURED(void) { return JXTA_NOT_CONFIGURED; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_UNREACHABLE_DEST(void) { return JXTA_UNREACHABLE_DEST; }
JXTA_DECLARE(Jxta_status) jxta_get_JXTA_TTL_EXPIRED(void) {return JXTA_TTL_EXPIRED;}

JXTA_DECLARE(int) jxta_get_DISC_PEER(void) { return DISC_PEER; }
JXTA_DECLARE(int) jxta_get_DISC_GROUP(void) { return DISC_GROUP; }
JXTA_DECLARE(int) jxta_get_DISC_ADV(void) { return DISC_ADV; }
JXTA_DECLARE(Jxta_time_diff) jxta_get_DEFAULT_LIFETIME(void) { return DEFAULT_LIFETIME; }
JXTA_DECLARE(Jxta_time_diff) jxta_get_DEFAULT_EXPIRATION(void) { return DEFAULT_EXPIRATION; }

JXTA_DECLARE(unsigned int) jxta_get_config_adhoc(void) { return config_adhoc; }
JXTA_DECLARE(unsigned int) jxta_get_config_edge(void) { return config_edge; }
JXTA_DECLARE(unsigned int) jxta_get_config_rendezvous(void) { return config_rendezvous; }

JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_INVALID(void) { return JXTA_LOG_LEVEL_INVALID;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_MIN(void) { return JXTA_LOG_LEVEL_MIN;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_FATAL(void) { return JXTA_LOG_LEVEL_FATAL;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_ERROR(void) { return JXTA_LOG_LEVEL_ERROR;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_WARNING(void) { return JXTA_LOG_LEVEL_WARNING;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_INFO(void) { return JXTA_LOG_LEVEL_INFO;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_DEBUG(void) { return JXTA_LOG_LEVEL_DEBUG;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_TRACE(void) { return JXTA_LOG_LEVEL_TRACE;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_PARANOID(void) { return JXTA_LOG_LEVEL_PARANOID;}
JXTA_DECLARE(Jxta_log_level) jxta_get_JXTA_LOG_LEVEL_MAX(void) { return JXTA_LOG_LEVEL_MAX;}

JXTA_DECLARE(Jxta_id *) jxta_get_id_worldNetPeerGroupID(void) { return jxta_id_worldNetPeerGroupID; } 
JXTA_DECLARE(Jxta_id *) jxta_get_id_defaultNetPeerGroupID(void) { return jxta_id_defaultNetPeerGroupID; }
JXTA_DECLARE(Jxta_id *) jxta_get_id_defaultMonPeerGroupID(void) { return jxta_id_defaultMonPeerGroupID; }

JXTA_DECLARE(Jxta_write_func) jxta_get_addr_writefunc_appender()
{
	return (Jxta_write_func)jstring_writefunc_appender;
}

