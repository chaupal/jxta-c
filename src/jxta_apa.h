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


#ifndef JXTA_ACCESSPOINTADVERTISEMENT_H__
#define JXTA_ACCESSPOINTADVERTISEMENT_H__

#include "jxta_advertisement.h"
#include "jxta_endpoint_address.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jxta_AccessPointAdvertisement Jxta_AccessPointAdvertisement;

JXTA_DECLARE(Jxta_AccessPointAdvertisement *) jxta_AccessPointAdvertisement_new(void);
JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_get_xml(Jxta_AccessPointAdvertisement *, JString **);
JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_parse_charbuffer(Jxta_AccessPointAdvertisement *, const char *, int len);
JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_parse_file(Jxta_AccessPointAdvertisement *, FILE * stream);

JXTA_DECLARE(Jxta_id *) jxta_AccessPointAdvertisement_get_PID(Jxta_AccessPointAdvertisement *);
JXTA_DECLARE(void) jxta_AccessPointAdvertisement_set_PID(Jxta_AccessPointAdvertisement *, Jxta_id *);

JXTA_DECLARE(Jxta_vector *) jxta_AccessPointAdvertisement_get_EndpointAddresses(Jxta_AccessPointAdvertisement *);
JXTA_DECLARE(void) jxta_AccessPointAdvertisement_set_EndpointAddresses(Jxta_AccessPointAdvertisement *, Jxta_vector *);

JXTA_DECLARE(Jxta_status) jxta_AccessPointAdvertisement_add_EndpointAddress(Jxta_AccessPointAdvertisement *me, 
                                                                            Jxta_endpoint_address *ea);

JXTA_DECLARE(Jxta_vector *) jxta_AccessPointAdvertisement_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* JXTA_ACCESSPOINTDVERTISEMENT_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
