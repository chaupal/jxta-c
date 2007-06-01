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
 * $Id: jxta_rm.c,v 1.37 2005/09/13 21:55:10 slowhog Exp $
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

#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_rm.h"
#include "jxta_xml_util.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

/** Each of these corresponds to a tag in the
 * xml ad.
 */
enum tokentype {
    Null_,
    EndpointRouterMessage_,
    Src_,
    Dest_,
    Last_,
    GatewayForward_,
    GatewayReverse_
};

/** This is the representation of the
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _EndpointRouterMessage {
    Jxta_advertisement jxta_advertisement;
    char *EndpointRouterMessage;
    Jxta_endpoint_address *Src;
    Jxta_endpoint_address *Dest;
    Jxta_endpoint_address *Last;
    Jxta_vector *reverseGateways;
    Jxta_vector *forwardGateways;
};


/** Handler functions.  Each of these is responsible for
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleEndpointRouterMessage(void *userdata, const XML_Char * cd, int len)
{
    /* EndpointRouterMessage * ad = (EndpointRouterMessage*)userdata; */
}

static void handleSrc(void *userdata, const XML_Char * cd, int len)
{
    EndpointRouterMessage *ad = (EndpointRouterMessage *) userdata;

    char *tok = (char *) malloc(256);
    memset(tok, 0, 256);

    extract_char_data(cd, len, tok);

    if (strlen(tok) != 0) {
        if (ad->Src != NULL) {
            JXTA_OBJECT_RELEASE(ad->Src);
        }
        JXTA_LOG("Src: [%s]\n", tok);
        ad->Src = jxta_endpoint_address_new(tok);
    }
    free(tok);
}

static void handleDest(void *userdata, const XML_Char * cd, int len)
{
    EndpointRouterMessage *ad = (EndpointRouterMessage *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);
        if (strlen(tok) != 0) {
            if (ad->Dest != NULL) {
                JXTA_OBJECT_RELEASE(ad->Dest);
            }
            JXTA_LOG("Dest: [%s]\n", tok);
            ad->Dest = jxta_endpoint_address_new(tok);
        }
        free(tok);
    }
}

static void handleLast(void *userdata, const XML_Char * cd, int len)
{
    EndpointRouterMessage *ad = (EndpointRouterMessage *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            if (ad->Last != NULL) {
                JXTA_OBJECT_RELEASE(ad->Last);
            }
            JXTA_LOG("Last: [%s]\n", tok);
            ad->Last = jxta_endpoint_address_new(tok);
        }
        free(tok);
    }
}

static void handleGatewayForward(void *userdata, const XML_Char * cd, int len)
{
    EndpointRouterMessage *ad = (EndpointRouterMessage *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            Jxta_endpoint_address *addr = jxta_endpoint_address_new(tok);
            JXTA_LOG("Forward Gateway: [%s]\n", tok);
            if (addr != NULL) {
                jxta_vector_add_object_last(ad->forwardGateways, (Jxta_object *) addr);
                /* The vector shares automatically the object. We must release */
                JXTA_OBJECT_RELEASE(addr);
            }
        }
        free(tok);
    }
}

static void handleGatewayReverse(void *userdata, const XML_Char * cd, int len)
{
    EndpointRouterMessage *ad = (EndpointRouterMessage *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);

        extract_char_data(cd, len, tok);

        if (strlen(tok) != 0) {
            Jxta_endpoint_address *addr = jxta_endpoint_address_new(tok);
            JXTA_LOG("Reverse Gateway: [%s]\n", tok);
            if (addr != NULL) {
                jxta_vector_add_object_last(ad->reverseGateways, (Jxta_object *) addr);
                /* The vector shares automatically the object. We must release */
                JXTA_OBJECT_RELEASE(addr);
            }
        }
        free(tok);
    }
}



    /** The get/set functions represent the public
     * interface to the ad class, that is, the API.
     */
JXTA_DECLARE(char *) EndpointRouterMessage_get_EndpointRouterMessage(EndpointRouterMessage * ad)
{
    return NULL;
}

