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
 * $Id: jxta_svc.c,v 1.38 2005/04/03 01:47:57 bondolo Exp $
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jxta_svc.h"
#include "jxta_tta.h"
#include "jxta_hta.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_relaya.h"
#include  "jxta_routea.h"
#include "jxta_xml_util.h"


#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#include <apr_want.h>
#include <apr_network_io.h>
 
#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

 
/** Each of these corresponds to a tag in the 
 * xml ad.
 */
enum tokentype {
                Null_,
                Svc_,
                MCID_,
                Parm_,
                RootCert_,
                Addr_,
                Rdv_,
                TCPTransportAdvertisement_,
                HTTPTransportAdvertisement_,
		RelayAdvertisement_,
		RouteAdvertisement_,
                RdvConfigAdvertisement_,
		isClient_,
		isServer_,
		httpaddress_,
		tcpaddress_
};
 
 
/** This is the representation of the 
 * actual ad in the code.  It should
 * stay opaque to the programmer, and be 
 * accessed through the get/set API.
 */
struct _jxta_svc {
  Jxta_advertisement jxta_advertisement;
  char                       * Svc;
  Jxta_id                    * MCID;
  Jxta_vector                * addrlist;
  Jxta_boolean               rdv;
  JString                    * RootCert;
  Jxta_TCPTransportAdvertisement  * tta;
  Jxta_HTTPTransportAdvertisement * hta;
  Jxta_RelayAdvertisement * relaya;
  Jxta_RouteAdvertisement *route;
  Jxta_RdvConfigAdvertisement *rdvConfig;
};
 
/* forward decl for un-exported function */
static void jxta_svc_delete(Jxta_svc *);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name. This is the type of the entire sub-element. The begin
 * tag has been seen in the enclosing element. Here we see only the end tag
 * (if we ever see it...normally not)
 */
static void
handleSvc(void * userdata, const XML_Char * cd, int len)
{
   /* Jxta_svc * ad = (Jxta_svc*)userdata; */
   JXTA_LOG("End Svc element\n");
}

 
static void
handleMCID(void * userdata, const XML_Char * cd, int len)
{
  JString * tmp;
  Jxta_id * mcid = NULL;

  Jxta_svc * ad = (Jxta_svc*)userdata;
  
  if (len == 0) return;

  /* Make room for a final \0 in advance. fromString calls get_string
   * which adds a \0 at the end.
   */
  tmp = jstring_new_1(len + 1);

  jstring_append_0(tmp,(char*)cd,len);
  jstring_trim(tmp);

  jxta_id_from_jstring( &mcid, tmp);
  JXTA_OBJECT_RELEASE(tmp);

  if (mcid != NULL) {
      jxta_svc_set_MCID(ad, mcid);
      JXTA_OBJECT_RELEASE(mcid);
  }

  JXTA_LOG("Done MCID element\n");
}


/* There really isn't any reason to 1. have this tag, or 2. 
 * handle it here, which is why it was originally ignored.
 */ 
static void
handleParm(void * userdata, const XML_Char * cd, int len)
{
   /* Jxta_svc * ad = (Jxta_svc*)userdata; */
   JXTA_LOG("Seen Parm or /Parm tag\n");

   /* When we get the start and end tags on that one, we pretend it's just
    * a text element while in reality it is a complex subelement.
    * Since we have no text value in addition to sub-elements, it does not
    * matter; it just looks like an empty text element. The only impact is
    * that the jstring that serves to accumulate text values of elements
    * is reset twice in a row before this element ends: once at the
    * end of the last subelement, and once at the end of this element.
    * Breaks nothing. Only possible trbl: if there's an MCID inside the
    * Parm element, then it will be mistaken for a second occurence of
    * MCID at the top level. Incorrect but not a concern for now.
    */
}

static void
handleRootCert(void * userdata, const XML_Char * cd, int len)
{
  Jxta_svc * ad = (Jxta_svc*)userdata;
  JString* cert;
  if (len == 0) return; /* start tag or empty data. */

  cert = jstring_new_1(len + 1);
  jstring_append_0(cert, (char *) cd, len);
  jstring_trim(cert);
  jxta_svc_set_RootCert(ad, cert);
  JXTA_OBJECT_RELEASE(cert);

  JXTA_LOG("Done RootCert element\n");
}

