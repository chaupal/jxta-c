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
 * $Id: jxta_pipe_adv.h,v 1.10 2005/11/26 16:38:02 mmx2005 Exp $
 */

/************************************************************************
 ** This file describes the public API of the JXTA Pipe Advertisement.
 ************************************************************************/

#ifndef __Jxta_pipe_adv_H__
#define __Jxta_pipe_adv_H__

#include "jxta_advertisement.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

  /**
   ** Jxta_pipe_adv
   **
   ** Definition of the opaque type of the JXTA Pipe Advertisement.
   **/
typedef struct _jxta_pipe_adv Jxta_pipe_adv;

  /**
   ** Creates a new pipe advertisement object. The object is created
   ** empty, and is a Jxta_advertisement.
   **
   ** @return a pointer to a new empty Jxta_pipe_adv. Returns NULL if the
   ** system is out of memory.
   **/

JXTA_DECLARE(Jxta_pipe_adv *) jxta_pipe_adv_new(void);

  /**
   ** Builds and returns the XML (wire format) of the pipe advertisement.
   **
   ** @param adv a pointer to the advertisement object.
   ** @param xml return value: a pointer to a pointer which will the generated XML
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_get_xml(Jxta_pipe_adv * adv, JString ** xml);

  /**
   ** Parse an XML (wire format) document containing a JXTA pipe advertisement from
   ** a buffer.
   **
   ** @param adv a pointer to the advertisement object.
   ** @param buffer a pointer to the buffer where the advertisement is stored.
   ** @param len size of the buffer
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(void) jxta_pipe_adv_parse_charbuffer(Jxta_pipe_adv *, const char *buffer, int len);

  /**
   ** Parse an XML (wire format) document containing a JXTA pipe advertisement from
   ** a FILE.
   **
   ** @param adv a pointer to the advertisement object.
   ** @param stream the stream to get the advertisement from.
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(void) jxta_pipe_adv_parse_file(Jxta_pipe_adv *, FILE * stream);

  /**
   ** Gets the unique identifier of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @returns a pointer to a null terminated string containing the identifier. The lifetime
   ** of the returned string is the same as the advertisement object.
   **/

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Id(Jxta_pipe_adv * adv);

  /**
   ** Gets the unique identifier of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @returns a pointer to a JString containing the identifier.
   **/

JXTA_DECLARE(Jxta_id *) jxta_pipe_adv_get_pipeid(Jxta_pipe_adv * adv);

  /**
   ** Sets the unique identifier of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @param id a pointer to a null terminated string containing the id.
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Id(Jxta_pipe_adv * adv, const char *id);

  /**
   ** Gets the type of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @returns a pointer to a null terminated string containing the type. The lifetime
   ** of the returned string is the same as the advertisement object.
   **/

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Type(Jxta_pipe_adv * adv);

  /**
   ** Sets the type of the Pipe Advertisement.
   ** Types are described in jxta_pipe_service.h
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @param type a pointer to a null terminated string containing the type.
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Type(Jxta_pipe_adv * adv, const char *type);

  /**
   ** Gets the name of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @returns a pointer to a null terminated string containing the name. The lifetime
   ** of the returned string is the same as the advertisement object.
   **/

JXTA_DECLARE(const char *) jxta_pipe_adv_get_Name(Jxta_pipe_adv * adv);

  /**
   ** Sets the name of the Pipe Advertisement.
   **
   ** @param adv a pointer to the Pipe Advertisement.
   ** @param name a pointer to a null terminated string containing the name.
   ** @return a status: JXTA_SUCCESS when the call was successful,
   **                   JXTA_NOMEM when the system has ran out of memory
   **                   JXTA_INVALID_PARAMETER when an argument was invalid.
   **/

JXTA_DECLARE(Jxta_status) jxta_pipe_adv_set_Name(Jxta_pipe_adv * adv, const char *name);
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
JXTA_DECLARE(Jxta_vector *) jxta_pipe_adv_get_indexes(Jxta_advertisement *);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif
#endif /* __Jxta_pipe_adv_H__  */

/* vi: set ts=4 sw=4 tw=130 et: */
