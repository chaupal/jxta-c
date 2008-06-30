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

static const char *__log_cat = "PV_PONG";

#include <stdio.h>
#include <string.h>

#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"

#include "jxta_peerview_pong_msg.h"

const char JXTA_PEERVIEW_PONG_ELEMENT_NAME[] = "PeerviewPong";

static const char * pong_msg_action_text[] = { "PONG_INVITE",
                "PONG_PROMOTE",
                "PONG_DEMOTE",
                "PONG_STATUS"
};

static const char * pong_msg_state_text[] = { "RDV_STATE_RENDEZVOUS",
                "RDV_STATE_EDGE",
                "RDV_STATE_DEMOTING"
};

struct _Jxta_peerview_peer_info {
    JXTA_OBJECT_HANDLE;

    Jxta_id *peer_id;
    Jxta_PA *peer_adv;
    Jxta_boolean peer_adv_gen_set;
    apr_uuid_t peer_adv_gen;
    Jxta_time_diff peer_adv_exp;
    JString *target_hash;
    JString *target_hash_radius;
    Jxta_vector * options;
};

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/

struct _Jxta_peerview_pong_msg {
    Jxta_advertisement jxta_advertisement;
   
    Jxta_credential *credential;   

    JString *instance_mask;
    
    Jxta_peerview_peer_info * peer;

    Jxta_PA *peer_adv;
    Jxta_boolean peer_adv_gen_set;
    apr_uuid_t peer_adv_gen;
    Jxta_time_diff peer_adv_exp;
    Jxta_boolean is_rendezvous;
    Jxta_boolean is_demoting;

    Jxta_vector *partners;
    Jxta_vector *associates;
    Jxta_vector *candidates;

    Jxta_vector *options;


    Jxta_pong_msg_rdv_state  rdv_state;
    Jxta_pong_msg_action pong_action;


    /* Used during parsing */
    enum contexts { UNKNOWN, GLOBAL, PARTNER, ASSOCIATE, CANDIDATE } parse_context;
    
    /* THIS REFERENCE IS NOT SHARED AND DOES NOT NEED TO BE RELEASED. */
    Jxta_peerview_peer_info * current_peer_info;    

};

static Jxta_peerview_peer_info *peerview_peer_info_new( Jxta_id * id, apr_uuid_t * adv_gen_ptr, char const *target_hash, char const *target_hash_radius, Jxta_vector * options );
static void peerview_peer_info_delete(Jxta_object * me);

static void peerview_pong_msg_delete(Jxta_object * me);
static void handle_peerview_pong_msg(void *me, const XML_Char * cd, int len);
static void handle_credential(void *me, const XML_Char * cd, int len);
static void handle_instance_mask(void *me, const XML_Char * cd, int len);
static void handle_target_hash(void *me, const XML_Char * cd, int len);
static void handle_adv(void *me, const XML_Char * cd, int len);
static void handle_partner(void *me, const XML_Char * cd, int len);
static void handle_associate(void *me, const XML_Char * cd, int len);
static void handle_candidate(void *me, const XML_Char * cd, int len);
static void handle_option(void *me, const XML_Char * cd, int len);
static Jxta_status print_options_xml(JString *xml_string_j, Jxta_vector * options);
static Jxta_status validate_message(Jxta_peerview_pong_msg * myself);

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    Jxta_peerview_pong_msg_,
    Credential_,
    InstanceMask_,
    TargetHash_,
    Adv_,
    Partner_,
    ClusterMember_,
    Candidate_,
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
static const Kwdtab jxta_peerview_pong_msg_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PeerviewPong", Jxta_peerview_pong_msg_, *handle_peerview_pong_msg, NULL, NULL},
    {"Credential", Credential_, *handle_credential, NULL, NULL},
    {"InstanceMask", InstanceMask_, *handle_instance_mask, NULL, NULL},
    {"TargetHash", TargetHash_, *handle_target_hash, NULL, NULL},
    {"Adv", Adv_, *handle_adv, NULL, NULL},
    {"Partner", Partner_, *handle_partner, NULL, NULL},
    {"ClusterMember", ClusterMember_, *handle_associate, NULL, NULL},
    {"Candidate", Candidate_, *handle_candidate, NULL, NULL},
    {"Option", Option_, *handle_option, NULL, NULL},
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_peerview_pong_msg *) jxta_peerview_pong_msg_new(void)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) calloc(1, sizeof(Jxta_peerview_pong_msg));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_pong_msg NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, peerview_pong_msg_delete, NULL);

    myself = (Jxta_peerview_pong_msg *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:PeerviewPong",
                                                                 jxta_peerview_pong_msg_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_peerview_pong_msg_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);

    if (NULL != myself) {
        myself->peer = peerview_peer_info_new( JXTA_OBJECT_SHARE(jxta_id_nullID), NULL, NULL, NULL, NULL );
        myself->credential = NULL;
        myself->instance_mask = NULL;
        myself->peer_adv = NULL;
        myself->peer_adv_gen_set = FALSE;
        myself->peer_adv_exp = -1;
        myself->partners = jxta_vector_new(0);
        myself->associates = jxta_vector_new(0);
        myself->candidates = jxta_vector_new(0);
        myself->options = jxta_vector_new(0);
        myself->is_rendezvous = FALSE;
        myself->is_demoting = FALSE;
        myself->pong_action = PONG_STATUS;
    }

    return myself;
}

