/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
 * reserved.
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
 * $Id: jxta_endpoint_address.h,v 1.6 2005/02/08 01:04:46 bondolo Exp $
 */


#ifndef JXTA_ENDPOINT_ADDRESS_H
#define JXTA_ENDPOINT_ADDRESS_H

#include "jxta_object.h"

/* #include "jxta_types.h" */


#ifdef __cplusplus
extern "C" {

    /* avoid confusing indenters */
#if 0
}
#endif
#endif

/**
 ** Jxta_endpoint_address is the incomplete type refering to an object
 ** containing a JXTA Endpoint Address.
 **
 ** This is a Jxta_object: JXTA_OBJECT_SHARE and JXTA_OBJECT_RELEASE
 ** must be used when necessary.
 **/

typedef struct _jxta_endpoint_address   Jxta_endpoint_address;


/**
 ** Creates a new Jxta_endpoint_address of a URI.
 ** Note that the string which is passed as argument is copied by this
 ** function. The newly created Jxta_endpoint_address is created already
 ** shared.
 **
 ** @param s a pointer to a null-terminated string that contains the URI
 ** @return an Jxta_endpoint_address* pointing to the new Jxta_endpoint_addres.
 ** NULL is returned when the URI was incorrect.
 **/
Jxta_endpoint_address* jxta_endpoint_address_new (const char *s);

/**
 ** Creates a new Jxta_endpoint_address.
 ** Note that the strings which are passed as argument are copied by this
 ** function. The newly created Jxta_endpoint_address is created already
 ** shared.
 **
 ** @param protocol_name name of the protocol.
 ** @param protocol_address address.
 ** @param service_name name of the service
 ** @param service_param parameter of the service
 ** @return an Jxta_endpoint_address* pointing to the new Jxta_endpoint_addres.
 ** NULL is returned when the URI was incorrect.
 **/
Jxta_endpoint_address* jxta_endpoint_address_new2 (const char *protocol_name,
						   const char *protocol_address,
						   const char *service_name,
						   const char *service_params);

/**
 ** Get the protocol name.
 **
 ** The pointer returned by this function points to an internal buffer of the
 ** Jxta_endpoint_address, which is valid only as long as the Jxta_endpoint_address
 ** is valid.
 **
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns a pointer to a string containing the protocol name.
 **/
const char* jxta_endpoint_address_get_protocol_name    (const  Jxta_endpoint_address *addr);

/**
 ** Get the address.
 **
 ** The pointer returned by this function points to an internal buffer of the
 ** Jxta_endpoint_address, which is valid only as long as the Jxta_endpoint_address
 ** is valid.
 **
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns a pointer to a string containing the protocol address.
 **/
const char* jxta_endpoint_address_get_protocol_address (const Jxta_endpoint_address *addr);

/**
 ** Get the service name
 **
 ** The pointer returned by this function points to an internal buffer of the
 ** Jxta_endpoint_address, which is valid only as long as the Jxta_endpoint_address
 ** is valid.
 **
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns a pointer to a string containing the name of the service.
 **/
const char* jxta_endpoint_address_get_service_name     (const  Jxta_endpoint_address *addr);

/**
 ** Get the service parameter
 **
 ** The pointer returned by this function points to an internal buffer of the
 ** Jxta_endpoint_address, which is valid only as long as the Jxta_endpoint_address
 ** is valid.
 **
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns a pointer to a string containing the service parameter
 **/
const char* jxta_endpoint_address_get_service_params   (const  Jxta_endpoint_address *addr);

/**
 ** Returns the size of the endpoint address
 ** 
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns the size of the Jxta_endpoint_address.
 **/
int jxta_endpoint_address_size (const  Jxta_endpoint_address *addr);

/**
 ** Returns a newly created null terminated string that contains the URI
 ** representation of the Jxta_endpoint_address. The string returned by this
 ** function must be freed using free()
 **
 ** @param addr pointer to a Jxta_endpoint_address.
 ** @returns a pointer to a string containing the URI.
 **/
char *jxta_endpoint_address_to_string            (const Jxta_endpoint_address *addr);


/**
 ** Checks of two Jxta_endpoint_address refers to the same Endpoint Address.
 **
 ** @param addr1 pointer to a Jxta_endpoint_address
 ** @param addr2 pointer to a Jxta_endpoint_address
 ** @return a boolean. TRUE if the two Jxta_endpoint_address are equal, FALSE otherwise.
 **/
Jxta_boolean jxta_endpoint_address_equals        (const Jxta_endpoint_address* addr1,
						  const Jxta_endpoint_address* addr2);


#ifdef __cplusplus
}
#endif

#endif /* JXTA_ENDPOINT_ADDRESS_H */
