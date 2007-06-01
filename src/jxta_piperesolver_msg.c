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
 * $Id: jxta_piperesolver_msg.c,v 1.16 2005/08/03 05:51:17 slowhog Exp $
 */


/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c DiscoveryResponse.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

static const char *__log_cat = "PIPE_RESOLVER_MSG";

#include <stdio.h>
#include <string.h>
#include "jxta_piperesolver_msg.h"
#include "jxta_errno.h"
#include "jxta_xml_util.h"
#include "jstring.h"
#include "jxta_log.h"

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
    PipeResolver_,
    MsgType_,
    Type_,
    PipeId_,
    Peer_,
    PeerAdv_,
    Cached_,
    Found_
};


    /** This is the representation of the
     * actual ad in the code.  It should
     * stay opaque to the programmer, and be 
     * accessed through the get/set API.
     */
struct _jxta_piperesolver_msg {
    Jxta_advertisement jxta_advertisement;
    char *PipeResolver;
    char *MsgType;
    char *Type;
    char *PipeId;
    char *Peer;
    JString *PeerAdv;
    Jxta_boolean Cached;
    Jxta_boolean Found;
};


void jxta_piperesolver_msg_delete(Jxta_piperesolver_msg *);

    /** Handler functions.  Each of these is responsible for
     * dealing with all of the character data associated with the 
     * tag name.
     */
static void handlePipeResolver(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_piperesolver_msg* ad = (Jxta_piperesolver_msg*)userdata; */
}

static void handleMsgType(void *userdata, const XML_Char * cd, int len)
{
    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->MsgType = tok;
    } else {
        free(tok);
    }
}

static void handleType(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->Type = tok;
    } else {
        free(tok);
    }
}

static void handlePipeId(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->PipeId = tok;
    } else {
        free(tok);
    }
}

static void handleFound(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        if (strcmp(tok, "true") == 0) {
            ad->Found = TRUE;
        }
    }
    free(tok);
}

static void handleCached(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        if (strcmp(tok, "true") == 0) {
            ad->Cached = TRUE;
        }
    } else {
        free(tok);
    }
}

static void handlePeer(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    char *tok = (char *) malloc(len + 1);
    memset(tok, 0, len + 1);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        ad->Peer = tok;
    } else {
        free(tok);
    }
}

static void handlePeerAdv(void *userdata, const XML_Char * cd, int len)
{

    Jxta_piperesolver_msg *ad = (Jxta_piperesolver_msg *) userdata;

    jstring_append_0(ad->PeerAdv, (char *) cd, len);
}


    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */
JXTA_DECLARE(char *) jxta_piperesolver_msg_get_PipeResolver(Jxta_piperesolver_msg * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_piperesolver_msg_set_PipeResolver(Jxta_piperesolver_msg * ad, char *name)
{
}

JXTA_DECLARE(Jxta_id *) jxta_piperesolver_msg_get_pipeid(Jxta_piperesolver_msg * ad)
{
    Jxta_id *pipeid = NULL;
    JString *tmps = jstring_new_2(ad->PipeId);
    JXTA_OBJECT_SHARE(tmps);
    jxta_id_from_jstring(&pipeid, tmps);
    JXTA_OBJECT_SHARE(pipeid);
    JXTA_OBJECT_RELEASE(tmps);
    return pipeid;
}


JXTA_DECLARE(char *) jxta_piperesolver_msg_get_MsgType(Jxta_piperesolver_msg * ad)
{
    return ad->MsgType;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_MsgType(Jxta_piperesolver_msg * ad, char *val)
{
    if (ad->MsgType != NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    if (val != NULL) {
        ad->MsgType = (char *) malloc(strlen(val) + 1);
        strcpy(ad->MsgType, val);
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(char *) jxta_piperesolver_msg_get_Type(Jxta_piperesolver_msg * ad)
{
    return ad->Type;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_Type(Jxta_piperesolver_msg * ad, char *val)
{
    if (ad->Type != NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (val != NULL) {
        ad->Type = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Type, val);
    }
    return JXTA_SUCCESS;

}

JXTA_DECLARE(char *) jxta_piperesolver_msg_get_PipeId(Jxta_piperesolver_msg * ad)
{
    return ad->PipeId;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_PipeId(Jxta_piperesolver_msg * ad, char *val)
{
    if (ad->PipeId != NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (val != NULL) {
        ad->PipeId = (char *) malloc(strlen(val) + 1);
        strcpy(ad->PipeId, val);
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_piperesolver_msg_get_Found(Jxta_piperesolver_msg * ad)
{
    return ad->Found;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_Found(Jxta_piperesolver_msg * ad, Jxta_boolean val)
{
    ad->Found = val;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_boolean) jxta_piperesolver_msg_get_Cached(Jxta_piperesolver_msg * ad)
{
    return ad->Cached;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_Cached(Jxta_piperesolver_msg * ad, Jxta_boolean val)
{
    ad->Cached = val;
    return JXTA_SUCCESS;
}

JXTA_DECLARE(char *) jxta_piperesolver_msg_get_Peer(Jxta_piperesolver_msg * ad)
{
    return ad->Peer;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_Peer(Jxta_piperesolver_msg * ad, char *val)
{
    if (ad->Peer != NULL) {
        return JXTA_INVALID_ARGUMENT;
    }
    if (val != NULL) {
        ad->Peer = (char *) malloc(strlen(val) + 1);
        strcpy(ad->Peer, val);
    }
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jxta_piperesolver_msg_get_PeerAdv(Jxta_piperesolver_msg * ad, JString ** val)
{
    if (ad->PeerAdv != NULL) {
        jstring_trim(ad->PeerAdv);
        JXTA_OBJECT_SHARE(ad->PeerAdv);
    }
    *val = ad->PeerAdv;
}

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_set_PeerAdv(Jxta_piperesolver_msg * ad, JString * val)
{
    if (val != NULL) {
        JXTA_OBJECT_SHARE(val);
        if (NULL != ad->PeerAdv) {
            JXTA_OBJECT_RELEASE(ad->PeerAdv);
        }
        ad->PeerAdv = val;
    }
    return JXTA_SUCCESS;
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab PipeResolver_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:PipeResolver", PipeResolver_, *handlePipeResolver, NULL, NULL},
    {"MsgType", MsgType_, *handleMsgType, NULL, NULL},
    {"Type", Type_, *handleType, NULL, NULL},
    {"PipeId", PipeId_, *handlePipeId, NULL, NULL},
    {"Peer", Peer_, *handlePeer, NULL, NULL},
    {"PeerAdv", PeerAdv_, *handlePeerAdv, NULL, NULL},
    {"Cached", Cached_, *handleCached, NULL, NULL},
    {"Found", Found_, *handleFound, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status) jxta_piperesolver_msg_get_xml(Jxta_piperesolver_msg * ad, JString ** xml)
{
    JString *string = NULL;
    JString *tmp = NULL;
    Jxta_status rv = JXTA_SUCCESS;
    ;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:PipeResolver>");

    jstring_append_2(string, "<jxta:PipeResolver>\n");
    jstring_append_2(string, "<MsgType>");
    jstring_append_2(string, jxta_piperesolver_msg_get_MsgType(ad));
    jstring_append_2(string, "</MsgType>\n");
    jstring_append_2(string, "<Type>");
    jstring_append_2(string, jxta_piperesolver_msg_get_Type(ad));
    jstring_append_2(string, "</Type>\n");
    jstring_append_2(string, "<PipeId>");
    jstring_append_2(string, jxta_piperesolver_msg_get_PipeId(ad));
    jstring_append_2(string, "</PipeId>\n");
    if (jxta_piperesolver_msg_get_Peer(ad) != NULL) {
        jstring_append_2(string, "<Peer>");
        jstring_append_2(string, jxta_piperesolver_msg_get_Peer(ad));
        jstring_append_2(string, "</Peer>\n");
    }

    if (NULL != ad->PeerAdv) {
        rv = jxta_xml_util_encode_jstring(ad->PeerAdv, &tmp);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "error encoding the PeerAdv, retrun status :%d\n", rv);
            JXTA_OBJECT_RELEASE(string);
            return rv;
        }

        jstring_append_2(string, "<PeerAdv>");
        jstring_append_1(string, tmp);
        jstring_append_2(string, "</PeerAdv>\n");
        JXTA_OBJECT_RELEASE(tmp);
    }

    jstring_append_2(string, "<Cached>");
    if (jxta_piperesolver_msg_get_Cached(ad)) {
        jstring_append_2(string, "true");
    } else {
        jstring_append_2(string, "false");
    }
    jstring_append_2(string, "</Cached>\n");


    jstring_append_2(string, "<Found>");
    if (jxta_piperesolver_msg_get_Found(ad)) {
        jstring_append_2(string, "true");
    } else {
        jstring_append_2(string, "false");
    }
    jstring_append_2(string, "</Found>\n");

    jstring_append_2(string, "</jxta:PipeResolver>\n");
    *xml = string;
    return JXTA_SUCCESS;
}


    /** Get a new instance of the ad.
     * The memory gets shredded going in to 
     * a value that is easy to see in a debugger,
     * just in case there is a segfault (not that 
     * that would ever happen, but in case it ever did.)
     */
JXTA_DECLARE(Jxta_piperesolver_msg *) jxta_piperesolver_msg_new(void)
{
    Jxta_piperesolver_msg *ad;
    ad = (Jxta_piperesolver_msg *) malloc(sizeof(Jxta_piperesolver_msg));
    memset(ad, 0x0, sizeof(Jxta_piperesolver_msg));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:PipeResolver",
                                  PipeResolver_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_piperesolver_msg_get_xml,
                                  NULL, (JxtaAdvertisementGetIndexFunc) NULL, (FreeFunc) jxta_piperesolver_msg_delete);

    ad->MsgType = NULL;
    ad->Type = NULL;
    ad->PipeId = NULL;
    ad->Peer = NULL;
    ad->PeerAdv = jstring_new_0();
    ad->Cached = FALSE;
    ad->Found = FALSE;

    return ad;
}

    /** Shred the memory going out.  Again,
     * if there ever was a segfault (unlikely,
     * of course), the hex value dddddddd will 
     * pop right out as a piece of memory accessed
     * after it was freed...
     */
void jxta_piperesolver_msg_delete(Jxta_piperesolver_msg * ad)
{
    /* Fill in the required freeing functions here. */
    jxta_advertisement_delete((Jxta_advertisement *) ad);

    if (ad->MsgType) {
        free(ad->MsgType);
        ad->MsgType = NULL;
    }

    if (ad->Type) {
        free(ad->Type);
        ad->Type = NULL;
    }

    if (ad->PipeId) {
        free(ad->PipeId);
        ad->PipeId = NULL;
    }

    if (ad->Peer) {
        free(ad->Peer);
        ad->Peer = NULL;
    }

    if (ad->PeerAdv) {
        JXTA_OBJECT_RELEASE(ad->PeerAdv);
        ad->PeerAdv = NULL;
    }

    memset(ad, 0x00, sizeof(Jxta_piperesolver_msg));
    free(ad);
}

JXTA_DECLARE(void) jxta_piperesolver_msg_parse_charbuffer(Jxta_piperesolver_msg * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}



JXTA_DECLARE(void) jxta_piperesolver_msg_parse_file(Jxta_piperesolver_msg * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_piperesolver_msg *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_piperesolver_msg_new();

    testfile = fopen(argv[1], "r");
    jxta_piperesolver_msg_parse_file(ad, testfile);
    fclose(testfile);

    /* jxta_piperesolver_msg_print_xml(ad,fprintf,stdout); */
    jxta_piperesolver_msg_delete(ad);

    return 0;
}
#endif
#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
