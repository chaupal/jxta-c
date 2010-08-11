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

static const char *__log_cat = "PV3OptionEntry";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_peerview_option_entry.h"

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    PARequestMsg_,
    handle_entry_,
    credential_,
    peerid_,
    search_
};

struct _jxta_peerview_option_entry {
    Jxta_advertisement advertisement;

    Jxta_peerview_option_entry *entry;
    Jxta_credential * credential;
    Jxta_hashtable *entries;
};


static void peerview_option_entry_delete(Jxta_object * me);
static Jxta_peerview_option_entry *peerview_option_entry_construct(Jxta_peerview_option_entry * myself);
static Jxta_status validate_message(Jxta_peerview_option_entry * myself);
static void handle_peerview_option_entry(void *me, const XML_Char * cd, int len);
static void handle_entry(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_peerview_option_entry *) jxta_peerview_option_entry_new(void)
{
    Jxta_peerview_option_entry *myself = (Jxta_peerview_option_entry *) calloc(1, sizeof(Jxta_peerview_option_entry));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, peerview_option_entry_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PV3OptionEntry NEW [%pp]\n", myself );

    return peerview_option_entry_construct(myself);
}

static void peerview_option_entry_delete(Jxta_object * me)
{
    Jxta_peerview_option_entry *myself = (Jxta_peerview_option_entry *) me;

    if (myself->entries)
        JXTA_OBJECT_RELEASE(myself->entries);
    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_peerview_option_entry));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PV3OptionEntry FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab peerview_option_entry_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PV3OptionEntry", PARequestMsg_, *handle_peerview_option_entry, NULL, NULL},
    {"Entry", handle_entry_, *handle_entry, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_peerview_option_entry *peerview_option_entry_construct(Jxta_peerview_option_entry * myself)
{
    myself = (Jxta_peerview_option_entry *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:PV3OptionEntry",
                                     peerview_option_entry_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_peerview_option_entry_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->entries = jxta_hashtable_new(0);
        myself->credential = NULL;
    }

    return myself;
}

static Jxta_status validate_message(Jxta_peerview_option_entry * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);


#if 0 
    /* FIXME 20060827 bondolo Credential processing not yet implemented. */
    if ( NULL != myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_option_entry_get_xml(Jxta_peerview_option_entry * myself, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    char ** entries;
    char ** entries_save;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    string = jstring_new_0();

/*     jstring_append_2(string, "<jxta:PV3OptionEntry>"); */
    entries = jxta_hashtable_keys_get(myself->entries);
    entries_save = entries;
    if (NULL != *entries) {
        jstring_append_2(string, "<Option type=\"jxta:PV3OptionEntry\">");
    }
    while (*entries) {
        JString *value_j;
        JString *entry_j;
        JString *entry_encode_j;

        jxta_hashtable_get(myself->entries, *entries, strlen(*entries) + 1, JXTA_OBJECT_PPTR(&value_j));

        entry_j = jstring_new_2( "<Entry name=\"");
        jstring_append_2(entry_j, *entries);
        jstring_append_2(entry_j, "\"");
        jstring_append_2(entry_j, " value=\"");
        jstring_append_1(entry_j, value_j);
        jstring_append_2(entry_j, "\"/>");
        jxta_xml_util_encode_jstring(entry_j, &entry_encode_j);
        jstring_append_1(string, entry_encode_j);

        free(*entries);
        entries++;
        JXTA_OBJECT_RELEASE(entry_encode_j);
        JXTA_OBJECT_RELEASE(entry_j);
        JXTA_OBJECT_RELEASE(value_j);
    }
    if (*entries_save) {
        jstring_append_2(string, "</Option>");
    }
    free(entries_save);
/*    jstring_append_2(string, "</jxta:PV3OptionEntry>\n"); */
    *xml = string;
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_option_entry_get_value(Jxta_peerview_option_entry * me, const char *name, JString **value)
{
    Jxta_status res;
    JString *value_j=NULL;

    res = jxta_hashtable_get(me->entries, name, strlen(name) + 1, JXTA_OBJECT_PPTR(&value_j));
    if (JXTA_SUCCESS == res) {
        *value = value_j;
    }
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_option_entry_set_value(Jxta_peerview_option_entry * me, const char *name, JString *value, Jxta_boolean replace)
{
    Jxta_status res;

    res = jxta_hashtable_contains(me->entries, name, strlen(name) + 1);
    if (JXTA_SUCCESS == res) {
        if (replace) {
            jxta_hashtable_replace(me->entries, name, strlen(name) + 1, (Jxta_object *) value, NULL);
        }
    } else {
        jxta_hashtable_put(me->entries, name, strlen(name) + 1, (Jxta_object *) value);
    }
    return res;
}

static void handle_peerview_option_entry(void *me, const XML_Char * cd, int len)
{

}

static void handle_entry(void *me, const XML_Char * cd, int len)
{

}

static void handle_credential(void *me, const XML_Char * cd, int len)
{

}

