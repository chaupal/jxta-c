/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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

static const char *const __log_cat = "PGA";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_svc.h"
#include "jstring.h"
#include "jxta_advertisement.h"
#include "jxta_id.h"
#include "jxta_pga.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_PGA_,
    GID_,
    MSID_,
    Name_,
    Desc_,
    Svc_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_PGA {
    Jxta_advertisement jxta_advertisement;

    Jxta_id *GID;
    Jxta_id *MSID;
    JString *Name;
    JString *Desc;
    Jxta_vector *svclist;
};

/**
 * Forw decl. of unexported function.
 */
static void PGA_delete(Jxta_object *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_PGA(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *myself = (Jxta_PGA *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PGA> : [%pp]\n", myself);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PGA> : [%pp]\n", myself);
    }
}


static void handleGID(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <GID> : [%pp]\n", ad);
        return;
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *gid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&gid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_PGA_set_GID(ad, gid);
            JXTA_OBJECT_RELEASE(gid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid GID %s : [%pp]\n", jstring_get_string(tmp), ad);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <GID> : [%pp]\n", ad);
    }
}

static void handleMSID(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <MSID> : [%pp]\n", ad);
        return;
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *msid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&msid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_PGA_set_MSID(ad, msid);
            JXTA_OBJECT_RELEASE(msid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid MSID %s : [%pp]\n", jstring_get_string(tmp), ad);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <MSID> : [%pp]\n", ad);
    }
}

static void handleName(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Name> : [%pp]\n", ad);
        return;
    } else {
        JString *name = jstring_new_1(len );

        jstring_append_0( name, cd, len);
        jstring_trim(name);

        jxta_PGA_set_Name(ad, name);

        JXTA_OBJECT_RELEASE(name);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Name> : [%pp]\n", ad);
    }
}

static void handleDesc(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Desc> Element : [%pp]\n", ad);
        return;
    } else {
        JString *desc = jstring_new_1(len );

        jstring_append_0( desc, cd, len);
        jstring_trim(desc);

        jxta_PGA_set_Desc(ad, desc);

        JXTA_OBJECT_RELEASE(desc);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Desc> Element : [%pp]\n", ad);
    }
}

