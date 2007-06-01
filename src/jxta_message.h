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
 * $Id: jxta_message.h,v 1.7 2006/05/26 02:49:51 bondolo Exp $
 */

#ifndef __JXTAMSG_H__
#define __JXTAMSG_H__

#include "jxta_object.h"

#include "jxta_bytevector.h"
#include "jxta_vector.h"
#include "jstring.h"
#include "jxta_endpoint_address.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
*  JXTA messages are opaque JXTA objects. Jxta messages are not thread-safe.
*  You must provide your own locking if you wish to use mutable messages from
*  multiple threads.
**/
typedef struct _Jxta_message const Jxta_message;

/**
*  JXTA elements are opaque JXTA objects. Jxta elements are effectively 
*  thread-safe since they are immutable after construction.
**/
typedef struct _Jxta_message_element const Jxta_message_element;

/**
 *  Creates a new empty Jxta Message. Jxta_message is a Jxta_Object.
 *
 *
 *  @return the new JXTA message or NULL for errors.
 **/
JXTA_DECLARE(Jxta_message *) jxta_message_new(void);

/**
 *  Copies a Jxta message. The copy can then be modified seperately from the
 *  originial. (JXTA_SHARE_OBJECT does not copy the message). The cloned message
 *  will contain the same elements as the original message.
 *
 *  @param msg The message to be copied.
 *  @return the new JXTA message or NULL for errors.
 **/
JXTA_DECLARE(Jxta_message *) jxta_message_clone(Jxta_message * msg);

/**
 * Reads a Jxta message from a "stream". Any
 * existing content of the message will be discarded.
 *
 * @param  msg The message to read elements into. 
 * @param  mime_type The mime-type of the stream from which the message will be
 * read. If the implementation does not support messages of this type,
 * JXTA_INVALID_ARGUMENT will be returned. If NULL then the default type,
 * "application/x-jxta-msg" will be used.
 * @param  read_func The function to be called to read message bytes.
 * @param  stream  The identifier which will be passed to read_func to identify
 * the stream.
 * @return  JXTA_SUCCESS if the message was read successfully.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_read(Jxta_message * msg, char const *mime_type, ReadFunc read_func, void *stream);

/**
 * Writes a Jxta message to a "stream".
 *
 * @param  msg The message to which will be written.
 * @param  mime_type The mime-type which will be written to of the stream.
 * If the implementation does not support messages of this type,
 * JXTA_INVALID_ARGUMENT will be returned. If NULL then the default type,
 * "application/x-jxta-msg" will be used.
 * @param  write_func The function to be called to write message bytes.
 * @param  stream  The identifier which will be passed to write_func to identify
 * the stream.
 * @return  JXTA_SUCCESS if the message was written successfully.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_write(Jxta_message * msg, char const *mime_type, WriteFunc write_func, void *stream);

JXTA_DECLARE(Jxta_endpoint_address *) jxta_message_get_source(Jxta_message * msg);

JXTA_DECLARE(Jxta_status) jxta_message_set_source(Jxta_message * msg, Jxta_endpoint_address * src);

JXTA_DECLARE(Jxta_endpoint_address *) jxta_message_get_destination(Jxta_message * msg);

JXTA_DECLARE(Jxta_status) jxta_message_set_destination(Jxta_message * msg, Jxta_endpoint_address * dst);

/**
 * Add an element to a message. The element is shared.
 *
 * @param  msg The meessage
 * @param el  The message element to be added.
 * @return  JXTA_SUCCESS if the element was added, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_add_element(Jxta_message * msg, Jxta_message_element * el);

/**
 * Get an element from a message. The element is shared.
 *
 * @deprecated This method is being removed as the constant reparsing of qname is inefficient.
 *
 * @param  msg the meessage
 * @param  qname The qualified (potentially including namespace) name of
 *  the element.
 * @param el  Will contain the element if it could be found.
 * @return  JXTA_SUCCESS if the element was removed, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_get_element_1(Jxta_message * msg, char const *qname, Jxta_message_element ** el);

/**
 * Removes an element from a message.
 *
 * @param  msg the meessage
 * @param  ns The namespace of the element to be found.
 * @param  ncname The unqualified name of the element to be found.
 * @param el  Will contain the element if it could be found.
 * @return  JXTA_SUCCESS if the element was removed, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_get_element_2(Jxta_message * msg,
                                                     char const *ns, char const *name, Jxta_message_element ** el);

JXTA_DECLARE(Jxta_vector *) jxta_message_get_elements(Jxta_message * msg);

JXTA_DECLARE(Jxta_vector *) jxta_message_get_elements_of_namespace(Jxta_message * msg, char const *Namespace);

/**
 * Removes an element from a message. The first instance of the element
 * will be removed.
 *
 * @param  msg The message to be modified
 * @param  el The element to be removed.
 * @return  JXTA_SUCCESS if the element was removed, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_remove_element(Jxta_message * msg, Jxta_message_element * el);

/**
 * Removes an element from a message. The first element matching qname will
 * be removed.
 *
 * @deprecated This method is being removed as the constant reparsing of qname is inefficient.
 *
 * @param  msg The message to be modified
 * @param  qname The qualified (potentially including namespace) name of
 *  the element.
 * @return  JXTA_SUCCESS if the element was removed, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_remove_element_1(Jxta_message * msg, char const *qname);

/**
 * Removes an element from a message. The first element matching name in the specificed namespace
 * will be removed.
 *
 * @param  msg The message to be modified
 * @param  ns The namespace of the element to be removed.
 * @param  ncname The unqualified name of the element to be removed.
 * @return  JXTA_SUCCESS if the element was removed, JXTA_ITEM_NOTFOUND if
 * it was not present in the message.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_remove_element_2(Jxta_message * msg, char const *ns, char const *name);

 /**
 * Convenience method that produces the content of a message in the
 * wire format and appends it to the provided JString. This method is intended
 * primarily for debuging purpose, but, it can be use in any context. 
 *
 * @param the message to be emitted to the Jstring
 * @param mime_type  the wire representation to be used. If NULL then the
 * "application/x-jxta-msg" representation will be used.
 * @param string the string to which the message will be appended.
 * @return JXTA_SUCCESS if the message could be generated, JXTA_INVALID_ARGUMENT
 * for invalid parameters.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_to_jstring(Jxta_message * msg, char const *mime_type, JString * string);

 /** 
 * Print on the console the content of a message.
 *
 * @param  msg the message to be printed.
 **/
