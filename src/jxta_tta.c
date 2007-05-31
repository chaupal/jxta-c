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
 * $Id: jxta_tta.c,v 1.38 2005/02/24 22:51:17 slowhog Exp $
 */

   
/* 
* The following command will compile the output from the script 
* given the apr is installed correctly.
*/
/*
* gcc -DSTANDALONE jxta_advertisement.c Jxta_TCPTransportAdvertisement.c  -o PA \
  `/usr/local/apache2/bin/apr-config --cflags --includes --libs` \
  -lexpat -L/usr/local/apache2/lib/ -lapr
*/
 
#include <stdio.h>
#include <string.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_advertisement.h"
#include "jxta_tta.h"
#include "jxta_xml_util.h"

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <apr_want.h>
#include <apr_network_io.h>
#include "jpr/jpr_apr_wrapper.h"
 
#ifdef __cplusplus
extern "C" {
#endif

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
 
 
/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_TCPTransportAdvertisement {
   Jxta_advertisement jxta_advertisement;
   char    * jxta_TCPTransportAdvertisement;
   JString * Protocol;
   Jxta_port Port;
   Jxta_boolean MulticastOff;
   Jxta_in_addr MulticastAddr;
   Jxta_port MulticastPort;
   int       MulticastSize;
   JString * Server;
   Jxta_in_addr InterfaceAddress;
   JString * ConfigMode;
   Jxta_boolean ServerOff;
   Jxta_boolean ClientOff;
   Jxta_boolean PublicAddressOnly; /* not implemented yet..*/
};
 
    /* Forw decl. of un-exported function */
static void jxta_TCPTransportAdvertisement_delete(Jxta_TCPTransportAdvertisement *);
 
/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name.
 */
static void
handleJxta_TCPTransportAdvertisement(void * userdata, const XML_Char * cd, int len) {
    /* Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; */
}
 
static void
handleProtocol(void * userdata, const XML_Char * cd, int len) {
   JString *proto;
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; 

   if(len == 0) return;

   proto = jstring_new_1(len);
   jstring_append_0(proto, cd, len);
   jstring_trim(proto);

   jxta_TCPTransportAdvertisement_set_Protocol(ad, proto);
   JXTA_OBJECT_RELEASE(proto);
}

 
static void
handlePort(void * userdata, const XML_Char * cd, int len) {

   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; 
   extract_port(cd, len, &ad->Port);
}
 
static void
handleMulticastOff(void * userdata, const XML_Char * cd, int len)
{
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;

   if (len == 0) return;

    /* else the tag is present. That's all that counts. */
    ad->MulticastOff = TRUE;
}

static void
handleMulticastAddr(void * userdata, const XML_Char * cd, int len)
{
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; 
   Jxta_in_addr address = 0;

   extract_ip(cd,len,&address);
   if (address != 0) {
     ad->MulticastAddr = address;
   }
}
 
static void
handleMulticastPort(void * userdata, const XML_Char * cd, int len) {
    
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;
   extract_port(cd, len, &ad->MulticastPort);
   JXTA_LOG("TTA: In MulticastPort element\n");
}
 
static void
handleMulticastSize(void * userdata, const XML_Char * cd, int len) {
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; 
   ad->MulticastSize = atoi(cd);
}
 
static void
handleServer(void * userdata, const XML_Char * cd, int len) {
   JString *server;
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;

   if (len == 0) return;

   server = jstring_new_1(len);
   jstring_append_0(server, cd, len);
   jstring_trim(server);
   
   jxta_TCPTransportAdvertisement_set_Server(ad, server);
   JXTA_OBJECT_RELEASE(server);
}
 
static void
handleInterfaceAddress(void * userdata, const XML_Char * cd, int len) {

   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata; 
   Jxta_in_addr address = 0;

   extract_ip(cd,len,&address);
   if (address != 0) {
     ad->InterfaceAddress = address;
   }
}
 
static void
handleConfigMode(void * userdata, const XML_Char * cd, int len) {

   JString *mode;
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;

    if (len == 0) return;

    mode = jstring_new_1(len);
    jstring_append_0(mode, cd, len);
    jstring_trim(mode);
   
    jxta_TCPTransportAdvertisement_set_ConfigMode(ad, mode);
    JXTA_OBJECT_RELEASE(mode);
}

static void
handleServerOff(void * userdata, const XML_Char * cd, int len)
{
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;

   if (len == 0) return;

    /* else the tag is present. That's all that counts. */
    ad->ServerOff = TRUE;
}

static void
handleClientOff(void * userdata, const XML_Char * cd, int len)
{
   Jxta_TCPTransportAdvertisement * ad = (Jxta_TCPTransportAdvertisement*)userdata;

   if (len == 0) return;

    /* else the tag is present. That's all that counts. */
    ad->ClientOff = TRUE;
}
 
/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
char *
jxta_TCPTransportAdvertisement_get_Jxta_TCPTransportAdvertisement(Jxta_TCPTransportAdvertisement * ad) {
   return NULL;
}

 
void
jxta_TCPTransportAdvertisement_set_Jxta_TCPTransportAdvertisement(Jxta_TCPTransportAdvertisement * ad, char * name) {
  
}

 
JString *
jxta_TCPTransportAdvertisement_get_Protocol(Jxta_TCPTransportAdvertisement * ad) {
    JXTA_OBJECT_SHARE(ad->Protocol);
    return ad->Protocol;
}

char *
jxta_TCPTransportAdvertisement_get_Protocol_string(Jxta_advertisement *ad) {
	return strdup(jstring_get_string(((Jxta_TCPTransportAdvertisement*)ad)->Protocol));
}
 
void
jxta_TCPTransportAdvertisement_set_Protocol(Jxta_TCPTransportAdvertisement * ad, JString * protocol) {
	JXTA_OBJECT_SHARE(protocol);
	JXTA_OBJECT_RELEASE(ad->Protocol);
	ad->Protocol = protocol;
}

 
Jxta_port
jxta_TCPTransportAdvertisement_get_Port(Jxta_TCPTransportAdvertisement * ad) {
   return ad->Port;
}


char*
jxta_TCPTransportAdvertisement_get_Port_string(Jxta_advertisement * ad) {

    char* s = malloc(11); /* That's what it may take to print an int */
    sprintf(s,
    "%d",
    jxta_TCPTransportAdvertisement_get_Port((Jxta_TCPTransportAdvertisement*) ad));
   JXTA_LOG("port from get_port_string: %s\n",s);

   /*
    * The semantics of all other xx_string adv methods is to return a malloc'ed
    * strinf.
    */
   return s;
}
 

void
jxta_TCPTransportAdvertisement_set_Port(Jxta_TCPTransportAdvertisement * ad, Jxta_port port) {
    ad->Port = port;
}

Jxta_boolean
jxta_TCPTransportAdvertisement_get_MulticastOff(Jxta_TCPTransportAdvertisement * ad) {
 return ad->MulticastOff;
}

void
jxta_TCPTransportAdvertisement_set_MulticastOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff) {
 ad->MulticastOff = isOff;
}
 
