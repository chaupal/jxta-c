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
 *    if any, must include the following acknowledgement:
 *       "This product includes software developed by the
 *       Sun Microsystems, Inc. for Project JXTA."
 *    Alternately, this acknowledgement may appear in the software itself,
 *    if and wherever such third-party acknowledgements normally appear.
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
 * $Id: jxta_monitor_msg.c 207 2008-01-23 19:31:17Z ExocetRick $
 */

static const char *__log_cat = "MONITOR_MSG";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_peer.h"
#include "jxta_monitor_service.h"


const char JXTA_MONITOR_MSG_ELEMENT_NAME[] = "Monitor";

static const char * monitor_msg_type_text[] = {
        "MON_MONITOR",
        "MON_REQUEST",
        "MON_COMMAND",
        "MON_STATUS"
};

static const char * monitor_msg_cmd_text[] = {
        "CMD_START_MONITOR",
        "CMD_STOP_MONITOR",
        "CMD_GET_TYPE",
        "CMD_STATUS"
};

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/

struct _jxta_monitor_msg {
    Jxta_advertisement jxta_advertisement;
   
    Jxta_credential *credential;   
    Jxta_id *peer_id;
    Jxta_monitor_msg_cmd cmd;
    Jxta_monitor_msg_type type;
    //Jxta_monitor_entries
    Jxta_vector *entries;
    char * current_context;
    Jxta_hashtable * current_sub_context_hash;
    char * current_sub_context;
//    Jxta_vector * current_entries_vector;
    Jxta_hashtable *context_ids;
    char * current_entry_type;
};


static void monitor_msg_delete(Jxta_object * me);
static void handle_monitor_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_context(void *me, const XML_Char * cd, int len);
static void handle_sub_context(void *me, const XML_Char * cd, int len);
static void handle_entry(void *me, const XML_Char * cd, int len);
static Jxta_status print_entries_xml(JString *xml_string_j, Jxta_vector * options);
static Jxta_status validate_message(Jxta_monitor_msg * myself);

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    Jxta_monitor_msg_,
    Credential_,
    Context_,
    SubContext_,
    Entry_
};

/** 
 * Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, diffusion through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_monitor_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:Monitor", Jxta_monitor_msg_, *handle_monitor_msg, NULL, NULL},
    {"Credential", Credential_, *handle_credential, NULL, NULL},
    {"Context", Context_, *handle_context, NULL, NULL},
    {"SubContext", SubContext_, *handle_sub_context, NULL, NULL},
    {"Entry", Entry_, *handle_entry, NULL, NULL},
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_monitor_msg *) jxta_monitor_msg_new(void)
{
    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) calloc(1, sizeof(Jxta_monitor_msg));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_monitor_msg NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, monitor_msg_delete, NULL);

    myself = (Jxta_monitor_msg *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:Monitor",
                                                                 jxta_monitor_msg_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_monitor_msg_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);

    if (NULL != myself) {
        myself->credential = NULL;
        myself->peer_id = NULL;
        myself->entries = jxta_vector_new(0);
        myself->context_ids = jxta_hashtable_new(0);
    }

    return myself;
}

static void monitor_msg_delete(Jxta_object * me)
{
    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;

    if (myself->credential)
        JXTA_OBJECT_RELEASE(myself->credential);

    if (myself->entries)
        JXTA_OBJECT_RELEASE(myself->entries);

    if (myself->context_ids)
        JXTA_OBJECT_RELEASE(myself->context_ids);

    if (myself->current_sub_context_hash)
        JXTA_OBJECT_RELEASE(myself->current_sub_context_hash);

    if (myself->peer_id)
        JXTA_OBJECT_RELEASE(myself->peer_id);

    if (myself->current_context)
        free(myself->current_context);

    if (myself->current_sub_context)
        free(myself->current_sub_context);

    if (myself->current_entry_type)
        free(myself->current_entry_type);

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_monitor_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_monitor_msg FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_id *) jxta_monitor_msg_get_peer_id(Jxta_monitor_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->peer_id);
}

JXTA_DECLARE(void) jxta_monitor_msg_set_peer_id(Jxta_monitor_msg * myself, Jxta_id * peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(peer_id);

    JXTA_OBJECT_RELEASE(myself->peer_id);
    
    /* peer id may not be NULL. */
    myself->peer_id = JXTA_OBJECT_SHARE(peer_id);
}

