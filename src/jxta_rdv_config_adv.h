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
 * $Id: jxta_rdv_config_adv.h,v 1.13 2006/02/01 23:34:43 slowhog Exp $
 */


#ifndef JXTA_RDVCONFIGADVERTISEMENT_H__
#define JXTA_RDVCONFIGADVERTISEMENT_H__

#include "jxta_apr.h"
#include "jxta_types.h"
#include "jxta_advertisement.h"
#include "jxta_endpoint_address.h"
#include "jxta_vector.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
typedef struct _jxta_RdvConfigAdvertisement Jxta_RdvConfigAdvertisement;

enum RdvConfig_configurations {
    config_adhoc,
    config_edge,
    config_rendezvous
};

typedef enum RdvConfig_configurations RdvConfig_configuration;

JXTA_DECLARE(Jxta_RdvConfigAdvertisement *) jxta_RdvConfigAdvertisement_new(void);
JXTA_DECLARE(Jxta_status) jxta_RdvConfigAdvertisement_get_xml(Jxta_RdvConfigAdvertisement *, JString **);
JXTA_DECLARE(void) jxta_RdvConfigAdvertisement_parse_charbuffer(Jxta_RdvConfigAdvertisement *, const char *, size_t len);
JXTA_DECLARE(void) jxta_RdvConfigAdvertisement_parse_file(Jxta_RdvConfigAdvertisement *, FILE * stream);
JXTA_DECLARE(Jxta_vector *) jxta_RdvConfigAdvertisement_get_indexes(void);

JXTA_DECLARE(RdvConfig_configuration) jxta_RdvConfig_get_config(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_config(Jxta_RdvConfigAdvertisement *, RdvConfig_configuration);

JXTA_DECLARE(int) jxta_RdvConfig_get_max_clients(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_max_clients(Jxta_RdvConfigAdvertisement *, int);

JXTA_DECLARE(int) jxta_RdvConfig_get_max_ttl(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_max_ttl(Jxta_RdvConfigAdvertisement *, int);

JXTA_DECLARE(int) jxta_RdvConfig_get_min_happy_peerview(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_min_happy_peerview(Jxta_RdvConfigAdvertisement *, int);

JXTA_DECLARE(int) jxta_RdvConfig_get_max_probed(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_max_probed(Jxta_RdvConfigAdvertisement *, int);

JXTA_DECLARE(int) jxta_RdvConfig_get_min_connected_rendezvous(Jxta_RdvConfigAdvertisement * ad);
JXTA_DECLARE(void) jxta_RdvConfig_set_min_connected_rendezvous(Jxta_RdvConfigAdvertisement *, int);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_interval_peerview(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_interval_peerview(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_pve_expires_peerview(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_pve_expires_peerview(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_rdva_refresh(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_rdva_refresh(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_cycle_normal(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_connect_cycle_normal(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_cycle_fast(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_connect_cycle_fast(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_min_retry_delay(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_min_retry_delay(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_max_retry_delay(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_max_retry_delay(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_duration(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_lease_duration(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_renewal_delay(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_lease_renewal_delay(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_margin(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_lease_margin(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_auto_rdv_interval(Jxta_RdvConfigAdvertisement * ad);
JXTA_DECLARE(void) jxta_RdvConfig_set_auto_rdv_interval(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff auto_rdv_interval);

JXTA_DECLARE(Jxta_boolean) jxta_RdvConfig_use_only_seeds(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_use_only_sees(Jxta_RdvConfigAdvertisement *, Jxta_boolean);

JXTA_DECLARE(Jxta_vector *) jxta_RdvConfig_get_seeds(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_add_seed(Jxta_RdvConfigAdvertisement *, Jxta_endpoint_address *);

JXTA_DECLARE(Jxta_vector *) jxta_RdvConfig_get_seeding(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_add_seeding(Jxta_RdvConfigAdvertisement *, Jxta_endpoint_address *);

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_delay(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfig_set_connect_delay(Jxta_RdvConfigAdvertisement *, Jxta_time_diff);

JXTA_DECLARE(char *) jxta_RdvConfigAdvertisement_get_Jxta_RdvConfigAdvertisement(Jxta_RdvConfigAdvertisement *);
JXTA_DECLARE(void) jxta_RdvConfigAdvertisement_set_Jxta_RdvConfigAdvertisement(Jxta_RdvConfigAdvertisement *, char *);

/**
*   For other advertisement types which want to parse RdvConfig as a sub-section.    
**/
void handleJxta_RdvConfigAdvertisement(void *userdata, const XML_Char * cd, int len);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_RDVCONFIGADVERTISEMENT_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
