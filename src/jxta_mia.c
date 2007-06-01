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
 * $Id: jxta_mia.c,v 1.30 2006/02/18 00:32:51 slowhog Exp $
 */


/* 
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c jxta_MIA.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/
static const char *__log_cat = "MIA";

#include <stdio.h>
#include <string.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jxta_mia.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    jxta_MIA_,
    MSID_,
    Comp_,
    Code_,
    PURI_,
    Prov_,
    Desc_,
    Parm_,
    Efmt_,
    Bind_,
    MCID_,
    Svc_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_MIA {
    Jxta_advertisement jxta_advertisement;
    char *jxta_MIA;
    Jxta_id *MSID;
    JString *Comp;
    JString *Code;
    JString *PURI;
    JString *Prov;
    JString *Desc;
    JString *Parm;
    Jxta_boolean in_parm;       /* When this is true, Parm accumulates everything
                                   we see without interpretation. (including Comp) */
    Jxta_boolean in_comp;       /* When this is true, Comp accumulates everything
                                   we see without interpretation. (including Parm) */
};

/*
 * Forw decl. for unexported function
 */
static void jxta_MIA_delete(Jxta_MIA * ad);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 * MIA, Efmt, Bind, MCID, and Svc are all handled only as embedded opaque
 * elements. They can occur inside Comp or Param and we just copy them as-is
 * to the comp, or Param string. If they occur outside, they're ignored.
 */
static void handlejxta_MIA(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<jxta:MIA>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</jxta:MIA>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<jxta:MIA>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</jxta:MIA>\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In jxta:MIA element\n");
}

static void handleEfmt(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Efmt>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Efmt>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Efmt>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Efmt>\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Efmt element\n");
}

static void handleBind(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Bind>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Bind>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Bind>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Bind>\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Bind element\n");
}

static void handleMCID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<MCID>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</MCID>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<MCID>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</MCID>\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MCID element\n");
}

static void handleSvc(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Svc>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Svc>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Svc>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Svc>\n");
        return;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Svc element\n");
}

static void handleMSID(void *userdata, const XML_Char * cd, int len)
{
    JString *tmp;
    Jxta_id *msid = NULL;
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<MSID>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</MSID>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<MSID>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</MSID>\n");
        return;
    }

    if (len == 0)
        return;

    tmp = jstring_new_1(len + 1);
    jstring_append_0(tmp, cd, len);
    jstring_trim(tmp);
    jxta_id_from_jstring(&msid, tmp);
    JXTA_OBJECT_RELEASE(tmp);
    if (msid != NULL) {
        jxta_MIA_set_MSID(ad, msid);
        JXTA_OBJECT_RELEASE(msid);
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MSID element\n");
}

static void handleCode(void *userdata, const XML_Char * cd, int len)
{
    JString *tmp;
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Code>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Code>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Code>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Code>\n");
        return;
    }

    if (len == 0)
        return;

    tmp = jstring_new_1(len + 1);
    jstring_append_0(tmp, cd, len);
    jstring_trim(tmp);
    jxta_MIA_set_Code(ad, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Code element\n");
}

static void handlePURI(void *userdata, const XML_Char * cd, int len)
{
    JString *tmp;
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<PURI>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</PURI>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<PURI>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</PURI>\n");
        return;
    }
    if (len == 0)
        return;

    tmp = jstring_new_1(len + 1);
    jstring_append_0(tmp, cd, len);
    jstring_trim(tmp);
    jxta_MIA_set_PURI(ad, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In PURI element\n");
}

static void handleProv(void *userdata, const XML_Char * cd, int len)
{
    JString *tmp;
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Prov>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Prov>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Prov>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Prov>\n");
        return;
    }
    if (len == 0)
        return;

    tmp = jstring_new_1(len + 1);
    jstring_append_0(tmp, cd, len);
    jstring_trim(tmp);
    jxta_MIA_set_Prov(ad, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Prov element\n");
}

static void handleDesc(void *userdata, const XML_Char * cd, int len)
{
    JString *tmp;
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Desc>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Desc>\n");
        return;
    }
    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Desc>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Desc>\n");
        return;
    }

    if (len == 0)
        return;

    tmp = jstring_new_1(len + 1);
    jstring_append_0(tmp, cd, len);
    jstring_trim(tmp);
    jxta_MIA_set_Desc(ad, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Desc element\n");
}

static void handleComp(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_parm) {
        if (len == 0) {
            jstring_append_2(ad->Parm, "<Comp>");
            return;
        }
        jstring_append_0(ad->Parm, cd, len);
        jstring_append_2(ad->Parm, "</Comp>\n");
        return;
    }
    ad->in_comp = !ad->in_comp;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Comp element\n");
}

static void handleParm(void *userdata, const XML_Char * cd, int len)
{
    Jxta_MIA *ad = (Jxta_MIA *) userdata;

    if (ad->in_comp) {
        if (len == 0) {
            jstring_append_2(ad->Comp, "<Parm>");
            return;
        }
        jstring_append_0(ad->Comp, cd, len);
        jstring_append_2(ad->Comp, "</Parm>\n");
        return;
    }
    ad->in_parm = !ad->in_parm;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In Parm element\n");
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(char *) jxta_MIA_get_jxta_MIA(Jxta_MIA * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_MIA_set_jxta_MIA(Jxta_MIA * ad, char *name)
{

}

JXTA_DECLARE(Jxta_id *) jxta_MIA_get_MSID(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->MSID);
    return ad->MSID;
}

static char *JXTA_STDCALL jxta_get_msid_string(Jxta_advertisement * ad)
{
    JString *msids = NULL;
    char *tmps;
    Jxta_status status;

    status = jxta_id_to_jstring(((Jxta_MIA *) ad)->MSID, &msids);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(msids);
        return NULL;
    }
    tmps = strdup(jstring_get_string(msids));
    JXTA_OBJECT_RELEASE(msids);
    return tmps;
}

JXTA_DECLARE(void) jxta_MIA_set_MSID(Jxta_MIA * ad, Jxta_id * id)
{
    JXTA_OBJECT_SHARE(id);
    JXTA_OBJECT_RELEASE(ad->MSID);
    ad->MSID = id;
}

JXTA_DECLARE(JString *) jxta_MIA_get_Comp(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->Comp);
    return ad->Comp;
}

JXTA_DECLARE(void) jxta_MIA_set_Comp(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->Comp);
    ad->Comp = str;
}

JXTA_DECLARE(JString *) jxta_MIA_get_Code(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->Code);
    return ad->Code;
}

JXTA_DECLARE(void) jxta_MIA_set_Code(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->Code);
    ad->Code = str;
}

JXTA_DECLARE(JString *)
    jxta_MIA_get_PURI(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->PURI);
    return ad->PURI;
}

JXTA_DECLARE(void) jxta_MIA_set_PURI(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->PURI);
    ad->PURI = str;
}

JXTA_DECLARE(JString *) jxta_MIA_get_Prov(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->Prov);
    return ad->Prov;
}

JXTA_DECLARE(void) jxta_MIA_set_Prov(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->Prov);
    ad->Prov = str;
}

JXTA_DECLARE(JString *) jxta_MIA_get_Desc(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->Desc);
    return ad->Desc;
}

JXTA_DECLARE(void) jxta_MIA_set_Desc(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->Desc);
    ad->Desc = str;
}

JXTA_DECLARE(JString *) jxta_MIA_get_Parm(Jxta_MIA * ad)
{
    JXTA_OBJECT_SHARE(ad->Parm);
    return ad->Parm;
}

