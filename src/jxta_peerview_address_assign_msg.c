/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id$
 */

static const char *__log_cat = "PV_ADDR_ASSIGN";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"

#include "jxta_peerview_address_assign_msg.h"

const char JXTA_PEERVIEW_ADDRESS_ASSIGN_ELEMENT_NAME[] = "PeerviewAddressAssign";

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/
struct _Jxta_peerview_address_assign_msg {
    Jxta_advertisement jxta_advertisement;
   
    Jxta_id *peer_id;

    Jxta_credential *credential;   

    JString *instance_mask;    

    JString *target_hash;

    Jxta_vector *options;
};

static void peerview_address_assign_msg_delete(Jxta_object * me);
static void handle_peerview_address_assign_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_instance_mask(void *me, const XML_Char * cd, int len);
static void handle_target_hash(void *me, const XML_Char * cd, int len);
static void handle_option(void *me, const XML_Char * cd, int len);

static Jxta_status validate_message(Jxta_peerview_address_assign_msg * myself);

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    Jxta_peerview_address_assign_msg_,
    Credential_,
    InstanceMask_,
    TargetHash_,
    Adv_,
    Option_
};

/** 
 * Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, diffusion through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_peerview_address_assign_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PeerviewAddressAssign", Jxta_peerview_address_assign_msg_, *handle_peerview_address_assign_msg, NULL, NULL},
    {"Credential", Credential_, *handle_credential, NULL, NULL},
    {"InstanceMask", InstanceMask_, *handle_instance_mask, NULL, NULL},
    {"TargetHash", TargetHash_, *handle_target_hash, NULL, NULL},
    {"Option", Option_, *handle_option, NULL, NULL},
    
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_peerview_address_assign_msg *) jxta_peerview_address_assign_msg_new(void)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) calloc(1, sizeof(Jxta_peerview_address_assign_msg));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_address_assign_msg NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, peerview_address_assign_msg_delete, NULL);

    myself = (Jxta_peerview_address_assign_msg *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:PeerviewAddressAssign",
                                                                 jxta_peerview_address_assign_msg_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_peerview_address_assign_msg_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);

    if (NULL != myself) {
        myself->peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->credential = NULL;
        myself->instance_mask = NULL;
        myself->target_hash = NULL;
        myself->options = jxta_vector_new(0);
    }

    return myself;
}

static void peerview_address_assign_msg_delete(Jxta_object * me)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;

    JXTA_OBJECT_RELEASE(myself->peer_id);

    if (myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (myself->instance_mask) {
        JXTA_OBJECT_RELEASE(myself->instance_mask);
        myself->instance_mask = NULL;
    }

    if (myself->target_hash) {
        JXTA_OBJECT_RELEASE(myself->target_hash);
        myself->target_hash = NULL;
    }

    if (myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
        myself->options = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_peerview_address_assign_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_address_assign_msg FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_id *) jxta_peerview_address_assign_msg_get_peer_id(Jxta_peerview_address_assign_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->peer_id);
}

JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_peer_id(Jxta_peerview_address_assign_msg * myself, Jxta_id * peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(peer_id);

    JXTA_OBJECT_RELEASE(myself->peer_id);
    
    /* peer id may not be NULL. */
    myself->peer_id = JXTA_OBJECT_SHARE(peer_id);
}

JXTA_DECLARE(Jxta_credential *) jxta_peerview_address_assign_msg_get_credential(Jxta_peerview_address_assign_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_credential(Jxta_peerview_address_assign_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(const char *) jxta_peerview_address_assign_msg_get_instance_mask(Jxta_peerview_address_assign_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->instance_mask) {
        return jstring_get_string(myself->instance_mask);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_instance_mask(Jxta_peerview_address_assign_msg * myself, const char *instance_mask)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->instance_mask != NULL) {
        JXTA_OBJECT_RELEASE(myself->instance_mask);
        myself->instance_mask = NULL;
    }

    if (instance_mask != NULL) {
        myself->instance_mask = jstring_new_2(instance_mask);
        jstring_trim(myself->instance_mask);
    }
}

JXTA_DECLARE(const char *) jxta_peerview_address_assign_msg_get_target_hash(Jxta_peerview_address_assign_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->target_hash) {
        return jstring_get_string(myself->target_hash);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_address_assign_msg_set_target_hash(Jxta_peerview_address_assign_msg * myself, const char *target_hash)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->instance_mask != NULL) {
        JXTA_OBJECT_RELEASE(myself->target_hash);
        myself->target_hash = NULL;
    }

    if (target_hash != NULL) {
        myself->target_hash = jstring_new_2(target_hash);
        jstring_trim(myself->target_hash);
    }
}

JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_parse_charbuffer(Jxta_peerview_address_assign_msg * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_parse_file(Jxta_peerview_address_assign_msg * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_status validate_message(Jxta_peerview_address_assign_msg * myself) {

#if 0
    if ( NULL == myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    if ( NULL == myself->instance_mask ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Instance mask must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ( NULL == myself->target_hash  ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Target hash must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_address_assign_msg_get_xml(Jxta_peerview_address_assign_msg * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    JString *temp;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */
 
    JXTA_OBJECT_CHECK_VALID(myself);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    
    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:PeerviewAddressAssign ");
    jstring_append_2(string, " peer_id=\"");
    jxta_id_to_jstring(myself->peer_id, &temp);
    jstring_append_1(string, temp);
    JXTA_OBJECT_RELEASE(temp);
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");

    jstring_append_2(string, "<InstanceMask>");
    jstring_append_2(string, jxta_peerview_address_assign_msg_get_instance_mask(myself));
    jstring_append_2(string, "</InstanceMask>\n");
    
    jstring_append_2(string, "<TargetHash>\n");
    jstring_append_2(string, jxta_peerview_address_assign_msg_get_target_hash(myself));
    jstring_append_2(string, "</TargetHash>\n");

    /* FIXME Handle credentials and options */

    jstring_append_2(string, "</jxta:PeerviewAddressAssign>\n");

    *xml = string;
    
    return JXTA_SUCCESS;
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_peerview_address_assign_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PeerviewAddressAssign> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                
                JXTA_OBJECT_RELEASE(myself->peer_id);
                myself->peer_id = NULL;

                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    myself->peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }        
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PeerviewAddressAssign> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;
    
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

static void handle_instance_mask(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <InstanceMask> : [%pp]\n", myself);
    } else {
        JString *string = jstring_new_1(len );

        jstring_append_0( string, cd, len);
        jstring_trim(string);

        jxta_peerview_address_assign_msg_set_instance_mask(myself, jstring_get_string(string));

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <InstanceMask> : [%pp]\n", myself);
    }
}

static void handle_target_hash(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <TargetHash> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);

            atts += 2;
        }
    } else {
        JString *string = jstring_new_1(len );

        jstring_append_0( string, cd, len);
        jstring_trim(string);

        if( NULL != myself->target_hash ) {
            JXTA_OBJECT_RELEASE(myself->target_hash);
        }
        
        myself->target_hash = string;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <TargetHash> : [%pp]\n", myself);
    }
}

static void handle_option(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_address_assign_msg *myself = (Jxta_peerview_address_assign_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Option> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Option> : [%pp]\n", myself);
    }
}


/* vim: set ts=4 sw=4 et tw=130: */
