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
 *    permission, ppa contact Project JXTA at http://www.jxta.org.
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
 * information on Project JXTA, ppa see
 * <http://www.jxta.org/>.
 *
 * This license is based on the BSD license adopted by the Apache Foundation.
 *
 * $Id$
 */


static const char *__log_cat = "AdvResponse";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_adv_response_msg.h"

const char JXTA_ADV_RESPONSE_ELEMENT_NAME[] = "AdvResponseMsg";

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    AdvResponseMsg_,
    credential_,
    peer_
};

/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _Jxta_adv_response_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_credential * credential;
    Jxta_PA *client_adv;
    Jxta_vector *entries;
};

struct _jxta_adv_response_entry {
    JXTA_OBJECT_HANDLE;
    Jxta_advertisement  * adv;
    Jxta_expiration_time expiration;
}  ;

static void adv_response_msg_delete(Jxta_object * me);
static Jxta_adv_response_msg *adv_response_msg_construct(Jxta_adv_response_msg * myself);
static Jxta_status validate_message(Jxta_adv_response_msg * myself);
static void handle_adv_response_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_adv(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_adv_response_msg *) jxta_adv_response_msg_new(void)
{
    Jxta_adv_response_msg *myself = (Jxta_adv_response_msg *) calloc(1, sizeof(Jxta_adv_response_msg));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, adv_response_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "AdvResponse NEW [%pp]\n", myself );

    return adv_response_msg_construct(myself);
}