JXTA_DECLARE(void) EndpointRouterMessage_set_EndpointRouterMessage(EndpointRouterMessage * ad, char *name)
{
}

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Src(EndpointRouterMessage * ad)
{
    if (ad->Src != NULL) {
        JXTA_OBJECT_SHARE(ad->Src);
    }
    return ad->Src;
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Src(EndpointRouterMessage * ad, Jxta_endpoint_address * src)
{
    if (ad == NULL) {
        return;
    }
    if (ad->Src != NULL) {
        JXTA_OBJECT_RELEASE(ad->Src);
        ad->Src = NULL;
    }
    if (src != NULL) {
        JXTA_OBJECT_SHARE(src);
        ad->Src = src;
    }
}

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Dest(EndpointRouterMessage * ad)
{
    if (ad->Dest != NULL) {
        JXTA_OBJECT_SHARE(ad->Dest);
    }
    return ad->Dest;
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Dest(EndpointRouterMessage * ad, Jxta_endpoint_address * dest)
{
    if (ad == NULL) {
        return;
    }
    if (ad->Dest != NULL) {
        JXTA_OBJECT_RELEASE(ad->Dest);
        ad->Dest = NULL;
    }
    if (dest != NULL) {
        JXTA_OBJECT_SHARE(dest);
        ad->Dest = dest;
    }
}

JXTA_DECLARE(Jxta_endpoint_address *) EndpointRouterMessage_get_Last(EndpointRouterMessage * ad)
{
    if (ad->Last != NULL) {
        JXTA_OBJECT_SHARE(ad->Last);
    }
    return ad->Last;
}

JXTA_DECLARE(void) EndpointRouterMessage_set_Last(EndpointRouterMessage * ad, Jxta_endpoint_address * last)
{
    if (ad == NULL) {
        return;
    }
    if (ad->Last != NULL) {
        JXTA_OBJECT_RELEASE(ad->Last);
        ad->Last = NULL;
    }
    if (last != NULL) {
        JXTA_OBJECT_SHARE(last);
        ad->Last = last;
    }
}

JXTA_DECLARE(Jxta_vector *) EndpointRouterMessage_get_GatewayForward(EndpointRouterMessage * ad)
{
    if (ad->forwardGateways != NULL) {
        JXTA_OBJECT_SHARE(ad->forwardGateways);
        return ad->forwardGateways;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_GatewayForward(EndpointRouterMessage * ad, Jxta_vector * vector)
{
    if (ad->forwardGateways != NULL) {
        JXTA_OBJECT_RELEASE(ad->forwardGateways);
        ad->forwardGateways = NULL;
    }
    if (vector != NULL) {
        JXTA_OBJECT_SHARE(vector);
        ad->forwardGateways = vector;
    }
    return;
}

JXTA_DECLARE(Jxta_vector *) EndpointRouterMessage_get_GatewayReverse(EndpointRouterMessage * ad)
{
    if (ad->reverseGateways != NULL) {
        JXTA_OBJECT_SHARE(ad->reverseGateways);
        return ad->reverseGateways;
    } else {
        return NULL;
    }
}

JXTA_DECLARE(void) EndpointRouterMessage_set_GatewayReverse(EndpointRouterMessage * ad, Jxta_vector * vector)
{
    if (ad->reverseGateways != NULL) {
        JXTA_OBJECT_RELEASE(ad->reverseGateways);
        ad->reverseGateways = NULL;
    }
    if (vector != NULL) {
        JXTA_OBJECT_SHARE(vector);
        ad->reverseGateways = vector;
    }
    return;
}

    /** Now, build an array of the keyword structs.  Since
     * a top-level, or null state may be of interest, 
     * let that lead off.  Then, walk through the enums,
     * initializing the struct array with the correct fields.
     * Later, the stream will be dispatched to the handler based
     * on the value in the char * kwd.
     */
static const Kwdtab EndpointRouterMessage_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ERM", EndpointRouterMessage_, *handleEndpointRouterMessage, NULL, NULL},
    {"Src", Src_, *handleSrc, NULL, NULL},
    {"Dest", Dest_, *handleDest, NULL, NULL},
    {"Last", Last_, *handleLast, NULL, NULL},
    {"Fwd", GatewayForward_, *handleGatewayForward, NULL, NULL},
    {"Rvs", GatewayReverse_, *handleGatewayReverse, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};


JXTA_DECLARE(Jxta_status) EndpointRouterMessage_get_xml(EndpointRouterMessage * ad, JString ** xml)
{
    JString *string;
    int i = 0;
    int size_fwd = 0;
    int size_rvs = 0;
    char *pt;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<?xml version=\"1.0\"?>\n");
    jstring_append_2(string, "<!DOCTYPE jxta:ERM>");

    jstring_append_2(string, "<jxta:ERM>\n");
    jstring_append_2(string, "<Src>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Src)));
    free(pt);
    jstring_append_2(string, "</Src>\n");
    jstring_append_2(string, "<Dest>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Dest)));
    free(pt);
    jstring_append_2(string, "</Dest>\n");
    jstring_append_2(string, "<Last>");
    jstring_append_2(string, (pt = jxta_endpoint_address_to_string(ad->Last)));
    free(pt);
    jstring_append_2(string, "</Last>\n");

    size_fwd = jxta_vector_size(ad->forwardGateways);
    if (size_fwd > 0) {
        jstring_append_2(string, "<Fwd>\n");
        for (i = 0; i < size_fwd; ++i) {
            Jxta_endpoint_address *addr = NULL;
            Jxta_status res;

            res = jxta_vector_get_object_at(ad->forwardGateways, (Jxta_object **) & addr, i);
            if ((res == JXTA_SUCCESS) && (addr != NULL)) {
                char *pt = jxta_endpoint_address_to_string(addr);
                if (pt != NULL) {
                    jstring_append_2(string, "<GatewayForward>");
                    jstring_append_2(string, pt);
                    jstring_append_2(string, "</GatewayForward>\n");
                    free(pt);
                }
                JXTA_OBJECT_RELEASE(addr);
            } else {
                JXTA_LOG("Failed to get address from forward vector\n");
            }
        }
        jstring_append_2(string, "</Fwd>\n");
    } else {
        jstring_append_2(string, "<Fwd/>\n");
    }

    size_rvs = jxta_vector_size(ad->reverseGateways);
    if (size_rvs > 0) {
        jstring_append_2(string, "<Rvs>\n");
        for (i = 0; i < size_rvs; ++i) {
            Jxta_endpoint_address *addr = NULL;
            Jxta_status res;

            res = jxta_vector_get_object_at(ad->reverseGateways, (Jxta_object **) & addr, i);
            if ((res == JXTA_SUCCESS) && (addr != NULL)) {
                char *pt = jxta_endpoint_address_to_string(addr);
                if (pt != NULL) {
                    jstring_append_2(string, "<GatewayReverse>");
                    jstring_append_2(string, pt);
                    jstring_append_2(string, "</GatewayReverse>\n");
                    free(pt);
                }
                JXTA_OBJECT_RELEASE(addr);
            } else {
                JXTA_LOG("Failed to get address from reverseGateways\n");
            }
        }
        jstring_append_2(string, "</Rvs>\n");
    } else {
        jstring_append_2(string, "<Rvs/>\n");
    }
    jstring_append_2(string, "</jxta:ERM>\n");

    *xml = string;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(EndpointRouterMessage *) EndpointRouterMessage_new(void)
{
    EndpointRouterMessage *ad;
    ad = (EndpointRouterMessage *) malloc(sizeof(EndpointRouterMessage));
    memset(ad, 0x0, sizeof(EndpointRouterMessage));


    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:ERM",
                                  EndpointRouterMessage_tags,
                                  (JxtaAdvertisementGetXMLFunc) EndpointRouterMessage_get_xml,
                                  NULL, NULL, (FreeFunc) EndpointRouterMessage_delete);

    ad->reverseGateways = jxta_vector_new(1);
    ad->forwardGateways = jxta_vector_new(1);

    return ad;
}


void EndpointRouterMessage_delete(EndpointRouterMessage * ad)
{
    if (ad->Src) {
        JXTA_OBJECT_RELEASE(ad->Src);
    }

    if (ad->Dest) {
        JXTA_OBJECT_RELEASE(ad->Dest);
    }

    if (ad->Last) {
        JXTA_OBJECT_RELEASE(ad->Last);
    }

    if (ad->reverseGateways) {
        JXTA_OBJECT_RELEASE(ad->reverseGateways);
    }

    if (ad->forwardGateways) {
        JXTA_OBJECT_RELEASE(ad->forwardGateways);
    }

    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0x00, sizeof(EndpointRouterMessage));
    free(ad);
}

JXTA_DECLARE(void) EndpointRouterMessage_parse_charbuffer(EndpointRouterMessage * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) EndpointRouterMessage_parse_file(EndpointRouterMessage * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    EndpointRouterMessage *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = EndpointRouterMessage_new();

    testfile = fopen(argv[1], "r");
    EndpointRouterMessage_parse_file(ad, testfile);
    fclose(testfile);

    /* EndpointRouterMessage_print_xml(ad,fprintf,stdout); */
    EndpointRouterMessage_delete(ad);

    return 0;
}
#endif

#if 0
{
#endif
#ifdef __cplusplus
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
