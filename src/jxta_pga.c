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
 * $Id: jxta_pga.c,v 1.38 2006/02/18 00:32:52 slowhog Exp $
 */

static const char *const __log_cat = "PGA";

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
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
    char *Jxta_PGA;
    Jxta_id *GID;
    Jxta_id *MSID;
    JString *Name;
    JString *Desc;
    char *Svc;
};

/**
 * Forw decl. of unexported function.
 */
static void jxta_PGA_delete(Jxta_PGA *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_PGA(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_PGA * ad = (Jxta_PGA*)userdata; */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In PGA element...\n");
}

static void handleGID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *gid = NULL;
    Jxta_PGA *ad = (Jxta_PGA *) userdata;
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
        jxta_PGA_set_GID(ad, gid);
        JXTA_OBJECT_RELEASE(gid);
    }
}

static void handleMSID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *msid = NULL;
    Jxta_PGA *ad = (Jxta_PGA *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&msid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (msid != NULL) {
        jxta_PGA_set_MSID(ad, msid);
        JXTA_OBJECT_RELEASE(msid);
    }
}

static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) userdata;
    JString *nm;

    if (len == 0)
        return;

    nm = jstring_new_1(len);
    jstring_append_0(nm, cd, len);
    jstring_trim(nm);

    jxta_PGA_set_Name(ad, nm);

    JXTA_OBJECT_RELEASE(nm);
}

static void handleDesc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_PGA *ad = (Jxta_PGA *) userdata;
    JString *nm;

    if (len == 0)
        return;

    nm = jstring_new_1(len);
    jstring_append_0(nm, cd, len);
    jstring_trim(nm);

    jxta_PGA_set_Desc(ad, nm);

    JXTA_OBJECT_RELEASE(nm);
}

static void handleSvc(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_PGA * ad = (Jxta_PGA*)userdata; */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Svc element...\n");
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(char *) jxta_PGA_get_Jxta_PGA(Jxta_PGA * ad)
{
    return NULL;
}

char *JXTA_STDCALL jxta_PGA_get_Jxta_PGA_string(Jxta_advertisement * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_PGA_set_Jxta_PGA(Jxta_PGA * ad, char *name)
{

}

JXTA_DECLARE(Jxta_id *) jxta_PGA_get_GID(Jxta_PGA * ad)
{
    JXTA_OBJECT_SHARE(ad->GID);
    return ad->GID;
}

char *JXTA_STDCALL jxta_PGA_get_GID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_PGA *) ad)->GID, &js);

    /*
     * since we return a plain string we have to dup it (get_string does not
     * do it for us). All this because we have to release the new js now,
     * otherwise there's be no other opportunity; the reference will be
     * forgotten when we return.
     * FIXME: jice@jxta.org 20020228 - It would be much better to return the
     * jstring directly.
     */

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PGA_set_GID(Jxta_PGA * ad, Jxta_id * id)
{
    JXTA_OBJECT_SHARE(id);
    JXTA_OBJECT_RELEASE(ad->GID);
    ad->GID = id;
}

JXTA_DECLARE(Jxta_id *) jxta_PGA_get_MSID(Jxta_PGA * ad)
{
    JXTA_OBJECT_SHARE(ad->MSID);
    return ad->MSID;
}

char *JXTA_STDCALL jxta_PGA_get_MSID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_PGA *) ad)->MSID, &js);

    /*
     * since we return a plain string we have to dup it (get_string does not
     * do it for us). All this because we have to release the new js now,
     * otherwise there's be no other opportunity; the reference will be
     * forgotten when we return.
     * FIXME: jice@jxta.org 20020228 - It would be much better to return the
     * jstring directly.
     */

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_PGA_set_MSID(Jxta_PGA * ad, Jxta_id * id)
{
    JXTA_OBJECT_SHARE(id);
    JXTA_OBJECT_RELEASE(ad->MSID);
    ad->MSID = id;
}

JXTA_DECLARE(JString *) jxta_PGA_get_Name(Jxta_PGA * ad)
{
    JXTA_OBJECT_SHARE(ad->Name);
    return ad->Name;
}

char *JXTA_STDCALL jxta_PGA_get_Name_string(Jxta_advertisement * ad)
{
    /*
     * since we return a plain string we have to dup it (get_string does not
     * do it for us) otherwise the pointer would become invalid as soon
     * as this object is released, which would be rather unsafe since
     * some code could end-up naively holding a char pointer which validity
     * is subject to the reference count of an advertisement it might not
     * know anything about.
     * FIXME: jice@jxta.org 20020228 - It would be much better to return the
     * jstring directly.
     */

    return strdup(jstring_get_string(((Jxta_PGA *) ad)->Name));
}

JXTA_DECLARE(void) jxta_PGA_set_Name(Jxta_PGA * ad, JString * name)
{
    JXTA_OBJECT_SHARE(name);
    JXTA_OBJECT_RELEASE(ad->Name);
    ad->Name = name;
}

