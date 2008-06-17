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

static const char *const __log_cat = "MSA";

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_msa.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    MSA_,
    MSID_,
    Name_,
    Crtr_,
    SURI_,
    Vers_,
    Desc_,
    Parm_,
    PipeAdvertisement_,
    Proxy_,
    Auth_
};

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/
struct _jxta_MSA {
    Jxta_advertisement jxta_advertisement;
    char *MSA;
    Jxta_id *MSID;
    JString *Name;
    JString *Crtr;
    JString *SURI;
    JString *Vers;
    JString *Desc;
    JString *Parm;
    Jxta_pipe_adv *PipeAdvertisement;
    JString *Proxy;
    JString *Auth;
};

static void MSA_delete(void * me);

/** Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/
static void handleMSA(void *userdata, const XML_Char * cd, int len)
{
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MSA element \n");
}

static void handleMSID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *msid = NULL;
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
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
        jxta_MSA_set_MSID(ad, msid);
        JXTA_OBJECT_RELEASE(msid);
    }
}

static void handleName(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *nm;

    if (len == 0)
        return;

    nm = jstring_new_1(len);
    jstring_append_0(nm, cd, len);
    jstring_trim(nm);

    jxta_MSA_set_Name(ad, nm);
    JXTA_OBJECT_RELEASE(nm);
}

static void handleCrtr(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Crtr;

    if (len == 0)
        return;

    Crtr = jstring_new_1(len);
    jstring_append_0(Crtr, cd, len);
    jstring_trim(Crtr);

    jxta_MSA_set_Crtr(ad, Crtr);
    JXTA_OBJECT_RELEASE(Crtr);
}

static void handleSURI(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *SURI;

    if (len == 0)
        return;

    SURI = jstring_new_1(len);
    jstring_append_0(SURI, cd, len);
    jstring_trim(SURI);

    jxta_MSA_set_SURI(ad, SURI);
    JXTA_OBJECT_RELEASE(SURI);
}

static void handleVers(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Vers;

    if (len == 0)
        return;

    Vers = jstring_new_1(len);
    jstring_append_0(Vers, cd, len);
    jstring_trim(Vers);

    jxta_MSA_set_Vers(ad, Vers);
    JXTA_OBJECT_RELEASE(Vers);
}

static void handleDesc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Desc;

    if (len == 0)
        return;

    Desc = jstring_new_1(len);
    jstring_append_0(Desc, cd, len);
    jstring_trim(Desc);

    jxta_MSA_set_Desc(ad, Desc);
    JXTA_OBJECT_RELEASE(Desc);
}

static void handleParm(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Parm;

    if (len == 0)
        return;

    Parm = jstring_new_1(len);
    jstring_append_0(Parm, cd, len);
    jstring_trim(Parm);

    jxta_MSA_set_Parm(ad, Parm);
    JXTA_OBJECT_RELEASE(Parm);
}

static void handlePipeAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    Jxta_pipe_adv *pipeadv;
    
    if( 0 != len ) {
        return;
    }

    pipeadv = jxta_pipe_adv_new();
    jxta_MSA_set_PipeAdvertisement(ad, pipeadv);
    JXTA_OBJECT_RELEASE(pipeadv);

    jxta_advertisement_set_handlers((Jxta_advertisement *) pipeadv, ((Jxta_advertisement *) ad)->parser, (void *) ad);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In PipeAdvertisement element\n");
}

static void handleProxy(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Proxy;

    if (len == 0)
        return;

    Proxy = jstring_new_1(len);
    jstring_append_0(Proxy, cd, len);
    jstring_trim(Proxy);

    jxta_MSA_set_Proxy(ad, Proxy);
    JXTA_OBJECT_RELEASE(Proxy);
}

static void handleAuth(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MSA *ad = (Jxta_MSA *) userdata;
    JString *Auth;

    if (len == 0)
        return;

    Auth = jstring_new_1(len);
    jstring_append_0(Auth, cd, len);
    jstring_trim(Auth);

    jxta_MSA_set_Auth(ad, Auth);
    JXTA_OBJECT_RELEASE(Auth);
}

/** The get/set functions represent the public
* interface to the ad class, that is, the API.
*/
JXTA_DECLARE(char *) jxta_MSA_get_MSA(Jxta_MSA * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_MSA_set_MSA(Jxta_MSA * ad, char *name)
{
}

JXTA_DECLARE(Jxta_id *) jxta_MSA_get_MSID(Jxta_MSA * ad)
{
    if(ad->MSID){
        JXTA_OBJECT_SHARE(ad->MSID);
    }

    return ad->MSID;
}

JXTA_DECLARE(void) jxta_MSA_set_MSID(Jxta_MSA * ad, Jxta_id * MSID)
{
    if (ad->MSID)
        JXTA_OBJECT_RELEASE(ad->MSID);

    if (MSID)
        JXTA_OBJECT_SHARE(MSID);

    ad->MSID = MSID;
}

static char *JXTA_STDCALL jxta_MSA_get_MSID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;

    jxta_id_to_jstring(((Jxta_MSA *) ad)->MSID, &js);

    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);

    return res;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Name(Jxta_MSA * ad)
{
    if (ad->Name)
        JXTA_OBJECT_SHARE(ad->Name);

    return ad->Name;
}

