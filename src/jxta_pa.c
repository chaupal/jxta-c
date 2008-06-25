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

static const char *const __log_cat = "PA";

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <apr.h>

#include "jxta_log.h"
#include "jxta_errno.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_rdv.h"
#include "jxta_id_uuid_priv.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_PA_,
    PID_,
    GID_,
    Name_,
    Desc_,
    SN_,
    Svc_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_PA {
    Jxta_advertisement jxta_advertisement;
    
    Jxta_id *PID;
    Jxta_id *GID;
    JString *Name;
    JString *Desc;
    Jxta_version *protocol_version;
    Jxta_vector *svclist;
    Jxta_boolean adv_gen_set;
    apr_uuid_t adv_gen;
    apr_uuid_t *adv_gen_ptr;
    enum contexts { UNKNOWN, UUID, TSP } uuid_context;

};

/**
 * Forw decl of unexported function.
 */
static void PA_delete(Jxta_object *);
static Jxta_status validate_adv(Jxta_PA * myself);


/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_PA(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *myself = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if (0 == len) {

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <jxta:PA> : [%pp]\n", myself);
        const char **atts = ((Jxta_advertisement *) myself)->atts;

        while (atts && *atts) {
            if (0 == strcmp(*atts, "xmlns:jxta")) {
                /* just silently skip it. */
            } else if (0 == strcmp(*atts, "pVer")) {
                const char * verNum = atts[1];
                char * minorPointer = strstr(verNum, ".");
                if(minorPointer != NULL) {
                    jxta_version_set_major_version(myself->protocol_version, atoi(verNum));
                    jxta_version_set_minor_version(myself->protocol_version, atoi(&(minorPointer[1])));
                } else {
                    jxta_version_set_major_version(myself->protocol_version, 1);
                    jxta_version_set_minor_version(myself->protocol_version, 0);
                }

            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Unrecognized attribute : \"%s\" = \"%s\"\n", *atts, atts[1]);
            }

            atts += 2;
        }

    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <jxta:PA> : [%pp]\n", myself);
    }
}

static void handlePID(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <PID> : [%pp]\n", ad);
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *pid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&pid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_PA_set_PID(ad, pid);
            JXTA_OBJECT_RELEASE(pid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid PID %s : [%pp]\n", jstring_get_string(tmp), ad);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <PID> : [%pp]\n", ad);
    }
}

static void handleGID(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <GID> : [%pp]\n", ad);
    } else {
        Jxta_status res;
        JString *tmp = jstring_new_1(len );
        Jxta_id *gid = NULL;

        jstring_append_0(tmp, (char *) cd, len);
        jstring_trim(tmp);

        res = jxta_id_from_jstring(&gid, tmp);
        if( JXTA_SUCCESS == res ) {
            jxta_PA_set_GID(ad, gid);
            JXTA_OBJECT_RELEASE(gid);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Invalid GID %s : [%pp]\n", jstring_get_string(tmp), ad);    
        }
        
        JXTA_OBJECT_RELEASE(tmp);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <GID> : [%pp]\n", ad);
    }
}

static void handleName(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Name> : [%pp]\n", ad);
    } else {
        JString *name = jstring_new_1(len );

        jstring_append_0( name, cd, len);
        jstring_trim(name);

        jxta_PA_set_Name(ad, name);

        JXTA_OBJECT_RELEASE(name);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Name> : [%pp]\n", ad);
    }
}

static void handleDesc(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Desc> Element : [%pp]\n", ad);
    } else {
        JString *desc = jstring_new_1(len );

        jstring_append_0( desc, cd, len);
        jstring_trim(desc);

        jxta_PA_set_Desc(ad, desc);

        JXTA_OBJECT_RELEASE(desc);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Desc> Element : [%pp]\n", ad);
    }
}

