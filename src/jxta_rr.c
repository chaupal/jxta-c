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
 * $Id: jxta_rr.c,v 1.48 2005/11/22 22:00:58 mmx2005 Exp $
 */



/*
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c ResolverResponse.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_object.h"
#include "jxta_rr.h"
#include "jxta_xml_util.h"
#include "jxta_advertisement.h"
#include "jxta_apr.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Each of these corresponds to a tag in the
* xml ad.
*/
enum tokentype {
    Null_,
    ResolverResponse_,
    Credential_,
    ResPeerID_,
    HandlerName_,
    QueryID_,
    Response_
};

/** This is the representation of the
* actual ad in the code.  It should
* stay opaque to the programmer, and be 
* accessed through the get/set API.
*/
struct _ResolverResponse {
    Jxta_advertisement jxta_advertisement;
    char *ResolverResponse;
    JString *Credential;
    Jxta_id *ResPeerID;
    JString *HandlerName;
    long QueryID;
    JString *Response;
};

/** Handler functions.  Each of these is responsible for
* dealing with all of the character data associated with the 
* tag name.
*/
static void handleResolverResponse(void *userdata, const XML_Char * cd, int len) 
{
    /* ResolverResponse * ad = (ResolverResponse*)userdata; */
} 

static void handleCredential(void *userdata, const XML_Char * cd, int len) 
{
    ResolverResponse *ad = (ResolverResponse *) userdata;
    
    jstring_append_0(ad->Credential, cd, len);
}

static void handleHandlerName(void *userdata, const XML_Char * cd, int len) 
{
    ResolverResponse *ad = (ResolverResponse *) userdata;

    jstring_append_0(ad->HandlerName, cd, len);
}

static void handleResPeerID(void *userdata, const XML_Char * cd, int len)
{
    ResolverResponse *ad = (ResolverResponse *) userdata;
    char *tok;

    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        /* XXXX hamada@jxta.org it is possible to get called many times, some of which are
         * calls on white-space, we only care for the id therefore
         * we create the id only when we have data, this not an elagant way
         * of dealing with this.
         */
        if (*tok != '\0') {
            JString *tmps = jstring_new_2(tok);
            /* hamada@jxta.org it would be desireable to have a fucntion that
               takes a "char *" we would not need to create a JString to create 
               the ID
             */
            ad->ResPeerID = NULL;
            jxta_id_from_jstring(&ad->ResPeerID, tmps);
            JXTA_OBJECT_RELEASE(tmps);
        }
        free(tok);
    }
}

static void handleQueryID(void *userdata, const XML_Char * cd, int len) 
{
    ResolverResponse *ad = (ResolverResponse *) userdata;
    char *tok;
    
    /* XXXX hamada@jxta.org this can be cleaned up once parsing is corrected */
    if (len > 0) {
        tok = (char *) malloc(len + 1);
        memset(tok, 0, len + 1);
        extract_char_data(cd, len, tok);
        if (*tok != '\0') {
            ad->QueryID = (int) strtol(cd, NULL, 0);
        }
        free(tok);
    }
}

static void handleResponse(void *userdata, const XML_Char * cd, int len) 
{
    ResolverResponse *ad = (ResolverResponse *) userdata;
    
    jstring_append_0(ad->Response, (char *) cd, len);
}

static void trim_elements(ResolverResponse * adv) 
{
    jstring_trim(adv->Credential);
    jstring_trim(adv->HandlerName);
    jstring_trim(adv->Response);
}

/** The get/set functions represent the public
    * interface to the ad class, that is, the API.
*/
    
