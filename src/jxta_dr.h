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


#ifndef __Jxta_DiscoveryResponse_H__
#define __Jxta_DiscoveryResponse_H__

#include "jxta_advertisement.h"
#include "jxta_discovery_service.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

#define DR_TAG_START "<Response Expiration=\"%"
#define DR_TAG_START_RIGHT_PAREN "\">\n"
#define DR_TAG_END "</Response>\n"
#define DR_TAG_1 "<?xml version=\"1.0\"?>\n<!DOCTYPE jxta:DiscoveryResponse>\n<jxta:DiscoveryResponse>\n"
#define DR_TAG_2 "<Type>"
#define DR_TAG_3 "</Type>\n"

#define DR_TAG_4 "<Count>"

#define DR_TAG_5 "</Count>\n"

#define DR_TAG_6 "<PeerAdv>"

#define DR_TAG_7 "</PeerAdv>\n"

#define DR_TAG_8    "<Attr>"
#define DR_TAG_9    "</Attr>\n"

#define DR_TAG_10    "<Value>"
#define DR_TAG_11    "</Value>\n"

#define DR_TAG_12    "</jxta:DiscoveryResponse>\n"


#define DR_TAG_SIZE sizeof(DR_TAG_START) + sizeof(DR_TAG_START_RIGHT_PAREN) + sizeof(DR_TAG_END) + 10

typedef struct _Jxta_DiscoveryResponse Jxta_DiscoveryResponse;
typedef struct _jxta_DiscoveryResponseElement Jxta_DiscoveryResponseElement;

/* Expiration is an attribute that will have to be handled for
 * each response.
 */

struct _jxta_DiscoveryResponseElement {
    JXTA_OBJECT_HANDLE;
    Jxta_expiration_time expiration;
    JString *response;
    JString *query;
};

/**
 * Allocate a new discovery response.
 *
 * @param void takes no arguments.
 *
 * @return Pointer to discovery response.
 */
JXTA_DECLARE(Jxta_DiscoveryResponse *) jxta_discovery_response_new(void);

JXTA_DECLARE(Jxta_DiscoveryResponse *) jxta_discovery_response_new_1(short type,
                                                                     const char *attr,
                                                                     const char *value,
                                                                     int threshold, JString * peeradv, Jxta_vector * responses);

/**
 * Constructs a representation of a discovery response in
 * xml format.
 *
 * @param Jxta_DiscoveryResponse * pointer to discovery response
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of discovery response.
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_xml(Jxta_DiscoveryResponse *, JString **);


/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a discovery response structure.
 *
 * @param Jxta_DiscoveryResponse * pointer to discovery response
 *        advertisement to contain the data in the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_parse_charbuffer(Jxta_DiscoveryResponse *, const char *, int len);



/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a discovery response structure.
 *
 * @param Jxta_DiscoveryResponse * discovery response
 *        to contain the data in the xml.
 * @param FILE * an input stream.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_parse_file(Jxta_DiscoveryResponse *, FILE * stream);

/**
 * Gets the query ID of the resolver query generates this discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return long set to value of the query ID.
 */
JXTA_DECLARE(long) jxta_discovery_response_get_query_id(Jxta_DiscoveryResponse *);

/**
 * Return the pointer to the discovery service received the response
 *
 * @param me * a pointer to the discovery response.
 *
 * @return the pointer to the discovery service received the response
 */
JXTA_DECLARE(Jxta_discovery_service*) jxta_discovery_response_discovery_service(Jxta_DiscoveryResponse * me);

/**
 * Gets the Type of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return short set to value of the type.
 */
JXTA_DECLARE(short) jxta_discovery_response_get_type(Jxta_DiscoveryResponse *);


/**
 * Sets the Type of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param char * containing the Type.
 *
 * @return Jxta_status JXTA_SUCCESS on success.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_type(Jxta_DiscoveryResponse *, short type);


/**
 * Gets the Count of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return int containing the count.
 */
