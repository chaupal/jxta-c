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
 
static const char *const __log_cat = "RdvAdv";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_rdv.h"
#include "jxta_routea.h"

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    RdvAdvertisement_,
    Name_,
    RdvGroupId_,
    RdvPeerId_,
    RdvSvcId_,
    Route_
};

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_RdvAdvertisement {
    Jxta_advertisement jxta_advertisement;
    JString *Name;
    Jxta_id *RdvGroupId;
    Jxta_id *RdvPeerId;
    JString *Service;
    Jxta_RouteAdvertisement *route;
};

static void jxta_RdvAdvertisement_delete(Jxta_RdvAdvertisement * ad);
static Jxta_status jxta_RdvAdvertisement_getxml(Jxta_RdvAdvertisement * self, Jxta_write_func func, void *stream);
static Jxta_status jxta_RdvAdvertisement_getxml_1(Jxta_RdvAdvertisement * self, Jxta_write_func func, void *stream);
static Jxta_RdvAdvertisement *jxta_RdvAdvertisement_construct(Jxta_RdvAdvertisement * self);
static void jxta_RdvAdvertisement_destruct(Jxta_RdvAdvertisement * ad);

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleRdvAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    /* RdvAdvertisement * ad = (Jxta_RdvAdvertisement*)userdata; */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In RdvAdvertisement element\n");
}

static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) userdata;

    if (len == 0)
        return;

    jstring_append_0(ad->Name, (char *) cd, len);
    jstring_trim(ad->Name);
}

static void handleRdvGroupId(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *gid = NULL;
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&gid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (gid != NULL) {
        jxta_RdvAdvertisement_set_RdvGroupId(ad, gid);
        JXTA_OBJECT_RELEASE(gid);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In RdvGroupId element\n");
}

static void handleRdvPeerId(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *gid = NULL;
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&gid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (gid != NULL) {
        jxta_RdvAdvertisement_set_RdvPeerId(ad, gid);
        JXTA_OBJECT_RELEASE(gid);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In RdvPeerId element\n");
}


static void handleService(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) userdata;

    if (len == 0)
        return;

    jstring_append_0(ad->Service, (char *) cd, len);
    jstring_trim(ad->Service);
}

static void handleRoute(void *userdata, const XML_Char * cd, int len)
{
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) userdata;
    Jxta_RouteAdvertisement *ra;
    
    if( 0 != len ) {
        return;
    }

    ra = jxta_RouteAdvertisement_new();
    jxta_RdvAdvertisement_set_Route(ad, ra);
    JXTA_OBJECT_RELEASE(ra);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Begin handleRouteAdvertisement" " (end may not show)\n");

    jxta_advertisement_set_handlers((Jxta_advertisement *) ra, ((Jxta_advertisement *) ad)->parser, (void *) ad);
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */

JXTA_DECLARE(char *) jxta_RdvAdvertisement_get_Name(Jxta_RdvAdvertisement * ad)
{
    return strdup(jstring_get_string(ad->Name));
}

static char *JXTA_STDCALL jxta_RdvAdvertisement_get_Name_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string( ((Jxta_RdvAdvertisement *)ad)->Name));
}

JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Name(Jxta_RdvAdvertisement * ad, const char *name)
{
    if (!name) {
        return;
    }
    if (ad->Name) {
        JXTA_OBJECT_RELEASE(ad->Name);
    }
    ad->Name = jstring_new_2(name);
}

JXTA_DECLARE(char *) jxta_RdvAdvertisement_get_Service(Jxta_RdvAdvertisement * ad)
{
    return strdup(jstring_get_string(ad->Service));
}

static char *JXTA_STDCALL jxta_RdvAdvertisement_get_Service_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_RdvAdvertisement *) ad)->Service));
}

JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Service(Jxta_RdvAdvertisement * ad, const char *service)
{
    if (!service) {
        return;
    }
    if (ad->Service) {
        JXTA_OBJECT_RELEASE(ad->Service);
    }
    ad->Service = jstring_new_2(service);
}

JXTA_DECLARE(Jxta_id *) jxta_RdvAdvertisement_get_RdvGroupId(Jxta_RdvAdvertisement * ad)
{
    if (NULL != ad->RdvGroupId) {
        JXTA_OBJECT_SHARE(ad->RdvGroupId);
    }

    return ad->RdvGroupId;
}