Jxta_in_addr
jxta_TCPTransportAdvertisement_get_MulticastAddr(Jxta_TCPTransportAdvertisement * ad) {

   return ad->MulticastAddr;
}

 
void
jxta_TCPTransportAdvertisement_set_MulticastAddr(Jxta_TCPTransportAdvertisement * ad, Jxta_in_addr addr) {
    ad->MulticastAddr = addr;
}

 
Jxta_port
jxta_TCPTransportAdvertisement_get_MulticastPort(Jxta_TCPTransportAdvertisement * ad) {
    return ad->MulticastPort;
}
 

void
jxta_TCPTransportAdvertisement_set_MulticastPort(Jxta_TCPTransportAdvertisement * ad, Jxta_port port) {
    ad->MulticastPort = port;
}
 
int
jxta_TCPTransportAdvertisement_get_MulticastSize(Jxta_TCPTransportAdvertisement * ad) {
  return ad->MulticastSize;
}
 
void
jxta_TCPTransportAdvertisement_set_MulticastSize(Jxta_TCPTransportAdvertisement * ad, int size) {
  ad->MulticastSize = size;
}
 
JString *
jxta_TCPTransportAdvertisement_get_Server(Jxta_TCPTransportAdvertisement * ad) {
    JXTA_OBJECT_SHARE(ad->Server);
    return ad->Server;
}

char *
jxta_TCPTransportAdvertisement_get_Server_string(Jxta_TCPTransportAdvertisement * ad) {
    return strdup(jstring_get_string(
                ((Jxta_TCPTransportAdvertisement*) ad)->Server));
}
 
void
jxta_TCPTransportAdvertisement_set_Server(Jxta_TCPTransportAdvertisement * ad, JString * server) {
    JXTA_OBJECT_SHARE(server);
    if(ad->Server != NULL)
	    JXTA_OBJECT_RELEASE(ad->Server);
    ad->Server = server;
}
 
Jxta_in_addr
jxta_TCPTransportAdvertisement_get_InterfaceAddress(Jxta_TCPTransportAdvertisement * ad) {
   return ad->InterfaceAddress;
}
 
