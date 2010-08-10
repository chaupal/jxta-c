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

static const char *__log_cat = "PV_PING";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_endpoint_address.h"

#include "jxta_peerview_ping_msg.h"

const char JXTA_PEERVIEW_PING_ELEMENT_NAME[] = "PeerviewPing";

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    PeerviewPingMsg_,
    Credential_,
    Group_,
    Option_
};

/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _Jxta_peerview_ping_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *src_peer_id;
    Jxta_id *dst_peer_id;
    Jxta_endpoint_address *dst_peer_ea;
    Jxta_boolean dst_peer_adv_gen_set;
    apr_uuid_t dst_peer_adv_gen;
    Jxta_boolean pv_id_only;
    Jxta_credential * credential;
    Jxta_vector * groups;
    Jxta_vector * options;
};

struct _jxta_pv_ping_msg_group_entry {
    JXTA_OBJECT_HANDLE;
    JString *groupid;
    Jxta_boolean pv_id_only;
    Jxta_peer *pve;
    apr_uuid_t peer_adv_gen;
    Jxta_boolean peer_adv_gen_set;
};


static void peerview_ping_msg_delete(Jxta_object * me);
static Jxta_peerview_ping_msg *peerview_ping_msg_construct(Jxta_peerview_ping_msg * myself);
static Jxta_status validate_message(Jxta_peerview_ping_msg * myself);
static void handle_peerview_ping_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_group(void *me, const XML_Char * cd, int len);
static void handle_option(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_peerview_ping_msg *) jxta_peerview_ping_msg_new(void)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) calloc(1, sizeof(Jxta_peerview_ping_msg));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, peerview_ping_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PeerviewPing NEW [%pp]\n", myself );

    return peerview_ping_msg_construct(myself);
}

