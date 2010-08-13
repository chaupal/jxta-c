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

static const char *__log_cat = "DiffusionMsg";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jstring.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

#include "jxta_rdv_diffusion_msg.h"

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/
struct _Jxta_rdv_diffusion {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *src_peer_id;

    Jxta_rdv_diffusion_policy policy;
    Jxta_rdv_diffusion_scope scope;

    unsigned int ttl;

    JString *target_hash;

    JString *dest_svc_name;
    JString *dest_svc_param;
};

const char JXTA_RDV_DIFFUSION_ELEMENT_NAME[] = "RdvDiffusion";

static void diffusion_delete(Jxta_object * me);
static void handle_rdv_diffusion(void *me, const XML_Char * cd, int len);
static void handle_target_hash(void *userdata, const XML_Char * cd, int len);
static void handle_dest_svc_name(void *userdata, const XML_Char * cd, int len);
static void handle_dest_svc_param(void *userdata, const XML_Char * cd, int len);
static Jxta_status validate_message(Jxta_rdv_diffusion * myself);

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    Jxta_rdv_diffusion_,
    TargetHash_,
    DestSvcName_,
    DestSvcParam_
};

/** 
 * Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, diffusion through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_rdv_diffusion_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RdvDiffusion", Jxta_rdv_diffusion_, *handle_rdv_diffusion, NULL, NULL},
    {"TargetHash", TargetHash_, *handle_target_hash, NULL, NULL},
    {"DestSvcName", DestSvcName_, *handle_dest_svc_name, NULL, NULL},
    {"DestSvcParam", DestSvcParam_, *handle_dest_svc_param, NULL, NULL},
    {NULL, 0, NULL, NULL, NULL}
};

    /** Get a new instance of the ad.
     */
JXTA_DECLARE(Jxta_rdv_diffusion *) jxta_rdv_diffusion_new(void)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) calloc(1, sizeof(Jxta_rdv_diffusion));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_rdv_diffusion NEW [%pp]\n", myself);

    JXTA_OBJECT_INIT(myself, diffusion_delete, NULL);

    myself = (Jxta_rdv_diffusion *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                                                 "jxta:RdvDiffusion",
                                                                 jxta_rdv_diffusion_tags,
                                                                 (JxtaAdvertisementGetXMLFunc) jxta_rdv_diffusion_get_xml,
                                                                 (JxtaAdvertisementGetIDFunc) NULL,
                                                                 (JxtaAdvertisementGetIndexFunc) NULL);

    if (NULL != myself) {
        myself->src_peer_id = JXTA_OBJECT_SHARE(jxta_id_nullID);
        myself->policy = JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL;
        myself->scope = JXTA_RDV_DIFFUSION_SCOPE_TERMINAL;
        myself->ttl = 0;
        myself->dest_svc_name = NULL;
        myself->dest_svc_param = NULL;
    }

    return myself;
}

static void diffusion_delete(Jxta_object * me)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) me;

    /* Fill in the required freeing functions here. */
    if (myself->src_peer_id) {
        JXTA_OBJECT_RELEASE(myself->src_peer_id);
        myself->src_peer_id = NULL;
    }

    if (myself->dest_svc_name) {
        JXTA_OBJECT_RELEASE(myself->dest_svc_name);
        myself->dest_svc_name = NULL;
    }

    if (myself->dest_svc_param) {
        JXTA_OBJECT_RELEASE(myself->dest_svc_param);
        myself->dest_svc_param = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_rdv_diffusion));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Jxta_rdv_diffusion FREE [%pp]\n", myself);

    free(myself);
}

