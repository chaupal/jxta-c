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
 * $Id: jxta_srdi.c,v 1.9 2005/02/02 02:58:33 exocetrick Exp $
 */

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_srdi.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

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
    Jxta_SRDIMessage_,
    TTL_,
    PeerID_,
    PrimaryKey_,
    Entry_
};


/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _Jxta_SRDIMessage {
    Jxta_advertisement jxta_advertisement;
    int TTL;
    Jxta_id *PeerID;
    JString *PrimaryKey;
    Jxta_vector *Entries;
};

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_SRDIMessage(void *userdata, const XML_Char * cd, int len)
{
    JXTA_LOG("In Jxta_SRDIMessage element\n");
}

static void handleTTL(void *userdata, const XML_Char * cd, int len)
{

    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    char *tok;
    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->TTL = (short) strtol(cd, NULL, 0);
            JXTA_LOG("Type is :%d\n", ad->TTL);
        }
        free(tok);
    }
    JXTA_LOG("In Type element\n");
}

static void handlePeerID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    char *tok;
    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            JString *tmps = jstring_new_2(tok);
            ad->PeerID = NULL;
            jxta_id_from_jstring(&ad->PeerID, tmps);
            JXTA_OBJECT_RELEASE(tmps);
        }
        free(tok);
    }
}

static void handlePrimaryKey(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    jstring_append_0((JString *) ad->PrimaryKey, cd, len);
}

static void DRE_Free(Jxta_object * o)
{
    Jxta_SRDIEntryElement *dre = (Jxta_SRDIEntryElement *) o;
    if (dre->key) {
        JXTA_OBJECT_RELEASE(dre->key);
    }
    if (dre->value) {
        JXTA_OBJECT_RELEASE(dre->value);
    }
    free((void *) o);
}

static void handleEntry(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    JString *value;
    Jxta_SRDIEntryElement *entry;
    const char **atts = ((Jxta_advertisement *) ad)->atts;
    entry = (Jxta_SRDIEntryElement *) malloc(sizeof(Jxta_SRDIEntryElement));
    JXTA_OBJECT_INIT(entry, DRE_Free, 0);

    if (len == 0) {
        return;
    }

    /*
     * This tag can only be handled properly with the JXTA_ADV_PAIRED_CALLS
     * option. We depend on it. There must be exactly one non-empty call
     * per address.
     */
    if (atts != NULL) {
        int i;
        for (i = 0; i < sizeof(atts); i++) {
            if (!strncmp("SKey", atts[i], strlen("SKey"))) {
                entry->key = jstring_new_1(strlen(atts[i + 1]));
                jstring_append_0(entry->key, atts[i + 1], strlen(atts[i + 1]));
                /* next attribute */
                i++;
            } else if (!strncmp("Expiration", atts[i], strlen("Expiration"))) {
                entry->expiration = atol(atts[i + 1]);
                /* next attribute */
                i++;
            }
        }
    }

    value = jstring_new_1(len);
    jstring_append_0(value, cd, len);
    jstring_trim(value);

    if (jstring_length(value) == 0) {
        JXTA_OBJECT_RELEASE(value);
        return;
    }

    entry->value = value;
    jxta_vector_add_object_last(ad->Entries, (Jxta_object *) entry);
    JXTA_OBJECT_RELEASE(entry);

    JXTA_LOG("In Srdi Entry element\n");
}


int jxta_srdi_message_get_ttl(Jxta_SRDIMessage * ad)
{
    return ad->TTL;
}


Jxta_status jxta_srdi_message_decrement_ttl(Jxta_SRDIMessage * ad)
{
    if (ad->TTL > 0) {
        ad->TTL--;
        return JXTA_SUCCESS;
    }
    return JXTA_FAILED;
}


Jxta_status jxta_srdi_message_get_peerID(Jxta_SRDIMessage * ad, Jxta_id ** peerid)
{
    if (ad->PeerID) {
        JXTA_OBJECT_SHARE(ad->PeerID);
    }
    *peerid = ad->PeerID;
    return JXTA_SUCCESS;
}

