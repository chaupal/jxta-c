/* 
 * Copyright (c) 2005-2006 Sun Microsystems, Inc.  All rights reserved.
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

static const char *const __log_cat = "EPCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_endpoint_service.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"
#include "jxta_traffic_shaping_priv.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_EndPointConfigAdvertisement_
};

#define DIRECTION_BOTH "both"
#define DIRECTION_INBOUND "inbound"
#define DIRECTION_OUTBOUND "outbound"

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_EndPointConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    Jxta_time_diff nc_timeout_init;
    Jxta_time_diff nc_timeout_max;
    size_t ncrq_size;
    size_t ncrq_retry;
    int init_threads;
    int max_threads;

    /* traffic shaping */
    apr_int64_t ts_size;
    Jxta_time ts_time;
    int ts_interval;
    Jxta_time ts_frame;
    Jxta_time ts_look_ahead;
    int ts_reserve;
    Ts_max_option ts_max_option;
};

/* Forward decl. of un-exported function */
static void jxta_EndPointConfigAdvertisement_delete(Jxta_object *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_EndPointConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_EndPointConfigAdvertisement *ad = (Jxta_EndPointConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:EndPointConfig\n");
    if (0 != len) return;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "threads_init")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->init_threads = -1;
            } else {
                ad->init_threads = atoi(atts[1]);
            }
        } else if (0 == strcmp(*atts, "threads_max")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->max_threads = -1;
            } else {
                ad->max_threads = atoi(atts[1]);
            }
        } else if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        }

        atts+=2;
    }
}

static void handleNegativeCache(void *me, const XML_Char * cd, int len)
{
    Jxta_EndPointConfigAdvertisement *_self = (Jxta_EndPointConfigAdvertisement *) me;
    const char **atts = ((Jxta_advertisement *) me)->atts;

    while (atts && *atts) {
        if (0 == strcmp(*atts, "timeout")) {
            _self->nc_timeout_init = apr_atoi64(atts[1]) * 1000;
        } else if (0 == strcmp(*atts, "maxTimeOut")) {
            _self->nc_timeout_max = apr_atoi64(atts[1]) * 1000;
        } else if (0 == strcmp(*atts, "queueSize")) {
            _self->ncrq_size = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "msgToRetry")) {
            _self->ncrq_retry = atoi(atts[1]);
        }
        atts += 2;
    }
}

JXTA_DECLARE(void) jxta_epcfg_set_nc_timeout_init(Jxta_EndPointConfigAdvertisement * me, int timeout)
{
    me->nc_timeout_init = timeout;
}

JXTA_DECLARE(Jxta_time_diff) jxta_epcfg_get_nc_timeout_init(Jxta_EndPointConfigAdvertisement * me)
{
    return me->nc_timeout_init;
}

JXTA_DECLARE(void) jxta_epcfg_set_nc_timeout_max(Jxta_EndPointConfigAdvertisement * me, int timeout)
{
    me->nc_timeout_max = timeout;
}

JXTA_DECLARE(Jxta_time_diff) jxta_epcfg_get_nc_timeout_max(Jxta_EndPointConfigAdvertisement * me)
{
    return me->nc_timeout_max;
}

JXTA_DECLARE(void) jxta_epcfg_set_ncrq_size(Jxta_EndPointConfigAdvertisement * me, size_t sz)
{
    me->ncrq_size = sz;
}

JXTA_DECLARE(size_t) jxta_epcfg_get_ncrq_size(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ncrq_size;
}

JXTA_DECLARE(void) jxta_epcfg_set_ncrq_retry(Jxta_EndPointConfigAdvertisement * me, size_t cnt)
{
    me->ncrq_retry = cnt;
}

JXTA_DECLARE(size_t) jxta_epcfg_get_ncrq_retry(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ncrq_retry;
}

