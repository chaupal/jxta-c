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
 * $Id: jxta_wm.c,v 1.26 2007/04/10 02:02:34 mmx2005 Exp $
 */

static const char *const __log_cat = "WireMessage";

#include <stdio.h>
#include <string.h>

#include "jxta_wm.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jstring.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the
 * xml ad.
 */ 
enum tokentype {
    Null_,
    JxtaWire_,
    SrcPeer_,
    VisitedPeer_,
    TTL_,
    PipeId_,
    MsgId_
};


/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _JxtaWire {
    Jxta_advertisement jxta_advertisement;
    char *JxtaWire;
    char *SrcPeer;
    Jxta_vector *VisitedPeer;
    int TTL;
    char *pipeId;
    char *msgId;
};

static void JxtaWire_delete(Jxta_object * obj);

/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxtaWire(void *userdata, const XML_Char * cd, int len)
{
    /* JxtaWire * ad = (JxtaWire*)userdata; */
    
	if(len == 0)
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Begining parse of JxtaWire\n");
	else
		jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "END JxtaWire\n");
}

static void handleSrcPeer(void *userdata, const XML_Char * cd, int len)
{
    JxtaWire *ad = (JxtaWire *) userdata;
    char *tok = (char *) malloc(len + 1);

    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->SrcPeer = tok;
    } else {
        free(tok);
    }
}

static void handleMsgId(void *userdata, const XML_Char * cd, int len)
{
    JxtaWire *ad = (JxtaWire *) userdata;
    char *tok = (char *) malloc(len + 1);

    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->msgId = tok;
    } else {
        free(tok);
    }
}

static void handlePipeId(void *userdata, const XML_Char * cd, int len)
{
    JxtaWire *ad = (JxtaWire *) userdata;
    char *tok = (char *) malloc(len + 1);

    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->pipeId = tok;
    } else {
        free(tok);
    }
}

static void handleTTL(void *userdata, const XML_Char * cd, int len)
{
    JxtaWire *ad = (JxtaWire *) userdata;
    char *tok = (char *) calloc(len + 1, sizeof(char));

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        sscanf(tok, "%d", &ad->TTL);
    }
    free(tok);
}

static void handleVisitedPeer(void *userdata, const XML_Char * cd, int len)
{
    JxtaWire *ad = (JxtaWire *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            JString *pt = jstring_new_2(tok);

            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "VisitedPeer: [%s]\n", tok);
            jxta_vector_add_object_last(ad->VisitedPeer, (Jxta_object *) pt);
            /* The vector shares automatically the object. We must release */
            JXTA_OBJECT_RELEASE(pt);
        }
        free(tok);
    }
}


/** The get/set functions represent the public
     * interface to the ad class, that is, the API.
*/

JXTA_DECLARE(char *) JxtaWire_get_JxtaWire(JxtaWire * ad)
{
    return NULL;
}

JXTA_DECLARE(void) JxtaWire_set_JxtaWire(JxtaWire * ad, const char *name)
{
}

JXTA_DECLARE(char *) JxtaWire_get_SrcPeer(JxtaWire * ad)
{
    return ad->SrcPeer;
}

JXTA_DECLARE(char *) JxtaWire_get_MsgId(JxtaWire * ad)
{
    return ad->msgId;
}

JXTA_DECLARE(char *) JxtaWire_get_PipeId(JxtaWire * ad)
{
    return ad->pipeId;
}

JXTA_DECLARE(void) JxtaWire_set_SrcPeer(JxtaWire * ad, const char *src)
{
    if (ad->SrcPeer != NULL) {
        free(ad->SrcPeer);
        ad->SrcPeer = NULL;
    }
    if (src != NULL) {
        ad->SrcPeer = (char *) malloc(strlen(src) + 1);
        strcpy(ad->SrcPeer, src);
    } else {
        ad->SrcPeer = NULL;
    }
}

JXTA_DECLARE(void) JxtaWire_set_PipeId(JxtaWire * ad, const char *src)
{
    if (ad->pipeId != NULL) {
        free(ad->pipeId);
        ad->pipeId = NULL;
    }
    if (src != NULL) {
        ad->pipeId = (char *) malloc(strlen(src) + 1);
        strcpy(ad->pipeId, src);
    } else {
        ad->pipeId = NULL;
    }
}

JXTA_DECLARE(void) JxtaWire_set_MsgId(JxtaWire * ad, const char *src)
{
    if (ad->msgId != NULL) {
        free(ad->msgId);
        ad->msgId = NULL;
    }
    if (src != NULL) {
        ad->msgId = (char *) malloc(strlen(src) + 1);
        strcpy(ad->msgId, src);
    } else {
        ad->msgId = NULL;
    }
}

