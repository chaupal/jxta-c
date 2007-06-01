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
 * $Id: jxta_membership_service.h,v 1.8 2006/09/15 18:31:33 bondolo Exp $
 */


#ifndef __Jxta_Membership_Service_H__
#define __Jxta_Membership_Service_H__

#include "jxta_service.h"

#include "jxta_vector.h"
#include "jxta_cred.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
*   The membership service is responsible for establishing and maintaining
*   relations between identities and peers. Both authentication of local
*   identity and validation of remote identity are provided.
**/
typedef struct _jxta_membership_service const Jxta_membership_service;

/**
*   An authenticator allows an application to provide authentication information
*   for the purpose of establishing a claim of identity to the membership service.
**/
typedef struct _jxta_membership_authenticator const Jxta_membership_authenticator;

/**
*   Request creation of an authenticator object. The <tt>authCred</tt> is
*   provided to allow selection of authenticator method and/or authentication
*   parameters.
*
*   @param self The membership service.
*   @param authCred A credential object which specifies any authentication 
*   parameters and/or authentication pre-requisites.
*   @param auth The authenticator object returned.
*   @return JXTA_SUCCESS if the authentication credential was accepted and
*   an authenticator is returned otherwise errors specific to membership 
*   service implementation.
**/
JXTA_DECLARE(Jxta_status) jxta_membership_service_apply(Jxta_membership_service * self, Jxta_credential * authCred,
                                                        Jxta_membership_authenticator ** auth);

/**
*   Request creation of a credential using a completed authenticator object.
*
*   @param self The membership service.
*   @param auth The authenticator which is being used to create the credential.
*   @param newcred The resulting credential if the join request is successful.
*   @return JXTA_SUCCESS if the join request is successful otherwise errors
*   specific to membership service implementation.
**/
JXTA_DECLARE(Jxta_status) jxta_membership_service_join(Jxta_membership_service * self, Jxta_membership_authenticator * auth,
                                                       Jxta_credential ** newcred);

/**
*   Causes the membership service to invalidate and forget all currently active credentials it has generated.
*
*   @param self The membership service.
*   @return JXTA_SUCCESS
**/
JXTA_DECLARE(Jxta_status) jxta_membership_service_resign(Jxta_membership_service * self);


/**
*   Returns the set of currently active credentials. The first element is the default credential.
*
*   @param self The membership service.
*   @param creds The current active set of credentials.
*   @return JXTA_SUCCESS
**/
JXTA_DECLARE(Jxta_status) jxta_membership_service_get_currentcreds(Jxta_membership_service * self, Jxta_vector ** creds);

/**
*   Processes and validates an XML serialized remote credential. The remote
*   credential is validated according to the policies of the membership service.
*   If JXTA_SUCCESS is returned then the resulting credential object is known to be
*   a valid claim of the specified credential identity by the specified peer.
*
*   @param self The membership service.
*   @param cred The remote credential.
*   @return JXTA_SUCCESS if the credential was acceptable otherwise errors 
*   specific to the membership service implementation.
**/
JXTA_DECLARE(Jxta_status) jxta_membership_service_makecred(Jxta_membership_service * self, JString * somecred,
                                                           Jxta_credential ** cred);


#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_Membership_Service_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