static void handleSvc(void *me, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (len == 0) {
        Jxta_svc *svc;
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Svc> Element [%pp]\n", ad );
        
        svc = jxta_svc_new();

        jxta_vector_add_object_last(ad->svclist, (Jxta_object *) svc);

        jxta_advertisement_set_handlers((Jxta_advertisement *) svc, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(svc);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Svc> Element [%pp]\n", ad );
    }
}

JXTA_DECLARE(Jxta_id *) jxta_PGA_get_GID(Jxta_PGA * ad)
{    
    return JXTA_OBJECT_SHARE(ad->GID);
}

static char *JXTA_STDCALL jxta_PGA_get_GID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_PGA *) ad)->GID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PGA_set_GID(Jxta_PGA * ad, Jxta_id * id)
{    
    if( NULL != ad->GID ) {
        JXTA_OBJECT_RELEASE(ad->GID);
        ad->GID = NULL;
    }

    /* NULL may not be set for GID */
    ad->GID = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(Jxta_id *) jxta_PGA_get_MSID(Jxta_PGA * ad)
{    
    return JXTA_OBJECT_SHARE(ad->MSID);
}

static char *JXTA_STDCALL jxta_PGA_get_MSID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_PGA *) ad)->MSID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PGA_set_MSID(Jxta_PGA * ad, Jxta_id * id)
{    
    if( NULL != ad->MSID ) {
        JXTA_OBJECT_RELEASE(ad->MSID);
        ad->MSID = NULL;
    }

    /* NULL may not be set for MSID */
    ad->MSID = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(JString *) jxta_PGA_get_Name(Jxta_PGA * ad)
{
    if( NULL != ad->Name ) {
        return JXTA_OBJECT_SHARE(ad->Name);
    } else {
        return NULL;
    }
}

static char *JXTA_STDCALL jxta_PGA_get_Name_string(Jxta_advertisement * ad)
{
    char * result = NULL;

    if( NULL != ((Jxta_PGA *) ad)->Name ) {
        result = strdup(jstring_get_string(((Jxta_PGA *) ad)->Name));
    }

    return result;
}

JXTA_DECLARE(void) jxta_PGA_set_Name(Jxta_PGA * ad, JString * name)
{
    if( NULL != ad->Name ) {
        JXTA_OBJECT_RELEASE(ad->Name);
        ad->Name = NULL;
    }

    if( ( NULL != name ) && (jstring_length(name)>0)) {
        ad->Name = JXTA_OBJECT_SHARE(name);
    }
}

JXTA_DECLARE(JString *) jxta_PGA_get_Desc(Jxta_PGA * ad)
{
    if( NULL != ad->Desc ) {
        return JXTA_OBJECT_SHARE(ad->Desc);
    } else {
        return NULL;
    }
}

char *JXTA_STDCALL jxta_PGA_get_Desc_string(Jxta_advertisement * ad)
{
    char * result = NULL;

    if( NULL != ((Jxta_PGA *) ad)->Desc ) {
        result = strdup(jstring_get_string(((Jxta_PGA *) ad)->Desc));
    }

    return result;
}

JXTA_DECLARE(void) jxta_PGA_set_Desc(Jxta_PGA * ad, JString * desc)
{
    if( NULL != ad->Desc ) {
        JXTA_OBJECT_RELEASE(ad->Desc);
        ad->Desc = NULL;
    }

    if( ( NULL != desc ) && (jstring_length(desc)>0)) {
        ad->Desc = JXTA_OBJECT_SHARE(desc);
    }
}

/*
 * It would be more convenient if we had an interface to manipulate
 * individual services, but it is not absolutely necessary.
 */
JXTA_DECLARE(Jxta_vector *) jxta_PGA_get_Svc(Jxta_PGA * ad)
{    
    return JXTA_OBJECT_SHARE(ad->svclist);
}

JXTA_DECLARE(Jxta_status) jxta_PGA_get_Svc_with_id(Jxta_PGA * ad, Jxta_id * id, Jxta_svc ** svc)
{
    unsigned int i;
    unsigned int sz;

    *svc = NULL;

    sz = jxta_vector_size(ad->svclist);
    for (i = 0; i < sz; i++) {
        Jxta_svc *tmpsvc = NULL;
        Jxta_id *mcid;
        jxta_vector_get_object_at(ad->svclist, JXTA_OBJECT_PPTR(&tmpsvc), i);
        mcid = jxta_svc_get_MCID(tmpsvc);
        if (jxta_id_equals(mcid, id)) {
            *svc = tmpsvc;
            JXTA_OBJECT_RELEASE(mcid);
            return JXTA_SUCCESS;
        }
        JXTA_OBJECT_RELEASE(mcid);
        JXTA_OBJECT_RELEASE(tmpsvc);
    }
    
    return JXTA_ITEM_NOTFOUND;
}

JXTA_DECLARE(void) jxta_PGA_set_Svc(Jxta_PGA * ad, Jxta_vector * svclist)
{
    if( NULL != ad->svclist ) {
        JXTA_OBJECT_RELEASE(ad->svclist);
        ad->svclist = NULL;
    }

    if( NULL != svclist ) {
        ad->svclist = JXTA_OBJECT_SHARE(svclist);
    }
}

/* This is an internal function so we can assume what best fits us
 * as for the allocation of the returned jstring. We assume it
 * already exists, is passed by invoking code, and we append to it.
 */
static Jxta_status svc_printer(Jxta_PGA * myself, JString * js)
{
    Jxta_status res = JXTA_SUCCESS;
    int sz = jxta_vector_size(myself->svclist);
    int i;

    for (i = 0; i < sz; ++i) {
        Jxta_svc *svc;
        JString *tmp;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "Printing Svc %d [%pp]\n", i, myself );
        
        res = jxta_vector_get_object_at(myself->svclist, JXTA_OBJECT_PPTR(&svc), i);
        if( res != JXTA_SUCCESS ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failure printing svc [%pp]\n", myself );
            break;
        }

        res = jxta_svc_get_xml(svc, &tmp);
        JXTA_OBJECT_RELEASE(svc);
        if( JXTA_SUCCESS == res ) {
            jstring_append_1(js, tmp);
            JXTA_OBJECT_RELEASE(tmp);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failure printing svc [%pp]\n", myself );
        }
        
    }
    
    return res;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_PGA_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PGA", Jxta_PGA_, *handleJxta_PGA, NULL, NULL},
    {"GID", GID_, *handleGID, *jxta_PGA_get_GID_string, NULL},
    {"MSID", MSID_, *handleMSID, *jxta_PGA_get_MSID_string, NULL},
    {"Name", Name_, *handleName, *jxta_PGA_get_Name_string, NULL},
    {"Desc", Desc_, *handleDesc, *jxta_PGA_get_Desc_string, NULL},
    {"Svc", Svc_, *handleSvc, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_PGA_get_xml(Jxta_PGA * ad, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *tmpref = NULL;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_1(8192);

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n" "<!DOCTYPE jxta:PGA>\n" "<jxta:PGA xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(string, "<GID>");
    jxta_id_to_jstring(ad->GID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    tmpref = NULL;
    jstring_append_2(string, "</GID>\n");

    jstring_append_2(string, "<MSID>");
    jxta_id_to_jstring(ad->MSID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    tmpref = NULL;
    jstring_append_2(string, "</MSID>\n");

    if(( NULL != ad->Name ) && (jstring_length(ad->Name) > 0) ) {
        jstring_append_2(string, "<Name>");
        jstring_append_1(string, ad->Name);
        jstring_append_2(string, "</Name>\n");
    }

    if(( NULL != ad->Desc ) && (jstring_length(ad->Desc) > 0) ) {
        jstring_append_2(string, "<Desc>");
        jstring_append_1(string, ad->Desc);
        jstring_append_2(string, "</Desc>\n");
    }

    /* Print services */
    res = svc_printer(ad, string);
    if( JXTA_SUCCESS != res ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failure printing svcs [%pp]\n", ad );
        JXTA_OBJECT_RELEASE(string);
        return res;
    }

    jstring_append_2(string, "</jxta:PGA>\n");

    *xml = string;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_PGA *) jxta_PGA_new()
{
    Jxta_PGA *ad = (Jxta_PGA *) calloc(1, sizeof(Jxta_PGA));
 
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer Group Advertisement NEW [%pp]\n", ad );

    JXTA_OBJECT_INIT(ad, PGA_delete, NULL);

    ad = (Jxta_PGA *) jxta_advertisement_construct((Jxta_advertisement *) ad,
                                  "jxta:PGA",
                                  jxta_PGA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_PGA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_PGA_get_GID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_PGA_get_indexes );
                                  
    if( NULL != ad ) {                                      
        ad->GID = JXTA_OBJECT_SHARE(jxta_id_nullID);
        ad->MSID = JXTA_OBJECT_SHARE(jxta_id_nullID);
        ad->Name = jstring_new_0();
        ad->Desc = jstring_new_0();
        ad->svclist = jxta_vector_new(3);
    }

    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void PGA_delete(Jxta_object * me)
{
    Jxta_PGA *ad = (Jxta_PGA *) me;

    JXTA_OBJECT_RELEASE(ad->GID);
    JXTA_OBJECT_RELEASE(ad->MSID);
    if( NULL != ad->Name ) {
        JXTA_OBJECT_RELEASE(ad->Name);
        ad->Name = NULL;
    }
    
    if( NULL != ad->Desc ) {
        JXTA_OBJECT_RELEASE(ad->Desc);
        ad->Desc = NULL;
    }
    
    if( NULL != ad->svclist ) {
        JXTA_OBJECT_RELEASE(ad->svclist);
        ad->svclist = NULL;
    }


    jxta_advertisement_destruct((Jxta_advertisement *) ad);
    
    memset(ad, 0xdd, sizeof(Jxta_PGA));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Peer Advertisement FREE [%pp]\n", ad );

    free(ad);
}

JXTA_DECLARE(Jxta_vector *) jxta_PGA_get_indexes(Jxta_advertisement * dummy)
{
    /** 
    * Define the indexed fields
    */
    const char *PGA_idx[][2] = {
        {"Name", NULL},
        {"Name", "NameAttribute"},
        {"GID", NULL},
        {"Desc", NULL},
        {"MSID", NULL},
        {NULL, NULL}
    };

    return jxta_advertisement_return_indexes(PGA_idx[0]);
}

JXTA_DECLARE(Jxta_status) jxta_PGA_parse_charbuffer(Jxta_PGA * ad, const char *buf, int len)
{
    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

/** The main external method used to extract 
 * the data from the xml stream.
 */

JXTA_DECLARE(Jxta_status) jxta_PGA_parse_file(Jxta_PGA * ad, FILE * stream)
{
    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vim: set ts=4 sw=4 et tw=130: */