JXTA_DECLARE(Jxta_status) jxta_resolver_response_get_xml(ResolverResponse * adv, JString ** document) 
{
    JString *doc = NULL;
    JString *tmps = NULL;
    Jxta_status status;
    char *buf;

    if (adv == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    JXTA_OBJECT_CHECK_VALID(adv);

    buf = (char *) calloc(1, 128);
    trim_elements(adv);
    doc = jstring_new_2("<?xml version=\"1.0\"?>\n");
    jstring_append_2(doc, "<!DOCTYPE jxta:ResolverResponse>");
    jstring_append_2(doc, "<jxta:ResolverResponse>\n");

    if (jstring_length(adv->Credential) != 0) {
        jstring_append_2(doc, "<jxta:Cred>");
        jstring_append_1(doc, adv->Credential);
        jstring_append_2(doc, "</jxta:Cred>\n");
    }

	if (adv->ResPeerID != jxta_id_nullID) {
	  jstring_append_2(doc, "<ResPeerID>");
	  jxta_id_to_jstring(adv->ResPeerID, &tmps);
	  jstring_append_1(doc, tmps);
	  JXTA_OBJECT_RELEASE(tmps);
	  jstring_append_2(doc, "</ResPeerID>\n");
	}
	  
	jstring_append_2(doc, "<HandlerName>");
    jstring_append_1(doc, adv->HandlerName);
    jstring_append_2(doc, "</HandlerName>\n");

    apr_snprintf(buf, 128, "%ld\n", adv->QueryID);
    jstring_append_2(doc, "<QueryID>");
    jstring_append_2(doc, buf);
    jstring_append_2(doc, "</QueryID>\n");
    
    /* done with buf, free it */
    free(buf);

    jstring_append_2(doc, "<Response>");
    status = jxta_xml_util_encode_jstring(adv->Response, &tmps);
    if (status != JXTA_SUCCESS) {
        JXTA_LOG("error encoding the response, retrun status :%d\n", status);
        JXTA_OBJECT_RELEASE(doc);
        return status;
    }

    jstring_append_1(doc, tmps);
    JXTA_OBJECT_RELEASE(tmps);
    jstring_append_2(doc, "</Response>\n");
    jstring_append_2(doc, "</jxta:ResolverResponse>\n");

    *document = doc;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(JString *) jxta_resolver_response_get_credential(ResolverResponse * ad) 
{
    if (ad->Credential) {
        jstring_trim(ad->Credential);
        JXTA_OBJECT_SHARE(ad->Credential);
    }
    
    return ad->Credential;
}

JXTA_DECLARE(void) jxta_resolver_response_set_credential(ResolverResponse * ad, JString * credential) 
{
    if (ad == NULL) {
        return;
    }
    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
        ad->Credential = NULL;
    }
    
    JXTA_OBJECT_SHARE(credential);
    ad->Credential = credential;
}

JXTA_DECLARE(Jxta_id *) jxta_resolver_response_get_res_peer_id(ResolverResponse * ad)
{
    if (ad->ResPeerID != jxta_id_nullID) {
        JXTA_OBJECT_SHARE(ad->ResPeerID);
    }
    return ad->ResPeerID;
}

JXTA_DECLARE(void) jxta_resolver_response_set_res_peer_id(ResolverResponse * ad, Jxta_id * id)
{
    if (ad == NULL) {
        return;
    }
    if (ad->ResPeerID != NULL ) {
        JXTA_OBJECT_RELEASE(ad->ResPeerID);
        ad->ResPeerID = NULL;
    }
    if (!jxta_id_equals(id, jxta_id_nullID)) 
      JXTA_OBJECT_SHARE(id);
    ad->ResPeerID = id;
}

JXTA_DECLARE(void) jxta_resolver_response_get_handlername(ResolverResponse * ad, JString ** str) 
{
    if (ad->HandlerName) {
        jstring_trim(ad->HandlerName);
        JXTA_OBJECT_SHARE(ad->HandlerName);
    }
        *str = ad->HandlerName;
}

JXTA_DECLARE(void) jxta_resolver_response_set_handlername(ResolverResponse * ad, JString * handlerName) 
{
    if (ad == NULL) {
        return;
    }
    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
        ad->HandlerName = NULL;
    }

    JXTA_OBJECT_SHARE(handlerName);
    ad->HandlerName = handlerName;
}

JXTA_DECLARE(long) jxta_resolver_response_get_queryid(ResolverResponse * ad) 
{
    return ad->QueryID;
}

JXTA_DECLARE(void) jxta_resolver_response_set_queryid(ResolverResponse * ad, long queryID) 
{
    ad->QueryID = queryID;
}

JXTA_DECLARE(void) jxta_resolver_response_get_response(ResolverResponse * ad, JString ** str) 
{
    if (ad->Response) {
        jstring_trim(ad->Response);
        JXTA_OBJECT_SHARE(ad->Response);
    }
    *str = ad->Response;
}

JXTA_DECLARE(void) jxta_resolver_response_set_response(ResolverResponse * ad, JString * response) 
{
    if (ad == NULL) {
        return;
    }
    if (ad->Response != NULL) {
        JXTA_OBJECT_RELEASE(ad->Response);
        ad->Response = NULL;
    }
    if (response != NULL) {
        JXTA_OBJECT_SHARE(response);
        ad->Response = response;
    }
}