static void peerview_ping_msg_delete(Jxta_object * me)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) me;

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    JXTA_OBJECT_RELEASE(myself->dst_peer_id);

    if (NULL != myself->dst_peer_ea) {
        JXTA_OBJECT_RELEASE(myself->dst_peer_ea);
        myself->dst_peer_ea = NULL;
    }

    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (NULL != myself->groups) {
        JXTA_OBJECT_RELEASE(myself->groups);
        myself->groups = NULL;
    }
    if (NULL != myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
        myself->options = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_peerview_ping_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PeerviewPing FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab peerview_ping_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PeerviewPing", PeerviewPingMsg_, *handle_peerview_ping_msg, NULL, NULL},
    {"Credential", Credential_, *handle_credential, NULL, NULL},
    {"Group", Group_, *handle_group, NULL, NULL},
    {"Option", Option_, *handle_option, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_peerview_ping_msg *peerview_ping_msg_construct(Jxta_peerview_ping_msg * myself)
{
    myself = (Jxta_peerview_ping_msg *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:PeerviewPing",
                                     peerview_ping_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_peerview_ping_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, 
                                     (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->dst_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->dst_peer_ea = NULL;
        myself->pv_id_only = FALSE;
        myself->dst_peer_adv_gen_set = FALSE;
        myself->credential = NULL;
        myself->groups = jxta_vector_new(0);
        myself->options = jxta_vector_new(0);
    }

    return myself;
}

static Jxta_status validate_message(Jxta_peerview_ping_msg * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->src_peer_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Source peer id must not be NULL [%pp]\n", myself);
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

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_parse_charbuffer(Jxta_peerview_ping_msg * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_parse_file(Jxta_peerview_ping_msg * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_get_xml(Jxta_peerview_ping_msg * myself, JString ** xml)
{
    JString *string;
    JString *tempstr;
    int i;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<jxta:PeerviewPing ");


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

    if (myself->dst_peer_adv_gen_set) {
        jstring_append_2(string, " dst_adv_gen=\"");
        apr_uuid_format(tmpbuf, &myself->dst_peer_adv_gen);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }
    jstring_append_2(string, ">\n ");
    for (i=0; NULL != myself->groups && i < jxta_vector_size(myself->groups); i++) {
        Jxta_pv_ping_msg_group_entry *entry=NULL;

        jxta_vector_get_object_at(myself->groups, JXTA_OBJECT_PPTR(&entry), i);
        jstring_append_2(string, "<Group");

        jstring_append_2(string, " pv_id_only=\"");
        jstring_append_2(string, TRUE == entry->pv_id_only ? "yes":"no");
        jstring_append_2(string, "\"");
        if (entry->peer_adv_gen_set) {
            jstring_append_2(string, " adv_gen=\"");
            apr_uuid_format(tmpbuf, &entry->peer_adv_gen);
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "\"");
        }
        jstring_append_2(string, " id=\"");
        jstring_append_2(string, jstring_get_string(entry->groupid));
        jstring_append_2(string, "\"");
        jstring_append_2(string, ">");

        JXTA_OBJECT_RELEASE(entry);
        jstring_append_2(string, "</Group>");
    }
    for (i=0; i < jxta_vector_size(myself->options); i++) {
        Jxta_peerview_option_entry *entry=NULL;
        JString *entry_j;

        jxta_vector_get_object_at(myself->options, JXTA_OBJECT_PPTR(&entry), i);

        jxta_peerview_option_entry_get_xml(entry, &entry_j);
        jstring_append_1(string, entry_j);

        JXTA_OBJECT_RELEASE(entry);
        JXTA_OBJECT_RELEASE(entry_j);
    }

    
    /* FIXME 20060915 bondolo Handle credentials and options */

    jstring_append_2(string, "</jxta:PeerviewPing>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_id *) jxta_peerview_ping_msg_get_src_peer_id(Jxta_peerview_ping_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_src_peer_id(Jxta_peerview_ping_msg * myself, Jxta_id * src_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(src_peer_id);

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    
    /* src peer id may not be NULL. */
    myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(Jxta_id *) jxta_peerview_ping_msg_get_dst_peer_id(Jxta_peerview_ping_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->dst_peer_id);
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dst_peer_id(Jxta_peerview_ping_msg * myself, Jxta_id * dst_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(dst_peer_id);

    JXTA_OBJECT_RELEASE(myself->dst_peer_id);
    
    /* dest peer id may not be NULL. */
    myself->dst_peer_id = JXTA_OBJECT_SHARE(dst_peer_id);
}

JXTA_DECLARE(Jxta_endpoint_address * ) jxta_peerview_ping_msg_get_dst_peer_ea(Jxta_peerview_ping_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if( NULL != myself->dst_peer_ea ) {
        return JXTA_OBJECT_SHARE(myself->dst_peer_ea);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dst_peer_ea(Jxta_peerview_ping_msg * myself, Jxta_endpoint_address * dst_peer_ea)
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

JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_is_pv_id_only(Jxta_peerview_ping_msg * myself)
{
    return myself->pv_id_only;
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_pv_id_only(Jxta_peerview_ping_msg * myself, Jxta_boolean pv_id_only)
{
    myself->pv_id_only = pv_id_only;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_is_broadcast(Jxta_peerview_ping_msg *myself)
{
    return (jxta_id_equals(myself->dst_peer_id, jxta_id_nullID) && NULL == myself->dst_peer_ea) ? TRUE:FALSE;
}

JXTA_DECLARE(apr_uuid_t *) jxta_peerview_ping_msg_get_dest_peer_adv_gen(Jxta_peerview_ping_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if ( myself->dst_peer_adv_gen_set ) {
        apr_uuid_t *result = calloc(1, sizeof(apr_uuid_t));

        if (NULL == result) {
            return NULL;
        }

        memmove(result, &myself->dst_peer_adv_gen, sizeof(apr_uuid_t));
        return result;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_dest_peer_adv_gen(Jxta_peerview_ping_msg * myself, apr_uuid_t * dst_peer_adv_gen)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL == dst_peer_adv_gen) {
        myself->dst_peer_adv_gen_set = FALSE;
    } else {
        memmove( &myself->dst_peer_adv_gen, dst_peer_adv_gen, sizeof(apr_uuid_t));
        myself->dst_peer_adv_gen_set = TRUE;
    }
}

JXTA_DECLARE(Jxta_credential *) jxta_peerview_ping_msg_get_credential(Jxta_peerview_ping_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_set_credential(Jxta_peerview_ping_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_add_group_entry(Jxta_peerview_ping_msg * myself, Jxta_pv_ping_msg_group_entry *entry)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return jxta_vector_add_object_last(myself->groups, (Jxta_object *) entry);

}

JXTA_DECLARE(Jxta_status) jxta_peerview_ping_msg_get_group_entries(Jxta_peerview_ping_msg * myself, Jxta_vector **entries)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    *entries = JXTA_OBJECT_SHARE(myself->groups);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_peer *) jxta_peerview_ping_msg_entry_get_pve(Jxta_pv_ping_msg_group_entry * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return NULL != myself->pve ? JXTA_OBJECT_SHARE(myself->pve):NULL;
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_add_option_entry(Jxta_peerview_ping_msg *me, Jxta_peerview_option_entry *option_entry)
{
    jxta_vector_add_object_last(me->options, (Jxta_object *) option_entry);
    return;
}

JXTA_DECLARE(void) jxta_peerview_ping_msg_entry_set_pv_id_only(Jxta_pv_ping_msg_group_entry * me, Jxta_boolean pv_id_only) 
{
    JXTA_OBJECT_CHECK_VALID(me);

    me->pv_id_only = pv_id_only;
}

JXTA_DECLARE(JString *) jxta_peerview_ping_msg_entry_group_name(Jxta_pv_ping_msg_group_entry *me)
{
    return NULL != me->groupid ? JXTA_OBJECT_SHARE(me->groupid):NULL;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_ping_msg_entry_is_pv_id_only(Jxta_pv_ping_msg_group_entry * me)
{
    return me->pv_id_only;
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_peerview_ping_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PeerviewPing> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "pv_id_only")) {
                if (0 == strcmp(atts[1], "yes")) {
                    myself->pv_id_only = TRUE;
                } else {
                    myself->pv_id_only = FALSE;
                }
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
            } else if (0 == strcmp(*atts, "dst_adv_gen")) {
                myself->dst_peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &myself->dst_peer_adv_gen, atts[1]));
                
                if( !myself->dst_peer_adv_gen_set ) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                }
            } else if (0 == strcmp(*atts, "dst_ea")) {
                JString *eastr = jstring_new_2(atts[1]);
                Jxta_endpoint_address * dst_ea;
                
                jstring_trim(eastr);

                dst_ea = jxta_endpoint_address_new_1( eastr );
                JXTA_OBJECT_RELEASE(eastr);
                
                jxta_peerview_ping_msg_set_dst_peer_ea( myself, dst_ea );
                JXTA_OBJECT_RELEASE(dst_ea);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PeerviewPing> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) me;
    
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

static void pv_group_entry_free(Jxta_object *me)
{
    Jxta_pv_ping_msg_group_entry *myself = (Jxta_pv_ping_msg_group_entry *) me;

    if (myself->groupid)
        JXTA_OBJECT_RELEASE(myself->groupid);
    if (myself->pve)
        JXTA_OBJECT_RELEASE(myself->pve);
    memset(myself, 0xdd, sizeof(Jxta_pv_ping_msg_group_entry));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peerview Ping Group Entry FREE [%pp]\n", myself );

    free(myself);
}

JXTA_DECLARE(Jxta_pv_ping_msg_group_entry *) jxta_pv_ping_msg_entry_new(Jxta_peer *peer, apr_uuid_t peer_adv_gen, Jxta_boolean peer_adv_gen_set, const char *group, Jxta_boolean pv_id_only)
{
    Jxta_pv_ping_msg_group_entry *entry;

    entry = calloc(1, sizeof(Jxta_pv_ping_msg_group_entry));

    JXTA_OBJECT_INIT(entry, pv_group_entry_free, NULL);

    entry->groupid = jstring_new_2(group);
    entry->pv_id_only = pv_id_only;
    entry->peer_adv_gen_set = peer_adv_gen_set;
    entry->peer_adv_gen = peer_adv_gen;
    entry->pve = NULL != peer ? JXTA_OBJECT_SHARE(peer):NULL;

    return entry;
}

static void handle_group(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) me;
    apr_uuid_t peer_adv_gen;
    const char **atts = ((Jxta_advertisement *) myself)->atts;
    Jxta_boolean pv_id_only=FALSE;
    Jxta_boolean peer_adv_gen_set=FALSE;
    JString *value=NULL;

    JXTA_OBJECT_CHECK_VALID(myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Group> : [%pp]\n", myself);

    /** handle attributes */
    while (atts && *atts) {
        if (0 == strcmp(*atts, "pv_id_only")) {
            pv_id_only = (0 == strcmp(atts[1], "yes")) ? TRUE:FALSE;
        } else if (0 == strcmp(*atts, "adv_gen")) {
            peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &peer_adv_gen, atts[1]));
            if( !peer_adv_gen_set ) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                            FILEANDLINE "UUID parse failure for group adv_gen [%pp] : %s\n", myself, atts[1]);
            }
        } else if (0 == strcmp(*atts, "id")) {
            Jxta_pv_ping_msg_group_entry *group_entry;
            value = jstring_new_2(atts[1]);

            if (jstring_length(value) > 0) {
                group_entry = jxta_pv_ping_msg_entry_new(NULL, peer_adv_gen, peer_adv_gen_set, jstring_get_string(value),  pv_id_only);
                jxta_vector_add_object_last(myself->groups, (Jxta_object *) group_entry);
                JXTA_OBJECT_RELEASE(group_entry);
            }
            JXTA_OBJECT_RELEASE(value);
        } else {
            /* just silently skip it. */
        }
        atts+=2;
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Group> : [%pp]\n", myself);
}

static void handle_option(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_ping_msg *myself = (Jxta_peerview_ping_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Option> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Option> : [%pp]\n", myself);
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
