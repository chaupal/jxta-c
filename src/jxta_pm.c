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
 * $Id: jxta_pm.c,v 1.19 2005/04/06 00:38:52 bondolo Exp $
 */

#include <stdio.h>
#include <string.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_pm.h"
#include "jxta_xml_util.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#endif

    /** Each of these corresponds to a tag in the
     * xml ad.
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
        char        * RendezVousPropagateMessage;
        char        * DestSName;
        char        * DestSParam;
        char        * MessageId;
        Jxta_vector * Path;
        int           TTL;
    };


    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
    static void
    handleRendezVousPropagateMessage(void * userdata, const XML_Char * cd, int len) {
        /* RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata; */
    }

    static void
    handleDestSName(void * userdata, const XML_Char * cd, int len) {
        RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata;

        char* tok = malloc (len + 1);
        memset (tok, 0, len + 1);

        extract_char_data(cd,len,tok);

        if (strlen (tok) != 0) {
            ad->DestSName = tok;
        } else {
            free (tok);
        }
    }

    static void
    handleDestSParam(void * userdata, const XML_Char * cd, int len) {

        RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata;

        char* tok = malloc (len + 1);
        memset (tok, 0, len + 1);

        extract_char_data(cd,len,tok);

        if (strlen (tok) != 0) {
            ad->DestSParam = tok;
        } else {
            free (tok);
        }
    }

    static void
    handleMessageId(void * userdata, const XML_Char * cd, int len) {

        RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata;

        char* tok = malloc (len + 1);
        memset (tok, 0, len + 1);

        extract_char_data(cd,len,tok);

        if (strlen (tok) != 0) {
            ad->MessageId = tok;
        } else {
            free (tok);
        }
    }

    static void
    handleTTL(void * userdata, const XML_Char * cd, int len) {

        RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata;

        char* tok = malloc (len + 1);
        memset (tok, 0, len + 1);

        extract_char_data(cd,len,tok);

        if (strlen (tok) != 0) {
            sscanf (tok, "%d", &ad->TTL);
        } else {
            free (tok);
        }
    }

    static void
    handlePath(void * userdata, const XML_Char * cd, int len) {

        RendezVousPropagateMessage * ad = (RendezVousPropagateMessage*)userdata;
        char* tok;

        if (len > 0) {

            tok = malloc (len + 1);
            memset (tok, 0, len + 1);

            extract_char_data(cd,len,tok);

            if (strlen (tok) != 0) {
                JString* pt = jstring_new_2 (tok);
                JXTA_LOG("Path: [%s]\n", tok);
                jxta_vector_add_object_last (ad->Path, (Jxta_object*) pt);
                /* The vector shares automatically the object. We must release */
                JXTA_OBJECT_RELEASE (pt);
            }
            free (tok);
        }

    }


    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */
    const char *
    RendezVousPropagateMessage_get_RendezVousPropagateMessage(RendezVousPropagateMessage * ad) {
        return NULL;
    }

    void
    RendezVousPropagateMessage_set_RendezVousPropagateMessage(RendezVousPropagateMessage * ad, const char * name) {}

    const char *
    RendezVousPropagateMessage_get_DestSName(RendezVousPropagateMessage * ad) {
        return ad->DestSName;
    }

    void
    RendezVousPropagateMessage_set_DestSName(RendezVousPropagateMessage * ad, const char* src) {

        if (ad->DestSName != NULL) {
            free (ad->DestSName);
            ad->DestSName = NULL;
        }
        if (src != NULL) {
            ad->DestSName = malloc (strlen (src) + 1);
            strcpy (ad->DestSName, src);
        } else {
            ad->DestSName = NULL;
        }
    }

    const char *
    RendezVousPropagateMessage_get_DestSParam(RendezVousPropagateMessage * ad) {
        return ad->DestSParam;
    }

    void
    RendezVousPropagateMessage_set_DestSParam(RendezVousPropagateMessage * ad, const char * dest) {

        if (ad->DestSParam != NULL) {
            free (ad->DestSParam);
        }
        if (dest != NULL) {
            ad->DestSParam = malloc (strlen (dest) + 1);
            strcpy (ad->DestSParam, dest);
        }
    }

    const char *
    RendezVousPropagateMessage_get_MessageId(RendezVousPropagateMessage * ad) {
        return ad->MessageId;
    }

    void
    RendezVousPropagateMessage_set_MessageId(RendezVousPropagateMessage * ad, const char* messageId) {

        if (ad->MessageId != NULL) {
            free (ad->MessageId);
            ad->MessageId = NULL;
        }
        if (messageId != NULL) {
            ad->MessageId = malloc (strlen (messageId) + 1);
            strcpy (ad->MessageId, messageId);
        }

    }

    Jxta_vector*
    RendezVousPropagateMessage_get_Path(RendezVousPropagateMessage * ad) {

        if (ad->Path != NULL) {
            JXTA_OBJECT_SHARE (ad->Path);
            return ad->Path;
        } else {
            return NULL;
        }
    }

    void
    RendezVousPropagateMessage_set_Path(RendezVousPropagateMessage * ad, Jxta_vector* vector) {

        if (ad->Path != NULL) {
            JXTA_OBJECT_RELEASE (ad->Path);
            ad->Path = NULL;
        }
        if (vector != NULL) {
            JXTA_OBJECT_SHARE (vector);
            ad->Path = vector;
        }
        return;
    }

    int
    RendezVousPropagateMessage_get_TTL(RendezVousPropagateMessage * ad) {
        return ad->TTL;
    }

    void
    RendezVousPropagateMessage_set_TTL(RendezVousPropagateMessage * ad, int TTL) {

        ad->TTL = TTL;
    }

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
    const static Kwdtab RendezVousPropagateMessage_tags[] = {
                {"Null",                           Null_,                       NULL,                            NULL
                },
                {"jxta:RendezVousPropagateMessage",RendezVousPropagateMessage_,*handleRendezVousPropagateMessage,NULL},
                {"DestSName",                      DestSName_,                 *handleDestSName,                 NULL},
                {"DestSParam",                     DestSParam_,                *handleDestSParam,                NULL},
                {"MessageId",                      MessageId_,                 *handleMessageId,                 NULL},
                {"Path",                           Path_,                      *handlePath,                      NULL},
                {"TTL",                            TTL_,                       *handleTTL,                       NULL},
                {NULL,                             0,                           0,                               NULL}
            };


    Jxta_status RendezVousPropagateMessage_get_xml(RendezVousPropagateMessage * ad,
            JString** xml) {
        JString* string;
        char buf[10]; /* We use this buffer to store a string representation of a int < 10 */
        int i = 0;

        if (xml == NULL) {
            return JXTA_INVALID_ARGUMENT;
        }

        string = jstring_new_0();

        jstring_append_2 (string, "<?xml version=\"1.0\"?>\n");
        jstring_append_2 (string, "<!DOCTYPE jxta:RendezVousPropagateMessage>");

        jstring_append_2 (string,"<jxta:RendezVousPropagateMessage>\n");
        jstring_append_2 (string,"<DestSName>");
        jstring_append_2 (string,RendezVousPropagateMessage_get_DestSName(ad));
        jstring_append_2 (string,"</DestSName>\n");
        jstring_append_2 (string,"<DestSParam>");
        jstring_append_2 (string,RendezVousPropagateMessage_get_DestSParam(ad));
        jstring_append_2 (string,"</DestSParam>\n");
        jstring_append_2 (string,"<MessageId>");
        jstring_append_2 (string,RendezVousPropagateMessage_get_MessageId(ad));
        jstring_append_2 (string,"</MessageId>\n");
        jstring_append_2 (string,"<TTL>");
        sprintf (buf, "%d", RendezVousPropagateMessage_get_TTL(ad));
        jstring_append_2 (string, buf);
        jstring_append_2 (string,"</TTL>\n");

        for (i = 0; i < jxta_vector_size (ad->Path); ++i) {
            JString* path;
            Jxta_status res;

            res = jxta_vector_get_object_at (ad->Path, (Jxta_object**) &path, i);
            if ((res == JXTA_SUCCESS) && (path != NULL)) {
                jstring_append_2 (string,"<Path>");
                jstring_append_2 (string, jstring_get_string (path));
                jstring_append_2 (string,"</Path>\n");
                JXTA_OBJECT_RELEASE (path);
            } else {
                JXTA_LOG ("Failed to get Path from vector\n");
            }
        }
        jstring_append_2 (string,"</jxta:RendezVousPropagateMessage>\n");
        *xml = string;
        return JXTA_SUCCESS;
    }


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
    RendezVousPropagateMessage *
    RendezVousPropagateMessage_new(void) {

        RendezVousPropagateMessage * ad;
        ad = (RendezVousPropagateMessage *) malloc (sizeof (RendezVousPropagateMessage));
        memset (ad, 0x0, sizeof (RendezVousPropagateMessage));

        jxta_advertisement_initialize((Jxta_advertisement*)ad,
                                      "jxta:RendezVousPropagateMessage",
                                      RendezVousPropagateMessage_tags,
                                      (JxtaAdvertisementGetXMLFunc)RendezVousPropagateMessage_get_xml,
                                      NULL,NULL,
                                      (FreeFunc)RendezVousPropagateMessage_delete);

        ad->DestSName  = NULL;
        ad->DestSParam = NULL;
        ad->MessageId  = NULL;
        ad->Path       = jxta_vector_new (7);
        ad->TTL        = 0;
        return ad;
    }

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
    void
    RendezVousPropagateMessage_delete (RendezVousPropagateMessage * ad) {
        /* Fill in the required freeing functions here. */
        if (ad->DestSName) {
            free (ad->DestSName);
            ad->DestSName = NULL;
        }

        if (ad->DestSParam) {
            free (ad->DestSParam);
            ad->DestSParam = NULL;
        }

        if (ad->MessageId) {
            free (ad->MessageId);
            ad->MessageId = NULL;
        }

        if (ad->Path) {
            JXTA_OBJECT_RELEASE (ad->Path);
            ad->Path = NULL;
        }

        jxta_advertisement_delete((Jxta_advertisement*)ad);
        memset (ad, 0xdd, sizeof (RendezVousPropagateMessage));
        free (ad);
    }

    void
    RendezVousPropagateMessage_parse_charbuffer(RendezVousPropagateMessage * ad, const char * buf, int len) {

        jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
    }



    void
    RendezVousPropagateMessage_parse_file(RendezVousPropagateMessage * ad, FILE * stream) {

        jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
    }

#ifdef __cplusplus
}
#endif
