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

static const char *__log_cat = "TCPADV";

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "jxta_errno.h"
#include "jxta_tta.h"
#include "jxta_log.h"
#include "jxta_xml_util.h"
#include "jxta_apr.h"
#include "jpr/jpr_apr_wrapper.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */ 
enum tokentype {
    Null_,
    Jxta_TCPTransportAdvertisement_,
    Protocol_,
    Port_,
    MulticastOff_,
    MulticastAddr_,
    MulticastPort_,
    MulticastSize_,
    Server_,
    InterfaceAddress_,
    ConfigMode_,
    ServerOff_,
    ClientOff_
};

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_TCPTransportAdvertisement {
    Jxta_advertisement jxta_advertisement;
    char *jxta_TCPTransportAdvertisement;
    JString *Protocol;
    Jxta_port Port;
    Jxta_boolean MulticastOff;
    Jxta_in_addr MulticastAddr;
    Jxta_port MulticastPort;
    int MulticastSize;
    JString *Server;
    char * InterfaceAddress;
    JString *ConfigMode;
    Jxta_boolean ServerOff;
    Jxta_boolean ClientOff;
    Jxta_boolean PublicAddressOnly; /* not implemented yet.. */
};

    /* Forw decl. of un-exported function */
static void jxta_TCPTransportAdvertisement_delete(Jxta_object * obj);
static Jxta_TCPTransportAdvertisement *jxta_TCPTransportAdvertisement_construct(Jxta_TCPTransportAdvertisement * self);
static void jxta_TCPTransportAdvertisement_destruct(Jxta_TCPTransportAdvertisement * self);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void handleJxta_TCPTransportAdvertisement(void *userdata, const XML_Char * cd, int len)
{
    /* Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; */
}

static void handleProtocol(void *userdata, const XML_Char * cd, int len)
{
    JString *proto;
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    proto = jstring_new_1(len);
    jstring_append_0(proto, cd, len);
    jstring_trim(proto);

    jxta_TCPTransportAdvertisement_set_Protocol(ad, proto);
    JXTA_OBJECT_RELEASE(proto);
}

static void handlePort(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;
    extract_port(cd, len, &ad->Port);
}

static void handleMulticastOff(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    /* else the tag is present. That's all that counts. */
    ad->MulticastOff = TRUE;
}

static void handleMulticastAddr(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;
    Jxta_in_addr address = 0;

    extract_ip(cd, len, &address);
    if (address != 0) {
        ad->MulticastAddr = address;
    }
}

static void handleMulticastPort(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;
    extract_port(cd, len, &ad->MulticastPort);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "In MulticastPort element\n");
}

static void handleMulticastSize(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;
    ad->MulticastSize = atoi(cd);
}

static void handleServer(void *userdata, const XML_Char * cd, int len)
{
    JString *server;
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    server = jstring_new_1(len);
    jstring_append_0(server, cd, len);
    jstring_trim(server);

    jxta_TCPTransportAdvertisement_set_Server(ad, server);
    JXTA_OBJECT_RELEASE(server);
}

static void handleInterfaceAddress(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = userdata;
    int i;

    if (len == 0)
        return;

    assert(NULL == ad->InterfaceAddress);
    i = 0;
    while (len > 0 && ('\n' == *cd || '\r' == *cd || ' ' == *cd)) {
        ++cd;
        --len;
    }
    while (len > 0 && ('\n' == cd[len - 1] || '\r' == cd[len - 1] || ' ' == cd[len - 1])) {
        len--;
    }

    ad->InterfaceAddress = calloc(len + 1, sizeof(XML_Char));
    strncpy(ad->InterfaceAddress, cd, len);
}

static void handleConfigMode(void *userdata, const XML_Char * cd, int len)
{
    JString *mode;
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    mode = jstring_new_1(len);
    jstring_append_0(mode, cd, len);
    jstring_trim(mode);

    jxta_TCPTransportAdvertisement_set_ConfigMode(ad, mode);
    JXTA_OBJECT_RELEASE(mode);
}

static void handleServerOff(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    /* else the tag is present. That's all that counts. */
    ad->ServerOff = TRUE;
}

