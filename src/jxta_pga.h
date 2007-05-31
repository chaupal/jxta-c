/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_pga.h,v 1.4 2005/02/02 02:58:29 exocetrick Exp $
 */

   
#ifndef JXTA_PGA_H__
#define JXTA_PGA_H__

#include "jxta_advertisement.h"

#include "jstring.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

typedef struct _jxta_PGA Jxta_PGA;

/**
 * Allocates memory for a new peer group advertisement.
 *
 * @param void Doesn't take any arguments.
 *
 * @return Jxta_PGA * peer group advertisement pointer.
 */
Jxta_PGA * jxta_PGA_new(void);


/**
 * Gets the xml representation of the peer group advertisement.
 *
 * @param Jxta_PGA * peer group advertisement pointer.
 * @param JString * contains buffer holding xml representation
 *        of peer group advertisement.
 *
 * @return void Doesn't return anything.
 */
Jxta_status jxta_PGA_get_xml(Jxta_PGA *ad, JString **string);


/**
 * Wrapper for jxta_advertisement_parse_charbuffer,
 * when call completes, the xml representation of a 
 * buffer is loaded into a peer group advertisement structure.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement to contain the data in 
 *        the xml.
 * @param const char * buffer containing characters 
 *        with xml syntax.
 * @param int len length of character buffer.
 *
 * @return void Doesn't return anything.
 */
void jxta_PGA_parse_charbuffer(Jxta_PGA *ad, const char *buf, int len); 


/**
 * Wrapper for jxta_advertisement_parse_file,
 * when call completes, the xml representation of a 
 * data is loaded into a peer group advertisement structure.
 *
 * @param Jxta_Jxta_PGA * peer advertisement to contain the data in 
 *        the xml.
 * @param FILE * an input stream.
 *
 * @return void Doesn't return anything.
 */
void jxta_PGA_parse_file(Jxta_PGA *ad, FILE * stream);
 

/**
 * This function is an artifact of the generating script.
 */
char * jxta_PGA_get_Jxta_PGA(Jxta_PGA *ad);


/**
 * This function is an artifact of the generating script.
 */
void jxta_PGA_set_Jxta_PGA(Jxta_PGA *ad, char *name);

 
/**
 * Function gets the Jxta_id associated with the peer 
 * group advertisement.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement.
 *
 * @return Jxta_id Jxta_id associated with peer group advertisement.
 */
Jxta_id * jxta_PGA_get_GID(Jxta_PGA *ad);

/**
 * Sets a Jxta_id associated with a peer group advertisement.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 * @param Jxta_id * a Jxta_id to be set for the peer group advertisement.
 *
 * @return void Doesn't return anything.
 */
void jxta_PGA_set_GID(Jxta_PGA *ad, Jxta_id *id);
 
/**
 * Function gets the Jxta_id associated with the peer 
 * group advertisement module services.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement.
 *
 * @return Jxta_id Jxta_id associated with peer group advertisement
 * module services.
 */
Jxta_id * jxta_PGA_get_MSID(Jxta_PGA *ad);

/**
 * Sets a Jxta_id associated with a peer group advertisement
 * module services.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 * @param Jxta_id * a Jxta_id to be set for the peer group advertisement
 * module services.
 *
 * @return void Doesn't return anything.
 */
void jxta_PGA_set_MSID(Jxta_PGA *ad, Jxta_id *id);
 
/**
 * Gets the name of the peer group.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 *
 * @return JString * containing peer group name.
 */
JString * jxta_PGA_get_Name(Jxta_PGA *ad);


/**
 * Sets the peer group name.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 * @param JString * peer group name is contained here.
 *
 * @return void Doesn't return anything.
 */
void jxta_PGA_set_Name(Jxta_PGA *ad, JString *name);
 

/**
 * Gets the peer group description.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 *
 * @return JString * peer group description.
 */
JString * jxta_PGA_get_Desc(Jxta_PGA *ad);

/**
 * Sets the peer group name.
 *
 * @param Jxta_Jxta_PGA * peer group advertisement
 * @param JString * peer group description.
 *
 * return void Doesn't return anything.
 */
void jxta_PGA_set_Desc(Jxta_PGA *ad, JString *desc);

  /**
   ** Return a vector of indexes to be applied to advertisement tags and attributes. 
   **
   ** The vector returns element/attribute pairs contained in a Jxta_index entry. If the
   ** attribute is NULL then the index is applied to the element. If an attribute is 
   ** desired the element name and attribute name are specified.
   **
   ** @return Jxta_vector: return a vector of element/attribute pairs in Jxta_index struct.
   **
  **/

Jxta_vector * jxta_PGA_get_indexes(void);

 
 
 
#ifdef __cplusplus
}
#endif

#endif /* JXTA_PGA_H__  */
