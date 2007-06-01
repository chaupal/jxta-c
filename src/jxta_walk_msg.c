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
 * $Id: jxta_walk_msg.c,v 1.7 2005/08/18 19:01:52 slowhog Exp $
 */

static const char *__log_cat = "WalkMsg";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxtaapr.h"

#include "jxta_walk_msg.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif
    /** Each of these corresponds to a tag in the
     * xml ad.
     */ enum tokentype {
    Null_,
    LimitedRangeRdvMessage_,
    TTL_,
    Dir_,
    SrcPeerID_,
    SrcSvcName_,
    SrcSvcParams_
};

    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
struct _LimitedRangeRdvMessage {
    Jxta_advertisement jxta_advertisement;

    int ttl;
    Walk_direction dir;

    JString *src_peer_id;

    JString *svc_name;
    JString *svc_params;
};

    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */

static void handleLimitedRangeRdvMessage(void *userdata, const XML_Char * cd, int len)
{
    /* LimitedRangeRdvMessage * ad = (LimitedRangeRdvMessage*)userdata; */
}

static void handleTTL(void *userdata, const XML_Char * cd, int len)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) userdata;

    char *tok = (char *) calloc(len + 1, sizeof(char));

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->ttl = atoi(tok);
    }

    free(tok);
}

static void handleDir(void *userdata, const XML_Char * cd, int len)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) userdata;

    char *tok = (char *) calloc(len + 1, sizeof(char));

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->dir = (Walk_direction) atoi(tok);
    }

    free(tok);
}

static void handleSrcPeerID(void *userdata, const XML_Char * cd, int len)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) userdata;

    ad->src_peer_id = jstring_new_1(len);
    jstring_append_0(ad->src_peer_id, cd, len);
    jstring_trim(ad->src_peer_id);
}

static void handleSrcSvcName(void *userdata, const XML_Char * cd, int len)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) userdata;

    ad->svc_name = jstring_new_1(len);
    jstring_append_0(ad->svc_name, cd, len);
    jstring_trim(ad->svc_name);
}

static void handleSrcSvcParams(void *userdata, const XML_Char * cd, int len)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) userdata;

    ad->svc_params = jstring_new_1(len);
    jstring_append_0(ad->svc_params, cd, len);
    jstring_trim(ad->svc_params);
}

JXTA_DECLARE(int) LimitedRangeRdvMessage_get_TTL(LimitedRangeRdvMessage * ad)
{
    return ad->ttl;
}

JXTA_DECLARE(void) LimitedRangeRdvMessage_set_TTL(LimitedRangeRdvMessage * ad, int ttl)
{
    ad->ttl = ttl;
}

JXTA_DECLARE(Walk_direction) LimitedRangeRdvMessage_get_direction(LimitedRangeRdvMessage * ad)
{
    return ad->dir;
}

