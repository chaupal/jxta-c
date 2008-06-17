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

static const char *__log_cat = "RA";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jdlist.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_RouteAdvertisement_,
    DESTPID_,
    DEST_,
    HOPS_,
    APA_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_RouteAdvertisement {

    Jxta_advertisement jxta_advertisement;

    Jxta_id * pid;
    Jxta_AccessPointAdvertisement *dest;
    Jxta_vector *hops;
    
    /* Used during parsing */
    enum apa_locations { NOWHERE, IN_DST, IN_HOPS } apa_location;
};

/* Forw decl for un-exported function */
static void RouteAdvertisement_delete(Jxta_object * me);
static char *JXTA_STDCALL jxta_RouteAdvertisement_get_DestPID_string(Jxta_advertisement * ad);
static Jxta_status hops_printer(Jxta_RouteAdvertisement * ad, JString * js);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_RouteAdvertisement(void *me, const XML_Char * cd, int len)
{
    Jxta_RouteAdvertisement *myself = (Jxta_RouteAdvertisement *) me;

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:RA> : [%pp]\n", myself);
    } else {
        if( NULL != myself->pid ) {
            jxta_AccessPointAdvertisement_set_PID(myself->dest, myself->pid);
        }
    
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:RA> : [%pp]\n", myself);
    }
}

static void handleDest(void *me, const XML_Char * cd, int len)
{
    Jxta_RouteAdvertisement *ad = (Jxta_RouteAdvertisement *) me;

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Dst> Element : [%pp]\n", ad);
        
        ad->apa_location = IN_DST;
    } else {
        ad->apa_location = NOWHERE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Dst> Element : [%pp]\n", ad);
    }
}

static void handleAPA(void *me, const XML_Char * cd, int len) {
    Jxta_RouteAdvertisement *ad = (Jxta_RouteAdvertisement *) me;

    if (len == 0) {
        Jxta_AccessPointAdvertisement *apa = jxta_AccessPointAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:APA> Element : [%pp]\n", ad);

        switch( ad->apa_location ) {
            case NOWHERE :
                /* XXX 20060823 bondolo This causes a leak but there doesn't seem to be any other choice. */
                JXTA_OBJECT_SHARE(apa);
                break;
                
           case IN_DST : 
                jxta_RouteAdvertisement_set_Dest( ad, apa );
                break;
                
           case IN_HOPS : 
                jxta_vector_add_object_last(ad->hops, (Jxta_object *) apa);
                break;
        }
            
        jxta_advertisement_set_handlers((Jxta_advertisement *) apa, ((Jxta_advertisement *) ad)->parser, (Jxta_advertisement *) ad);
        
        JXTA_OBJECT_RELEASE(apa);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:APA> Element : [%pp]\n", ad);
    }
}

static void handleDestPID(void *me, const XML_Char * cd, int len)
{
    Jxta_RouteAdvertisement *ad = (Jxta_RouteAdvertisement *) me;

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <DstPID> Element : [%pp]\n", ad);
        return;
    } else {
        JString *tmp = jstring_new_1(len);
#ifdef UNUSED_VWF
        Jxta_id *pid = NULL;
#endif
        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        jxta_id_from_jstring(&ad->pid, tmp);
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <DstPID> Element : [%pp]\n", ad);
    }
}

static void handleHops(void *me, const XML_Char * cd, int len)
{
    Jxta_RouteAdvertisement *ad = (Jxta_RouteAdvertisement *) me;

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Hops> Element : [%pp]\n", ad);
        
        ad->apa_location = IN_HOPS;
    } else {
        ad->apa_location = NOWHERE;
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Hops> Element : [%pp]\n", ad);
    }
}


/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(Jxta_AccessPointAdvertisement *) jxta_RouteAdvertisement_get_Dest(Jxta_RouteAdvertisement * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->dest != NULL) {
        if( NULL != ad->pid ) {
            /* Set the APA PID from our DstPID if it's available. */
            jxta_AccessPointAdvertisement_set_PID( ad->dest, ad->pid );
        }
    
        return JXTA_OBJECT_SHARE(ad->dest);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) jxta_RouteAdvertisement_set_Dest(Jxta_RouteAdvertisement * ad, Jxta_AccessPointAdvertisement * dst)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if (ad->dest != NULL) {
        JXTA_OBJECT_RELEASE(ad->dest);
        ad->dest = NULL;
    }
    
    if( NULL != dst ) {
        ad->dest = JXTA_OBJECT_SHARE(dst);
    }
}