void
jxta_TCPTransportAdvertisement_set_InterfaceAddress(Jxta_TCPTransportAdvertisement * ad, Jxta_in_addr addr) {
    ad->InterfaceAddress = addr;
 
}
 
JString *
jxta_TCPTransportAdvertisement_get_ConfigMode(Jxta_TCPTransportAdvertisement * ad) {
    JXTA_OBJECT_SHARE(ad->ConfigMode);
    return ad->ConfigMode;
}

char *
jxta_TCPTransportAdvertisement_get_ConfigMode_string(Jxta_TCPTransportAdvertisement * ad) {
    return strdup(jstring_get_string(
                ((Jxta_TCPTransportAdvertisement*) ad)->ConfigMode));
}
 
void
jxta_TCPTransportAdvertisement_set_ConfigMode(Jxta_TCPTransportAdvertisement * ad, JString * mode) {
    JXTA_OBJECT_SHARE(mode);
    if(ad->ConfigMode != NULL)
	    JXTA_OBJECT_RELEASE(ad->ConfigMode);
    ad->ConfigMode = mode;
}

Jxta_boolean
jxta_TCPTransportAdvertisement_get_ServerOff(Jxta_TCPTransportAdvertisement * ad) {
 return ad->ServerOff;
}

void
jxta_TCPTransportAdvertisement_set_ServerOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff) {
 ad->ServerOff = isOff;
}

Jxta_boolean
jxta_TCPTransportAdvertisement_get_ClientOff(Jxta_TCPTransportAdvertisement * ad) {
 return ad->ClientOff;
}

void
jxta_TCPTransportAdvertisement_set_ClientOff(Jxta_TCPTransportAdvertisement * ad, Jxta_boolean isOff) {
 ad->ClientOff = isOff;
}
 
/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
const static Kwdtab Jxta_TCPTransportAdvertisement_tags[] = {
{"Null",                          Null_,                           NULL,                                NULL},
{"jxta:TCPTransportAdvertisement",Jxta_TCPTransportAdvertisement_,*handleJxta_TCPTransportAdvertisement,NULL},
{"Protocol",                      Protocol_,                      *handleProtocol,                      NULL},
{"Port",                          Port_,                          *handlePort,                          NULL},
{"MulticastOff",                  MulticastOff_,                  *handleMulticastOff,                  NULL},
{"MulticastAddr",                 MulticastAddr_,                 *handleMulticastAddr,                 NULL},
{"MulticastPort",                 MulticastPort_,                 *handleMulticastPort,                 NULL},
{"MulticastSize",                 MulticastSize_,                 *handleMulticastSize,                 NULL},
{"Server",                        Server_,                        *handleServer,                        NULL},
{"InterfaceAddress",              InterfaceAddress_,              *handleInterfaceAddress,              NULL},
{"ConfigMode",                    ConfigMode_,                    *handleConfigMode,                    NULL},
{"ClientOff",                     ClientOff_,                     *handleClientOff,                     NULL},
{"ServerOff",                     ServerOff_,                     *handleServerOff,                     NULL},
{NULL,                            0,                               0,                                   NULL}
};

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

Jxta_status
jxta_TCPTransportAdvertisement_get_xml(Jxta_TCPTransportAdvertisement * ad,
        JString ** result) {

   char addr_buf[INET_ADDRSTRLEN] = {0};
   char port[11] = {0};

   JString* string = jstring_new_0();

   jstring_append_2(string,"<jxta:TransportAdvertisement xmlns:jxta=\"http://jxta.org\" type=\"jxta:TCPTransportAdvertisement\">\n");
   
   jstring_append_2(string,"<Protocol>");
   jstring_append_2(string,jxta_TCPTransportAdvertisement_get_Protocol_string((Jxta_advertisement *) ad));
   jstring_append_2(string,"</Protocol>\n");

   jstring_append_2(string,"<Port>");
   snprintf(port, sizeof(port),"%d", ad->Port);
   jstring_append_2(string,port);
   jstring_append_2(string,"</Port>\n");
   

   if( ad->MulticastOff ) {
      jstring_append_2(string, "<MulticastOff>");
      jstring_append_2(string, "</MulticastOff>\n");
   }

   jstring_append_2(string,"<MulticastAddr>");   
   jpr_inet_ntop(AF_INET, &(ad->MulticastAddr), addr_buf, INET_ADDRSTRLEN);
   jstring_append_2(string,addr_buf);
   jstring_append_2(string,"</MulticastAddr>\n");

   
   jstring_append_2(string,"<MulticastPort>");
   snprintf(port, sizeof(port),"%d", ad->MulticastPort);
   jstring_append_2(string,port);
   jstring_append_2(string,"</MulticastPort>\n");


   jstring_append_2(string,"<MulticastSize>");
   snprintf(port, sizeof(port),"%d", ad->MulticastSize);
   jstring_append_2(string,port);
   jstring_append_2(string,"</MulticastSize>\n");


   jstring_append_2(string,"<InterfaceAddress>");
   jpr_inet_ntop(AF_INET, &(ad->InterfaceAddress), addr_buf, INET_ADDRSTRLEN);
   jstring_append_2(string,addr_buf);

   jstring_append_2(string,"</InterfaceAddress>\n");

  
   jstring_append_2(string,"<ConfigMode>");
   jstring_append_1(string,ad->ConfigMode);
   jstring_append_2(string,"</ConfigMode>\n");

   if( ad->ServerOff ) {
      jstring_append_2(string,"<ServerOff");
      jstring_append_2(string,"</ServerOff>\n");
   }

   if( ad->ClientOff ) {
      jstring_append_2(string,"<ClientOff");
      jstring_append_2(string,"</ClientOff>\n");
   }

   jstring_append_2(string,"</jxta:TransportAdvertisement>\n");

   *result = string;
   return JXTA_SUCCESS;

}



