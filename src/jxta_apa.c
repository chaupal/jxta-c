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
 * $Id: jxta_apa.c,v 1.7 2005/02/02 02:58:26 exocetrick Exp $
 */


/* 
 * The following command will compile the output from the script 
 * given the apr is installed correctly.
 */
/*
 * gcc -DSTANDALONE jxta_advertisement.c jxta_AccessPointAdvertisement.c  -o PA \
     `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
     -lexpat -L/usr/local/apache2/lib/ -lapr
 */

#include <stdio.h>
#include <string.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jdlist.h"
#include "jxta_apa.h"
#include "jxta_xml_util.h"

#define DEBUG 1


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
/** Each of these corresponds to a tag in the 
 * xml ad.
 */ enum tokentype {
    Null_,
    Jxta_AccessPointAdvertisement_,
    PID_,
    EA_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_AccessPointAdvertisement {

    Jxta_advertisement jxta_advertisement;
    char *jxta_AccessPointAdvertisement;
    Jxta_id *PID;
    Jxta_vector *endpoints;
};

/* Forw decl for un-exported function */
static void jxta_AccessPointAdvertisement_delete(Jxta_AccessPointAdvertisement *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_AccessPointAdvertisement(void *userdata, const XML_Char * cd, int len)
{
}


static void handlePID(void *userdata, const XML_Char * cd, int len)
{
    Jxta_id *pid = NULL;
    Jxta_AccessPointAdvertisement *ad = (Jxta_AccessPointAdvertisement *) userdata;
    JString *tmp;

    if (len == 0)
        return;

    /* Make room for a final \0 in advance; we'll likely need it. */
    tmp = jstring_new_1(len + 1);

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&pid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (pid != NULL) {
        jxta_AccessPointAdvertisement_set_PID(ad, pid);
        JXTA_OBJECT_RELEASE(pid);
    }
}


static void handleEndpoint(void *userdata, const XML_Char * cd, int len)
{

    JString *endpoint;
    Jxta_AccessPointAdvertisement *ad = (Jxta_AccessPointAdvertisement *) userdata;

    if (len == 0)
        return;

    JXTA_LOG("EndpointPoint element\n");

    endpoint = jstring_new_1(len);
    jstring_append_0(endpoint, cd, len);
    jstring_trim(endpoint);

    if (jstring_length(endpoint) > 0) {

        jxta_vector_add_object_last(ad->endpoints, (Jxta_object *) endpoint);
    }

    JXTA_OBJECT_RELEASE(endpoint);
    endpoint = NULL;
}


/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
char *jxta_AccessPointAdvertisement_get_Jxta_AccessPointAdvertisement(Jxta_AccessPointAdvertisement * ad)
{
    return NULL;
}

char *jxta_AccessPointAdvertisement_get_Jxta_AccessPointAdvertisement_string(Jxta_advertisement * ad)
{
    return NULL;
}

void jxta_AccessPointAdvertisement_set_Jxta_AccessPointAdvertisement(Jxta_AccessPointAdvertisement * ad, char *name)
{
}

Jxta_id *jxta_AccessPointAdvertisement_get_PID(Jxta_AccessPointAdvertisement * ad)
{
    if (ad->PID != NULL) {
        JXTA_OBJECT_SHARE(ad->PID);
        return ad->PID;
    } else {
        return NULL;
    }
}


void jxta_AccessPointAdvertisement_set_PID(Jxta_AccessPointAdvertisement * ad, Jxta_id * id)
{

    if (id == NULL)
        return;

    if (ad->PID != NULL)
        JXTA_OBJECT_RELEASE(ad->PID);

    ad->PID = id;
    JXTA_OBJECT_SHARE(ad->PID);
}


Jxta_vector *jxta_AccessPointAdvertisement_get_EndpointAddresses(Jxta_AccessPointAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->endpoints);
    return ad->endpoints;
}

void jxta_AccessPointAdvertisement_set_EndpointAddresses(Jxta_AccessPointAdvertisement * ad, Jxta_vector * addresses)
{
    JXTA_OBJECT_SHARE(addresses);
    JXTA_OBJECT_RELEASE(ad->endpoints);
    ad->endpoints = addresses;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
const static Kwdtab Jxta_AccessPointAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL},
    {"jxta:APA", Jxta_AccessPointAdvertisement_, *handleJxta_AccessPointAdvertisement,
     *jxta_AccessPointAdvertisement_get_Jxta_AccessPointAdvertisement_string},
    {"PID", PID_, *handlePID, *jxta_AccessPointAdvertisement_get_PID},
    {"EA", EA_, *handleEndpoint, NULL},
    {NULL, 0, 0, NULL}
};

