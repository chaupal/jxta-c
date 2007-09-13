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
 * $Id$
 */

static const char *const __log_cat = "RdvCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */ 
enum tokentype {
    Null_,
    Jxta_RdvConfigAdvertisement_,
    Seeds_,
    addr_
};

struct _jxta_RdvConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;

    RdvConfig_configuration config;

    int max_ttl;
    Jxta_time_diff auto_rdv_interval;
    Jxta_boolean probe_relays;
    int max_clients;
    int max_probed;
    Jxta_time_diff lease_duration;
    Jxta_time_diff lease_margin;
    Jxta_time_diff min_retry_delay;
    Jxta_time_diff max_retry_delay;
    Jxta_time_diff connect_cycle_normal;
    Jxta_time_diff connect_cycle_fast;
    Jxta_time_diff lease_renewal_delay;
    Jxta_time_diff interval_peerview;
    Jxta_time_diff pve_expires_peerview;
    Jxta_time_diff rdva_refresh;
    Jxta_time_diff connect_delay;
    int min_connected_rendezvous;
    int min_happy_peerview;
    Jxta_boolean use_only_seeds;
    Jxta_vector *seeds;
    Jxta_vector *seeding;
};

    /* Forward decl. of un-exported function */
static void jxta_RdvConfigAdvertisement_delete(Jxta_object *);


JXTA_DECLARE(RdvConfig_configuration) jxta_RdvConfig_get_config(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->config;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_config(Jxta_RdvConfigAdvertisement * ad, RdvConfig_configuration config)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->config = config;
}

JXTA_DECLARE(int) jxta_RdvConfig_get_max_clients(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->max_clients;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_max_clients(Jxta_RdvConfigAdvertisement * ad, int max_clients)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->max_clients = max_clients;
}

JXTA_DECLARE(int) jxta_RdvConfig_get_max_probed(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->max_probed;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_max_probed(Jxta_RdvConfigAdvertisement * ad, int max_probed)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->max_probed = max_probed;
}

JXTA_DECLARE(int) jxta_RdvConfig_get_max_ttl(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->max_ttl;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_max_ttl(Jxta_RdvConfigAdvertisement * ad, int max_ttl)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->max_ttl = max_ttl;
}

JXTA_DECLARE(int) jxta_RdvConfig_get_min_happy_peerview(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->min_happy_peerview;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_min_happy_peerview(Jxta_RdvConfigAdvertisement * ad, int happy_peerview)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->min_happy_peerview = happy_peerview;
}