static void peerview_pong_msg_delete(Jxta_object * me)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;

    if (myself->credential) {
        JXTA_OBJECT_RELEASE(myself->credential);
        myself->credential = NULL;
    }

    if (myself->instance_mask) {
        JXTA_OBJECT_RELEASE(myself->instance_mask);
        myself->instance_mask = NULL;
    }

    JXTA_OBJECT_RELEASE(myself->peer);

    if (myself->peer_adv) {
        JXTA_OBJECT_RELEASE(myself->peer_adv);
        myself->peer_adv = NULL;
    }

    if (myself->partners) {
        JXTA_OBJECT_RELEASE(myself->partners);
        myself->partners = NULL;
    }

    if (myself->associates) {
        JXTA_OBJECT_RELEASE(myself->associates);
        myself->associates = NULL;
    }

    if (myself->candidates) {
        JXTA_OBJECT_RELEASE(myself->candidates);
        myself->candidates = NULL;
    }

    if (myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
        myself->options = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_peerview_pong_msg));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_pong_msg FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_id *) jxta_peerview_pong_msg_get_peer_id(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->peer->peer_id);
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_peer_id(Jxta_peerview_pong_msg * myself, Jxta_id * peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(peer_id);

    JXTA_OBJECT_RELEASE(myself->peer->peer_id);
    
    /* peer id may not be NULL. */
    myself->peer->peer_id = JXTA_OBJECT_SHARE(peer_id);
}

JXTA_DECLARE(Jxta_pong_msg_action) jxta_peerview_pong_msg_get_action(Jxta_peerview_pong_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return me->pong_action;
}

JXTA_DECLARE(const char *) jxta_peerview_pong_msg_action_text(Jxta_peerview_pong_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return pong_msg_action_text[me->pong_action];
}

JXTA_DECLARE(const char *) jxta_peerview_pong_msg_state_text(Jxta_peerview_pong_msg * me)
{
    JXTA_OBJECT_CHECK_VALID(me);
    return pong_msg_state_text[me->rdv_state];
}


JXTA_DECLARE(Jxta_status) jxta_peerview_pong_msg_set_action(Jxta_peerview_pong_msg * me, Jxta_pong_msg_action action)
{
    Jxta_status res = JXTA_SUCCESS;
    JXTA_OBJECT_CHECK_VALID(me);
    me->pong_action = action;
    return res;
}

