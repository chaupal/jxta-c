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
    Jxta_hashtable *fc_entries;
    Jxta_traffic_shaping *ts;
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

static void handleFlowControl(void *me, const XML_Char * cd, int len)
{
    Jxta_EndPointConfigAdvertisement *_self = (Jxta_EndPointConfigAdvertisement *) me;
    const char **atts = ((Jxta_advertisement *) me)->atts;
    Jxta_ep_flow_control *msg_fc;
    char *msg_type=NULL;

    msg_fc = jxta_ep_flow_control_new();

    while (atts && *atts) {
        int tmp;
        if (0 == strcmp(*atts, "msg_type")) {
            jxta_ep_fc_set_msg_type(msg_fc, atts[1]);
        } else if (0 == strcmp(*atts, "direction")) {
            Jxta_boolean both = (0 == strcmp(atts[1], DIRECTION_BOTH));

            jxta_ep_fc_set_inbound(msg_fc, 0 == strcmp(atts[1], DIRECTION_INBOUND) || both ? TRUE:FALSE);
            jxta_ep_fc_set_outbound(msg_fc, 0 == strcmp(atts[1], DIRECTION_OUTBOUND) || both ? TRUE:FALSE);
        } else if (0 == strcmp(*atts, "rate")) {
            tmp = atoi(atts[1]);
            jxta_ep_fc_set_rate(msg_fc, tmp);
        }

        atts += 2;
    }
    if (JXTA_SUCCESS == jxta_ep_fc_get_msg_type(msg_fc, &msg_type)) {
        jxta_hashtable_put(_self->fc_entries, msg_type, strlen(msg_type) + 1, (Jxta_object *) msg_fc);
    }
    if (msg_type)
        free(msg_type);

    JXTA_OBJECT_RELEASE(msg_fc);
}

static void handleTrafficShaping(void *me, const XML_Char * cd, int len)
{
    Jxta_EndPointConfigAdvertisement *_self = (Jxta_EndPointConfigAdvertisement *) me;
    const char **atts = ((Jxta_advertisement *) me)->atts;
    Jxta_traffic_shaping *ts;

    if (0 != len) return;

    ts = _self->ts;
    if (NULL == ts) {
        ts = traffic_shaping_new();
        _self->ts = ts;
    }

    while (atts && *atts) {
        int tmp;
        apr_int64_t tmp_l;

        if (0 == strcmp(*atts, "size")) {
            tmp_l = apr_atoi64(atts[1]);
            traffic_shaping_set_size(ts, tmp_l);
        } else if (0 == strcmp(*atts, "time")) {
            tmp = atoi(atts[1]);
            traffic_shaping_set_time(ts, tmp);
        } else if (0 == strcmp(*atts, "interval")) {
            tmp = atoi(atts[1]);
            traffic_shaping_set_interval(ts, tmp);
        } else if (0 == strcmp(*atts, "frame")) {
            tmp = atoi(atts[1]);
            traffic_shaping_set_frame(ts, tmp);
        } else if (0 == strcmp(*atts, "look_ahead")) {
            tmp = atoi(atts[1]);
            traffic_shaping_set_look_ahead(ts, tmp);
        } else if (0 == strcmp(*atts, "reserve")) {
            tmp = atoi(atts[1]);
            traffic_shaping_set_reserve(ts, tmp);
        } else if (0 == strcmp(*atts, "max_option")) {
            traffic_shaping_set_max_option(ts
                        , strcmp(atts[1], "frame") == 0 ? TS_MAX_OPTION_FRAME:TS_MAX_OPTION_LOOK_AHEAD);
        }
        atts += 2;
    }
}

