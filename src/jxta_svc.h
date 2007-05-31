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
 * $Id: jxta_svc.h,v 1.2 2005/01/10 17:22:19 brent Exp $
 */

   
#ifndef Svc_H__
#define Svc_H__

#include "jxta_advertisement.h"
#include "jxta_hta.h"
#include "jxta_tta.h"
#include "jxta_relaya.h"
#include "jxta_routea.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _jxta_svc Jxta_svc;

Jxta_svc * jxta_svc_new(void);

Jxta_status jxta_svc_get_xml(Jxta_svc *, JString **);
void jxta_svc_parse_charbuffer(Jxta_svc *, const char *, int len); 
void jxta_svc_parse_file(Jxta_svc *, FILE * stream);
 
char * jxta_svc_get_Svc(Jxta_svc *);
void jxta_svc_set_Svc(Jxta_svc *, char *);
 
char * jxta_svc_get_Parm(Jxta_svc *);
void jxta_svc_set_Parm(Jxta_svc *, char *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
Jxta_id * jxta_svc_get_MCID(Jxta_svc *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
void jxta_svc_set_MCID(Jxta_svc *, Jxta_id *);
 
/*
 * Unlike similar accessors in other advs, this one may return NULL
 * if there was no Addr element instead of an empty vector. 
 */
Jxta_vector* jxta_svc_get_Addr(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL
 * instead of an empty vector.
 */
void jxta_svc_set_Addr(Jxta_svc *, Jxta_vector *);

/*
 * Unlike similar accessors in other advs, this one may return NULL
 * if there was no IsClient  element instead of an empty vector. 
 */
JString* jxta_svc_get_IsClient(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL
 * instead of an empty vector.
 */
void jxta_svc_set_IsClient(Jxta_svc *, JString *);

/*
 * Unlike similar accessors in other advs, this one may return NULL
 * if there was no IsServer  element instead of an empty vector. 
 */
JString* jxta_svc_get_IsServer(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL
 * instead of an empty vector.
 */
void jxta_svc_set_IsServer(Jxta_svc *, JString *);

/*
 * Returns the value of the Rdv field. If the field is irrelevant to
 * that particular service it is FALSE.
 */
Jxta_boolean jxta_svc_get_Rdv(Jxta_svc *);

/*
 * Sets the value of the Rdv field.
 */
void jxta_svc_set_Rdv(Jxta_svc *, Jxta_boolean);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
Jxta_HTTPTransportAdvertisement * jxta_svc_get_HTTPTransportAdvertisement(Jxta_svc *);
 
/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
void jxta_svc_set_HTTPTransportAdvertisement(Jxta_svc *,
					Jxta_HTTPTransportAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
Jxta_RouteAdvertisement * jxta_svc_get_RouteAdvertisement(Jxta_svc *);
 
/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
void jxta_svc_set_RouteAdvertisement(Jxta_svc *,
				     Jxta_RouteAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
Jxta_TCPTransportAdvertisement * jxta_svc_get_TCPTransportAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
void jxta_svc_set_TCPTransportAdvertisement(Jxta_svc *,
				       Jxta_TCPTransportAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
Jxta_RelayAdvertisement * jxta_svc_get_RelayAdvertisement(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL as a means
 * to remove the element.
 */
void jxta_svc_set_RelayAdvertisement(Jxta_svc *,
				       Jxta_RelayAdvertisement *);

/*
 * Unlike similar accessors in other advs, this one may return NULL if
 * there is no such element.
 */
JString* jxta_svc_get_RootCert(Jxta_svc *);

/*
 * Unlike similar mutators in other advs, it is valid to pass NULL instead
 * of an empty JString.
 */
void jxta_svc_set_RootCert(Jxta_svc *, JString*);

#ifdef __cplusplus
}
#endif

#endif /* Svc_H__  */