JXTA_DECLARE(Jxta_credential *) jxta_peerview_pong_msg_get_credential(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->credential) {
        return JXTA_OBJECT_SHARE(myself->credential);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_credential(Jxta_peerview_pong_msg * myself, Jxta_credential *credential)
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

JXTA_DECLARE(const char *) jxta_peerview_pong_msg_get_instance_mask(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->instance_mask) {
        return jstring_get_string(myself->instance_mask);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(Jxta_vector *) jxta_peerview_pong_msg_get_options(Jxta_peerview_pong_msg * myself) {
    Jxta_vector *result = jxta_vector_new(0);
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    jxta_vector_addall_objects_last( result, myself->options );
    
    return result;
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_clear_options(Jxta_peerview_pong_msg * myself ) {
    JXTA_OBJECT_CHECK_VALID(myself);

    jxta_vector_clear( myself->options );
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_add_option(Jxta_peerview_pong_msg * myself, Jxta_advertisement *adv ) {
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(adv);

    jxta_vector_add_object_last( myself->options, (Jxta_object*) adv );
}


JXTA_DECLARE(void) jxta_peerview_pong_msg_set_instance_mask(Jxta_peerview_pong_msg * myself, const char *instance_mask)
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

JXTA_DECLARE(const char *) jxta_peerview_pong_msg_get_target_hash(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->peer->target_hash) {
        return jstring_get_string(myself->peer->target_hash);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_target_hash(Jxta_peerview_pong_msg * myself, const char *target_hash)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->peer->target_hash != NULL) {
        JXTA_OBJECT_RELEASE(myself->peer->target_hash);
        myself->peer->target_hash = NULL;
    }

    if (target_hash != NULL) {
        myself->peer->target_hash = jstring_new_2(target_hash);
        jstring_trim(myself->peer->target_hash);
    }
}

JXTA_DECLARE(const char *) jxta_peerview_pong_msg_get_target_hash_radius(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->peer->target_hash_radius) {
        return jstring_get_string(myself->peer->target_hash_radius);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_target_hash_radius(Jxta_peerview_pong_msg * myself, const char *target_hash_radius)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->peer->target_hash_radius != NULL) {
        JXTA_OBJECT_RELEASE(myself->peer->target_hash_radius);
        myself->peer->target_hash_radius = NULL;
    }

    if (target_hash_radius != NULL) {
        myself->peer->target_hash_radius = jstring_new_2(target_hash_radius);
        jstring_trim(myself->peer->target_hash_radius);
    }
}

JXTA_DECLARE(Jxta_PA *) jxta_peerview_pong_msg_get_peer_adv(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->peer_adv != NULL) {
        return JXTA_OBJECT_SHARE(myself->peer_adv);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_peer_adv(Jxta_peerview_pong_msg * myself, Jxta_PA * peer_adv )
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->peer_adv != NULL) {
        JXTA_OBJECT_RELEASE(myself->peer_adv);
        myself->peer_adv = NULL;
    }

    if (peer_adv != NULL) {
        myself->peer_adv = JXTA_OBJECT_SHARE(peer_adv);
    }
}

JXTA_DECLARE(apr_uuid_t *) jxta_peerview_pong_msg_get_peer_adv_gen(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->peer_adv_gen_set) {
        apr_uuid_t *result = (apr_uuid_t *) calloc(1, sizeof(apr_uuid_t));

        if (NULL != result) {
            *result = myself->peer_adv_gen;
        }

        return result;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_peer_adv_gen(Jxta_peerview_pong_msg * myself, apr_uuid_t const * peer_adv_gen)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->peer_adv_gen_set = (NULL != peer_adv_gen);

    if (myself->peer_adv_gen_set) {
        memcpy( &myself->peer_adv_gen, peer_adv_gen, sizeof(apr_uuid_t) );
    }
}

JXTA_DECLARE(Jxta_time_diff) jxta_peerview_pong_msg_get_peer_adv_exp(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->peer_adv_exp;
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_peer_adv_exp(Jxta_peerview_pong_msg * myself, Jxta_time_diff expiration)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->peer_adv_exp = expiration;
}

JXTA_DECLARE(Jxta_pong_msg_rdv_state) jxta_peerview_pong_msg_get_state(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->rdv_state;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_pong_msg_is_rendezvous(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    return myself->is_rendezvous;
}

JXTA_DECLARE(void) jxta_peerview_pong_msg_set_rendezvous(Jxta_peerview_pong_msg * myself, Jxta_boolean rdv)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    myself->is_rendezvous = rdv;
    return;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_pong_msg_is_demoting(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    return myself->is_demoting;
}


JXTA_DECLARE(void) jxta_peerview_pong_msg_set_is_demoting(Jxta_peerview_pong_msg * myself, Jxta_boolean demote)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    myself->is_demoting = demote;
    return;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_pong_msg_parse_charbuffer(Jxta_peerview_pong_msg * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_pong_msg_parse_file(Jxta_peerview_pong_msg * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_message(myself);
    }
    
    return res;
}

static Jxta_status validate_message(Jxta_peerview_pong_msg * myself) {

    if ( (NULL == myself->peer->peer_id) || jxta_id_equals(myself->peer->peer_id, jxta_id_nullID) ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ( NULL == myself->instance_mask ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Instance mask must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ( NULL == myself->peer->target_hash ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "target hash must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ( NULL == myself->peer->target_hash_radius ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "target hash radius must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    
    if( NULL != myself->peer_adv ) {
        Jxta_id *pid = jxta_PA_get_PID(myself->peer_adv);
        Jxta_boolean same = jxta_id_equals(pid, myself->peer->peer_id);
        
        JXTA_OBJECT_RELEASE(pid);
        if( !same ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer id and peer advertisement id not the same. [%pp]\n", myself);
            return JXTA_INVALID_ARGUMENT;
        }
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_pong_msg_get_xml(Jxta_peerview_pong_msg * myself, JString ** xml)
{
    Jxta_status res=JXTA_SUCCESS;
    JString *string;
    JString *temp;
    unsigned int i=0;
    char tmpbuf[256];   /* We use this buffer to store a string representation of a int */
    apr_uuid_t adv_gen;
 
    JXTA_OBJECT_CHECK_VALID(myself);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    
    res = validate_message(myself);
    if( JXTA_SUCCESS != res ) {
        return res;
    }
    
    string = jstring_new_0();

    jstring_append_2(string, "<jxta:PeerviewPong");
    jstring_append_2(string, " peer_id=\"");
    jxta_id_to_jstring(myself->peer->peer_id, &temp);
    jstring_append_1(string, temp);
    JXTA_OBJECT_RELEASE(temp);
    jstring_append_2(string, "\"");
    jstring_append_2(string, " rdv_state=\"");
    myself->rdv_state = myself->is_rendezvous ? RDV_STATE_RENDEZVOUS:RDV_STATE_EDGE;
    if (myself->is_demoting) {
        myself->rdv_state = RDV_STATE_DEMOTING;
    }
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->rdv_state);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "\"");
    jstring_append_2(string, " pong_action=\"");
    apr_snprintf(tmpbuf, sizeof(tmpbuf), "%d", myself->pong_action);
    jstring_append_2(string, tmpbuf);
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");

    jstring_append_2(string, "<InstanceMask>");
    jstring_append_2(string, jxta_peerview_pong_msg_get_instance_mask(myself));
    jstring_append_2(string, "</InstanceMask>\n");
    
    jstring_append_2(string, "<TargetHash");
    jstring_append_2(string, " radius=\"");
    jstring_append_2(string, jxta_peerview_pong_msg_get_target_hash_radius(myself));    
    jstring_append_2(string, "\"");
    jstring_append_2(string, ">\n");
    jstring_append_2(string, jxta_peerview_pong_msg_get_target_hash(myself));
    jstring_append_2(string, "</TargetHash>\n");
    
    if( NULL != myself->peer_adv ) {
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;
        apr_uuid_t *aadv_gen = jxta_peerview_pong_msg_get_peer_adv_gen(myself);
 
        if (NULL != aadv_gen) {
            attrs[attr_idx++] = "adv_gen";

            apr_uuid_format(tmpbuf, aadv_gen);
            free(aadv_gen);
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }

        if (-1L != jxta_peerview_pong_msg_get_peer_adv_exp(myself)) {
            attrs[attr_idx++] = "expiration";

            apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, jxta_peerview_pong_msg_get_peer_adv_exp(myself));
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }

        attrs[attr_idx] = NULL;

        res = jxta_PA_get_xml_1(myself->peer_adv, &temp, "Adv", attrs);

        if( JXTA_SUCCESS == res ) {
            jstring_append_1(string, temp);
        }
        JXTA_OBJECT_RELEASE(temp);
        for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
            if (free_mask & (1 << attr_idx)) {
                free( (void*) attrs[attr_idx]);
            }
        }
    }

    if (NULL != myself->options) {
        print_options_xml(string, myself->options);
    }

    for (i = 0; i < jxta_vector_size(myself->associates); i++) {
        Jxta_peerview_peer_info *peer_info=NULL;
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;
        res = jxta_vector_get_object_at(myself->associates, JXTA_OBJECT_PPTR(&peer_info), i);
        if (JXTA_SUCCESS != res) continue;
        JString *jPeerid = NULL;
        jstring_append_2(string, "<ClusterMember");
        jstring_append_2(string, " peer_id = \"");
        jxta_id_to_jstring(peer_info->peer_id, &jPeerid);
        jstring_append_2(string, jstring_get_string(jPeerid));
        jstring_append_2(string, "\"");
        if (peer_info->peer_adv_gen_set) {
            jstring_append_2(string, " adv_gen = \"");
            memmove(&adv_gen, &peer_info->peer_adv_gen, sizeof(apr_uuid_t));
            apr_uuid_format(tmpbuf, &adv_gen);
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "\"");
        }
        jstring_append_2(string, " >");
        jstring_append_2(string, "<TargetHash");
        jstring_append_2(string, " radius=\"");
        jstring_append_2(string, jstring_get_string(peer_info->target_hash_radius));
        jstring_append_2(string, "\"");
        jstring_append_2(string, ">\n");
        jstring_append_2(string, jstring_get_string(peer_info->target_hash));
        jstring_append_2(string, "</TargetHash>\n");

        if (-1L != peer_info->peer_adv_exp) {
            attrs[attr_idx++] = "expiration";

            apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, peer_info->peer_adv_exp);
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }
        if (peer_info->peer_adv_gen_set) {
            attrs[attr_idx++] = "adv_gen";
            memmove(&adv_gen, &peer_info->peer_adv_gen, sizeof(apr_uuid_t));
            apr_uuid_format(tmpbuf, &adv_gen);
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }
        attrs[attr_idx] = NULL;
        for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
            if (free_mask & (1 << attr_idx)) {
                free( (void*) attrs[attr_idx]);
            }
        }
        jstring_append_2(string, "</ClusterMember>\n");

        JXTA_OBJECT_RELEASE(jPeerid);
        JXTA_OBJECT_RELEASE(peer_info);
    }

    for (i = 0; i < jxta_vector_size(myself->partners); i++) {
        Jxta_peerview_peer_info *peer_info=NULL;
        res = jxta_vector_get_object_at(myself->partners, JXTA_OBJECT_PPTR(&peer_info), i);
        if (JXTA_SUCCESS != res) continue;
        JString *jPeerid = NULL;
        jstring_append_2(string, "<Partner");
        jstring_append_2(string, " peer_id = \"");
        jxta_id_to_jstring(peer_info->peer_id, &jPeerid);
        jstring_append_2(string, jstring_get_string(jPeerid));
        jstring_append_2(string, "\"");
        if (peer_info->peer_adv_gen_set) {
            jstring_append_2(string, " adv_gen = \"");
            memmove(&adv_gen, &peer_info->peer_adv_gen, sizeof(apr_uuid_t));
            apr_uuid_format(tmpbuf, &adv_gen);
            jstring_append_2(string, tmpbuf);
            jstring_append_2(string, "\"");
        }
        jstring_append_2(string, " >");
        jstring_append_2(string, "<TargetHash");
        jstring_append_2(string, " radius=\"");
        jstring_append_2(string, jstring_get_string(peer_info->target_hash_radius));
        jstring_append_2(string, "\"");
        jstring_append_2(string, ">\n");
        jstring_append_2(string, jstring_get_string(peer_info->target_hash));
        jstring_append_2(string, "</TargetHash>\n");
        char const *attrs[8] = { "type", "jxta:PA" };
        unsigned int free_mask = 0;
        int attr_idx = 2;

        if (-1L != peer_info->peer_adv_exp) {
            attrs[attr_idx++] = "expiration";

            apr_snprintf(tmpbuf, sizeof(tmpbuf), JPR_DIFF_TIME_FMT, peer_info->peer_adv_exp);
            free_mask |= (1 << attr_idx);
            attrs[attr_idx++] = strdup(tmpbuf);
        }

        attrs[attr_idx] = NULL;

        for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
            if (free_mask & (1 << attr_idx)) {
               free( (void*) attrs[attr_idx]);
            }
        }

        jstring_append_2(string, "</Partner>\n");
        JXTA_OBJECT_RELEASE(jPeerid);
        JXTA_OBJECT_RELEASE(peer_info);
    }
    for (i = 0; i < jxta_vector_size(myself->candidates); i++) {
        Jxta_peer *peer=NULL;
        Jxta_id *pid = NULL;
        Jxta_PA *padv;
        Jxta_vector * options = NULL;

        res = jxta_vector_get_object_at(myself->candidates, JXTA_OBJECT_PPTR(&peer), i);
        if (JXTA_SUCCESS != res) continue;
        JString *jPeerid = NULL;
        jstring_append_2(string, "<Candidate");
        jstring_append_2(string, " peer_id = \"");
        jxta_peer_get_peerid(peer, &pid);
        jxta_id_to_jstring(pid, &jPeerid);
        JXTA_OBJECT_RELEASE(pid);
        jstring_append_2(string, jstring_get_string(jPeerid));
        jstring_append_2(string, "\"");
        JXTA_OBJECT_RELEASE(jPeerid);
        jstring_append_2(string, ">\n");
        jxta_peer_get_adv(peer, &padv);
        if( NULL != padv) {
            char const *attrs[8] = { "type", "jxta:PA" };
            unsigned int free_mask = 0;
            int attr_idx = 2;

            res = jxta_PA_get_xml_1(padv, &temp, "Adv", attrs);

            if( JXTA_SUCCESS == res ) {
                jstring_append_1(string, temp);
            }
            JXTA_OBJECT_RELEASE(temp);
            for (attr_idx = 0; attrs[attr_idx]; attr_idx++) {
                if (free_mask & (1 << attr_idx)) {
                    free( (void*) attrs[attr_idx]);
                }
            }
            JXTA_OBJECT_RELEASE(padv);
        }
        jxta_peer_get_options(peer, &options);

        if (NULL != options) {
            print_options_xml(string, options);
            JXTA_OBJECT_RELEASE(options);
        }

        jstring_append_2(string, "</Candidate>\n");
        JXTA_OBJECT_RELEASE(peer);
     }

    /* FIXME Handle credentials */

    jstring_append_2(string, "</jxta:PeerviewPong>\n");

    *xml = string;

    return JXTA_SUCCESS;
}