static char *JXTA_STDCALL jxta_RdvAdvertisement_get_RdvGroupId_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js;

    if (NULL == ((Jxta_RdvAdvertisement *) ad)->RdvGroupId) {
        return NULL;
    }

    jxta_id_to_jstring(((Jxta_RdvAdvertisement *) ad)->RdvGroupId, &js);
    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_RdvAdvertisement_set_RdvGroupId(Jxta_RdvAdvertisement * ad, Jxta_id * id)
{
    ad->RdvGroupId = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(Jxta_id *) jxta_RdvAdvertisement_get_RdvPeerId(Jxta_RdvAdvertisement * ad)
{
    if (NULL != ad->RdvPeerId) {
        JXTA_OBJECT_SHARE(ad->RdvPeerId);
    }

    return ad->RdvPeerId;
}

static char *JXTA_STDCALL jxta_RdvAdvertisement_get_RdvPeerId_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_RdvAdvertisement *) ad)->RdvPeerId, &js);
    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_RdvAdvertisement_set_RdvPeerId(Jxta_RdvAdvertisement * ad, Jxta_id * id)
{
    if (!id) {
        return;
    }
    if (ad->RdvPeerId) {
        JXTA_OBJECT_RELEASE(ad->RdvPeerId);
    }
    ad->RdvPeerId = JXTA_OBJECT_SHARE(id);
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_RdvAdvertisement_get_Route(Jxta_RdvAdvertisement * ad)
{
    if (ad->route != NULL)
        JXTA_OBJECT_SHARE(ad->route);
    return ad->route;
}

JXTA_DECLARE(void) jxta_RdvAdvertisement_set_Route(Jxta_RdvAdvertisement * ad, Jxta_RouteAdvertisement * ra)
{
    if (ra != NULL)
        JXTA_OBJECT_SHARE(ra);
    if (ad->route != NULL)
        JXTA_OBJECT_RELEASE(ad->route);
    ad->route = ra;
}

/** Now, build an array of the keyword structs.  Since
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab RdvAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RdvAdvertisement", RdvAdvertisement_, *handleRdvAdvertisement, NULL, NULL},
    {"Name", Name_, *handleName, jxta_RdvAdvertisement_get_Name_string, NULL},
    {"RdvGroupId", RdvGroupId_, *handleRdvGroupId, jxta_RdvAdvertisement_get_RdvGroupId_string, NULL},
    {"RdvPeerId", RdvPeerId_, *handleRdvPeerId, jxta_RdvAdvertisement_get_RdvPeerId_string, NULL},
    {"RdvServiceName", RdvSvcId_, *handleService, jxta_RdvAdvertisement_get_Service_string, NULL},
    {"RdvRoute", Route_, *handleRoute, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_get_xml(Jxta_RdvAdvertisement * ad, JString ** string)
{
    Jxta_status status;
    JString *result = jstring_new_0();

    status = jxta_RdvAdvertisement_getxml(ad, jstring_writefunc_appender, result);

    if (JXTA_SUCCESS == status)
        *string = result;
    else {
        JXTA_OBJECT_RELEASE(result);
        *string = NULL;
    }
    return status;
}

static Jxta_status jxta_RdvAdvertisement_getxml(Jxta_RdvAdvertisement * self, Jxta_write_func func, void *stream)
{
    static const char *XMLDECL = "<?xml version=\"1.0\"?>\n\n";
    static const char *DOCTYPEDECL = "<!DOCTYPE jxta:RdvAdvertisement>\n\n";
    Jxta_status res;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == func)
        return JXTA_INVALID_ARGUMENT;

    (func) (stream, XMLDECL, strlen(XMLDECL) - 1, &res);

    if (JXTA_SUCCESS != res)
        return res;

    (func) (stream, DOCTYPEDECL, strlen(DOCTYPEDECL) - 1, &res);

    if (JXTA_SUCCESS != res)
        return res;

    return jxta_RdvAdvertisement_getxml_1(self, func, stream);
}

static Jxta_status jxta_RdvAdvertisement_getxml_1(Jxta_RdvAdvertisement * self, Jxta_write_func func, void *stream)
{
#define PARAM           ((const char*) -1)
#define STARTTAGSTART   "<"
#define STARTTAGEND     ">\n"
#define ENDTAGSTART     "</"
#define ENDTAGEND       STARTTAGEND
#define RDVADVTAG       "jxta:RdvAdvertisement"
#define RDVADVNSATTR    " xmlns:jxta=\"http://jxta.org\""
#define NAMETAG         "Name"
#define GROUPTAG        "RdvGroupId"
#define PEERTAG         "RdvPeerId"
#define SERVICETAG      "RdvServiceName"
#define ROUTETAG        "RdvRoute"

    static const char *const docschema[] = {
        STARTTAGSTART, RDVADVTAG, RDVADVNSATTR, STARTTAGEND,
        STARTTAGSTART, NAMETAG, STARTTAGEND, PARAM, ENDTAGSTART, NAMETAG, ENDTAGEND,
        STARTTAGSTART, GROUPTAG, STARTTAGEND, PARAM, ENDTAGSTART, GROUPTAG, ENDTAGEND,
        STARTTAGSTART, PEERTAG, STARTTAGEND, PARAM, ENDTAGSTART, PEERTAG, ENDTAGEND,
        STARTTAGSTART, SERVICETAG, STARTTAGEND, PARAM, ENDTAGSTART, SERVICETAG, ENDTAGEND,
        STARTTAGSTART, ROUTETAG, STARTTAGEND, PARAM, ENDTAGSTART, ROUTETAG, ENDTAGEND,
        ENDTAGSTART, RDVADVTAG, ENDTAGEND, NULL
    };
    char const *const *eachPart = docschema;

    Jxta_status res = JXTA_SUCCESS;

    const char *params[6] = {
        NULL, NULL, NULL, NULL, NULL, NULL
    };
    int eachParam = 0;
    JString *tmp = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == func)
        return JXTA_INVALID_ARGUMENT;

    /* name param */
    params[0] = strdup(jstring_get_string(self->Name));

    /* group param */
    res = jxta_id_to_jstring(self->RdvGroupId, &tmp);

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    params[1] = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    tmp = NULL;

    /* peer param */
    res = jxta_id_to_jstring(self->RdvPeerId, &tmp);

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    params[2] = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    tmp = NULL;

    /* service param */
    params[3] = strdup(jstring_get_string(self->Service));

    /* The route should not be NULL but it has happened and this insures no segfault */
    if (NULL != self->route) {
        /* route param */
        res = jxta_RouteAdvertisement_get_xml(self->route, &tmp);

        if (JXTA_SUCCESS != res)
            goto Common_Exit;

        params[4] = strdup(jstring_get_string(tmp));
        JXTA_OBJECT_RELEASE(tmp);
    } else {
        params[4] = NULL;
    }
    tmp = NULL;

    /* write it! */
    while (NULL != *eachPart) {
        if (PARAM != *eachPart)
            (func) (stream, *eachPart, strlen(*eachPart), &res);
        else {
            (func) (stream, params[eachParam], strlen(params[eachParam]), &res);
            eachParam++;
        }

        if (JXTA_SUCCESS != res)
            goto Common_Exit;
        eachPart++;
    }

  Common_Exit:
    if (NULL != tmp)
        JXTA_OBJECT_RELEASE(tmp);
    tmp = NULL;

    for (eachParam = 0; eachParam < (sizeof(params) / sizeof(const char *)); eachParam++) {
        if (NULL != params[eachParam]) {
        free((void *) params[eachParam]);
        }
        params[eachParam] = NULL;
    }

    return res;
}

