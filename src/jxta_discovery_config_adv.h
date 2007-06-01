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
 * $Id: jxta_discovery_config_adv.h,v 1.1 2006/02/02 21:45:01 slowhog Exp $
 */


#ifndef JXTA_DISCOVERYCONFIGADVERTISEMENT_H__
#define JXTA_DISCOVERYCONFIGADVERTISEMENT_H__

#include "jxta_types.h"
#include "jxta_advertisement.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
typedef struct _jxta_DiscoveryConfigAdvertisement Jxta_DiscoveryConfigAdvertisement;

JXTA_DECLARE(Jxta_DiscoveryConfigAdvertisement *) jxta_DiscoveryConfigAdvertisement_new(void);
JXTA_DECLARE(Jxta_status) jxta_DiscoveryConfigAdvertisement_get_xml(Jxta_DiscoveryConfigAdvertisement *, JString **);
JXTA_DECLARE(void) jxta_DiscoveryConfigAdvertisement_parse_charbuffer(Jxta_DiscoveryConfigAdvertisement *, const char *,
                                                                      size_t len);
JXTA_DECLARE(void) jxta_DiscoveryConfigAdvertisement_parse_file(Jxta_DiscoveryConfigAdvertisement *, FILE * stream);
JXTA_DECLARE(Jxta_vector *) jxta_DiscoveryConfigAdvertisement_get_indexes(void);

JXTA_DECLARE(void) jxta_discovery_config_set_delta_update_cycle(Jxta_DiscoveryConfigAdvertisement * adv, int time);
JXTA_DECLARE(Jxta_time_diff) jxta_discovery_config_get_delta_update_cycle(Jxta_DiscoveryConfigAdvertisement * adv);
JXTA_DECLARE(void) jxta_discovery_config_set_expired_adv_cycle(Jxta_DiscoveryConfigAdvertisement * adv, int time);
JXTA_DECLARE(Jxta_time_diff) jxta_discovery_config_get_expired_adv_cycle(Jxta_DiscoveryConfigAdvertisement * adv);

/**
*   For other advertisement types which want to parse DiscoveryConfig as a sub-section.    
**/
void handleJxta_DiscoveryConfigAdvertisement(void *userdata, const XML_Char * cd, int len);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_DISCOVERYCONFIGADVERTISEMENT_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
