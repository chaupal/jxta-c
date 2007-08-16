/*
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights
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
 * $Id$
 */


/**
 * JString provides an encapsulation of c-style character 
 * buffers.
 *
 *
 * @warning JString is not thread-safe.
 */

#ifndef __JXTA_STRING_H__
#define __JXTA_STRING_H__


#include <stddef.h>

#include "jxta_object.h"
#include "jxta_bytevector.h"


#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

typedef struct _jstring JString;

/**
 * Returns a pointer to a new JString with 
 * a fixed number of bytes of shredded memory
 * and '\0' in the initial position.  The number
 * of bytes for the initial preallocation is preset.
 */
JXTA_DECLARE(JString *) jstring_new_0(void);

/**
 * Returns a pointer to a user-specified number
 * of bytes of shredded memory with a '\0' in the
 * initial position.
 *
 * @todo Check that the ending char is \0
 */
JXTA_DECLARE(JString *) jstring_new_1(size_t initialSize);

/**
 * Constructs a JString from a constant character 
 * array.
 *
 */
JXTA_DECLARE(JString *) jstring_new_2(const char *);

/**
 * Constructs a JString from a byte vector
 * array.
 *
 * @todo Check that the ending char is \0
 */
JXTA_DECLARE(JString *) jstring_new_3(Jxta_bytevector *);

/**
 * Returns a pointer to a copy of the character 
 * data of the argument.
 *
 * @todo Maybe this should return an exact copy of
 *       argument?
 */
JXTA_DECLARE(JString *) jstring_clone(JString const *);

/**
 * Trims white-space from JString.  white-space is defined in
 * the "C" and "POSIX" locales, these are: space, form-feed ('\f'),
 * newline ('\n'), carriage return ('\r'), horizontal tab ('\t'), 
 * and vertical tab ('\v')
 */

JXTA_DECLARE(void) jstring_trim(JString *);

/** 
 * Each JString has a data length and buffer size.
 * This returns the length of the data in the char
 * buffer.
 */
JXTA_DECLARE(size_t) jstring_length(JString const *);

/** 
 * Each JString has a data length and buffer size.
 * This returns the length of the data in the char
 * buffer.
 */
JXTA_DECLARE(size_t) jstring_capacity(JString const *);

/**
*   Compare two strings in the same way that 'strcmp' does.
*/
JXTA_DECLARE(int) jstring_equals(JString const *me, JString const *you);

/**
 * Returns a C-style character buffer containing
 * NULL-terminated data.
 */
JXTA_DECLARE(char const *) jstring_get_string(JString const *js);

/**
 * Reset the JString.
 * If buf is NULL, the string contained in the JString is freed.
 * If buf is not NULL, the string contained in the JString is not freed
 * and is returned into buf. The caller is then responsible for freeing
 * (with free) the string.
 *
 * @param js a pointer to a JString
 * @param buf a pointer to a pointer that will contained the pointer to
 * the string.
 * @return JXTA_INVALID_ARGUMENT if an argument is invalid, JXTA_SUCCESS
 * otherwise.
 **/
JXTA_DECLARE(Jxta_status) jstring_reset(JString * js, char **buf);


JXTA_DECLARE(void) jstring_print(JString const *, PrintFunc printer, void *stream);

/* Can't "write" to a SOCKET in Win32. */
JXTA_DECLARE(void) jstring_send(JString const *, SendFunc sender, void *stream, unsigned int flags);

JXTA_DECLARE(void) jstring_write(JString const *, WriteFunc writer, void *stream);

JXTA_DECLARE(void) jstring_append_0(JString * oldstring, char const *newstring, size_t length);

JXTA_DECLARE(void) jstring_append_1(JString * oldstring, JString * newstring);

JXTA_DECLARE(void) jstring_append_2(JString * oldstring, const char *newstring);

JXTA_DECLARE(void) jstring_concat(JString * oldstring, int number, ...);

JXTA_DECLARE(int) jstring_writefunc_appender(void *stream, const char *buf, size_t len, Jxta_status * res);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_STRING_H__ */
