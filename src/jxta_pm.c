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

static const char *__log_cat = "PropMsg";

#include <stdio.h>
#include <string.h>

#include "jxta_pm.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"

/** Each of these corresponds to a tag in the xml ad.
 */
enum tokentype {
    Null_,
    RendezVousPropagateMessage_,
    DestSName_,
    DestSParam_,
    MessageId_,
    Path_,
    TTL_
};

    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
struct _RendezVousPropagateMessage {
    Jxta_advertisement jxta_advertisement;
    char *rendezVousPropagateMessage;
    JString *destSName;
    JString *destSParam;
    JString *messageId;
    Jxta_vector *path;
    int ttl;
};

static void RendezVousPropagateMessage_delete(Jxta_object * obj);
static Jxta_status validate_message(RendezVousPropagateMessage * myself);

    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */

static void handleRendezVousPropagateMessage(void *userdata, const XML_Char * cd, int len)
{
    /* RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata; */
}

static void handleDestSName(void *userdata, const XML_Char * cd, int len)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) userdata;

    if (0 == len)
        return;
    if (NULL == ad->destSName) {
        ad->destSName = jstring_new_1(len);
    }
    jstring_append_0(ad->destSName, cd, len);
    jstring_trim(ad->destSName);
}

static void handleDestSParam(void *userdata, const XML_Char * cd, int len)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) userdata;

    if (0 == len)
        return;
    if (NULL == ad->destSParam) {
        ad->destSParam = jstring_new_1(len);
    }
    jstring_append_0(ad->destSParam, cd, len);
    jstring_trim(ad->destSParam);
}

static void handleMessageId(void *userdata, const XML_Char * cd, int len)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) userdata;

    if (0 == len)
        return;
    if (NULL == ad->messageId) {
        ad->messageId = jstring_new_1(len);
    }
    jstring_append_0(ad->messageId, cd, len);
    jstring_trim(ad->messageId);
}

static void handleTTL(void *userdata, const XML_Char * cd, int len)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) userdata;

    char *tok = calloc(len + 1, sizeof(char));

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->ttl = atoi(tok);
    }
    free(tok);
}

static void handlePath(void *userdata, const XML_Char * cd, int len)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) userdata;
    char *tok;

    if (len > 0) {
        tok = malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            JString *pt = jstring_new_2(tok);
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Path: [%s]\n", tok);
            jxta_vector_add_object_last(ad->path, (Jxta_object *) pt);
            /* The vector shares automatically the object. We must release */
            JXTA_OBJECT_RELEASE(pt);
        }
        free(tok);
    }
}

 /** The get/set functions represent the public
   * interface to the ad class, that is, the API.
   */
JXTA_DECLARE(JString *) RendezVousPropagateMessage_get_DestSName(RendezVousPropagateMessage * ad)
{
    if (NULL != ad->destSName) {
        return JXTA_OBJECT_SHARE(ad->destSName);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) RendezVousPropagateMessage_set_DestSName(RendezVousPropagateMessage * ad, const char *dest)
{
    if (ad->destSName != NULL) {
        JXTA_OBJECT_RELEASE(ad->destSName);
        ad->destSName = NULL;
    }

    if (dest != NULL) {
        ad->destSName = jstring_new_2(dest);
    }
}

JXTA_DECLARE(JString *) RendezVousPropagateMessage_get_DestSParam(RendezVousPropagateMessage * ad)
{
    if (NULL != ad->destSParam) {
        return JXTA_OBJECT_SHARE(ad->destSParam);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) RendezVousPropagateMessage_set_DestSParam(RendezVousPropagateMessage * ad, const char *dest)
{
    if (ad->destSParam != NULL) {
        JXTA_OBJECT_RELEASE(ad->destSParam);
        ad->destSParam = NULL;
    }

    if (dest != NULL) {
        ad->destSParam = jstring_new_2(dest);
    }
}

JXTA_DECLARE(JString *) RendezVousPropagateMessage_get_MessageId(RendezVousPropagateMessage * ad)
{
    if (NULL != ad->messageId) {
        return JXTA_OBJECT_SHARE(ad->messageId);
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) RendezVousPropagateMessage_set_MessageId(RendezVousPropagateMessage * ad, const char *messageId)
{
    if (ad->messageId != NULL) {
        JXTA_OBJECT_RELEASE(ad->messageId);
        ad->messageId = NULL;
    }

    if (messageId != NULL) {
        ad->messageId = jstring_new_2(messageId);
    }
}

JXTA_DECLARE(Jxta_vector *) RendezVousPropagateMessage_get_Path(RendezVousPropagateMessage * ad)
{
    if (ad->path != NULL) {
        JXTA_OBJECT_SHARE(ad->path);
    }

    return ad->path;
}

JXTA_DECLARE(void) RendezVousPropagateMessage_set_Path(RendezVousPropagateMessage * ad, Jxta_vector * vector)
{
    if (ad->path != NULL) {
        JXTA_OBJECT_RELEASE(ad->path);
        ad->path = NULL;
    }

    if (vector != NULL) {
        ad->path = JXTA_OBJECT_SHARE(vector);
    }
}

JXTA_DECLARE(int) RendezVousPropagateMessage_get_TTL(RendezVousPropagateMessage * ad)
{
    return ad->ttl;
}

JXTA_DECLARE(void) RendezVousPropagateMessage_set_TTL(RendezVousPropagateMessage * ad, int ttl)
{
    ad->ttl = ttl;
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab RendezVousPropagateMessage_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RendezVousPropagateMessage", RendezVousPropagateMessage_, *handleRendezVousPropagateMessage, NULL, NULL},
    {"DestSName", DestSName_, *handleDestSName, NULL, NULL},
    {"DestSParam", DestSParam_, *handleDestSParam, NULL, NULL},
    {"MessageId", MessageId_, *handleMessageId, NULL, NULL},
    {"Path", Path_, *handlePath, NULL, NULL},
    {"TTL", TTL_, *handleTTL, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

static Jxta_status validate_message(RendezVousPropagateMessage * myself)
{
    if(myself->destSName == NULL)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "destSName is null [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if(myself->destSParam == NULL)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "destSParam is null [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }
    if(myself->messageId == NULL)
    {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "messageId is null [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) RendezVousPropagateMessage_get_xml(RendezVousPropagateMessage * ad, JString ** xml)
{
    JString *string;
    Jxta_status res = JXTA_SUCCESS;
    char buf[18];               /* We use this buffer to store a string representation of a int < 10 */
    unsigned int i = 0;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:RendezVousPropagateMessage>");

    jstring_append_2(string, "<jxta:RendezVousPropagateMessage>\n");
    jstring_append_2(string, "<DestSName>");
    jstring_append_2(string, jstring_get_string(ad->destSName));
    jstring_append_2(string, "</DestSName>\n");
    jstring_append_2(string, "<DestSParam>");
    jstring_append_2(string, jstring_get_string(ad->destSParam));
    jstring_append_2(string, "</DestSParam>\n");
    jstring_append_2(string, "<MessageId>");
    jstring_append_2(string, jstring_get_string(ad->messageId));
    jstring_append_2(string, "</MessageId>\n");
    jstring_append_2(string, "<TTL>");
    apr_snprintf(buf, sizeof(buf), "%d", RendezVousPropagateMessage_get_TTL(ad));
    jstring_append_2(string, buf);
    jstring_append_2(string, "</TTL>\n");

    for (i = 0; i < jxta_vector_size(ad->path); ++i) {
        JString *path;
        Jxta_status res;

        res = jxta_vector_get_object_at(ad->path, JXTA_OBJECT_PPTR(&path), i);
        if ((res == JXTA_SUCCESS) && (path != NULL)) {
            jstring_append_2(string, "<Path>");
            jstring_append_2(string, jstring_get_string(path));
            jstring_append_2(string, "</Path>\n");
            JXTA_OBJECT_RELEASE(path);
        } else {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to get path idx %d from vector\n", i);
        }
    }
    jstring_append_2(string, "</jxta:RendezVousPropagateMessage>\n");
    *xml = string;
    return JXTA_SUCCESS;
}


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
JXTA_DECLARE(RendezVousPropagateMessage *) RendezVousPropagateMessage_new(void)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) calloc(1, sizeof(RendezVousPropagateMessage));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:RendezVousPropagateMessage",
                                  RendezVousPropagateMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) RendezVousPropagateMessage_get_xml,
                                  NULL, NULL, (FreeFunc) RendezVousPropagateMessage_delete);

    ad->destSName = NULL;
    ad->destSParam = NULL;
    ad->messageId = NULL;
    ad->path = jxta_vector_new(3);
    ad->ttl = 0;
    return ad;
}

static void RendezVousPropagateMessage_delete(Jxta_object * obj)
{
    RendezVousPropagateMessage *ad = (RendezVousPropagateMessage *) obj;

    /* Fill in the required freeing functions here. */
    if (ad->destSName) {
        JXTA_OBJECT_RELEASE(ad->destSName);
        ad->destSName = NULL;
    }

    if (ad->destSParam) {
        JXTA_OBJECT_RELEASE(ad->destSParam);
        ad->destSParam = NULL;
    }

    if (ad->messageId) {
        JXTA_OBJECT_RELEASE(ad->messageId);
        ad->messageId = NULL;
    }

    if (ad->path) {
        JXTA_OBJECT_RELEASE(ad->path);
        ad->path = NULL;
    }

    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(RendezVousPropagateMessage));
    free(ad);
}

JXTA_DECLARE(Jxta_status) RendezVousPropagateMessage_parse_charbuffer(RendezVousPropagateMessage * ad, const char *buf,
                                                                      size_t len)
{
    Jxta_status rv =  jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

JXTA_DECLARE(Jxta_status) RendezVousPropagateMessage_parse_file(RendezVousPropagateMessage * ad, FILE * stream)
{
    Jxta_status rv =jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
    if(rv == JXTA_SUCCESS) {
        rv = validate_message(ad);
    }
    return rv;
}

/* vim: set ts=4 sw=4 tw=130 et: */