JXTA_DECLARE(void) LimitedRangeRdvMessage_set_direction(LimitedRangeRdvMessage * ad, Walk_direction dir)
{
    ad->dir = dir;
}

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcPeerID(LimitedRangeRdvMessage * ad)
{
    if (NULL != ad->src_peer_id) {
        return jstring_get_string(ad->src_peer_id);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcPeerID(LimitedRangeRdvMessage * ad, const char *src_peer_id)
{
    if (ad->src_peer_id != NULL) {
        JXTA_OBJECT_RELEASE(ad->src_peer_id);
        ad->src_peer_id = NULL;
    }

    if (src_peer_id != NULL) {
        ad->svc_name = jstring_new_2(src_peer_id);
    }
}

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcSvcName(LimitedRangeRdvMessage * ad)
{
    if (NULL != ad->svc_name) {
        return jstring_get_string(ad->svc_name);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcSvcName(LimitedRangeRdvMessage * ad, const char *svc_name)
{
    if (ad->svc_name != NULL) {
        JXTA_OBJECT_RELEASE(ad->svc_name);
        ad->svc_name = NULL;
    }

    if (svc_name != NULL) {
        ad->svc_name = jstring_new_2(svc_name);
    }
}

JXTA_DECLARE(const char *) LimitedRangeRdvMessage_get_SrcSvcParams(LimitedRangeRdvMessage * ad)
{
    if (NULL != ad->svc_params) {
        return jstring_get_string(ad->svc_params);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) LimitedRangeRdvMessage_set_SrcSvcParams(LimitedRangeRdvMessage * ad, const char *svc_params)
{
    if (ad->svc_params != NULL) {
        JXTA_OBJECT_RELEASE(ad->svc_params);
        ad->svc_params = NULL;
    }

    if (svc_params != NULL) {
        ad->svc_params = jstring_new_2(svc_params);
    }
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab LimitedRangeRdvMessage_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:LimitedRangeRdvMessage", LimitedRangeRdvMessage_, *handleLimitedRangeRdvMessage, NULL, NULL},
    {"TTL", TTL_, *handleTTL, NULL, NULL},
    {"Dir", Dir_, *handleDir, NULL, NULL},
    {"SrcPeerID", SrcPeerID_, *handleSrcPeerID, NULL, NULL},
    {"SrcSvcName", SrcSvcName_, *handleSrcSvcName, NULL, NULL},
    {"SrcSvcParams", SrcSvcParams_, *handleSrcSvcParams, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_get_xml(LimitedRangeRdvMessage * ad, JString ** xml)
{
    JString *string;
    char buf[12];               /* We use this buffer to store a string representation of a int < 10 */

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:LimitedRangeRdvMessage>");

    jstring_append_2(string, "<jxta:LimitedRangeRdvMessage>\n");
    jstring_append_2(string, "<TTL>");
    apr_snprintf(buf, sizeof(buf),"%d", LimitedRangeRdvMessage_get_TTL(ad));
    jstring_append_2(string, buf);
    jstring_append_2(string, "</TTL>\n");
    jstring_append_2(string, "<Dir>");
    apr_snprintf(buf, sizeof(buf),"%d", LimitedRangeRdvMessage_get_direction(ad));
    jstring_append_2(string, buf);
    jstring_append_2(string, "</Dir>\n");
    jstring_append_2(string, "<SrcPeerID>");
    jstring_append_2(string, LimitedRangeRdvMessage_get_SrcPeerID(ad));
    jstring_append_2(string, "</SrcPeerID>\n");
    jstring_append_2(string, "<SrcSvcName>");
    jstring_append_2(string, LimitedRangeRdvMessage_get_SrcSvcName(ad));
    jstring_append_2(string, "</SrcSvcName>\n");
    jstring_append_2(string, "<SrcSvcParams>");
    jstring_append_2(string, LimitedRangeRdvMessage_get_SrcSvcParams(ad));
    jstring_append_2(string, "</SrcSvcParams>\n");
    jstring_append_2(string, "</jxta:LimitedRangeRdvMessage>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(LimitedRangeRdvMessage *) LimitedRangeRdvMessage_new(void)
{
    LimitedRangeRdvMessage *ad = (LimitedRangeRdvMessage *) calloc(1, sizeof(LimitedRangeRdvMessage));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:LimitedRangeRdvMessage",
                                  LimitedRangeRdvMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) LimitedRangeRdvMessage_get_xml,
                                  NULL, NULL, (FreeFunc) LimitedRangeRdvMessage_delete);

    ad->ttl = 0;
    ad->dir = WALK_BOTH;
    ad->src_peer_id = NULL;
    ad->svc_name = NULL;
    ad->svc_params = NULL;

    return ad;
}

void LimitedRangeRdvMessage_delete(LimitedRangeRdvMessage * ad)
{
    /* Fill in the required freeing functions here. */
    if (ad->src_peer_id) {
        JXTA_OBJECT_RELEASE(ad->src_peer_id);
        ad->src_peer_id = NULL;
    }

    if (ad->svc_name) {
        JXTA_OBJECT_RELEASE(ad->svc_name);
        ad->svc_name = NULL;
    }

    if (ad->svc_params) {
        JXTA_OBJECT_RELEASE(ad->svc_params);
        ad->svc_params = NULL;
    }

    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(LimitedRangeRdvMessage));
    free(ad);
}

JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_parse_charbuffer(LimitedRangeRdvMessage * ad, const char *buf, int len)
{
    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(Jxta_status) LimitedRangeRdvMessage_parse_file(LimitedRangeRdvMessage * ad, FILE * stream)
{
    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif
