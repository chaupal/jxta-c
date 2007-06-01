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
 * $Id: jstring.c,v 1.67 2006/04/04 18:19:46 slowhog Exp $
 */


/* Notes from info libc (I can't ever remember this crap).
 * This stuff is pasted directly in from the info page.
 *
 *  - Function: char * strcpy (char *restrict TO, 
 *                             const char *restrict FROM)
 *    This copies characters from the string FROM (up to and including
 *    the terminating null character) into the string TO.  Like
 *    `memcpy', this function has undefined results if the strings
 *    overlap.  The return value is the value of TO.
 *
 *  - Function: char * strncpy (char *restrict TO, 
 *                              const char *restrict FROM, 
 *                              size_t SIZE)
 *    This function is similar to `strcpy' but always copies exactly
 *    SIZE characters into TO.
 * 
 *  - Function: size_t strlen (const char *S)
 *    The `strlen' function returns the length of the null-terminated
 *    string S in bytes.  (In other words, it returns the offset of the
 *    terminating null character within the array.)
 *
 *    For example,
 *         strlen ("hello, world")
 *             => 12
 *
 */


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>  /* isspace */

#include "jxta_errno.h"
#include "jxta_debug.h"
#include "jstring.h"

const size_t DEFAULTBUFSIZE = 128;

/**
 * @todo  Does the length include or not include the terminating
 * '\0'?  Is the following code consistent with whichever convention 
 * is being used?  Propose to define "length" as number of characters 
 * in charbuf, and ensure that the buffer size is always at least 
 * length + 1.
 *
 * @todo Rename "string" member to "charbuf". 
 */
struct _jstring {
    JXTA_OBJECT_HANDLE;
    size_t length;
    size_t bufsize;
    char *string;
};

static void jstring_delete(Jxta_object * js);

/**
 * @todo Find a way to make sure we get a '\0' in the 
 * correct place after realloc.
 */
static char *resizeBuf(char *p, size_t size)
{
    return (char *) realloc(p, size);
}

static char *newCharBuf(size_t size)
{
    char *buf = (char *) calloc(size + 1, sizeof(char));

    return buf;
}

static void deleteCharBuf(char *buf, size_t size)
{
    /**
     * Definitely shred going out.
     */
    memset(buf, 0xdd, size);

    free(buf);
}

/* Proposed changes: please comment
 * 1. ensure null termination on concat, appends, etc.
 * 2. remove null term check from get_string and get_charbuf_copy
 * 3. rename get_string to get_charbuf because that is what it does.
 */


JXTA_DECLARE(JString *) jstring_new_0()
{
    return jstring_new_1(DEFAULTBUFSIZE);
}


JXTA_DECLARE(JString *) jstring_new_1(size_t bufsize)
{
    JString *js;

    if (bufsize <= 0)
        bufsize = DEFAULTBUFSIZE;

    js = (JString *) calloc(1, sizeof(JString));

    if (NULL == js)
        return NULL;

    JXTA_OBJECT_INIT(js, jstring_delete, 0);

    js->bufsize = bufsize;
    js->length = 0;
    js->string = newCharBuf(bufsize);

    return js;
}


JXTA_DECLARE(JString *) jstring_new_2(const char *string)
{
    JString *js;
    size_t length;

    if (NULL == string)
        return NULL;

    length = strlen(string);

    /* adjust the bufsize only if its zero. we want to use source strings own
       size if its non-zero because we may never grow the string. */

    if (length == 0)
        js = jstring_new_1(DEFAULTBUFSIZE);
    else
        js = jstring_new_1(length);

    if (NULL == js)
        return NULL;

    jstring_append_0(js, string, length);

    return js;
}

JXTA_DECLARE(JString *) jstring_new_3(Jxta_bytevector * bytes)
{
    JString *js;
    size_t length;

    JXTA_OBJECT_CHECK_VALID(bytes);

    if (NULL == bytes)
        return NULL;

    length = jxta_bytevector_size(bytes);
    js = jstring_new_1(length);

    if (NULL == js)
        return NULL;

    if (JXTA_SUCCESS != jxta_bytevector_get_bytes_at(bytes, (unsigned char *) js->string, 0, length)) {
        JXTA_OBJECT_RELEASE(js);
        return NULL;
    };

    js->length = length;

    return js;
}


/* test this using strcmp and strcncmp */
/**
 *  This function clones just the data in the 
 *  buffer, when maybe it should clone the entire
 *  buffer using memset or whatever.
 */
JXTA_DECLARE(JString *) jstring_clone(JString const *origstring)
{
    JString *js = jstring_new_1(origstring->bufsize);
    jstring_append_2(js, jstring_get_string(origstring));

    return js;
}


JXTA_DECLARE(void) jstring_trim(JString * js)
{
    size_t len = js->length;
    unsigned int st = 0;
    char *val = js->string;
    /* strip out leading white-space */
    while ((st < len) && (isspace(js->string[st]))) {
        st++;
        val++;
    }

    /* strip out trailing white-space */
    while ((st < len) && (isspace(val[len - st - 1]))) {
        len--;
    }
    /* did the string change? */
    if (st > 0 || len < js->length) {
        memmove(js->string, val, len - st);
        js->length = len - st;
        js->string[len - st] = 0;
    }
}

