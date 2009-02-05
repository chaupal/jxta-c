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
 * $Id: jxta_monitor_entry.c 207 2008-01-23 19:31:17Z ExocetRick $
 */

static const char *__log_cat = "MONITOR_ENTRY";

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_peer.h"
#include "jxta_monitor_service.h"


struct _jxta_monitor_entry {
    Jxta_advertisement advertisement;
    Jxta_credential *credential;
    char *peer_id;
    char *context;
    char *sub_context;
    char *type;
    Jxta_time time_stamp;
    Jxta_advertisement * adv;
 };

static void monitor_entry_delete(Jxta_object * me);
static void handle_monitor_entry(void *me, const XML_Char * cd, int len);
static Jxta_status validate_message(Jxta_monitor_entry * myself);

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    Jxta_monitor_entry_,
    Event_
};

/** 
 * Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, diffusion through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_monitor_entry_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:MonEntry", Jxta_monitor_entry_, *handle_monitor_entry, NULL, NULL},
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_monitor_entry *) jxta_monitor_entry_new()
{
    Jxta_monitor_entry *myself = (Jxta_monitor_entry *) calloc(1, sizeof(Jxta_monitor_entry));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_monitor_entry NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, monitor_entry_delete, NULL);

    myself = (Jxta_monitor_entry *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:MonEntry",
                                                                 jxta_monitor_entry_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_monitor_entry_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);

    if (NULL != myself) {
        myself->credential = NULL;
        myself->peer_id = NULL;
        myself->context = NULL;
        myself->sub_context = NULL;
    }

    return myself;
}

JXTA_DECLARE(Jxta_monitor_entry *) jxta_monitor_entry_new_1(Jxta_id *context, Jxta_id *sub_context, Jxta_advertisement * adv)
{
    Jxta_monitor_entry * entry;

    JXTA_OBJECT_CHECK_VALID(sub_context);
    JXTA_OBJECT_CHECK_VALID(context);
    JXTA_OBJECT_CHECK_VALID(adv);

    entry = jxta_monitor_entry_new();
    entry->context = JXTA_OBJECT_SHARE(context);
    entry->sub_context = JXTA_OBJECT_SHARE(sub_context);
    entry->adv = JXTA_OBJECT_SHARE(adv);
    return entry;
}

static void monitor_entry_delete(Jxta_object * me)
{
    Jxta_monitor_entry *myself = (Jxta_monitor_entry *) me;

    if (myself->credential)
        JXTA_OBJECT_RELEASE(myself->credential);

    if (myself->adv)
        JXTA_OBJECT_RELEASE(myself->adv);

    if (NULL != myself->peer_id) {
        free(myself->peer_id);
    }
    if (NULL != myself->context) {
        free(myself->context);
    }
    if (NULL != myself->sub_context) {
        free(myself->sub_context);
    }
    if (NULL != myself->type) {
        free(myself->type);
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_monitor_entry));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_monitor_entry FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_context(Jxta_monitor_entry * myself, char **context)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != context);

    *context = strdup(myself->context);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_context(Jxta_monitor_entry * myself, const char * context)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != context);

    if (myself->context) {
        free(myself->context);
    }
    myself->context = strdup(context);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_sub_context(Jxta_monitor_entry * myself, char **sub_context)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != sub_context);

    *sub_context = strdup(myself->sub_context);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_sub_context(Jxta_monitor_entry * myself, const char * sub_context)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != sub_context);

    if (myself->sub_context) {
        free(myself->sub_context);
    }

    myself->sub_context = strdup(sub_context);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_type(Jxta_monitor_entry * myself, char **type)
{
    assert(NULL != type);
    JXTA_OBJECT_CHECK_VALID(myself);

    *type = strdup(myself->type);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_type(Jxta_monitor_entry * myself, const char * type)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != type);

    if (myself->type) {
        free(myself->type);
    }

    myself->type = strdup(type);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_set_entry(Jxta_monitor_entry * myself, Jxta_advertisement * adv)
{
    Jxta_status res = JXTA_SUCCESS;
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(adv);
    if (myself->adv) {
        JXTA_OBJECT_RELEASE(myself->adv);
        myself->adv = NULL;
    }
    myself->adv = JXTA_OBJECT_SHARE(adv);
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_entry(Jxta_monitor_entry * myself, Jxta_advertisement **adv)
{

    JXTA_OBJECT_CHECK_VALID(myself);
    assert(NULL != adv);

    if (myself->adv) {
        *adv = JXTA_OBJECT_SHARE(myself->adv);
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_parse_charbuffer(Jxta_monitor_entry * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_parse_file(Jxta_monitor_entry * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_status validate_message(Jxta_monitor_entry * myself) {

/* 
    if ( (NULL == myself->peer_id) || jxta_id_equals(myself->peer_id, jxta_id_nullID) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
*/
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_monitor_entry_get_xml(Jxta_monitor_entry * myself, JString ** xml)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *string;
    JString *temp = NULL;
 
    JXTA_OBJECT_CHECK_VALID(myself);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    
    /* res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    } */
    
    string = jstring_new_0();

    jstring_append_2(string, "<Entry type=\"");
    jstring_append_2(string, myself->type);
    jstring_append_2(string, "\">");


    res = jxta_advertisement_get_xml( myself->adv, &temp );
    if( JXTA_SUCCESS == res ) {
        JString *tmp1;
        jxta_xml_util_encode_jstring(temp, &tmp1);
        jstring_append_1(string, tmp1);
        JXTA_OBJECT_RELEASE(tmp1);
    }
    if (NULL != temp) {
        JXTA_OBJECT_RELEASE(temp);
    }
    jstring_append_2(string, "</Entry>\n");

    *xml = string;

    return JXTA_SUCCESS;
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_monitor_entry(void *me, const XML_Char * cd, int len)
{
    Jxta_monitor_entry *myself = (Jxta_monitor_entry *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:MonEntry> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
                myself->type = strdup(atts[1]);
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:MonEntry> : [%pp]\n", myself);
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
