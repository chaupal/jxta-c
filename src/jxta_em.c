/*
 * Copyright (c) 2009 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id:$
 */

static const char *__log_cat = "EP_MSG";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_cred.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_endpoint_address.h"
#include "jxta_em.h"


/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    EndpointMsg_,
    Credential_,
    EPEntry_,
    Option_
};

/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _jxta_endpoint_message {
    Jxta_advertisement jxta_advertisement;
    Jxta_vector *entries;
    Jxta_id *src_peer_id;
    Jxta_credential * credential;
    Jxta_id *dst_peer_id;
    Jxta_endpoint_address *dst_peer_ea;
    Jxta_vector * options;
    Jxta_endpoint_msg_entry_element *curr_elem;
    Msg_priority priority;
    Jxta_time timestamp;
};

struct _jxta_endpoint_msg_entry_element {
    JXTA_OBJECT_HANDLE;
    Jxta_endpoint_address *ea;
    Jxta_message *msg;
    Jxta_message_element *value;
    char *elem;
    char *mime;
    char *ns;
    char *name;
    Jxta_hashtable *attributes;
};

static void endpoint_msg_delete(Jxta_object * me);
static Jxta_endpoint_message *endpoint_msg_construct(Jxta_endpoint_message * myself);
static Jxta_status validate_message(Jxta_endpoint_message * myself);
static void handle_endpoint_msg(void *me, const XML_Char * cd, int len);

static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_ep_entry(void *me, const XML_Char * cd, int len);
/* static void handle_option(void *me, const XML_Char * cd, int len); */


JXTA_DECLARE(Jxta_endpoint_message *) jxta_endpoint_msg_new(void)
{
    Jxta_endpoint_message *myself = (Jxta_endpoint_message *) calloc(1, sizeof(Jxta_endpoint_message));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, endpoint_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint Message NEW [%pp]\n", myself );

    return endpoint_msg_construct(myself);
}

static void endpoint_msg_delete(Jxta_object * me)
{
    Jxta_endpoint_message *myself = (Jxta_endpoint_message *) me;

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    JXTA_OBJECT_RELEASE(myself->dst_peer_id);

    if (myself->entries) {
        JXTA_OBJECT_RELEASE(myself->entries);
        myself->entries = NULL;
    }

    if (NULL != myself->dst_peer_ea) {
        JXTA_OBJECT_RELEASE(myself->dst_peer_ea);
        myself->dst_peer_ea = NULL;
    }

    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (NULL != myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
        myself->options = NULL;
    }
    if (NULL != myself->curr_elem) {
        JXTA_OBJECT_RELEASE(myself->curr_elem);
        myself->curr_elem = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_endpoint_message));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint Message FREE [%pp]\n", myself );

    free(myself);
}

static void endpoint_msg_entry_delete(Jxta_object *me)
{
    Jxta_endpoint_msg_entry_element *myself = (Jxta_endpoint_msg_entry_element *) me;

    if (myself->value)
        JXTA_OBJECT_RELEASE(myself->value);
    if (myself->attributes)
        JXTA_OBJECT_RELEASE(myself->attributes);
    if (myself->msg)
        JXTA_OBJECT_RELEASE(myself->msg);
    if (myself->ea)
        JXTA_OBJECT_RELEASE(myself->ea);
    if (myself->elem)
        free(myself->elem);
    if (myself->mime)
        free(myself->mime);
    if (myself->ns)
        free(myself->ns);
    if (myself->name)
        free(myself->name);
    memset(myself, 0xdd, sizeof(Jxta_endpoint_msg_entry_element));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Endpoint Message Entry FREE [%pp]\n", myself );
    free(myself);

}

