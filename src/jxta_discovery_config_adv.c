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
 * $Id: jxta_discovery_config_adv.c,v 1.2 2006/02/18 00:32:51 slowhog Exp $
 */

static const char *const __log_cat = "DiscCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_discovery_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

#define DELTA_UPDATE_CYCLE         (30 * 1000 ) /* 30 sec */
#define EXPIRED_ADV_CYCLE          (5 * 60 * 1000)  /* 5 minutes */

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_DiscoveryConfigAdvertisement_,
    addr_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_DiscoveryConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    int delta_update_cycle;
    int expired_adv_cycle;
};

    /* Forward decl. of un-exported function */
static void jxta_DiscoveryConfigAdvertisement_delete(Jxta_object *);



/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_DiscoveryConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryConfigAdvertisement *ad = (Jxta_DiscoveryConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:DiscoveryConfig\n");

    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "deltaCycle")) {
            ad->delta_update_cycle = (atoi(atts[1]) * 1000);
        } else if (0 == strcmp(*atts, "expiredAdvCycle")) {
            ad->expired_adv_cycle = (atoi(atts[1]) * 1000);
        }
        atts += 2;
    }
}

JXTA_DECLARE(void) jxta_discovery_config_set_delta_update_cycle(Jxta_DiscoveryConfigAdvertisement * adv, int time)
{
    if (time > 0) {
        adv->delta_update_cycle = time;
    }

}

JXTA_DECLARE(Jxta_time_diff) jxta_discovery_config_get_delta_update_cycle(Jxta_DiscoveryConfigAdvertisement * adv)
{
    return adv->delta_update_cycle;

}
JXTA_DECLARE(void) jxta_discovery_config_set_expired_adv_cycle(Jxta_DiscoveryConfigAdvertisement * adv, int time)
{
    if (time > 0) {
        adv->expired_adv_cycle = time;
    }

}

JXTA_DECLARE(Jxta_time_diff) jxta_discovery_config_get_expired_adv_cycle(Jxta_DiscoveryConfigAdvertisement * adv)
{
    return adv->expired_adv_cycle;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_DiscoveryConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:DiscoveryConfig", Jxta_DiscoveryConfigAdvertisement_, *handleJxta_DiscoveryConfigAdvertisement, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_DiscoveryConfigAdvertisement_get_xml(Jxta_DiscoveryConfigAdvertisement * ad, JString ** result)
{
    char tmpbuf[256];
    JString *string = jstring_new_0();
    jstring_append_2(string, "<!-- JXTA Discovery Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:DiscoveryConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:DiscoveryConfig\"\n");
    jstring_append_2(string, " deltaCycle=\"");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%ld", (long) ad->delta_update_cycle / 1000);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "\"\n");
    jstring_append_2(string, " expiredAdvCycle=\"");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%ld", (long) ad->expired_adv_cycle / 1000);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");
    jstring_append_2(string, "</jxta:DiscoveryConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_DiscoveryConfigAdvertisement *jxta_DiscoveryConfigAdvertisement_construct(Jxta_DiscoveryConfigAdvertisement * self)
{
    self = (Jxta_DiscoveryConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:DiscoveryConfig",
                                     Jxta_DiscoveryConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_DiscoveryConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->delta_update_cycle = DELTA_UPDATE_CYCLE;
        self->expired_adv_cycle = EXPIRED_ADV_CYCLE;
    }

    return self;
}

void jxta_DiscoveryConfigAdvertisement_destruct(Jxta_DiscoveryConfigAdvertisement * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_DiscoveryConfigAdvertisement *) jxta_DiscoveryConfigAdvertisement_new(void)
{
    Jxta_DiscoveryConfigAdvertisement *ad =
        (Jxta_DiscoveryConfigAdvertisement *) calloc(1, sizeof(Jxta_DiscoveryConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_DiscoveryConfigAdvertisement_delete, 0);

    return jxta_DiscoveryConfigAdvertisement_construct(ad);
}

static void jxta_DiscoveryConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_DiscoveryConfigAdvertisement_destruct((Jxta_DiscoveryConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_DiscoveryConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_DiscoveryConfigAdvertisement_parse_charbuffer(Jxta_DiscoveryConfigAdvertisement * ad, const char *buf,
                                                                      size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_DiscoveryConfigAdvertisement_parse_file(Jxta_DiscoveryConfigAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vim: set ts=4 sw=4 et tw=130: */