/* This being an internal call, we chose a behaviour for handling the jstring
 * that's better suited to our need: we are passed an existing jstring
 * and append to it.
 */

void endpoints_printer(Jxta_AccessPointAdvertisement * ad, JString * js)
{

    int sz;
    int i;
    Jxta_vector *lists = ad->endpoints;
    sz = jxta_vector_size(lists);

    for (i = 0; i < sz; ++i) {
        JString *endpoint;

        jxta_vector_get_object_at(lists, (Jxta_object **) & endpoint, i);
        jstring_append_2(js, "<EA>");
        jstring_append_1(js, endpoint);
        jstring_append_2(js, "</EA>\n");
        JXTA_OBJECT_RELEASE(endpoint);
    }
}

Jxta_status jxta_AccessPointAdvertisement_get_xml(Jxta_AccessPointAdvertisement * ad, JString ** result)
{

    JString *string = jstring_new_0();
    JString *tmpref;

    jstring_append_2(string, "<jxta:APA xmlns:jxta=\"http://jxta.org\" type=\"jxta:APA\">\n");

    if (ad->PID != NULL) {
        jstring_append_2(string, "<PID>");
        tmpref = NULL;
        jxta_id_to_jstring(ad->PID, &tmpref);
        jstring_append_1(string, tmpref);
        JXTA_OBJECT_RELEASE(tmpref);
        jstring_append_2(string, "</PID>\n");
    }

    endpoints_printer(ad, string);

    jstring_append_2(string, "</jxta:APA>\n");

    *result = string;
    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
Jxta_AccessPointAdvertisement *jxta_AccessPointAdvertisement_new(void)
{

    Jxta_AccessPointAdvertisement *ad;

    ad = (Jxta_AccessPointAdvertisement *) malloc(sizeof(Jxta_AccessPointAdvertisement));

    memset(ad, 0, sizeof(Jxta_AccessPointAdvertisement));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:APA",
                                  Jxta_AccessPointAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_AccessPointAdvertisement_get_xml,
                                  NULL,
                                  (JxtaAdvertisementGetIndexFunc) jxta_AccessPointAdvertisement_get_indexes,
                                  (FreeFunc) jxta_AccessPointAdvertisement_delete);

    ad->PID = NULL;
    ad->endpoints = jxta_vector_new(1);

    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void jxta_AccessPointAdvertisement_delete(Jxta_AccessPointAdvertisement * ad)
{
    if (ad->PID != NULL)
        JXTA_OBJECT_RELEASE(ad->PID);

    if (ad->endpoints != NULL)
        JXTA_OBJECT_RELEASE(ad->endpoints);

    jxta_advertisement_delete((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_AccessPointAdvertisement));
    free(ad);
}


void jxta_AccessPointAdvertisement_parse_charbuffer(Jxta_AccessPointAdvertisement * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

void jxta_AccessPointAdvertisement_parse_file(Jxta_AccessPointAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

Jxta_vector *jxta_AccessPointAdvertisement_get_indexes(void)
{
    const char *idx[][2] = {
        {"PID", NULL},
        {NULL, NULL}
    };
    return jxta_advertisement_return_indexes(idx);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    Jxta_AccessPointAdvertisement *ad;
    FILE *testfile;

    if (argc != 2) {
        printf("usage: ad <filename>\n");
        return -1;
    }

    ad = jxta_AccessPointAdvertisement_new();

    testfile = fopen(argv[1], "r");
    jxta_AccessPointAdvertisement_parse_file(ad, testfile);
    fclose(testfile);

    jxta_AccessPointAdvertisement_print_xml(ad, fprintf, stdout);
    jxta_AccessPointAdvertisement_delete(ad);

    return 0;
}
#endif


#ifdef __cplusplus
}
#endif