static Jxta_status print_options_xml(JString *xml_string_j, Jxta_vector * options) {

    unsigned int each_option;
    unsigned int all_options;
    Jxta_status res = JXTA_SUCCESS;

    all_options = jxta_vector_size(options);
    for( each_option = 0; each_option < all_options; each_option++ ) {
        Jxta_advertisement * option = NULL;

        res = jxta_vector_get_object_at( options, JXTA_OBJECT_PPTR(&option), each_option );

        if( JXTA_SUCCESS == res ) {
            JString *tempstr;
            res = jxta_advertisement_get_xml( option, &tempstr );
            if( JXTA_SUCCESS == res ) {
                jstring_append_1(xml_string_j, tempstr);
                JXTA_OBJECT_RELEASE(tempstr);
                tempstr = NULL;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed generating XML for option [%pp].\n", option);
            }
        } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failed getting option [%pp].\n", option);
        }
        JXTA_OBJECT_RELEASE(option);
    }

    return res;
} 
/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_peerview_pong_msg(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PeerviewPong> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (myself->peer->peer_id) {
                    JXTA_OBJECT_RELEASE(myself->peer->peer_id);
                    myself->peer->peer_id = NULL;
                }
                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->peer->peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    myself->peer->peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "rdv_state")) {
                Jxta_pong_msg_rdv_state state;
                myself->is_demoting = FALSE;
                myself->is_rendezvous = TRUE;
                state = atoi(atts[1]);
                switch (state) {
                    case RDV_STATE_RENDEZVOUS:
                        break;
                    case RDV_STATE_EDGE:
                        myself->is_rendezvous = FALSE;
                        break;
                    case RDV_STATE_DEMOTING:
                        myself->is_demoting = TRUE;
                        break;
                };
            } else if (0 == strcmp(*atts, "pong_action")) {
                Jxta_pong_msg_action action;
                action = atoi(atts[1]);
                myself->pong_action = action;
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
        
        myself->parse_context = GLOBAL;
        myself->current_peer_info = myself->peer;
    } else {
        myself->parse_context = UNKNOWN;
        myself->current_peer_info = NULL;
                
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PeerviewPong> : [%pp]\n", myself);
    }
}

