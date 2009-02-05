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

static const char *__log_cat = "PV3MonEntry";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_monitor_service.h"
#include "jxta_peerview_monitor_entry.h"

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

struct _jxta_peerview_monitor_entry {
    Jxta_advertisement advertisement;

    Jxta_monitor_entry *entry;
    Jxta_credential * credential;
    Jxta_id *src_peer_id;

    JString *pong_msg_j;
    int cluster_number;
    int cluster_size;
    Jxta_time uptime;
    Jxta_time rdv_time;
    Jxta_time pv_time;
    int pv_instance_switches;
    int rdv_switches;
    int promoted_invitations;
    int promoted_edges;
    RdvConfig_configuration rdv_config;
};


static void peerview_monitor_entry_delete(Jxta_object * me);
static Jxta_peerview_monitor_entry *peerview_monitor_entry_construct(Jxta_peerview_monitor_entry * myself);
static Jxta_status validate_message(Jxta_peerview_monitor_entry * myself);
static void handle_peerview_monitor_entry(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_pong(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_peerview_monitor_entry *) jxta_peerview_monitor_entry_new(void)
{
    Jxta_peerview_monitor_entry *myself = (Jxta_peerview_monitor_entry *) calloc(1, sizeof(Jxta_peerview_monitor_entry));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, peerview_monitor_entry_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PV3MonEntry NEW [%pp]\n", myself );

    return peerview_monitor_entry_construct(myself);
}

static void peerview_monitor_entry_delete(Jxta_object * me)
{
    Jxta_peerview_monitor_entry *myself = (Jxta_peerview_monitor_entry *) me;

    JXTA_OBJECT_RELEASE(myself->src_peer_id);
    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }
    if (myself->pong_msg_j)
        JXTA_OBJECT_RELEASE(myself->pong_msg_j);
    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_peerview_monitor_entry));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "PV3MonEntry FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab peerview_monitor_entry_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PV3MonEntry", PARequestMsg_, *handle_peerview_monitor_entry, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {"PongMsg", search_, *handle_pong, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_peerview_monitor_entry *peerview_monitor_entry_construct(Jxta_peerview_monitor_entry * myself)
{
    myself = (Jxta_peerview_monitor_entry *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:PV3MonEntry",
                                     peerview_monitor_entry_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_peerview_monitor_entry_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->credential = NULL;
    }

    return myself;
}

static Jxta_status validate_message(Jxta_peerview_monitor_entry * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->src_peer_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Source peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if (NULL == myself->pong_msg_j) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Pong must not be NULL [%pp]\n", myself);
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

JXTA_DECLARE(Jxta_status) jxta_peerview_monitor_entry_get_xml(Jxta_peerview_monitor_entry * myself, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *tempstr;
    char tmpbuf[256];

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    /* res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    */
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:PV3MonEntry ");

    jstring_append_2(string, " src_id=\"");
    jxta_id_to_jstring(myself->src_peer_id, &tempstr);
    jstring_append_1(string, tempstr);
    jstring_append_2(string, "\"");
    JXTA_OBJECT_RELEASE(tempstr);

    apr_snprintf(tmpbuf, sizeof(tmpbuf), " cluster_number=\"%d\"", myself->cluster_number);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " cluster_size=\"%d\"", myself->cluster_size);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " uptime=\"%" APR_INT64_T_FMT "\"", myself->uptime);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " rdv_time=\"%" APR_INT64_T_FMT "\"", myself->rdv_time);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " pv_time=\"%" APR_INT64_T_FMT "\"", myself->pv_time);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " pv_instance_switches=\"%d\"", myself->pv_instance_switches);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " rdv_switches=\"%d\"", myself->rdv_switches);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " promoted_invitations=\"%d\"", myself->promoted_invitations);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " promoted_edges=\"%d\"", myself->promoted_edges);
    jstring_append_2(string, tmpbuf);
    apr_snprintf(tmpbuf, sizeof(tmpbuf), " rdv_config=\"%d\"", myself->rdv_config);
    jstring_append_2(string, tmpbuf);

    jstring_append_2(string, ">\n ");
    if (NULL != myself->pong_msg_j) {
        JString *tmp;
        jstring_append_2(string, "<PongMsg>");
        jxta_xml_util_encode_jstring(myself->pong_msg_j, &tmp);
        jstring_append_1(string, tmp);
        jstring_append_2(string, "</PongMsg>");
        JXTA_OBJECT_RELEASE(tmp);
    }
    jstring_append_2(string, "</jxta:PV3MonEntry>\n");
    *xml = string;
    return res;
}