static void handleTrafficShaping(void *me, const XML_Char * cd, int len)
{
    Jxta_EndPointConfigAdvertisement *_self = (Jxta_EndPointConfigAdvertisement *) me;
    const char **atts = ((Jxta_advertisement *) me)->atts;

    if (0 != len) return;

    while (atts && *atts) {

        if (0 == strcmp(*atts, "size")) {
            _self->ts_size = apr_atoi64(atts[1]);
        } else if (0 == strcmp(*atts, "time")) {
            _self->ts_time = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "interval")) {
            _self->ts_interval = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "frame")) {
            _self->ts_frame = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "look_ahead")) {
            _self->ts_look_ahead = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "reserve")) {
            _self->ts_reserve = atoi(atts[1]);
        } else if (0 == strcmp(*atts, "max_option")) {
            _self->ts_max_option = strcmp(atts[1], "frame") == 0 ? TS_MAX_OPTION_FRAME:TS_MAX_OPTION_LOOK_AHEAD;
        }
        atts += 2;
    }
}

JXTA_DECLARE(apr_int64_t) jxta_ep_cfg_ts_size(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_size;
}

JXTA_DECLARE(void) jxta_ep_cfg_set_ts_size(Jxta_EndPointConfigAdvertisement * me,apr_int64_t parm )
{
    me->ts_size = parm;
}

JXTA_DECLARE(Jxta_time) jxta_ep_cfg_ts_time(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_time;
}

JXTA_DECLARE(void) jxta_ep_cfg_set_ts_time(Jxta_EndPointConfigAdvertisement * me, Jxta_time parm)
{
    me->ts_time = parm;
}

JXTA_DECLARE(int) jxta_ep_cfg_ts_interval(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_interval;
}

JXTA_DECLARE(void) jxta_ep_cfg_ts_set_interval(Jxta_EndPointConfigAdvertisement * me, int parm)
{
    me->ts_interval = parm;
}

JXTA_DECLARE(Jxta_time) jxta_ep_cfg_ts_frame(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_frame;
}

JXTA_DECLARE(void) jxta_ep_cfg_ts_set_frame(Jxta_EndPointConfigAdvertisement * me, Jxta_time parm)
{
    me->ts_frame = parm;
}

JXTA_DECLARE(Jxta_time) jxta_ep_cfg_ts_look_ahead(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_look_ahead;
}

JXTA_DECLARE(void) jxta_ep_cfg_ts_set_look_ahead(Jxta_EndPointConfigAdvertisement * me, Jxta_time parm)
{
    me->ts_look_ahead = parm;
}

JXTA_DECLARE(int) jxta_ep_cfg_ts_reserve(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_reserve;
}

JXTA_DECLARE(void) jxta_ep_cfg_ts_set_reserve(Jxta_EndPointConfigAdvertisement * me, int parm)
{
    me->ts_reserve = parm;
}

JXTA_DECLARE(Ts_max_option) jxta_ep_cfg_ts_max_option(Jxta_EndPointConfigAdvertisement * me)
{
    return me->ts_max_option;
}

