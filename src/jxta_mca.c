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
 * $Id: jxta_mca.c,v 1.27 2006/09/29 01:28:44 slowhog Exp $
 */

static const char *const __log_cat = "MCA";

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_mca.h"

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    MCA_,
    MCID_,
    Name_,
    Desc_
};

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/
struct _jxta_MCA {
    Jxta_advertisement jxta_advertisement;
    char *jxta_MCA;

    Jxta_id *MCID;
    JString *Name;
    JString *Desc;
};

static void MCA_delete(void * me);

/** Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/
static void handleMCA(void *userdata, const XML_Char * cd, int len)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MCA element\n");
}

static void handleMCID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *cid = NULL;
    Jxta_MCA *ad = (Jxta_MCA *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&cid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (cid != NULL) {
        jxta_MCA_set_MCID(ad, cid);
        JXTA_OBJECT_RELEASE(cid);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MCID element\n");
}

static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MCA *ad = (Jxta_MCA *) userdata;
    JString *nm;

    if (len == 0)
        return;

    nm = jstring_new_1(len);
    jstring_append_0(nm, cd, len);
    jstring_trim(nm);

    jxta_MCA_set_Name(ad, nm);

    JXTA_OBJECT_RELEASE(nm);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Name element\n");
}

static void handleDesc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MCA *ad = (Jxta_MCA *) userdata;
    JString *desc;

    if (len == 0)
        return;

    desc = jstring_new_1(len);
    jstring_append_0(desc, cd, len);
    jstring_trim(desc);

    jxta_MCA_set_Desc(ad, desc);

    JXTA_OBJECT_RELEASE(desc);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Desc element\n");
}

/** The get/set functions represent the public
* interface to the ad class, that is, the API.
*/
JXTA_DECLARE(char *) jxta_MCA_get_MCA(Jxta_MCA * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_MCA_set_MCA(Jxta_MCA * ad, char *name)
{
}

JXTA_DECLARE(Jxta_id *) jxta_MCA_get_MCID(Jxta_MCA * ad)
{
    if (ad->MCID)
        JXTA_OBJECT_SHARE(ad->MCID);

    return ad->MCID;
}

JXTA_DECLARE(void) jxta_MCA_set_MCID(Jxta_MCA * ad, Jxta_id * id)
{
    if (ad->MCID)
        JXTA_OBJECT_RELEASE(ad->MCID);

    if (id)
        JXTA_OBJECT_SHARE(id);

    ad->MCID = id;
}

static char *JXTA_STDCALL jxta_PGA_get_MCID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_MCA *) ad)->MCID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);

    return res;
}

JXTA_DECLARE(JString *) jxta_MCA_get_Name(Jxta_MCA * ad)
{
    if (ad->Name)
        JXTA_OBJECT_SHARE(ad->Name);

    return ad->Name;
}

static  char *JXTA_STDCALL jxta_MCA_get_Name_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_MCA *) ad)->Name));
}

JXTA_DECLARE(void) jxta_MCA_set_Name(Jxta_MCA * ad, JString * name)
{
    if (ad->Name)
        JXTA_OBJECT_RELEASE(ad->Name);

    if (name)
        JXTA_OBJECT_SHARE(name);

    ad->Name = name;
}

JXTA_DECLARE(JString *) jxta_MCA_get_Desc(Jxta_MCA * ad)
{
    if (ad->Desc)
        JXTA_OBJECT_SHARE(ad->Desc);

    return ad->Desc;
}

JXTA_DECLARE(void) jxta_MCA_set_Desc(Jxta_MCA * ad, JString * desc)
{
    if (ad->Desc)
        JXTA_OBJECT_RELEASE(ad->Desc);

    if (desc)
        JXTA_OBJECT_SHARE(desc);

    ad->Desc = desc;
}

/** Now, build an array of the keyword structs.  Since
* a top-level, or null state may be of interest, 
* let that lead off.  Then, walk through the enums,
* initializing the struct array with the correct fields.
* Later, the stream will be dispatched to the handler based
* on the value in the char * kwd.
*/
static const Kwdtab MCA_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:MCA", MCA_, *handleMCA, NULL, NULL},
    {"MCID", MCID_, *handleMCID, *jxta_PGA_get_MCID_string, NULL},
    {"Name", Name_, *handleName, *jxta_MCA_get_Name_string, NULL},
    {"Desc", Desc_, *handleDesc, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_MCA_get_xml(Jxta_MCA * ad, JString ** string)
{
    JString *cids;
    JString *tmp = jstring_new_0();

    jstring_append_2(tmp, "<?xml version=\"1.0\"?>" "<!DOCTYPE jxta:MCA>" "<jxta:MCA xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(tmp, "<MCID>");
    jxta_id_to_jstring(ad->MCID, &cids);
    jstring_append_1(tmp, cids);
    JXTA_OBJECT_RELEASE(cids);
    jstring_append_2(tmp, "</MCID>\n");

    jstring_append_2(tmp, "<Name>");
    jstring_append_1(tmp, ad->Name);
    jstring_append_2(tmp, "</Name>\n");

    jstring_append_2(tmp, "<Desc>");
    jstring_append_1(tmp, ad->Desc);
    jstring_append_2(tmp, "</Desc>\n");

    jstring_append_2(tmp, "</jxta:MCA>\n");

    *string = tmp;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad.
* The memory gets shredded going in to 
* a value that is easy to see in a debugger,
* just in case there is a segfault (not that 
* that would ever happen, but in case it ever did.)
*/
JXTA_DECLARE(Jxta_MCA *) jxta_MCA_new()
{
    Jxta_MCA *ad;

    ad = (Jxta_MCA *) malloc(sizeof(Jxta_MCA));
    memset(ad, 0xda, sizeof(Jxta_MCA));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:MCA",
                                  MCA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_MCA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_MCA_get_MCID,
                                  jxta_MCA_get_indexes, MCA_delete);

    JXTA_OBJECT_SHARE(jxta_id_nullID);
    ad->MCID = jxta_id_nullID;
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
static void MCA_delete(void * me)
{
    Jxta_MCA * ad = (Jxta_MCA *) me;
    /* Fill in the required freeing functions here. */

    JXTA_OBJECT_RELEASE(ad->MCID);
    JXTA_OBJECT_RELEASE(ad->Name);
    JXTA_OBJECT_RELEASE(ad->Desc);

    jxta_advertisement_destruct((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_MCA));
    free(ad);
}

JXTA_DECLARE(void) jxta_MCA_parse_charbuffer(Jxta_MCA * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_MCA_parse_file(Jxta_MCA * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *) jxta_MCA_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"Name", NULL},
        {"MCID", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_MCA *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_MCA_new();

    testfile = fopen(argv[1], "r");
    jxta_MCA_parse_file(ad, testfile);
    fclose(testfile);

    /* MCA_print_xml(ad,fprintf,stdout); */
    JXTA_OBJECT_RELEASE(ad);

    return 0;
}

#endif

/* vim: set ts=4 sw=4 et tw=130: */
