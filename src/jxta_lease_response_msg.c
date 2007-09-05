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

static const char *__log_cat = "LeaseResponse";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_cred.h"

#include "jxta_lease_response_msg.h"

const char JXTA_LEASE_RESPONSE_ELEMENT_NAME[] = "LeaseResponseMsg";

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    LeaseResponseMsg_,
    credential_,
    server_adv_,
    referral_adv_,
    option_
};

struct _Jxta_lease_adv_info {
    JXTA_OBJECT_HANDLE;

    Jxta_PA *adv;
    Jxta_boolean adv_gen_set;
    apr_uuid_t adv_gen;
    Jxta_time_diff adv_exp;
};

static void lease_adv_info_delete(Jxta_object * me);
static Jxta_lease_adv_info *lease_adv_info_new(Jxta_PA * adv, apr_uuid_t const * adv_gen, Jxta_time_diff adv_exp);
static Jxta_status validate_message(Jxta_lease_response_msg * myself);

/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _Jxta_lease_response_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *server_id;
    Jxta_time_diff offered_lease;

    Jxta_credential * credential;

    Jxta_lease_adv_info *server;

    Jxta_vector *referrals;

    Jxta_vector * options;
};

static Jxta_lease_response_msg *lease_response_msg_construct(Jxta_lease_response_msg * myself);
static void lease_response_msg_delete(Jxta_object * me);


/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

void handle_lease_response_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:LeaseResponse> : [%pp]\n", myself);
        
        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "server_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                
                JXTA_OBJECT_RELEASE(myself->server_id);

                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->server_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for jxta:LeaseResponseMsg [%pp]\n", myself);
                    myself->server_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "offered_lease")) {
                myself->offered_lease = atol(atts[1]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:LeaseResponse> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;
    
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

static void handle_server_adv(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_PA *server_adv;
        apr_uuid_t server_adv_gen;
        Jxta_boolean server_adv_gen_set = FALSE;
        Jxta_time_diff server_adv_exp = -1;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <ServerAdv> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "adv_gen")) {
                if (APR_SUCCESS != apr_uuid_parse(&server_adv_gen, atts[1])) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "UUID parse failure for ServerAdv [%pp] : %s\n", myself, atts[1]);
                    server_adv_gen_set = FALSE;
                } else {
                    server_adv_gen_set = TRUE;
                }
            } else if (0 == strcmp(*atts, "expiration")) {
                server_adv_exp = atol(atts[1]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }

        server_adv = jxta_PA_new();

        if (NULL != myself->server) {
            JXTA_OBJECT_RELEASE(myself->server);
            myself->server = NULL;
        }

        myself->server = lease_adv_info_new(server_adv, server_adv_gen_set ? &server_adv_gen : NULL, server_adv_exp);
        JXTA_OBJECT_RELEASE(server_adv);

        jxta_advertisement_set_handlers((Jxta_advertisement *) server_adv, ((Jxta_advertisement *) myself)->parser, (void *) myself);
   } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <ServerAdv> : [%pp]\n", myself);
   }
}

static void handle_referral_adv(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_PA *referral_adv;
        Jxta_lease_adv_info *referral_info;
        apr_uuid_t referral_adv_gen;
        Jxta_boolean referral_adv_gen_set = FALSE;
        Jxta_time_diff referral_adv_exp = -1;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <ReferralAdv> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "adv_gen")) {
                if (APR_SUCCESS != apr_uuid_parse(&referral_adv_gen, atts[1])) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "UUID parse failure for ReferralAdv [%pp] : %s\n", myself, atts[1]);
                    referral_adv_gen_set = FALSE;
                } else {
                    referral_adv_gen_set = TRUE;
                }
            } else if (0 == strcmp(*atts, "expiration")) {
                referral_adv_exp = atol(atts[1]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }

        referral_adv = jxta_PA_new();
        referral_info = lease_adv_info_new(referral_adv, NULL, referral_adv_exp);
        JXTA_OBJECT_RELEASE(referral_adv);

        jxta_vector_add_object_last(myself->referrals, (Jxta_object *) referral_info);
        JXTA_OBJECT_RELEASE(referral_info);

        jxta_advertisement_set_handlers((Jxta_advertisement *) referral_adv, ((Jxta_advertisement *) myself)->parser,
                                        (void *) myself);
   } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <ReferralAdv> : [%pp]\n", myself);
   }
}