Jxta_status jxta_srdi_message_set_peerID(Jxta_SRDIMessage * ad, Jxta_id * peerid)
{
    if (ad == NULL || peerid == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->PeerID != NULL) {
        JXTA_OBJECT_RELEASE(ad->PeerID);
        ad->PeerID = NULL;
    }
    JXTA_OBJECT_SHARE(peerid);
    ad->PeerID = peerid;
    return JXTA_SUCCESS;
}

Jxta_status jxta_srdi_message_get_primaryKey(Jxta_SRDIMessage * ad, JString ** primaryKey)
{
    jstring_trim(ad->PrimaryKey);
    JXTA_OBJECT_SHARE(ad->PrimaryKey);
    *primaryKey = ad->PrimaryKey;
    return JXTA_SUCCESS;
}

Jxta_status jxta_srdi_message_set_primaryKey(Jxta_SRDIMessage * ad, JString * primaryKey)
{
    if (ad == NULL || primaryKey == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->PrimaryKey) {
        JXTA_OBJECT_RELEASE(ad->PrimaryKey);
        ad->PrimaryKey = NULL;
    }
    JXTA_OBJECT_SHARE(primaryKey);
    ad->PrimaryKey = primaryKey;
    return JXTA_SUCCESS;
}

Jxta_status jxta_srdi_message_get_entries(Jxta_SRDIMessage * ad, Jxta_vector ** entries)
{
    if (ad->Entries != NULL) {
        JXTA_OBJECT_SHARE(ad->Entries);
        *entries = ad->Entries;
    } else {
        *entries = NULL;
    }
    return JXTA_SUCCESS;
}

void jxta_srdi_message_set_entries(Jxta_SRDIMessage * ad, Jxta_vector * entries)
{
    if (ad->Entries != NULL) {
        JXTA_OBJECT_RELEASE(ad->Entries);
        ad->Entries = NULL;
    }
    if (entries != NULL) {
        JXTA_OBJECT_SHARE(entries);
        ad->Entries = entries;
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
static const Kwdtab Jxta_SRDIMessage_tags[] = {
    {"Null", Null_, NULL, NULL},
    {"jxta:GenSRDI", Jxta_SRDIMessage_, *handleJxta_SRDIMessage, NULL},
    {"PID", PeerID_, *handlePeerID, NULL},
    {"ttl", TTL_, *handleTTL, NULL},
    {"PKey", PrimaryKey_, *handlePrimaryKey, NULL},
    {"Entry", Entry_, *handleEntry, NULL},
    {NULL, 0, 0, NULL}
};

Jxta_status srdi_message_print(Jxta_SRDIMessage * ad, JString * js)
{
    char buf[128];
    Jxta_vector *entries;
    int eachElement;

    jxta_srdi_message_get_entries(ad, &entries);

    for (eachElement = jxta_vector_size(entries) - 1; eachElement >= 0; eachElement--) {
        Jxta_SRDIEntryElement *anElement = NULL;
        jxta_vector_get_object_at(entries, (Jxta_object **) & anElement, eachElement);
        if (NULL == anElement) {
            continue;
        }
        JXTA_OBJECT_CHECK_VALID(anElement);
#ifndef WIN32
        sprintf(buf, "<Entry SKey=\"%s\" Expiration=\"%lld\">\n", jstring_get_string(anElement->key), anElement->expiration);
#else
        sprintf(buf, "<Entry SKey=\"%s\" Expiration=\"%I64d\">\n", jstring_get_string(anElement->key), anElement->expiration);
#endif
        jstring_append_2(js, buf);
        jstring_append_1(js, anElement->value);
        jstring_append_2(js, "</Entry>\n");
        JXTA_OBJECT_RELEASE(anElement);
    }
    return JXTA_SUCCESS;
}

Jxta_status jxta_srdi_message_get_xml(Jxta_SRDIMessage * ad, JString ** document)
{

    JString *doc;
    JString *tmps = NULL;
    char *buf = malloc(128);
    memset(buf, 0, 128);

    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:GenSRDI>\n");
    jstring_append_2(doc, "<jxta:GenSRDI>\n");

    jstring_append_2(doc, "<ttl>");
    sprintf(buf, "%d", ad->TTL);
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</ttl>\n");

    jstring_append_2(doc, "<PID>");
    jxta_id_to_jstring(ad->PeerID, &tmps);
    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</PID>\n");

    jstring_append_2(doc, "<PKey>");
    jstring_append_2(doc, jstring_get_string(ad->PrimaryKey));
    jstring_append_2(doc, "</PKey>\n");

    srdi_message_print(ad, doc);
    jstring_append_2(doc, "</jxta:GenSRDI>\n");
    free(buf);
    *document = doc;
    return JXTA_SUCCESS;
}

Jxta_SRDIMessage *jxta_srdi_message_new(void)
{

    Jxta_SRDIMessage *ad;
    ad = (Jxta_SRDIMessage *) malloc(sizeof(Jxta_SRDIMessage));
    memset(ad, 0x0, sizeof(Jxta_SRDIMessage));
    ad->PeerID = jxta_id_nullID;
    ad->TTL = 0;
    ad->Entries = jxta_vector_new(4);
    ad->PrimaryKey = jstring_new_0();
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:GenSRDI",
                                  Jxta_SRDIMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_srdi_message_get_xml,
                                  NULL,NULL,
				  (FreeFunc) jxta_srdi_message_free);
    return ad;
}

Jxta_SRDIMessage *jxta_srdi_message_new_1(int ttl, Jxta_id * peerid, char *primarykey, Jxta_vector * entries)
{

    Jxta_SRDIMessage *ad;
    ad = (Jxta_SRDIMessage *) malloc(sizeof(Jxta_SRDIMessage));
    memset(ad, 0x0, sizeof(Jxta_SRDIMessage));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:GenSRDI",
                                  Jxta_SRDIMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_srdi_message_get_xml,
                                  NULL, NULL, (FreeFunc) jxta_srdi_message_free);

    JXTA_OBJECT_SHARE(peerid);
    JXTA_OBJECT_SHARE(entries);

    ad->PeerID = peerid;
    ad->Entries = entries;
    ad->TTL = ttl;
    ad->PrimaryKey = jstring_new_2(primarykey);
    return ad;
}

