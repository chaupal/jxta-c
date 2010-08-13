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

static const char *const __log_cat = "RouterMessage";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_rm.h"
#include "jxta_xml_util.h"
#include "jxta_apa.h"

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    EndpointRouterMessage_,
    Src_,
    Dest_,
    Last_,
    GatewayForward_,
    GatewayReverse_
};

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _EndpointRouterMessage {
    Jxta_advertisement jxta_advertisement;
    char *EndpointRouterMessage;
    Jxta_endpoint_address *Src;
    Jxta_endpoint_address *Dest;
    Jxta_endpoint_address *Last;
    Jxta_vector *reverseGateways;
    Jxta_vector *forwardGateways;
};

static void EndpointRouterMessage_delete(Jxta_object *me);
static Jxta_status validate_message(EndpointRouterMessage * myself);

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleEndpointRouterMessage(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:ERM> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:ERM> : [%pp]\n", myself);
    }
}

static void handleSrc(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Src> Element : [%pp]\n", myself);
    } else {
        JString *src = jstring_new_1(len );

        jstring_append_0( src, cd, len);
        jstring_trim(src);

        if (myself->Src != NULL) {
            JXTA_OBJECT_RELEASE(myself->Src);
        }
        myself->Src = jxta_endpoint_address_new_1(src);

        JXTA_OBJECT_RELEASE(src);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Src> Element : [%pp]\n", myself);
    }
}

static void handleDest(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Dest> Element : [%pp]\n", myself);
    } else {
        JString *dest = jstring_new_1(len );

        jstring_append_0( dest, cd, len);
        jstring_trim(dest);

        if (myself->Dest != NULL) {
            JXTA_OBJECT_RELEASE(myself->Dest);
        }
        myself->Dest = jxta_endpoint_address_new_1(dest);

        JXTA_OBJECT_RELEASE(dest);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Dest> Element : [%pp]\n", myself);
    }
}

static void handleLast(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Last> Element : [%pp]\n", myself);
    } else {
        JString *last = jstring_new_1(len );

        jstring_append_0( last, cd, len);
        jstring_trim(last);

        if (myself->Last != NULL) {
            JXTA_OBJECT_RELEASE(myself->Last);
        }
        myself->Last = jxta_endpoint_address_new_1(last);

        JXTA_OBJECT_RELEASE(last);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Last> Element : [%pp]\n", myself);
    }
}

static void handleGatewayForward(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;
    Jxta_AccessPointAdvertisement *apa = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s <Fwd> \n", 0 == len ? "START" : "FINISH");
    if (0 != len) {
        return;
    }

    apa = jxta_AccessPointAdvertisement_new();
    
    if (NULL == apa) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate APA\n");
        return;
    }

    jxta_advertisement_set_handlers((Jxta_advertisement *) apa, ((Jxta_advertisement *) myself)->parser, (Jxta_advertisement *) myself);
    jxta_vector_add_object_last(myself->forwardGateways, (Jxta_object *) apa);
    JXTA_OBJECT_RELEASE(apa);
}