JXTA_DECLARE(int) jxta_discovery_response_get_count(Jxta_DiscoveryResponse *);


/**
 * Sets the Count of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param char * containing the Count.
 *
 * @return Jxta_status Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_count(Jxta_DiscoveryResponse *, int count);


/**
 * Gets the peer advertisement of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return char * containing the peer advertisement.
 */
/* FIXME: NEed to change this to Jxta_PA * */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_peeradv(Jxta_DiscoveryResponse *, JString **);


/**
 * Sets the peer advertisement of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param char * containing the peer advertisement.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_peeradv(Jxta_DiscoveryResponse *, JString *);

/**
 * Gets the peer advertisement object of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return Jxta_advertisement *  the peer advertisement.
 */
/* FIXME: NEed to change this to Jxta_PA * */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_peer_advertisement(Jxta_DiscoveryResponse *, Jxta_advertisement **);


/**
 * Sets the peer advertisement object of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param Jxta_advertisement * the peer advertisement.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_peer_advertisement(Jxta_DiscoveryResponse *, Jxta_advertisement *);


/**
 * Gets the Attr of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return char * containing the Attr.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_attr(Jxta_DiscoveryResponse *, JString **);


/**
 * Sets the Attr of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param char * containing the Attr.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_set_attr(Jxta_DiscoveryResponse *, JString *);


/**
 * Gets the Value of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 *
 * @return char * containing the Value.
 */
JXTA_DECLARE(Jxta_status) jxta_discovery_response_get_value(Jxta_DiscoveryResponse *, JString **);


/**
 * Sets the Value of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param char * containing the Value.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(Jxta_status)
    jxta_discovery_response_set_value(Jxta_DiscoveryResponse *, JString *);


/**
 * Gets the Response of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param responses pointer to vector containing the responses
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status)
    jxta_discovery_response_get_responses(Jxta_DiscoveryResponse * ad, Jxta_vector ** responses);


/**
 * Sets the Response of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param Jxta_vector * containing the Response.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_discovery_response_set_responses(Jxta_DiscoveryResponse *, Jxta_vector * response);

/**
 * Gets the advertisements of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param responses pointer to vector containing the responses
 *
 * @return Jxta_status 
 */
JXTA_DECLARE(Jxta_status)
    jxta_discovery_response_get_advertisements(Jxta_DiscoveryResponse * ad, Jxta_vector ** advertisements);


/**
 * Sets the advertisements of the discovery response.
 *
 * @param Jxta_DiscoveryResponse * a pointer to the discovery 
 *        response.
 * @param Jxta_vector * containing the Response.
 *
 * @return void Doesn't return anything.
 */
JXTA_DECLARE(void) jxta_discovery_response_set_advertisements(Jxta_DiscoveryResponse *, Jxta_vector * response);

/**
 * Creates a new Discovery Response element
 * @return Jxta_DiscoveryResponseElement 
 */
JXTA_DECLARE(Jxta_DiscoveryResponseElement *) jxta_discovery_response_new_element(void);

/**
 * Creates a new Discovery Response element
 * @param JString response
 * @param long the expiration time associated with the response
 * @return Jxta_DiscoveryResponseElement 
 */
JXTA_DECLARE(Jxta_DiscoveryResponseElement *) jxta_discovery_response_new_element_1(JString * response,
                                                                                    Jxta_expiration_time expiration);

JXTA_DECLARE(Jxta_time) jxta_discovery_response_timestamp(Jxta_DiscoveryResponse *);
JXTA_DECLARE(void) jxta_discovery_response_set_timestamp(Jxta_DiscoveryResponse *, Jxta_time);
JXTA_DECLARE(void) jxta_discovery_response_set_query(Jxta_DiscoveryResponse * dr, JString * query);
JXTA_DECLARE(void) jxta_discovery_response_get_query(Jxta_DiscoveryResponse * dr, JString **query);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __Jxta_DiscoveryResponse_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
