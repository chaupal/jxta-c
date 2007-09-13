/* 
 * Copyright (c) 2002-2006 Sun Microsystems, Inc.  All rights reserved.
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

static const char *__log_cat = "SVCADV";

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "jxta_apr.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_svc.h"
#include "jxta_tta.h"
#include "jxta_tls_config_adv.h"
#include "jxta_hta.h"
#include "jxta_discovery_config_adv.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_relaya.h"
#include "jxta_routea.h"
#include "jxta_xml_util.h"

/** Each of these corresponds to a tag in the 
 * xml ad.
 */ 
enum tokentype {
    Null_,
    Svc_,
    MCID_,
    Parm_,
    TCPTransportAdvertisement_,
    HTTPTransportAdvertisement_,
    RouteAdvertisement_,
    CacheConfigAdvertisement_,
    DiscoveryConfigAdvertisement_,
    RdvConfigAdvertisement_,
    SrdiConfigAdvertisement_,
    TlsConfigAdvertisement_,
    EndPointConfigAdvertisement_,
    RelayConfigAdvertisement_,
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

    Jxta_id *MCID;
    
    Jxta_advertisement *parm;
    
    JString             *text_parm;

};

/* forward decl for un-exported function */
static void svc_delete(Jxta_object *);
static void handleCacheConfigAdv(void *me, const XML_Char *cd, int len);
static void handleDiscoveryConfigAdv(void *me, const XML_Char * cd, int len);
static void handleRdvConfigAdv(void *me, const XML_Char * cd, int len);
static void handleSrdiConfigAdv(void *me, const XML_Char * cd, int len);
static void handleEndPointConfigAdv(void *me, const XML_Char * cd, int len);
static void handleRelayConfig(void *me, const XML_Char * cd, int len);

static Jxta_status validate_advertisement(Jxta_svc *myself);

/** Handler functions.  Each of these is responsible for 
 * dealing with all of the character data associated with the 
 * tag name. This is the type of the entire sub-element. The begin
 * tag has been seen in the enclosing element. Here we see only the end tag
 * (if we ever see it...normally not)
 */
static void handleSvc(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "START <Svc> : [%pp]\n", ad);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "FINISH <Svc> : [%pp]\n", ad);
    }
}

static void handleMCID(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <MCID> : [%pp]\n", ad);
        return;
    } else {
        JString *tmp = jstring_new_1(len);
        Jxta_id *mcid = NULL;

    jstring_append_0(tmp, (char *) cd, len);
    jstring_trim(tmp);

    jxta_id_from_jstring(&mcid, tmp);
    JXTA_OBJECT_RELEASE(tmp);

    if (mcid != NULL) {
        jxta_svc_set_MCID(ad, mcid);
        JXTA_OBJECT_RELEASE(mcid);
    }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <MCID> : [%pp]\n", ad);
}
}

static void handleParm(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *myself = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(myself);

    if( 0 == len ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <Parm> : [%pp]\n", myself);
    } else {
        if( NULL == myself->parm ) {
            JString *tmp = jstring_new_1(len);

            jstring_append_0(tmp, (char *) cd, len);
            jstring_trim(tmp);

            myself->text_parm = tmp;
    }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <Parm> : [%pp]\n", myself);
    }
}

static void handleHTTPTransportAdvertisement(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        Jxta_HTTPTransportAdvertisement *hta = jxta_HTTPTransportAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:HTTPTransportAdvertisement> Element[%pp]\n", ad );

        jxta_svc_set_HTTPTransportAdvertisement(ad, hta);

        jxta_advertisement_set_handlers((Jxta_advertisement *) hta, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(hta);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:HTTPTransportAdvertisement> Element [%pp]\n", ad );
    }
}

static void handleRouteAdvertisement(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
    Jxta_RouteAdvertisement *ra = jxta_RouteAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:RA> : [%pp]\n", ad);

        jxta_svc_set_RouteAdvertisement(ad, ra);

    jxta_advertisement_set_handlers((Jxta_advertisement *) ra, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(ra);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:RA> : [%pp]\n", ad);
    }
}