static void handle_credential(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    
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
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <InstanceMask> : [%pp]\n", myself);
    } else {
        JString *string = jstring_new_1(len );

        jstring_append_0( string, cd, len);
        jstring_trim(string);

        jxta_peerview_pong_msg_set_instance_mask(myself, jstring_get_string(string));

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <InstanceMask> : [%pp]\n", myself);
    }
}

static void handle_target_hash(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <TargetHash> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "radius")) {
                JString *radius = jstring_new_2(atts[1]);
                jstring_trim(radius);
                
                switch(myself->parse_context) {
                    case UNKNOWN :
                        break;

                    case GLOBAL :
                    case PARTNER : 
                    case ASSOCIATE :
                    case CANDIDATE :
                        if( NULL != myself->current_peer_info->target_hash_radius ) {
                            JXTA_OBJECT_RELEASE(myself->current_peer_info->target_hash_radius);
                        }
                        myself->current_peer_info->target_hash_radius = JXTA_OBJECT_SHARE( radius );
                        break;               
                }

                JXTA_OBJECT_RELEASE(radius);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        JString *string = jstring_new_1(len );

        jstring_append_0( string, cd, len);
        jstring_trim(string);

        switch(myself->parse_context) {
            case UNKNOWN :
                break;
                
            case GLOBAL :
            case PARTNER : 
            case ASSOCIATE :
            case CANDIDATE :
                if( NULL != myself->current_peer_info->target_hash ) {
                    JXTA_OBJECT_RELEASE(myself->current_peer_info->target_hash);
                }
                myself->current_peer_info->target_hash = JXTA_OBJECT_SHARE( string );
                break;               
        }

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <TargetHash> : [%pp]\n", myself);
    }
}

