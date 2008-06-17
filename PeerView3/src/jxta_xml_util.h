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
 * $Id$
 */

/** The utility functions in this file could be implemented 
 *  in each of the advertisement parser interfaces, but 
 *  collecting everything here makes it easier to fix bugs.
 */

#include "jxta_types.h"
#include "jstring.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
};
#endif

/** Single call to extract an ip address and port number from 
 *  a character buffer. 
 *
 * @param  string contains character data in the form "127.0.0.1:9700"
 *         corresponding to ip address and port number.
 * @param  length is necessary because of the way strtok works.  We
 *         don't want to operate on possibly const char data, so we 
 *         need to copy it to a temporary buffer, which is statically 
 *         preallocated.  Copying only the first length chars prevents 
 *         buffer nastiness.
 * @param  ip points to a calling address.  What we really want is to 
 *         return two values which could be done by dropping the value
 *         of a struct on the stack, or passing in addresses.  Passing 
 *         in addresses works for now.
 * @param  port works the same way as the ip address does, represents the 
 *         second return address.
 * 
 * @return Two return arguments, ip and port, are passed back through 
 *         pointers in the function argument list.
 *
 * @todo   Make sure that apr_strtok is reentrant.
 *
 * @todo   Implement some way for this function to degrade gracefully
 *         if someone tries to ram a huge buffer into it.  Or maybe that 
 *         won't be an issue. At any rate, the behavior for data of 
 *         length 129 should be documented.
 *
 * @todo   Implement some of way handling bogus data nicely. For example,
 *         if someone feeds us something like "foo.bar.baz.foo:abcd", 
 *         it would be nice to do something intelligent with it rather 
 *         than relying on the underlying library to barf.  Or at least 
 *         catch the barf here.
 */
JXTA_DECLARE(void) extract_ip_and_port(const char *buf, int buf_length, Jxta_in_addr * ip_address, Jxta_port * port_number);

/** Single call to extract an ip address from 
 *  a character buffer. 
 *
 * @param  buf contains character data in the form "127.0.0.1"
 *         corresponding to ip address.
 * @param  buf_length is necessary because of the way strtok works.  We
 *         don't want to operate on possibly const char data, so we 
 *         need to copy it to a temporary buffer, which is statically 
 *         preallocated.  Copying only the first length chars prevents 
 *         buffer nastiness.
 * @param  ip_address calling address.
 * 
 * @return ip_address is passed back through 
 *         pointers in the function argument list. 
 *
 * @todo   Make sure that apr_strtok is reentrant.
 *
 * @todo   Implement some way for this function to degrade gracefully
 *         if someone tries to ram a huge buffer into it.  Or maybe that 
 *         won't be an issue. At any rate, the behavior for data of 
 *         length 129 should be documented.
 *
 * @todo   Implement some of way handling bogus data nicely. For example,
 *         if someone feeds us something like "foo.bar.baz.foo:abcd", 
 *         it would be nice to do something intelligent with it rather 
 *         than relying on the underlying library to barf.  Or at least 
 *         catch the barf here.
 * 
 */
JXTA_DECLARE(void) extract_ip(const char *buf, int buf_length, Jxta_in_addr * ip_address);

/** Single call to extract a port number from 
 *  a character buffer. 
 *
 * @param  buf contains character data in the form "9700"
 *         corresponding to port number.
 * @param  buf_length is necessary because of the way strtok works.  We
 *         don't want to operate on possibly const char data, so we 
 *         need to copy it to a temporary buffer, which is statically 
 *         preallocated.  Copying only the first length chars prevents 
 *         buffer nastiness.
 * @param  port number.
 * 
 * @return Two return arguments, ip and port, are passed back through 
 *         pointers in the function argument list.
 *
 * @todo   Make sure that apr_strtok is reentrant.
 *
 * @todo   Implement some way for this function to degrade gracefully
 *         if someone tries to ram a huge buffer into it.  Or maybe that 
 *         won't be an issue. At any rate, the behavior for data of 
 *         length 129 should be documented.
 *
 * @todo   Implement some of way handling bogus data nicely. For example,
 *         if someone feeds us something like "foo.bar.baz.foo:abcd", 
 *         it would be nice to do something intelligent with it rather 
 *         than relying on the underlying library to barf.  Or at least 
 *         catch the barf here.
 */
JXTA_DECLARE(void) extract_port(const char *buf, int buf_length, Jxta_port * port);

/** Function invokes apr_strtok to eliminate whitespace and 
 *  grab a token, primarily useful for grabbing a string 
 *  of non-whitespace know to be together.  May need further
 *  tweaking in the future.
 *
 * @param  string has char data
 * 
 * @param  length as determined by expat.
 *
 * @param  tok something to copy the string into because we can't 
 *         depend on stuff in apr_strtok.
 *
 * @return tok the character data snipped out. 
 */
JXTA_DECLARE(void) extract_char_data(const char *string, int strlength, char *tok);

/**
*  Removes escapes from the contents of a JString that has been passed as XML
*  element content.
*  
* @param src the string to decode.
* @param dest the resulting decoded string.
* @result JXTA_SUCCESS for success, JXTA_INVALID_ARGUMENT for bad params and
*  (rarely) JXTA_NOMEM
**/
JXTA_DECLARE(Jxta_status) jxta_xml_util_decode_jstring(JString * src, JString ** dest);

/**
*   Escapes the contents of a JString to make it suitable for passing as xml
*   element content.
*  
* @param src the string to encode.
* @param dest the resulting encoded string.
* @result JXTA_SUCCESS for success, JXTA_INVALID_ARGUMENT for bad params and
*  (rarely) JXTA_NOMEM
**/
JXTA_DECLARE(Jxta_status) jxta_xml_util_encode_jstring(JString * src, JString ** dest);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

/* vi: set ts=4 sw=4 tw=130 et: */