JXTA_DECLARE(size_t) jstring_length(JString const *js)
{
    return js->length;
}

JXTA_DECLARE(size_t) jstring_capacity(JString const *js)
{
    return js->bufsize;
}

static void jstring_delete(Jxta_object * obj)
{
    JString *js = (JString *) obj;

    if (NULL == js)
        return;

    deleteCharBuf(js->string, js->bufsize);
    memset(js, 0xdd, sizeof(JString));
    free(js);
}

JXTA_DECLARE(void) jstring_append_0(JString * js, char const *newstring, size_t length)
{
    JXTA_OBJECT_CHECK_VALID(js);

    if (NULL == newstring) {
        JXTA_LOG("NULL string passed to append_0\n");
        return; /* error */
    }

    /* ensure capacity +1 (for terminal \0) */
    while (js->bufsize <= (js->length + length + 1)) {
        js->bufsize *= 2;
        js->string = resizeBuf(js->string, js->bufsize);
    }

    memmove(&js->string[js->length], newstring, length);
    js->length += length;

    /* Proposed change: ensure that char buf is null termed. */

}

JXTA_DECLARE(void) jstring_append_1(JString * jsorig, JString * jsnew)
{
    JXTA_OBJECT_CHECK_VALID(jsorig);
    JXTA_OBJECT_CHECK_VALID(jsnew);

    jstring_append_0(jsorig, jsnew->string, jsnew->length);
}

JXTA_DECLARE(void) jstring_append_2(JString * js, char const *string)
{
    JXTA_OBJECT_CHECK_VALID(js);

    if (NULL == string) {
        JXTA_LOG("NULL string passed to append_2\n");
        return; /* error */
    }

    jstring_append_0(js, string, strlen(string));
}

/* ... are const char* */
JXTA_DECLARE(void) jstring_concat(JString * js, int number, ...)
{
    va_list argv;
    const char *s;
    int i;

    va_start(argv, number);
    for (i = 0; i < number; i++) {
        s = va_arg(argv, const char *);
        jstring_append_2(js, s);
    }
    va_end(argv);

    /* Proposed change: ensure that char buf is null termed. */
}

/**
 * Write some more test code for this so that when the 
 * implementation changes, it can be verified.
 *
 * @todo Move null termination check elsewhere.
 */
JXTA_DECLARE(char const *) jstring_get_string(JString const *js)
{
    /*  by const we mean that we wont change the contents. we wont. */
    JString *myjs = (JString *) js;

    JXTA_OBJECT_CHECK_VALID(myjs);

    /* Proposed change: ensuring null term for other ops simplifies this
     * code. 
     */

    /*  we need to make sure there is a null on the string. */
    if (myjs->length == myjs->bufsize) {
        myjs->string = resizeBuf(myjs->string, myjs->bufsize + 1);
        myjs->bufsize++;
    }

    *(myjs->string + (ptrdiff_t) myjs->length) = 0;

    return myjs->string;
}

JXTA_DECLARE(Jxta_status) jstring_reset(JString * js, char **buf)
{
    JString *myjs = (JString *) js;

    JXTA_OBJECT_CHECK_VALID(myjs);

    if (myjs == NULL) {
        JXTA_LOG("JString is null\n");
        return JXTA_INVALID_ARGUMENT;
    }

    if (buf != NULL) {
        *buf = (char *) jstring_get_string(myjs);
    } else {
        deleteCharBuf(js->string, js->bufsize);
    }

    /* Reset the string */

    myjs->bufsize = DEFAULTBUFSIZE;
    myjs->length = 0;
    myjs->string = newCharBuf(DEFAULTBUFSIZE);
    return JXTA_SUCCESS;
}

JXTA_DECLARE(void) jstring_print(JString const *js, PrintFunc printer, void *stream)
{
    printer(stream, "%s\n", jstring_get_string(js));
}

JXTA_DECLARE(void) jstring_send(JString const *js, SendFunc sender, void *stream, unsigned int flags)
{
    sender(stream, js->string, js->length, flags);
}

JXTA_DECLARE(void) jstring_write(JString const *js, WriteFunc writer, void *stream)
{
    writer(stream, js->string, js->length);
}

JXTA_DECLARE(int) jstring_writefunc_appender(void *stream, const char *buf, size_t len, Jxta_status * res)
{
    JString *string = (JString *) stream;
    Jxta_status status = JXTA_SUCCESS;

    if (!JXTA_OBJECT_CHECK_VALID(string)) {
        status = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    if (NULL == buf) {
        status = JXTA_INVALID_ARGUMENT;
        goto Common_Exit;
    }

    jstring_append_0(string, buf, len);

  Common_Exit:
    if (NULL != res)
        *res = status;

    return (JXTA_SUCCESS == status) ? len : -1;
}

/* vim: set ts=4 sw=4 et tw=130: */