JXTA_DECLARE(Jxta_status) jxta_epcfg_get_fc_parm(Jxta_EndPointConfigAdvertisement * me, const char *msg_type, Jxta_ep_flow_control **fc_parm)
{
    Jxta_status res;

    res = jxta_hashtable_get(me->fc_entries, msg_type, strlen(msg_type) + 1, JXTA_OBJECT_PPTR(fc_parm));

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_epcfg_set_fc_parm(Jxta_EndPointConfigAdvertisement * me, const char *msg_type, Jxta_ep_flow_control *fc_entry)
{
    Jxta_status res;

    if (JXTA_SUCCESS != jxta_hashtable_contains(me->fc_entries, msg_type, strlen(msg_type) + 1)) {
        jxta_hashtable_put(me->fc_entries, msg_type, strlen(msg_type) +1, (Jxta_object *) fc_entry);
        res = JXTA_SUCCESS;
    } else {
        res = JXTA_ITEM_EXISTS;
    }
    return res;
}

JXTA_DECLARE(void) jxta_epcfg_get_traffic_shaping(Jxta_EndPointConfigAdvertisement * me
                    , Jxta_traffic_shaping **ts)
{
    *ts = NULL;
    if (NULL != me->ts) {
        *ts = JXTA_OBJECT_SHARE(me->ts);
    }
    return;
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
    {"FlowControl", Null_, *handleFlowControl, NULL, NULL},
    {"TrafficShaping", Null_, *handleTrafficShaping, NULL, NULL},
    {"NegativeCache", Null_, *handleNegativeCache, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_EndPointConfigAdvertisement_get_xml(Jxta_EndPointConfigAdvertisement * me, JString ** result)
{
    char tmpbuf[256];
    JString *string = jstring_new_0();
    int timeout;
    char **fc_keys;
    char **fc_keys_save;

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

    if (-1 == traffic_shaping_time(me->ts)) {
        traffic_shaping_set_time(me->ts, 60);
    }

    if (-1 == traffic_shaping_size(me->ts)) {
        traffic_shaping_set_size(me->ts, 600000);
    }

    if (-1 == traffic_shaping_interval(me->ts)) {
        traffic_shaping_set_interval(me->ts, 1);
    }

    if (-1 == traffic_shaping_frame(me->ts)) {
        traffic_shaping_set_frame(me->ts, 5);
    }

    if (-1 == traffic_shaping_look_ahead(me->ts)) {
        traffic_shaping_set_look_ahead(me->ts, 20);
    }

    if (-1 == traffic_shaping_reserve(me->ts)) {
        traffic_shaping_set_reserve(me->ts, 10);
    }

    if (-1 == traffic_shaping_max_option(me->ts)) {
        traffic_shaping_set_max_option(me->ts, TS_MAX_OPTION_LOOK_AHEAD);
    }

    jstring_append_2(string, "<!-- Traffic Shaping -->\n");
    jstring_append_2(string, "<TrafficShaping");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " size=\"%" APR_INT64_T_FMT "\"\n", traffic_shaping_size( me->ts));
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " time=\"%ld\"\n", (long) traffic_shaping_time( me->ts));
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " interval=\"%d\"\n", traffic_shaping_interval(me->ts));
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " look_ahead=\"" JPR_DIFF_TIME_FMT "\"\n"
                        , traffic_shaping_look_ahead(me->ts));
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " frame=\"" JPR_DIFF_TIME_FMT "\"\n"
                        , traffic_shaping_frame(me->ts));
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " reserve=\"%d\"\n", traffic_shaping_reserve(me->ts));
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, " max_option=\"");
    jstring_append_2(string, traffic_shaping_max_option(me->ts) == TS_MAX_OPTION_LOOK_AHEAD ? "look_ahead":"frame");
    jstring_append_2(string, "\"");
    jstring_append_2(string, "/>\n");

    fc_keys = jxta_hashtable_keys_get(me->fc_entries);
    fc_keys_save = fc_keys;
    while (NULL != fc_keys && *fc_keys) {
        Jxta_ep_flow_control *fc_entry=NULL;

        if (JXTA_SUCCESS == jxta_hashtable_get(me->fc_entries, *fc_keys, strlen(*fc_keys) + 1, JXTA_OBJECT_PPTR(&fc_entry))) {
            char *tmp_char;
            const char *direction;

            jstring_append_2(string, "<FlowControl");
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " cfg_parm=\"%s\"", *fc_keys);
            jstring_append_2(string, tmpbuf);
            jxta_ep_fc_get_msg_type(fc_entry, &tmp_char);
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " msg_type=\"%s\"", tmp_char);
            jstring_append_2(string, tmpbuf);
            free(tmp_char);
            direction="both";
            if (jxta_ep_fc_outbound(fc_entry) && jxta_ep_fc_inbound(fc_entry)) {
                direction="both";
            } else if (jxta_ep_fc_outbound(fc_entry)) {
                direction="outbound";
            } else if (jxta_ep_fc_inbound(fc_entry)) {
                direction="indbound";
            }
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " direction=\"%s\"", direction);
            jstring_append_2(string, tmpbuf);
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " rate=\"%d\"", jxta_ep_fc_rate(fc_entry));
            jstring_append_2(string, tmpbuf);
            apr_snprintf(tmpbuf, sizeof(tmpbuf), " rate_window=\"%d\"", jxta_ep_fc_rate_window(fc_entry));
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "/>\n");
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Could not retrieve entry in flow control: %s\n", *fc_keys);
        }
        if (fc_entry)
            JXTA_OBJECT_RELEASE(fc_entry);
        free(*(fc_keys++));
    }
    if (fc_keys_save)
        free(fc_keys_save);
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
        self->fc_entries = jxta_hashtable_new(0);
        self->ts = traffic_shaping_new();
    }

    return self;
}

void jxta_EndPointConfigAdvertisement_destruct(Jxta_EndPointConfigAdvertisement * self)
{

    if (self->ts) {
        JXTA_OBJECT_RELEASE(self->ts);
    }
    if (self->fc_entries) {
        JXTA_OBJECT_RELEASE(self->fc_entries);
    }
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