static void handleTCPTransportAdvertisement(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (len == 0) {
    Jxta_TCPTransportAdvertisement *tta = jxta_TCPTransportAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:TCPTransportAdvertisement> Element [%pp]\n", ad );

        jxta_svc_set_TCPTransportAdvertisement(ad, tta);

    jxta_advertisement_set_handlers((Jxta_advertisement *) tta, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(tta);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:TCPTransportAdvertisement> Element [%pp]\n", ad );
    }
}
 
static void handleCacheConfigAdv(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        Jxta_CacheConfigAdvertisement *cacheConfig = jxta_CacheConfigAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:CacheConfig> Element [%pp]\n", ad );

        jxta_svc_set_CacheConfig(ad, cacheConfig);

        jxta_advertisement_set_handlers((Jxta_advertisement *) cacheConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(cacheConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:CacheConfig> Element [%pp]\n", ad );
    }
}

static void handleDiscoveryConfigAdv(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        Jxta_DiscoveryConfigAdvertisement *discoveryConfig = jxta_DiscoveryConfigAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:DiscoveryConfig> Element [%pp]\n", ad );

        jxta_svc_set_DiscoveryConfig(ad, discoveryConfig);

        jxta_advertisement_set_handlers((Jxta_advertisement *) discoveryConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(discoveryConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:DiscoveryConfig> Element [%pp]\n", ad );
    }
}

static void handleRdvConfigAdv(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        Jxta_RdvConfigAdvertisement *rdvConfig = jxta_RdvConfigAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:RdvConfig> Element [%pp]\n", ad );

        jxta_svc_set_RdvConfig(ad, rdvConfig);

        jxta_advertisement_set_handlers((Jxta_advertisement *) rdvConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(rdvConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:RdvConfig> Element [%pp]\n", ad );
    }
}

static void handleSrdiConfigAdv(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (len == 0) {
        Jxta_SrdiConfigAdvertisement *srdiConfig = jxta_SrdiConfigAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:SrdiConfig> Element [%pp]\n", ad );

        jxta_svc_set_SrdiConfig(ad, srdiConfig);

        jxta_advertisement_set_handlers((Jxta_advertisement *) srdiConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(srdiConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:SrdiConfig> Element [%pp]\n", ad );
    }
}

static void handleTlsConfigAdv(void *userdata, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) userdata;

    if (len == 0) {
        Jxta_TlsConfigAdvertisement *tlsConfig = jxta_TlsConfigAdvertisement_new();

        jxta_svc_set_TlsConfig(ad, tlsConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Begin Svc handleTlsConfig\n");

        jxta_advertisement_set_handlers((Jxta_advertisement *) tlsConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);

        ((Jxta_advertisement *) tlsConfig)->atts = ((Jxta_advertisement *) ad)->atts;
        handleJxta_TlsConfigAdvertisement(tlsConfig, cd, len);
        JXTA_OBJECT_RELEASE(tlsConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "END Svc handleTlsConfig\n");
    }
}


static void handleEndPointConfigAdv(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if (len == 0) {
    Jxta_EndPointConfigAdvertisement *endpointConfig = jxta_EndPointConfigAdvertisement_new();
        
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:EndPointConfig> Element [%pp]\n", ad );

        jxta_svc_set_EndPointConfig(ad, endpointConfig);

        jxta_advertisement_set_handlers((Jxta_advertisement *) endpointConfig, ((Jxta_advertisement *) ad)->parser, (void *) ad);
        JXTA_OBJECT_RELEASE(endpointConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:EndPointConfig> Element [%pp]\n", ad );
    }
}

static void handleRelayConfig(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        Jxta_RelayAdvertisement * relayConfig = jxta_RelayAdvertisement_new();

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START <jxta:RelayAdvertisement> Element [%pp]\n", ad );

        jxta_svc_set_RelayAdvertisement( ad, relayConfig );
 
        JXTA_OBJECT_RELEASE(relayConfig);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH <jxta:RelayAdvertisement> Element [%pp]\n", ad );
    }
}

static void handleHttpRelay(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START DEPRECATED <httpaddress> Element [%pp]\n", ad );
        return;
    } else {
        JString *relay = jstring_new_1(len);
        Jxta_RelayAdvertisement * relayConfig = jxta_svc_get_RelayAdvertisement( ad );

        if (relayConfig == NULL) {
            relayConfig = jxta_RelayAdvertisement_new();

            jxta_svc_set_RelayAdvertisement( ad, relayConfig );
        }

        jstring_append_0(relay, cd, len);
        jstring_trim(relay);

        jxta_RelayAdvertisement_add_HttpRelay(relayConfig, relay );

        JXTA_OBJECT_RELEASE(relay);
        JXTA_OBJECT_RELEASE(relayConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH DEPRECATED <httpaddress> Element [%pp]\n", ad );
    }
}

static void handleTcpRelay(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START DEPRECATED <tcpaddress> Element [%pp]\n", ad );
        return;
    } else {
        JString *relay = jstring_new_1(len);
        Jxta_RelayAdvertisement * relayConfig = jxta_svc_get_RelayAdvertisement( ad );

        if (relayConfig == NULL) {
            relayConfig = jxta_RelayAdvertisement_new();

            jxta_svc_set_RelayAdvertisement( ad, relayConfig );
        }

        jstring_append_0(relay, cd, len);
        jstring_trim(relay);

        jxta_RelayAdvertisement_add_TcpRelay(relayConfig, relay );

        JXTA_OBJECT_RELEASE(relay);
        JXTA_OBJECT_RELEASE(relayConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH DEPRECATED <tcpaddress> Element [%pp]\n", ad );
    }
}

static void handleIsClient(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START DEPRECATED <isClient> Element [%pp]\n", ad );
        return;
    } else {
        JString *isClient = jstring_new_1(len);
        Jxta_RelayAdvertisement * relayConfig = jxta_svc_get_RelayAdvertisement( ad );

        if (relayConfig == NULL) {
            relayConfig = jxta_RelayAdvertisement_new();
            
            jxta_svc_set_RelayAdvertisement( ad, relayConfig );
        }

    jstring_append_0(isClient, cd, len);
    jstring_trim(isClient);

        jxta_RelayAdvertisement_set_IsClient(relayConfig, isClient );
    JXTA_OBJECT_RELEASE(isClient);
        JXTA_OBJECT_RELEASE(relayConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH DEPRECATED <isClient> Element [%pp]\n", ad );
    }
}

static void handleIsServer(void *me, const XML_Char * cd, int len)
{
    Jxta_svc *ad = (Jxta_svc *) me;
    
    JXTA_OBJECT_CHECK_VALID(ad);

    if (len == 0) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "START DEPRECATED <isServer> Element [%pp]\n", ad );
        return;
    } else {
        JString *isServer = jstring_new_1(len);
        Jxta_RelayAdvertisement * relayConfig = jxta_svc_get_RelayAdvertisement( ad );

        if (relayConfig == NULL) {
            relayConfig = jxta_RelayAdvertisement_new();
            
            jxta_svc_set_RelayAdvertisement( ad, relayConfig );
        }

    jstring_append_0(isServer, cd, len);
    jstring_trim(isServer);
        
        jxta_RelayAdvertisement_set_IsServer(relayConfig, isServer);
    JXTA_OBJECT_RELEASE(isServer);
        JXTA_OBJECT_RELEASE(relayConfig);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_PARANOID, "FINISH DEPRECATED <isServer> Element [%pp]\n", ad );
    }
}


/** The get/set functions represent the public
 * interface to the ad class, that is, the API.
 */
JXTA_DECLARE(Jxta_id *) jxta_svc_get_MCID(Jxta_svc * ad)
{
    if (ad->MCID != NULL)
        JXTA_OBJECT_SHARE(ad->MCID);
    return ad->MCID;
}

char *jxta_svc_get_MCID_string(Jxta_advertisement * ad)
{
    char *res;
    JString *js = NULL;
    jxta_id_to_jstring(((Jxta_svc *) ad)->MCID, &js);
    res = strdup(jstring_get_string(js));
    JXTA_OBJECT_RELEASE(js);
    return res;
}

JXTA_DECLARE(void) jxta_svc_set_MCID(Jxta_svc * ad, Jxta_id * id)
{
    if (id != NULL)
        JXTA_OBJECT_SHARE(id);
    if (ad->MCID != NULL)
        JXTA_OBJECT_RELEASE(ad->MCID);
    ad->MCID = id;
}

JXTA_DECLARE(Jxta_advertisement *) jxta_svc_get_Parm(Jxta_svc * ad)
{
    JXTA_OBJECT_CHECK_VALID(ad);
    
    if( NULL == ad->parm ) {
    return NULL;
    } else {
        return JXTA_OBJECT_SHARE(ad->parm);
    }
}

JXTA_DECLARE(Jxta_advertisement *) jxta_svc_get_Parm_type(Jxta_svc * ad, const char * doctype )
{
    Jxta_advertisement * result = jxta_svc_get_Parm( ad );

    if( NULL != result ) {
        if( 0 != strcmp( result->document_name, doctype ) ) {
            JXTA_OBJECT_RELEASE(result);
            result = NULL;
}
}

    return result;
}



JXTA_DECLARE(void) jxta_svc_set_Parm(Jxta_svc * ad, Jxta_advertisement *parm)
{
    JXTA_OBJECT_CHECK_VALID(ad);

    if( NULL != ad->parm ) {
        JXTA_OBJECT_RELEASE(ad->parm);
        ad->parm = NULL;
}

    if( NULL != parm ) {
        ad->parm = JXTA_OBJECT_SHARE(parm);
    }
}

JXTA_DECLARE(Jxta_HTTPTransportAdvertisement *) jxta_svc_get_HTTPTransportAdvertisement(Jxta_svc * ad)
{
    return (Jxta_HTTPTransportAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:HTTPTransportAdvertisement" );
}

JXTA_DECLARE(void) jxta_svc_set_HTTPTransportAdvertisement(Jxta_svc * ad, Jxta_HTTPTransportAdvertisement * hta)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement*) hta );
}

JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_svc_get_RouteAdvertisement(Jxta_svc * ad)
{
    return (Jxta_RouteAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:RA" );
}

JXTA_DECLARE(void) jxta_svc_set_RouteAdvertisement(Jxta_svc * ad, Jxta_RouteAdvertisement * ra)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement*) ra );
}

