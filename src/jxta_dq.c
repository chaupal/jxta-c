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
 * $Id: jxta_dq.c,v 1.55 2006/09/29 01:28:44 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_dq.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_discovery_service.h"
#include "jxta_apr.h"

static const char *__log_cat = "DiscoveryQuery";

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_DiscoveryQuery_,
    Type_,
    Threshold_,
    PeerAdv_,
    Attr_,
    Value_,
    ExtendedQuery_
};


/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct jxta_DiscoveryQuery {
    Jxta_advertisement jxta_advertisement;
    char *Jxta_DiscoveryQuery;
    short Type;
    int Threshold;
    JString *PeerAdv;
    JString *Attr;
    JString *Value;
    JString *ExtendedQuery;
    const Jxta_qos * qos;
};

/**
 * Delete a discovery query advertisement.
 *
 * @param pointer to discovery query advertisement to delete.
 *
 * @return void Doesn't return anything.
 */
static void discovery_query_free(void * me);


/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_DiscoveryQuery(void *userdata, const XML_Char * cd, int len)
{
    /*Jxta_DiscoveryQuery * ad = (Jxta_DiscoveryQuery*)userdata; */
}

static void handleType(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;
    char *tok;

    /* XXXX hamada@jxta.org this can be cleaned up once parsing is corrected */
    if (len > 0) {
        tok = malloc(len + 1);
        if (tok == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            return;
        }
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->Type = (short) strtol(cd, NULL, 0);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Type is :%d\n", ad->Type);
        }
        free(tok);
    }
}

static void handleThreshold(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;
    char *tok;
    /* XXXX hamada@jxta.org this can be cleaned up once parsing is corrected */
    if (len > 0) {
        tok = malloc(len + 1);
        if (tok == NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
            return;
        }
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->Threshold = (int) strtol(cd, NULL, 0);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Threshold is :%d\n", ad->Threshold);
        }
        free(tok);
    }
}

static void handlePeerAdv(void *userdata, const XML_Char * cd, int len)
{
    if (len != 0) {
        Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;
        if (NULL != ad->PeerAdv) {
            JXTA_OBJECT_RELEASE(ad->PeerAdv);
        }
        ad->PeerAdv = jstring_new_1(len + 1);
        if (NULL == ad->PeerAdv) {
            return;
        }
        jstring_append_0(ad->PeerAdv, cd, len);
    }
}

static void handleAttr(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;

    /* skip begin tag */
    if (0 == len) {
        return;
    }

    if (!ad->Attr) {
        ad->Attr = jstring_new_1(len);
    }
    jstring_append_0((JString *) ad->Attr, cd, len);
    jstring_trim(ad->Attr);
}

static void handleValue(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;

    /* skip begin tag */
    if (0 == len) {
        return;
    }

    if (!ad->Value) {
        ad->Value = jstring_new_1(len);
    }
    jstring_append_0((JString *) ad->Value, cd, len);
    jstring_trim(ad->Value);
}

static void handleExtendedQuery(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryQuery *ad = (Jxta_DiscoveryQuery *) userdata;

    /* skip begin tag */
    if (0 == len) {
        return;
    }

    if (!ad->ExtendedQuery) {
       ad->ExtendedQuery = jstring_new_1(len);
    } 
    jstring_append_0((JString *) ad->ExtendedQuery, cd, len);
    jstring_trim(ad->ExtendedQuery);
}

