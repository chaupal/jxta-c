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
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
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

#include "jxta_platformconfig.h"

#ifndef WIN32
#include <arpa/inet.h>
#endif

#include "jxta_tta.h"
#include "jxta_hta.h"
#include "jxta_relaya.h"
#include "jxta_svc.h"
#include "jxta_peergroup.h"

static const char *__log_cat = "PLATFORMCONFIG";

JXTA_DECLARE(Jxta_PA *) jxta_PlatformConfig_create_default()
{
  Jxta_PA *config_adv = NULL;
  Jxta_id *pid = NULL;
  Jxta_TCPTransportAdvertisement *tta = NULL; /* append */
  Jxta_HTTPTransportAdvertisement *hta = NULL;
  Jxta_RelayAdvertisement *rla = NULL;
  Jxta_DiscoveryConfigAdvertisement *disc = NULL;
  Jxta_RdvConfigAdvertisement *rdv = NULL;
  Jxta_SrdiConfigAdvertisement *srdi = NULL;
  Jxta_EndPointConfigAdvertisement *ep = NULL;
  Jxta_CacheConfigAdvertisement *cache = NULL;
  
  Jxta_svc *tcpsvc;   /* append */
  Jxta_svc *htsvc;
  Jxta_svc *discsvc;
  Jxta_svc *rdvsvc;
  Jxta_svc *srdisvc;
  Jxta_svc *rlsvc;
  Jxta_svc *epsvc;
  Jxta_svc *cachesvc;

  JString *tcp_proto; /* append */
  JString *http_proto;
  Jxta_endpoint_address *def_rdv;
  JString *auto_confmod;
  JString *def_name;
  Jxta_vector *services;
  
  tcp_proto = jstring_new_2("tcp");   /* append */
  
  http_proto = jstring_new_2("http");
  def_rdv = jxta_endpoint_address_new("http://rdv.jxtahosts.net/cgi-bin/rendezvous.cgi?2");
  auto_confmod = jstring_new_2("auto");
  def_name = jstring_new_2("JXTA-C Peer");
  
  /* TCP */
  tta = jxta_TCPTransportAdvertisement_new(); /* append */
  tcpsvc = jxta_svc_new();
  jxta_TCPTransportAdvertisement_set_Protocol(tta, tcp_proto);
  jxta_TCPTransportAdvertisement_set_InterfaceAddress(tta, htonl(0x00000000));
  jxta_TCPTransportAdvertisement_set_Port(tta, 9701);
  jxta_TCPTransportAdvertisement_set_ConfigMode(tta, auto_confmod);
  jxta_TCPTransportAdvertisement_set_ServerOff(tta, FALSE);
  jxta_TCPTransportAdvertisement_set_ClientOff(tta, FALSE);
  jxta_TCPTransportAdvertisement_set_MulticastAddr(tta, htonl(0xE0000155));
  jxta_TCPTransportAdvertisement_set_MulticastPort(tta, 1234);
  jxta_TCPTransportAdvertisement_set_MulticastSize(tta, 16384);
  jxta_TCPTransportAdvertisement_set_MulticastOff(tta, TRUE);
  jxta_svc_set_TCPTransportAdvertisement(tcpsvc, tta);
  jxta_svc_set_MCID(tcpsvc, jxta_tcpproto_classid);
  
  /* HTTP */
  hta = jxta_HTTPTransportAdvertisement_new();
  htsvc = jxta_svc_new();
  jxta_HTTPTransportAdvertisement_set_Protocol(hta, http_proto);
  jxta_HTTPTransportAdvertisement_set_InterfaceAddress(hta, htonl(0x00000000));
  jxta_HTTPTransportAdvertisement_set_ConfigMode(hta, auto_confmod);
  jxta_HTTPTransportAdvertisement_set_Port(hta, 9700);
  jxta_HTTPTransportAdvertisement_set_ServerOff(hta, TRUE);
  jxta_HTTPTransportAdvertisement_set_ProxyOff(hta, TRUE);
  
  jxta_svc_set_HTTPTransportAdvertisement(htsvc, hta);
  jxta_svc_set_MCID(htsvc, jxta_httpproto_classid);
  
  /* Relay */
  rla = jxta_RelayAdvertisement_new();
  rlsvc = jxta_svc_new();
  jxta_RelayAdvertisement_set_IsClient(rla, jstring_new_2("true"));
  jxta_RelayAdvertisement_set_IsServer(rla, jstring_new_2("false"));
  jxta_svc_set_RelayAdvertisement(rlsvc, rla);
  jxta_svc_set_MCID(rlsvc, jxta_relayproto_classid);

  cache = jxta_CacheConfigAdvertisement_new();
  cachesvc = jxta_svc_new();
  jxta_CacheConfigAdvertisement_create_default(cache, FALSE);
  jxta_svc_set_CacheConfig(cachesvc, cache);
  jxta_svc_set_MCID(cachesvc, jxta_cache_classid);

  /* Discovery */
  disc = jxta_DiscoveryConfigAdvertisement_new();
  discsvc = jxta_svc_new();
  jxta_svc_set_DiscoveryConfig(discsvc, disc);
  jxta_svc_set_MCID(discsvc, jxta_discovery_classid);
  
  /* EndPoint */
  ep = jxta_EndPointConfigAdvertisement_new();
  epsvc = jxta_svc_new();
  jxta_svc_set_EndPointConfig(epsvc, ep);
  jxta_svc_set_MCID(epsvc, jxta_endpoint_classid);

  /* Rendezvous */
  rdv = jxta_RdvConfigAdvertisement_new();
  jxta_RdvConfig_set_config(rdv, config_edge);
  jxta_RdvConfig_add_seeding(rdv, def_rdv);
  rdvsvc = jxta_svc_new();
  jxta_svc_set_RdvConfig(rdvsvc, rdv);
  jxta_svc_set_MCID(rdvsvc, jxta_rendezvous_classid);
  
  /* SRDI */
  srdi = jxta_SrdiConfigAdvertisement_new();
  srdisvc = jxta_svc_new();
  jxta_svc_set_SrdiConfig(srdisvc, srdi);
  jxta_svc_set_MCID(srdisvc, jxta_srdi_classid);          
          
          
  config_adv = jxta_PA_new();
  services = jxta_PA_get_Svc(config_adv);
  jxta_vector_add_object_last(services, (Jxta_object *) rdvsvc);
  jxta_vector_add_object_last(services, (Jxta_object *) tcpsvc);      /* append */
  jxta_vector_add_object_last(services, (Jxta_object *) htsvc);
  jxta_vector_add_object_last(services, (Jxta_object *) rlsvc);
  jxta_vector_add_object_last(services, (Jxta_object *) srdisvc);
  jxta_vector_add_object_last(services, (Jxta_object *) epsvc);
  jxta_vector_add_object_last(services, (Jxta_object *) discsvc);
  jxta_vector_add_object_last(services, (Jxta_object *) cachesvc);
  
  jxta_id_peerid_new_1(&pid, jxta_id_defaultNetPeerGroupID);
  jxta_PA_set_PID(config_adv, pid);
  jxta_PA_set_Name(config_adv, def_name);

  return config_adv;
}

JXTA_DECLARE(Jxta_PA *) jxta_PlatformConfig_read(const char *fname)
{

    Jxta_PA *ad;
    FILE *advfile;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Reading configuration ...\n");
    ad = jxta_PA_new();
    if (ad == NULL) {
        return NULL;
    }

    advfile = fopen(fname, "r");
    if (advfile == NULL) {
        JXTA_OBJECT_RELEASE(ad);
        return NULL;
    }
    jxta_PA_parse_file(ad, advfile);
    fclose(advfile);
    return ad;
}

JXTA_DECLARE(Jxta_status) jxta_PlatformConfig_write(Jxta_PA * ad, const char *fname)
{
    FILE *advfile;
    JString *advs;

    advfile = fopen(fname, "w");
    if (advfile == NULL) {
        return JXTA_IOERR;
    }

    jxta_PA_get_xml(ad, &advs);
    fwrite(jstring_get_string(advs), jstring_length(advs), 1, advfile);
    JXTA_OBJECT_RELEASE(advs);

    fclose(advfile);
    return JXTA_SUCCESS;
}

