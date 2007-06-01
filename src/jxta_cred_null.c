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
 * $Id: jxta_cred_null.c,v 1.9 2006/08/23 20:44:49 bondolo Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_cred_null.h"

struct _jxta_cred_null {
    struct _Jxta_credential common;

    Jxta_service *service;

    Jxta_id *pg;

    Jxta_id *peer;

    JString *identity;
};


typedef struct _jxta_cred_null jxta_cred_null_mutable;

static void cred_null_delete(Jxta_object * ptr);
static Jxta_status jxta_cred__null_getpeergroup(Jxta_credential * cred, Jxta_id ** pg);
static Jxta_status jxta_cred__null_getpeer(Jxta_credential * cred, Jxta_id ** peer);
static Jxta_status jxta_cred__null_getsource(Jxta_credential * cred, Jxta_service ** service);
static Jxta_status jxta_cred__null_getxml(Jxta_credential * cred, Jxta_write_func func, void *stream);
static Jxta_status jxta_cred__null_getxml_1(Jxta_credential * cred, Jxta_write_func func, void *stream);

static Jxta_cred_vtable _cred_null_vtable = {
    "NullCrendenital",

    jxta_cred__null_getpeergroup,
    jxta_cred__null_getpeer,
    jxta_cred__null_getsource,
    jxta_cred__null_getxml,
    jxta_cred__null_getxml_1
};

jxta_cred_null *jxta_cred_null_new(Jxta_service * service, Jxta_id * pg, Jxta_id * peer, JString * identity)
{
    jxta_cred_null_mutable *cred = (jxta_cred_null_mutable *) calloc(1, sizeof(struct _jxta_cred_null));

    if (NULL == cred) {
        JXTA_LOG("Could not malloc cred");
        return NULL;
    }

    JXTA_OBJECT_INIT(cred, cred_null_delete, NULL);

    cred->common.credfuncs = &_cred_null_vtable;

    cred->service = JXTA_OBJECT_SHARE(service);
    cred->pg = JXTA_OBJECT_SHARE(pg);
    cred->peer = JXTA_OBJECT_SHARE(peer);
    cred->identity = JXTA_OBJECT_SHARE(identity);

    return (jxta_cred_null *) cred;
}

static void cred_null_delete(Jxta_object * ptr)
{
    jxta_cred_null_mutable *cred = (jxta_cred_null_mutable *) ptr;

    JXTA_OBJECT_RELEASE(cred->service);
    JXTA_OBJECT_RELEASE(cred->pg);
    JXTA_OBJECT_RELEASE(cred->peer);
    JXTA_OBJECT_RELEASE(cred->identity);

    free(ptr);
    ptr = NULL;
}

static Jxta_status jxta_cred__null_getpeergroup(Jxta_credential * cred, Jxta_id ** pg)
{
    jxta_cred_null *self = (jxta_cred_null *) cred;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == pg)
        return JXTA_INVALID_ARGUMENT;

    *pg = JXTA_OBJECT_SHARE(self->pg);

    return JXTA_SUCCESS;
}

static Jxta_status jxta_cred__null_getpeer(Jxta_credential * cred, Jxta_id ** peer)
{
    jxta_cred_null *self = (jxta_cred_null *) cred;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == peer)
        return JXTA_INVALID_ARGUMENT;

    *peer = JXTA_OBJECT_SHARE(self->peer);

    return JXTA_SUCCESS;
}

static Jxta_status jxta_cred__null_getsource(Jxta_credential * cred, Jxta_service ** service)
{
    jxta_cred_null *self = (jxta_cred_null *) cred;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == service)
        return JXTA_INVALID_ARGUMENT;

    *service = JXTA_OBJECT_SHARE(self->service);

    return JXTA_SUCCESS;
}

static Jxta_status jxta_cred__null_getxml(Jxta_credential * cred, Jxta_write_func func, void *stream)
{
    static const char *XMLDECL = "<?xml version=\"1.0\" standalone=no charset=\"UTF8\"?>\n\n";
    static const char *DOCTYPEDECL = "<!DOCTYPE jxta:Cred>\n\n";

    Jxta_status res;
    jxta_cred_null *self = (jxta_cred_null *) cred;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == func)
        return JXTA_INVALID_ARGUMENT;

    (func) (stream, XMLDECL, sizeof(XMLDECL) - 1, &res);

    if (JXTA_SUCCESS != res)
        return res;

    (func) (stream, DOCTYPEDECL, sizeof(DOCTYPEDECL) - 1, &res);

    if (JXTA_SUCCESS != res)
        return res;

    return jxta_cred__null_getxml_1(cred, func, stream);
}

static Jxta_status jxta_cred__null_getxml_1(Jxta_credential * cred, Jxta_write_func func, void *stream)
{
#define PARAM           ((const char*) -1)
#define STARTTAGSTART   "<"
#define STARTTAGEND     ">\n"
#define ENDTAGSTART     "</"
#define ENDTAGEND       STARTTAGEND
#define CREDTAG         "jxta:Cred"
#define CREDNSATTR      " xmlns:jxta=\"http://jxta.org\""
#define CREDTYPEATTR    " type=\"jxta:NullCred\""
#define PGTAG           "PeerGroupID"
#define PEERTAG         "PeerID"
#define IDENTITYTAG     "Identity"

    static const char *const docschema[] = {
        STARTTAGSTART, CREDTAG, CREDNSATTR, CREDTYPEATTR, STARTTAGEND,
        STARTTAGSTART, PGTAG, STARTTAGEND, PARAM, ENDTAGSTART, PGTAG, ENDTAGEND,
        STARTTAGSTART, PEERTAG, STARTTAGEND, PARAM, ENDTAGSTART, PEERTAG, ENDTAGEND,
        STARTTAGSTART, IDENTITYTAG, STARTTAGEND, PARAM, ENDTAGSTART, IDENTITYTAG, ENDTAGEND,
        ENDTAGSTART, CREDTAG, ENDTAGEND, NULL
    };
    char const *const *eachPart = docschema;

    Jxta_status res = JXTA_SUCCESS;
    jxta_cred_null *self = (jxta_cred_null *) cred;

    const char *params[4] = {
        NULL, NULL, NULL, NULL
    };
    int eachParam = 0;

    JString *tmp = NULL;

    if (!JXTA_OBJECT_CHECK_VALID(self))
        return JXTA_INVALID_ARGUMENT;

    if (NULL == func)
        return JXTA_INVALID_ARGUMENT;

    /* pg param */
    res = jxta_id_to_jstring(self->pg, &tmp);

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    params[0] = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    tmp = NULL;

    /* peer param */
    res = jxta_id_to_jstring(self->peer, &tmp);

    if (JXTA_SUCCESS != res)
        goto Common_Exit;

    params[1] = strdup(jstring_get_string(tmp));
    JXTA_OBJECT_RELEASE(tmp);
    tmp = NULL;

    /* identity param */
    params[2] = strdup(jstring_get_string(self->identity));

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
        free((void *) params[eachParam]);
        params[eachParam] = NULL;
    }

    return res;

}

/* vim: set ts=4 sw=4 et tw=130: */
