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
 
static const char *__log_cat = "SRDI";

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_srdi.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the
 * xml ad.
 */ 
enum tokentype {
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
    Jxta_boolean deltaSupport;
    Jxta_vector *Entries;
    Jxta_vector *resendEntries;
};

static void jxta_srdi_message_free(Jxta_SRDIMessage *);


/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_SRDIMessage(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *myself = (Jxta_SRDIMessage *) userdata;
    Jxta_SRDIEntryElement *entry;
    unsigned int i;
    Jxta_status rv;
    const char *pk;

    if (0 == len) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Begin Jxta_SRDIMessage element\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "End Jxta_SRDIMessage element\n");

        /* creat default mapping for namespace as JSE peer does not support namespace */
        for (i = 0; i < jxta_vector_size(myself->Entries); i++) {
            rv = jxta_vector_get_object_at(myself->Entries, JXTA_OBJECT_PPTR(&entry), i);
            if (JXTA_SUCCESS != rv) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Cannot go through SRDI entry at %d\n", i);
                break;
            }
            if (entry->nameSpace) {
                JXTA_OBJECT_RELEASE(entry);
                continue;
            }
            
            pk = jstring_get_string(myself->PrimaryKey);
            if (!strcmp(pk, "Peers")) {
                entry->nameSpace = jstring_new_2("jxta:PA");
            } else if (!strcmp(pk, "Groups")) {
                entry->nameSpace = jstring_new_2("jxta:PGA");
            } else {
                entry->nameSpace = jstring_new_2("jxta:ADV");
            }
            JXTA_OBJECT_RELEASE(entry);
        }
    }
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
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Type is :%d\n", ad->TTL);
        }
        free(tok);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Type element\n");
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
    jstring_trim(ad->PrimaryKey);
}

static void handleDelta(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    ad->deltaSupport = TRUE;
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
    if (dre->nameSpace) {
        JXTA_OBJECT_RELEASE(dre->nameSpace);
    }
    if (dre->advId) {
        JXTA_OBJECT_RELEASE(dre->advId);
    }
    if (dre->range) {
        JXTA_OBJECT_RELEASE(dre->range);
    }
    if (dre->sn_cs_values) {
        JXTA_OBJECT_RELEASE(dre->sn_cs_values);
    }
    free((void *) o);
}

static void normalize_entry(Jxta_SRDIEntryElement *entry)
{
    if (!entry->seqNumber) {
        if (!entry->value) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Received a SRDI entry without value\n");
            entry->value = jstring_new_2("");
        }
        if (!entry->advId) {
            JXTA_OBJECT_SHARE(entry->value);
            entry->advId = entry->value;
        }
    }
}

static void handleEntry(void *userdata, const XML_Char * cd, int len)
{
    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) userdata;
    JString *value = NULL;
    Jxta_SRDIEntryElement *entry;
    const char **atts = ((Jxta_advertisement *) ad)->atts;

    if (len == 0) {
        return;
    }

    entry = (Jxta_SRDIEntryElement *) calloc(1, sizeof(Jxta_SRDIEntryElement));

    JXTA_OBJECT_INIT(entry, DRE_Free, 0);

    /*
     * This tag can only be handled properly with the JXTA_ADV_PAIRED_CALLS
     * option. We depend on it. There must be exactly one non-empty call
     * per address.
     */
     entry->replicate = TRUE;
    if (atts != NULL) {
        while (*atts) {
            if (0 == strcmp("SKey", *atts)) {
                entry->key = jstring_new_1(strlen(atts[1]));
                jstring_append_0(entry->key, atts[1], strlen(atts[1]));
            } else if (0 == strcmp("Expiration", *atts)) {
                entry->expiration = apr_atoi64(atts[1]);
            } else if (0 == strcmp("nSpace", *atts)) {
                entry->nameSpace = jstring_new_1(strlen(atts[1]));
                jstring_append_0(entry->nameSpace, atts[1], strlen(atts[1]));
            } else if (0 == strcmp("sNs_csv", *atts)) {
                entry->sn_cs_values = jstring_new_1(strlen(atts[1]));
                jstring_append_0(entry->sn_cs_values, atts[1], strlen(atts[1]));
            } else if (0 == strcmp("sN", *atts)) {
                entry->seqNumber = apr_atoi64(atts[1]);
            } else if (0 == strcmp("resend", *atts)) {
                entry->resend = TRUE;
            } else if (0 == strcmp("AdvId", *atts)) {
                if (NULL == entry->advId) {
                    entry->advId = jstring_new_1(strlen(atts[1]));
                } else {
                    jstring_reset(entry->advId, NULL);
                }
                jstring_append_0(entry->advId, atts[1], strlen(atts[1]));
            } else if (0 == strcmp("Range", *atts)) {
                if (NULL == entry->range) {
                    entry->range = jstring_new_1(strlen(atts[1]));
                } else {
                    jstring_reset(entry->range, NULL);
                }
                jstring_append_0(entry->range, atts[1], strlen(atts[1]));
            } else if (0 == strcmp("replicate", *atts)) {
                entry->replicate = FALSE;
            }
            
            atts += 2;
        }
    }
    if (len > 0) {
        value = jstring_new_1(len);
        jstring_append_0(value, cd, len);
        jstring_trim(value);

        if (jstring_length(value) == 0) {
            JXTA_OBJECT_RELEASE(value);
            value = NULL;
        }
    }
    entry->value = value;
    if (entry->resend) {
        jxta_vector_add_object_last(ad->resendEntries, (Jxta_object *) entry);
    } else {
        normalize_entry(entry);
        jxta_vector_add_object_last(ad->Entries, (Jxta_object *) entry);
    }
    JXTA_OBJECT_RELEASE(entry);
}

