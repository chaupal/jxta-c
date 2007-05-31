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
 * $Id: jxta_pa.c,v 1.70 2005/04/03 01:47:58 bondolo Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_pa.h"
#include "jxta_svc.h"
#include "jxta_xml_util.h"
#include "jxta_routea.h"
#include "jxta_apa.h"
#include "jxta_rdv.h"

#ifdef __cplusplus
extern "C" {

#if 0
}
#endif
#endif
/** Each of these corresponds to a tag in the 
 * xml ad.
 */ enum tokentype {
    Null_,
    Jxta_PA_,
    PID_,
    GID_,
    Name_,
    Dbg_,
    Svc_
};


/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_PA {
    Jxta_advertisement jxta_advertisement;
    char *Jxta_PA;
    Jxta_id *PID;
    Jxta_id *GID;
    JString *Name;
    JString *Dbg;
    Jxta_vector *svclist;
};

/**
 * Forward decl of index delete function
 *
 */
static void jxta_PA_idx_delete(Jxta_object *);

/**
 * Forw decl of unexported function.
 */
static void jxta_PA_delete(Jxta_PA *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_PA(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_PA * ad = (Jxta_PA*)userdata; */
    JXTA_LOG("In Jxta_PA element\n");
}

/*
 * I like this one better than what's in jstring right now.
 * so I'll keep it around with the intend to cut/paste it later
 * if get around to it.
 *
static void trim(const XML_Char** cdp, int* lenp)
{
    const XML_Char* p;

    const XML_Char* cd = *cdp;
    int len = *lenp;

    while (len) {
	if (! isspace((int) *(char*)cd)) break;
	++cd;
	--len;
    }

    if (len == 0) {
	*cdp = cd;
	*lenp = len;
	return;
    }

    p = &(cd[len - 1]);
    while (len) {
	if (! isspace((int) *(char*)p)) break;
	--p;
	--len;
    }

    *cdp = cd;
    *lenp = len;
}

*/

static void handlePID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *pid = NULL;
    Jxta_PA *ad = (Jxta_PA *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&pid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (pid != NULL) {
        jxta_PA_set_PID(ad, pid);
        JXTA_OBJECT_RELEASE(pid);
    }
}

/* Same problem here as previous comment. */
static void handleGID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *gid = NULL;
    JString *tmp;
    Jxta_PA *ad = (Jxta_PA *) userdata;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance. fromString calls get_string
     * which adds a \0 at the end.
     */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&gid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (gid != NULL) {
        jxta_PA_set_GID(ad, gid);
        JXTA_OBJECT_RELEASE(gid);
    }

    /*  printf("In GID element\n"); */
}



static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) userdata;
    JString *nm;

    if (len == 0)
        return;

    nm = jstring_new_1(len);
    jstring_append_0(nm, cd, len);
    jstring_trim(nm);

    jxta_PA_set_Name(ad, nm);

    JXTA_OBJECT_RELEASE(nm);
}


static void handleDbg(void *userdata, const XML_Char * cd, int len)
{
    JString *dbg;
    Jxta_PA *ad = (Jxta_PA *) userdata;

    if (len == 0)
        return;

    dbg = jstring_new_1(len);
    jstring_append_0(dbg, (char *) cd, len);
    jstring_trim(dbg);

    jxta_PA_set_Dbg(ad, dbg);
    JXTA_OBJECT_RELEASE(dbg);
}