JXTA_DECLARE(Jxta_monitor_msg_cmd) jxta_monitor_msg_get_cmd(Jxta_monitor_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return me->cmd;
}

JXTA_DECLARE(const char *) jxta_monitor_msg_cmd_text(Jxta_monitor_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return monitor_msg_cmd_text[me->cmd];
}

JXTA_DECLARE(const char *) jxta_monitor_msg_type_text(Jxta_monitor_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return monitor_msg_type_text[me->type];
}


JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_cmd(Jxta_monitor_msg * me, Jxta_monitor_msg_cmd cmd)
{
    Jxta_status res = JXTA_SUCCESS;
    JXTA_OBJECT_CHECK_VALID(me);
    me->cmd = cmd;
    return res;
}

JXTA_DECLARE(Jxta_credential *) jxta_monitor_msg_get_credential(Jxta_monitor_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_monitor_msg_set_credential(Jxta_monitor_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get_entries(Jxta_monitor_msg * myself, Jxta_vector ** result)
{
    Jxta_status ret = JXTA_SUCCESS;
    char **c_keys;
    char **c_keys_save;

    *result = jxta_vector_new(0);
    
    JXTA_OBJECT_CHECK_VALID(myself);
    c_keys = jxta_hashtable_keys_get(myself->context_ids);
    c_keys_save = c_keys;

    while (*c_keys) {
        Jxta_hashtable *sub_contexts = NULL;

        ret = jxta_hashtable_get(myself->context_ids, *c_keys, strlen(*c_keys) + 1, JXTA_OBJECT_PPTR(&sub_contexts));
        if (JXTA_SUCCESS == ret) {
            char **s_keys;
            char **s_keys_save;

            s_keys = jxta_hashtable_keys_get(sub_contexts);
            s_keys_save = s_keys;
            while (*s_keys) {
                Jxta_hashtable *type_hashtable = NULL;
                ret = jxta_hashtable_get(sub_contexts, *s_keys, strlen(*s_keys) + 1, JXTA_OBJECT_PPTR(&type_hashtable));
                if (JXTA_SUCCESS == ret) {
                    char **t_keys;
                    char **t_keys_save;

                    t_keys = jxta_hashtable_keys_get(type_hashtable);
                    t_keys_save = t_keys;
                    while(*t_keys) {
                    
                        Jxta_vector *entries = NULL;

                        ret = jxta_hashtable_get(type_hashtable, *t_keys, strlen(*t_keys) +1, JXTA_OBJECT_PPTR(&entries));
                        
                        unsigned int i;
                        for (i = 0; i < jxta_vector_size(entries); i++) {
                            Jxta_monitor_entry * entry;

                            ret = jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i);
                            if (JXTA_SUCCESS != ret) {
                                continue;
                            }
                            jxta_vector_add_object_last(*result, (Jxta_object *) entry);
                            JXTA_OBJECT_RELEASE(entry);
                        }
                        JXTA_OBJECT_RELEASE(entries);

                        free(*t_keys);
                        t_keys++;
                    }
                    free(t_keys_save);

                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "unable to retrieve service with key:%s\n", *s_keys);
                }
                free(*s_keys);
                s_keys++;
            }
            free(s_keys_save);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "unable to retrieve context with key:%s\n", *c_keys);
        }
        JXTA_OBJECT_RELEASE(sub_contexts);
        free(*c_keys);
        c_keys++;
    }
    free(c_keys_save);

    return ret;
}