JXTA_DECLARE(int) jxta_srdi_message_get_ttl(Jxta_SRDIMessage * ad)
{
    return ad->TTL;
}


JXTA_DECLARE(Jxta_status) jxta_srdi_message_decrement_ttl(Jxta_SRDIMessage * ad)
{
    if (ad->TTL > 0) {
        ad->TTL--;
        return JXTA_SUCCESS;
    }
    return JXTA_FAILED;
}


JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_peerID(Jxta_SRDIMessage * ad, Jxta_id ** peerid)
{
    if (ad->PeerID) {
        JXTA_OBJECT_SHARE(ad->PeerID);
    }
    *peerid = ad->PeerID;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_set_peerID(Jxta_SRDIMessage * ad, Jxta_id * peerid)
{
    if (ad == NULL || peerid == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (ad->PeerID != NULL) {
        JXTA_OBJECT_RELEASE(ad->PeerID);
        ad->PeerID = NULL;
    }
    ad->PeerID = JXTA_OBJECT_SHARE(peerid);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_primaryKey(Jxta_SRDIMessage * ad, JString ** primaryKey)
{
    JXTA_OBJECT_SHARE(ad->PrimaryKey);
    *primaryKey = ad->PrimaryKey;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_set_primaryKey(Jxta_SRDIMessage * ad, JString * primaryKey)
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
    jstring_trim(ad->PrimaryKey);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_srdi_message_delta_supported(Jxta_SRDIMessage * ad)
{
    return ad->deltaSupport;
}

JXTA_DECLARE(void) jxta_srdi_message_set_support_delta(Jxta_SRDIMessage * ad, Jxta_boolean support)
{
    ad->deltaSupport = support;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_entries(Jxta_SRDIMessage * ad, Jxta_vector ** entries)
{
    if (ad->Entries != NULL) {
        JXTA_OBJECT_SHARE(ad->Entries);
        *entries = ad->Entries;
    } else {
        *entries = NULL;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_resend_entries(Jxta_SRDIMessage * ad, Jxta_vector ** entries)
{
    if (ad->resendEntries) {
        *entries = JXTA_OBJECT_SHARE(ad->resendEntries);
    } else {
        *entries = NULL;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_srdi_message_set_resend_entries(Jxta_SRDIMessage * ad, Jxta_vector * entries)
{
    if (ad->resendEntries) {
        JXTA_OBJECT_RELEASE(ad->resendEntries);
        ad->resendEntries = NULL;
    }
    if (entries != NULL) {
        JXTA_OBJECT_SHARE(entries);
        ad->resendEntries = entries;
    }
    return;
}

JXTA_DECLARE(void) jxta_srdi_message_set_entries(Jxta_SRDIMessage * ad, Jxta_vector * entries)
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
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:GenSRDI", Jxta_SRDIMessage_, *handleJxta_SRDIMessage, NULL, NULL},
    {"PID", PeerID_, *handlePeerID, NULL, NULL},
    {"ttl", TTL_, *handleTTL, NULL, NULL},
    {"PKey", PrimaryKey_, *handlePrimaryKey, NULL, NULL},
    {"delta", PrimaryKey_, *handleDelta, NULL, NULL},
    {"Entry", Entry_, *handleEntry, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

Jxta_status srdi_message_print(Jxta_SRDIMessage * ad, JString * js)
{
    char *buf = calloc( sizeof(char), 512);
    char tmpbuf[32];
    Jxta_vector *entries;
    unsigned int eachElement = 0;
    Jxta_SRDIEntryElement *anElement;
    jxta_srdi_message_get_entries(ad, &entries);
    for (eachElement = 0; entries != NULL && eachElement < jxta_vector_size(entries); eachElement++) {
        anElement = NULL;
        jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&anElement), eachElement);
        if (NULL == anElement) {
            continue;
        }
        jstring_append_2(js, "<Entry ");
        apr_snprintf(buf, 512, " Expiration=\"%" APR_INT64_T_FMT "\"",
                      anElement->expiration);
        jstring_append_2(js, buf);

        if (anElement->resend) {
            jstring_append_2(js, " resend=\"yes\"");
        }
        if (NULL != anElement->key) {
            jstring_append_2(js, " SKey=\"");
            jstring_append_1(js, anElement->key);
            jstring_append_2(js, "\"");
        }
        if (NULL != anElement->nameSpace) {
            jstring_append_2(js, " nSpace=\"");
            jstring_append_1(js, anElement->nameSpace);
            jstring_append_2(js, "\"");
        }

        if (NULL != anElement->advId) {
            jstring_append_2(js, " AdvId=\"");
            jstring_append_1(js, anElement->advId);
            jstring_append_2(js, "\"");
        }
        if (NULL != anElement->sn_cs_values) {
            jstring_append_2(js, " sNs_csv=\"");
            jstring_append_1(js, anElement->sn_cs_values);
            jstring_append_2(js, "\"");
        }
        if (NULL != anElement->range) {
            jstring_append_2(js, " Range=\"");
            jstring_append_1(js, anElement->range);
            jstring_append_2(js, "\"");
        }
        if (anElement->seqNumber > 0) {
            jstring_append_2(js, " sN=\"");
            apr_snprintf(tmpbuf, sizeof(tmpbuf), JXTA_SEQUENCE_NUMBER_FMT , anElement->seqNumber);
            jstring_append_2(js, tmpbuf);
            jstring_append_2(js, "\"");
        }

        if (anElement->replicate == FALSE) {
            jstring_append_2(js, " replicate=\"FALSE\"");
        }
        
        jstring_append_2(js, ">\n");
        if (NULL != anElement->value) {
            jstring_append_1(js, anElement->value);
        }
        jstring_append_2(js, "</Entry>\n");
        JXTA_OBJECT_RELEASE(anElement);
    }

    free(buf);

    if (entries)
        JXTA_OBJECT_RELEASE(entries);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_get_xml(Jxta_SRDIMessage * ad, JString ** document)
{
    JString *doc;
    JString *tmps = NULL;
    char buf[32];

    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:GenSRDI>\n");
    jstring_append_2(doc, "<jxta:GenSRDI>\n");

    jstring_append_2(doc, "<ttl>");
    apr_snprintf(buf, sizeof(buf), "%d", ad->TTL);
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

    if (ad->deltaSupport) {
        jstring_append_2(doc, "<delta />\n");
    }
    srdi_message_print(ad, doc);

    jstring_append_2(doc, "</jxta:GenSRDI>\n");

    *document = doc;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new(void)
{

    Jxta_SRDIMessage *ad = (Jxta_SRDIMessage *) calloc(1, sizeof(Jxta_SRDIMessage));

    ad->PeerID = jxta_id_nullID;
    ad->TTL = 0;
    ad->Entries = jxta_vector_new(4);
    ad->resendEntries = jxta_vector_new(0);
    ad->PrimaryKey = jstring_new_0();
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:GenSRDI",
                                  Jxta_SRDIMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_srdi_message_get_xml,
                                  NULL, NULL, (FreeFunc) jxta_srdi_message_free);
    return ad;
}

JXTA_DECLARE(Jxta_SRDIMessage *) jxta_srdi_message_new_1(int ttl, Jxta_id * peerid, char *primarykey, Jxta_vector * entries)
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
    jstring_trim(ad->PrimaryKey);
    return ad;
}

static void jxta_srdi_message_free(Jxta_SRDIMessage * ad)
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
    if (ad->resendEntries) {
        JXTA_OBJECT_RELEASE(ad->resendEntries);
    }
    jxta_advertisement_delete((Jxta_advertisement *) ad);

    free(ad);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_parse_charbuffer(Jxta_SRDIMessage * ad, const char *buf, int len)
{

    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(Jxta_status) jxta_srdi_message_parse_file(Jxta_SRDIMessage * ad, FILE * stream)
{

    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
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
    if (dse->nameSpace != NULL) {
        JXTA_OBJECT_RELEASE(dse->nameSpace);
    }
    if (dse->range != NULL) {
        JXTA_OBJECT_RELEASE(dse->range);
    }
    if (dse->advId != NULL) {
        JXTA_OBJECT_RELEASE(dse->advId);
    }
    if (dse->sn_cs_values != NULL) {
        JXTA_OBJECT_RELEASE(dse->sn_cs_values);
    }
    free((void *) o);
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element(void)
{
    Jxta_SRDIEntryElement *dse = (Jxta_SRDIEntryElement *) calloc(1, sizeof(Jxta_SRDIEntryElement));
    JXTA_OBJECT_INIT(dse, entry_element_free, 0);
    return dse;
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_element_clone(Jxta_SRDIEntryElement * entry)
{
    Jxta_SRDIEntryElement *newEntry=NULL;
    newEntry = jxta_srdi_new_element();

    assert(!entry->resend);
    newEntry->resend = FALSE;
    newEntry->replicate = entry->replicate;
    newEntry->seqNumber = entry->seqNumber;
    newEntry->expiration = entry->expiration;
    newEntry->next_update_time = entry->next_update_time;

    if (newEntry->seqNumber > 0) {
        return newEntry;
    }

    /* those element should have appropriate value given it is not an update*/
    newEntry->nameSpace = jstring_clone(entry->nameSpace);
    newEntry->advId = jstring_clone(entry->advId);
    newEntry->key = jstring_clone(entry->key);
    newEntry->value = jstring_clone(entry->value);

    if (entry->range) {
        newEntry->range = jstring_clone(entry->range);
    }
    return newEntry;
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_1(JString * key, JString * value, JString * nameSpace,
                                                              Jxta_expiration_time expiration)
{
    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();

    dse->key = JXTA_OBJECT_SHARE(key);
    dse->value = JXTA_OBJECT_SHARE(value);
    dse->nameSpace = JXTA_OBJECT_SHARE(nameSpace);
    dse->advId = JXTA_OBJECT_SHARE(value);
    dse->expiration = expiration;
    dse->seqNumber = 0;
    dse->resend = FALSE;
    dse->next_update_time = 0;
    dse->replicate = TRUE;
    return dse;
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_2(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * jrange, Jxta_expiration_time expiration)
{
    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();

    dse->key = JXTA_OBJECT_SHARE(key);
    dse->value = JXTA_OBJECT_SHARE(value);
    dse->nameSpace = JXTA_OBJECT_SHARE(nameSpace);
    if (advId) {
        dse->advId = JXTA_OBJECT_SHARE(advId);
    } else {
        dse->advId = JXTA_OBJECT_SHARE(value);
    }
    if (jrange) {
        dse->range = JXTA_OBJECT_SHARE(jrange);
    }
    dse->seqNumber = 0;
    dse->expiration = expiration;
    dse->resend = FALSE;
    dse->next_update_time = 0;
    dse->replicate = TRUE;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "This is the spot in number 2 %s \n", jstring_get_string(dse->key));
    return dse;
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_3(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * jrange, Jxta_expiration_time expiration,
                                                              Jxta_sequence_number seqNumber)
{
    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();

    dse->key = JXTA_OBJECT_SHARE(key);
    dse->value = JXTA_OBJECT_SHARE(value);
    dse->nameSpace = JXTA_OBJECT_SHARE(nameSpace);
    if (advId) {
        dse->advId = JXTA_OBJECT_SHARE(advId);
    } else {
        dse->advId = JXTA_OBJECT_SHARE(value);
    }
    if (jrange) {
        dse->range = JXTA_OBJECT_SHARE(jrange);
    }
    dse->seqNumber = seqNumber;
    dse->expiration = expiration;
    dse->resend = FALSE;
    dse->next_update_time = 0 ;
    dse->replicate = TRUE;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "This is the spot in number 3 %s \n", jstring_get_string(dse->key));
    return dse;
}


JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_4(JString * key, JString * value, JString * nameSpace,
                                                              JString * advId, JString * jrange, Jxta_expiration_time expiration,
                                                              Jxta_sequence_number seqNumber, Jxta_boolean replicate)
{
    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();

    dse->key = JXTA_OBJECT_SHARE(key);
    dse->value = JXTA_OBJECT_SHARE(value);
    dse->nameSpace = JXTA_OBJECT_SHARE(nameSpace);
    if (advId) {
     dse->advId = JXTA_OBJECT_SHARE(advId);
    } else {
     dse->advId = JXTA_OBJECT_SHARE(value);
    }
    if (jrange) {
     dse->range = JXTA_OBJECT_SHARE(jrange);
    }
    dse->seqNumber = seqNumber;
    dse->expiration = expiration;
    dse->resend = FALSE;
    dse->next_update_time = 0;
    dse->replicate = replicate;
    return dse;
}

JXTA_DECLARE(Jxta_SRDIEntryElement *) jxta_srdi_new_element_resend(Jxta_sequence_number seqNumber)
{
    Jxta_SRDIEntryElement *dse = jxta_srdi_new_element();

    dse->key = NULL;
    dse->value = NULL;
    dse->nameSpace = NULL;
    dse->seqNumber = seqNumber;
    dse->expiration = 0;
    dse->resend = TRUE;
    dse->next_update_time = 0;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "This is the spot in number 4\n");
    return dse;
}

/* vi: set ts=4 sw=4 tw=130 et: */