/** Get a new instance of the ad. 
 * The memory gets shredded going in to 
 * a value that is easy to see in a debugger,
 * just in case there is a segfault (not that 
 * that would ever happen, but in case it ever did.)
 */
Jxta_TCPTransportAdvertisement *
jxta_TCPTransportAdvertisement_new(void) {

  Jxta_TCPTransportAdvertisement * ad;
  /*
  char * attr, flags;
  char ** tas = attrs;
  int len = 0;
  */

  ad = (Jxta_TCPTransportAdvertisement *) malloc (sizeof (Jxta_TCPTransportAdvertisement));
  memset (ad, 0xda, sizeof (Jxta_TCPTransportAdvertisement));

  ad->Protocol = jstring_new_0();
  ad->ConfigMode = jstring_new_0();
  ad->Server = jstring_new_0();
  ad->MulticastOff = FALSE;
  ad->ClientOff = FALSE;
  ad->ServerOff = FALSE;
  ad->PublicAddressOnly = FALSE;

  /*
  JXTA_OBJECT_INIT(ad, jxta_TCPTransportAdvertisement_delete, 0);
  */

  jxta_advertisement_initialize((Jxta_advertisement*)ad,
    "jxta:TCPTransportAdvertisement",
    Jxta_TCPTransportAdvertisement_tags,
    (JxtaAdvertisementGetXMLFunc)jxta_TCPTransportAdvertisement_get_xml,
    NULL,
    (JxtaAdvertisementGetIndexFunc)jxta_TCPTransportAdvertisement_get_indexes,
    (FreeFunc)jxta_TCPTransportAdvertisement_delete);


 /* Fill in the required initialization code here. */

  return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void
jxta_TCPTransportAdvertisement_delete (Jxta_TCPTransportAdvertisement * ad) {

  JXTA_OBJECT_RELEASE(ad->Protocol);
  JXTA_OBJECT_RELEASE(ad->ConfigMode);

  memset (ad, 0xdd, sizeof (Jxta_TCPTransportAdvertisement));
  free (ad);
}

void 
jxta_TCPTransportAdvertisement_parse_charbuffer(Jxta_TCPTransportAdvertisement * ad, const char * buf, int len) {

  jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
}


  
void 
jxta_TCPTransportAdvertisement_parse_file(Jxta_TCPTransportAdvertisement * ad, FILE * stream) {

   jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
}

Jxta_vector * 
jxta_TCPTransportAdvertisement_get_indexes(void) {
        const char * idx[][2] = { { NULL, NULL } };
    return jxta_advertisement_return_indexes(idx);
}

#ifdef STANDALONE
int
main (int argc, char **argv) {
   Jxta_TCPTransportAdvertisement * ad;
   FILE *testfile;
   JString *jstr = jstring_new_0();
   unsigned long inet_addr = 0;

   if(argc != 2)
     {
       printf("usage: ad <filename>\n");
       return -1;
     }

   apr_app_initialize(&argc, &argv, NULL);

   ad = jxta_TCPTransportAdvertisement_new();

   testfile = fopen (argv[1], "r");
   jxta_TCPTransportAdvertisement_parse_file(ad, testfile);
   fclose(testfile);

   /* jxta_HTTPTransportAdvertisement_print_xml(ad,fprintf,stdout); */
   jxta_HTTPTransportAdvertisement_delete(ad);

   return 0;
}
#endif


#ifdef __cplusplus
}
#endif