JXTA_DECLARE(Jxta_id *) jxta_RouteAdvertisement_get_DestPID(Jxta_RouteAdvertisement * ad)
{
    Jxta_id * result;

    JXTA_OBJECT_CHECK_VALID(ad);

    if ((NULL == ad->pid) && ( NULL != ad->dest )) {
        /* If we don't have a defined DstPID get the pid from the Dst APA (assuming it has one) */
        ad->pid = jxta_AccessPointAdvertisement_get_PID( ad->dest );
    }
        
    result = ad->pid;
    
    if( NULL != result ) {
        result = JXTA_OBJECT_SHARE(result);
    }
    
    return result;
}

static char *JXTA_STDCALL jxta_RouteAdvertisement_get_DestPID_string(Jxta_advertisement * me)
{
    Jxta_RouteAdvertisement *ad = (Jxta_RouteAdvertisement *) me;
    Jxta_id * pid;
    char *res = NULL;

    JXTA_OBJECT_CHECK_VALID(ad);
    
    pid = jxta_RouteAdvertisement_get_DestPID(ad);

    if (pid != NULL) {
        JString *js = NULL;

        jxta_id_to_jstring(ad->pid, &js);
        
        if( NULL != js ) {
            res = strdup(jstring_get_string(js));
            JXTA_OBJECT_RELEASE(js);
        }
        JXTA_OBJECT_RELEASE(pid);
    }

    return res;
}

JXTA_DECLARE(void) jxta_RouteAdvertisement_set_DestPID(Jxta_RouteAdvertisement * ad, Jxta_id * id)
{
    if (ad->pid != NULL) {
        JXTA_OBJECT_RELEASE(ad->pid);
        ad->pid = NULL;
    }
    
    if( NULL != id ) {
        ad->pid = JXTA_OBJECT_SHARE(id);
    }
}

JXTA_DECLARE(Jxta_vector *)  jxta_RouteAdvertisement_get_Hops(Jxta_RouteAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->hops);
}

JXTA_DECLARE(void) jxta_RouteAdvertisement_set_Hops(Jxta_RouteAdvertisement * ad, Jxta_vector * hops)
{
    JXTA_OBJECT_RELEASE(ad->hops);
    ad->hops = JXTA_OBJECT_SHARE(hops);
}