JXTA_DECLARE(void) jxta_monitor_msg_clear_entries(Jxta_monitor_msg * myself ) {
    JXTA_OBJECT_CHECK_VALID(myself);

    jxta_vector_clear( myself->entries);
}

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_set_entries(Jxta_monitor_msg * myself, Jxta_hashtable * grp_services)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    JXTA_OBJECT_CHECK_VALID(grp_services);

    if (NULL != myself->context_ids) {
        JXTA_OBJECT_RELEASE(myself->context_ids);
    }
    myself->context_ids = JXTA_OBJECT_SHARE(grp_services);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_add_entry(Jxta_monitor_msg * myself, const char *context, const char *sub_context, Jxta_monitor_entry * entry)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_hashtable * sub_context_hash = NULL;
    Jxta_vector *entries_v = NULL;

    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(entry);


    if (JXTA_SUCCESS != jxta_hashtable_get(myself->context_ids, context, strlen(context) + 1, JXTA_OBJECT_PPTR(&sub_context_hash))) {

        sub_context_hash = jxta_hashtable_new(0);

        jxta_hashtable_put(myself->context_ids, context, strlen(context) + 1, (Jxta_object *) sub_context_hash);
    }

    if (JXTA_SUCCESS != jxta_hashtable_get(sub_context_hash, sub_context, strlen(sub_context) + 1, JXTA_OBJECT_PPTR(&entries_v))) {
        entries_v = jxta_vector_new(0);
        jxta_hashtable_put(sub_context_hash, sub_context, strlen(sub_context) +1, (Jxta_object *) entries_v);
    }

    if (NULL != entries_v) {
        res = jxta_vector_add_object_last( entries_v, (Jxta_object*) entry );
    }
    if (sub_context_hash)
        JXTA_OBJECT_RELEASE(sub_context_hash);
    if (entries_v)
        JXTA_OBJECT_RELEASE(entries_v);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_parse_charbuffer(Jxta_monitor_msg * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_parse_file(Jxta_monitor_msg * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_status validate_message(Jxta_monitor_msg * myself) {

    if ( (NULL == myself->peer_id) || jxta_id_equals(myself->peer_id, jxta_id_nullID) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_msg_get_xml(Jxta_monitor_msg * myself, JString ** xml)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *string;
    JString *temp;
    char ** keys;
    char ** keysSave;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */
 
    JXTA_OBJECT_CHECK_VALID(myself);

    if (xml == NULL) {
        res = JXTA_INVALID_ARGUMENT;
        goto FINAL_EXIT;
    }
    
/*    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
*/ 
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:Monitor");
    if (NULL != myself->peer_id) {
        jstring_append_2(string, " peer_id=\"");
        jxta_id_to_jstring(myself->peer_id, &temp);
        jstring_append_1(string, temp);
        JXTA_OBJECT_RELEASE(temp);
        jstring_append_2(string, "\"");
    }
    jstring_append_2(string, " type=\"");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->type);
    jstring_append_2(string, "\"");
    jstring_append_2(string, " command=\"");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->cmd);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");

    keys = jxta_hashtable_keys_get(myself->context_ids);
    keysSave = keys;

    while(*keys) {
        char ** sContext = NULL; 
        char ** sContextSave = NULL;
        Jxta_hashtable *sub_context_hash = NULL;

        jstring_append_2(string, "<Context id=\"");
        jstring_append_2(string, *keys);
        jstring_append_2(string, "\">\n");
        if (JXTA_SUCCESS == jxta_hashtable_get(myself->context_ids, *keys, strlen(*keys) + 1, JXTA_OBJECT_PPTR(&sub_context_hash))) {
            sContext = jxta_hashtable_keys_get(sub_context_hash);
            sContextSave = sContext;
            while (*sContext) {
                Jxta_hashtable *type_hashtable = NULL;
                jstring_append_2(string, "<SubContext id=\"");
                jstring_append_2(string, *sContext);
                jstring_append_2(string, "\">\n");
                if (JXTA_SUCCESS == jxta_hashtable_get(sub_context_hash, *sContext, strlen(*sContext) + 1, JXTA_OBJECT_PPTR(&type_hashtable))) {
                    char ** tKeys = NULL;
                    char ** tKeysSave = NULL;
                    tKeys = jxta_hashtable_keys_get(type_hashtable);
                    tKeysSave = tKeys;
                    while(*tKeys) {
                        Jxta_vector * entries = NULL;
                        if(JXTA_SUCCESS == jxta_hashtable_get(type_hashtable, *tKeys, strlen(*tKeys) +1, JXTA_OBJECT_PPTR(&entries))){
                            print_entries_xml(string, entries);
                            JXTA_OBJECT_RELEASE(entries);
                        }
                        free(*tKeys);
                        tKeys++;
                    }
                    free(tKeysSave);
                    JXTA_OBJECT_RELEASE(type_hashtable);
                }
                jstring_append_2(string, "</SubContext>\n");
                free(*sContext);
                sContext++;
            }
            free(sContextSave);
            JXTA_OBJECT_RELEASE(sub_context_hash);
        }
        jstring_append_2(string, "</Context>\n");
        free(*keys);
        keys++;
    }

    free(keysSave);

    jstring_append_2(string, "</jxta:Monitor>\n");

    *xml = string;

FINAL_EXIT:

    return res;
}

static Jxta_status print_entries_xml(JString *xml_string_j, Jxta_vector * entries) {

    int each_entry;
    unsigned int all_entries;
    Jxta_status res = JXTA_SUCCESS;

    all_entries = jxta_vector_size(entries);
    for( each_entry = all_entries-1; each_entry >= 0; each_entry-- ) {
        Jxta_advertisement * entry = NULL;

        res = jxta_vector_get_object_at( entries, JXTA_OBJECT_PPTR(&entry), each_entry );

        if( JXTA_SUCCESS == res ) {
            JString *tempstr;

            res = jxta_advertisement_get_xml( entry, &tempstr );

            if( JXTA_SUCCESS == res ) {
                jstring_append_1(xml_string_j, tempstr);
                JXTA_OBJECT_RELEASE(tempstr);
                tempstr = NULL;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed generating XML for entry [%pp].\n", entry);
            }
        } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed getting entry [%pp].\n", entry);
        }
        JXTA_OBJECT_RELEASE(entry);
    }

    return res;
} 
/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_monitor_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:Monitor> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (myself->peer_id) {
                    JXTA_OBJECT_RELEASE(myself->peer_id);
                    myself->peer_id = NULL;
                }
                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    myself->peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "command")) {
                Jxta_monitor_msg_cmd cmd;
                cmd = atoi(atts[1]);
                myself->cmd = cmd;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:Monitor> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;
    
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

static void handle_entry(void *me, const XML_Char * cd, int len)
{

    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Entry> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                if(myself->current_entry_type) 
                    free(myself->current_entry_type);
                myself->current_entry_type = strdup(atts[1]);
            }
            atts += 2;
        }
        if (NULL == myself->current_sub_context_hash ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received Entry element without a current Service : [%pp]\n", myself);
        }
    } else {
        Jxta_status status;
        Jxta_vector * res_vect = NULL;
        Jxta_advertisement * newAdv = NULL;
        const char *type = myself->current_entry_type;
        Jxta_hashtable * type_hashtable= NULL;
        Jxta_vector *entries_v = NULL;
        Jxta_monitor_entry * entry;


        newAdv = jxta_advertisement_new();
        status = jxta_advertisement_parse_charbuffer(newAdv, cd, len);
        jxta_advertisement_get_advs(newAdv, &res_vect);

        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to parse char buffer : %s\n", cd);
        } else {
            if (newAdv) {
                JXTA_OBJECT_RELEASE(newAdv);
                newAdv = NULL;
            }
            jxta_vector_get_object_at(res_vect, JXTA_OBJECT_PPTR(&newAdv), 0);

            /** make sure the type hashtable for this sub-context has been created **/
            if (JXTA_SUCCESS != jxta_hashtable_get(myself->current_sub_context_hash, myself->current_sub_context, strlen(myself->current_sub_context)+1, JXTA_OBJECT_PPTR(&type_hashtable))) {
                /** hashtable should have already been created... let's just do it again **/
                type_hashtable = jxta_hashtable_new(0);
                jxta_hashtable_put(myself->current_sub_context_hash, myself->current_sub_context, strlen(myself->current_sub_context)+1, (Jxta_object *) type_hashtable);
            }

            /** check if and entries vector has already been created for this type **/
            if (JXTA_SUCCESS != jxta_hashtable_get(type_hashtable, type, strlen(type)+1, JXTA_OBJECT_PPTR(entries_v))) {
                entries_v = jxta_vector_new(0);
                jxta_hashtable_put(type_hashtable, type, strlen(type)+1, (Jxta_object*) entries_v);
            }

            entry = jxta_monitor_entry_new();
            //sets advertisement
            jxta_monitor_entry_set_entry(entry, newAdv);
            jxta_monitor_entry_set_context(entry, myself->current_context);
            jxta_monitor_entry_set_sub_context(entry, myself->current_sub_context);
            jxta_monitor_entry_set_type(entry, type);
            jxta_vector_add_object_last(entries_v, (Jxta_object *) entry);
            JXTA_OBJECT_RELEASE(entry);
        
        }
        if (newAdv)
            JXTA_OBJECT_RELEASE(newAdv);
        if (res_vect)
            JXTA_OBJECT_RELEASE(res_vect);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <Entry> : [%pp] bytes: %d\n", myself, len);
    }

}