JXTA_DECLARE(Jxta_vector *) JxtaWire_get_VisitedPeer(JxtaWire * ad)
{
    if (ad->VisitedPeer != NULL) {
        JXTA_OBJECT_SHARE(ad->VisitedPeer);
        return ad->VisitedPeer;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) JxtaWire_set_VisitedPeer(JxtaWire * ad, Jxta_vector * vector)
{
    if (ad->VisitedPeer != NULL) {
        JXTA_OBJECT_RELEASE(ad->VisitedPeer);
        ad->VisitedPeer = NULL;
    }
    if (vector != NULL) {
        JXTA_OBJECT_SHARE(vector);
        ad->VisitedPeer = vector;
    }
    return;
}

JXTA_DECLARE(int) JxtaWire_get_TTL(JxtaWire * ad)
{
    return ad->TTL;
}

JXTA_DECLARE(void) JxtaWire_set_TTL(JxtaWire * ad, int TTL)
{
    ad->TTL = TTL;
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab JxtaWire_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"JxtaWire", JxtaWire_, *handleJxtaWire, NULL, NULL},
    {"SrcPeer", SrcPeer_, *handleSrcPeer, NULL, NULL},
    {"VisitedPeer", VisitedPeer_, *handleVisitedPeer, NULL, NULL},
    {"PipeId", PipeId_, *handlePipeId, NULL, NULL},
    {"MsgId", MsgId_, *handleMsgId, NULL, NULL},
    {"TTL", TTL_, *handleTTL, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) JxtaWire_get_xml(JxtaWire * ad, JString ** xml)
{
    JString *string;
    char buf[128];              /* We use this buffer to store a string representation of a int < 10 */
    unsigned int i = 0;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE JxtaWire>");

    jstring_append_2(string, "<JxtaWire>\n");
    jstring_append_2(string, "<SrcPeer>");
    jstring_append_2(string, JxtaWire_get_SrcPeer(ad));
    jstring_append_2(string, "</SrcPeer>\n");
    jstring_append_2(string, "<PipeId>");
    jstring_append_2(string, JxtaWire_get_PipeId(ad));
    jstring_append_2(string, "</PipeId>\n");
    jstring_append_2(string, "<MsgId>");
    jstring_append_2(string, JxtaWire_get_MsgId(ad));
    jstring_append_2(string, "</MsgId>\n");
    jstring_append_2(string, "<TTL>");
    apr_snprintf(buf, sizeof(buf), "%d", JxtaWire_get_TTL(ad));
    jstring_append_2(string, (const char *) buf);
    jstring_append_2(string, "</TTL>\n");

    for (i = 0; i < jxta_vector_size(ad->VisitedPeer); ++i) {
        JString *path;
        Jxta_status res;

        res = jxta_vector_get_object_at(ad->VisitedPeer, JXTA_OBJECT_PPTR(&path), i);
        if ((res == JXTA_SUCCESS) && (path != NULL)) {
            jstring_append_2(string, "<VisitedPeer>");
            jstring_append_2(string, jstring_get_string(path));
            jstring_append_2(string, "</VisitedPeer>\n");
            JXTA_OBJECT_RELEASE(path);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to get VisitedPeer from vector\n");
        }
    }
    jstring_append_2(string, "</JxtaWire>\n");
    *xml = string;
    return JXTA_SUCCESS;
}


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
JXTA_DECLARE(JxtaWire *) JxtaWire_new(void)
{
    JxtaWire *ad;
    ad = (JxtaWire *) calloc(1, sizeof(JxtaWire));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "JxtaWire",
                                  JxtaWire_tags,
                                  (JxtaAdvertisementGetXMLFunc) JxtaWire_get_xml, NULL, NULL, JxtaWire_delete);

    ad->SrcPeer = NULL;
    ad->msgId = NULL;
    ad->pipeId = NULL;
    ad->VisitedPeer = jxta_vector_new(7);
    ad->TTL = 0;
    return ad;
}

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
static void JxtaWire_delete(Jxta_object * obj)
{
    JxtaWire *ad = (JxtaWire *) obj;

    /* Fill in the required freeing functions here. */
    if (ad->SrcPeer) {
        free(ad->SrcPeer);
        ad->SrcPeer = NULL;
    }

    if (ad->msgId) {
        free(ad->msgId);
        ad->msgId = NULL;
    }

    if (ad->pipeId) {
        free(ad->pipeId);
        ad->pipeId = NULL;
    }

    if (ad->VisitedPeer) {
        JXTA_OBJECT_RELEASE(ad->VisitedPeer);
        ad->VisitedPeer = NULL;
    }

    jxta_advertisement_destruct(&ad->jxta_advertisement);
    memset(ad, 0xdd, sizeof(JxtaWire));
    free(ad);
}

JXTA_DECLARE(void) JxtaWire_parse_charbuffer(JxtaWire * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) JxtaWire_parse_file(JxtaWire * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

/* vim: set ts=4 sw=4 et tw=130: */