static void handleGatewayReverse(void *me, const XML_Char * cd, int len)
{
    EndpointRouterMessage *_self = (EndpointRouterMessage *) me;
    Jxta_AccessPointAdvertisement *apa = NULL;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s <Rvs> \n", 0 == len ? "START" : "FINISH");
    if (0 != len) {
        return;
    }

    apa = jxta_AccessPointAdvertisement_new();
    if (NULL == apa) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to allocate APA\n");
        return;
    }

    jxta_advertisement_set_handlers((Jxta_advertisement *) apa, ((Jxta_advertisement *) _self)->parser, (void *) _self);
    jxta_vector_add_object_last(_self->reverseGateways, (Jxta_object *) apa);
    JXTA_OBJECT_RELEASE(apa);
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Src(EndpointRouterMessage * me)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Src != NULL) {
        return JXTA_OBJECT_SHARE(myself->Src);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Src(EndpointRouterMessage * me, Jxta_endpoint_address * src)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Src != NULL) {
        JXTA_OBJECT_RELEASE(myself->Src);
        myself->Src = NULL;
    }
    if (src != NULL) {
        myself->Src = JXTA_OBJECT_SHARE(src);
    }
}

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Dest(EndpointRouterMessage * me)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Dest != NULL) {
        return JXTA_OBJECT_SHARE(myself->Dest);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Dest(EndpointRouterMessage * me, Jxta_endpoint_address * dest)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Dest != NULL) {
        JXTA_OBJECT_RELEASE(myself->Dest);
        myself->Dest = NULL;
    }
    if (dest != NULL) {
        myself->Dest = JXTA_OBJECT_SHARE(dest);
    }
}

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Last(EndpointRouterMessage * me)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Last != NULL) {
        return JXTA_OBJECT_SHARE(myself->Last);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Last(EndpointRouterMessage * me, Jxta_endpoint_address * last)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (myself->Last != NULL) {
        JXTA_OBJECT_RELEASE(myself->Last);
        myself->Last = NULL;
    }
    if (last != NULL) {
        myself->Last = JXTA_OBJECT_SHARE(last);
    }
}