JXTA_DECLARE(Jxta_RelayAdvertisement *) jxta_svc_get_RelayAdvertisement(Jxta_svc * ad)
{
    return (Jxta_RelayAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:RelayAdvertisement" );
}

JXTA_DECLARE(void) jxta_svc_set_RelayAdvertisement(Jxta_svc * ad, Jxta_RelayAdvertisement * relaya)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) relaya );
}

JXTA_DECLARE(Jxta_TCPTransportAdvertisement *) jxta_svc_get_TCPTransportAdvertisement(Jxta_svc * ad)
{
    return (Jxta_TCPTransportAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:TCPTransportAdvertisement" );
}

JXTA_DECLARE(void) jxta_svc_set_TCPTransportAdvertisement(Jxta_svc * ad, Jxta_TCPTransportAdvertisement * tta)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) tta );
}

JXTA_DECLARE(Jxta_DiscoveryConfigAdvertisement *) jxta_svc_get_DiscoveryConfig(Jxta_svc * ad)
{
    return (Jxta_DiscoveryConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:DiscoveryConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_DiscoveryConfig(Jxta_svc * ad, Jxta_DiscoveryConfigAdvertisement * discoveryConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) discoveryConfig );
}

JXTA_DECLARE(Jxta_RdvConfigAdvertisement *) jxta_svc_get_RdvConfig(Jxta_svc * ad)
{
    return (Jxta_RdvConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:RdvConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_RdvConfig(Jxta_svc * ad, Jxta_RdvConfigAdvertisement * rdvConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) rdvConfig );
}