JXTA_DECLARE(void) jxta_RouteAdvertisement_add_first_hop(Jxta_RouteAdvertisement * ad, Jxta_AccessPointAdvertisement * hop)
{
    jxta_vector_add_object_first(ad->hops, (Jxta_object *) hop);
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_RouteAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RA", Jxta_RouteAdvertisement_, *handleJxta_RouteAdvertisement, NULL, NULL},
    {"DstPID", DESTPID_, *handleDestPID, jxta_RouteAdvertisement_get_DestPID_string, NULL},
    {"Dst", DEST_, *handleDest, NULL, NULL},
    {"Hops", HOPS_, *handleHops, NULL, NULL},
    {"jxta:APA", APA_, *handleAPA, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/* This being an internal call, we chose a behaviour for handling the jstring
 * that's better suited to our need: we are passed an existing jstring
 * and append to it.
 */

static Jxta_status hops_printer(Jxta_RouteAdvertisement * ad, JString * js)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int sz = jxta_vector_size(ad->hops);
    unsigned int i;
    
    if( 0 == sz ) {
        return JXTA_SUCCESS;
    }

    jstring_append_2(js, "<Hops>\n");
    for (i = 0; i < sz; ++i) {
        JString *tmp = NULL;
        Jxta_AccessPointAdvertisement *hop = NULL;

        jxta_vector_get_object_at(ad->hops, JXTA_OBJECT_PPTR(&hop), i);
        if( JXTA_SUCCESS != res ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed retrieving <jxta:APA> %d. [%pp]\n", i, ad);
            return res;
        }
        res = jxta_AccessPointAdvertisement_get_xml(hop, &tmp);
        if( JXTA_SUCCESS != res ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed generating <jxta:APA>. [%pp]\n", hop);
            return res;
        }
        jstring_append_1(js, tmp);
        JXTA_OBJECT_RELEASE(tmp);
        
        JXTA_OBJECT_RELEASE(hop);
    }
    jstring_append_2(js, "</Hops>\n");
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_RouteAdvertisement_get_xml(Jxta_RouteAdvertisement * ad, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *tmpref = NULL;
    Jxta_id *pid;
    Jxta_boolean wrote_DstPID = FALSE;
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    if( NULL == ad->dest ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Dst not defined. [%pp]\n", ad);
        return JXTA_INVALID_ARGUMENT;        
    }

    string = jstring_new_0();

    jstring_append_2(string, "<jxta:RA xmlns:jxta=\"http://jxta.org\">\n");
    
    pid = jxta_RouteAdvertisement_get_DestPID(ad);

    if( NULL != pid ) {
        jstring_append_2(string, "<DstPID>");
        res = jxta_id_to_jstring( pid, &tmpref);
        JXTA_OBJECT_RELEASE(pid);
        if( JXTA_SUCCESS != res ) {           
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed generating <DstPID>. [%pp]\n", ad);
            JXTA_OBJECT_RELEASE(string);
            return res;
        }
        jstring_append_1(string, tmpref);
        jstring_append_2(string, "</DstPID>");
        JXTA_OBJECT_RELEASE(tmpref);
        tmpref = NULL;

        /* Since we are sending the peer id in the <DstPID> tag we can remove it in the <Dst> tag. */
        wrote_DstPID = TRUE;
    }

    jstring_append_2(string, "<Dst>");
    if( wrote_DstPID ) {
        jxta_AccessPointAdvertisement_set_PID( ad->dest, NULL );
    }
    res = jxta_AccessPointAdvertisement_get_xml(ad->dest, &tmpref);
    if( JXTA_SUCCESS != res ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed generating <Dst>. [%pp]\n", ad);
        JXTA_OBJECT_RELEASE(string);
        return res;
    }
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    jstring_append_2(string, "</Dst>\n");

    res = hops_printer(ad, string);
    if( JXTA_SUCCESS != res ) {
        JXTA_OBJECT_RELEASE(string);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failed generating <Hops>. [%pp]\n", ad);
        return res;
    }

    jstring_append_2(string, "</jxta:RA>\n");

    *xml = string;
    
    return res;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_RouteAdvertisement_new(void)
{
    Jxta_RouteAdvertisement *myself = (Jxta_RouteAdvertisement *) calloc(1, sizeof(Jxta_RouteAdvertisement));
   
    if( NULL == myself ) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, RouteAdvertisement_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta:RA NEW [%pp]\n", myself );    

    myself = (Jxta_RouteAdvertisement *) jxta_advertisement_construct((Jxta_advertisement *) myself,
                                  "jxta:RA",
                                  Jxta_RouteAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_RouteAdvertisement_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_RouteAdvertisement_get_DestPID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_RouteAdvertisement_get_indexes );

    if( NULL != myself ) {
        myself->pid = NULL;
        myself->dest = NULL;
        myself->hops = jxta_vector_new(3);
    }

    return myself;
}

static void RouteAdvertisement_delete(Jxta_object * me)
{
    Jxta_RouteAdvertisement *myself = (Jxta_RouteAdvertisement *) me;

    if (myself->pid != NULL) {
        JXTA_OBJECT_RELEASE(myself->pid);
    }

    if (myself->dest != NULL) {
        JXTA_OBJECT_RELEASE(myself->dest);
    }

    JXTA_OBJECT_RELEASE(myself->hops);

    jxta_advertisement_destruct((Jxta_advertisement *) myself);

    memset(myself, 0xdd, sizeof(Jxta_RouteAdvertisement));
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "jxta:RA FREE [%pp]\n", myself );    
    
    free(myself);
}

JXTA_DECLARE(Jxta_status) jxta_RouteAdvertisement_parse_charbuffer(Jxta_RouteAdvertisement * ad, const char *buf, int len)
{
    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(Jxta_status) jxta_RouteAdvertisement_parse_file(Jxta_RouteAdvertisement * ad, FILE * stream)
{
    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *)
    jxta_RouteAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"DstPID", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

/* vi: set ts=4 sw=4 tw=130 et: */