static void
handleAddr(void * userdata, const XML_Char * cd, int len)
{
    JString* addr;
    Jxta_svc * ad = (Jxta_svc*)userdata;

    if (len == 0) return;

    /*
     * This tag can only be handled properly with the JXTA_ADV_PAIRED_CALLS
     * option. We depend on it. There must be exactly one non-empty call
     * per address.
     */

    addr = jstring_new_1(len);
    jstring_append_0(addr, cd, len);
    jstring_trim(addr);

    if (jstring_length(addr) == 0) return;

    if (ad->addrlist == NULL) {
	ad->addrlist = jxta_vector_new(4);
    }
    jxta_vector_add_object_last(ad->addrlist, (Jxta_object*) addr);
    JXTA_OBJECT_RELEASE(addr);

    JXTA_LOG("Done Addr element\n");
}
 
static void
handleRdv(void * userdata, const XML_Char * cd, int len)
{
    const char* p;
    Jxta_svc* ad = (Jxta_svc*)userdata;
    if (len == 0) return;

    /*
     * else the tag is present. May be it is explicitly set to false,
     * implicit value is true.
     * The checking for FALSE may look weird but it is quite more
     * economical than than implementing stncasecmp (non-posix) just for
     * that.
     */
    p = (const char*) cd;
    while (isspace(*p)) { ++p; --len; }

    if (   ((len-- > 0) && (toupper(*p++) == 'F'))
	&& ((len-- > 0) && (toupper(*p++) == 'A'))
	&& ((len-- > 0) && (toupper(*p++) == 'L'))
	&& ((len-- > 0) && (toupper(*p++) == 'S'))
        && ((len-- > 0) && (toupper(*p++) == 'E'))) {

	ad->rdv = FALSE;
    } else {
	ad->rdv = TRUE;
    }
}


static void
handleHTTPTransportAdvertisement(void * userdata, const XML_Char * cd, int len)
{

   Jxta_svc * ad = (Jxta_svc*)userdata; 
   Jxta_HTTPTransportAdvertisement * hta =
       jxta_HTTPTransportAdvertisement_new();

   jxta_svc_set_HTTPTransportAdvertisement(ad, hta);
   JXTA_OBJECT_RELEASE(hta);

   JXTA_LOG("Begin Svc handleHTTPTransportAdvertisement"
    " (end may not show)\n");

   jxta_advertisement_set_handlers((Jxta_advertisement *)hta,
		                   ((Jxta_advertisement *)ad)->parser,
		                   (void*)ad);
} 


static void
handleRouteAdvertisement(void * userdata, const XML_Char * cd, int len)
{

   Jxta_svc * ad = (Jxta_svc*)userdata; 
   Jxta_RouteAdvertisement * ra =
       jxta_RouteAdvertisement_new();

   jxta_svc_set_RouteAdvertisement(ad, ra);
   JXTA_OBJECT_RELEASE(ra);

   JXTA_LOG("Begin Svc handleRouteAdvertisement"
    " (end may not show)\n");

   jxta_advertisement_set_handlers((Jxta_advertisement *)ra,
		                   ((Jxta_advertisement *)ad)->parser,
		                   (void*)ad);
} 


static void
handleTCPTransportAdvertisement(void * userdata, const XML_Char * cd, int len) {
   Jxta_svc * ad = (Jxta_svc*)userdata; 
   Jxta_TCPTransportAdvertisement * tta = jxta_TCPTransportAdvertisement_new();

   jxta_svc_set_TCPTransportAdvertisement(ad, tta);
   JXTA_OBJECT_RELEASE(tta);

   JXTA_LOG("Begin Svc handleTCPTransportAdvertisement"
	    " (end tag may not show)\n");


   jxta_advertisement_set_handlers((Jxta_advertisement *)tta,
		                   ((Jxta_advertisement *)ad)->parser,
		                   (void*)ad);

} 