JXTA_DECLARE(Jxta_CacheConfigAdvertisement *) jxta_svc_get_CacheConfig(Jxta_svc * ad)
{
    return (Jxta_CacheConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:CacheConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_CacheConfig(Jxta_svc * ad, Jxta_CacheConfigAdvertisement * cacheConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) cacheConfig );
    }


JXTA_DECLARE(Jxta_SrdiConfigAdvertisement *) jxta_svc_get_SrdiConfig(Jxta_svc * ad)
{
    return (Jxta_SrdiConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:SrdiConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_SrdiConfig(Jxta_svc * ad, Jxta_SrdiConfigAdvertisement * srdiConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) srdiConfig );
}

JXTA_DECLARE(Jxta_TlsConfigAdvertisement *) jxta_svc_get_TlsConfig(Jxta_svc * ad)
{
    return (Jxta_TlsConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:TlsConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_TlsConfig(Jxta_svc * ad, Jxta_TlsConfigAdvertisement * tlsConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) tlsConfig );
}

JXTA_DECLARE(Jxta_EndPointConfigAdvertisement *) jxta_svc_get_EndPointConfig(Jxta_svc * ad)
{
    return (Jxta_EndPointConfigAdvertisement*) jxta_svc_get_Parm_type( ad, "jxta:EndPointConfig" );
}

