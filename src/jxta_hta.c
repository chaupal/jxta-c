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
 * $Id: jxta_hta.c,v 1.56 2005/10/27 01:55:26 slowhog Exp $
 */


#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <apr_want.h>
#include <apr_network_io.h>

#include "jpr/jpr_apr_wrapper.h"

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_advertisement.h"
#include "jxta_hta.h"
#include "jxta_xml_util.h"


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
    Jxta_HTTPTransportAdvertisement_,
    Protocol_,
    InterfaceAddress_,
    ConfigMode_,
    Port_,
    Proxy_,
    ProxyOff_,
    Server_,
    ServerOff_
};

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_HTTPTransportAdvertisement {

    Jxta_advertisement jxta_advertisement;
    char *jxta_HTTPTransportAdvertisement;
    JString *Protocol;
    Jxta_in_addr InterfaceAddress;
    JString *ConfigMode;
    Jxta_port Port;
    JString *Proxy;
    Jxta_boolean ProxyOff;
    JString *Server;
    Jxta_boolean ServerOff;
};

/* Forw decl for un-exported function */
static void jxta_HTTPTransportAdvertisement_delete(Jxta_HTTPTransportAdvertisement *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_HTTPTransportAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_HTTPTransportAdvertisement * ad = (Jxta_HTTPTransportAdvertisement*)userdata; */
    /* JXTA_LOG("In Jxta_HTTPTransportAdvertisement element\n"); */
}


static void handleProtocol(void *userdata, const XML_Char * cd, int len)
{

    JString *proto;
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    proto = jstring_new_1(len);
    jstring_append_0(proto, cd, len);
    jstring_trim(proto);

    jxta_HTTPTransportAdvertisement_set_Protocol(ad, proto);
    JXTA_OBJECT_RELEASE(proto);
}

static void handleInterfaceAddress(void *userdata, const XML_Char * cd, int len)
{

    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;
    Jxta_in_addr address = 0;

    extract_ip(cd, len, &address);
    if (address != 0) {
        ad->InterfaceAddress = address;
    }

}


static void handleConfigMode(void *userdata, const XML_Char * cd, int len)
{

    JString *mode;
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    mode = jstring_new_1(len);
    jstring_append_0(mode, cd, len);
    jstring_trim(mode);

    jxta_HTTPTransportAdvertisement_set_ConfigMode(ad, mode);
    JXTA_OBJECT_RELEASE(mode);
}


static void handlePort(void *userdata, const XML_Char * cd, int len)
{

    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;
    extract_port(cd, len, &(ad->Port));

#if DEBUG
    JXTA_LOG("In Port element\n");
#endif

}

static void handleProxy(void *userdata, const XML_Char * cd, int len)
{

    JString *proxy;
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    proxy = jstring_new_1(len);
    jstring_append_0(proxy, cd, len);
    jstring_trim(proxy);

    jxta_HTTPTransportAdvertisement_set_Proxy(ad, proxy);
    JXTA_OBJECT_RELEASE(proxy);
}


static void handleProxyOff(void *userdata, const XML_Char * cd, int len)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;
    if (len == 0)
        return;

    /* else the tag is present. That's all that counts. */
    ad->ProxyOff = TRUE;
}


static void handleServer(void *userdata, const XML_Char * cd, int len)
{

    JString *server;
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    server = jstring_new_1(len);
    jstring_append_0(server, cd, len);
    jstring_trim(server);

    jxta_HTTPTransportAdvertisement_set_Server(ad, server);
    JXTA_OBJECT_RELEASE(server);

}


static void handleServerOff(void *userdata, const XML_Char * cd, int len)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) userdata;
    if (len == 0)
        return;

    /* else the tag is present. That's all that counts. */
    ad->ServerOff = TRUE;

    /* JXTA_LOG("In ServerOff element\n"); */
}




/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(char *)
    jxta_HTTPTransportAdvertisement_get_Jxta_HTTPTransportAdvertisement(Jxta_HTTPTransportAdvertisement * ad)
{
    return NULL;
}

JXTA_DECLARE(char *)
    jxta_HTTPTransportAdvertisement_get_Jxta_HTTPTransportAdvertisement_string(Jxta_advertisement * ad)
{
    return NULL;
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_Jxta_HTTPTransportAdvertisement(Jxta_HTTPTransportAdvertisement * ad, char *name)
{

}

JXTA_DECLARE(JString *)
    jxta_HTTPTransportAdvertisement_get_Protocol(Jxta_HTTPTransportAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->Protocol);
    return ad->Protocol;
}


char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_Protocol_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_HTTPTransportAdvertisement *) ad)->Protocol));
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_Protocol(Jxta_HTTPTransportAdvertisement * ad, JString * protocol)
{
    JXTA_OBJECT_SHARE(protocol);
    JXTA_OBJECT_RELEASE(ad->Protocol);
    ad->Protocol = protocol;
}

