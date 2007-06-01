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
 * $Id: jxta_relaya.c,v 1.16 2006/10/31 22:08:54 bondolo Exp $
 */


/* 
 * The following command will compile the output from the script 
 * given the apr is installed correctly.
 */
/*
 * gcc -DSTANDALONE jxta_advertisement.c Jxta_RelayAdvertisement.c  -o PA \
     `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
     -lexpat -L/usr/local/apache2/lib/ -lapr
 */

#include <stdio.h>
#include <string.h>

#include "jxta_debug.h"
#include "jxta_errno.h"
#include "jdlist.h"
#include "jxta_xml_util.h"
#include "jxta_relaya.h"

#include "jxta_apr.h"
#define DEBUG 1

/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
    Null_,
    Jxta_RelayAdvertisement_,
    IsClient_,
    IsServer_,
    HttpRelay_,
    TcpRelay_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_RelayAdvertisement {

    Jxta_advertisement jxta_advertisement;
    char *jxta_RelayAdvertisement;
    JString *IsClient;
    JString *IsServer;
    Jxta_vector *Httplist;
    Jxta_vector *Tcplist;

};

/* Forw decl for un-exported function */
static void jxta_RelayAdvertisement_delete(Jxta_RelayAdvertisement *);

static Jxta_vector *
    jxta_RelayAdvertisement_get_indexes(Jxta_advertisement * dummy);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_RelayAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_RelayAdvertisement * ad = (Jxta_RelayAdvertisement*)userdata; */
    /* JXTA_LOG("In Jxta_RelayAdvertisement element\n"); */
}


static void handleHttpRelay(void *userdata, const XML_Char * cd, int len)
{

    JString *relay;
    Jxta_RelayAdvertisement *ad = (Jxta_RelayAdvertisement *) userdata;

    if (len == 0)
        return;

    JXTA_LOG("Http relay element\n");

    relay = jstring_new_1(len);
    jstring_append_0(relay, cd, len);
    jstring_trim(relay);

    if (jstring_length(relay) > 0) {

        jxta_vector_add_object_last(ad->Httplist, (Jxta_object *) relay);
    }

    JXTA_OBJECT_RELEASE(relay);
    relay = NULL;
}


static void handleTcpRelay(void *userdata, const XML_Char * cd, int len)
{

    JString *relay;
    Jxta_RelayAdvertisement *ad = (Jxta_RelayAdvertisement *) userdata;

    if (len == 0)
        return;

    JXTA_LOG("TCP relay element\n");

    relay = jstring_new_1(len);
    jstring_append_0(relay, cd, len);
    jstring_trim(relay);

    if (jstring_length(relay) > 0) {

        jxta_vector_add_object_last(ad->Tcplist, (Jxta_object *) relay);
    }

    JXTA_OBJECT_RELEASE(relay);
    relay = NULL;
}


static void handleIsClient(void *userdata, const XML_Char * cd, int len)
{

    JString *isClient;

    Jxta_RelayAdvertisement *ad = (Jxta_RelayAdvertisement *) userdata;
    if (len == 0)
        return;

    isClient = jstring_new_1(len);
    jstring_append_0(isClient, cd, len);
    jstring_trim(isClient);
    ad->IsClient = isClient;
}

static void handleIsServer(void *userdata, const XML_Char * cd, int len)
{

    JString *isServer;

    Jxta_RelayAdvertisement *ad = (Jxta_RelayAdvertisement *) userdata;
    if (len == 0)
        return;

    isServer = jstring_new_1(len);
    jstring_append_0(isServer, cd, len);
    jstring_trim(isServer);
    ad->IsServer = isServer;
}

/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(JString *)
    jxta_RelayAdvertisement_get_IsServer(Jxta_RelayAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->IsServer);
    return ad->IsServer;
}


char *JXTA_STDCALL jxta_RelayAdvertisement_get_IsServer_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_RelayAdvertisement *) ad)->IsServer));
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_set_IsServer(Jxta_RelayAdvertisement * ad, JString * server)
{
    JXTA_OBJECT_SHARE(server);
    JXTA_OBJECT_RELEASE(ad->IsServer);
    ad->IsServer = server;
}


JXTA_DECLARE(JString *)
    jxta_RelayAdvertisement_get_IsClient(Jxta_RelayAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->IsClient);
    return ad->IsClient;
}


char *JXTA_STDCALL jxta_RelayAdvertisement_get_IsClient_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_RelayAdvertisement *) ad)->IsClient));
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_set_IsClient(Jxta_RelayAdvertisement * ad, JString * server)
{
    JXTA_OBJECT_SHARE(server);
    JXTA_OBJECT_RELEASE(ad->IsClient);
    ad->IsClient = server;
}


JXTA_DECLARE(Jxta_vector *)
    jxta_RelayAdvertisement_get_HttpRelay(Jxta_RelayAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->Httplist);
    return ad->Httplist;
}