JXTA_DECLARE(void) jxta_svc_set_EndPointConfig(Jxta_svc * ad, Jxta_EndPointConfigAdvertisement * endpointConfig)
{
    jxta_svc_set_Parm( ad, (Jxta_advertisement *) endpointConfig );
    }

JXTA_DECLARE(JString *) jxta_svc_get_RootCert(Jxta_svc * ad)
{
    return NULL;
}

JXTA_DECLARE(void) jxta_svc_set_RootCert(Jxta_svc * ad, JString * rootcert)
{
}

/** Now, build an array of the keyword structs.  Since 
 * a top-level, or null state may be of interest, 
 * let that lead off.  Then, walk through the enums,
 * initializing the struct array with the correct fields.
 * Later, the stream will be dispatched to the handler based
 * on the value in the char * kwd.
 */

static const Kwdtab Svc_tags[] = {
    {"Null", Null_, NULL, NULL, NULL},
    {"Svc", Svc_, *handleSvc, NULL, NULL},
    {"MCID", MCID_, *handleMCID, NULL, NULL},
    {"Parm", Parm_, *handleParm, NULL, NULL},
    
    {"jxta:CacheConfig", CacheConfigAdvertisement_, *handleCacheConfigAdv, NULL, NULL},
    {"jxta:DiscoveryConfig", DiscoveryConfigAdvertisement_, *handleDiscoveryConfigAdv, NULL, NULL},
    {"jxta:EndPointConfig", EndPointConfigAdvertisement_, *handleEndPointConfigAdv, NULL, NULL},
    {"jxta:RdvConfig", RdvConfigAdvertisement_, *handleRdvConfigAdv, NULL, NULL},
    {"jxta:SrdiConfig", SrdiConfigAdvertisement_, *handleSrdiConfigAdv, NULL, NULL},
    {"jxta:TlsConfig", TlsConfigAdvertisement_, *handleTlsConfigAdv, NULL, NULL},
    {"jxta:TCPTransportAdvertisement", TCPTransportAdvertisement_, *handleTCPTransportAdvertisement, NULL, NULL},
    {"jxta:HTTPTransportAdvertisement", HTTPTransportAdvertisement_, *handleHTTPTransportAdvertisement, NULL, NULL},
    {"jxta:RelayAdvertisement", RelayConfigAdvertisement_, *handleRelayConfig, NULL, NULL},
    {"jxta:RA", RouteAdvertisement_, *handleRouteAdvertisement, NULL, NULL},
    
    /* Deprecated : temporarily preserved for backwards compatibility. */
    
    {"isClient", isClient_, *handleIsClient, NULL, NULL},
    {"isServer", isServer_, *handleIsServer, NULL, NULL},
    {"httpaddress", httpaddress_, *handleHttpRelay, NULL, NULL},
    {"tcpaddress", tcpaddress_, *handleTcpRelay, NULL, NULL},
    {NULL, 0, 0, NULL, NULL}
};

static Jxta_status validate_advertisement(Jxta_svc *myself) {

    JXTA_OBJECT_CHECK_VALID(myself);


    if( NULL == myself->MCID ) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "MCID not defined. [%pp]\n", myself);
        return JXTA_INVALID_ARGUMENT;
    }

    return JXTA_SUCCESS;
    }

