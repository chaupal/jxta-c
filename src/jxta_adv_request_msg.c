/*
 * Copyright (c) 2007 Sun Microsystems, Inc.  All rights reserved.
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

static const char *__log_cat = "AdvRequest";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_adv_request_msg.h"

const char JXTA_ADV_REQUEST_ELEMENT_NAME[] = "AdvRequestMsg";

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    PARequestMsg_,
    credential_,
    peerid_,
    search_
};




/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _Jxta_adv_request_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_credential * credential;
    Jxta_id *src_peer_id;
    char *query;
};

static void adv_request_msg_delete(Jxta_object * me);
static Jxta_adv_request_msg *adv_request_msg_construct(Jxta_adv_request_msg * myself);
static Jxta_status validate_message(Jxta_adv_request_msg * myself);
static void handle_adv_request_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_query(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_adv_request_msg *) jxta_adv_request_msg_new(void)
{
    Jxta_adv_request_msg *myself = (Jxta_adv_request_msg *) calloc(1, sizeof(Jxta_adv_request_msg));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, adv_request_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "AdvRequest NEW [%pp]\n", myself );

    return adv_request_msg_construct(myself);
}

static void adv_request_msg_delete(Jxta_object * me)
{
    Jxta_adv_request_msg *myself = (Jxta_adv_request_msg *) me;

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }
    if (myself->query)
        free(myself->query);
    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_adv_request_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "AdvRequest FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab adv_request_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:AdvRequest", PARequestMsg_, *handle_adv_request_msg, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {"Query", search_, *handle_query, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_adv_request_msg *adv_request_msg_construct(Jxta_adv_request_msg * myself)
{
    myself = (Jxta_adv_request_msg *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:AdvRequest",
                                     adv_request_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_adv_request_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->credential = NULL;
    }

    return myself;
}

static Jxta_status validate_message(Jxta_adv_request_msg * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->src_peer_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Source peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if (NULL == myself->query) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Query must not be NULL [%pp]\n", myself);
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

JXTA_DECLARE(Jxta_status) jxta_adv_request_msg_get_xml(Jxta_adv_request_msg * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    JString *tempstr;
    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<jxta:AdvRequest ");

    jstring_append_2(string, " src_id=\"");
    jxta_id_to_jstring(myself->src_peer_id, &tempstr);
    jstring_append_1(string, tempstr);
    JXTA_OBJECT_RELEASE(tempstr);
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n ");
    if (NULL != myself->query) {
        jstring_append_2(string, "<Query>");
        jstring_append_2(string, myself->query);
        jstring_append_2(string, "</Query>");
    }
    jstring_append_2(string, "</jxta:AdvRequest>\n");
    *xml = string;
    return JXTA_SUCCESS;
}


JXTA_DECLARE(Jxta_id *) jxta_adv_request_msg_get_src_peer_id(Jxta_adv_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_adv_request_msg_set_src_peer_id(Jxta_adv_request_msg * myself, Jxta_id * src_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(src_peer_id);
    if (NULL != myself->src_peer_id) {
        JXTA_OBJECT_RELEASE(myself->src_peer_id);
    }

    myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(Jxta_status) jxta_adv_request_msg_set_query(Jxta_adv_request_msg * myself, const char *query, int threshold)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    if (NULL == query) return JXTA_INVALID_ARGUMENT;
    if (NULL != myself->query) {
        free(myself->query);
    }
    myself->query = strdup(query);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_adv_request_msg_get_query(Jxta_adv_request_msg * myself, char **query)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    if (NULL == query) return JXTA_INVALID_ARGUMENT;

    if (NULL != myself->query) {
        *query = strdup(myself->query);
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_credential *) jxta_adv_request_msg_get_credential(Jxta_adv_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_adv_request_msg_set_credential(Jxta_adv_request_msg * myself, Jxta_credential *credential)
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

static void handle_adv_request_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_request_msg *myself = (Jxta_adv_request_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:AdvRequest> : [%pp]\n", myself);

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
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:AdvRequest> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_request_msg *myself = (Jxta_adv_request_msg *) me;
    
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

static void handle_query(void *me, const XML_Char * cd, int len)
{
    Jxta_adv_request_msg *myself = (Jxta_adv_request_msg *) me;
    JXTA_OBJECT_CHECK_VALID(myself);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <query> : [%pp]\n", myself);
    if (len > 0) {
        if (myself->query != NULL) {
            free(myself->query);
        }
        myself->query = strdup(cd);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <query> : [%pp]\n", myself);
}
/* vim: set ts=4 sw=4 et tw=130: */