static void handle_context(void *me, const XML_Char * cd, int len)
{

    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;


        int context_id_size = 0;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Context> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "id")) {

                if (myself->current_context) {
                    free(myself->current_context);
                }

                myself->current_context = strdup(atts[1]);
                context_id_size = strlen(atts[1]) + 1;
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding Context : %s\n", myself->current_context);
                if (myself->current_sub_context_hash) {
                    JXTA_OBJECT_RELEASE(myself->current_sub_context_hash);
                    myself->current_sub_context_hash = NULL;
                }
                if (JXTA_SUCCESS != jxta_hashtable_get(myself->context_ids, atts[1], context_id_size, JXTA_OBJECT_PPTR(&myself->current_sub_context_hash))) {
                    Jxta_hashtable * sub_context_hash = NULL;

                    sub_context_hash = jxta_hashtable_new(0);
                    jxta_hashtable_put(myself->context_ids, atts[1], context_id_size, (Jxta_object *) sub_context_hash);
                    myself->current_sub_context_hash = sub_context_hash;
                }
            }
            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <Context> : [%pp] %s\n", myself, cd);
    }

}

static void handle_sub_context(void *me, const XML_Char * cd, int len)
{
    Jxta_monitor_msg *myself = (Jxta_monitor_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <SubContext> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "id")) {
                int sub_context_size = 0;

                if (myself->current_sub_context) {
                    free(myself->current_sub_context);
                }

                myself->current_sub_context = strdup(atts[1]);
                sub_context_size = strlen(atts[1]) + 1;
                if (NULL == myself->current_sub_context_hash) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Received SubContext element without a Context : [%pp]\n", myself);
                    break;
                }
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Received SubContext : %s\n", myself->current_sub_context);

                Jxta_hashtable * type_hashtable = NULL;

                /* atts[1] should be the service name */
                if (JXTA_SUCCESS != jxta_hashtable_get(myself->current_sub_context_hash, atts[1], sub_context_size, JXTA_OBJECT_PPTR(&type_hashtable))) {
                    type_hashtable = jxta_hashtable_new(0);
                    jxta_hashtable_put(myself->current_sub_context_hash, atts[1], sub_context_size, (Jxta_object *) type_hashtable);
                }
                JXTA_OBJECT_RELEASE(type_hashtable);
            }
            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <SubContext> : [%pp] %s\n", myself, cd);
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