/** 
 * Get a new instance of the ad.
 **/
JXTA_DECLARE(Jxta_RdvAdvertisement *) jxta_RdvAdvertisement_new(void)
{
    Jxta_RdvAdvertisement *ad = (Jxta_RdvAdvertisement *) calloc(1, sizeof(Jxta_RdvAdvertisement));

    JXTA_OBJECT_INIT(ad, (JXTA_OBJECT_FREE_FUNC) jxta_RdvAdvertisement_delete, 0);

    return jxta_RdvAdvertisement_construct(ad);
}

static Jxta_RdvAdvertisement *jxta_RdvAdvertisement_construct(Jxta_RdvAdvertisement * self)
{
    self = (Jxta_RdvAdvertisement *) jxta_advertisement_construct((Jxta_advertisement *) self,
                                                                  "jxta:RdvAdvertisement",
                                                                  RdvAdvertisement_tags, 
                                                                  (JxtaAdvertisementGetXMLFunc)jxta_RdvAdvertisement_get_xml,
                                                                  /* FIXME, need a decision on which ID to return, if any for now none */
                                                                  NULL, 
                                                                  jxta_RendezvousAdvertisement_get_indexes);

    if (NULL != self) {
        self->Name = jstring_new_0();
        self->Service = jstring_new_0();
        self->route = NULL;
        self->RdvGroupId = NULL;
        self->RdvPeerId = NULL;
    }

    return self;
}

static void jxta_RdvAdvertisement_destruct(Jxta_RdvAdvertisement * ad)
{
    /* Fill in the required freeing functions here. */
    if (ad->Name)
        JXTA_OBJECT_RELEASE(ad->Name);
    if (ad->Service)
        JXTA_OBJECT_RELEASE(ad->Service);
    if (ad->route)
        JXTA_OBJECT_RELEASE(ad->route);
    if (ad->RdvGroupId)
        JXTA_OBJECT_RELEASE(ad->RdvGroupId);
    if (ad->RdvPeerId)
        JXTA_OBJECT_RELEASE(ad->RdvPeerId);

    jxta_advertisement_destruct((Jxta_advertisement *) ad);
}

/** 
 */
static void jxta_RdvAdvertisement_delete(Jxta_RdvAdvertisement * ad)
{
    jxta_RdvAdvertisement_destruct(ad);

    memset(ad, 0xdd, sizeof(Jxta_RdvAdvertisement));
    free(ad);
}

/**
 * Callback to return the elements and attributes for indexing
 *
 */

JXTA_DECLARE(Jxta_vector *) jxta_RendezvousAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"RdvGroupId", NULL},
        {"Name", NULL},
        {"RdvServiceName", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_parse_charbuffer(Jxta_RdvAdvertisement * ad, const char *buf, size_t len)
{
    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(Jxta_status) jxta_RdvAdvertisement_parse_file(Jxta_RdvAdvertisement * ad, FILE * stream)
{
    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vi: set ts=4 sw=4 tw=130 et: */