static void handleClientOff(void *userdata, const XML_Char * cd, int len)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) userdata;

    if (len == 0)
        return;

    /* else the tag is present. That's all that counts. */
    ad->ClientOff = TRUE;
}

JXTA_DECLARE(JString *)
    jxta_TCPTransportAdvertisement_get_Protocol(Jxta_TCPTransportAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->Protocol);
}

JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_Protocol_string(Jxta_advertisement * ad)
{
    return strdup(jstring_get_string(((Jxta_TCPTransportAdvertisement *) ad)->Protocol));
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_Protocol(Jxta_TCPTransportAdvertisement * ad, JString * protocol)
{
    JXTA_OBJECT_SHARE(protocol);
    JXTA_OBJECT_RELEASE(ad->Protocol);
    ad->Protocol = protocol;
}

JXTA_DECLARE(Jxta_port)
    jxta_TCPTransportAdvertisement_get_Port(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->Port;
}

JXTA_DECLARE(char *) jxta_TCPTransportAdvertisement_get_Port_string(Jxta_advertisement * ad)
{
    char *s = calloc(21, sizeof(char)); /* That's what it may take to print an int */

    apr_snprintf(s, 21, "%d", jxta_TCPTransportAdvertisement_get_Port((Jxta_TCPTransportAdvertisement *) ad));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "port from get_port_string: %s\n", s);

    /*
     * The semantics of all other xx_string adv methods is to return a malloc'ed
     * string.
     */
    return s;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_Port(Jxta_TCPTransportAdvertisement * ad, Jxta_port port)
{
    ad->Port = port;
}

JXTA_DECLARE(Jxta_boolean)
    jxta_TCPTransportAdvertisement_get_MulticastOff(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->MulticastOff;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_MulticastOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff)
{
    ad->MulticastOff = isOff;
}

JXTA_DECLARE(Jxta_in_addr)
    jxta_TCPTransportAdvertisement_get_MulticastAddr(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->MulticastAddr;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_MulticastAddr(Jxta_TCPTransportAdvertisement * ad, Jxta_in_addr addr)
{
    ad->MulticastAddr = addr;
}

JXTA_DECLARE(Jxta_port)
    jxta_TCPTransportAdvertisement_get_MulticastPort(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->MulticastPort;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_MulticastPort(Jxta_TCPTransportAdvertisement * ad, Jxta_port port)
{
    ad->MulticastPort = port;
}

JXTA_DECLARE(int) jxta_TCPTransportAdvertisement_get_MulticastSize(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->MulticastSize;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_MulticastSize(Jxta_TCPTransportAdvertisement * ad, int size)
{
    ad->MulticastSize = size;
}

JXTA_DECLARE(JString *)
    jxta_TCPTransportAdvertisement_get_Server(Jxta_TCPTransportAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->Server);
}

JXTA_DECLARE(char *)
    jxta_TCPTransportAdvertisement_get_Server_string(Jxta_TCPTransportAdvertisement * ad)
{
    return strdup(jstring_get_string(ad->Server));
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_Server(Jxta_TCPTransportAdvertisement * ad, JString * server)
{
    if (ad->Server != NULL)
        JXTA_OBJECT_RELEASE(ad->Server);
    ad->Server = JXTA_OBJECT_SHARE(server);
}

JXTA_DECLARE(Jxta_in_addr)
    jxta_TCPTransportAdvertisement_get_InterfaceAddress(Jxta_TCPTransportAdvertisement * ad)
{
    Jxta_in_addr ia = 0;

    JXTA_DEPRECATED_API();
    extract_ip(ad->InterfaceAddress, strlen(ad->InterfaceAddress), &ia);
    return ia;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_InterfaceAddress(Jxta_TCPTransportAdvertisement * ad, Jxta_in_addr addr)
{
    JXTA_DEPRECATED_API();

    if (ad->InterfaceAddress) {
        free(ad->InterfaceAddress);
    }

    ad->InterfaceAddress = calloc(INET_ADDRSTRLEN + 1, sizeof(char));
    jpr_inet_ntop(AF_INET, &addr, ad->InterfaceAddress, INET_ADDRSTRLEN);
}

JXTA_DECLARE(const char*) jxta_TCPTransportAdvertisement_InterfaceAddress(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->InterfaceAddress;
}

JXTA_DECLARE(JString *)
    jxta_TCPTransportAdvertisement_get_ConfigMode(Jxta_TCPTransportAdvertisement * ad)
{
    return JXTA_OBJECT_SHARE(ad->ConfigMode);
}

JXTA_DECLARE(char *)
    jxta_TCPTransportAdvertisement_get_ConfigMode_string(Jxta_TCPTransportAdvertisement * ad)
{
    return strdup(jstring_get_string(ad->ConfigMode));
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_ConfigMode(Jxta_TCPTransportAdvertisement * ad, JString * mode)
{
    JXTA_OBJECT_RELEASE(ad->ConfigMode);
    ad->ConfigMode = JXTA_OBJECT_SHARE(mode);
}

JXTA_DECLARE(Jxta_boolean)
    jxta_TCPTransportAdvertisement_get_ServerOff(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->ServerOff;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_ServerOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff)
{
    ad->ServerOff = isOff;
}

JXTA_DECLARE(Jxta_boolean)
    jxta_TCPTransportAdvertisement_get_ClientOff(Jxta_TCPTransportAdvertisement * ad)
{
    return ad->ClientOff;
}

JXTA_DECLARE(void)
    jxta_TCPTransportAdvertisement_set_ClientOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff)
{
    ad->ClientOff = isOff;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
static const Kwdtab Jxta_TCPTransportAdvertisement_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"jxta:TCPTransportAdvertisement", Jxta_TCPTransportAdvertisement_, *handleJxta_TCPTransportAdvertisement, NULL, NULL},
    {"Protocol", Protocol_, *handleProtocol, NULL, NULL},
    {"Port", Port_, *handlePort, NULL, NULL},
    {"MulticastOff", MulticastOff_, *handleMulticastOff, NULL, NULL},
    {"MulticastAddr", MulticastAddr_, *handleMulticastAddr, NULL, NULL},
    {"MulticastPort", MulticastPort_, *handleMulticastPort, NULL, NULL},
    {"MulticastSize", MulticastSize_, *handleMulticastSize, NULL, NULL},
    {"Server", Server_, *handleServer, NULL, NULL},
    {"InterfaceAddress", InterfaceAddress_, *handleInterfaceAddress, NULL, NULL},
    {"ConfigMode", ConfigMode_, *handleConfigMode, NULL, NULL},
    {"ClientOff", ClientOff_, *handleClientOff, NULL, NULL},
    {"ServerOff", ServerOff_, *handleServerOff, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

JXTA_DECLARE(Jxta_status)
    jxta_TCPTransportAdvertisement_get_xml(Jxta_TCPTransportAdvertisement * ad, JString ** result)
{

    char addr_buf[INET_ADDRSTRLEN] = { 0 };
    char port[11] = { 0 };
    char *tmp;

    JString *string = jstring_new_0();

    jstring_append_2(string,
                     "<jxta:TransportAdvertisement xmlns:jxta=\"http://jxta.org\" type=\"jxta:TCPTransportAdvertisement\">\n");

    jstring_append_2(string, "<Protocol>");
    tmp = jxta_TCPTransportAdvertisement_get_Protocol_string((Jxta_advertisement *) ad);
    jstring_append_2(string, tmp);
    free(tmp);
    jstring_append_2(string, "</Protocol>\n");

    jstring_append_2(string, "<Port>");
    apr_snprintf(port, sizeof(port), "%d", ad->Port);
    jstring_append_2(string, port);
    jstring_append_2(string, "</Port>\n");


    if (ad->MulticastOff) {
        jstring_append_2(string, "<MulticastOff/>");
    }

    jstring_append_2(string, "<MulticastAddr>");
    jpr_inet_ntop(AF_INET, &(ad->MulticastAddr), addr_buf, INET_ADDRSTRLEN);
    jstring_append_2(string, addr_buf);
    jstring_append_2(string, "</MulticastAddr>\n");

    jstring_append_2(string, "<MulticastPort>");
    apr_snprintf(port, sizeof(port), "%d", ad->MulticastPort);
    jstring_append_2(string, port);
    jstring_append_2(string, "</MulticastPort>\n");


    jstring_append_2(string, "<MulticastSize>");
    apr_snprintf(port, sizeof(port), "%d", ad->MulticastSize);
    jstring_append_2(string, port);
    jstring_append_2(string, "</MulticastSize>\n");

    jstring_append_2(string, "<InterfaceAddress>");
    jstring_append_2(string, ad->InterfaceAddress ? ad->InterfaceAddress : APR_ANYADDR);
    jstring_append_2(string, "</InterfaceAddress>\n");
    
    if (ad->Server) {
        jstring_append_2(string, "<Server>");
        jstring_append_1(string, ad->Server);
        jstring_append_2(string, "</Server>\n");
    }

    jstring_append_2(string, "<ConfigMode>");
    jstring_append_1(string, ad->ConfigMode);
    jstring_append_2(string, "</ConfigMode>\n");

    if (ad->ServerOff) {
        jstring_append_2(string, "<ServerOff/>\n");
    }

    if (ad->ClientOff) {
        jstring_append_2(string, "<ClientOff/>\n");
    }

    jstring_append_2(string, "</jxta:TransportAdvertisement>\n");

    *result = string;
    return JXTA_SUCCESS;
}

static Jxta_TCPTransportAdvertisement *jxta_TCPTransportAdvertisement_construct(Jxta_TCPTransportAdvertisement * self)
{
    self = (Jxta_TCPTransportAdvertisement *)
        jxta_advertisement_construct((Jxta_advertisement *) self,
                                     "jxta:TCPTransportAdvertisement",
                                     Jxta_TCPTransportAdvertisement_tags,
                                     (JxtaAdvertisementGetXMLFunc) jxta_TCPTransportAdvertisement_get_xml,
                                     (JxtaAdvertisementGetIDFunc) NULL,
                                     (JxtaAdvertisementGetIndexFunc) jxta_TCPTransportAdvertisement_get_indexes);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->Protocol = jstring_new_2("tcp");
        self->ConfigMode = jstring_new_0();
        self->Server = jstring_new_0();
        self->MulticastOff = FALSE;
        self->ClientOff = FALSE;
        self->ServerOff = FALSE;
        self->PublicAddressOnly = FALSE;
        self->InterfaceAddress = NULL;
    }

    return self;
}

static void jxta_TCPTransportAdvertisement_destruct(Jxta_TCPTransportAdvertisement * self)
{
    JXTA_OBJECT_RELEASE(self->Protocol);
    JXTA_OBJECT_RELEASE(self->ConfigMode);
    JXTA_OBJECT_RELEASE(self->Server);
	free(self->InterfaceAddress);

    jxta_advertisement_destruct((Jxta_advertisement *) self);
}

/** 
    Get a new instance of the ad. 
 */
JXTA_DECLARE(Jxta_TCPTransportAdvertisement *) jxta_TCPTransportAdvertisement_new(void)
{
    Jxta_TCPTransportAdvertisement *ad = (Jxta_TCPTransportAdvertisement *) calloc(1, sizeof(Jxta_TCPTransportAdvertisement));;

    JXTA_OBJECT_INIT(ad, jxta_TCPTransportAdvertisement_delete, 0);

    return jxta_TCPTransportAdvertisement_construct(ad);
}

static void jxta_TCPTransportAdvertisement_delete(Jxta_object * obj)
{
    jxta_TCPTransportAdvertisement_destruct((Jxta_TCPTransportAdvertisement *) obj);

    memset(obj, 0xdd, sizeof(Jxta_TCPTransportAdvertisement));
    free(obj);
}

JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_parse_charbuffer(Jxta_TCPTransportAdvertisement * ad, const char *buf, int len)
{
    jxta_advertisement_parse_charbuffer((Jxta_advertisement *) ad, buf, len);
}

JXTA_DECLARE(void) jxta_TCPTransportAdvertisement_parse_file(Jxta_TCPTransportAdvertisement * ad, FILE * stream)
{
    jxta_advertisement_parse_file((Jxta_advertisement *) ad, stream);
}

JXTA_DECLARE(Jxta_vector *)
    jxta_TCPTransportAdvertisement_get_indexes(Jxta_advertisement * dummy)
{
    const char *idx[][2] = { {NULL, NULL} };
    return jxta_advertisement_return_indexes(idx[0]);
}

/* vim: set ts=4 sw=4 et tw=130: */