static void
handleRdvConfigAdv(void * userdata, const XML_Char * cd, int len) {
   Jxta_svc * ad = (Jxta_svc*)userdata; 
   Jxta_RdvConfigAdvertisement * rdvConfig = jxta_RdvConfigAdvertisement_new();

   jxta_svc_set_RdvConfig(ad, rdvConfig);
   JXTA_OBJECT_RELEASE(rdvConfig);

   JXTA_LOG("Begin Svc handleRdvConfig"
	    " (end tag may not show)\n");

   jxta_advertisement_set_handlers((Jxta_advertisement *)rdvConfig,
		                   ((Jxta_advertisement *)ad)->parser,
		                   (void*)ad);
   
   ((Jxta_advertisement *) rdvConfig)->atts = ((Jxta_advertisement *) ad)->atts;
   handleJxta_RdvConfigAdvertisement( rdvConfig, cd, len );
} 

static void
handleRelayAdvertisement(void * userdata, const XML_Char * cd, int len) {
   Jxta_svc * ad = (Jxta_svc*)userdata; 

   JXTA_LOG("Begin Svc handleRelayAdvertisement"
	    " (end tag may not show)\n");

   jxta_advertisement_set_handlers((Jxta_advertisement *)ad->relaya,
		                   ((Jxta_advertisement *)ad)->parser,
		                   (void*)ad);
} 


/*
 * create a new pointer to a Relay advertisement
 */
void
create_new_relay_advertisement(Jxta_svc * ad) {
  Jxta_vector* http_relays;
  Jxta_vector* tcp_relays;

  ad->relaya = jxta_RelayAdvertisement_new();
  http_relays = jxta_vector_new(0);
  tcp_relays = jxta_vector_new(0);
  jxta_RelayAdvertisement_set_HttpRelay(ad->relaya, http_relays);
  JXTA_OBJECT_RELEASE(http_relays);
  jxta_RelayAdvertisement_set_TcpRelay(ad->relaya, tcp_relays);
  JXTA_OBJECT_RELEASE(tcp_relays);
}

static void
handleHttpRelay(void * userdata, const XML_Char * cd, int len)
{

   JString* relay;
   Jxta_svc * ad = (Jxta_svc*)userdata; 
   Jxta_vector* relays;

   if (len == 0) return;
  
   JXTA_LOG("Http relay element\n");

   if (ad->relaya == NULL) 
     create_new_relay_advertisement(ad);

   relay = jstring_new_1(len);
   jstring_append_0(relay, cd, len);
   jstring_trim(relay);
   
   relays = jxta_RelayAdvertisement_get_HttpRelay(ad->relaya);
   
   if( jstring_length(relay) > 0 ) {
    jxta_vector_add_object_last(relays, (Jxta_object*) relay);
   }
   
   JXTA_OBJECT_RELEASE(relay); relay = NULL;
   JXTA_OBJECT_RELEASE(relays);
}

static void
handleTcpRelay(void * userdata, const XML_Char * cd, int len)
{
   JString* relay;
   Jxta_svc* ad = (Jxta_svc*)userdata; 
   Jxta_vector* relays;

   if (len == 0) return;
  
   JXTA_LOG("TCP relay element\n");

   if (ad->relaya == NULL) 
     create_new_relay_advertisement(ad);
   
   relay = jstring_new_1(len);
   jstring_append_0(relay, cd, len);
   jstring_trim(relay);
   
   relays = jxta_RelayAdvertisement_get_TcpRelay(ad->relaya);

   if( jstring_length(relay) > 0 ) {
     
    jxta_vector_add_object_last(relays, (Jxta_object*) relay);
   }

   JXTA_OBJECT_RELEASE(relay); relay = NULL;
   JXTA_OBJECT_RELEASE(relays);
}

 
static void
handleIsClient(void * userdata, const XML_Char * cd, int len) 
{
    JString* isClient;
    Jxta_svc * ad = (Jxta_svc*)userdata; 
    if (len == 0) return;

    if (ad->relaya == NULL) 
      create_new_relay_advertisement(ad);

    isClient = jstring_new_1(len);
    jstring_append_0(isClient, cd, len);
    jstring_trim(isClient);
    jxta_RelayAdvertisement_set_IsClient(ad->relaya, isClient);
    JXTA_OBJECT_RELEASE(isClient);
}