static void handle_option(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Option> : [%pp]\n", myself);
#if 0
        jxta_advertisement_set_handlers((Jxta_advertisement *) server_adv, ((Jxta_advertisement *) myself)->parser, (void *) myself);

        ((Jxta_advertisement *) server_adv)->atts = ((Jxta_advertisement *) myself)->atts;
#endif
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Option> : [%pp]\n", myself);
    }
}


    /** 
     * Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab lease_response_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:LeaseResponse", LeaseResponseMsg_, *handle_lease_response_msg, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {"ServerAdv", server_adv_, *handle_server_adv, NULL, NULL},
    {"ReferralAdv", referral_adv_, *handle_referral_adv, NULL, NULL},
    {"Option", option_, *handle_option, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_get_xml(Jxta_lease_response_msg * myself, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *tempstr=NULL;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */
    unsigned int each_referral;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:LeaseResponse>\n");

    jstring_append_2(string, "<jxta:LeaseResponse");

    jstring_append_2(string, " server_id=\"");
    jxta_id_to_jstring(myself->server_id, &tempstr);
    jstring_append_1(string, tempstr);
    JXTA_OBJECT_RELEASE(tempstr);
    tempstr = NULL;
    jstring_append_2(string, "\"");

    if (-1L != myself->offered_lease) {
        jstring_append_2(string, " offered_lease=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, myself->offered_lease);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    jstring_append_2(string, ">\n ");

    if (NULL != myself->server) {
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;
        apr_uuid_t *adv_gen = jxta_lease_adv_info_get_adv_gen(myself->server);
        Jxta_PA *server_adv;

        if (NULL != adv_gen) {
            attrs[attr_idx++] = "adv_gen";

            apr_uuid_format(tmpbuf, adv_gen);
            free(adv_gen);
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }

        if (-1L != jxta_lease_adv_info_get_adv_exp(myself->server)) {
            attrs[attr_idx++] = "expiration";

            apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, jxta_lease_adv_info_get_adv_exp(myself->server));
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }

        attrs[attr_idx] = NULL;

        server_adv = jxta_lease_adv_info_get_adv(myself->server);

        jxta_PA_get_xml_1(server_adv, &tempstr, "ServerAdv", attrs);
        JXTA_OBJECT_RELEASE(server_adv);

        for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
            if (free_mask & (1 << attr_idx)) {
                free( (void*) attrs[attr_idx]);
            }
        }
        if (tempstr) {
            jstring_append_1(string, tempstr);
            JXTA_OBJECT_RELEASE(tempstr);
            tempstr = NULL;
        }
    }

    for (each_referral = 0; each_referral < jxta_vector_size(myself->referrals); each_referral++) {
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;
        Jxta_lease_adv_info *referral;

        res = jxta_vector_get_object_at(myself->referrals, JXTA_OBJECT_PPTR(& referral), each_referral);

        if (JXTA_SUCCESS == res) {
            Jxta_PA *adv = jxta_lease_adv_info_get_adv(referral);

            if (-1L != jxta_lease_adv_info_get_adv_exp(referral)) {
                attrs[attr_idx++] = "expiration";

                apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, jxta_lease_adv_info_get_adv_exp(referral));
                free_mask |= (1 << attr_idx);
                attrs[attr_idx++] = strdup(tmpbuf);
            }

            attrs[attr_idx] = NULL;

            jxta_PA_get_xml_1(adv, &tempstr, "ReferralAdv", attrs);
            JXTA_OBJECT_RELEASE(adv);

            for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
                if (free_mask & (1 << attr_idx)) {
                    free( (void*) attrs[attr_idx]);
                }
            }
            if (tempstr) {
                jstring_append_1(string, tempstr);
                JXTA_OBJECT_RELEASE(tempstr);
                tempstr = NULL;
            }

            JXTA_OBJECT_RELEASE(referral);
        }

    }

    jstring_append_2(string, "</jxta:LeaseResponse>\n");
    if (tempstr)
        JXTA_OBJECT_RELEASE(tempstr);
    *xml = string;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_id *) jxta_lease_response_msg_get_server_id(Jxta_lease_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->server_id);
}

JXTA_DECLARE(void) jxta_lease_response_msg_set_server_id(Jxta_lease_response_msg * myself, Jxta_id * server_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(server_id);

    JXTA_OBJECT_RELEASE(myself->server_id);

    /* Server ID may not be NULL.*/
    myself->server_id = JXTA_OBJECT_SHARE(server_id);
}

JXTA_DECLARE(Jxta_time_diff) jxta_lease_response_msg_get_offered_lease(Jxta_lease_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->offered_lease;
}

JXTA_DECLARE(void) jxta_lease_response_msg_set_offered_lease(Jxta_lease_response_msg * myself,
                                                                 Jxta_time_diff offered_lease)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->offered_lease = offered_lease;
}

JXTA_DECLARE(Jxta_credential *) jxta_lease_response_msg_get_credential(Jxta_lease_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_lease_response_msg_set_credential(Jxta_lease_response_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(Jxta_lease_adv_info *) jxta_lease_response_msg_get_server_adv_info(Jxta_lease_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->server) {
        return JXTA_OBJECT_SHARE(myself->server);
    } else {
        return NULL;
    }

}

JXTA_DECLARE(void) jxta_lease_response_msg_set_server_adv_info(Jxta_lease_response_msg * myself, Jxta_PA * adv,
                                                                   apr_uuid_t const * gen, Jxta_time_diff exp)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->server) {
        JXTA_OBJECT_RELEASE(myself->server);
        myself->server = NULL;
    }

    if (NULL != adv) {
        myself->server = lease_adv_info_new(adv, gen, exp);
    }
}

