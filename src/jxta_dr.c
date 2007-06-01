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
 * $Id: jxta_dr.c,v 1.66 2005/11/22 22:00:57 mmx2005 Exp $
 */

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_dr.h"
#include "jxta_pa.h"
#include "jxta_dr_priv.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_discovery_service.h"
#include "jxta_apr.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
/** Each of these corresponds to a tag in the
 * xml ad.
 */ enum tokentype {
    Null_,
    Jxta_DiscoveryResponse_,
    Type_,
    Count_,
    PeerAdv_,
    Attr_,
    Value_,
    Response_
};


/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _Jxta_DiscoveryResponse {
    Jxta_advertisement jxta_advertisement;
    short Type;
    int Count;
    JString *PeerAdv;
    JString *Attr;
    JString *Value;
    Jxta_vector *responselist;
    Jxta_advertisement *peer_advertisement;
    Jxta_vector *advertisements;
    long resolver_query_id;
};

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_DiscoveryResponse(void *userdata, const XML_Char * cd, int len)
{
    JXTA_LOG("In Jxta_DiscoveryResponse element\n");
}

static void handleType(void *userdata, const XML_Char * cd, int len)
{

    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;
    char *tok;
    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->Type = (short) strtol(cd, NULL, 0);
            JXTA_LOG("Type is :%d\n", ad->Type);
        }
        free(tok);
    }
    JXTA_LOG("In Type element\n");
}

static void handleCount(void *userdata, const XML_Char * cd, int len)
{

    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;
    char *tok;
    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->Count = (int) strtol(cd, NULL, 0);
            JXTA_LOG("Count is :%d\n", ad->Count);
        }
        free(tok);
    }
    JXTA_LOG("In Count element\n");
}

static void handlePeerAdv(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;
    jstring_append_0((JString *) ad->PeerAdv, cd, len);
    jstring_trim(ad->PeerAdv);
}

static void handleAttr(void *userdata, const XML_Char * cd, int len)
{
    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;

    if (len == 0)
        return;
    jstring_append_0((JString *) ad->Attr, cd, len);
}

static void handleValue(void *userdata, const XML_Char * cd, int len)
{

    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;
    if (len == 0)
        return;
    jstring_append_0((JString *) ad->Value, cd, len);
}

static void DRE_Free(Jxta_object * o)
{
    Jxta_DiscoveryResponseElement *dre = (Jxta_DiscoveryResponseElement *) o;
    JXTA_OBJECT_RELEASE(dre->response);
    free((void *) o);
}

static void handleResponse(void *userdata, const XML_Char * cd, int len)
{

    Jxta_DiscoveryResponse *ad = (Jxta_DiscoveryResponse *) userdata;
    JString *a;
    Jxta_DiscoveryResponseElement *r;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (len == 0)
        return;

    /*
     * This tag can only be handled properly with the JXTA_ADV_PAIRED_CALLS
     * option. We depend on it. There must be exactly one non-empty call
     * per address.
     */

    a = jstring_new_1(len);
    jstring_append_0(a, cd, len);
    jstring_trim(a);

    if (jstring_length(a) == 0) {
        JXTA_OBJECT_RELEASE(a);
        return;
    }

    /*
     * FIXME: jice@jxta.org 20020404 - Mo, you forgot to JXTA_OBJECT_INIT
     * r. (and also to release it after the vector got his own ref.
     */
    r = (Jxta_DiscoveryResponseElement *) calloc(1, sizeof(Jxta_DiscoveryResponseElement));
    JXTA_OBJECT_INIT(r, DRE_Free, 0);

    r->expiration = 0;
    if (atts != NULL) {
        if (!strncmp("Expiration", atts[0], sizeof("Expiration") - 1)) {
            r->expiration = atol(atts[1]);
        }
    }
    r->response = a;
    jxta_vector_add_object_last(ad->responselist, (Jxta_object *) r);
    JXTA_OBJECT_RELEASE(r);

    JXTA_LOG("In Response element\n");
}

JXTA_DECLARE(long) jxta_discovery_response_get_query_id(Jxta_DiscoveryResponse * ad)
{
    return ad->resolver_query_id;
}

JXTA_DECLARE(void) jxta_discovery_response_set_query_id(Jxta_DiscoveryResponse *me, long qid)
{
    me->resolver_query_id = qid;
}