static void
handleIsServer(void * userdata, const XML_Char * cd, int len) 
{
    JString* isServer;
    Jxta_svc * ad = (Jxta_svc*)userdata; 
    if (len == 0) return;

    if (ad->relaya == NULL) 
      create_new_relay_advertisement(ad);

    isServer = jstring_new_1(len);
    jstring_append_0(isServer, cd, len);
    jstring_trim(isServer);
    jxta_RelayAdvertisement_set_IsServer(ad->relaya, isServer);
    JXTA_OBJECT_RELEASE(isServer);
}

 
/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
char *
jxta_svc_get_Svc(Jxta_svc * ad)
{
   return NULL;
}

 
void
jxta_svc_set_Svc(Jxta_svc * ad, char * name)
{
 
}

 
Jxta_id *
jxta_svc_get_MCID(Jxta_svc * ad)
{
    if (ad->MCID != NULL) JXTA_OBJECT_SHARE(ad->MCID);
    return ad->MCID;
}


char *
jxta_svc_get_MCID_string(Jxta_advertisement * ad)
{
    char* res;
    JString* js = NULL;
    jxta_id_to_jstring( ((Jxta_svc*) ad)->MCID, &js );
    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}
 

void
jxta_svc_set_MCID(Jxta_svc * ad, Jxta_id * id)
{
    if (id != NULL) JXTA_OBJECT_SHARE(id);
    if (ad->MCID != NULL) JXTA_OBJECT_RELEASE(ad->MCID);
    ad->MCID = id;
}

 
char *
jxta_svc_get_Parm(Jxta_svc * ad) {
   return NULL;
}

 
void
jxta_svc_set_Parm(Jxta_svc * ad, char * name) {
 
}
 
Jxta_vector*
jxta_svc_get_Addr(Jxta_svc * ad)
{
    if (ad->addrlist != NULL) JXTA_OBJECT_SHARE(ad->addrlist);
    return ad->addrlist;
}

void
jxta_svc_set_Addr(Jxta_svc * ad, Jxta_vector * addrs)
{
    if (addrs != NULL) JXTA_OBJECT_SHARE(addrs);
    if (ad->addrlist != NULL) JXTA_OBJECT_RELEASE(ad->addrlist);
    ad->addrlist = addrs;
}

JString*
jxta_svc_get_IsClient(Jxta_svc * ad)
{
  JString* val;
  val = jxta_RelayAdvertisement_get_IsClient(ad->relaya);
  if (val != NULL) JXTA_OBJECT_SHARE(val);
    return val;
}

void
jxta_svc_set_IsClient(Jxta_svc * ad, JString * val)
{
  JString* old;
  old = jxta_RelayAdvertisement_get_IsClient(ad->relaya);
  JXTA_OBJECT_RELEASE(old);
  jxta_RelayAdvertisement_set_IsClient(ad->relaya, val);
}

JString*
jxta_svc_get_IsServer(Jxta_svc * ad)
{
  JString* val;
  val = jxta_RelayAdvertisement_get_IsServer(ad->relaya);
  if (val != NULL) JXTA_OBJECT_SHARE(val);
    return val;
}

void
jxta_svc_set_IsServer(Jxta_svc * ad, JString * val)
{
  JString* old;
  old = jxta_RelayAdvertisement_get_IsServer(ad->relaya);
  JXTA_OBJECT_RELEASE(old);
  jxta_RelayAdvertisement_set_IsServer(ad->relaya, val);
}

void
jxta_svc_set_Relay_Tcppaddr(Jxta_svc * ad, Jxta_vector * addrs)
{
    Jxta_vector* old;

    if (addrs != NULL) JXTA_OBJECT_SHARE(addrs);
    old = jxta_RelayAdvertisement_get_TcpRelay(ad->relaya);
    if (old != NULL) JXTA_OBJECT_RELEASE(old);
    jxta_RelayAdvertisement_set_TcpRelay(ad->relaya, addrs);
}