static void handleSvc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_PA *ad = (Jxta_PA *) userdata;
    Jxta_svc *svc = jxta_svc_new();

    JXTA_LOG("Begin pa handleSvc (end tag may not show)\n");

    jxta_vector_add_object_last(ad->svclist, (Jxta_object *) svc);
    JXTA_OBJECT_RELEASE(svc);

    jxta_advertisement_set_handlers((Jxta_advertisement *) svc, ((Jxta_advertisement *) ad)->parser, (void *) ad);

}

 /** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

static char *jxta_PA_get_jxta_PA_string(Jxta_advertisement * ad)
{
    return NULL;
}

Jxta_RouteAdvertisement *jxta_PA_add_relay_address(Jxta_PA * ad, Jxta_RdvAdvertisement * relay)
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
        jxta_vector_get_object_at(svcs, (Jxta_object **) & svc, i);
        route = jxta_svc_get_RouteAdvertisement(svc);

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
            JXTA_OBJECT_RELEASE(svc);
            JXTA_OBJECT_RELEASE(routeRelay);
            JXTA_OBJECT_RELEASE(ap);
            break;
        }
        JXTA_OBJECT_RELEASE(svc);
    }

    return route;
}

Jxta_status jxta_PA_remove_relay_address(Jxta_PA * ad, Jxta_id * relayid)
{

    int sz;
    int i;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_svc *svc;
    Jxta_vector *hops = jxta_vector_new(0);

    Jxta_vector *svcs = ad->svclist;
    sz = jxta_vector_size(svcs);

    for (i = 0; i < sz; i++) {
        jxta_vector_get_object_at(svcs, (Jxta_object **) & svc, i);
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

Jxta_id *jxta_PA_get_PID(Jxta_PA * ad)
{
    JXTA_OBJECT_SHARE(ad->PID);
    return ad->PID;
}

Jxta_id *jxta_PA_get_GID(Jxta_PA * ad)
{
    JXTA_OBJECT_SHARE(ad->GID);
    return ad->GID;
}

static char *jxta_PA_get_PID_string(Jxta_advertisement * ad)
{

    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_PA *) ad)->PID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

static char *jxta_PA_get_GID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_PA *) ad)->GID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

void jxta_PA_set_PID(Jxta_PA * ad, Jxta_id * id)
{
    JXTA_OBJECT_SHARE(id);
    JXTA_OBJECT_RELEASE(ad->PID);
    ad->PID = id;
}

void jxta_PA_set_GID(Jxta_PA * ad, Jxta_id * id)
{
    JXTA_OBJECT_SHARE(id);
    JXTA_OBJECT_RELEASE(ad->GID);
    ad->GID = id;
}

JString *jxta_PA_get_Name(Jxta_PA * ad)
{
    JXTA_OBJECT_SHARE(ad->Name);
    return ad->Name;
}

static char *jxta_PA_get_Name_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_PA *) ad)->Name));
}


void jxta_PA_set_Name(Jxta_PA * ad, JString * name)
{
    JXTA_OBJECT_SHARE(name);
    JXTA_OBJECT_RELEASE(ad->Name);
    ad->Name = name;
}


JString *jxta_PA_get_Dbg(Jxta_PA * ad)
{
    JXTA_OBJECT_SHARE(ad->Dbg);
    return ad->Dbg;
}


static char *jxta_PA_get_Dbg_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_PA *) ad)->Dbg));
}


void jxta_PA_set_Dbg(Jxta_PA * ad, JString * dbg)
{
    JXTA_OBJECT_SHARE(dbg);
    JXTA_OBJECT_RELEASE(ad->Dbg);
    ad->Dbg = dbg;

}

/*
 * It would be more convenient if we had an interface to manipulate
 * individual services, but it is not absolutely necessary.
 */
Jxta_vector *jxta_PA_get_Svc(Jxta_PA * ad)
{
    JXTA_OBJECT_SHARE(ad->svclist);
    return ad->svclist;
}

void jxta_PA_set_Svc(Jxta_PA * ad, Jxta_vector * svclist)
{
    JXTA_OBJECT_SHARE(svclist);
    JXTA_OBJECT_RELEASE(ad->svclist);
    ad->svclist = svclist;
}

/* This is an internal function so we can assume what best fits us
 * as for the allocation of the returned jstring. We assume it
 * already exists, is passed by invoking code, and we append to it.
 */
void svc_printer(Jxta_PA * ad, JString * js)
{
    int sz;
    int i;
    Jxta_vector *svcs = ad->svclist;
    sz = jxta_vector_size(svcs);

    /* Svc is a self standing obj, so its get_xml routine has to
     * have the expected behaviour wrt to the allocation of the return
     * jstring. It creates one itself. Until we decide on generalizing
     * an append option, we have to take it and append it manually.
     * That'll do for now.
     */
    for (i = 0; i < sz; ++i) {
        Jxta_svc *svc;
        JString *tmp;

        JXTA_LOG("Jxta_PA: printing a Svc\n");
        jxta_vector_get_object_at(svcs, (Jxta_object **) & svc, i);

        jxta_svc_get_xml(svc, &tmp);
        jstring_append_1(js, tmp);
        JXTA_OBJECT_RELEASE(svc);
        JXTA_OBJECT_RELEASE(tmp);
    }
}