JXTA_DECLARE(int) jxta_RdvConfig_get_min_connected_rendezvous(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->min_connected_rendezvous;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_min_connected_rendezvous(Jxta_RdvConfigAdvertisement * ad, int min_rdv)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->min_connected_rendezvous = min_rdv;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_interval_peerview(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return (Jxta_time) ad->interval_peerview ;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_interval_peerview(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff interval)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->interval_peerview = interval;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_pve_expires_peerview(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->pve_expires_peerview  ;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_pve_expires_peerview(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff interval)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->pve_expires_peerview = interval;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_rdva_refresh(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->rdva_refresh;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_rdva_refresh(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff refresh)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->rdva_refresh = refresh;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_cycle_normal(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->connect_cycle_normal;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_connect_cycle_normal(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff time)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->connect_cycle_normal = time;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_cycle_fast(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->connect_cycle_fast;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_connect_cycle_fast(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff time)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->connect_cycle_fast = time;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_min_retry_delay(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->min_retry_delay;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_min_retry_delay(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff time)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->min_retry_delay = time;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_max_retry_delay(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->max_retry_delay;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_max_retry_delay(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff time)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->max_retry_delay = time;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_duration(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->lease_duration;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_lease_duration(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff lease_duration)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->lease_duration = lease_duration;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_renewal_delay(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->lease_renewal_delay;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_lease_renewal_delay(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff time)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->lease_renewal_delay = time;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_lease_margin(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->lease_margin;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_lease_margin(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff lease_margin)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->lease_margin = lease_margin;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_auto_rdv_interval(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    return ad->auto_rdv_interval;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_auto_rdv_interval(Jxta_RdvConfigAdvertisement * ad, Jxta_time_diff auto_rdv_interval)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    ad->auto_rdv_interval = auto_rdv_interval;
}

JXTA_DECLARE(Jxta_time_diff) jxta_RdvConfig_get_connect_delay(Jxta_RdvConfigAdvertisement *ad)
{
    return ad->connect_delay;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_connect_delay(Jxta_RdvConfigAdvertisement *ad, Jxta_time_diff delay)
{
    ad->connect_delay = delay;
}

JXTA_DECLARE(Jxta_vector *) jxta_RdvConfig_get_seeds(Jxta_RdvConfigAdvertisement * ad)
{
    Jxta_vector *result = NULL;
    JXTA_OBJECT_CHECK_VALID(ad);

    jxta_vector_clone(ad->seeds, &result, 0, INT_MAX);

    return result;
}

JXTA_DECLARE(Jxta_boolean) jxta_RdvConfig_use_only_seeds(Jxta_RdvConfigAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);
    return ad->use_only_seeds;
}

JXTA_DECLARE(void) jxta_RdvConfig_set_use_only_seeds(Jxta_RdvConfigAdvertisement * ad, Jxta_boolean tf)
{
    JXTA_OBJECT_CHECK_VALID(ad);
    ad->use_only_seeds = tf;
}

JXTA_DECLARE(void) jxta_RdvConfig_add_seed(Jxta_RdvConfigAdvertisement * ad, Jxta_endpoint_address * seed)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    jxta_vector_add_object_last(ad->seeds, (Jxta_object *) seed);
}

JXTA_DECLARE(Jxta_vector *) jxta_RdvConfig_get_seeding(Jxta_RdvConfigAdvertisement * ad)
{
    Jxta_vector *result = NULL;
    JXTA_OBJECT_CHECK_VALID(ad);

    jxta_vector_clone(ad->seeding, &result, 0, INT_MAX);

    return result;
}

JXTA_DECLARE(void) jxta_RdvConfig_add_seeding(Jxta_RdvConfigAdvertisement * ad, Jxta_endpoint_address * seed)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    jxta_vector_add_object_last(ad->seeding, (Jxta_object *) seed);
}


/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_RdvConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvConfigAdvertisement *ad = (Jxta_RdvConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:RdvConfig\n");

    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "xmlns:jxta")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "config")) {
            if (0 == strcmp(atts[1], "adhoc")) {
                ad->config = config_adhoc;
            } else if (0 == strcmp(atts[1], "client")) {
                ad->config = config_edge;
            } else if (0 == strcmp(atts[1], "rendezvous")) {
                ad->config = config_rendezvous;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized configuration : %s \n", atts[1]);
            }
        } else if (0 == strcmp(*atts, "maxTTL")) {
            ad->max_ttl = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "autoRendezvousInterval")) {
            ad->auto_rdv_interval = atol(atts[1]);
        } else if (0 == strcmp(*atts, "probeRelays")) {
            ad->probe_relays = 0 == strcmp(atts[1], "true");
        } else if (0 == strcmp(*atts, "connectCycleNormal")) {
            ad->connect_cycle_normal = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "connectCycleFast")) {
            ad->connect_cycle_fast = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "minRetryDelay")) {
            ad->min_retry_delay = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "maxRetryDelay")) {
            ad->max_retry_delay = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "maxClients")) {
            ad->max_clients = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "leaseDuration")) {
            ad->lease_duration = atol(atts[1]);
        } else if (0 == strcmp(*atts, "leaseRenewalDelay")) {
            ad->lease_renewal_delay = atol(atts[1]);
        } else if (0 == strcmp(*atts, "leaseMargin")) {
            ad->lease_margin = atol(atts[1]);
        }else if (0 == strcmp(*atts, "minConnectedRendezvous")) {
            ad->min_connected_rendezvous = atoi(atts[1]);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
        }

        atts += 2;
    }
}

static void handleSeeds(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvConfigAdvertisement *ad = (Jxta_RdvConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "useOnlySeeds")) {
            ad->use_only_seeds = 0 == strcmp(atts[1], "true");
        } else if (0 == strcmp(*atts, "connectDelay")) {
            ad->connect_delay = atol(atts[1]);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized seeds attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
        }

        atts += 2;
    }
}