JXTA_DECLARE(void) jxta_MIA_set_Parm(Jxta_MIA * ad, JString * str)
{
    JXTA_OBJECT_SHARE(str);
    JXTA_OBJECT_RELEASE(ad->Parm);
    ad->Parm = str;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab jxta_MIA_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:MIA", jxta_MIA_, *handlejxta_MIA, NULL, NULL},
    {"MSID", MSID_, *handleMSID, jxta_get_msid_string, NULL},
    {"Comp", Comp_, *handleComp, NULL, NULL},
    {"Code", Code_, *handleCode, NULL, NULL},
    {"PURI", PURI_, *handlePURI, NULL, NULL},
    {"Prov", Prov_, *handleProv, NULL, NULL},
    {"Desc", Desc_, *handleDesc, NULL, NULL},
    {"Parm", Parm_, *handleParm, NULL, NULL},
    {"Efmt", Efmt_, *handleEfmt, NULL, NULL},
    {"Bind", Bind_, *handleBind, NULL, NULL},
    {"MCID", MCID_, *handleMCID, NULL, NULL},
    {"Svc", Svc_, *handleSvc, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_MIA_get_xml(Jxta_MIA * ad, JString ** string)
{
    JString *mcids;
    JString *tmp = jstring_new_0();

    jstring_append_2(tmp, "<?xml version=\"1.0\"?>" "<!DOCTYPE jxta:MIA>" "<jxta:MIA xmlns:jxta=\"http://jxta.org\">\n");

    jstring_append_2(tmp, "<MSID>");
    jxta_id_to_jstring(ad->MSID, &mcids);
    jstring_append_1(tmp, mcids);
    JXTA_OBJECT_RELEASE(mcids);
    jstring_append_2(tmp, "</MSID>\n");

    jstring_append_2(tmp, "<Comp>");
    jstring_append_1(tmp, ad->Comp);
    jstring_append_2(tmp, "</Comp>\n");

    jstring_append_2(tmp, "<Code>");
    jstring_append_1(tmp, ad->Code);
    jstring_append_2(tmp, "</Code>\n");

    jstring_append_2(tmp, "<PURI>");
    jstring_append_1(tmp, ad->PURI);
    jstring_append_2(tmp, "</PURI>\n");

    jstring_append_2(tmp, "<Prov>");
    jstring_append_1(tmp, ad->Prov);
    jstring_append_2(tmp, "</Prov>\n");

    jstring_append_2(tmp, "<Desc>");
    jstring_append_1(tmp, ad->Desc);
    jstring_append_2(tmp, "</Desc>\n");

    jstring_append_2(tmp, "<Parm>");
    jstring_append_1(tmp, ad->Parm);
    jstring_append_2(tmp, "</Parm>\n");

    jstring_append_2(tmp, "</jxta:MIA>\n");

    *string = tmp;

    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_MIA *) jxta_MIA_new()
{
    Jxta_MIA *ad;

    ad = (Jxta_MIA *) malloc(sizeof(Jxta_MIA));
    memset(ad, 0, sizeof(Jxta_MIA));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:MIA",
                                  jxta_MIA_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_MIA_get_xml,
                                  (JxtaAdvertisementGetIDFunc) jxta_MIA_get_MSID,
                                  (JxtaAdvertisementGetIndexFunc) jxta_MIA_get_indexes, (FreeFunc) jxta_MIA_delete);

    /* Fill in the required initialization code here. */
    JXTA_OBJECT_SHARE(jxta_id_nullID);
    ad->MSID = jxta_id_nullID;
    ad->Comp = jstring_new_0();
    ad->Code = jstring_new_0();
    ad->PURI = jstring_new_0();
    ad->Prov = jstring_new_0();
    ad->Desc = jstring_new_0();
    ad->Parm = jstring_new_0();
    ad->in_parm = FALSE;
    ad->in_comp = FALSE;
    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void jxta_MIA_delete(Jxta_MIA * ad)
{
    /* Fill in the required freeing functions here. */

    JXTA_OBJECT_RELEASE(ad->MSID);
    JXTA_OBJECT_RELEASE(ad->Comp);
    JXTA_OBJECT_RELEASE(ad->Code);
    JXTA_OBJECT_RELEASE(ad->PURI);
    JXTA_OBJECT_RELEASE(ad->Prov);
    JXTA_OBJECT_RELEASE(ad->Desc);
    JXTA_OBJECT_RELEASE(ad->Parm);

    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(Jxta_MIA));
    free(ad);
}

JXTA_DECLARE(Jxta_vector *) jxta_MIA_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = {
        {"MSID", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx[0]);
}

JXTA_DECLARE(void) jxta_MIA_parse_charbuffer(Jxta_MIA * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_MIA_parse_file(Jxta_MIA * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_MIA *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_MIA_new();

    testfile = fopen(argv[1], "r");
    jxta_MIA_parse_file(ad, testfile);
    fclose(testfile);

    /* jxta_MIA_print_xml(ad,fprintf,stdout); */
    jxta_MIA_delete(ad);

    return 0;
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