static void handle_adv(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_PA * peer_adv = NULL;
        Jxta_boolean peer_adv_gen_set = FALSE;
        apr_uuid_t peer_adv_gen;
        Jxta_time_diff peer_adv_exp = 0;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Adv> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "adv_gen")) {
                peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &peer_adv_gen, atts[1])) ? TRUE:FALSE;
                if( !peer_adv_gen_set ) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                }
            } else if (0 == strcmp(*atts, "expiration")) {
                peer_adv_exp = atol(atts[1]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
        switch(myself->parse_context) {
            case UNKNOWN :
                break;

            case GLOBAL :
                if (myself->peer_adv)
                    JXTA_OBJECT_RELEASE(myself->peer_adv);
                myself->peer_adv = jxta_PA_new();
                myself->peer_adv_gen_set = peer_adv_gen_set;
                myself->peer_adv_gen = peer_adv_gen;
                myself->peer_adv_exp = peer_adv_exp;
                peer_adv = myself->peer_adv;
                break;
            case PARTNER : 
            case ASSOCIATE :
            case CANDIDATE :
                peer_adv = jxta_PA_new();
                if( NULL != myself->current_peer_info->peer_adv ) {
                    JXTA_OBJECT_RELEASE(myself->current_peer_info->peer_adv);
                }
                myself->current_peer_info->peer_adv = peer_adv;
                myself->current_peer_info->peer_adv_gen_set = peer_adv_gen_set;
                if (peer_adv_gen_set) {
                    memmove(&myself->current_peer_info->peer_adv_gen, &peer_adv_gen, sizeof(apr_uuid_t));
                }
                myself->current_peer_info->peer_adv_exp = peer_adv_exp;
                break;
        }
 
        jxta_advertisement_set_handlers((Jxta_advertisement *) peer_adv, ((Jxta_advertisement *) myself)->parser, (void *) myself);
   } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Adv> : [%pp]\n", myself);
   }
}

static void handle_partner(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
        Jxta_boolean peer_adv_gen_set = FALSE;
        apr_uuid_t peer_adv_gen;
    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_id * peer_id = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Partner> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (JXTA_SUCCESS != jxta_id_from_jstring(&peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "adv_gen")) {
                peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &peer_adv_gen, atts[1])) ? TRUE:FALSE;
                if( !peer_adv_gen_set ) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Attribute adv_gen: %s\n", atts[1]);
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }

        if( NULL != peer_id ) {
            Jxta_peerview_peer_info* peer_info = peerview_peer_info_new( peer_id, NULL, NULL, NULL, NULL );
            jxta_vector_add_object_last( myself->partners, (Jxta_object*) peer_info );

            myself->parse_context = PARTNER;
            myself->current_peer_info = peer_info; /* the object is retained in the cluster_members. */
            myself->current_peer_info->peer_adv_gen_set = peer_adv_gen_set;
            if (peer_adv_gen_set) {
                memmove(&myself->current_peer_info->peer_adv_gen, &peer_adv_gen, sizeof(apr_uuid_t));
            }
            JXTA_OBJECT_RELEASE(peer_info);
            JXTA_OBJECT_RELEASE(peer_id);
        }
    } else {
        myself->parse_context = UNKNOWN;
        myself->current_peer_info = NULL;
    
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <Partner> : [%pp]\n", myself);
    }
}

static void handle_associate(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    Jxta_boolean peer_adv_gen_set = FALSE;
    apr_uuid_t peer_adv_gen;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_id * peer_id = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <ClusterMember> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (JXTA_SUCCESS != jxta_id_from_jstring(&peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "adv_gen")) {
                peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &peer_adv_gen, atts[1]))? TRUE:FALSE;
                if( !peer_adv_gen_set ) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Attribute adv_gen: %s\n", atts[1]);
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
        
        if( NULL != peer_id ) {
            Jxta_peerview_peer_info* peer_info = peerview_peer_info_new( peer_id, NULL, NULL, NULL, NULL );
            jxta_vector_add_object_last( myself->associates, (Jxta_object*) peer_info );
            
            myself->parse_context = ASSOCIATE;
            myself->current_peer_info = peer_info; /* the object is retained in the partners. */
            myself->current_peer_info->peer_adv_gen_set = peer_adv_gen_set;
            if (peer_adv_gen_set) {
                memmove(&myself->current_peer_info->peer_adv_gen, &peer_adv_gen, sizeof(apr_uuid_t));
            }
            JXTA_OBJECT_RELEASE(peer_info);
            JXTA_OBJECT_RELEASE(peer_id);
        }
    } else {
        myself->parse_context = UNKNOWN;
        myself->current_peer_info = NULL;
    
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <ClusterMember> : [%pp]\n", myself);
    }
}