static void adv_response_msg_delete(Jxta_object * me)
{
    Jxta_adv_response_msg *myself = (Jxta_adv_response_msg *) me;

    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (NULL != myself->entries) {
        JXTA_OBJECT_RELEASE(myself->entries);
        myself->entries = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_adv_response_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "AdvResponse FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab adv_response_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:AdvResponse", AdvResponseMsg_, *handle_adv_response_msg, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {"Adv", peer_, *handle_adv, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_adv_response_msg *adv_response_msg_construct(Jxta_adv_response_msg * myself)
{
    myself = (Jxta_adv_response_msg *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:AdvResponse",
                                     adv_response_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_adv_response_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->credential = NULL;
        myself->entries = jxta_vector_new(0);
    }

    return myself;
}

static Jxta_status validate_message(Jxta_adv_response_msg * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

#if 0 
    /* FIXME 20060827 bondolo Credential processing not yet implemented. */
    if ( NULL != myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif
    if (NULL == myself->entries || jxta_vector_size(myself->entries) < 1) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "entries must not be NULL or empty [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_adv_response_msg_get_xml(Jxta_adv_response_msg * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    unsigned int i;
    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<jxta:AdvResponse>\n");

    for (i=0; i<jxta_vector_size(myself->entries); i++) {
        Jxta_advertisement *adv;
        Jxta_expiration_time exp;
        Jxta_adv_response_entry * entry;
        JString *xml;
        char buf[128];
        JString *encode_xml;
        if (JXTA_SUCCESS != jxta_vector_get_object_at(myself->entries, JXTA_OBJECT_PPTR(&entry), i)) {
            continue;
        }
        adv = jxta_adv_response_entry_get_adv(entry);
        exp = jxta_adv_response_entry_get_expiration(entry);
        jstring_append_2(string, "<Adv ");
        jstring_append_2(string, " Expiration=\"");
        apr_snprintf(buf, sizeof(buf), "%" APR_INT64_T_FMT "\"", entry->expiration);
        jstring_append_2(string, buf);
        jstring_append_2(string, ">");
        jxta_advertisement_get_xml(adv, &xml);
        if (NULL != xml) {
            jxta_xml_util_encode_jstring(xml, &encode_xml);
            jstring_append_1(string, encode_xml);
            JXTA_OBJECT_RELEASE(xml);
            JXTA_OBJECT_RELEASE(encode_xml);
        }
        jstring_append_2(string, "</Adv>");
        JXTA_OBJECT_RELEASE(adv);

    }
    jstring_append_2(string, "</jxta:AdvResponse>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

static void delete_entry(Jxta_object *obj)
{
    Jxta_adv_response_entry * me = (Jxta_adv_response_entry *) obj;
    if (me->adv)
        JXTA_OBJECT_RELEASE(me->adv);
    free(me);
}

static Jxta_adv_response_entry * new_entry(Jxta_advertisement *adv, Jxta_expiration_time exp)
{
    Jxta_adv_response_entry * entry;
    entry = calloc(1, sizeof(Jxta_adv_response_entry));
    JXTA_OBJECT_INIT(entry, delete_entry, NULL);

    entry->adv = JXTA_OBJECT_SHARE(adv);
    entry->expiration = exp;
    return entry;
}

JXTA_DECLARE(Jxta_status) jxta_adv_response_msg_add_advertisement(Jxta_adv_response_msg *msg, Jxta_advertisement *adv, Jxta_expiration_time exp)
{
    Jxta_status ret;
    Jxta_adv_response_entry *entry;

    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(pid);

    entry = new_entry(adv, exp);
    ret = jxta_vector_add_object_last(msg->entries, (Jxta_object *) entry);

    return ret;
}

JXTA_DECLARE(Jxta_status) jxta_adv_response_get_entries(Jxta_adv_response_msg *msg, Jxta_vector **entries)
{
    Jxta_status ret = JXTA_SUCCESS;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != msg->entries) {
        *entries = JXTA_OBJECT_SHARE(msg->entries);
    }
    return ret;
}

JXTA_DECLARE(Jxta_advertisement *) jxta_adv_response_entry_get_adv(Jxta_adv_response_entry *entry)
{
    Jxta_advertisement * ret = NULL;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != entry->adv) {
        ret = JXTA_OBJECT_SHARE(entry->adv);
    }
    return ret;
}

JXTA_DECLARE(Jxta_expiration_time) jxta_adv_response_entry_get_expiration(Jxta_adv_response_entry *entry)
{

    JXTA_OBJECT_CHECK_VALID(entry);

    return entry->expiration;
}

JXTA_DECLARE(Jxta_credential *) jxta_adv_response_msg_get_credential(Jxta_adv_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_adv_response_msg_set_credential(Jxta_adv_response_msg * myself, Jxta_credential *credential)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->credential != NULL) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (credential != NULL) {
        myself->credential = JXTA_OBJECT_SHARE(credential);
    }
}


/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_adv_response_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_response_msg *myself = (Jxta_adv_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:AdvResponse> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:AdvResponse> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_response_msg *myself = (Jxta_adv_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Credential> : [%pp]\n", myself);
#if 0
        jxta_advertisement_set_handlers((Jxta_advertisement *) server_adv, ((Jxta_advertisement *) myself)->parser, (void *) myself);

        ((Jxta_advertisement *) server_adv)->atts = ((Jxta_advertisement *) myself)->atts;
#endif
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Credential> : [%pp]\n", myself);
    }
}

static void handle_adv(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_response_msg *myself = (Jxta_adv_response_msg *) me;
    JXTA_OBJECT_CHECK_VALID(myself);
    Jxta_advertisement * adv = NULL;
    Jxta_status status = JXTA_SUCCESS;
    JString *decode_xml = NULL;

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Adv> : [%pp]\n", myself);
   } else {
        JString *tmps = NULL;
        Jxta_vector *res_vect=NULL;
        Jxta_adv_response_entry * entry; 
        Jxta_advertisement *res_adv;
        Jxta_expiration_time exp=0;
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "<Adv> : %s\n", cd);
        adv = jxta_advertisement_new();
        status = jxta_advertisement_parse_charbuffer(adv, cd, len);

        if (status != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Error parsing adv: %s\n", cd);
            goto FINAL_EXIT;
        }

        jxta_advertisement_get_advs(adv, &res_vect);
        if (NULL != res_vect && jxta_vector_size(res_vect) > 0) {
            jxta_vector_get_object_at(res_vect, JXTA_OBJECT_PPTR(&res_adv), (unsigned int) 0);
        }
        jxta_advertisement_get_xml((Jxta_advertisement *) res_adv, &tmps);
        if (tmps != NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Got a string after parsing: %s\n", jstring_get_string(tmps));
            JXTA_OBJECT_RELEASE(tmps);
        }

        if (atts != NULL) {
            if (!strncmp("Expiration", atts[0], sizeof("Expiration") - 1)) {
                exp = atol(atts[1]);
            }
        }
        entry = new_entry(res_adv, exp);
        jxta_vector_add_object_last(myself->entries, (Jxta_object *) entry);
        JXTA_OBJECT_RELEASE(res_adv);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Adv> : [%pp]\n", myself);
        if (res_vect)
            JXTA_OBJECT_RELEASE(res_vect);
        if (entry)
            JXTA_OBJECT_RELEASE(entry);
   }
FINAL_EXIT:
    if (decode_xml)
        JXTA_OBJECT_RELEASE(decode_xml);
    if (adv)
        JXTA_OBJECT_RELEASE(adv);
}
/* vim: set ts=4 sw=4 et tw=130: */
