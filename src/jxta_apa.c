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

static const char *__log_cat = "APA";

#include <stdio.h>
#include <string.h>

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jdlist.h"
#include "jxta_apa.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_AccessPointAdvertisement_,
    PID_,
    EA_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_AccessPointAdvertisement {
    Jxta_advertisement jxta_advertisement;
    Jxta_id *PID;
    Jxta_vector *endpoints;
};

/* Forw decl for un-exported function */
static void AccessPointAdvertisement_delete(Jxta_object *);
static Jxta_status validate_message(Jxta_AccessPointAdvertisement * myself);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_AccessPointAdvertisement(void *me, const XML_Char * cd, int len)
{
    Jxta_AccessPointAdvertisement *myself = (Jxta_AccessPointAdvertisement *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:APA> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:APA> : [%pp]\n", myself);
    }
}

static void handlePID(void *me, const XML_Char * cd, int len)
{
    Jxta_AccessPointAdvertisement *ad = (Jxta_AccessPointAdvertisement *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <PID> : [%pp]\n", ad);
        return;
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *pid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&pid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_AccessPointAdvertisement_set_PID(ad, pid);
            JXTA_OBJECT_RELEASE(pid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid PID %s : [%pp]\n", jstring_get_string(tmp), ad);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <PID> : [%pp]\n", ad);
    }
}

static void handleEndpoint(void *userdata, const XML_Char * cd, int len)
{
    Jxta_AccessPointAdvertisement *ad = (Jxta_AccessPointAdvertisement *) userdata;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <EA> : [%pp]\n", ad);
        return;
    } else {
        JString *endpoint_address = jstring_new_1(len);
        jstring_append_0(endpoint_address, cd, len);
        jstring_trim(endpoint_address);

        if (jstring_length(endpoint_address) > 0) {
            jxta_vector_add_object_last(ad->endpoints, (Jxta_object *) endpoint_address);
        }

        JXTA_OBJECT_RELEASE(endpoint_address);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <EA> : [%pp]\n", ad);
    }
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

JXTA_DECLARE(Jxta_id *) jxta_AccessPointAdvertisement_get_PID(Jxta_AccessPointAdvertisement * ad)
{
    if (ad->PID != NULL) {
        return JXTA_OBJECT_SHARE(ad->PID);
    } else {
        return NULL;
    }
}


JXTA_DECLARE(void) jxta_AccessPointAdvertisement_set_PID(Jxta_AccessPointAdvertisement * ad, Jxta_id * id)
{
    if (ad->PID != NULL) {
        JXTA_OBJECT_RELEASE(ad->PID);
        ad->PID = NULL;
    }

    if ( NULL != id ) {
        ad->PID = JXTA_OBJECT_SHARE(id);
    }
}

static char *JXTA_STDCALL jxta_AccessPointAdvertisement_get_PID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_AccessPointAdvertisement *) ad)->PID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;

}

JXTA_DECLARE(Jxta_vector *) jxta_AccessPointAdvertisement_get_EndpointAddresses(Jxta_AccessPointAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->endpoints);
    return ad->endpoints;
}

JXTA_DECLARE(void) jxta_AccessPointAdvertisement_set_EndpointAddresses(Jxta_AccessPointAdvertisement * ad,
                                                                       Jxta_vector * addresses)
{
    JXTA_OBJECT_SHARE(addresses);
    JXTA_OBJECT_RELEASE(ad->endpoints);
    ad->endpoints = addresses;
}

JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_add_EndpointAddress(Jxta_AccessPointAdvertisement * me,
                                                                            Jxta_endpoint_address * ea)
{
    char *cstr;
    JString *jstr;

    if (NULL == me->endpoints) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "unable to add EA to NULL endpoints list\n");
        return JXTA_INVALID_ARGUMENT; 
    }

    cstr = jxta_endpoint_address_get_transport_addr(ea);
    if (NULL == cstr) {
        return JXTA_INVALID_ARGUMENT;
    }

    jstr = jstring_new_2(cstr);
    free(cstr);
    if (NULL == jstr) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate a JString, not enough memory?\n");
        return JXTA_NOMEM;
    }
    jxta_vector_add_object_last(me->endpoints, (Jxta_object *) jstr);
    JXTA_OBJECT_RELEASE(jstr);

    return JXTA_SUCCESS;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_AccessPointAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:APA", Jxta_AccessPointAdvertisement_, *handleJxta_AccessPointAdvertisement, NULL, NULL},
    {"PID", PID_, *handlePID, *jxta_AccessPointAdvertisement_get_PID_string, NULL},
    {"EA", EA_, *handleEndpoint, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/* This being an internal call, we chose a behaviour for handling the jstring
 * that's better suited to our need: we are passed an existing jstring
 * and append to it.
 */

Jxta_status endpoints_printer(Jxta_AccessPointAdvertisement * ad, JString * js)
{
    int sz = jxta_vector_size(ad->endpoints);
    int i;

    for (i = 0; i < sz; ++i) {
        JString *endpoint;

        jxta_vector_get_object_at(ad->endpoints, JXTA_OBJECT_PPTR(&endpoint), i);
        jstring_append_2(js, "<EA>");
        jstring_append_1(js, endpoint);
        jstring_append_2(js, "</EA>\n");
        JXTA_OBJECT_RELEASE(endpoint);
    }
    
    return JXTA_SUCCESS;
}
static Jxta_status validate_message(Jxta_AccessPointAdvertisement * myself)
{
    if (myself->PID == NULL ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "PID is NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if (myself->endpoints == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "endpoints are NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_get_xml(Jxta_AccessPointAdvertisement * ad, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;

    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();    
    
    jstring_append_2(string, "<jxta:APA xmlns:jxta=\"http://jxta.org\">\n");

    if (ad->PID != NULL) {
        JString *tmpref = NULL;
        jstring_append_2(string, "<PID>");
        jxta_id_to_jstring(ad->PID, &tmpref);
        jstring_append_1(string, tmpref);
        JXTA_OBJECT_RELEASE(tmpref);
        jstring_append_2(string, "</PID>\n");
    }

    res = endpoints_printer(ad, string);
    if( JXTA_SUCCESS != res ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed generating <EA>s. [%pp]\n", ad);
        JXTA_OBJECT_RELEASE(string);
        return res;
    }

    jstring_append_2(string, "</jxta:APA>\n");

    *xml = string;
    return res;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_AccessPointAdvertisement *) jxta_AccessPointAdvertisement_new(void)
{
    Jxta_AccessPointAdvertisement *myself = (Jxta_AccessPointAdvertisement *) calloc(1, sizeof(Jxta_AccessPointAdvertisement));
    
    if( NULL == myself ) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, AccessPointAdvertisement_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta:APA NEW [%pp]\n", myself );    

    myself = (Jxta_AccessPointAdvertisement *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                  "jxta:APA",
                                  Jxta_AccessPointAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_AccessPointAdvertisement_get_xml,
                                  NULL,
                                  (JxtaAdvertisementGetIndexFunc) jxta_AccessPointAdvertisement_get_indexes );

    if( NULL != myself ) {
        myself->PID = NULL;
        myself->endpoints = jxta_vector_new(3);
    }

    return myself;
}

static void AccessPointAdvertisement_delete(Jxta_object * me)
{
    Jxta_AccessPointAdvertisement *myself = (Jxta_AccessPointAdvertisement *) me;

    if (myself->PID != NULL)
        JXTA_OBJECT_RELEASE(myself->PID);

    if (myself->endpoints != NULL)
        JXTA_OBJECT_RELEASE(myself->endpoints);

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_AccessPointAdvertisement));
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta:APA FREE [%pp]\n", myself );    
    
    free(myself);
}


JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_parse_charbuffer(Jxta_AccessPointAdvertisement * ad, const char *buf, int len)
{
    JXTA_OBJECT_CHECK_VALID(ad);
    Jxta_status rv = JXTA_SUCCESS;
    rv = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_parse_file(Jxta_AccessPointAdvertisement * ad, FILE * stream)
{
    JXTA_OBJECT_CHECK_VALID(ad);
    Jxta_status rv = JXTA_SUCCESS;
    rv = jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

JXTA_DECLARE(Jxta_vector *) jxta_AccessPointAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"PID", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

/* vi: set ts=4 sw=4 tw=130 et: */