JXTA_DECLARE(Jxta_id *) jxta_rdv_diffusion_get_src_peer_id(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return JXTA_OBJECT_SHARE(myself->src_peer_id);
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_src_peer_id(Jxta_rdv_diffusion * myself, Jxta_id * src_peer_id)
{
    JXTA_OBJECT_CHECK_VALID(myself);
    JXTA_OBJECT_CHECK_VALID(src_peer_id);

    JXTA_OBJECT_RELEASE(myself->src_peer_id);

    myself->src_peer_id = JXTA_OBJECT_SHARE(src_peer_id);
}

JXTA_DECLARE(Jxta_rdv_diffusion_policy) jxta_rdv_diffusion_get_policy(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->policy;
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_policy(Jxta_rdv_diffusion * myself, Jxta_rdv_diffusion_policy policy)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->policy = policy;
}

JXTA_DECLARE(Jxta_rdv_diffusion_scope) jxta_rdv_diffusion_get_scope(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->scope;
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_scope(Jxta_rdv_diffusion * myself, Jxta_rdv_diffusion_scope scope)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->scope = scope;
}

JXTA_DECLARE(unsigned int) jxta_rdv_diffusion_get_ttl(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    return myself->ttl;
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_ttl(Jxta_rdv_diffusion * myself, unsigned int ttl)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    myself->ttl = ttl;
}

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_target_hash(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->target_hash) {
        jstring_trim(myself->target_hash);

        return jstring_get_string(myself->target_hash);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_target_hash(Jxta_rdv_diffusion * myself, const char *target_hash)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->target_hash != NULL) {
        JXTA_OBJECT_RELEASE(myself->target_hash);
        myself->target_hash = NULL;
    }

    if (target_hash != NULL) {
        myself->target_hash = jstring_new_2(target_hash);
        jstring_trim(myself->target_hash);
    }
}

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_dest_svc_name(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->dest_svc_name) {
        jstring_trim(myself->dest_svc_name);

        return jstring_get_string(myself->dest_svc_name);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_dest_svc_name(Jxta_rdv_diffusion * myself, const char *svc_name)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->dest_svc_name != NULL) {
        JXTA_OBJECT_RELEASE(myself->dest_svc_name);
        myself->dest_svc_name = NULL;
    }

    if (svc_name != NULL) {
        myself->dest_svc_name = jstring_new_2(svc_name);
        jstring_trim(myself->dest_svc_name);
    }
}

JXTA_DECLARE(const char *) jxta_rdv_diffusion_get_dest_svc_param(Jxta_rdv_diffusion * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL != myself->dest_svc_param) {
        jstring_trim(myself->dest_svc_param);

        return jstring_get_string(myself->dest_svc_param);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_rdv_diffusion_set_dest_svc_param(Jxta_rdv_diffusion * myself, const char *svc_params)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->dest_svc_param != NULL) {
        JXTA_OBJECT_RELEASE(myself->dest_svc_param);
        myself->dest_svc_param = NULL;
    }

    if (svc_params != NULL) {
        myself->dest_svc_param = jstring_new_2(svc_params);
        jstring_trim(myself->dest_svc_param);
    }
}