JXTA_DECLARE(Jxta_vector *) EndpointRouterMessage_get_GatewayForward(EndpointRouterMessage * ad)
{
    if (ad->forwardGateways != NULL) {
        JXTA_OBJECT_SHARE(ad->forwardGateways);
        return ad->forwardGateways;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_GatewayForward(EndpointRouterMessage * ad, Jxta_vector * vector)
{
    if (ad->forwardGateways != NULL) {
        JXTA_OBJECT_RELEASE(ad->forwardGateways);
        ad->forwardGateways = NULL;
    }
    if (vector != NULL) {
        ad->forwardGateways = JXTA_OBJECT_SHARE(vector);
    }
    return;
}

JXTA_DECLARE(Jxta_vector *) EndpointRouterMessage_get_GatewayReverse(EndpointRouterMessage * ad)
{
    if (ad->reverseGateways != NULL) {
        JXTA_OBJECT_SHARE(ad->reverseGateways);
        return ad->reverseGateways;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_GatewayReverse(EndpointRouterMessage * ad, Jxta_vector * vector)
{
    if (ad->reverseGateways != NULL) {
        JXTA_OBJECT_RELEASE(ad->reverseGateways);
        ad->reverseGateways = NULL;
    }
    if (vector != NULL) {
        ad->reverseGateways = JXTA_OBJECT_SHARE(vector);
    }
    return;
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab EndpointRouterMessage_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ERM", EndpointRouterMessage_, *handleEndpointRouterMessage, NULL, NULL},
    {"Src", Src_, *handleSrc, NULL, NULL},
    {"Dest", Dest_, *handleDest, NULL, NULL},
    {"Last", Last_, *handleLast, NULL, NULL},
    {"Fwd", GatewayForward_, *handleGatewayForward, NULL, NULL},
    {"Rvs", GatewayReverse_, *handleGatewayReverse, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};
static Jxta_status validate_message(EndpointRouterMessage * myself)
{
    if(myself->Src == NULL ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Src was NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if(myself->Dest == NULL ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Dest was NULL [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) EndpointRouterMessage_get_xml(EndpointRouterMessage * ad, JString ** xml)
{
    JString *string;
    int i = 0;
    int size_fwd = 0;
    int size_rvs = 0;
    char *pt;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:ERM>");

    jstring_append_2(string, "<jxta:ERM>\n");
    jstring_append_2(string, "<Src>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Src)));
    free(pt);
    jstring_append_2(string, "</Src>\n");
    jstring_append_2(string, "<Dest>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Dest)));
    free(pt);
    jstring_append_2(string, "</Dest>\n");
    jstring_append_2(string, "<Last>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Last)));
    free(pt);
    jstring_append_2(string, "</Last>\n");

    size_fwd = jxta_vector_size(ad->forwardGateways);
    if (size_fwd > 0) {
        jstring_append_2(string, "<Fwd>\n");
        for (i = 0; i < size_fwd; ++i) {
            Jxta_AccessPointAdvertisement *apa = NULL;
            JString *apa_xml = NULL;
            Jxta_status res;

            res = jxta_vector_get_object_at(ad->forwardGateways, JXTA_OBJECT_PPTR(&apa), i);
            if ((JXTA_SUCCESS == res) && (NULL != apa)) {
                res = jxta_AccessPointAdvertisement_get_xml(apa, &apa_xml);
                if (NULL != apa_xml) {
                    jstring_append_1(string, apa_xml);
                    JXTA_OBJECT_RELEASE(apa_xml);
                }
                JXTA_OBJECT_RELEASE(apa);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Failed to get APA from forward vector\n");
            }
        }
        jstring_append_2(string, "</Fwd>\n");
    } else {
        jstring_append_2(string, "<Fwd/>\n");
    }

    size_rvs = jxta_vector_size(ad->reverseGateways);
    if (size_rvs > 0) {
        jstring_append_2(string, "<Rvs>\n");
        for (i = 0; i < size_rvs; ++i) {
            Jxta_AccessPointAdvertisement *apa = NULL;
            JString *apa_xml = NULL;
            Jxta_status res;

            res = jxta_vector_get_object_at(ad->reverseGateways, JXTA_OBJECT_PPTR(&apa), i);
            if ((JXTA_SUCCESS == res) && (NULL != apa)) {
                res = jxta_AccessPointAdvertisement_get_xml(apa, &apa_xml);
                if (NULL != apa_xml) {
                    jstring_append_1(string, apa_xml);
                    JXTA_OBJECT_RELEASE(apa_xml);
                }
                JXTA_OBJECT_RELEASE(apa);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Failed to get APA from reverseGateways\n");
            }
        }
        jstring_append_2(string, "</Rvs>\n");
    } else {
        jstring_append_2(string, "<Rvs/>\n");
    }
    jstring_append_2(string, "</jxta:ERM>\n");

    *xml = string;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(EndpointRouterMessage *) EndpointRouterMessage_new(void)
{
    EndpointRouterMessage *myself = (EndpointRouterMessage *) calloc(1, sizeof(EndpointRouterMessage));

    if (NULL == myself) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "EndpointRouterMessage NEW [%pp]\n", myself );

    JXTA_OBJECT_INIT(myself, EndpointRouterMessage_delete, NULL);

    myself = (EndpointRouterMessage *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                  "jxta:ERM",
                                  EndpointRouterMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) EndpointRouterMessage_get_xml,
                                  (JxtaAdvertisementGetIDFunc) NULL, 
                                  (JxtaAdvertisementGetIndexFunc) NULL);

    if ( NULL != myself ) {
        myself->reverseGateways = jxta_vector_new(1);
        myself->forwardGateways = jxta_vector_new(1);
    }

    return myself;
}

static void EndpointRouterMessage_delete(Jxta_object * me)
{
    EndpointRouterMessage * myself = (EndpointRouterMessage * )me;

    if (myself->Src) {
        JXTA_OBJECT_RELEASE(myself->Src);
    }

    if (myself->Dest) {
        JXTA_OBJECT_RELEASE(myself->Dest);
    }

    if (myself->Last) {
        JXTA_OBJECT_RELEASE(myself->Last);
    }

    if (myself->reverseGateways) {
        JXTA_OBJECT_RELEASE(myself->reverseGateways);
    }

    if (myself->forwardGateways) {
        JXTA_OBJECT_RELEASE(myself->forwardGateways);
    }

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xDD, sizeof(EndpointRouterMessage));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "EndpointRouterMessage FREE [%pp]\n", myself );

    free(myself);
}

JXTA_DECLARE(Jxta_status) EndpointRouterMessage_parse_charbuffer(EndpointRouterMessage * ad, const char *buf, int len)
{
    Jxta_status rv = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

JXTA_DECLARE(Jxta_status) EndpointRouterMessage_parse_file(EndpointRouterMessage * ad, FILE * stream)
{
    Jxta_status rv = jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

/* vim: set ts=4 sw=4 et tw=130: */