JXTA_DECLARE(Jxta_status) jxta_svc_get_xml(Jxta_svc * ad, JString ** xml)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *string;
    JString *ids = NULL;

    if (xml == NULL) {
        return JXTA_INVALID_ARGUMENT;
    }

    res = validate_advertisement(ad);
    if( JXTA_SUCCESS != res ) {
         return res;
    }

    string = jstring_new_0();

    jstring_append_2(string, "<Svc>\n");

    jstring_append_2(string, "<MCID>");
    ids = NULL;
    res = jxta_id_to_jstring(ad->MCID, &ids);
    if( JXTA_SUCCESS != res ) {
        JXTA_OBJECT_RELEASE( string );
        return res;
    }
    jstring_append_1(string, ids);
    JXTA_OBJECT_RELEASE(ids);
    jstring_append_2(string, "</MCID>\n");

    jstring_append_2(string, "<Parm>\n");
    
    if (ad->parm != NULL) {
        JString *tmp;
        res = jxta_advertisement_get_xml(ad->parm, &tmp);
        if( JXTA_SUCCESS != res ) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Failure printing param [%pp]\n", ad );
            JXTA_OBJECT_RELEASE( string );
            return res;
    }
        jstring_append_1(string, tmp);
        JXTA_OBJECT_RELEASE(tmp);
    } if( NULL != ad->text_parm ) {
        jstring_append_1(string, ad->text_parm );
    }

    jstring_append_2(string, "</Parm>\n");
    jstring_append_2(string, "</Svc>\n");

    *xml = string;
    
    return res;
}

/** Get a new instance of the ad. 
 * Svc has a set of possible content. Some of them are
 * bulky, so, as a matter of principle we do not install
 * defaults, we use and test for NULL wherever appropriate.
 */

static Jxta_svc *jxta_svc_construct(Jxta_svc * self)
{
    self = (Jxta_svc *) jxta_advertisement_construct((Jxta_advertisement *) self,
                                                     "Svc",
                                                     Svc_tags,
                                                     (JxtaAdvertisementGetXMLFunc) jxta_svc_get_xml,
                                                     (JxtaAdvertisementGetIDFunc) jxta_svc_get_MCID,
                                                     (JxtaAdvertisementGetIndexFunc) NULL);

    /* Fill in the required initialization code here. */
    if (NULL != self) {
        self->MCID = NULL;
        self->parm = NULL;
    }

    return self;
}

JXTA_DECLARE(Jxta_svc *) jxta_svc_new(void)
{
    Jxta_svc *myself = (Jxta_svc *) calloc(1, sizeof(Jxta_svc));

    if( NULL == myself ) {
        return NULL;
    }

    JXTA_OBJECT_INIT(myself, svc_delete, NULL);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Svc NEW [%pp]\n", myself );    

    return jxta_svc_construct(myself);
}

static void svc_delete(Jxta_object * me)
{
    Jxta_svc *ad = (Jxta_svc *) me;

    if (ad->MCID != NULL)
        JXTA_OBJECT_RELEASE(ad->MCID);
        
    if (ad->parm != NULL)
        JXTA_OBJECT_RELEASE(ad->parm);

    jxta_advertisement_destruct((Jxta_advertisement *) ad);
    
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Svc FREE [%pp]\n", ad );    
    
    memset(ad, 0xdd, sizeof(Jxta_svc));
    
    free(ad);
}

JXTA_DECLARE(Jxta_status) jxta_svc_parse_charbuffer(Jxta_svc * myself, const char *buf, int len)
{
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(myself);

    res = jxta_advertisement_parse_charbuffer((Jxta_advertisement *) myself, buf, len);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_advertisement(myself);
}

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_svc_parse_file(Jxta_svc * myself, FILE * stream)
{
    Jxta_status res;

    JXTA_OBJECT_CHECK_VALID(myself);

    res = jxta_advertisement_parse_file((Jxta_advertisement *) myself, stream);
    
    if( JXTA_SUCCESS == res ) {
        res = validate_advertisement(myself);
    }
    
    return res;
}

/**
*   @deprecated
**/
JXTA_DECLARE(JString*) jxta_svc_get_IsClient(Jxta_svc * relay_svc )
{
    Jxta_RelayAdvertisement * relay;

    JXTA_OBJECT_CHECK_VALID(relay_svc);

    relay = jxta_svc_get_RelayAdvertisement(relay_svc);
    
    if( NULL == relay ) {
        return NULL;
    }
    
    JXTA_OBJECT_CHECK_VALID(relay);
    
    return jxta_RelayAdvertisement_get_IsClient(relay);
}

/* vi: set ts=4 sw=4 tw=130 et: */