static void handleSvc(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;
    
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

static void handleSN(void *me, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        const char **atts = ((Jxta_advertisement *) ad)->atts;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <SN> Element : [%pp]\n", ad);
        while (atts && *atts) {
            if (0 == strcmp(*atts, "type")) {
                JString *type_str = jstring_new_2(atts[1]);
                jstring_trim(type_str);
                if (!strncmp(jstring_get_string(type_str), "uuid", 4)) {
                    ad->uuid_context = UUID;
                }
                JXTA_OBJECT_RELEASE(type_str);
            }
        atts += 2;
        }

    } else {
        JString *adv_gen = jstring_new_1(len );
        char tmpbuf[64];
        apr_status_t ret;
        apr_uuid_t peer_adv_gen;

        jstring_append_0( adv_gen, cd, len);
        jstring_trim(adv_gen);
        switch (ad->uuid_context) {
            case UUID:
            ret = apr_uuid_parse( &peer_adv_gen, jstring_get_string(adv_gen));
            if (APR_SUCCESS == ret) {

                jxta_PA_set_SN(ad, &peer_adv_gen);

                apr_uuid_format(tmpbuf, &peer_adv_gen);
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "parsed :%s\n", tmpbuf);
            } else {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Error parsing UUID:%s\n", jstring_get_string(adv_gen));
            }
            break;
            case TSP:
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "TSP type not supported\n");
                break;
            case UNKNOWN:
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unknown SN type:%s\n", jstring_get_string(adv_gen));
                break;
        }
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <SN> Element:%s\n", jstring_get_string(adv_gen) , tmpbuf);

        JXTA_OBJECT_RELEASE(adv_gen);
    }

}

 /** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_PA_add_relay_address(Jxta_PA * ad, Jxta_RdvAdvertisement * relay)
{
    int sz;
    int i;
    Jxta_AccessPointAdvertisement *ap = NULL;
    Jxta_RouteAdvertisement *routeRelay = NULL;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_svc *svc;
    Jxta_id *pid = NULL;

    Jxta_vector *svcs = ad->svclist;
    sz = jxta_vector_size(svcs);

    for (i = 0; i < sz; i++) {
        jxta_vector_get_object_at(svcs, (Jxta_object **) (void *) &svc, i);
        route = jxta_svc_get_RouteAdvertisement(svc);
        JXTA_OBJECT_RELEASE(svc);
        /*
         * the only service service which has a route is the endpoint
         * service which is the service where we want to modify the route
         */
        if (route != NULL) {
            routeRelay = jxta_RdvAdvertisement_get_Route(relay);
            pid = jxta_RdvAdvertisement_get_RdvPeerId(relay);
            ap = jxta_RouteAdvertisement_get_Dest(routeRelay);
            jxta_AccessPointAdvertisement_set_PID(ap, pid);
            jxta_RouteAdvertisement_add_first_hop(route, ap);
            JXTA_OBJECT_RELEASE(pid);
            JXTA_OBJECT_RELEASE(routeRelay);
            JXTA_OBJECT_RELEASE(ap);
            break;
        }
    }

    return route;
}

JXTA_DECLARE(Jxta_status) jxta_PA_remove_relay_address(Jxta_PA * ad, Jxta_id * relayid)
{
    int sz;
    int i;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_svc *svc;
    Jxta_vector *hops = jxta_vector_new(0);
    Jxta_vector *svcs = ad->svclist;

    sz = jxta_vector_size(svcs);

    for (i = 0; i < sz; i++) {
        jxta_vector_get_object_at(svcs, (Jxta_object **) (void *) &svc, i);
        route = jxta_svc_get_RouteAdvertisement(svc);

        /*
         * the only service service which has a route is the endpoint
         * service which is the service where we want to modify the route
         */
        if (route != NULL) {
            jxta_RouteAdvertisement_set_Hops(route, hops);
            JXTA_OBJECT_RELEASE(route);
            JXTA_OBJECT_RELEASE(svc);
            break;
        }
        JXTA_OBJECT_RELEASE(svc);
    }

    JXTA_OBJECT_RELEASE(hops);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_id *) jxta_PA_get_PID(Jxta_PA * ad)
{   
    return JXTA_OBJECT_SHARE(ad->PID);
}

