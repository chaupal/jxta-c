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

static const char *__log_cat = "LeaseRequest";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"

#include "jxta_lease_request_msg.h"

const char JXTA_LEASE_REQUEST_ELEMENT_NAME[] = "LeaseRequestMsg";

/** 
*   Each of these corresponds to a tag in the xml.
*/
enum tokentype {
    Null_,
    LeaseRequestMsg_,
    credential_,
    adv_,
    option_
};

/** 
*   This is the structure which holds the details of the advertisement.
*   All external users should use the get/set API.
*/
struct _Jxta_lease_request_msg {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *client_id;
    
    Jxta_credential * credential;
    
    Jxta_PA *client_adv;
    Jxta_time_diff client_adv_exp;
    
    Jxta_time_diff requested_lease;
    apr_uuid_t *server_adv_gen;
    unsigned int referral_advs;
    Jxta_boolean disconnect;
    
    Jxta_vector *options;
};

static void lease_request_msg_delete(Jxta_object * me);
static Jxta_lease_request_msg *lease_request_msg_construct(Jxta_lease_request_msg * myself);
static Jxta_status validate_message(Jxta_lease_request_msg * myself);
static void handle_lease_request_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_client_adv(void *me, const XML_Char * cd, int len);
static void handle_option(void *me, const XML_Char * cd, int len);

JXTA_DECLARE(Jxta_lease_request_msg *) jxta_lease_request_msg_new(void)
{
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) calloc(1, sizeof(Jxta_lease_request_msg));

    if (NULL == myself) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, lease_request_msg_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "LeaseRequest NEW [%pp]\n", myself );

    return lease_request_msg_construct(myself);
}

