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

static const char *const __log_cat = "SrdiCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */ 
enum tokentype {
    Null_,
    Jxta_SrdiConfigAdvertisement_,
    NoRange_,
    RepThreshold_,
    addr_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_SrdiConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    int replicationThreshold;
    Jxta_boolean noRangeWithValue;
    Jxta_boolean SRDIDeltaSupport;
    int SRDIDeltaWindow;
};

#define DEFAULT_REPLICATION_THRESHOLD 4
#define DEFAULT_SRDI_DELTA_WINDOW 80

    /* Forward decl. of un-exported function */
static void jxta_SrdiConfigAdvertisement_delete(Jxta_object *);



/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_SrdiConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SrdiConfigAdvertisement *ad = (Jxta_SrdiConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:SrdiConfig\n");

    while (atts && *atts) {
        if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        } else if (0 == strcmp(*atts, "SRDIDeltaSupport")) {
            if (0 == strcmp(atts[1], "no")) {
                ad->SRDIDeltaSupport = FALSE;
            }
        } else if (0 == strcmp(*atts, "delta_window")) {
                ad->SRDIDeltaWindow = atoi(atts[1]);
        }
        atts += 2;
    }
}

static void handleNoRange(void *userdata, const XML_Char *cd, int len)
{
    Jxta_SrdiConfigAdvertisement *ad = (Jxta_SrdiConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "withValue")) {
            ad->noRangeWithValue = 0 == strcmp(atts[1], "true");
        } else {
            ad->noRangeWithValue = 0;
        }
        atts += 2;
    }
}
static void handleReplicationThreshold(void *userdata, const XML_Char *cd, int len)
{
    Jxta_SrdiConfigAdvertisement *ad = (Jxta_SrdiConfigAdvertisement *) userdata;
    if (0 == len) {
        ad->replicationThreshold = 0;
        return;
    }
    ad->replicationThreshold = atoi(cd);
}

JXTA_DECLARE(void) jxta_srdi_config_set_no_range(Jxta_SrdiConfigAdvertisement * adv, Jxta_boolean tf)
{
    adv->noRangeWithValue = tf;
}

JXTA_DECLARE(Jxta_boolean) jxta_srdi_cfg_get_no_range(Jxta_SrdiConfigAdvertisement * adv)
{
    return adv->noRangeWithValue;
}

JXTA_DECLARE(void) jxta_srdi_config_set_replication_threshold(Jxta_SrdiConfigAdvertisement * adv, int threshold)
{
    adv->replicationThreshold = threshold;
}

JXTA_DECLARE(int) jxta_srdi_cfg_get_replication_threshold(Jxta_SrdiConfigAdvertisement * adv)
{
    return adv->replicationThreshold;
}

JXTA_DECLARE(Jxta_boolean) jxta_srdi_cfg_is_delta_cache_supported(Jxta_SrdiConfigAdvertisement * adv)
{
    return adv->SRDIDeltaSupport;
}

JXTA_DECLARE(int) jxta_srdi_cfg_get_delta_window(Jxta_SrdiConfigAdvertisement * adv)
{
    return adv->SRDIDeltaWindow;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_SrdiConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:SrdiConfig", Jxta_SrdiConfigAdvertisement_, *handleJxta_SrdiConfigAdvertisement, NULL, NULL},
    {"NoRangeReplication", NoRange_, *handleNoRange, NULL, NULL},
    {"ReplicationThreshold", RepThreshold_, *handleReplicationThreshold, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_SrdiConfigAdvertisement_get_xml(Jxta_SrdiConfigAdvertisement * ad, JString ** result)
{
    char tmpbuf[64];
    JString *string = jstring_new_0();
    jstring_append_2(string, "<!-- JXTA SRDI Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:SrdiConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:SrdiConfig\"");
    jstring_append_2(string, " SRDIDeltaSupport = \"");
    jstring_append_2(string, ad->SRDIDeltaSupport == TRUE ? "yes":"no");
    jstring_append_2(string, "\"");
    if (ad->SRDIDeltaSupport) {
        jstring_append_2(string, " delta_window = \"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", ad->SRDIDeltaWindow);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    jstring_append_2(string, ">\n");
    jstring_append_2(string, "<NoRangeReplication withValue=\"");
    jstring_append_2(string, jxta_srdi_cfg_get_no_range(ad) ? "true":"false");
    jstring_append_2(string, "\"/>\n");
    jstring_append_2(string, "<ReplicationThreshold>");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", jxta_srdi_cfg_get_replication_threshold(ad));
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "</ReplicationThreshold>\n");
    jstring_append_2(string, "</jxta:SrdiConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_SrdiConfigAdvertisement *jxta_SrdiConfigAdvertisement_construct(Jxta_SrdiConfigAdvertisement * self)
{
    self = (Jxta_SrdiConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:SrdiConfig",
                                     Jxta_SrdiConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_SrdiConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL,
                                     (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->noRangeWithValue=FALSE;
        self->replicationThreshold = DEFAULT_REPLICATION_THRESHOLD;
        self->SRDIDeltaSupport = TRUE;
        self->SRDIDeltaWindow = DEFAULT_SRDI_DELTA_WINDOW;
    }
    return self;
}

void jxta_SrdiConfigAdvertisement_destruct(Jxta_SrdiConfigAdvertisement * self)
{
    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_SrdiConfigAdvertisement *) jxta_SrdiConfigAdvertisement_new(void)
{
    Jxta_SrdiConfigAdvertisement *ad = (Jxta_SrdiConfigAdvertisement *) calloc(1, sizeof(Jxta_SrdiConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_SrdiConfigAdvertisement_delete, 0);

    return jxta_SrdiConfigAdvertisement_construct(ad);
}

static void jxta_SrdiConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_SrdiConfigAdvertisement_destruct((Jxta_SrdiConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_SrdiConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_SrdiConfigAdvertisement_parse_charbuffer(Jxta_SrdiConfigAdvertisement * ad, const char *buf, size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_SrdiConfigAdvertisement_parse_file(Jxta_SrdiConfigAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vim: set ts=4 sw=4 et tw=130: */