JXTA_DECLARE(Jxta_status) jxta_message_print(Jxta_message * msg);

/**
 *
 * JXTA MESSAGE ELEMENT
 *
 **/


/**
 * Build a message element from components.
 *
 * @deprecated This method is being removed as the constant reparsing of qname is inefficient.
 * 
 * @param qname qualified (potentially including namespace) name for the element.
 * @param optional mimetype for element. defaults to 'application/octet-stream
 * if parameter is NULL.
 * @param value  the body of the element. THIS DATA IS COPIED.
 * @param length the length of the body.
 * @param sig    an optional associated element which may contain the signature
 * or message digest of this element. If NULL then no signature.
 * @return the resulting message element or NULL if it could not be allocated.
 **/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_1(char const *qname,
                                                                char const *mime_type,
                                                                char const *value, size_t length, Jxta_message_element * sig);

 /**
 * Build a message element from components.
 * 
 * @param ns namespace for the element
 * @param ncname unqalified name for the element
 * @param optional mimetype for element. defaults to 'application/octet-stream
 * if parameter is NULL.
 * @param value  the body of the element. THIS DATA IS COPIED.
 * @param length the length of the body.
 * @param sig    an optional associated element which may contain the signature
 * or message digest of this element. If NULL then no signature.
 * @return the resulting message element or NULL if it could not be allocated.
 **/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_2(char const *ns, char const *ncname,
                                                                char const *mime_type,
                                                                char const *value, size_t length, Jxta_message_element * sig);

 /**
 * Build a message element from components.
 * 
 * @param ns namespace for the element
 * @param ncname unqalified name for the element
 * @param optional mimetype for element. defaults to 'application/octet-stream
 * if parameter is NULL.
 * @param value  the body of the element.
 * @param length the length of the body.
 * @param sig    an optional associated element which may contain the signature
 * or message digest of this element. If NULL then no signature.
 * @return the resulting message element or NULL if it could not be allocated.
 **/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_new_3(char const *ns, char const *ncname,
                                                                char const *mime_type,
                                                                Jxta_bytevector * value, Jxta_message_element * sig);

/**
  @return the pointer returned is valid until the message element is destroyed.
  you dont have to dispose it.
 **/
JXTA_DECLARE(char const *) jxta_message_element_get_namespace(Jxta_message_element * el);

/**
  @return the pointer returned is valid until the message element is destroyed.
  you dont have to dispose it.
 **/
JXTA_DECLARE(char const *) jxta_message_element_get_name(Jxta_message_element * el);

/**
 * @return the pointer returned is valid until the message element is destroyed.
 * you dont have to dispose it.
 **/
JXTA_DECLARE(char const *) jxta_message_element_get_mime_type(Jxta_message_element * el);

/**
 * Get the value of this element. The returned byte vector is shared and
 * must be released when you are finished with it.
 *
 * @param  el the element who's value is desired
 * @return the byte vector.
 **/
JXTA_DECLARE(Jxta_bytevector *) jxta_message_element_get_value(Jxta_message_element * el);

/**
 * Return a shared version of the signature element for this element.
 *
 * @param  the element who's signature is desired.
 * @return the signature element for this element.
 **/
JXTA_DECLARE(Jxta_message_element *) jxta_message_element_get_signature(Jxta_message_element * el);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif

/* vi: set ts=4 sw=4 tw=130 et: */