static Jxta_status validate_message(Jxta_rdv_diffusion * myself)
{

    JXTA_OBJECT_CHECK_VALID(myself);

    if (jxta_id_equals(myself->src_peer_id, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Source peer id must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ((jxta_rdv_diffusion_get_policy(myself) < JXTA_RDV_DIFFUSION_POLICY_BROADCAST)
        || (jxta_rdv_diffusion_get_policy(myself) > JXTA_RDV_DIFFUSION_POLICY_TRAVERSAL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Illegal policy value %d [%pp]\n", jxta_rdv_diffusion_get_policy(myself),
                        myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if ((jxta_rdv_diffusion_get_scope(myself) < JXTA_RDV_DIFFUSION_SCOPE_GLOBAL)
        || (jxta_rdv_diffusion_get_scope(myself) > JXTA_RDV_DIFFUSION_SCOPE_TERMINAL)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Illegal scope value %d [%pp]\n", jxta_rdv_diffusion_get_scope(myself),
                        myself);
        return JXTA_INVALID_ARGUMENT;
    }

    if (NULL == jxta_rdv_diffusion_get_dest_svc_name(myself)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Service Name must not be NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_get_xml(Jxta_rdv_diffusion * myself, JString ** xml)
{
    JString *string;
    JString *temp;
    char buf[12];               /* We use this buffer to store a string representation of a int < 10 */

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:RdvDiffusion>\n");

    jstring_append_2(string, "<jxta:RdvDiffusion");
    jstring_append_2(string, " src_peer_id=\"");
    jxta_id_to_jstring(myself->src_peer_id, &temp);
    jstring_append_1(string, temp);
    JXTA_OBJECT_RELEASE(temp);
    jstring_append_2(string, "\"");
    jstring_append_2(string, " policy=\"");
    apr_snprintf(buf, sizeof(buf), "%d", jxta_rdv_diffusion_get_policy(myself));
    jstring_append_2(string, buf);
    jstring_append_2(string, "\"");
    jstring_append_2(string, " scope=\"");
    apr_snprintf(buf, sizeof(buf), "%d", jxta_rdv_diffusion_get_scope(myself));
    jstring_append_2(string, buf);
    jstring_append_2(string, "\"");
    if (0 != jxta_rdv_diffusion_get_ttl(myself)) {
        jstring_append_2(string, " ttl=\"");
        apr_snprintf(buf, sizeof(buf), "%d", jxta_rdv_diffusion_get_ttl(myself));
        jstring_append_2(string, buf);
        jstring_append_2(string, "\"");
    }
    jstring_append_2(string, ">\n");

    if (NULL != jxta_rdv_diffusion_get_target_hash(myself)) {
        jstring_append_2(string, "<TargetHash>");
        jstring_append_2(string, jxta_rdv_diffusion_get_target_hash(myself));
        jstring_append_2(string, "</TargetHash>\n");
    }

    jstring_append_2(string, "<DestSvcName>");
    jstring_append_2(string, jxta_rdv_diffusion_get_dest_svc_name(myself));
    jstring_append_2(string, "</DestSvcName>\n");

    if (NULL != jxta_rdv_diffusion_get_dest_svc_param(myself)) {
        jstring_append_2(string, "<DestSvcParam>");
        jstring_append_2(string, jxta_rdv_diffusion_get_dest_svc_param(myself));
        jstring_append_2(string, "</DestSvcParam>\n");
    }

    jstring_append_2(string, "</jxta:RdvDiffusion>\n");

    *xml = string;
    return JXTA_SUCCESS;
}

/** 
* Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/

static void handle_rdv_diffusion(void *me, const XML_Char * cd, int len)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (0 == len) {
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "START <jxta:RdvDiffusion> [%pp]\n", myself);

        /** handle attributes */
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "src_peer_id")) {
                JString *idstr = jstring_new_2(atts[1]);
                jstring_trim(idstr);

                if (JXTA_SUCCESS != jxta_id_from_jstring(&myself->src_peer_id, idstr)) {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "ID parse failure for src_peer_id [%pp]\n",
                                    myself);
                    myself->src_peer_id = NULL;
                }
                JXTA_OBJECT_RELEASE(idstr);
            } else if (0 == strcmp(*atts, "policy")) {
                myself->policy = (Jxta_rdv_diffusion_policy) atoi(atts[1]);
            } else if (0 == strcmp(*atts, "scope")) {
                myself->scope = (Jxta_rdv_diffusion_scope) atoi(atts[1]);
            } else if (0 == strcmp(*atts, "ttl")) {
                myself->ttl = atoi(atts[1]);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "FINISH <jxta:RdvDiffusion> [%pp]\n", myself);
    }
}

static void handle_target_hash(void *me, const XML_Char * cd, int len)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <TargetHash> : [%pp]\n", myself);
    } else {
        JString *string = jstring_new_1(len);

        jstring_append_0(string, cd, len);
        jstring_trim(string);

        jxta_rdv_diffusion_set_target_hash(myself, jstring_get_string(string));

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <TargetHash> : [%pp]\n", myself);
    }
}

static void handle_dest_svc_name(void *me, const XML_Char * cd, int len)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <DstSvcName> : [%pp]\n", myself);
    } else {
        JString *string = jstring_new_1(len);

        jstring_append_0(string, cd, len);
        jstring_trim(string);

        jxta_rdv_diffusion_set_dest_svc_name(myself, jstring_get_string(string));

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <DstSvcName> : [%pp]\n", myself);
    }
}

static void handle_dest_svc_param(void *me, const XML_Char * cd, int len)
{
    Jxta_rdv_diffusion *myself = (Jxta_rdv_diffusion *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <DstSvcParam> : [%pp]\n", myself);
    } else {
        JString *string = jstring_new_1(len);

        jstring_append_0(string, cd, len);
        jstring_trim(string);

        jxta_rdv_diffusion_set_dest_svc_param(myself, jstring_get_string(string));

        JXTA_OBJECT_RELEASE(string);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <DstSvcParam> : [%pp]\n", myself);
    }
}

JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_parse_charbuffer(Jxta_rdv_diffusion * myself, const char *buf, int len)
{
    Jxta_status res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);

    if (JXTA_SUCCESS == res) {
        res = validate_message(myself);
    }

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_rdv_diffusion_parse_file(Jxta_rdv_diffusion * myself, FILE * stream)
{
    Jxta_status res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);

    if (JXTA_SUCCESS == res) {
        res = validate_message(myself);
    }

    return res;
}

/* vim: set ts=4 sw=4 et tw=130: */