static void handle_candidate(void *me, const XML_Char * cd, int len)
{
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;
    Jxta_boolean peer_adv_gen_set = FALSE;
    apr_uuid_t peer_adv_gen;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;
        Jxta_id * peer_id = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Candidate> : [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);
                if (JXTA_SUCCESS != jxta_id_from_jstring(&peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                    FILEANDLINE "ID parse failure for peer_id [%pp]\n", myself);
                    peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "adv_gen")) {
                peer_adv_gen_set = (APR_SUCCESS == apr_uuid_parse( &peer_adv_gen, atts[1]))? TRUE:FALSE;
                if( !peer_adv_gen_set ) {
                        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR,
                                        FILEANDLINE "UUID parse failure for server_adv_gen [%pp] : %s\n", myself, atts[1]);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Attribute adv_gen: %s\n", atts[1]);
                }
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
        
        if( NULL != peer_id ) {
            Jxta_peerview_peer_info* peer_info = peerview_peer_info_new( peer_id, NULL, NULL, NULL, NULL);
            jxta_vector_add_object_last( myself->candidates, (Jxta_object*) peer_info );
            
            myself->parse_context = CANDIDATE;
            myself->current_peer_info = peer_info; /* the object is retained in the partners. */
            myself->current_peer_info->peer_adv_gen_set = peer_adv_gen_set;
            if (peer_adv_gen_set) {
                memmove(&myself->current_peer_info->peer_adv_gen, &peer_adv_gen, sizeof(apr_uuid_t));
            }
            JXTA_OBJECT_RELEASE(peer_info);
            JXTA_OBJECT_RELEASE(peer_id);
        }
    } else {
        myself->parse_context = UNKNOWN;
        myself->current_peer_info = NULL;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <Candidate> : [%pp]\n", myself);
    }
}


