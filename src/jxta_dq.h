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
 * $Id: jxta_dq.h,v 1.3 2005/01/12 21:46:57 bondolo Exp $
 */

   
#ifndef __Jxta_DiscoveryQuery_H__
#define __Jxta_DiscoveryQuery_H__

#include "jxta_advertisement.h"
#include "jstring.h"


#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

typedef struct _Jxta_DiscoveryQuery Jxta_DiscoveryQuery;


/**
 * Allocate a new discovery query advertisement.
 *
 * @param void takes no arguments.
 *
 * @return Pointer to discovery query advertisement.
 */
Jxta_DiscoveryQuery * jxta_discovery_query_new(void);

Jxta_DiscoveryQuery * jxta_discovery_query_new_1(short type, 
			   const char * attr, 
			   const char * value,
			   int threshold,
			   JString * peeradv);

/**
 * Delete a discovery query advertisement.
 *
 * @param pointer to discovery query advertisement to delete.
 *
 * @return void Doesn't return anything.
 */
void jxta_discovery_query_free(Jxta_DiscoveryQuery *);


/**
 * Constructs a representation of a discovery query advertisement in
 * xml format.
 *
 * @param Jxta_DiscoveryQuery * pointer to discovery query advertisement
 * @param JString ** address of pointer to JString that 
 *        accumulates xml representation of discovery query advertisement.
 *
 * @return Jxta_status 
 */
Jxta_status jxta_discovery_query_get_xml(/*Jxta_advertisement **/ Jxta_DiscoveryQuery * adv, JString **);


/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a discovery query advertisement structure.
 *
 * @param Jxta_DiscoveryQuery * pointer to discovery query
 *        advertisement to contain the data in the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return void Doesn't return anything.
 */
Jxta_status
jxta_discovery_query_parse_charbuffer(Jxta_DiscoveryQuery *, const char *, int len); 


/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a discovery query advertisement structure.
 *
 * @param Jxta_DiscoveryQuery * discovery response advertisement
 *        to contain the data in the xml.
 * @param FILE * an input stream.
 *
 * @return void Doesn't return anything.
 */
Jxta_status jxta_discovery_query_parse_file(Jxta_DiscoveryQuery *, FILE * stream);

/**
 * Gets the Type of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return char * containing the type.
 */ 
short 
jxta_discovery_query_get_type(Jxta_DiscoveryQuery *);


/**
 * Sets the Type of the discovery response advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param char * containing the Type.
 *
 * @return void Doesn't return anything.
 */ 
Jxta_status jxta_discovery_query_set_type(Jxta_DiscoveryQuery *, short );
 

/**
 * Gets the Threshold of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return char * containing the Threshold.
 */ 
int  jxta_discovery_query_get_threshold(Jxta_DiscoveryQuery *);


Jxta_status jxta_discovery_query_set_threshold(Jxta_DiscoveryQuery *, int );
 

/**
 * Gets the peer advertisement of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return char * containing the peer advertisement.
 */ 
Jxta_status 
jxta_discovery_query_get_peeradv(Jxta_DiscoveryQuery * ad, JString ** padv );


/**
 * Sets the peer advertisement of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param char * containing the peer advertisement.
 *
 * @return void Doesn't return anything.
 */ 
Jxta_status jxta_discovery_query_set_peeradv(Jxta_DiscoveryQuery *, JString *);
 

/**
 * Gets the Attr of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 *
 * @return char * containing the Attr.
 */ 
Jxta_status jxta_discovery_query_get_attr(Jxta_DiscoveryQuery *, JString ** );


/**
 * Sets the Attr of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param char * containing the Attr.
 *
 * @return void Doesn't return anything.
 */ 
Jxta_status jxta_discovery_query_set_attr(Jxta_DiscoveryQuery *, JString *);
 

/**
 * Sets the Value of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param char * containing the Value.
 *
 * @return void Doesn't return anything.
 */ 
Jxta_status jxta_discovery_query_get_value(Jxta_DiscoveryQuery *, JString **);


/**
 * Sets the Value of the discovery query advertisement.
 *
 * @param Jxta_DiscoveryQuery * a pointer to the discovery 
 *        query advertisement.
 * @param char * containing the Value.
 *
 * @return void Doesn't return anything.
 */ 
Jxta_status jxta_discovery_query_set_value(Jxta_DiscoveryQuery *, JString *);


#endif /* __Jxta_DiscoveryQuery_H__  */
#ifdef __cplusplus
}
#endif