void jxta_srdi_message_free(Jxta_SRDIMessage * ad)
{
    if (ad->PeerID) {
        JXTA_OBJECT_RELEASE(ad->PeerID);
    }
    if (ad->PrimaryKey) {
        JXTA_OBJECT_RELEASE(ad->PrimaryKey);
    }
    if (ad->Entries) {
        JXTA_OBJECT_RELEASE(ad->Entries);
    }

    jxta_advertisement_delete((Jxta_advertisement *) ad);

    memset(ad, 0x0, sizeof(Jxta_SRDIMessage));
    free(ad);
}

void jxta_srdi_message_parse_charbuffer(Jxta_SRDIMessage * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

void jxta_srdi_message_parse_file(Jxta_SRDIMessage * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

static void entry_element_free(Jxta_object * o)
{
    Jxta_SRDIEntryElement *dse = (Jxta_SRDIEntryElement *) o;
    if (dse->key != NULL) {
        JXTA_OBJECT_RELEASE(dse->key);
    }
    if (dse->value != NULL) {
        JXTA_OBJECT_RELEASE(dse->value);
    }
    free((void *) o);
}

Jxta_SRDIEntryElement *jxta_srdi_new_element(void)
{

    Jxta_SRDIEntryElement *dse = (Jxta_SRDIEntryElement *) malloc(sizeof(Jxta_SRDIEntryElement));
    memset(dse, 0x0, sizeof(Jxta_SRDIEntryElement));
    JXTA_OBJECT_INIT(dse, entry_element_free, 0);
    return dse;
}

Jxta_SRDIEntryElement *jxta_srdi_new_element_1(JString * key, JString * value, Jxta_expiration_time expiration)
{

    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();


    if (dse->key != NULL)
        JXTA_OBJECT_RELEASE(dse->key);
    if (dse->value != NULL)
        JXTA_OBJECT_RELEASE(dse->value);

    dse->key = key;
    dse->value = value;
    JXTA_OBJECT_SHARE(dse->key);
    JXTA_OBJECT_SHARE(dse->value);
    dse->expiration = expiration;
    return dse;
}

#ifdef __cplusplus
}
#endif