JXTA_DECLARE(Jxta_in_addr)
    jxta_HTTPTransportAdvertisement_get_InterfaceAddress(Jxta_HTTPTransportAdvertisement * ad)
{
    return ad->InterfaceAddress;
}

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_InterfaceAddress_string(Jxta_advertisement * adv)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) adv;

    char *returned_buf = (char *) malloc(INET_ADDRSTRLEN);

    /*
     * FIXME: jice@jxta.org - we're not supposed to support IPV6, are we ?
     */
    return (char *) jpr_inet_ntop(AF_INET, &(ad->InterfaceAddress), returned_buf, INET_ADDRSTRLEN);
}

JXTA_DECLARE(void)
jxta_HTTPTransportAdvertisement_set_InterfaceAddress(Jxta_HTTPTransportAdvertisement * ad, Jxta_in_addr address)
{
    ad->InterfaceAddress = address;
}



JXTA_DECLARE(JString *)
    jxta_HTTPTransportAdvertisement_get_ConfigMode(Jxta_HTTPTransportAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->ConfigMode);
    return ad->ConfigMode;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_ConfigMode_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_HTTPTransportAdvertisement *) ad)->ConfigMode));
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_ConfigMode(Jxta_HTTPTransportAdvertisement * ad, JString * mode)
{
    JXTA_OBJECT_SHARE(mode);
    JXTA_OBJECT_RELEASE(ad->ConfigMode);
    ad->ConfigMode = mode;
}


JXTA_DECLARE(Jxta_port)
    jxta_HTTPTransportAdvertisement_get_Port(Jxta_HTTPTransportAdvertisement * ad)
{
    return ad->Port;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_Port_string(Jxta_advertisement * adv)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) adv;

    char *portstr = (char *) calloc(1, 11);

    apr_snprintf(portstr, 11,"%d", ad->Port);
    return portstr;
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_Port(Jxta_HTTPTransportAdvertisement * ad, Jxta_port port)
{
    ad->Port = port;
}

JXTA_DECLARE(JString *)
    jxta_HTTPTransportAdvertisement_get_Proxy(Jxta_HTTPTransportAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->Proxy);
    return ad->Proxy;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_Proxy_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_HTTPTransportAdvertisement *) ad)->Proxy));
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_Proxy(Jxta_HTTPTransportAdvertisement * ad, JString * proxy)
{
    JXTA_OBJECT_SHARE(proxy);
    JXTA_OBJECT_RELEASE(ad->Proxy);
    ad->Proxy = proxy;
}

JXTA_DECLARE(Jxta_boolean)
    jxta_HTTPTransportAdvertisement_get_ProxyOff(Jxta_HTTPTransportAdvertisement * ad)
{
    return ad->ProxyOff;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_ProxyOff_string(Jxta_advertisement * adv)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) adv;

    return strdup(ad->ProxyOff ? "TRUE" : "FALSE");
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_ProxyOff(Jxta_HTTPTransportAdvertisement * ad, Jxta_boolean poff)
{
    ad->ProxyOff = poff;
}



JXTA_DECLARE(JString *)
    jxta_HTTPTransportAdvertisement_get_Server(Jxta_HTTPTransportAdvertisement * ad)
{
    JXTA_OBJECT_SHARE(ad->Server);
    return ad->Server;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_Server_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_HTTPTransportAdvertisement *) ad)->Server));
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_Server(Jxta_HTTPTransportAdvertisement * ad, JString * server)
{
    JXTA_OBJECT_SHARE(server);
    JXTA_OBJECT_RELEASE(ad->Server);
    ad->Server = server;
}

JXTA_DECLARE(Jxta_boolean)
    jxta_HTTPTransportAdvertisement_get_ServerOff(Jxta_HTTPTransportAdvertisement * ad)
{
    return ad->ServerOff;
}

char *JXTA_STDCALL jxta_HTTPTransportAdvertisement_get_ServerOff_string(Jxta_advertisement * adv)
{
    Jxta_HTTPTransportAdvertisement *ad = (Jxta_HTTPTransportAdvertisement *) adv;

    return strdup(ad->ServerOff ? "TRUE" : "FALSE");
}

JXTA_DECLARE(void)
    jxta_HTTPTransportAdvertisement_set_ServerOff(Jxta_HTTPTransportAdvertisement * ad, Jxta_boolean soff)
{
    ad->ServerOff = soff;
}