JXTA_DECLARE(Jxta_id *) jxta_peerview_monitor_entry_get_src_peer_id(Jxta_peerview_monitor_entry * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_peerview_monitor_entry_set_src_peer_id(Jxta_peerview_monitor_entry * myself, Jxta_id * src_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(src_peer_id);
    if (NULL != myself->src_peer_id) {
        JXTA_OBJECT_RELEASE(myself->src_peer_id);
    }

    myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(int) jxta_peerview_monitor_entry_get_cluster_number(Jxta_peerview_monitor_entry * me) {
    return me->cluster_number;
}

JXTA_DECLARE(void) jxta_peerview_monitor_entry_set_cluster_number(Jxta_peerview_monitor_entry * me, int cluster_number) {
    me->cluster_number = cluster_number;
}

JXTA_DECLARE(Jxta_time) jxta_peerview_monitor_entry_get_rdv_time(Jxta_peerview_monitor_entry * me) {
    return me->rdv_time;
}

JXTA_DECLARE(void) jxta_peerview_monitor_entry_set_rdv_time(Jxta_peerview_monitor_entry * me, Jxta_time time) {
    me->rdv_time = time;
}

JXTA_DECLARE(void) jxta_peerview_monitor_entry_set_pong_msg(Jxta_peerview_monitor_entry * me, Jxta_peerview_pong_msg * pong)
{
    if (NULL != me->pong_msg_j) {
        JXTA_OBJECT_RELEASE(me->pong_msg_j);
    }
    jxta_peerview_pong_msg_get_xml(pong, &me->pong_msg_j);
}

JXTA_DECLARE(void) jxta_peerview_monitor_entry_get_pong_msg(Jxta_peerview_monitor_entry * me, Jxta_peerview_pong_msg ** pong)
{
    if (NULL != me->pong_msg_j) {
        *pong = jxta_peerview_pong_msg_new();
        jxta_peerview_pong_msg_parse_charbuffer(*pong, jstring_get_string(me->pong_msg_j), jstring_length(me->pong_msg_j));
    } else {
        *pong = NULL;
    }

}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_peerview_monitor_entry(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_monitor_entry *myself = (Jxta_peerview_monitor_entry *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PV3MonEntry> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "cluster_number")) {
                myself->cluster_number = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "cluster_size")) {
                myself->cluster_size = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "uptime")) {
                myself->uptime = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "rdv_time")) {
               myself->rdv_time = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "pv_time")) {
               myself->pv_time = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "pv_instance_switches")) {
               myself->pv_instance_switches = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "rdv_switches")) {
               myself->rdv_switches = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "promoted_invitations")) {
               myself->promoted_invitations = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "promoted_edges")) {
               myself->promoted_edges = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "rdv_config")) {
               myself->rdv_config = atoi(atts[1]);
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
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PV3MonEntry> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_monitor_entry *myself = (Jxta_peerview_monitor_entry *) me;
    
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

static void handle_pong(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_monitor_entry *myself = (Jxta_peerview_monitor_entry *) me;
    JXTA_OBJECT_CHECK_VALID(myself);
    if (0 == len) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <PongMsg> : [%pp]\n", myself);
    } else {
        if (myself->pong_msg_j != NULL) {
            JXTA_OBJECT_RELEASE(myself->pong_msg_j);
        }
        myself->pong_msg_j = jstring_new_2(cd);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <PongMsg> : [%pp]\n", myself);
    }
}
/* vim: set ts=4 sw=4 et tw=130: */