static void handle_option(void *me, const XML_Char * cd, int len)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_peerview_pong_msg *myself = (Jxta_peerview_pong_msg *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

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

                if(myself->parse_context == GLOBAL ||
                    myself->parse_context == CANDIDATE ||
                    myself->parse_context == PARTNER ||
                    myself->parse_context == ASSOCIATE)
                {
                    //If the parse_context is UNKNOWN it will cause a segfault so we won't set the handlers for that case
                    jxta_advertisement_set_handlers(new_ad, ((Jxta_advertisement *) myself)->parser, (void *) myself);
                }
                
                switch (myself->parse_context) {
                    case UNKNOWN:
                        break;
                    case GLOBAL:
                        jxta_peerview_pong_msg_add_option(myself, new_ad );
                        break;
                    case CANDIDATE:
                    case PARTNER:
                    case ASSOCIATE:
                    {
                        if (NULL == myself->current_peer_info->options) {
                            myself->current_peer_info->options = jxta_vector_new(0);
                        }
                        jxta_vector_add_object_last(myself->current_peer_info->options, (Jxta_object *) new_ad);
                        break;
                    }
                }
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

JXTA_DECLARE(Jxta_vector *) jxta_pong_msg_get_partner_infos(Jxta_peerview_pong_msg * myself)
{
    Jxta_vector *infos;
    
    JXTA_OBJECT_CHECK_VALID(myself);

    if( JXTA_SUCCESS == jxta_vector_clone( myself->partners, &infos, 0, INT_MAX ) ) {
        return infos;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(unsigned int) jxta_pong_msg_get_partners_size(Jxta_peerview_pong_msg * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->partners) {
        return jxta_vector_size(myself->partners);
    } else {
        return 0;
    }
}

JXTA_DECLARE(void) jxta_pong_msg_clear_partner_infos(Jxta_peerview_pong_msg * myself )
{
    JXTA_OBJECT_CHECK_VALID(myself);
    
    jxta_vector_clear( myself->partners );
}

JXTA_DECLARE(void) jxta_pong_msg_add_partner_info(Jxta_peerview_pong_msg * myself, Jxta_id * peer,  apr_uuid_t * adv_gen_ptr, const char *target_hash, const char *target_hash_radius)
{
    Jxta_peerview_peer_info * new_info;

    JXTA_OBJECT_CHECK_VALID(myself);

    new_info = peerview_peer_info_new( peer, adv_gen_ptr, target_hash, target_hash_radius, NULL);

    if( NULL != new_info ) {
        jxta_vector_add_object_last( myself->partners, (Jxta_object*) new_info );
        JXTA_OBJECT_RELEASE( new_info );
    }
}

JXTA_DECLARE(Jxta_vector *) jxta_pong_msg_get_associate_infos(Jxta_peerview_pong_msg * myself)
{
    Jxta_vector *infos;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( JXTA_SUCCESS == jxta_vector_clone( myself->associates, &infos, 0, INT_MAX ) ) {
        return infos;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_pong_msg_clear_associate_infos(Jxta_peerview_pong_msg * myself )
{
    JXTA_OBJECT_CHECK_VALID(myself);

    jxta_vector_clear( myself->associates );
}

JXTA_DECLARE(void) jxta_pong_msg_add_associate_info(Jxta_peerview_pong_msg * myself,  Jxta_id * peer, apr_uuid_t * adv_gen_ptr, const char *target_hash, const char *target_hash_radius)
{
    Jxta_peerview_peer_info * new_info;
    JXTA_OBJECT_CHECK_VALID(myself);
    new_info = peerview_peer_info_new( peer, adv_gen_ptr, target_hash, target_hash_radius, NULL);

    if( NULL != new_info ) {
        jxta_vector_add_object_last( myself->associates, (Jxta_object*) new_info );
        JXTA_OBJECT_RELEASE( new_info );
    }
}

JXTA_DECLARE(Jxta_vector *) jxta_pong_msg_get_candidate_infos(Jxta_peerview_pong_msg * myself)
{
    if (NULL != myself->candidates) {
        JXTA_OBJECT_SHARE(myself->candidates);
    }
    return myself->candidates;
}

JXTA_DECLARE(void) jxta_pong_msg_add_candidate_info(Jxta_peerview_pong_msg * myself, Jxta_peer * peer)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(peer);
    jxta_vector_add_object_last(myself->candidates, (Jxta_object *) peer);
    return;
}

static Jxta_peerview_peer_info *peerview_peer_info_new( Jxta_id * id, apr_uuid_t * adv_gen_ptr, char const *target_hash, char const *target_hash_radius, Jxta_vector * options )
{
    Jxta_peerview_peer_info *myself;
    if ( NULL == id ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "ID may not be NULL.\n" );
        return NULL;
    }

    myself = (Jxta_peerview_peer_info *) calloc(1, sizeof(Jxta_peerview_peer_info));

    if (NULL == myself) {
        return NULL;
    }
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_peer_info NEW [%pp]\n", myself);
        
    JXTA_OBJECT_INIT(myself, peerview_peer_info_delete, NULL);
    
    myself->peer_id = JXTA_OBJECT_SHARE(id);
    if (NULL == adv_gen_ptr) {
        myself->peer_adv_gen_set = FALSE;
    } else {
        memmove (&myself->peer_adv_gen, adv_gen_ptr, sizeof(apr_uuid_t));
        myself->peer_adv_gen_set = TRUE;
    }
    if( NULL != target_hash ) {
        myself->target_hash = jstring_new_2(target_hash);
        jstring_trim(myself->target_hash);
    }
    if( NULL != target_hash_radius ) {
        myself->target_hash_radius = jstring_new_2(target_hash_radius);
        jstring_trim(myself->target_hash_radius);
    }
    if (NULL != options) {
        myself->options = JXTA_OBJECT_SHARE(options);
    }
    return myself;
}

static void peerview_peer_info_delete(Jxta_object * me)
{
    Jxta_peerview_peer_info *myself = (Jxta_peerview_peer_info *) me;
    
    JXTA_OBJECT_RELEASE(myself->peer_id);

    if( NULL != myself->target_hash ) {
        JXTA_OBJECT_RELEASE(myself->target_hash);
        myself->target_hash = NULL;
    }
    if( NULL != myself->peer_adv) {
        JXTA_OBJECT_RELEASE(myself->peer_adv);
        myself->peer_adv = NULL;
    }
    if( NULL != myself->target_hash_radius ) {
        JXTA_OBJECT_RELEASE(myself->target_hash_radius);
        myself->target_hash_radius = NULL;
    }
    if (NULL != myself->options) {
        JXTA_OBJECT_RELEASE(myself->options);
        myself->options = NULL;
    }
    memset(myself, 0xdd, sizeof(Jxta_peerview_peer_info));
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_peer_info FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_id *) jxta_peerview_peer_info_get_peer_id(Jxta_peerview_peer_info * peer)
{
    JXTA_OBJECT_CHECK_VALID(peer);

    return JXTA_OBJECT_SHARE(peer->peer_id);
}

JXTA_DECLARE(Jxta_PA *) jxta_peerview_peer_info_get_peer_adv(Jxta_peerview_peer_info * peer)
{
    JXTA_OBJECT_CHECK_VALID(peer);

    return JXTA_OBJECT_SHARE(peer->peer_adv);
}

JXTA_DECLARE(const char *) jxta_peerview_peer_info_get_target_hash(Jxta_peerview_peer_info * peer)
{
    JXTA_OBJECT_CHECK_VALID(peer);

    if (NULL != peer->target_hash) {
        return jstring_get_string(peer->target_hash);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(apr_uuid_t *) jxta_peerview_peer_info_get_adv_gen(Jxta_peerview_peer_info * peer)
{
    JXTA_OBJECT_CHECK_VALID(peer);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_peerview_peer_info get_adv_gen [%pp] %s\n", peer, peer->peer_adv_gen_set == TRUE? "true":"false");

    if (peer->peer_adv_gen_set) {
        return &peer->peer_adv_gen;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(Jxta_status) jxta_peerview_peer_info_get_adv_gen_id(Jxta_peerview_peer_info * peer, Jxta_id ** id)
{
    Jxta_status res = JXTA_SUCCESS;
    apr_uuid_t adv_gen;
    apr_uuid_t *adv_gen_ptr;
    char tmp[64];
    Jxta_id * jadv_gen;
    adv_gen_ptr = jxta_peerview_peer_info_get_adv_gen(peer);
    if (NULL != adv_gen_ptr) {
        memmove(&adv_gen, adv_gen_ptr, sizeof(apr_uuid_t));
        memmove(tmp, "urn:jxta:uuid-", 14);
        apr_uuid_format(&tmp[14], &adv_gen);
        res = jxta_id_from_cstr(&jadv_gen, tmp);
        if (JXTA_SUCCESS != res) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG,"Error creating a JXTA UUID from - %s\n", tmp);
        } else {
            JString * jstring;
            jxta_id_to_jstring(jadv_gen, &jstring);
            if (NULL != id) {
                *id = jadv_gen;
            }
            JXTA_OBJECT_RELEASE(jstring);
        }
    }
    return res;
}

JXTA_DECLARE(const char *) jxta_peerview_peer_info_get_target_hash_radius(Jxta_peerview_peer_info * peer)
{
    JXTA_OBJECT_CHECK_VALID(peer);

    if (NULL != peer->target_hash_radius) {
        return jstring_get_string(peer->target_hash_radius);
    } else {
        return NULL;
    }
}

/* vim: set ts=4 sw=4 et tw=130: */