static void handlePeerView(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvConfigAdvertisement *ad = (Jxta_RdvConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "pIntervalPeerView")) {
            ad->interval_peerview = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "pveExpiresPeerView")) {
            ad->pve_expires_peerview = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "rdvaRefreshPeerView")) {
            ad->rdva_refresh = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "minHappyPeerView")) {
            ad->min_happy_peerview = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "maxProbed")) {
            ad->max_probed = atoi(atts[1]);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized PeerView attribute : \"%s\" = \"%s\"\n", *atts,
                            atts[1]);
        }

        atts += 2;
    }
}

static void handleAddr(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvConfigAdvertisement *ad = (Jxta_RdvConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
    Jxta_boolean seeding = FALSE;
    JString *addrStr;
    Jxta_endpoint_address *addr;

    if (len == 0)
        return;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "seeding")) {
            seeding = 0 == strcmp(atts[1], "true");
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized addr attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
        }

        atts += 2;
    }

    addrStr = jstring_new_1(len);
    jstring_append_0(addrStr, cd, len);
    jstring_trim(addrStr);
    addr = jxta_endpoint_address_new_1(addrStr);

    if (seeding) {
        jxta_vector_add_object_last(ad->seeding, (Jxta_object *) addr);
    } else {
        jxta_vector_add_object_last(ad->seeds, (Jxta_object *) addr);
    }

    JXTA_OBJECT_RELEASE(addrStr);
    JXTA_OBJECT_RELEASE(addr);
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_RdvConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RdvConfig", Jxta_RdvConfigAdvertisement_, *handleJxta_RdvConfigAdvertisement, NULL, NULL},
    {"seeds", Seeds_, *handleSeeds, NULL, NULL},
    {"peerview", Null_, *handlePeerView, NULL, NULL},
    {"addr", addr_, *handleAddr, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_RdvConfigAdvertisement_get_xml(Jxta_RdvConfigAdvertisement * ad, JString ** result)
{
    char tmpbuf[256];
    JString *string = jstring_new_0();
    unsigned int eachSeed;
    jstring_append_2(string, "<!-- JXTA Rendezvous Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:RdvConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:RdvConfig\"");
    jstring_append_2(string, " config=\"");
    switch (ad->config) {
    case config_adhoc:
        jstring_append_2(string, "adhoc\"");
        break;
    case config_edge:
        jstring_append_2(string, "client\"");
        break;
    case config_rendezvous:
        jstring_append_2(string, "rendezvous\"");
        break;
    default:
        jstring_append_2(string, "client\"");
        break;
    }

    if (-1 != ad->max_ttl) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " maxTTL=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->max_ttl);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->auto_rdv_interval) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " autoRendezvousInterval=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->auto_rdv_interval);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (!ad->probe_relays) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " probeRelays=\"false\"");
    }

    if (-1 != ad->max_clients) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " maxClients=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->max_clients);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->connect_cycle_normal) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "connectCycleNormal=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->connect_cycle_normal);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->connect_cycle_fast) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "connectCycleFast=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->connect_cycle_fast);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    if (-1 != ad->min_retry_delay) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "minRetryDelay=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->min_retry_delay);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->max_retry_delay) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "maxRetryDelay=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->max_retry_delay);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
        
    if (-1 != ad->lease_duration) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " leaseDuration=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->lease_duration);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
        
    if (-1 != ad->lease_renewal_delay) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "leaseRenewalDelay=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->lease_renewal_delay);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->lease_margin) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " leaseMargin=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->lease_margin);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    
    if (-1 != ad->min_connected_rendezvous) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "minConnectedRendezvous=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->min_connected_rendezvous);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
   
    jstring_append_2(string, ">\n");

    jstring_append_2(string, "<peerview ");
    if (-1 != ad->interval_peerview) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "pIntervalPeerView=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->interval_peerview);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    
    if (-1 != ad->pve_expires_peerview) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "pveExpiresPeerView=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->pve_expires_peerview);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->min_happy_peerview) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, " minHappyPeerView=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->min_happy_peerview);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->max_probed) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "maxProbed=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->max_probed);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (-1 != ad->rdva_refresh) {
        jstring_append_2(string, "\n    ");
        jstring_append_2(string, "rdvaRefreshPeerView=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%"APR_INT64_T_FMT, ad->rdva_refresh);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    
    jstring_append_2(string, "/>\n");
    if ((jxta_vector_size(ad->seeds)) > 0 || (jxta_vector_size(ad->seeding) > 0)) {
        jstring_append_2(string, "<seeds");
        /* FIXME: We want to support useOnlySeeds */
        /*
        if (ad->use_only_seeds) {
            jstring_append_2(string, " useOnlySeeds=\"true\"");
        }
        */
        if (-1 != ad->connect_delay) {
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " connectDelay=\"%d\"", ad->connect_delay);
            jstring_append_2(string, tmpbuf);
        }
        jstring_append_2(string, ">\n");
        for (eachSeed = 0; eachSeed < jxta_vector_size(ad->seeds); eachSeed++) {
            Jxta_endpoint_address *addr;
            char *addrStr;

            jxta_vector_get_object_at(ad->seeds, JXTA_OBJECT_PPTR(&addr), eachSeed);

            addrStr = jxta_endpoint_address_to_string(addr);

            jstring_append_2(string, "<addr>");
            jstring_append_2(string, addrStr);
            jstring_append_2(string, "</addr>\n");

            JXTA_OBJECT_RELEASE(addr);
            free(addrStr);
        }

        for (eachSeed = 0; eachSeed < jxta_vector_size(ad->seeding); eachSeed++) {
            Jxta_endpoint_address *addr;
            char *addrStr;

            jxta_vector_get_object_at(ad->seeding, JXTA_OBJECT_PPTR(&addr), eachSeed);

            addrStr = jxta_endpoint_address_to_string(addr);

            jstring_append_2(string, "<addr seeding=\"true\">");
            jstring_append_2(string, addrStr);
            jstring_append_2(string, "</addr>\n");

            JXTA_OBJECT_RELEASE(addr);
            free(addrStr);
        }

        jstring_append_2(string, "</seeds>\n");
    }

    jstring_append_2(string, "</jxta:RdvConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_RdvConfigAdvertisement *jxta_RdvConfigAdvertisement_construct(Jxta_RdvConfigAdvertisement * self)
{
    self = (Jxta_RdvConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:RdvConfig",
                                     Jxta_RdvConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_RdvConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL,
                                     (JxtaAdvertisementGetIndexFunc) jxta_RdvConfigAdvertisement_get_indexes);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->config = config_edge;
        self->max_ttl = -1;
        self->auto_rdv_interval = -1;
        self->probe_relays = TRUE;
        self->max_clients = -1;
        self->max_probed = -1;
        self->lease_duration = -1;
        self->lease_renewal_delay = -1;
        self->min_retry_delay = -1;
        self->max_retry_delay = -1;
        self->lease_margin = -1;
        self->min_connected_rendezvous = -1;
        self->min_happy_peerview = -1;
        self->connect_cycle_normal = -1;
        self->connect_cycle_fast = -1;
        self->interval_peerview = -1;
        self->pve_expires_peerview = -1;
        self->rdva_refresh = -1;
        self->use_only_seeds = FALSE;
        self->connect_delay = -1;
        self->seeds = jxta_vector_new(4);
        self->seeding = jxta_vector_new(4);
    }

    return self;
}