/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_PA_tags[] = {
    {"Null", Null_, NULL, NULL},
    {"jxta:PA", Jxta_PA_, *handleJxta_PA, jxta_PA_get_jxta_PA_string},
    {"PID", PID_, *handlePID, jxta_PA_get_PID_string},
    {"GID", GID_, *handleGID, jxta_PA_get_GID_string},
    {"Name", Name_, *handleName, jxta_PA_get_Name_string},
    {"Dbg", Dbg_, *handleDbg, jxta_PA_get_Dbg_string},
    {"Svc", Svc_, *handleSvc, NULL},
    {NULL, 0, 0, 0}
};




Jxta_status jxta_PA_get_xml(Jxta_PA * ad, JString ** xml)
{
    JString *string;
    JString *tmpref = NULL;

    if (ad == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_1(8192);

    jstring_append_2(string, "<?xml version=\"1.0\"?>" "<!DOCTYPE jxta:PA>" "<jxta:PA xmlns:jxta=\"http://jxta.org\">\n");
    jstring_append_2(string, "<PID>");
    jxta_id_to_jstring(ad->PID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    jstring_append_2(string, "</PID>\n");

    jstring_append_2(string, "<GID>");
    tmpref = NULL;
    jxta_id_to_jstring(ad->GID, &tmpref);
    jstring_append_1(string, tmpref);
    JXTA_OBJECT_RELEASE(tmpref);
    jstring_append_2(string, "</GID>\n");

    jstring_append_2(string, "<Name>");
    jstring_append_1(string, ad->Name);
    jstring_append_2(string, "</Name>\n");


    jstring_append_2(string, "<Dbg>");
  /**
   * @bug ad->Dbg is getting tossed out of jstring_append_1
   * as being uninitialized, the magic number is changed.
   */
    jstring_append_1(string, ad->Dbg);
    jstring_append_2(string, "</Dbg>\n");


    svc_printer(ad, string);

    jstring_append_2(string, "</jxta:PA>\n");

    *xml = string;

    return JXTA_SUCCESS;
}


/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
Jxta_PA *jxta_PA_new()
{
    Jxta_PA *ad;
    ad = (Jxta_PA *) calloc(1, sizeof(Jxta_PA));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:PA",
                                  jxta_PA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_PA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_PA_get_PID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_PA_get_indexes, (FreeFunc) jxta_PA_delete);

    /* Fill in the required initialization code here. */

    /*
     * Supply place holders, so that everything can be safely released
     * shared, printed, etc.
     */

    JXTA_OBJECT_SHARE((Jxta_object *) jxta_id_nullID);
    JXTA_OBJECT_SHARE(jxta_id_nullID);
    ad->PID = jxta_id_nullID;
    ad->GID = jxta_id_nullID;
    ad->Name = jstring_new_0();
    ad->Dbg = jstring_new_2("warning");
    ad->svclist = jxta_vector_new(4);

    return ad;
}


/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void jxta_PA_delete(Jxta_PA * ad)
{

    JXTA_OBJECT_RELEASE(ad->Name);
    JXTA_OBJECT_RELEASE(ad->PID);
    JXTA_OBJECT_RELEASE(ad->GID);
    JXTA_OBJECT_RELEASE(ad->Dbg);
    JXTA_OBJECT_RELEASE(ad->svclist);

    jxta_advertisement_delete((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_PA));

    free(ad);
}

/**
 * Callback to return the elements and attributes for indexing
 *
 */

Jxta_vector *jxta_PA_get_indexes(void)
{
    static const char *idx[][2] = {
        {"Name", NULL},
        {"PID", NULL},
        {"Desc", NULL},
        {"Dbg", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx);
}


void jxta_PA_parse_charbuffer(Jxta_PA * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

void jxta_PA_parse_file(Jxta_PA * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef __cplusplus
}
#endif
