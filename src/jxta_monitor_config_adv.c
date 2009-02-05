/* 
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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

static const char *const __log_cat = "MonitorCfgAdv";

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_monitor_config_adv.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_MonitorConfigAdvertisement_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_MonitorConfigAdvertisement {
    Jxta_advertisement jxta_advertisement;
    char *config_name;
    Jxta_boolean enabled;
    apr_interval_time_t broadcast_interval;
    int init_threads;
    int max_threads;
    Jxta_vector * monitors_disabled;
};


static const apr_interval_time_t MONITOR_BROADCAST_INTERVAL_DEFAULT = 10 * APR_USEC_PER_SEC;

/* Forward decl. of un-exported function */
static void jxta_MonitorConfigAdvertisement_delete(Jxta_object *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
void handleJxta_MonitorConfigAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MonitorConfigAdvertisement *ad = (Jxta_MonitorConfigAdvertisement *) userdata;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of jxta:MonitorConfig\n");

    while (atts && *atts) {
        if (0 == strcmp(*atts, "threads_init")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->init_threads = -1;
            } else {
                ad->init_threads = atoi(atts[1]);
            }
        } else if (0 == strcmp(*atts, "enabled")) {
            ad->enabled = (0 == strcmp(atts[1], "true")) ? TRUE:FALSE;
        } else if (0 == strcmp(*atts, "threads_max")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->max_threads = -1;
            } else {
                ad->max_threads = atoi(atts[1]);
            }
        }  else if (0 == strcmp(*atts, "broadcast_interval")) {
            if (0 == strcmp(atts[1], "default")) {
                ad->broadcast_interval = -1;
            } else {
                ad->broadcast_interval = (atoi(atts[1]) * APR_USEC_PER_SEC);
            }
        } else if (0 == strcmp(*atts, "config_name")) {
            ad->config_name = strdup(atts[1]);
        } else if (0 == strcmp(*atts, "type")) {
            /* just silently skip it. */
        }

        atts+=2;
    }
}

void handle_type(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MonitorConfigAdvertisement *ad = (Jxta_MonitorConfigAdvertisement *) userdata;

    if (0 == len) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "BEGIN <MonitorType>\n");

    } else {
        const char **atts = ((Jxta_advertisement *) ad)->atts;
        Jxta_boolean monitor = TRUE;
        JString *typeStr;

        while (atts && *atts) {
            if (0 == strcmp(*atts, "monitor")) {
                monitor = (0 == strcmp(atts[1], "true") ? TRUE:FALSE);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized addr attribute : \"%s\" = \"%s\"\n", *atts,
                                atts[1]);
            }

            atts += 2;
        }

        typeStr = jstring_new_1(len);
        jstring_append_0(typeStr, cd, len);
        jstring_trim(typeStr);

        if (!monitor) {
            jxta_vector_add_object_last(ad->monitors_disabled, (Jxta_object *) typeStr);
        }
        JXTA_OBJECT_RELEASE(typeStr);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "END <MonitorType>\n");
    }

}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Jxta_MonitorConfigAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:MonitorConfig", Null_, *handleJxta_MonitorConfigAdvertisement, NULL, NULL},
    {"MonitorType", Null_, *handle_type, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_MonitorConfigAdvertisement_get_xml(Jxta_MonitorConfigAdvertisement * me, JString ** result)
{
    char tmpbuf[256];
    JString *string = jstring_new_0();

    jstring_append_2(string, "<!-- JXTA Monitor Configuration Advertisement -->\n");
    jstring_append_2(string, "<jxta:MonitorConfig xmlns:jxta=\"http://jxta.org\" type=\"jxta:MonitorConfig\" ");

    if (NULL != me->config_name) {
        jstring_append_2(string, " config_name=\"");
        jstring_append_2(string, me->config_name);
        jstring_append_2(string, "\"\n");
    }
    jstring_append_2(string, " enabled=\"");
    jstring_append_2(string, me->enabled == TRUE ? "true\"":"false\"");

    if(me->broadcast_interval == -1) {
        jstring_append_2(string, " broadcast_interval=\"default\" ");
    }
    else {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), " broadcast_interval=\"%" APR_INT64_T_FMT "\"", me->broadcast_interval/APR_USEC_PER_SEC);
        jstring_append_2(string, tmpbuf);
    }

    if(me->init_threads == -1) {
        jstring_append_2(string, " threads_init=\"default\" ");
    }
    else {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), " threads_init=\"%d\" ", me->init_threads);
        jstring_append_2(string, tmpbuf);
    }

    if(me->max_threads == -1) {
        jstring_append_2(string, " threads_max=\"default\">\n");
    }
    else {
        apr_snprintf(tmpbuf, sizeof(tmpbuf), " threads_max=\"%d\">\n", me->max_threads);
        jstring_append_2(string, tmpbuf);
    }
    jstring_append_2(string, "</jxta:MonitorConfig>\n");

    *result = string;
    return JXTA_SUCCESS;
}