static char *JXTA_STDCALL jxta_MSA_get_Name_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_MSA *) ad)->Name));
}

JXTA_DECLARE(void) jxta_MSA_set_Name(Jxta_MSA * ad, JString * name)
{
    if (ad->Name)
        JXTA_OBJECT_RELEASE(ad->Name);

    if (name)
        JXTA_OBJECT_SHARE(name);

    ad->Name = name;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Crtr(Jxta_MSA * ad)
{
    if (ad->Crtr)
        JXTA_OBJECT_SHARE(ad->Crtr);

    return ad->Crtr;
}

JXTA_DECLARE(void) jxta_MSA_set_Crtr(Jxta_MSA * ad, JString * Crtr)
{
    if (ad->Crtr)
        JXTA_OBJECT_RELEASE(ad->Crtr);

    if (Crtr)
        JXTA_OBJECT_SHARE(Crtr);

    ad->Crtr = Crtr;
}

JXTA_DECLARE(JString *) jxta_MSA_get_SURI(Jxta_MSA * ad)
{
    if (ad->SURI)
        JXTA_OBJECT_SHARE(ad->SURI);

    return ad->SURI;
}

JXTA_DECLARE(void) jxta_MSA_set_SURI(Jxta_MSA * ad, JString * SURI)
{
    if (ad->SURI)
        JXTA_OBJECT_RELEASE(ad->SURI);

    if (SURI)
        JXTA_OBJECT_SHARE(SURI);

    ad->SURI = SURI;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Vers(Jxta_MSA * ad)
{
    if (ad->Vers)
        JXTA_OBJECT_SHARE(ad->Vers);

    return ad->Vers;
}

JXTA_DECLARE(void) jxta_MSA_set_Vers(Jxta_MSA * ad, JString * Vers)
{
    if (ad->Vers)
        JXTA_OBJECT_RELEASE(ad->Vers);

    if (Vers)
        JXTA_OBJECT_SHARE(Vers);

    ad->Vers = Vers;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Desc(Jxta_MSA * ad)
{
    if (ad->Desc)
        JXTA_OBJECT_SHARE(ad->Desc);

    return ad->Desc;
}

JXTA_DECLARE(void) jxta_MSA_set_Desc(Jxta_MSA * ad, JString * Desc)
{
    if (ad->Desc)
        JXTA_OBJECT_RELEASE(ad->Desc);

    if (Desc)
        JXTA_OBJECT_SHARE(Desc);

    ad->Desc = Desc;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Parm(Jxta_MSA * ad)
{
    if (ad->Parm)
        JXTA_OBJECT_SHARE(ad->Parm);

    return ad->Parm;
}

JXTA_DECLARE(void) jxta_MSA_set_Parm(Jxta_MSA * ad, JString * Parm)
{
    if (ad->Parm)
        JXTA_OBJECT_RELEASE(ad->Parm);

    if (Parm)
        JXTA_OBJECT_SHARE(Parm);

    ad->Parm = Parm;
}

JXTA_DECLARE(Jxta_pipe_adv *) jxta_MSA_get_PipeAdvertisement(Jxta_MSA * ad)
{
    if (ad->PipeAdvertisement)
        JXTA_OBJECT_SHARE(ad->PipeAdvertisement);

    return ad->PipeAdvertisement;
}

JXTA_DECLARE(void) jxta_MSA_set_PipeAdvertisement(Jxta_MSA * ad, Jxta_pipe_adv * PipeAdvertisement)
{
    if (ad->PipeAdvertisement)
        JXTA_OBJECT_RELEASE(ad->PipeAdvertisement);

    if (PipeAdvertisement)
        JXTA_OBJECT_SHARE(PipeAdvertisement);

    ad->PipeAdvertisement = PipeAdvertisement;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Proxy(Jxta_MSA * ad)
{
    if (ad->Proxy)
        JXTA_OBJECT_SHARE(ad->Proxy);

    return ad->Proxy;
}

JXTA_DECLARE(void) jxta_MSA_set_Proxy(Jxta_MSA * ad, JString * Proxy)
{
    if (ad->Proxy)
        JXTA_OBJECT_RELEASE(ad->Proxy);

    if (Proxy)
        JXTA_OBJECT_SHARE(Proxy);

    ad->Proxy = Proxy;
}

JXTA_DECLARE(JString *) jxta_MSA_get_Auth(Jxta_MSA * ad)
{
    if (ad->Auth)
        JXTA_OBJECT_SHARE(ad->Auth);

    return ad->Auth;
}

JXTA_DECLARE(void) jxta_MSA_set_Auth(Jxta_MSA * ad, JString * Auth)
{
    if (ad->Auth)
        JXTA_OBJECT_RELEASE(ad->Auth);

    if (Auth)
        JXTA_OBJECT_SHARE(Auth);

    ad->Auth = Auth;
}

/** Now, build an array of the keyword structs.  Since
* a top-level, or null state may be of interest, 
* let that lead off.  Then, walk through the enums,
* initializing the struct array with the correct fields.
* Later, the stream will be dispatched to the handler based
* on the value in the char * kwd.
*/
static const Kwdtab MSA_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:MSA", MSA_, *handleMSA, NULL, NULL},
    {"MSID", MSID_, *handleMSID, *jxta_MSA_get_MSID_string, NULL},
    {"Name", Name_, *handleName, *jxta_MSA_get_Name_string, NULL},
    {"Crtr", Crtr_, *handleCrtr, NULL, NULL},
    {"SURI", SURI_, *handleSURI, NULL, NULL},
    {"Vers", Vers_, *handleVers, NULL, NULL},
    {"Desc", Desc_, *handleDesc, NULL, NULL},
    {"Parm", Parm_, *handleParm, NULL, NULL},
    {"jxta:PipeAdvertisement", PipeAdvertisement_, *handlePipeAdvertisement, NULL, NULL},
    {"Proxy", Proxy_, *handleProxy, NULL, NULL},
    {"Auth", Auth_, *handleAuth, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_MSA_get_xml(Jxta_MSA * ad, JString ** string)
{
    JString *msids;
    JString *pipeadv_str;
    Jxta_status status;
    JString *tmp = jstring_new_0();

    jstring_append_2(tmp, "<?xml version=\"1.0\"?>" "<!DOCTYPE jxta:MSA>" "<jxta:MSA xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(tmp, "<MSID>");
    jxta_id_to_jstring(ad->MSID, &msids);
    jstring_append_1(tmp, msids);
    JXTA_OBJECT_RELEASE(msids);
    jstring_append_2(tmp, "</MSID>\n");

    jstring_append_2(tmp, "<Name>");
    jstring_append_1(tmp, ad->Name);
    jstring_append_2(tmp, "</Name>\n");

    jstring_append_2(tmp, "<Crtr>");
    jstring_append_1(tmp, ad->Crtr);
    jstring_append_2(tmp, "</Crtr>\n");

    jstring_append_2(tmp, "<SURI>");
    jstring_append_1(tmp, ad->SURI);
    jstring_append_2(tmp, "</SURI>\n");

    jstring_append_2(tmp, "<Vers>");
    jstring_append_1(tmp, ad->Vers);
    jstring_append_2(tmp, "</Vers>\n");

    jstring_append_2(tmp, "<Desc>");
    jstring_append_1(tmp, ad->Desc);
    jstring_append_2(tmp, "</Desc>\n");

    jstring_append_2(tmp, "<Parm>");
    jstring_append_1(tmp, ad->Parm);
    jstring_append_2(tmp, "</Parm>\n");

    jstring_append_2(tmp, "<Proxy>");
    jstring_append_1(tmp, ad->Proxy);
    jstring_append_2(tmp, "</Proxy>\n");

    if (ad->PipeAdvertisement) {
        status = jxta_pipe_adv_get_xml(ad->PipeAdvertisement, &pipeadv_str);
        if (status == JXTA_SUCCESS) {
            jstring_append_1(tmp, pipeadv_str);
            JXTA_OBJECT_RELEASE(pipeadv_str);
        }
    }

    jstring_append_2(tmp, "<Auth>");
    jstring_append_1(tmp, ad->Auth);
    jstring_append_2(tmp, "</Auth>\n");

    jstring_append_2(tmp, "</jxta:MSA>\n");

    *string = tmp;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad.
* The memory gets shredded going in to 
* a value that is easy to see in a debugger,
* just in case there is a segfault (not that 
* that would ever happen, but in case it ever did.)
*/
JXTA_DECLARE(Jxta_MSA *) jxta_MSA_new()
{
    Jxta_MSA *ad;

    ad = (Jxta_MSA *) malloc(sizeof(Jxta_MSA));
    memset(ad, 0xda, sizeof(Jxta_MSA));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:MSA",
                                  MSA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_MSA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_MSA_get_MSID,
                                  jxta_MSA_get_indexes, MSA_delete);

    JXTA_OBJECT_SHARE(jxta_id_nullID);
    ad->MSID = jxta_id_nullID;
    ad->Name = jstring_new_0();
    ad->Crtr = jstring_new_0();
    ad->SURI = jstring_new_0();
    ad->Vers = jstring_new_0();
    ad->Desc = jstring_new_0();
    ad->Parm = jstring_new_0();
    ad->PipeAdvertisement = NULL;
    ad->Proxy = jstring_new_0();
    ad->Auth = jstring_new_0();

    return ad;
}

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
static void MSA_delete(void * me)
{
    Jxta_MSA * ad = (Jxta_MSA *) me;

    /* Fill in the required freeing functions here. */

    JXTA_OBJECT_RELEASE(ad->MSID);
    JXTA_OBJECT_RELEASE(ad->Name);
    JXTA_OBJECT_RELEASE(ad->Crtr);
    JXTA_OBJECT_RELEASE(ad->SURI);
    JXTA_OBJECT_RELEASE(ad->Vers);
    JXTA_OBJECT_RELEASE(ad->Desc);
    JXTA_OBJECT_RELEASE(ad->Parm);
    JXTA_OBJECT_RELEASE(ad->PipeAdvertisement);
    JXTA_OBJECT_RELEASE(ad->Proxy);
    JXTA_OBJECT_RELEASE(ad->Auth);

    jxta_advertisement_destruct((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_MSA));
    free(ad);
}

JXTA_DECLARE(void) jxta_MSA_parse_charbuffer(Jxta_MSA * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_MSA_parse_file(Jxta_MSA * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *) jxta_MSA_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"Name", NULL},
        {"MSID", NULL},
        {NULL, NULL}
    };

    return jxta_advertisement_return_indexes(idx[0]);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_MSA *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_MSA_new();

    testfile = fopen(argv[1], "r");
    jxta_MSA_parse_file(ad, testfile);
    fclose(testfile);

    /* MSA_print_xml(ad,fprintf,stdout); */
    JXTA_OBJECT_RELEASE(ad);

    return 0;
}

#endif

/* vim: set ts=4 sw=4 et tw=130: */
