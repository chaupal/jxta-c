/* 
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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


#ifndef JXTA_CACHECONFIGADVERTISEMENT_H__
#define JXTA_CACHECONFIGADVERTISEMENT_H__

#include "jxta_types.h"
#include "jxta_advertisement.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif


typedef struct jxta_CacheConfigAdvertisement Jxta_CacheConfigAdvertisement;
typedef struct jxta_addressSpace Jxta_addressSpace;
typedef struct jxta_nameSpace Jxta_nameSpace;

JXTA_DECLARE(Jxta_CacheConfigAdvertisement *) jxta_CacheConfigAdvertisement_new(void);

JXTA_DECLARE(Jxta_status) jxta_CacheConfigAdvertisement_create_default(Jxta_CacheConfigAdvertisement * adv, Jxta_boolean legacy);

/*
JXTA_DECLARE(Jxta_status) jxta_CacheConfigAdvertisement_get_xml(Jxta_CacheConfigAdvertisement *, JString **);

JXTA_DECLARE(void) jxta_CacheConfigAdvertisement_parse_charbuffer(Jxta_CacheConfigAdvertisement *, const char *, size_t len);

JXTA_DECLARE(void) jxta_CacheConfigAdvertisement_parse_file(Jxta_CacheConfigAdvertisement *, FILE * stream);
*/
JXTA_DECLARE(Jxta_vector *) jxta_cache_config_get_addressSpaces(Jxta_CacheConfigAdvertisement * adv);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_single_db(Jxta_CacheConfigAdvertisement * adv);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_name(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_ephemeral(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_highPerformance(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_auto_vacuum(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_default(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_delta(Jxta_addressSpace * jads);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_DBName(Jxta_addressSpace * jads);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_DBEngine(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_time_diff) jxta_cache_config_get_persistInterval(Jxta_addressSpace * jads);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_persistName(Jxta_addressSpace * jads);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_QoS(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_addr_is_local(Jxta_addressSpace * jads);

JXTA_DECLARE(JString *) jxta_cache_config_addr_get_connectString(Jxta_addressSpace * jads);

JXTA_DECLARE(int) jxta_cache_config_addr_get_xaction_threshold(Jxta_addressSpace * jads);

JXTA_DECLARE(Jxta_vector *) jxta_cache_config_get_nameSpaces(Jxta_CacheConfigAdvertisement * adv);

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_ns(Jxta_nameSpace * jns);

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_adv_type(Jxta_nameSpace * jns);

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_prefix(Jxta_nameSpace * jns);

JXTA_DECLARE(Jxta_boolean) jxta_cache_config_ns_is_type_wildcard(Jxta_nameSpace * jns);

JXTA_DECLARE(JString *) jxta_cache_config_ns_get_addressSpace_string(Jxta_nameSpace * jns);

/**
*   For other advertisement types which want to parse CacheConfig as a sub-section.    
**/
void handleJxta_CacheConfigAdvertisement(void *userdata, const XML_Char * cd, int len);
int cache_config_threads_needed(Jxta_CacheConfigAdvertisement * me);
int cache_config_threads_maximum(Jxta_CacheConfigAdvertisement * me);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_CACHECONFIGADVERTISEMENT_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