void jxta_RdvConfigAdvertisement_destruct(Jxta_RdvConfigAdvertisement * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

    if (self->seeds) {
        JXTA_OBJECT_RELEASE(self->seeds);
    }

    if (self->seeding) {
        JXTA_OBJECT_RELEASE(self->seeding);
    }
}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_RdvConfigAdvertisement *) jxta_RdvConfigAdvertisement_new(void)
{
    Jxta_RdvConfigAdvertisement *ad = (Jxta_RdvConfigAdvertisement *) calloc(1, sizeof(Jxta_RdvConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_RdvConfigAdvertisement_delete, 0);

    return jxta_RdvConfigAdvertisement_construct(ad);
}

static void jxta_RdvConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_RdvConfigAdvertisement_destruct((Jxta_RdvConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_RdvConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_RdvConfigAdvertisement_parse_charbuffer(Jxta_RdvConfigAdvertisement * ad, const char *buf, size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_RdvConfigAdvertisement_parse_file(Jxta_RdvConfigAdvertisement * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *) jxta_RdvConfigAdvertisement_get_indexes(void)
{
    const char *idx[][2] = { {NULL, NULL} };
    return jxta_advertisement_return_indexes(idx[0]);
}

/* vim: set ts=4 sw=4 et tw=130: */