void
jxta_svc_set_Relay_Httpaddr(Jxta_svc * ad, Jxta_vector * addrs)
{
  Jxta_vector* old;

    if (addrs != NULL) JXTA_OBJECT_SHARE(addrs);
    old = jxta_RelayAdvertisement_get_HttpRelay(ad->relaya);
    if (old != NULL) JXTA_OBJECT_RELEASE(old);
    jxta_RelayAdvertisement_set_HttpRelay(ad->relaya, addrs);
}

Jxta_boolean
jxta_svc_get_Rdv(Jxta_svc * ad) {
   return ad->rdv;
}

void
jxta_svc_set_Rdv(Jxta_svc * ad, Jxta_boolean rdv)
{
    ad->rdv = rdv;
}

Jxta_HTTPTransportAdvertisement *
jxta_svc_get_HTTPTransportAdvertisement(Jxta_svc * ad)
{
    if (ad->hta != NULL) JXTA_OBJECT_SHARE(ad->hta);
    return ad->hta;
}

void
jxta_svc_set_HTTPTransportAdvertisement(Jxta_svc * ad,
				   Jxta_HTTPTransportAdvertisement * hta)
{
    if (hta != NULL) JXTA_OBJECT_SHARE(hta);
    if (ad->hta != NULL) JXTA_OBJECT_RELEASE(ad->hta);
    ad->hta = hta;
}

Jxta_RouteAdvertisement *
jxta_svc_get_RouteAdvertisement(Jxta_svc * ad)
{
    if (ad->route != NULL) JXTA_OBJECT_SHARE(ad->route);
    return ad->route;
}

void
jxta_svc_set_RouteAdvertisement(Jxta_svc * ad,
				   Jxta_RouteAdvertisement * ra)
{
    if (ra != NULL) JXTA_OBJECT_SHARE(ra);
    if (ad->route != NULL) JXTA_OBJECT_RELEASE(ad->route);
    ad->route = ra;
}

Jxta_RelayAdvertisement *
jxta_svc_get_RelayAdvertisement(Jxta_svc * ad)
{
    if (ad->relaya != NULL) JXTA_OBJECT_SHARE(ad->relaya);
    return ad->relaya;
}

void
jxta_svc_set_RelayAdvertisement(Jxta_svc * ad,
				   Jxta_RelayAdvertisement * relaya)
{
    if (relaya != NULL) JXTA_OBJECT_SHARE(relaya);
    if (ad->relaya != NULL) JXTA_OBJECT_RELEASE(ad->relaya);
    ad->relaya = relaya;
}

Jxta_TCPTransportAdvertisement *
jxta_svc_get_TCPTransportAdvertisement(Jxta_svc * ad)
{
    if (ad->tta != NULL) JXTA_OBJECT_SHARE(ad->tta);
    return ad->tta;
}

void
jxta_svc_set_TCPTransportAdvertisement(Jxta_svc * ad,
				   Jxta_TCPTransportAdvertisement * tta)
{
    if (tta != NULL) JXTA_OBJECT_SHARE(tta);
    if (ad->tta != NULL) JXTA_OBJECT_RELEASE(ad->tta);
    ad->tta = tta;
}

Jxta_RdvConfigAdvertisement * jxta_svc_get_RdvConfig(Jxta_svc * ad)
{
    if (ad->rdvConfig != NULL) JXTA_OBJECT_SHARE(ad->rdvConfig);
    return ad->rdvConfig;
}

void
jxta_svc_set_RdvConfig(Jxta_svc * ad,
				   Jxta_RdvConfigAdvertisement * rdvConfig)
{
    if (rdvConfig != NULL) JXTA_OBJECT_SHARE(rdvConfig);
    if (ad->rdvConfig != NULL) JXTA_OBJECT_RELEASE(ad->rdvConfig);
    ad->rdvConfig = rdvConfig;
}

JString*
jxta_svc_get_RootCert(Jxta_svc * ad)
{
    if (ad->RootCert != NULL) JXTA_OBJECT_SHARE(ad->RootCert);
    return ad->RootCert;
}