static char *JXTA_STDCALL jxta_PA_get_PID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_PA *) ad)->PID, &js);
    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PA_set_PID(Jxta_PA * ad, Jxta_id * id)
{    
    JXTA_OBJECT_RELEASE(ad->PID);

    /* NULL may not be set for PID */
    ad->PID = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(Jxta_id *) jxta_PA_get_GID(Jxta_PA * ad)
{    
    return JXTA_OBJECT_SHARE(ad->GID);
}

static char *JXTA_STDCALL jxta_PA_get_GID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_PA *) ad)->GID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PA_set_GID(Jxta_PA * ad, Jxta_id * id)
{    
    JXTA_OBJECT_RELEASE(ad->GID);

    /* NULL may not be set for GID */
    ad->GID = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(JString *) jxta_PA_get_Name(Jxta_PA * ad)
{
    if( NULL != ad->Name ) {
        return JXTA_OBJECT_SHARE(ad->Name);
    } else {
        return NULL;
    }
}

static char *JXTA_STDCALL jxta_PA_get_Name_string(Jxta_advertisement * ad)
{
    char * result = NULL;

    if( NULL != ((Jxta_PA *) ad)->Name ) {
        result = strdup(jstring_get_string(((Jxta_PA *) ad)->Name));
    }

    return result;
}

JXTA_DECLARE(void) jxta_PA_set_Name(Jxta_PA * ad, JString * name)
{
    if( NULL != ad->Name ) {
        JXTA_OBJECT_RELEASE(ad->Name);
        ad->Name = NULL;
    }

    if( ( NULL != name ) && (jstring_length(name)>0)) {
        ad->Name = JXTA_OBJECT_SHARE(name);
    }
}

JXTA_DECLARE(JString *) jxta_PA_get_Desc(Jxta_PA * ad)
{
    if( NULL != ad->Desc ) {
        return JXTA_OBJECT_SHARE(ad->Desc);
    } else {
        return NULL;
    }
}

static char *JXTA_STDCALL jxta_PA_get_Desc_string(Jxta_advertisement * ad)
{
    char * result = NULL;

    if( NULL != ((Jxta_PA *) ad)->Desc ) {
        result = strdup(jstring_get_string(((Jxta_PA *) ad)->Desc));
    }

    return result;
}

JXTA_DECLARE(void) jxta_PA_set_Desc(Jxta_PA * ad, JString * desc)
{
    if( NULL != ad->Desc ) {
        JXTA_OBJECT_RELEASE(ad->Desc);
        ad->Desc = NULL;
    }

    if( ( NULL != desc ) && (jstring_length(desc)>0)) {
        ad->Desc = JXTA_OBJECT_SHARE(desc);
    }
}

JXTA_DECLARE(Jxta_boolean) jxta_PA_get_SN(Jxta_PA * myself, apr_uuid_t * ret)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if ( myself->adv_gen_set ) {
        memmove(ret, &myself->adv_gen, sizeof(apr_uuid_t));
        return TRUE;
    } else {
        return FALSE;
    }
}

JXTA_DECLARE(void) jxta_PA_set_SN(Jxta_PA * myself, apr_uuid_t * adv_gen)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if (NULL == adv_gen) {
        myself->adv_gen_set = FALSE;
    } else {
        memmove(&myself->adv_gen, adv_gen, sizeof(apr_uuid_t));
        myself->adv_gen_set = TRUE;
    }
}

JXTA_DECLARE(Jxta_boolean) jxta_PA_is_recent(Jxta_PA * myself, apr_uuid_t *uuid)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    Jxta_boolean ret = FALSE;
    int cmp;
    if (!myself->adv_gen_set) return FALSE;
    cmp = jxta_id_uuid_compare(&myself->adv_gen, uuid);
    if (UUID_NOT_COMPARABLE == cmp) {
        ret = (UUID_LESS_THAN != jxta_id_uuid_time_stamp_compare(&myself->adv_gen, uuid)) ? TRUE:FALSE;
        if (TRUE == ret) {
            ret = (UUID_GREATER_THAN == jxta_id_uuid_seq_no_compare(&myself->adv_gen, uuid)) ? TRUE:FALSE;
        }
    } else {
        ret = (UUID_LESS_THAN != cmp) ? TRUE:FALSE;
    }
    return ret;
}

