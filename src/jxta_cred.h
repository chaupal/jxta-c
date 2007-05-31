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
 * $Id: jxta_cred.h,v 1.2 2005/01/10 17:23:42 brent Exp $
 */


#ifndef __Jxta_Credential_H__
#define __Jxta_Credential_H__

#include "jxta_advertisement.h"
#include "jstring.h"
#include "jxta_service.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

    typedef struct _Jxta_credential const Jxta_credential;

    /*typedef struct _Jxta_membership_service const Jxta_membership_service;*/

    /**
     * Constructs a representation of a credential in
     * xml format. This version includes the document header information.
     *
     * @param ad pointer to credential
     * @param func pointer to the function that will be called to write to the stream
     *        that accumulates xml representation of credential.
     * @param stream parameter that will be passed to the write function.
     *
     * @return Jxta_status 
     */
    Jxta_status
    jxta_credential_get_xml( Jxta_advertisement *ad , Jxta_write_func func, void* stream );

    /**
     * Constructs a representation of a credential in
     * xml format. This version begins output at the root node and is suitable for inclusion
     * in other documents
     *
     * @param ad pointer to credential
     * @param func pointer to the function that will be called to write to the stream
     *        that accumulates xml representation of credential.
     * @param stream parameter that will be passed to the write function.
     *
     * @return Jxta_status 
     */
    Jxta_status
    jxta_credential_get_xml_1( Jxta_advertisement *ad, Jxta_write_func func, void* stream );

    /**
     * Wrapper for jxta_advertisement_parse_charbuffer,
     * when call completes, the xml representation of a 
     * buffer is loaded into a credential structure.
     *
     * @param Jxta_credential * pointer to discovery query
     *        advertisement to contain the data in the xml.
     * @param const char * buffer containing characters 
     *        with xml syntax.
     * @param int len length of character buffer.
     *
     * @return void Doesn't return anything.
     */
    Jxta_status
    jxta_credential_parse_charbuffer( Jxta_credential *, const char *, int len );


    /**
     * Wrapper for jxta_advertisement_parse_file,
     * when call completes, the xml representation of a 
     * data is loaded into a credential structure.
     *
     * @param Jxta_credential * credential
     *        to contain the data in the xml.
     * @param FILE * an input stream.
     *
     * @return void Doesn't return anything.
     */
    Jxta_status
    jxta_credential_parse_file( Jxta_credential *, FILE * stream );

    /**
     * Gets the peergroup id of the credential.
     *
     * @param Jxta_credential * a pointer to the credential.
     *
     * @return char * containing the type.
     */
    Jxta_status
    jxta_credential_get_peergroupid( Jxta_credential *, Jxta_id** pg );

    /**
     * Gets the peer id of the credential.
     *
     * @param Jxta_credential * a pointer to the credential.
     *
     * @return char * containing the type.
     */
    Jxta_status
    jxta_credential_get_peerid( Jxta_credential *, Jxta_id** peer );

    /**
     * Gets the service which issued this credential.
     *
     * @param Jxta_credential * a pointer to the credential.
     *
     * @return char * containing the type.
     */
    Jxta_status
    jxta_credential_get_source( Jxta_credential *, Jxta_service** svc );

#endif /* __Jxta_Credential_H__  */
#ifdef __cplusplus
}
#endif