JXTA_DECLARE(JString *) jxta_PGA_get_Desc(Jxta_PGA * ad)
{
    JXTA_OBJECT_SHARE(ad->Desc);
    return ad->Desc;
}

char *JXTA_STDCALL jxta_PGA_get_Desc_string(Jxta_advertisement * ad)
{
    /*
     * since we return a plain string we have to dup it (get_string does not
     * do it for us) otherwise the pointer would become invalid as soon
     * as this object is released, which would be rather unsafe since
     * some code could end-up naively holding a char pointer which validity
     * is subject to the reference count of an advertisement it might not
     * know anything about.
     * FIXME: jice@jxta.org 20020228 - It would be much better to return the
     * jstring directly.
     */

    return strdup(jstring_get_string(((Jxta_PGA *) ad)->Desc));
}

JXTA_DECLARE(void) jxta_PGA_set_Desc(Jxta_PGA * ad, JString * desc)
{
    JXTA_OBJECT_SHARE(desc);
    JXTA_OBJECT_RELEASE(ad->Desc);
    ad->Desc = desc;
}

JXTA_DECLARE(char *) jxta_PGA_get_Svc(Jxta_PGA * ad)
{
    return NULL;
}

char *JXTA_STDCALL jxta_PGA_get_Svc_string(Jxta_advertisement * ad)
{
    return NULL;
}

void jxta_PGA_set_Svc(Jxta_PGA * ad, char *name)
{

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
    {"jxta:PGA", Jxta_PGA_, *handleJxta_PGA, *jxta_PGA_get_Jxta_PGA_string, NULL},
    {"GID", GID_, *handleGID, *jxta_PGA_get_GID_string, NULL},
    {"MSID", MSID_, *handleMSID, *jxta_PGA_get_MSID_string, NULL},
    {"Name", Name_, *handleName, *jxta_PGA_get_Name_string, NULL},
    {"Desc", Desc_, *handleDesc, *jxta_PGA_get_Desc_string, NULL},
    {"Svc", Svc_, *handleSvc, *jxta_PGA_get_Svc_string, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_PGA_get_xml(Jxta_PGA * ad, JString ** string)
{
    JString *gids;
    JString *msids;
    JString *tmp = jstring_new_0();

    jstring_append_2(tmp, "<?xml version=\"1.0\"?>" "<!DOCTYPE jxta:PGA>" "<jxta:PGA xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(tmp, "<GID>");
    jxta_id_to_jstring(ad->GID, &gids);
    jstring_append_1(tmp, gids);
    JXTA_OBJECT_RELEASE(gids);
    jstring_append_2(tmp, "</GID>\n");

    jstring_append_2(tmp, "<MSID>");
    jxta_id_to_jstring(ad->MSID, &msids);
    jstring_append_1(tmp, msids);
    JXTA_OBJECT_RELEASE(msids);
    jstring_append_2(tmp, "</MSID>\n");

    jstring_append_2(tmp, "<Name>");
    jstring_append_1(tmp, ad->Name);
    jstring_append_2(tmp, "</Name>\n");

    jstring_append_2(tmp, "<Desc>");
    jstring_append_1(tmp, ad->Desc);
    jstring_append_2(tmp, "</Desc>\n");

    jstring_append_2(tmp, "</jxta:PGA>\n");

    *string = tmp;

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
    Jxta_PGA *ad;

    ad = (Jxta_PGA *) malloc(sizeof(Jxta_PGA));
    memset(ad, 0xda, sizeof(Jxta_PGA));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:PGA",
                                  jxta_PGA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_PGA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_PGA_get_GID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_PGA_get_indexes, (FreeFunc) jxta_PGA_delete);

    JXTA_OBJECT_SHARE(jxta_id_nullID);
    JXTA_OBJECT_SHARE(jxta_id_nullID);

    ad->GID = jxta_id_nullID;
    ad->MSID = jxta_id_nullID;
    ad->Name = jstring_new_0();
    ad->Desc = jstring_new_0();

    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void jxta_PGA_delete(Jxta_PGA * ad)
{
    /* Fill in the required freeing functions here. */

    JXTA_OBJECT_RELEASE(ad->GID);
    JXTA_OBJECT_RELEASE(ad->MSID);
    JXTA_OBJECT_RELEASE(ad->Name);
    JXTA_OBJECT_RELEASE(ad->Desc);

    jxta_advertisement_destruct((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(Jxta_PGA));
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

JXTA_DECLARE(void) jxta_PGA_parse_charbuffer(Jxta_PGA * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

/** The main external method used to extract 
 * the data from the xml stream.
 */

JXTA_DECLARE(void) jxta_PGA_parse_file(Jxta_PGA * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_PGA *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = Jxta_PGA_new();

    testfile = fopen(argv[1], "r");
    Jxta_PGA_parse_file(ad, fprintf, testfile);
    fclose(testfile);

    Jxta_PGA_print_xml(ad, stdout);
    Jxta_PGA_delete(ad);

    return 0;
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