JXTA_DECLARE(JString *) jxta_PA_get_Dbg(Jxta_PA * ad)
{
    return NULL;
}

#ifdef UNUSED_VWF
static char *JXTA_STDCALL jxta_PA_get_Dbg_string(Jxta_advertisement * ad)
{
    return NULL;
}
#endif
JXTA_DECLARE(void) jxta_PA_set_Dbg(Jxta_PA * ad, JString * dbg)
{
    /* do nothing */
}

/*
 * It would be more convenient if we had an interface to manipulate
 * individual services, but it is not absolutely necessary.
 */
JXTA_DECLARE(Jxta_vector *) jxta_PA_get_Svc(Jxta_PA * ad)
{    
    return JXTA_OBJECT_SHARE(ad->svclist);
}

JXTA_DECLARE(Jxta_status) jxta_PA_get_Svc_with_id(Jxta_PA * ad, Jxta_id * id, Jxta_svc ** svc)
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

JXTA_DECLARE(void) jxta_PA_set_Svc(Jxta_PA * ad, Jxta_vector * svclist)
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
static Jxta_status svc_printer(Jxta_PA * myself, JString * js)
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
static const Kwdtab jxta_PA_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PA", Jxta_PA_, *handleJxta_PA, NULL, NULL},
    {"PID", PID_, *handlePID, jxta_PA_get_PID_string, NULL},
    {"GID", GID_, *handleGID, jxta_PA_get_GID_string, NULL},
    {"Name", Name_, *handleName, jxta_PA_get_Name_string, NULL},
    {"Desc", Desc_, *handleDesc, jxta_PA_get_Desc_string, NULL},
    {"SN", SN_, *handleSN, NULL, NULL},
    {"Svc", Svc_, *handleSvc, NULL, NULL},
    {NULL, 0, 0, 0, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_PA_get_xml(Jxta_PA * ad, JString ** xml)
{
    Jxta_status result = JXTA_SUCCESS;
    JString *preamble = NULL;
    JString *content = NULL;
    char const * attrs[] = { "xmlns:jxta", "http://jxta.org", NULL };

    JXTA_OBJECT_CHECK_VALID(ad);

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    result = jxta_PA_get_xml_1(ad, &content, "jxta:PA", attrs );
    if( JXTA_SUCCESS == result) {
        preamble = jstring_new_1(100 + jstring_length(content));
        jstring_append_2(preamble, "<?xml version=\"1.0\"?>\n" "<!DOCTYPE jxta:PA>\n" );
        jstring_append_1( preamble, content );
        JXTA_OBJECT_RELEASE(content);
        
        *xml = preamble;

        return JXTA_SUCCESS;
    } else {
        return result;
    }
}

/**
*   Ensure that all required and optional fields have acceptable values.
*/
static Jxta_status validate_adv(Jxta_PA * myself) 
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if ( jxta_id_equals(myself->PID, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer id must not be null ID [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

#if 0
    /* Currently disabled for PlatformConfig. */
    if ( jxta_id_equals(myself->GID, jxta_id_nullID)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "peer group id must not be null ID [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
#endif

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_PA_get_xml_1(Jxta_PA * ad, JString ** xml, char const * element_name, char const ** attrs )
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *tmpref = NULL;
    char tmpbuf[64];
    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    
    res = validate_adv( ad );
    
    if( JXTA_SUCCESS != res ) {
        return res;
    }

    string = jstring_new_1(8192);

    jstring_append_2(string, "<" );
    jstring_append_2(string, element_name );
    while( *attrs ) {
        jstring_append_2(string, " ");
        jstring_append_2(string, *attrs );        
        jstring_append_2(string, "=\"");
        jstring_append_2(string, *(attrs+1) );
        jstring_append_2(string, "\"");
        
        attrs += 2;
    }

    char implString[15];
    memset(implString, 0, 15);
    snprintf(implString, 15, " pVer=\"%d.%d\"", 
                jxta_version_get_major_version(ad->protocol_version),
                jxta_version_get_minor_version(ad->protocol_version));
    jstring_append_2(string, implString);

    jstring_append_2(string, ">\n");
    
    jstring_append_2(string, "<PID>");
    jxta_id_to_jstring(ad->PID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    tmpref = NULL;
    jstring_append_2(string, "</PID>\n");

    if (ad->adv_gen_set) {
        apr_uuid_format(tmpbuf, &ad->adv_gen);
        jstring_append_2(string, "<SN type='uuid'>");
        jstring_append_2(string, tmpbuf);
        jstring_append_2(string, "</SN>\n");
    }
    jstring_append_2(string, "<GID>");
    jxta_id_to_jstring(ad->GID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    tmpref = NULL;
    jstring_append_2(string, "</GID>\n");

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

    jstring_append_2(string, "</" );
    jstring_append_2(string, element_name );
    jstring_append_2(string, ">\n");

    *xml = string;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_PA *) jxta_PA_new()
{
    Jxta_PA *ad = (Jxta_PA *) calloc(1, sizeof(Jxta_PA));

    if (NULL == ad) {
        return NULL;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peer Advertisement NEW [%pp]\n", ad );

    JXTA_OBJECT_INIT(ad, PA_delete, NULL);

    ad = (Jxta_PA *) jxta_advertisement_construct((Jxta_advertisement *) ad,
                                  "jxta:PA",
                                  jxta_PA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_PA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_PA_get_PID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_PA_get_indexes);

    if ( NULL != ad ) {
        ad->PID = JXTA_OBJECT_SHARE(jxta_id_nullID);
        ad->GID = JXTA_OBJECT_SHARE(jxta_id_nullID);
        ad->Name = jstring_new_0();
        ad->Desc = jstring_new_0();
        ad->protocol_version = jxta_version_new();
        ad->svclist = jxta_vector_new(4);
    }
    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void PA_delete(Jxta_object * me)
{
    Jxta_PA *ad = (Jxta_PA *) me;

    JXTA_OBJECT_RELEASE(ad->PID);
    
    JXTA_OBJECT_RELEASE(ad->GID);

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

    if( NULL != ad->protocol_version) {
        JXTA_OBJECT_RELEASE(ad->protocol_version);
        ad->protocol_version = NULL;
    }

    jxta_advertisement_destruct((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_PA));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peer Advertisement FREE [%pp]\n", ad );

    free(ad);
}

/**
 * Callback to return the elements and attributes for indexing
 *
 */
JXTA_DECLARE(Jxta_vector *) jxta_PA_get_indexes(Jxta_advertisement * dummy)
{
    static const char *idx[][2] = {
        {"Name", NULL},
        {"PID", NULL},
        {"Desc", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

JXTA_DECLARE(Jxta_status) jxta_PA_parse_charbuffer(Jxta_PA * myself, const char *buf, int len)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_adv(myself);
    }
    
    return res;
}

JXTA_DECLARE(Jxta_status) jxta_PA_parse_file(Jxta_PA * myself, FILE * stream)
{
    Jxta_status res;
    
    JXTA_OBJECT_CHECK_VALID(myself);
    
    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_adv(myself);
    }
    
    return res;
}

JXTA_DECLARE(void) jxta_PA_set_version(Jxta_PA * myself, Jxta_version *version)
{
    if(myself->protocol_version) {
        JXTA_OBJECT_RELEASE(myself->protocol_version);
    }
    myself->protocol_version = JXTA_OBJECT_SHARE(version); 
}

JXTA_DECLARE(Jxta_version*) jxta_PA_get_version(Jxta_PA * myself)
{
    JXTA_OBJECT_CHECK_VALID(myself);

    if( myself->protocol_version != NULL)
    {
        return JXTA_OBJECT_SHARE(myself->protocol_version);
    }
    
    return NULL;
}

/* vi: set ts=4 sw=4 tw=130 et: */