static const Kwdtab endpoint_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:EndpointMessage", EndpointMsg_, *handle_endpoint_msg, NULL, NULL},
    {"Credential", Credential_, *handle_credential, NULL, NULL},
    {"EPEntry", EPEntry_, *handle_ep_entry, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_endpoint_message *endpoint_msg_construct(Jxta_endpoint_message * myself)
{
    myself = (Jxta_endpoint_message *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:EndpointMessage",
                                     endpoint_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_endpoint_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, 
                                     (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->dst_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->dst_peer_ea = NULL;
        myself->credential = NULL;
        myself->entries = jxta_vector_new(0);
        myself->priority = MSG_EXPEDITED;
    }

    return myself;
}

static Jxta_status validate_message(Jxta_endpoint_message * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if (TRUE) return JXTA_SUCCESS;
    if ( jxta_id_equals(myself->src_peer_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Source peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ( jxta_id_equals(myself->dst_peer_id, jxta_id_nullID)) {
        if ( NULL == myself->dst_peer_ea ) { 
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Dest peer id and EA must not both be NULL [%pp]\n", myself);
            return JXTA_INVALID_ARGUMENT;
        }
    } else if ( NULL != myself->dst_peer_ea ) { 
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Dest peer id and EA must not both be defined. [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

#if 0 
    /* FIXME 20060827 bondolo Credential processing not yet implemented. */
    if ( NULL != myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_parse_charbuffer(Jxta_endpoint_message * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_parse_file(Jxta_endpoint_message * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_get_xml(Jxta_endpoint_message * myself
                                    , Jxta_boolean encode, JString ** xml, Jxta_boolean with_entries)
{
    Jxta_status res;
    JString *string;
    JString *tempstr;
    int i;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:EndpointMessage");

    jstring_append_2(string, " src_id=\"");
    jxta_id_to_jstring(myself->src_peer_id, &tempstr);
    jstring_append_1(string, tempstr);
    JXTA_OBJECT_RELEASE(tempstr);
    jstring_append_2(string, "\"");

    if( !jxta_id_equals(myself->dst_peer_id, jxta_id_nullID)) {
        jstring_append_2(string, " dst_id=\"");
        jxta_id_to_jstring(myself->dst_peer_id, &tempstr);
        jstring_append_1(string, tempstr);
        JXTA_OBJECT_RELEASE(tempstr);
        jstring_append_2(string, "\"");
    } else if( NULL != myself->dst_peer_ea ){
        char * ea_str;
        
        jstring_append_2(string, " dst_ea=\"");
        ea_str = jxta_endpoint_address_to_string(myself->dst_peer_ea);
        jstring_append_2(string, ea_str);
        free(ea_str);
        jstring_append_2(string, "\"");
    }

    jstring_append_2(string, ">\n ");

    for (i=0; TRUE == with_entries && i< jxta_vector_size(myself->entries); i++) {
        Jxta_status status;
        Jxta_endpoint_msg_entry_element *entry;
        char **keys=NULL;
        char **keys_save=NULL;
        JString *tmp_j=NULL;
        JString *tmp_val_j=NULL;
        Jxta_message_element *elem=NULL;
        Jxta_bytevector *bytes=NULL;

        status = jxta_vector_get_object_at(myself->entries, JXTA_OBJECT_PPTR(&entry), i);
        if (JXTA_SUCCESS != status) {
            continue;
        }
        jstring_append_2(string, "<EPEntry ");
        jstring_append_2(string, " mime=\"");
        jstring_append_2(string, entry->mime);
        jstring_append_2(string, "\"");
        jstring_append_2(string, " ns=\"");
        jstring_append_2(string, entry->ns);
        jstring_append_2(string, "\"");
        jstring_append_2(string, " name=\"");
        jstring_append_2(string, entry->name);
        jstring_append_2(string, "\"");
        keys = jxta_hashtable_keys_get(entry->attributes);
        keys_save = keys;
        while (*keys) {
            JString *att_val_j=NULL;

            if (JXTA_SUCCESS != jxta_hashtable_get(entry->attributes, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&att_val_j))) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "Unable to retrieve attribute from EPEntry [%pp] hash key:%s\n", myself, *keys);

            } else {
                jstring_append_2(string, " ");
                jstring_append_2(string, *keys);
                jstring_append_2(string, "=\"");
                jstring_append_1(string, att_val_j);
                jstring_append_2(string, "\"");
            }
            if (att_val_j)
                JXTA_OBJECT_RELEASE(att_val_j);
            free(*(keys++));
        }
        while (*keys) free(*(keys++));
        free(keys_save);
        jstring_append_2(string, ">");
        jxta_endpoint_msg_entry_get_value(entry, &elem);
        bytes = jxta_message_element_get_value(elem);
        tmp_val_j = jstring_new_2(jxta_bytevector_content_ptr(bytes));
        if (encode) {
            jxta_xml_util_encode_jstring(tmp_val_j, &tmp_j);

            jstring_append_1(string, tmp_j);
        } else {
            jstring_append_1(string, tmp_val_j);
        }
        jstring_append_2(string, "</EPEntry>\n");

        if (entry)
            JXTA_OBJECT_RELEASE(entry);
        if (elem)
            JXTA_OBJECT_RELEASE(elem);
        if (tmp_j)
            JXTA_OBJECT_RELEASE(tmp_j);
        if (tmp_val_j)
            JXTA_OBJECT_RELEASE(tmp_val_j);
        if (bytes)
            JXTA_OBJECT_RELEASE(bytes);
    }
    
    /* FIXME 20060915 bondolo Handle credentials and options */

    jstring_append_2(string, "</jxta:EndpointMessage>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_id *) jxta_endpoint_msg_get_src_peer_id(Jxta_endpoint_message * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_endpoint_msg_set_src_peer_id(Jxta_endpoint_message * myself, Jxta_id * src_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(src_peer_id);

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    
    /* src peer id may not be NULL. */
    myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(Jxta_id *) jxta_endpoint_msg_get_dst_peer_id(Jxta_endpoint_message * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->dst_peer_id);
}

JXTA_DECLARE(void) jxta_endpoint_msg_set_dst_peer_id(Jxta_endpoint_message * myself, Jxta_id * dst_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(dst_peer_id);

    JXTA_OBJECT_RELEASE(myself->dst_peer_id);
    
    /* dest peer id may not be NULL. */
    myself->dst_peer_id = JXTA_OBJECT_SHARE(dst_peer_id);
}

JXTA_DECLARE(Jxta_endpoint_address * ) jxta_endpoint_msg_get_dst_peer_ea(Jxta_endpoint_message * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if( NULL != myself->dst_peer_ea ) {
        return JXTA_OBJECT_SHARE(myself->dst_peer_ea);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_endpoint_msg_set_dst_peer_ea(Jxta_endpoint_message * myself, Jxta_endpoint_address * dst_peer_ea)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if( NULL != myself->dst_peer_ea ) {
        JXTA_OBJECT_RELEASE(myself->dst_peer_ea);
        myself->dst_peer_ea = NULL;
    }
    
    if( NULL != dst_peer_ea ) {
        JXTA_OBJECT_CHECK_VALID(dst_peer_ea);
        myself->dst_peer_ea = JXTA_OBJECT_SHARE(dst_peer_ea);
    } else {
        myself->dst_peer_ea = NULL;
    }
}

JXTA_DECLARE(Jxta_credential *) jxta_endpoint_msg_get_credential(Jxta_endpoint_message * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_endpoint_msg_set_credential(Jxta_endpoint_message * myself, Jxta_credential *credential)
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

JXTA_DECLARE(void) jxta_endpoint_msg_set_priority(Jxta_endpoint_message *ep_msg, Msg_priority p)
{
    ep_msg->priority = p;
}

JXTA_DECLARE(Msg_priority) jxta_endpoint_msg_priority(Jxta_endpoint_message *ep_msg)
{
    return ep_msg->priority;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_set_timestamp(Jxta_endpoint_message *myself, Jxta_time timestamp)
{
    myself->timestamp = timestamp;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_time) jxta_endpoint_msg_timestamp(Jxta_endpoint_message *myself)
{
    return myself->timestamp;
}

JXTA_DECLARE(Jxta_endpoint_msg_entry_element *) jxta_endpoint_message_entry_new()
{
    Jxta_endpoint_msg_entry_element *myself = (Jxta_endpoint_msg_entry_element *) calloc(1, sizeof(Jxta_endpoint_msg_entry_element));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, endpoint_msg_entry_delete, NULL);

    myself->attributes = jxta_hashtable_new(0);
    return myself;
}

JXTA_DECLARE(void) jxta_endpoint_msg_entry_set_value(Jxta_endpoint_msg_entry_element *entry, Jxta_message_element *value)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    if (entry->value)
        JXTA_OBJECT_RELEASE(entry->value);

    entry->mime = strdup(jxta_message_element_get_mime_type(value));
    entry->ns = strdup(jxta_message_element_get_namespace(value));
    entry->name = strdup(jxta_message_element_get_name(value));
    entry->value = (NULL != value) ? JXTA_OBJECT_SHARE(value):NULL;
}

JXTA_DECLARE(void) jxta_endpoint_msg_entry_get_value(Jxta_endpoint_msg_entry_element *entry, Jxta_message_element **value)
{
    JXTA_OBJECT_CHECK_VALID(entry);
    *value = (NULL != entry->value) ? JXTA_OBJECT_SHARE(entry->value):NULL;
}

JXTA_DECLARE(void) jxta_endpoint_msg_entry_set_msg(Jxta_endpoint_msg_entry_element *entry, Jxta_message * msg)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    if (entry->msg)
        JXTA_OBJECT_RELEASE(msg);
    entry->msg = NULL != msg ? JXTA_OBJECT_SHARE(msg):NULL;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_get_msg(Jxta_endpoint_msg_entry_element *entry, Jxta_message ** msg)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    *msg = NULL != entry->msg ? JXTA_OBJECT_SHARE(entry->msg):NULL;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_add_attribute(Jxta_endpoint_msg_entry_element *entry, const char *attribute, const char *value)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    Jxta_status res = JXTA_SUCCESS;
    JString *att_value_j=NULL;

    if (JXTA_SUCCESS == jxta_hashtable_get(entry->attributes, attribute, strlen(attribute) + 1, JXTA_OBJECT_PPTR(&att_value_j))) {
        res = JXTA_ITEM_EXISTS;
    } else {
        att_value_j = jstring_new_2(value);
        jxta_hashtable_put(entry->attributes, attribute, strlen(attribute) + 1, (Jxta_object *) att_value_j);
    }

    if (att_value_j)
        JXTA_OBJECT_RELEASE(att_value_j);
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_get_attribute(Jxta_endpoint_msg_entry_element *entry, const char *attribute
                                    , JString ** get_att_j)
{
    Jxta_status res;
    JString * att_j;

    if (JXTA_SUCCESS == jxta_hashtable_get(entry->attributes, attribute, strlen(attribute) + 1,  JXTA_OBJECT_PPTR(&att_j))) {
        res = JXTA_ITEM_EXISTS;
        *get_att_j = att_j;
    } else {
        res = JXTA_ITEM_NOTFOUND;
    }
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_add_entry(Jxta_endpoint_message *msg, Jxta_endpoint_msg_entry_element * entry)
{
    JXTA_OBJECT_CHECK_VALID(msg);
    Jxta_status res = JXTA_SUCCESS;

    if (NULL == entry) {
        res = JXTA_INVALID_ARGUMENT;
    } else {
        res = jxta_vector_add_object_last(msg->entries, (Jxta_object *) entry);
    }
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_get_entries(Jxta_endpoint_message *msg, Jxta_vector **entries)
{
    *entries = NULL != msg->entries ? JXTA_OBJECT_SHARE(msg->entries):NULL;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_get_attributes(Jxta_endpoint_msg_entry_element *entry, Jxta_hashtable **attributes)
{
    *attributes = NULL != entry->attributes ? JXTA_OBJECT_SHARE(entry->attributes):NULL;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_endpoint_msg_entry_set_ea(Jxta_endpoint_msg_entry_element *entry, Jxta_endpoint_address *ea)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    if (entry->ea)
        JXTA_OBJECT_RELEASE(entry->ea);
    entry->ea = NULL != ea ? JXTA_OBJECT_SHARE(ea):NULL;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_endpoint_msg_entry_get_ea(Jxta_endpoint_msg_entry_element *entry, Jxta_endpoint_address **ea)
{
    JXTA_OBJECT_CHECK_VALID(entry);

    *ea = NULL != entry->ea ? JXTA_OBJECT_SHARE(entry->ea):NULL;
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_endpoint_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_endpoint_message *myself = (Jxta_endpoint_message *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:EndpointMessage> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "src_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                
                JXTA_OBJECT_RELEASE(myself->src_peer_id);
                myself->src_peer_id = NULL;

                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->src_peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for src peer id [%pp]\n", myself);
                    myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "dst_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                
                JXTA_OBJECT_RELEASE(myself->dst_peer_id);
                myself->dst_peer_id = NULL;

                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->dst_peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for dest peer id [%pp]\n", myself);
                    myself->dst_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "dst_ea")) {
                JString *eastr = jstring_new_2(atts[1]);
                Jxta_endpoint_address * dst_ea;
                
                jstring_trim(eastr);

                dst_ea = jxta_endpoint_address_new_1( eastr );
                JXTA_OBJECT_RELEASE(eastr);
                
                jxta_endpoint_msg_set_dst_peer_ea( myself, dst_ea );
                JXTA_OBJECT_RELEASE(dst_ea);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:EndpointMessage> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_endpoint_message *myself = (Jxta_endpoint_message *) me;
    
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


static void handle_ep_entry(void *me, const XML_Char * cd, int len)
{
    Jxta_endpoint_message *myself = (Jxta_endpoint_message *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    Jxta_endpoint_msg_entry_element *entry=NULL;

    const char **atts = ((Jxta_advertisement *) myself)->atts;

    if (NULL == myself->curr_elem) {
        entry = (Jxta_endpoint_msg_entry_element *) calloc(1, sizeof(Jxta_endpoint_msg_entry_element));

        JXTA_OBJECT_INIT(entry, endpoint_msg_entry_delete, 0);
        myself->curr_elem = JXTA_OBJECT_SHARE(entry);

        entry->attributes = jxta_hashtable_new(0);
    } else {
        entry = JXTA_OBJECT_SHARE(myself->curr_elem);
    }

    if (len == 0) {

        if (atts != NULL) {
            while (*atts) {
                JString *val_j=NULL;
                if (0 == strcmp(*atts, "elem")) {
                    entry->elem = strdup(atts[1]);
                } else if (0 == strcmp(*atts, "mime")) {
                    entry->mime = strdup(atts[1]);
                } else if (0 == strcmp(*atts, "ns")) {
                    entry->ns = strdup(atts[1]);
                } else if (0 == strcmp(*atts, "name")) {
                    entry->name = strdup(atts[1]);
                } else {
                    val_j = jstring_new_2(atts[1]);
                    jxta_hashtable_put(entry->attributes, *atts, strlen(*atts) + 1, (Jxta_object *) val_j);
                    JXTA_OBJECT_RELEASE(val_j);
                }
                atts += 2;
            }
        }
    } else if (len > 0) {
        JString *value = NULL;
        JString *entry_value = NULL;
        Jxta_message_element *msg_elem=NULL;
        int byte_length;

        value = jstring_new_1(len);
        jstring_append_0(value, cd, len);
        jstring_trim(value);

        if (jstring_length(value) == 0) {
            JXTA_OBJECT_RELEASE(value);
            value = NULL;
        }

        if (JXTA_SUCCESS == jxta_xml_util_decode_jstring(value, &entry_value)) {
            byte_length = strlen(jstring_get_string(entry_value));

            msg_elem = jxta_message_element_new_2(entry->ns, entry->name, entry->mime, jstring_get_string(entry_value), byte_length, NULL);

            if (NULL != msg_elem) {
                entry->value = JXTA_OBJECT_SHARE(msg_elem);

                jxta_vector_add_object_last(myself->entries, (Jxta_object *) entry);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "got an EPEntry %s\n", jstring_get_string(entry_value));
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "No message element ns:%s name:%s\n%s\n", entry->ns, entry->name, jstring_get_string(entry_value));
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unable to decode string in endpoint message entry\n");
        }
        JXTA_OBJECT_RELEASE(msg_elem);
        if (value)
            JXTA_OBJECT_RELEASE(value);
        if (entry_value)
            JXTA_OBJECT_RELEASE(entry_value);
        JXTA_OBJECT_RELEASE(myself->curr_elem);
        myself->curr_elem = NULL;
    }

    JXTA_OBJECT_RELEASE(entry);
}

/* vim: set ts=4 sw=4 et tw=130: */