JXTA_DECLARE(void) jxta_ep_cfg_ts_set_max_option(Jxta_EndPointConfigAdvertisement * me, Ts_max_option parm)
{
    me->ts_max_option = parm;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_EndPointConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:EndPointConfig", Null_, *handleJxta_EndPointConfigAdvertisement, NULL, NULL},
    {"TrafficShaping", Null_, *handleTrafficShaping, NULL, NULL},
    {"NegativeCache", Null_, *handleNegativeCache, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_EndPointConfigAdvertisement_get_xml(Jxta_EndPointConfigAdvertisement * me, JString ** result)
{
    char tmpbuf[256];
    JString *string = jstring_new_0();
    int timeout;

    jstring_append_2(string, "<!-- JXTA EndPoint Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:EndPointConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:EndPointConfig\" ");

    if(me->init_threads == -1) {
        jstring_append_2(string, "threads_init=\"default\" ");
    }
    else {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "threads_init=\"%d\" ", me->init_threads);
        jstring_append_2(string, tmpbuf);
    }

    if(me->max_threads == -1) {
        jstring_append_2(string, "threads_max=\"default\">\n");
    }
    else {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "threads_max=\"%d\">\n", me->max_threads);
        jstring_append_2(string, tmpbuf);
    }

    jstring_append_2(string, "<!-- NegativeCache timeout - seconds -->\n");
    jstring_append_2(string, "<NegativeCache\n");
    timeout = (int) (me->nc_timeout_init / 1000);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "  timeout=\"%d\"", timeout);
    jstring_append_2(string, tmpbuf);
    timeout = (int) (me->nc_timeout_max / 1000);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " maxTimeOut=\"%d\"\n", timeout);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "  queueSize=\"%d\"", me->ncrq_size);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " msgToRetry=\"%d\"\n", me->ncrq_retry);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "/>\n");

    jstring_append_2(string, "<!-- Traffic Shaping -->\n");
    jstring_append_2(string, "<TrafficShaping");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " size=\"%" APR_INT64_T_FMT "\"\n", me->ts_size);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " time=\""JPR_DIFF_TIME_FMT "\"\n", me->ts_time);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " interval=\"%d\"\n", me->ts_interval);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " look_ahead=\"" JPR_DIFF_TIME_FMT "\"\n"
                        , me->ts_look_ahead);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " frame=\"" JPR_DIFF_TIME_FMT "\"\n"
                        , me->ts_frame);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " reserve=\"%d\"\n", me->ts_reserve);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, " max_option=\"");
    jstring_append_2(string, me->ts_max_option == TS_MAX_OPTION_LOOK_AHEAD ? "look_ahead":"frame");
    jstring_append_2(string, "\"");
    jstring_append_2(string, "/>\n");

    jstring_append_2(string, "</jxta:EndPointConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_EndPointConfigAdvertisement *jxta_EndPointConfigAdvertisement_construct(Jxta_EndPointConfigAdvertisement * self)
{
    self = (Jxta_EndPointConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:EndPointConfig",
                                     Jxta_EndPointConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_EndPointConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->nc_timeout_init = (Jxta_time_diff) 15 * 1000;
        self->nc_timeout_max = (Jxta_time_diff) 5 * 60 * 1000;
        self->ncrq_size = 5;
        self->ncrq_retry = 20;
        self->init_threads = -1;
        self->max_threads = -1;
        self->ts_size = -1;
        self->ts_time = -1;
        self->ts_interval = -1;
        self->ts_frame = -1;
        self->ts_look_ahead = -1;
        self->ts_reserve = -1;
        self->ts_max_option = -1;
    }

    return self;
}

void jxta_EndPointConfigAdvertisement_destruct(Jxta_EndPointConfigAdvertisement * self)
{

    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_EndPointConfigAdvertisement *) jxta_EndPointConfigAdvertisement_new(void)
{
    Jxta_EndPointConfigAdvertisement *ad =
        (Jxta_EndPointConfigAdvertisement *) calloc(1, sizeof(Jxta_EndPointConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_EndPointConfigAdvertisement_delete, 0);

    return jxta_EndPointConfigAdvertisement_construct(ad);
}

static void jxta_EndPointConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_EndPointConfigAdvertisement_destruct((Jxta_EndPointConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_EndPointConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_EndPointConfigAdvertisement_parse_charbuffer(Jxta_EndPointConfigAdvertisement * ad, const char *buf,
                                                                     size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_EndPointConfigAdvertisement_parse_file(Jxta_EndPointConfigAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

int endpoint_config_threads_init(Jxta_EndPointConfigAdvertisement * me)
{
    return (-1 == me->init_threads) ? 1 : me->init_threads;
}

int endpoint_config_threads_maximum(Jxta_EndPointConfigAdvertisement * me)
{
    return (-1 == me->max_threads) ? 5 : me->max_threads;
}

/* vim: set ts=4 sw=4 et tw=130: */
