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
 * $Id: jxta_rsrdi.c,v 1.14 2005/08/03 05:51:19 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <expat.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_rsrdi.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
/** 
 * Each of these corresponds to a tag in the
 * xml message
 */ enum tokentype {
    Null_,
    ResolverSrdi_,
    Credential_,
    HandlerName_,
    Payload_
};

/** This is the representation of the
 * actual message in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _ResolverSrdi {
    Jxta_advertisement jxta_advertisement;
    char *ResolverSrdi;
    JString *Credential;
    JString *HandlerName;
    JString *Payload;
};

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleResolverSrdi(void *userdata, const XML_Char * cd, int len)
{
    /*ResolverSrdi * ad = (ResolverSrdi*)userdata; */
}

static void handleCredential(void *userdata, const XML_Char * cd, int len)
{
    ResolverSrdi *ad = (ResolverSrdi *) userdata;
    jstring_append_0(((struct _ResolverSrdi *) ad)->Credential, cd, len);
}

static void handleHandlerName(void *userdata, const XML_Char * cd, int len)
{
    ResolverSrdi *ad = (ResolverSrdi *) userdata;
    jstring_append_0((JString *) ad->HandlerName, cd, len);
}

static void handlePayload(void *userdata, const XML_Char * cd, int len)
{
    ResolverSrdi *ad = (ResolverSrdi *) userdata;
    jstring_append_0(ad->Payload, (char *) cd, len);
}

static void trim_elements(ResolverSrdi * adv)
{
    jstring_trim(adv->Credential);
    jstring_trim(adv->HandlerName);
    jstring_trim(adv->Payload);
}

/**
 * return a JString represntation of the advertisement
 * it is the responsiblity of the caller to release the JString object
 * @param adv a pointer to the advertisement.
 * @param doc  a pointer to the JString object representing the document.
 * @return Jxta_status 
 */

Jxta_status jxta_resolver_srdi_get_xml(ResolverSrdi * adv, JString ** document)
{

    JString *doc;
    JString *tmps;
    Jxta_status status;

    if (adv == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    trim_elements(adv);
    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:ResolverSRDI>");
    jstring_append_2(doc, "<jxta:ResolverSRDI>\n");

    if (jstring_length(adv->Credential) != 0) {
        jstring_append_2(doc, "<jxta:Cred>");
        jstring_append_1(doc, adv->Credential);
        jstring_append_2(doc, "</jxta:Cred>\n");
    }

    jstring_append_2(doc, "<HandlerName>");
    jstring_append_1(doc, adv->HandlerName);
    jstring_append_2(doc, "</HandlerName>\n");

    jstring_append_2(doc, "<Payload>");
    status = jxta_xml_util_encode_jstring(adv->Payload, &tmps);

    if (status != JXTA_SUCCESS) {
        JXTA_LOG("error encoding the payload, retrun status :%d\n", status);
        JXTA_OBJECT_RELEASE(doc);
        return status;
    }
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</Payload>\n");
    jstring_append_2(doc, "</jxta:ResolverSRDI>\n");
    *document = doc;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(JString *) jxta_resolver_srdi_get_credential(ResolverSrdi * ad)
{
    if (ad->Credential) {
        jstring_trim(ad->Credential);
        JXTA_OBJECT_SHARE(ad->Credential);
    }
    return ad->Credential;
}

JXTA_DECLARE(void) jxta_resolver_srdi_set_credential(ResolverSrdi * ad, JString * credential)
{

    if (ad == NULL) {
        return;
    }
    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
        ad->Credential = NULL;
    }
    JXTA_OBJECT_SHARE(credential);
    ad->Credential = credential;
}

JXTA_DECLARE(void) jxta_resolver_srdi_get_handlername(ResolverSrdi * ad, JString ** str)
{
    if (ad->HandlerName) {
        jstring_trim(ad->HandlerName);
        JXTA_OBJECT_SHARE(ad->HandlerName);
    }
    *str = ad->HandlerName;
}

JXTA_DECLARE(void) jxta_resolver_srdi_set_handlername(ResolverSrdi * ad, JString * handlerName)
{
    if (ad == NULL) {
        return;
    }
    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
        ad->HandlerName = NULL;
    }
    JXTA_OBJECT_SHARE(handlerName);
    ad->HandlerName = handlerName;
}

JXTA_DECLARE(void) jxta_resolver_srdi_get_payload(ResolverSrdi * ad, JString ** payload)
{

    if (ad->Payload) {
        jstring_trim(ad->Payload);
        JXTA_OBJECT_SHARE(ad->Payload);
    }
    *payload = ad->Payload;
}

JXTA_DECLARE(void) jxta_resolver_srdi_set_payload(ResolverSrdi * ad, JString * payload)
{

    if (ad == NULL) {
        return;
    }
    if (ad->Payload != NULL) {
        JXTA_OBJECT_RELEASE(ad->Payload);
        ad->Payload = NULL;
    }
    if (payload != NULL) {
        JXTA_OBJECT_SHARE(payload);
        ad->Payload = payload;
    }
}

/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab ResolverSrdi_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ResolverSRDI", ResolverSrdi_, *handleResolverSrdi, NULL, NULL},
    {"jxta:Cred", Credential_, *handleCredential, NULL, NULL},
    {"HandlerName", HandlerName_, *handleHandlerName, NULL, NULL},
    {"Payload", Payload_, *handlePayload, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/** Get a new instance of the ad.
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(ResolverSrdi *) jxta_resolver_srdi_new(void)
{

    ResolverSrdi *ad;
    ad = (ResolverSrdi *) malloc(sizeof(ResolverSrdi));
    memset(ad, 0xda, sizeof(ResolverSrdi));
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:ResolverSRDI",
                                  ResolverSrdi_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_resolver_srdi_get_xml,
                                  NULL, (JxtaAdvertisementGetIndexFunc) NULL, (FreeFunc) jxta_resolver_srdi_free);

    /* Fill in the required initialization code here. */
    ad->Credential = jstring_new_0();
    ad->HandlerName = jstring_new_0();
    ad->Payload = jstring_new_0();
    return ad;
}


JXTA_DECLARE(ResolverSrdi *) jxta_resolver_srdi_new_1(JString * handlername, JString * payload, Jxta_id * src_pid)
{

    ResolverSrdi *ad;

    ad = (ResolverSrdi *) malloc(sizeof(ResolverSrdi));
    memset(ad, 0xda, sizeof(ResolverSrdi));
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:ResolverSRDI",
                                  ResolverSrdi_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_resolver_srdi_get_xml,
                                  NULL, NULL, (FreeFunc) jxta_resolver_srdi_free);

    /* Fill in the required initialization code here. */
    ad->Credential = jstring_new_0();
    ad->Payload = payload;
    JXTA_OBJECT_SHARE(ad->Payload);
    ad->HandlerName = jstring_new_2(jstring_get_string(handlername));
    return ad;
}


/** 
 * free all objects and memory occupied by this object
 */
void jxta_resolver_srdi_free(ResolverSrdi * ad)
{

    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
    }
    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
    }
    if (ad->Payload) {
        JXTA_OBJECT_RELEASE(ad->Payload);
    }
    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(ResolverSrdi));
    free(ad);
}


JXTA_DECLARE(void) jxta_resolver_srdi_parse_charbuffer(ResolverSrdi * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_resolver_srdi_parse_file(ResolverSrdi * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