/** Now, build an array of the keyword structs.  Since
* a top-level, or null state may be of interest, 
* let that lead off.  Then, walk through the enums,
* initializing the struct array with the correct fields.
* Later, the stream will be dispatched to the handler based
* on the value in the char * kwd.
*/
static const Kwdtab ResolverResponse_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:ResolverResponse", ResolverResponse_, *handleResolverResponse, NULL, NULL},
    {"jxta:Cred", Credential_, *handleCredential, NULL, NULL},
    {"ResPeerID", ResPeerID_, *handleResPeerID, NULL, NULL},
    {"HandlerName", HandlerName_, *handleHandlerName, NULL, NULL},
    {"QueryID", QueryID_, *handleQueryID, NULL, NULL},
    {"Response", Response_, *handleResponse, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/** Get a new instance of the ad.
* The memory gets shredded going in to 
* a value that is easy to see in a debugger,
* just in case there is a segfault (not that 
* that would ever happen, but in case it ever did.)
*/
JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new(void) 
{
    ResolverResponse *ad;
    
    ad = (ResolverResponse *) malloc(sizeof(ResolverResponse));
    memset(ad, 0xda, sizeof(ResolverResponse));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                "jxta:ResolverResponse",
                                ResolverResponse_tags,
                                (JxtaAdvertisementGetXMLFunc) jxta_resolver_response_get_xml,
                                NULL, 
                                NULL, 
                                (FreeFunc) jxta_resolver_response_free);

    /* Fill in the required initialization code here. */
	ad->ResPeerID = jxta_id_nullID;
    ad->Response = jstring_new_0();
    ad->QueryID = -1;
    ad->HandlerName = jstring_new_0();
    ad->Credential = jstring_new_0();

    return ad;
}

JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new_1(JString * handlername, JString * response, long qid) 
{
    return jxta_resolver_response_new_2(handlername, response, qid, NULL);
}

JXTA_DECLARE(ResolverResponse *) jxta_resolver_response_new_2(JString * handlername, JString * response, long qid, Jxta_id * res_pid) 
{
    JString *temps = NULL;

    ResolverResponse *ad;
    ad = (ResolverResponse *) malloc(sizeof(ResolverResponse));
    memset(ad, 0xda, sizeof(ResolverResponse));
    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                "jxta:ResolverResponse",
                                ResolverResponse_tags,
                                (JxtaAdvertisementGetXMLFunc) jxta_resolver_response_get_xml,
                                NULL, 
                                NULL, 
                                (FreeFunc) jxta_resolver_response_free);

    /* Fill in the required initialization code here. */
    ad->Credential = jstring_new_0();
	if (res_pid != NULL) {
	  jxta_id_to_jstring(res_pid, &temps);
	  jxta_id_from_jstring(&ad->ResPeerID, temps);
	  JXTA_OBJECT_RELEASE(temps);
	} else {
	  ad->ResPeerID = jxta_id_nullID;
	}

    JXTA_OBJECT_SHARE(handlername);
    ad->HandlerName = handlername;
    ad->QueryID = qid;
    JXTA_OBJECT_SHARE(response);
    ad->Response = response;
    return ad;
}

/** Shred the memory going out.  Again,
* if there ever was a segfault (unlikely,
* of course), the hex value dddddddd will 
* pop right out as a piece of memory accessed
* after it was freed...
*/
void jxta_resolver_response_free(ResolverResponse * ad) 
{
    if (ad->Credential) {
        JXTA_OBJECT_RELEASE(ad->Credential);
    }
	if (ad->ResPeerID) {
	    JXTA_OBJECT_RELEASE(ad->ResPeerID);
	}
    if (ad->HandlerName) {
        JXTA_OBJECT_RELEASE(ad->HandlerName);
    }
    if (ad->Response) {
        JXTA_OBJECT_RELEASE(ad->Response);
    }
    jxta_advertisement_delete((Jxta_advertisement *) ad);
    memset(ad, 0xdd, sizeof(ResolverResponse));
    free(ad);
}

JXTA_DECLARE(void) jxta_resolver_response_parse_charbuffer(ResolverResponse * ad, const char *buf, int len) 
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_resolver_response_parse_file(ResolverResponse * ad, FILE * stream) 
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

#ifdef STANDALONE
int main(int argc, char **argv) {
    ResolverResponse *ad;
    FILE *testfile;
    JString *doc;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_resolver_response_new();

    testfile = fopen(argv[1], "r");
    jxta_resolver_response_parse_file(ad, testfile);
    fclose(testfile);
    jxta_resolver_response_get_xml(ad, &doc);
    fprintf(stdout, "%s\n", jstring_get_string(doc));

    /* ResolverResponse_print_xml(ad,fprintf,stdout); */
    jxta_resolver_response_free(ad);
    JXTA_OBJECT_RELEASE(doc);

    return 0;
}
#endif

#ifdef __cplusplus
}
#endif