JXTA_DECLARE(Jxta_vector *) jxta_lease_response_msg_get_referral_advs(Jxta_lease_response_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->referrals) {
        return JXTA_OBJECT_SHARE(myself->referrals);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_lease_response_msg_set_referral_advs(Jxta_lease_response_msg * myself,
                                                                 Jxta_vector * referral_advs)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->referrals) {
        JXTA_OBJECT_RELEASE(myself->referrals);
        myself->referrals = NULL;
    }

    if (NULL != referral_advs) {
        myself->referrals = JXTA_OBJECT_SHARE(referral_advs);
    }
}

JXTA_DECLARE(void) jxta_lease_response_msg_add_referral_adv(Jxta_lease_response_msg * myself,
                                                                 Jxta_PA * adv, Jxta_time_diff exp )
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL == myself->referrals) {
        myself->referrals = jxta_vector_new(1);
    }

    if (NULL != adv) {
        Jxta_lease_adv_info * referral = lease_adv_info_new(adv, NULL, exp);
        jxta_vector_add_object_last(myself->referrals, (Jxta_object *) referral);
        JXTA_OBJECT_RELEASE(referral);
    }
}

    /** 
    * Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_lease_response_msg *) jxta_lease_response_msg_new(void)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) calloc(1, sizeof(Jxta_lease_response_msg));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, lease_response_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "LeaseResponse NEW [%pp]\n", myself );

    return lease_response_msg_construct(myself);
}

static void lease_response_msg_delete(Jxta_object * me)
{
    Jxta_lease_response_msg *myself = (Jxta_lease_response_msg *) me;

    /* Fill in the required freeing functions here. */
    if (NULL != myself->server_id) {
        JXTA_OBJECT_RELEASE(myself->server_id);
    }

    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
    }

    if (NULL != myself->server) {
        JXTA_OBJECT_RELEASE(myself->server);
    }

    if (NULL != myself->referrals) {
        JXTA_OBJECT_RELEASE(myself->referrals);
    }

    if (NULL != myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_lease_response_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "LeaseResponse FREE [%pp]\n", myself );

    free(myself);
}

static Jxta_lease_response_msg *lease_response_msg_construct(Jxta_lease_response_msg * myself)
{
    myself = (Jxta_lease_response_msg *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:LeaseResponse",
                                     lease_response_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_lease_response_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->server_id = NULL;
        myself->offered_lease = -1L;
        myself->credential = NULL;
        myself->server = NULL;
        myself->referrals = jxta_vector_new(1);
        myself->options = jxta_vector_new(1);
    }

    return myself;
}

static Jxta_status validate_message(Jxta_lease_response_msg * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->server_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Server peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

#if 0 
    /* FIXME 20060827 bondolo Credential processing not yet implemented. */
    if ( NULL != myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    if ( (NULL != myself->server) && (NULL != myself->server->adv) ) {
        Jxta_id * peer_id = jxta_PA_get_PID(myself->server->adv);
        Jxta_boolean same = jxta_id_equals( myself->server_id, peer_id);
        
        if( NULL != peer_id ) {
            JXTA_OBJECT_RELEASE(peer_id);
        }
        
        if( !same ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Peer id of Advertisement must match server_id [%pp].\n", myself);
            return JXTA_INVALID_ARGUMENT;
        }
    }

    if ( myself->offered_lease < -1L ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Offered lease must be >= -1 [%pp].\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_parse_charbuffer(Jxta_lease_response_msg * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_lease_response_msg_parse_file(Jxta_lease_response_msg * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_lease_adv_info *lease_adv_info_new(Jxta_PA * adv, apr_uuid_t const * adv_gen, Jxta_time_diff adv_exp)
{
    Jxta_lease_adv_info *myself = (Jxta_lease_adv_info *) calloc(1, sizeof(Jxta_lease_adv_info));

    if (NULL == myself)
        return NULL;

    JXTA_OBJECT_INIT(myself, lease_adv_info_delete, NULL);

    myself->adv = JXTA_OBJECT_SHARE(adv);
    if (NULL != adv_gen) {
        myself->adv_gen = *adv_gen;
        myself->adv_gen_set = TRUE;
    } else {
        myself->adv_gen_set = FALSE;
    }
    
    myself->adv_exp = adv_exp;
    
    return myself;
}

static void lease_adv_info_delete(Jxta_object * me)
{
    Jxta_lease_adv_info *myself = (Jxta_lease_adv_info *) me;

    JXTA_OBJECT_RELEASE(myself->adv);
    myself->adv = NULL;

    memset(myself, 0xdd, sizeof(Jxta_lease_adv_info));
    free(myself);
}

JXTA_DECLARE(Jxta_PA *) jxta_lease_adv_info_get_adv(Jxta_lease_adv_info * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->adv);
}

JXTA_DECLARE(apr_uuid_t *) jxta_lease_adv_info_get_adv_gen(Jxta_lease_adv_info * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->adv_gen_set) {
        apr_uuid_t *result = (apr_uuid_t *) calloc(1, sizeof(apr_uuid_t));

        if (NULL != result) {
            *result = myself->adv_gen;
        }

        return result;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(Jxta_time_diff) jxta_lease_adv_info_get_adv_exp(Jxta_lease_adv_info * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->adv_exp;
}

/* vim: set ts=4 sw=4 et tw=130: */