char *JXTA_STDCALL jxta_RelayAdvertisement_get_HttpRelay_string(Jxta_advertisement * ad)
{
    return NULL;
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_set_HttpRelay(Jxta_RelayAdvertisement * ad, Jxta_vector * relays)
{
    JXTA_OBJECT_SHARE(relays);
    JXTA_OBJECT_RELEASE(ad->Httplist);
    ad->Httplist = relays;
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_add_HttpRelay(Jxta_RelayAdvertisement * ad, JString * relay)
{
    jxta_vector_add_object_last( ad->Httplist, (Jxta_object*) relay );
}


JXTA_DECLARE(Jxta_vector *)
    jxta_RelayAdvertisement_get_TcpRelay(Jxta_RelayAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->Tcplist);
    return ad->Tcplist;
}

char *JXTA_STDCALL jxta_RelayAdvertisement_get_TcpRelay_string(Jxta_advertisement * ad)
{
    return NULL;
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_set_TcpRelay(Jxta_RelayAdvertisement * ad, Jxta_vector * relays)
{
    JXTA_OBJECT_SHARE(relays);
    JXTA_OBJECT_RELEASE(ad->Tcplist);
    ad->Tcplist = relays;
}

JXTA_DECLARE(void)
    jxta_RelayAdvertisement_add_TcpRelay(Jxta_RelayAdvertisement * ad, JString * relay)
{
    jxta_vector_add_object_last( ad->Tcplist, (Jxta_object*) relay );
}


/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_RelayAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:RelayAdvertisement", Jxta_RelayAdvertisement_, *handleJxta_RelayAdvertisement, NULL, NULL},
    {"isClient", IsClient_, *handleIsClient, *jxta_RelayAdvertisement_get_IsClient_string, NULL},
    {"isServer", IsServer_, *handleIsServer, *jxta_RelayAdvertisement_get_IsServer_string, NULL},
    {"httpaddress", HttpRelay_, *handleHttpRelay, *jxta_RelayAdvertisement_get_HttpRelay_string, NULL},
    {"tcpaddress", TcpRelay_, *handleTcpRelay, *jxta_RelayAdvertisement_get_TcpRelay_string, NULL},
    {"httpaddr", HttpRelay_, *handleHttpRelay, *jxta_RelayAdvertisement_get_HttpRelay_string, NULL},
    {"tcpaddr", TcpRelay_, *handleTcpRelay, *jxta_RelayAdvertisement_get_TcpRelay_string, NULL},
    {NULL, 0, 0, NULL, NULL}
};

/* This being an internal call, we chose a behaviour for handling the jstring
 * that's better suited to our need: we are passed an existing jstring
 * and append to it.
 */

JXTA_DECLARE(void) httpRelay_printer(Jxta_RelayAdvertisement * ad, JString * js)
{

    int sz;
    int i;
    Jxta_vector *relays = ad->Httplist;
    sz = jxta_vector_size(relays);

    for (i = 0; i < sz; ++i) {
        JString *relay;

        jxta_vector_get_object_at(relays, JXTA_OBJECT_PPTR(&relay), i);
        jstring_append_2(js, "<httpaddress>");
        jstring_append_1(js, relay);
        jstring_append_2(js, "</httpaddress>\n");

        JXTA_OBJECT_RELEASE(relay);
    }
}

JXTA_DECLARE(void) tcpRelay_printer(Jxta_RelayAdvertisement * ad, JString * js)
{

    int sz;
    int i;
    Jxta_vector *relays = ad->Tcplist;
    sz = jxta_vector_size(relays);

    for (i = 0; i < sz; ++i) {
        JString *relay;

        jxta_vector_get_object_at(relays, JXTA_OBJECT_PPTR(&relay), i);
        jstring_append_2(js, "<tcpaddress>");
        jstring_append_1(js, relay);
        jstring_append_2(js, "</tcpaddress>\n");

        JXTA_OBJECT_RELEASE(relay);
    }
}

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

JXTA_DECLARE(Jxta_status)
    jxta_RelayAdvertisement_get_xml(Jxta_RelayAdvertisement * ad, JString ** result)
{
    JString *string = jstring_new_0();

    jstring_append_2(string, "<jxta:RelayAdvertisement xmlns:jxta=\"http://jxta.org\" type=\"jxta:RelayAdvertisement\">\n");
    jstring_append_2(string, "<isClient>");
    jstring_append_1(string, ad->IsClient);
    jstring_append_2(string, "</isClient>\n");

    jstring_append_2(string, "<isServer>");
    jstring_append_1(string, ad->IsServer);
    jstring_append_2(string, "</isServer>\n");

    httpRelay_printer(ad, string);
    tcpRelay_printer(ad, string);

    jstring_append_2(string, "</jxta:RelayAdvertisement>\n");

    *result = string;
    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_RelayAdvertisement *)
    jxta_RelayAdvertisement_new(void)
{

    Jxta_RelayAdvertisement *ad;

    ad = (Jxta_RelayAdvertisement *) malloc(sizeof(Jxta_RelayAdvertisement));

    memset(ad, 0, sizeof(Jxta_RelayAdvertisement));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:RelayAdvertisement",
                                  Jxta_RelayAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_RelayAdvertisement_get_xml,
                                  NULL,
                                  (JxtaAdvertisementGetIndexFunc) jxta_RelayAdvertisement_get_indexes,
                                  (FreeFunc) jxta_RelayAdvertisement_delete);

    ad->IsServer = jstring_new_0();
    ad->IsClient = jstring_new_0();
    ad->Httplist = jxta_vector_new(2);
    ad->Tcplist = jxta_vector_new(2);

    return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void jxta_RelayAdvertisement_delete(Jxta_RelayAdvertisement * ad)
{
    JXTA_OBJECT_RELEASE(ad->IsClient);
    JXTA_OBJECT_RELEASE(ad->IsServer);
    JXTA_OBJECT_RELEASE(ad->Httplist);
    JXTA_OBJECT_RELEASE(ad->Tcplist);

    jxta_advertisement_delete((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_RelayAdvertisement));
    free(ad);
}


JXTA_DECLARE(Jxta_status) jxta_RelayAdvertisement_parse_charbuffer(Jxta_RelayAdvertisement * ad, const char *buf, int len)
{

    return jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(Jxta_status) jxta_RelayAdvertisement_parse_file(Jxta_RelayAdvertisement * ad, FILE * stream)
{

    return jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

static Jxta_vector *
    jxta_RelayAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = { {NULL, NULL} };
    return jxta_advertisement_return_indexes(idx[0]);
}

/* vi: set ts=4 sw=4 tw=130 et: */