JXTA_DECLARE(short) jxta_discovery_response_get_type(Jxta_DiscoveryResponse * ad)
{
    return ad->Type;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_type(Jxta_DiscoveryResponse * ad, short type)
{
    if (type < DISC_PEER || type > DISC_ADV) {
        return JXTA_INVALID_ARGUMENT;
    }
    ad->Type = type;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(int) jxta_discovery_response_get_count(Jxta_DiscoveryResponse * ad)
{
    return ad->Count;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_count(Jxta_DiscoveryResponse * ad, int count)
{
    ad->Count = count;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_peeradv(Jxta_DiscoveryResponse * ad, JString ** padv)
{
    if (ad->PeerAdv) {
        jstring_trim(ad->PeerAdv);
        JXTA_OBJECT_SHARE(ad->PeerAdv);
    }
    *padv = ad->PeerAdv;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_peeradv(Jxta_DiscoveryResponse * ad, JString * padv)
{
    if (ad == NULL || padv == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->PeerAdv != NULL) {
        JXTA_OBJECT_RELEASE(ad->PeerAdv);
        ad->PeerAdv = NULL;
    }
    JXTA_OBJECT_SHARE(padv);
    ad->PeerAdv = padv;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_peer_advertisement(Jxta_DiscoveryResponse * ad,
                                                                         Jxta_advertisement ** peer_advertisement)
{
    if (ad->peer_advertisement) {
        JXTA_OBJECT_SHARE(ad->peer_advertisement);
    } else if (jstring_length(ad->PeerAdv) > 0) {
        ad->peer_advertisement = (Jxta_advertisement *) jxta_PA_new();
        jxta_PA_parse_charbuffer((Jxta_PA *) ad->peer_advertisement, jstring_get_string(ad->PeerAdv), jstring_length(ad->PeerAdv));
        JXTA_OBJECT_SHARE(ad->peer_advertisement);
    }
    *peer_advertisement = ad->peer_advertisement;
    return JXTA_SUCCESS;
}

Jxta_status jxta_discovery_response_set_peeradvertisement(Jxta_DiscoveryResponse * ad, Jxta_advertisement * peer_advertisement)
{
    if (ad == NULL || peer_advertisement == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->peer_advertisement != NULL) {
        JXTA_OBJECT_RELEASE(ad->peer_advertisement);
        ad->peer_advertisement = NULL;
    }

    JXTA_OBJECT_SHARE(peer_advertisement);
    ad->peer_advertisement = peer_advertisement;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_attr(Jxta_DiscoveryResponse * ad, JString ** attr)
{
    jstring_trim(ad->Attr);
    JXTA_OBJECT_SHARE(ad->Attr);
    *attr = ad->Attr;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_attr(Jxta_DiscoveryResponse * ad, JString * attr)
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
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_value(Jxta_DiscoveryResponse * ad, JString ** value)
{
    jstring_trim(ad->Value);
    JXTA_OBJECT_SHARE(ad->Value);
    *value = ad->Value;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_value(Jxta_DiscoveryResponse * ad, JString * value)
{
    if (ad == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->Value) {
        JXTA_OBJECT_RELEASE(ad->Value);
        ad->Value = NULL;
    }
    JXTA_OBJECT_SHARE(value);
    ad->Value = value;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_responses(Jxta_DiscoveryResponse * ad, Jxta_vector ** responses)
{
    if (ad->responselist != NULL) {
        JXTA_OBJECT_SHARE(ad->responselist);
        *responses = ad->responselist;
    } else {
        *responses = NULL;
    }
    return JXTA_SUCCESS;
}


JXTA_DECLARE(void) jxta_discovery_response_set_responses(Jxta_DiscoveryResponse * ad, Jxta_vector * responselist)
{
    if (ad->responselist != NULL) {
        JXTA_OBJECT_RELEASE(ad->responselist);
        ad->responselist = NULL;
    }
    if (responselist != NULL) {
        JXTA_OBJECT_SHARE(responselist);
        ad->responselist = responselist;
    }
    return;
}


JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_advertisements(Jxta_DiscoveryResponse * ad, Jxta_vector ** advertisements)
{
    Jxta_DiscoveryResponseElement *res;
    Jxta_advertisement *radv = NULL;
    unsigned int i = 0;

    if (jxta_vector_size(ad->advertisements) < jxta_vector_size(ad->responselist)) {
        Jxta_vector *adv_vec = NULL;
        if (NULL != ad->advertisements) {
            JXTA_OBJECT_RELEASE(ad->advertisements);
        }
        ad->advertisements = jxta_vector_new(1);
        for (i = 0; i < jxta_vector_size(ad->responselist); i++) {
            Jxta_object *tmpadv = NULL;
            res = NULL;
            jxta_vector_get_object_at(ad->responselist, JXTA_OBJECT_PPTR(&res), i);
            if (res != NULL) {
                radv = jxta_advertisement_new();
                jxta_advertisement_parse_charbuffer(radv, jstring_get_string(res->response), jstring_length(res->response));
                jxta_advertisement_get_advs(radv, &adv_vec);
                jxta_vector_get_object_at(adv_vec, &tmpadv, 0);
                if (tmpadv != NULL) {
                    jxta_vector_add_object_last(ad->advertisements, (Jxta_object *) tmpadv);
                }
                /* call listener(s) if any */
                JXTA_OBJECT_RELEASE(res);
                JXTA_OBJECT_RELEASE(radv);
                JXTA_OBJECT_RELEASE(tmpadv);
                JXTA_OBJECT_RELEASE(adv_vec);
            }
        }
    }
    *advertisements = JXTA_OBJECT_SHARE(ad->advertisements);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_discovery_response_set_advertisements(Jxta_DiscoveryResponse * ad, Jxta_vector * advertisements)
{
    if (ad->advertisements != NULL) {
        JXTA_OBJECT_RELEASE(ad->advertisements);
        ad->advertisements = NULL;
    }
    if (advertisements != NULL) {
        JXTA_OBJECT_SHARE(advertisements);
        ad->advertisements = advertisements;
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
static const Kwdtab Jxta_DiscoveryResponse_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:DiscoveryResponse", Jxta_DiscoveryResponse_, *handleJxta_DiscoveryResponse, NULL, NULL},
    {"Type", Type_, *handleType, NULL, NULL},
    {"Count", Count_, *handleCount, NULL, NULL},
    {"PeerAdv", PeerAdv_, *handlePeerAdv, NULL, NULL},
    {"Attr", Attr_, *handleAttr, NULL, NULL},
    {"Value", Value_, *handleValue, NULL, NULL},
    {"Response", Response_, *handleResponse, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


Jxta_status response_print(Jxta_DiscoveryResponse * ad, JString * js)
{

    char buf[128];
    Jxta_vector *responselist;
    int eachElement;
    JString *tmps = NULL;
    Jxta_status status;

    jxta_discovery_response_get_responses(ad, &responselist);

    for (eachElement = jxta_vector_size(responselist) - 1; eachElement >= 0; eachElement--) {
        Jxta_DiscoveryResponseElement *anElement = NULL;
        jxta_vector_get_object_at(responselist, JXTA_OBJECT_PPTR(&anElement), eachElement);
        if (NULL == anElement) {
            continue;
        }

        JXTA_OBJECT_CHECK_VALID(anElement);
        apr_snprintf(buf, sizeof(buf), "<Response Expiration=\"%" APR_INT64_T_FMT "\">\n", anElement->expiration);
        jstring_append_2(js, buf);
        status = jxta_xml_util_encode_jstring(anElement->response, &tmps);
        if (status != JXTA_SUCCESS) {
            JXTA_LOG("error encoding the response, retrun status :%d\n", status);
            return status;
        }

        jstring_append_1(js, tmps);
        JXTA_OBJECT_RELEASE(tmps);
        jstring_append_2(js, "</Response>\n");

        JXTA_OBJECT_RELEASE(anElement);
    }
    JXTA_OBJECT_RELEASE(responselist);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_xml(Jxta_DiscoveryResponse * ad, JString ** document)
{

    JString *doc;
    JString *tmps = NULL;
    Jxta_status status;
    char *buf = calloc(1, 128);

    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:DiscoveryResponse>\n");

    jstring_append_2(doc, "<jxta:DiscoveryResponse>\n");

    jstring_append_2(doc, "<Type>");
    apr_snprintf(buf, 128, "%d", ad->Type);
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</Type>\n");

    jstring_append_2(doc, "<Count>");
    apr_snprintf(buf, 128, "%d", ad->Count);
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</Count>\n");

    jstring_append_2(doc, "<PeerAdv>");
    status = jxta_xml_util_encode_jstring(ad->PeerAdv, &tmps);
    if (status != JXTA_SUCCESS) {
        JXTA_LOG("error encoding the PeerAdv, retrun status :%d\n", status);
        JXTA_OBJECT_RELEASE(doc);
        return status;
    }
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</PeerAdv>\n");

    jstring_append_2(doc, "<Attr>");
    jstring_append_2(doc, "</Attr>\n");

    jstring_append_2(doc, "<Value>");
    jstring_append_2(doc, "</Value>\n");
    response_print(ad, doc);
    jstring_append_2(doc, "</jxta:DiscoveryResponse>\n");

    free(buf);
    *document = doc;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_DiscoveryResponse *) jxta_discovery_response_new(void)
{

    Jxta_DiscoveryResponse *ad;
    ad = (Jxta_DiscoveryResponse *) malloc(sizeof(Jxta_DiscoveryResponse));

    memset(ad, 0x0, sizeof(Jxta_DiscoveryResponse));
    ad->PeerAdv = jstring_new_0();
    ad->responselist = jxta_vector_new(4);
    ad->advertisements = jxta_vector_new(4);
    ad->Attr = jstring_new_0();
    ad->Value = jstring_new_0();
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:DiscoveryResponse",
                                  Jxta_DiscoveryResponse_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_discovery_response_get_xml,
                                  NULL, NULL, (FreeFunc) jxta_discovery_response_free);
    return ad;
}

JXTA_DECLARE(Jxta_DiscoveryResponse *) jxta_discovery_response_new_1(short type,
                                                                     char *attr,
                                                                     char *value, int threshold, JString * peeradv,
                                                                     Jxta_vector * responses)
{

    Jxta_DiscoveryResponse *ad;
    ad = (Jxta_DiscoveryResponse *) malloc(sizeof(Jxta_DiscoveryResponse));

    memset(ad, 0x0, sizeof(Jxta_DiscoveryResponse));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:DiscoveryResponse",
                                  Jxta_DiscoveryResponse_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_discovery_response_get_xml,
                                  NULL, NULL, (FreeFunc) jxta_discovery_response_free);

    JXTA_OBJECT_SHARE(peeradv);
    JXTA_OBJECT_SHARE(responses);

    ad->PeerAdv = peeradv;
    ad->responselist = responses;
    ad->Type = type;
    ad->Attr = jstring_new_2(attr);
    ad->Value = jstring_new_2(value);
    ad->Count = threshold;

    return ad;
}

void jxta_discovery_response_free(Jxta_DiscoveryResponse * ad)
{
    if (ad->PeerAdv)
        JXTA_OBJECT_RELEASE(ad->PeerAdv);
    if (ad->Attr)
        JXTA_OBJECT_RELEASE(ad->Attr);
    if (ad->Value)
        JXTA_OBJECT_RELEASE(ad->Value);
    if (ad->responselist)
        JXTA_OBJECT_RELEASE(ad->responselist);
    if (ad->advertisements)
        JXTA_OBJECT_RELEASE(ad->advertisements);
    if (ad->peer_advertisement)
        JXTA_OBJECT_RELEASE(ad->peer_advertisement);
    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0x0, sizeof(Jxta_DiscoveryResponse));
    free(ad);
}



JXTA_DECLARE(void) jxta_discovery_response_parse_charbuffer(Jxta_DiscoveryResponse * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_discovery_response_parse_file(Jxta_DiscoveryResponse * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

static void response_element_free(Jxta_object * o)
{
    Jxta_DiscoveryResponseElement *dre = (Jxta_DiscoveryResponseElement *) o;
    if (dre->response != NULL)
        JXTA_OBJECT_RELEASE(dre->response);
    free((void *) o);
}


JXTA_DECLARE(Jxta_DiscoveryResponseElement *) jxta_discovery_response_new_element(void)
{

    Jxta_DiscoveryResponseElement *dre = (Jxta_DiscoveryResponseElement *) malloc(sizeof(Jxta_DiscoveryResponseElement));
    memset(dre, 0x0, sizeof(Jxta_DiscoveryResponseElement));
    JXTA_OBJECT_INIT(dre, response_element_free, 0);
    return dre;
}

JXTA_DECLARE(Jxta_DiscoveryResponseElement *) jxta_discovery_response_new_element_1(JString * response,
                                                                                    Jxta_expiration_time expiration)
{

    Jxta_DiscoveryResponseElement *dre = jxta_discovery_response_new_element();

    JXTA_OBJECT_SHARE(response);
    if (dre->response != NULL)
        JXTA_OBJECT_RELEASE(dre->response);

    dre->response = response;
    dre->expiration = expiration;
    return dre;
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