static void lease_request_msg_delete(Jxta_object * me)
{
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) me;

    JXTA_OBJECT_RELEASE(myself->client_id);

    if (NULL != myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (NULL != myself->client_adv) {
        JXTA_OBJECT_RELEASE(myself->client_adv);
        myself->client_adv = NULL;
    }

    if (NULL != myself->server_adv_gen) {
        free(myself->server_adv_gen);
        myself->server_adv_gen = NULL;
    }

    JXTA_OBJECT_RELEASE(myself->options);

    jxta_advertisement_destruct((Jxta_advertisement *) myself);
    
    memset(myself, 0xdd, sizeof(Jxta_lease_request_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "LeaseRequest FREE [%pp]\n", myself );

    free(myself);
}

static const Kwdtab lease_request_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:LeaseRequest", LeaseRequestMsg_, *handle_lease_request_msg, NULL, NULL},
    {"Credential", credential_, *handle_credential, NULL, NULL},
    {"ClientAdv", adv_, *handle_client_adv, NULL, NULL},
    {"Option", option_, *handle_option, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


static Jxta_lease_request_msg *lease_request_msg_construct(Jxta_lease_request_msg * myself)
{
    myself = (Jxta_lease_request_msg *)
        jxta_advertisement_construct((Jxta_advertisement *) myself,
                                     "jxta:LeaseRequest",
                                     lease_request_msg_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_lease_request_msg_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL, (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != myself) {
        myself->client_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->credential = NULL;
        myself->client_adv = NULL;
        myself->client_adv_exp = -1L;
        myself->requested_lease = -1L;
        myself->server_adv_gen = NULL;
        myself->referral_advs = 0;
        myself->disconnect = FALSE;
        myself->options = jxta_vector_new(0);
    }

    return myself;
}

static Jxta_status validate_message(Jxta_lease_request_msg * myself) {

    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->client_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Client peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

#if 0 
    /* FIXME 20060827 bondolo Credential processing not yet implemented. */
    if ( NULL != myself->credential ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Credential must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    if ( NULL != myself->client_adv ) {
        Jxta_id * peer_id = jxta_PA_get_PID(myself->client_adv);
        Jxta_boolean same = jxta_id_equals( myself->client_id, peer_id);
        
        if( NULL != peer_id ) {
            JXTA_OBJECT_RELEASE(peer_id);
        }
        
        if( !same ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Peer id of Advertisement must match client_id [%pp].\n", myself);
            return JXTA_INVALID_ARGUMENT;
        }
    }

    if ( (NULL != myself->client_adv) && (myself->client_adv_exp <= 0L) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Advertisement expiration must be a positive value [%pp].\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    /* -1L indicates requested lease is not specified */
    if (myself->requested_lease < 0L && -1L != myself->requested_lease) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Requested lease must be >= 0 [%pp].\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_lease_request_msg_parse_charbuffer(Jxta_lease_request_msg * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_lease_request_msg_parse_file(Jxta_lease_request_msg * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_lease_request_msg_get_xml(Jxta_lease_request_msg * myself, JString ** xml)
{
    Jxta_status res;
    JString *string;
    JString *tempstr;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */
    unsigned int each_option;
    unsigned int all_options;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:LeaseRequest");

    if (NULL != myself->client_id) {
        jstring_append_2(string, " client_id=\"");
        jxta_id_to_jstring(myself->client_id, &tempstr);
        jstring_append_1(string, tempstr);
        JXTA_OBJECT_RELEASE(tempstr);
        jstring_append_2(string, "\"");
    }

    if (-1 != myself->requested_lease) {
        jstring_append_2(string, " requested_lease=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, myself->requested_lease);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (myself->server_adv_gen) {
        jstring_append_2(string, " server_adv_gen=\"");
        apr_uuid_format(tmpbuf, myself->server_adv_gen);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (0 != myself->referral_advs) {
        jstring_append_2(string, " referral_advs=\"");
        apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->referral_advs);
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "\"");
    }

    if (0 != myself->disconnect) {
        jstring_append_2(string, " disconnect=\"");
        jstring_append_2(string, "true\"");
    }


    jstring_append_2(string, ">\n ");
    
    if (NULL != myself->client_adv) {
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;
        Jxta_PA *client_adv;

        attrs[attr_idx++] = "expiration";

        apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, jxta_lease_request_msg_get_client_adv_exp(myself));
        free_mask |= (1 << attr_idx);
        attrs[attr_idx++] = strdup(tmpbuf);

        attrs[attr_idx] = NULL;

        client_adv = jxta_lease_request_msg_get_client_adv(myself);
        tempstr = NULL;

        jxta_PA_get_xml_1(client_adv, &tempstr, "ClientAdv", attrs);
        JXTA_OBJECT_RELEASE(client_adv);

        for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
            if (free_mask & (1 << attr_idx)) {
                free( (void*) attrs[attr_idx]);
            }
        }

        jstring_append_1(string, tempstr);
        JXTA_OBJECT_RELEASE(tempstr);
        tempstr = NULL;
    }
    
    all_options = jxta_vector_size(myself->options);
    for( each_option = 0; each_option < all_options; each_option++ ) {
        Jxta_advertisement * option = NULL;
        
        res = jxta_vector_get_object_at( myself->options, JXTA_OBJECT_PPTR(&option), each_option );
        
        if( JXTA_SUCCESS == res ) {
            res = jxta_advertisement_get_xml( option, &tempstr );
            if( JXTA_SUCCESS == res ) {
                jstring_append_1(string, tempstr);
                JXTA_OBJECT_RELEASE(tempstr);
                tempstr = NULL;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed generating XML for option [%pp].\n", myself);
            }
        } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed getting option [%pp].\n", myself);
        }
        JXTA_OBJECT_RELEASE(option);
    }

    jstring_append_2(string, "</jxta:LeaseRequest>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_id *) jxta_lease_request_msg_get_client_id(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->client_id);
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_client_id(Jxta_lease_request_msg * myself, Jxta_id * client_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(client_id);

    JXTA_OBJECT_RELEASE(myself->client_id);
    
    /* Client id may not be NULL. */
    myself->client_id = JXTA_OBJECT_SHARE(client_id);
}

JXTA_DECLARE(Jxta_time_diff) jxta_lease_request_msg_get_requested_lease(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->requested_lease;
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_requested_lease(Jxta_lease_request_msg * myself,
                                                                  Jxta_time_diff requested_lease)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->requested_lease = requested_lease;
}

JXTA_DECLARE(apr_uuid_t *) jxta_lease_request_msg_get_server_adv_gen(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->server_adv_gen) {
        apr_uuid_t *result = calloc(1, sizeof(apr_uuid_t));

        if (NULL == result) {
            return NULL;
        }

        memmove(result, myself->server_adv_gen, sizeof(apr_uuid_t));
        return result;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_server_adv_gen(Jxta_lease_request_msg * myself, apr_uuid_t * server_adv_gen)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->server_adv_gen) {
        free(myself->server_adv_gen);
        myself->server_adv_gen = NULL;
    }

    if (NULL != server_adv_gen) {
        myself->server_adv_gen = calloc(1, sizeof(apr_uuid_t));

        if (NULL != myself->server_adv_gen) {
            memmove(myself->server_adv_gen, server_adv_gen, sizeof(apr_uuid_t));
        }
    }
}

JXTA_DECLARE(unsigned int) jxta_lease_request_msg_get_referral_advs(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->referral_advs;
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_referral_advs(Jxta_lease_request_msg * myself, unsigned int referral_advs)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->referral_advs = referral_advs;
}


JXTA_DECLARE(Jxta_boolean) jxta_lease_request_msg_get_disconnect(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->disconnect;
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_disconnect(Jxta_lease_request_msg * myself, Jxta_boolean disconnect)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->disconnect = disconnect;
}


JXTA_DECLARE(Jxta_credential *) jxta_lease_request_msg_get_credential(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_credential(Jxta_lease_request_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(Jxta_PA *) jxta_lease_request_msg_get_client_adv(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->client_adv) {
        return JXTA_OBJECT_SHARE(myself->client_adv);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_client_adv(Jxta_lease_request_msg * myself, Jxta_PA *client_adv)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->client_adv != NULL) {
        JXTA_OBJECT_RELEASE(myself->client_adv);
        myself->client_adv = NULL;
    }

    if (client_adv != NULL) {
        myself->client_adv = JXTA_OBJECT_SHARE(client_adv);
    }
}

JXTA_DECLARE(Jxta_time_diff) jxta_lease_request_msg_get_client_adv_exp(Jxta_lease_request_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->client_adv_exp;
}

JXTA_DECLARE(void) jxta_lease_request_msg_set_client_adv_exp(Jxta_lease_request_msg * myself, Jxta_time_diff expiration)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->client_adv_exp = expiration;
}

JXTA_DECLARE(Jxta_vector *) jxta_lease_request_msg_get_options(Jxta_lease_request_msg * myself) {
    Jxta_vector *result = jxta_vector_new(0);
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    jxta_vector_addall_objects_last( result, myself->options );
    
    return result;
}

JXTA_DECLARE(void) jxta_lease_request_msg_clear_options(Jxta_lease_request_msg * myself ) {
    JXTA_OBJECT_CHECK_VALID(myself);

    jxta_vector_clear( myself->options );
}

JXTA_DECLARE(void) jxta_lease_request_msg_add_option(Jxta_lease_request_msg * myself, Jxta_advertisement *adv ) {
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(adv);

    jxta_vector_add_object_last( myself->options, (Jxta_object*) adv );
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_lease_request_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:LeaseRequest> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "client_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (myself->client_id) JXTA_OBJECT_RELEASE(myself->client_id);
                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->client_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for client_id [%pp]\n", myself);
                    myself->client_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "requested_lease")) {
                myself->requested_lease = atol(atts[1]);
            } else if (0 == strcmp(*atts, "server_adv_gen")) {
                myself->server_adv_gen = calloc(1, sizeof(apr_uuid_t));

                if (NULL == myself->server_adv_gen) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "Allocation failure during parse of server_adv_gen [%pp]\n", myself);

                } else {
                    if (APR_SUCCESS != apr_uuid_parse(myself->server_adv_gen, atts[1])) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                        free(myself->server_adv_gen);
                        myself->server_adv_gen = NULL;
                    }
                }
            } else if (0 == strcmp(*atts, "referral_advs")) {
                myself->referral_advs = atoi(atts[1]);
            } else if (0 == strcmp(*atts, "disconnect")) {
                myself->disconnect = TRUE;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:LeaseRequest> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) me;
    
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

static void handle_client_adv(void *me, const XML_Char * cd, int len)
{
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <ClientAdv> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "expiration")) {
                myself->client_adv_exp = atol(atts[1]);
            } else if (0 == strcmp(*atts, "pVer")) {
                /* just silently skip it. */
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }

        if (NULL != myself->client_adv) {
            JXTA_OBJECT_RELEASE(myself->client_adv);
            myself->client_adv = NULL;
        }

        myself->client_adv = jxta_PA_new();

        jxta_advertisement_set_handlers((Jxta_advertisement *) myself->client_adv, ((Jxta_advertisement *) myself)->parser, (void *) myself);
   } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <ClientAdv> : [%pp]\n", myself);
   }
}

static void handle_option(void *me, const XML_Char * cd, int len)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_lease_request_msg *myself = (Jxta_lease_request_msg *) me;

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        const char *type = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Option> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                type = atts[1];
            } else {
                /* just silently skip it. */
            }

            atts += 2;
        }
        
        if( NULL != type ) {
            Jxta_advertisement *new_ad = NULL;
            
            res = jxta_advertisement_global_handler((Jxta_advertisement *) myself, type, &new_ad);
    
            if( NULL != new_ad ) {
                /* set new handlers */
                jxta_advertisement_set_handlers(new_ad, ((Jxta_advertisement *) myself)->parser, (void *) myself);

                 /* hook it into our list of advs */
                jxta_lease_request_msg_add_option(myself, new_ad );
                JXTA_OBJECT_RELEASE(new_ad);
            } else {
                 jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized Option %s : [%pp]\n", type, myself);
            }
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid Option : [%pp]\n", myself);
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Option> : [%pp]\n", myself);
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