JXTA_DECLARE(short) jxta_discovery_query_get_type(Jxta_DiscoveryQuery * ad)
{
    return ad->Type;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_type(Jxta_DiscoveryQuery * ad, short type)
{
    if (type < DISC_PEER || type > DISC_ADV) {
        return JXTA_INVALID_ARGUMENT;
    }
    ad->Type = type;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(int) jxta_discovery_query_get_threshold(Jxta_DiscoveryQuery * ad)
{
    return ad->Threshold;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_threshold(Jxta_DiscoveryQuery * ad, int threshold)
{
    if (threshold < 0) {
        return JXTA_INVALID_ARGUMENT;
    }

    ad->Threshold = threshold;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_peeradv(Jxta_DiscoveryQuery * ad, JString ** padv)
{
    if (NULL == ad->PeerAdv) {
        return JXTA_ITEM_NOTFOUND;
    }

    jstring_trim(ad->PeerAdv);
    JXTA_OBJECT_SHARE(ad->PeerAdv);
    *padv = ad->PeerAdv;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_peeradv(Jxta_DiscoveryQuery * ad, JString * padv)
{
    if (ad == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->PeerAdv != NULL) {
        JXTA_OBJECT_RELEASE(ad->PeerAdv);
        ad->PeerAdv = NULL;
    }
    ad->PeerAdv = padv;
    if (NULL != padv) {
        JXTA_OBJECT_SHARE(padv);
    }
    return JXTA_SUCCESS;
}

/**
 * @todo return the appropriate status.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_attr(Jxta_DiscoveryQuery * ad, JString ** attr)
{
    if (ad->Attr) 
        JXTA_OBJECT_SHARE(ad->Attr);
    *attr = ad->Attr;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_attr(Jxta_DiscoveryQuery * ad, JString * attr)
{
    if (ad == NULL || attr == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->Attr) {
        JXTA_OBJECT_RELEASE(ad->Attr);
        ad->Attr = NULL;
    }
    JXTA_OBJECT_SHARE(attr);
    ad->Attr = attr;
    jstring_trim(ad->Attr);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_value(Jxta_DiscoveryQuery * ad, JString ** value)
{
    if (ad->Value) {
        JXTA_OBJECT_SHARE(ad->Value);
    }
    *value = ad->Value;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_set_value(Jxta_DiscoveryQuery * ad, JString * value)
{
    if (ad == NULL || value == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->Value) {
        JXTA_OBJECT_RELEASE(ad->Value);
        ad->Value = NULL;
    }
    JXTA_OBJECT_SHARE(value);
    ad->Value = value;
    jstring_trim(ad->Value);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_extended_query(Jxta_DiscoveryQuery * ad, JString ** value)
{
    if (ad->ExtendedQuery) {
        JXTA_OBJECT_SHARE(ad->ExtendedQuery);
    }
    *value = ad->ExtendedQuery;
    return JXTA_SUCCESS;
}

/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_DiscoveryQuery_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:DiscoveryQuery", Jxta_DiscoveryQuery_, *handleJxta_DiscoveryQuery, NULL, NULL},
    {"Type", Type_, *handleType, NULL, NULL},
    {"Threshold", Threshold_, *handleThreshold, NULL, NULL},
    {"PeerAdv", PeerAdv_, *handlePeerAdv, NULL, NULL},
    {"Attr", Attr_, *handleAttr, NULL, NULL},
    {"Value", Value_, *handleValue, NULL, NULL},
    {"ExtendedQuery", ExtendedQuery_, *handleExtendedQuery, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_discovery_query_get_xml(Jxta_DiscoveryQuery * adv, JString ** document)
{
    JString *doc;
    JString *tmps = NULL;
    Jxta_status status;

    char *buf = calloc(1, 128);
    if (buf == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return JXTA_NOMEM;
    }
    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    if (doc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        free(buf);
        return JXTA_NOMEM;
    }
    jstring_append_2(doc, "<!DOCTYPE jxta:DiscoveryQuery>");
    jstring_append_2(doc, "<jxta:DiscoveryQuery>\n");

    apr_snprintf(buf, 128, "%d\n", adv->Type);
    jstring_append_2(doc, "<Type>");
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</Type>\n");

    apr_snprintf(buf, 128, "%d\n", adv->Threshold);
    jstring_append_2(doc, "<Threshold>");
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</Threshold>\n");
    /* done with buf, free it */
    free(buf);

    if (adv->PeerAdv) {
        jstring_append_2(doc, "<PeerAdv>");
        status = jxta_xml_util_encode_jstring(adv->PeerAdv, &tmps);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "error encoding the PeerAdv, return status :%d\n", status);
            JXTA_OBJECT_RELEASE(doc);
            return status;
        }
        jstring_append_1(doc, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        jstring_append_2(doc, "</PeerAdv>\n");
    }

    if (NULL != adv->Attr && jstring_length(adv->Attr) > 0) {
        jstring_append_2(doc, "<Attr>");
        jstring_append_1(doc, adv->Attr);
        jstring_append_2(doc, "</Attr>\n");
    }

    if (NULL != adv->Value && jstring_length(adv->Value) > 0) {
        jstring_append_2(doc, "<Value>");
        jstring_append_1(doc, adv->Value);
        jstring_append_2(doc, "</Value>\n");
    }

    if (NULL != adv->ExtendedQuery && jstring_length(adv->ExtendedQuery) > 0) {
        jstring_append_2(doc, "<ExtendedQuery>");
        status = jxta_xml_util_encode_jstring(adv->ExtendedQuery, &tmps);
        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "error encoding the Extended query, return status :%d\n", status);
            JXTA_OBJECT_RELEASE(doc);
            return status;
        }
        jstring_append_1(doc, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        jstring_append_2(doc, "</ExtendedQuery>");
    }
    jstring_append_2(doc, "</jxta:DiscoveryQuery>\n");

    *document = doc;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new(void)
{
    Jxta_DiscoveryQuery *ad;
    
    ad = (Jxta_DiscoveryQuery *) malloc(sizeof(Jxta_DiscoveryQuery));
    if (ad == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return NULL;
    }
    memset(ad, 0, sizeof(Jxta_DiscoveryQuery));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:DiscoveryQuery",
                                  Jxta_DiscoveryQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_discovery_query_get_xml,
                                  NULL, NULL, discovery_query_free);

    ad->Type = 0;
    ad->Threshold = 0;
    ad->PeerAdv = NULL;
    ad->Attr = NULL;
    ad->Value = NULL;
    ad->ExtendedQuery = NULL;
    ad->qos = NULL;
    return ad;
}

/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new_1(short type, const char *attr, const char *value, int threshold,
                                                               JString * peeradv)
{
    Jxta_DiscoveryQuery *ad;

    ad = (Jxta_DiscoveryQuery *) malloc(sizeof(Jxta_DiscoveryQuery));
    if (ad == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return NULL;
    }
    memset(ad, 0, sizeof(Jxta_DiscoveryQuery));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:DiscoveryQuery",
                                  Jxta_DiscoveryQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_discovery_query_get_xml,
                                  NULL, NULL, discovery_query_free);

    ad->Type = type;
    ad->Threshold = threshold;
    ad->PeerAdv = peeradv;
    if (peeradv) {
        JXTA_OBJECT_SHARE(peeradv);
    }
    ad->Attr = jstring_new_2(attr);
    ad->Value = jstring_new_2(value);
    if (ad->Attr == NULL || ad->Value == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        JXTA_OBJECT_RELEASE(ad);
        return NULL;
    }
    ad->ExtendedQuery = NULL;
    ad->qos = NULL;
    return ad;
}

JXTA_DECLARE(Jxta_DiscoveryQuery *) jxta_discovery_query_new_2(const char *query, int threshold, JString * peeradv)
{
    Jxta_DiscoveryQuery *ad;

    ad = (Jxta_DiscoveryQuery *) malloc(sizeof(Jxta_DiscoveryQuery));
    if (ad == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        return NULL;
    }
    memset(ad, 0, sizeof(Jxta_DiscoveryQuery));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:DiscoveryQuery",
                                  Jxta_DiscoveryQuery_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_discovery_query_get_xml,
                                  NULL, NULL, discovery_query_free);

    ad->Type = DISC_ADV;
    ad->Threshold = threshold;
    ad->PeerAdv = peeradv;
    if (peeradv) { 
        JXTA_OBJECT_SHARE(peeradv);
    }
    ad->ExtendedQuery = jstring_new_2(query);
    if (ad->ExtendedQuery == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Out of memory\n");
        JXTA_OBJECT_RELEASE(ad);
        return NULL;
    }
    ad->Attr = NULL;
    ad->Value = NULL;
    ad->qos = NULL;
    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void discovery_query_free(void * me)
{
    Jxta_DiscoveryQuery * ad = (Jxta_DiscoveryQuery *)me;

    /* Fill in the required freeing functions here. */
    if (ad->Attr != NULL)
        JXTA_OBJECT_RELEASE(ad->Attr);
    if (ad->Value != NULL)
        JXTA_OBJECT_RELEASE(ad->Value);
    if (ad->ExtendedQuery != NULL)
        JXTA_OBJECT_RELEASE(ad->ExtendedQuery);
    JXTA_OBJECT_RELEASE(ad->PeerAdv);
    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(Jxta_DiscoveryQuery));
    free(ad);
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_parse_charbuffer(Jxta_DiscoveryQuery * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    /* xxx when the above returns a status we should return it, for now return success */
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_parse_file(Jxta_DiscoveryQuery * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    /* xxx when the above returns a status we should return it, for now return success */
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_query_attach_qos(Jxta_discovery_query * me, const Jxta_qos * qos)
{
    me->qos = qos;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(const Jxta_qos *) jxta_discovery_query_qos(Jxta_discovery_query * me)
{
    return me->qos;
}
/* vi: set ts=4 sw=4 tw=130 et: */