Jxta_MonitorConfigAdvertisement *jxta_MonitorConfigAdvertisement_construct(Jxta_MonitorConfigAdvertisement * self)
{
    self = (Jxta_MonitorConfigAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:MonitorConfig",
                                     Jxta_MonitorConfigAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_MonitorConfigAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->enabled=FALSE;
        self->broadcast_interval = -1;
        self->init_threads = -1;
        self->max_threads = -1;
        self->config_name = NULL;
    }

    return self;
}

void jxta_MonitorConfigAdvertisement_destruct(Jxta_MonitorConfigAdvertisement * self)
{

    if (NULL != self->config_name) {
        free(self->config_name);
    }

    jxta_advertisement_destruct((Jxta_advertisement *) self);

}

/** 
 *   Get a new instance of the ad. 
 **/
JXTA_DECLARE(Jxta_MonitorConfigAdvertisement *) jxta_MonitorConfigAdvertisement_new(void)
{
    Jxta_MonitorConfigAdvertisement *ad =
        (Jxta_MonitorConfigAdvertisement *) calloc(1, sizeof(Jxta_MonitorConfigAdvertisement));

    JXTA_OBJECT_INIT(ad, jxta_MonitorConfigAdvertisement_delete, 0);

    return jxta_MonitorConfigAdvertisement_construct(ad);
}

static void jxta_MonitorConfigAdvertisement_delete(Jxta_object * ad)
{
    jxta_MonitorConfigAdvertisement_destruct((Jxta_MonitorConfigAdvertisement *) ad);

    memset(ad, 0xDD, sizeof(Jxta_MonitorConfigAdvertisement));
    free(ad);
}

JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_parse_charbuffer(Jxta_MonitorConfigAdvertisement * ad, const char *buf,
                                                                     size_t len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_parse_file(Jxta_MonitorConfigAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_boolean)  jxta_MonitorConfig_is_service_enabled(Jxta_MonitorConfigAdvertisement * me)
{
    return me->enabled;
}

JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_set_config_name(Jxta_MonitorConfigAdvertisement * ad, const char *buf) {
    if (NULL != ad->config_name) {
        free(ad->config_name);
    }
    ad->config_name = strdup(buf);
}

JXTA_DECLARE(void) jxta_MonitorConfigAdvertisement_get_config_name(Jxta_MonitorConfigAdvertisement * ad, char **fn) {
    if (NULL != ad->config_name) {
        *fn = strdup(ad->config_name);
    }
}

JXTA_DECLARE(apr_interval_time_t) jxta_MonitorConfig_broadcast_interval(Jxta_MonitorConfigAdvertisement * me)
{
    return (-1 == me->broadcast_interval) ? MONITOR_BROADCAST_INTERVAL_DEFAULT : me->broadcast_interval;
}

int monitor_config_threads_init(Jxta_MonitorConfigAdvertisement * me)
{
    return (-1 == me->init_threads) ? 1 : me->init_threads;
}

int monitor_config_threads_maximum(Jxta_MonitorConfigAdvertisement * me)
{
    return (-1 == me->max_threads) ? 5 : me->max_threads;
}

JXTA_DECLARE(void) jxta_MonitorConfig_get_disabled_types(Jxta_MonitorConfigAdvertisement * me, Jxta_vector ** types)
{

    if (NULL != me->monitors_disabled) {
        *types = JXTA_OBJECT_SHARE(me->monitors_disabled);
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