void
jxta_svc_set_RootCert(Jxta_svc * ad, JString* rootcert)
{
    if (rootcert != NULL) JXTA_OBJECT_SHARE(rootcert);
    if (ad->RootCert != NULL) JXTA_OBJECT_RELEASE(ad->RootCert);
    ad->RootCert = rootcert;
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */
const static Kwdtab Svc_tags[] = {
{"Null",                           Null_,                       NULL,                            NULL},
{"Svc",                            Svc_,                       *handleSvc,                       NULL},
{"MCID",                           MCID_,                      *handleMCID,                      NULL},
{"Parm",                           Parm_,                      *handleParm,                      NULL},
{"RootCert",                       RootCert_,                  *handleRootCert,                  NULL},
{"Addr",                           Addr_,                      *handleAddr,                      NULL},
{"Rdv",                            Rdv_,                       *handleRdv,                       NULL},
{"jxta:RdvConfig",                 RdvConfigAdvertisement_,    *handleRdvConfigAdv,              NULL},
{"jxta:TCPTransportAdvertisement", TCPTransportAdvertisement_, *handleTCPTransportAdvertisement, NULL},
{"jxta:HTTPTransportAdvertisement",HTTPTransportAdvertisement_,*handleHTTPTransportAdvertisement,NULL},
{"jxta:RA"                        ,RouteAdvertisement_,*handleRouteAdvertisement,NULL},
{"isClient"                       ,isClient_                  ,*handleIsClient,        NULL},
{"isServer"                       ,isServer_                  ,*handleIsServer,        NULL},
{"httpaddress"                    ,httpaddress_               ,*handleHttpRelay,        NULL},
{"tcpaddress"                     ,tcpaddress_                ,*handleTcpRelay,        NULL},
{NULL,                             0,                           0,                               NULL}
};


/* This is an internal routine; we assume what best fits us in terms
 * of jstring allocation. Here we just assume that we are passed an
 * existing jstring an just append to it.
 */
void
addrs_print(Jxta_svc * ad, JString * js) {

    int i;
    int sz;
    Jxta_vector* addresses = ad->addrlist;
    sz = jxta_vector_size(addresses);

    for (i = 0; i < sz; ++i) {  
	JString* addr;
	jxta_vector_get_object_at(addresses, (Jxta_object**) &addr, i);

	jstring_append_2(js,"<Addr>");
	jstring_append_1(js,addr);
	jstring_append_2(js,"</Addr>\n");
	JXTA_OBJECT_RELEASE(addr);
    }
}

  
Jxta_status
jxta_svc_get_xml(Jxta_svc * ad, JString ** result) {

    JString* ids;
    JString* string = jstring_new_0();

    jstring_append_2(string,"<Svc>\n");

    if (ad->MCID != NULL) {
	jstring_append_2(string,"<MCID>");
	ids = NULL;
	jxta_id_to_jstring( ad->MCID, &ids );
	jstring_append_1(string, ids);
	JXTA_OBJECT_RELEASE(ids);
	jstring_append_2(string,"</MCID>\n");
    }

    jstring_append_2(string,"<Parm>\n");

    if (ad->addrlist != NULL) {
	addrs_print(ad,string);
    }

    if (ad->rdv) {
	jstring_append_2(string, "<Rdv>true</Rdv>\n");
    }

    if(ad->RootCert != NULL) {
	jstring_append_2(string,"<RootCert>\n");
	jstring_append_1(string,ad->RootCert); 
	jstring_append_2(string,"\n</RootCert>\n");
    }

    if(ad->hta != NULL) {
	/* http transport adv is a self standing adv. It's get_xml
	 * method has the std nehaviour; it allocates a new jstring.
	 */
	JString* tmp;
	jxta_HTTPTransportAdvertisement_get_xml(ad->hta, &tmp);
	jstring_append_1(string, tmp);
	JXTA_OBJECT_RELEASE(tmp);
    }
  
    if(ad->route != NULL) {
	/* route adv is a self standing adv. It's get_xml
	 * method has the std nehaviour; it allocates a new jxtring.
	 */
	JString* tmp;
	jxta_RouteAdvertisement_get_xml(ad->route, &tmp);
	jstring_append_1(string, tmp);
	JXTA_OBJECT_RELEASE(tmp);

    }
    if(ad->tta != NULL) {
	/* tcp transport adv is a self standing adv. It's get_xml
	 * method has the std nehaviour; it allocates a new jxtring.
	 */
	JString* tmp;
	jxta_TCPTransportAdvertisement_get_xml(ad->tta, &tmp);
	jstring_append_1(string, tmp);
	JXTA_OBJECT_RELEASE(tmp);
    }

   
    if(ad->relaya != NULL) {
	/* relay transport adv is a self standing adv. It's get_xml
	 * method has the std nehaviour; it allocates a new jxtring.
	 */
	JString* tmp = jstring_new_0();

	jstring_append_2(tmp,"<isClient>");
	jstring_append_1(tmp, jxta_RelayAdvertisement_get_IsClient(ad->relaya));
	jstring_append_2(tmp,"</isClient>\n");
   
	jstring_append_2(tmp,"<isServer>");
	jstring_append_1(tmp, jxta_RelayAdvertisement_get_IsServer(ad->relaya));
	jstring_append_2(tmp,"</isServer>\n");

	httpRelay_printer(ad->relaya,tmp);
	tcpRelay_printer(ad->relaya,tmp);

	jstring_append_1(string, tmp);
	JXTA_OBJECT_RELEASE(tmp);
    }

    jstring_append_2(string,"</Parm>\n");
    jstring_append_2(string,"</Svc>\n");

    *result = string;
    return JXTA_SUCCESS;  
}



/** Get a new instance of the ad. 
 * Svc has a set of possible content. Some of them are
 * bulky, so, as a matter of principle we do not install
 * defaults, we use and test for NULL wherever appropriate.
 */

Jxta_svc *
jxta_svc_new(void) {

  Jxta_svc * ad;

  ad = (Jxta_svc *) malloc (sizeof (Jxta_svc));
  memset (ad, 0x00, sizeof (Jxta_svc));

  jxta_advertisement_initialize((Jxta_advertisement*)ad,
				"Svc",
				Svc_tags,
				(JxtaAdvertisementGetXMLFunc)jxta_svc_get_xml,
				(JxtaAdvertisementGetIDFunc)jxta_svc_get_MCID,
				NULL,
				(FreeFunc)jxta_svc_delete);


 /* Fill in the required initialization code here. */
  ad->rdv = FALSE;
  ad->relaya = NULL;
  return ad;
}

/** Shred the memory going out.  Again,
 * if there ever was a segfault (unlikely,
 * of course), the hex value dddddddd will 
 * pop right out as a piece of memory accessed
 * after it was freed...
 */
static void
jxta_svc_delete (Jxta_svc * ad) {

  if (ad->RootCert != NULL) JXTA_OBJECT_RELEASE(ad->RootCert);
  if (ad->MCID != NULL) JXTA_OBJECT_RELEASE(ad->MCID);
  if (ad->tta != NULL) JXTA_OBJECT_RELEASE(ad->tta);
  if (ad->hta != NULL) JXTA_OBJECT_RELEASE(ad->hta);
  if (ad->addrlist != NULL) JXTA_OBJECT_RELEASE(ad->addrlist);
  if (ad->relaya != NULL) JXTA_OBJECT_RELEASE(ad->relaya);

  jxta_advertisement_delete((Jxta_advertisement*)ad);
  memset (ad, 0xdd, sizeof (Jxta_svc));
  free (ad);
}


void 
jxta_svc_parse_charbuffer(Jxta_svc * ad, const char * buf, int len) {

  jxta_advertisement_parse_charbuffer((Jxta_advertisement*)ad,buf,len);
}


  
void 
jxta_svc_parse_file(Jxta_svc * ad, FILE * stream) {

   jxta_advertisement_parse_file((Jxta_advertisement*)ad, stream);
}


#ifdef __cplusplus
}
#endif
