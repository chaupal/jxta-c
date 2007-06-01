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
 * $Id: jxta_svc.h,v 1.13 2006/09/01 03:08:10 bondolo Exp $
 */


#ifndef Svc_H__
#define Svc_H__

#include "jxta_advertisement.h"
#include "jxta_hta.h"
#include "jxta_tta.h"
#include "jxta_cache_config_adv.h"
#include "jxta_discovery_config_adv.h"
#include "jxta_endpoint_config_adv.h"
#include "jxta_rdv_config_adv.h"
#include "jxta_srdi_config_adv.h"
#include "jxta_relaya.h"
#include "jxta_routea.h"
#include "jxta_cache_config_adv.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_svc Jxta_svc;

JXTA_DECLARE(Jxta_svc *) jxta_svc_new(void);

JXTA_DECLARE(Jxta_status) jxta_svc_get_xml(Jxta_svc *, JString **);
JXTA_DECLARE(void) jxta_svc_parse_charbuffer(Jxta_svc *, const char *, int len);
JXTA_DECLARE(void) jxta_svc_parse_file(Jxta_svc *, FILE * stream);

JXTA_DECLARE(char *) jxta_svc_get_Svc(Jxta_svc *);
JXTA_DECLARE(void) jxta_svc_set_Svc(Jxta_svc *, char *);

JXTA_DECLARE(char *) jxta_svc_get_Parm(Jxta_svc *);
JXTA_DECLARE(void) jxta_svc_set_Parm(Jxta_svc *, char *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_id *) jxta_svc_get_MCID(Jxta_svc *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(void) jxta_svc_set_MCID(Jxta_svc *, Jxta_id *);

/*
 * Unlike similar accessors in other advs, this one may return NULL
 * if there was no IsClient  element instead of an empty vector. 
 */
JXTA_DECLARE(JString *) jxta_svc_get_IsClient(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL
 * instead of an empty vector.
 */
JXTA_DECLARE(void) jxta_svc_set_IsClient(Jxta_svc *, JString *);

/*
 * Unlike similar accessors in other advs, this one may return NULL
 * if there was no IsServer  element instead of an empty vector. 
 */
JXTA_DECLARE(JString *) jxta_svc_get_IsServer(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL
 * instead of an empty vector.
 */
JXTA_DECLARE(void) jxta_svc_set_IsServer(Jxta_svc *, JString *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_HTTPTransportAdvertisement *) jxta_svc_get_HTTPTransportAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_HTTPTransportAdvertisement(Jxta_svc *, Jxta_HTTPTransportAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_RouteAdvertisement *) jxta_svc_get_RouteAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_RouteAdvertisement(Jxta_svc *, Jxta_RouteAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_TCPTransportAdvertisement *) jxta_svc_get_TCPTransportAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_TCPTransportAdvertisement(Jxta_svc *, Jxta_TCPTransportAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_DiscoveryConfigAdvertisement *) jxta_svc_get_DiscoveryConfig(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_DiscoveryConfig(Jxta_svc *, Jxta_DiscoveryConfigAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_CacheConfigAdvertisement *) jxta_svc_get_CacheConfig(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_CacheConfig(Jxta_svc *, Jxta_CacheConfigAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_RdvConfigAdvertisement *) jxta_svc_get_RdvConfig(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_RdvConfig(Jxta_svc *, Jxta_RdvConfigAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_EndPointConfigAdvertisement *) jxta_svc_get_EndPointConfig(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_EndPointConfig(Jxta_svc *, Jxta_EndPointConfigAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_SrdiConfigAdvertisement *) jxta_svc_get_SrdiConfig(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_SrdiConfig(Jxta_svc *, Jxta_SrdiConfigAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(Jxta_RelayAdvertisement *) jxta_svc_get_RelayAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
JXTA_DECLARE(void) jxta_svc_set_RelayAdvertisement(Jxta_svc *, Jxta_RelayAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JXTA_DECLARE(JString *) jxta_svc_get_RootCert(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL instead
 * of an empty JString.
 */
JXTA_DECLARE(void) jxta_svc_set_RootCert(Jxta_svc *, JString *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* Svc_H__  */

/* vim: set ts=4 sw=4 et tw=130: */