/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_HTTPTransportAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},

    {"jxta:HTTPTransportAdvertisement", Jxta_HTTPTransportAdvertisement_, *handleJxta_HTTPTransportAdvertisement,
     *jxta_HTTPTransportAdvertisement_get_Jxta_HTTPTransportAdvertisement_string, NULL},

    {"Protocol", Protocol_, *handleProtocol, *jxta_HTTPTransportAdvertisement_get_Protocol_string, NULL},
    {"InterfaceAddress", InterfaceAddress_, *handleInterfaceAddress, *jxta_HTTPTransportAdvertisement_get_InterfaceAddress_string,
     NULL},
    {"ConfigMode", ConfigMode_, *handleConfigMode, *jxta_HTTPTransportAdvertisement_get_ConfigMode_string, NULL},
    {"Port", Port_, *handlePort, *jxta_HTTPTransportAdvertisement_get_Port_string, NULL},
    {"Proxy", Proxy_, *handleProxy, *jxta_HTTPTransportAdvertisement_get_Proxy_string, NULL},
    {"ProxyOff", ProxyOff_, *handleProxyOff, *jxta_HTTPTransportAdvertisement_get_ProxyOff_string, NULL},
    {"Server", Server_, *handleServer, *jxta_HTTPTransportAdvertisement_get_Server_string, NULL},
    {"ServerOff", ServerOff_, *handleServerOff, *jxta_HTTPTransportAdvertisement_get_ServerOff_string, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status)
    jxta_HTTPTransportAdvertisement_get_xml(Jxta_HTTPTransportAdvertisement * ad, JString ** result)
{
    char addr_buf[INET_ADDRSTRLEN] = { 0 };
    char port[11] = { 0 };
    JString *string = jstring_new_0();

    jstring_append_2(string,
                     "<jxta:TransportAdvertisement xmlns:jxta=\"http://jxta.org\" type=\"jxta:HTTPTransportAdvertisement\">\n");
    jstring_append_2(string, "<Protocol>");
    jstring_append_1(string, ad->Protocol);
    jstring_append_2(string, "</Protocol>\n");

    jstring_append_2(string, "<InterfaceAddress>");
    jpr_inet_ntop(AF_INET, &(ad->InterfaceAddress), addr_buf, INET_ADDRSTRLEN);
    jstring_append_2(string, addr_buf);
    jstring_append_2(string, "</InterfaceAddress>\n");

    jstring_append_2(string, "<ConfigMode>");
    jstring_append_1(string, ad->ConfigMode);
    jstring_append_2(string, "</ConfigMode>\n");

    jstring_append_2(string, "<Port>");
    snprintf(port, sizeof(port), "%d", ad->Port);
    jstring_append_2(string, port);
    jstring_append_2(string, "</Port>\n");

    jstring_append_2(string, "<Proxy>");
    jstring_append_1(string, ad->Proxy);
    jstring_append_2(string, "</Proxy>\n");

    if (ad->ProxyOff) {
        jstring_append_2(string, "<ProxyOff/>");
    }

    if (ad->ServerOff) {
        jstring_append_2(string, "<ServerOff/>\n");
    }

    jstring_append_2(string, "</jxta:TransportAdvertisement>\n");

    *result = string;
    return JXTA_SUCCESS;
}

/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
JXTA_DECLARE(Jxta_HTTPTransportAdvertisement *)
    jxta_HTTPTransportAdvertisement_new(void)
{

    Jxta_HTTPTransportAdvertisement *ad;

    ad = (Jxta_HTTPTransportAdvertisement *) malloc(sizeof(Jxta_HTTPTransportAdvertisement));

    memset(ad, 0, sizeof(Jxta_HTTPTransportAdvertisement));

    jxta_advertisement_initialize((Jxta_advertisement *) ad,
                                  "jxta:HTTPTransportAdvertisement",
                                  Jxta_HTTPTransportAdvertisement_tags,
                                  (JxtaAdvertisementGetXMLFunc) jxta_HTTPTransportAdvertisement_get_xml,
                                  NULL,
                                  (JxtaAdvertisementGetIndexFunc) jxta_HTTPTransportAdvertisement_get_indexes,
                                  (FreeFunc) jxta_HTTPTransportAdvertisement_delete);

    ad->Protocol = jstring_new_0();
    ad->ConfigMode = jstring_new_0();
    ad->Proxy = jstring_new_0();
    ad->Server = jstring_new_0();
    ad->ProxyOff = FALSE;
    ad->ServerOff = FALSE;

    return ad;
}

static void jxta_HTTPTransportAdvertisement_delete(Jxta_HTTPTransportAdvertisement * ad)
{
    JXTA_OBJECT_RELEASE(ad->Protocol);
    JXTA_OBJECT_RELEASE(ad->ConfigMode);
    JXTA_OBJECT_RELEASE(ad->Proxy);
    JXTA_OBJECT_RELEASE(ad->Server);

    jxta_advertisement_delete((Jxta_advertisement *) ad);

    memset(ad, 0xdd, sizeof(Jxta_HTTPTransportAdvertisement));
    free(ad);
}

JXTA_DECLARE(void)
jxta_HTTPTransportAdvertisement_parse_charbuffer(Jxta_HTTPTransportAdvertisement * ad, const char *buf, int len)
{

    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_HTTPTransportAdvertisement_parse_file(Jxta_HTTPTransportAdvertisement * ad, FILE * stream)
{

    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *)
    jxta_HTTPTransportAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = { {NULL, NULL} };
    return jxta_advertisement_return_indexes(idx[0]);
}

#ifdef __cplusplus
#if 0
{
#endif
}
#endif
