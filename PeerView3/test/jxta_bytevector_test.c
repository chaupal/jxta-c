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

#include <stdlib.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

#include "unittest_jxta_func.h"

/**
* Test the jxta_bytevector_new_0   function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_new_0(void)
{
    Jxta_bytevector *jbv = jxta_bytevector_new_0();
    JString *js;

    if (NULL == jbv)
        return FILEANDLINE;

    if (0 != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* 
     *   Check that the content of the bytevector is as expected  - i.e. 
     *  jstring_new_3 returns NULL as there are not data 
     */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js != NULL) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }

    return NULL;
}

/**
* Test the jxta_bytevector_new_1   function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_new_1(void)
{
    Jxta_bytevector *jbv = jxta_bytevector_new_1(100);
    JString *js;

    if (NULL == jbv)
        return FILEANDLINE;

    if (0 != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* 
     *   Check that the content of the bytevector is as expected  - i.e. 
     *  jstring_new_3 returns NULL as there are not data 
     */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js != NULL) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }

    return NULL;
}


/**
* Test the jxta_bytevector_new_2   function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_new_2(void)
{
    unsigned char *source;
    size_t size = 3;
    Jxta_bytevector *jbv;
    JString *js;

    source = (unsigned char *) malloc(sizeof(char) * 3);
    if (source == NULL) {
        return FILEANDLINE;
    }
    source[0] = 'a';
    source[1] = 'b';
    source[2] = 'c';

    jbv = jxta_bytevector_new_2(source, size, 100);
    if (NULL == jbv)
        return FILEANDLINE;

    if (size != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("abc", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }

    /* Release the allocated string */
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/** 
 * Allocate a new jxta_bytevector object which contains the indicated 
 * string. This is a helper function
 * 
 */
Jxta_bytevector *get_jxta_bytevector_object(const char *buffer)
{
    unsigned char *source;
    size_t size = strlen(buffer);
    Jxta_bytevector *jbv;

    source = (unsigned char *) malloc(sizeof(char) * (size + 1));
    if (source == NULL) {
        return NULL;
    }
    source[0] = '\0';
    strcat(source, buffer);

    jbv = jxta_bytevector_new_2(source, size, 100);
    if (NULL == jbv) {
        JXTA_OBJECT_RELEASE(jbv);
        return NULL;
    }

    if (size != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return NULL;
    }

    return jbv;

}

/**
* Test the jxta_clear function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_clear(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;


    jbv = get_jxta_bytevector_object("yahoo!");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Now clear the vector and test that the result is as expected */
    status = jxta_bytevector_clear(jbv, 50);
    if ((status != JXTA_SUCCESS) || (0 != jxta_bytevector_size(jbv))) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js != NULL) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }

    return NULL;
}


/**
* Test the jxta_bytevector_add_byte_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_byte_at(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("yahoo!");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_add_byte_at(jbv, '1', 1);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_byte_at(jbv, '5', 5);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_add_byte_at(jbv, '1', -1);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_byte_at(jbv, '5', 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("y1aho5o!", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_add_byte_first function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_byte_first(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("yahoo!");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    status = jxta_bytevector_add_byte_first(jbv, '1');
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("1yahoo!", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_add_byte_last function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_byte_last(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("yahoo!");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    status = jxta_bytevector_add_byte_last(jbv, 'x');
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("yahoo!x", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_add_bytes_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_bytes_at(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;
    static const unsigned char *source = "abcd";
    size_t size = 2;

    jbv = get_jxta_bytevector_object("123456");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_add_bytes_at(jbv, source, 0, size);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytes_at(jbv, source, 4, size);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytes_at(jbv, source, 10, size);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_add_bytes_at(jbv, source, -1, size);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytes_at(jbv, source, 20, size);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("ab12ab3456ab", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}

/**
* Test the jxta_bytevector_add_bytevector_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_bytevector_at(void)
{
    Jxta_bytevector *jbv, *inset;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("123456");
    if (NULL == jbv) {
        return FILEANDLINE;
    }
    inset = get_jxta_bytevector_object("ab");
    if (NULL == inset) {
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_add_bytevector_at(jbv, inset, 0);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytevector_at(jbv, inset, 4);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytevector_at(jbv, inset, 10);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_add_bytevector_at(jbv, inset, 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_bytevector_at(jbv, inset, -1);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    JXTA_OBJECT_RELEASE(inset);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("ab12ab3456ab", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_add_partial_bytevector_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_partial_bytevector_at(void)
{
    Jxta_bytevector *jbv, *inset;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("123456");
    if (NULL == jbv) {
        return FILEANDLINE;
    }
    inset = get_jxta_bytevector_object("abcdef");
    if (NULL == inset) {
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_add_partial_bytevector_at(jbv, inset, 2, 2, 0);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_partial_bytevector_at(jbv, inset, 0, 2, 4);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_partial_bytevector_at(jbv, inset, 4, 2, 10);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_add_partial_bytevector_at(jbv, inset, 100, -1, 2);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }
    status = jxta_bytevector_add_partial_bytevector_at(jbv, inset, 4, 2, -11);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        JXTA_OBJECT_RELEASE(inset);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    JXTA_OBJECT_RELEASE(inset);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("cd12ab3456ef", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_add_from_stream_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_add_from_stream_at(void)
{
    static const unsigned char *source = "abcd";
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;
    read_write_test_buffer stream;

    jbv = get_jxta_bytevector_object("123456");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    stream.buffer = (char *) source;
    stream.position = 0;
    status = jxta_bytevector_add_from_stream_at(jbv, readFromStreamFunction, (void *) (&stream), 2, 0);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    stream.position = 0;
    status = jxta_bytevector_add_from_stream_at(jbv, readFromStreamFunction, (void *) (&stream), 2, 4);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    stream.position = 0;
    status = jxta_bytevector_add_from_stream_at(jbv, readFromStreamFunction, (void *) (&stream), 2, 10);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("ab12ab3456ab", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_get_byte_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_get_byte_at(void)
{
    Jxta_bytevector *jbv;
    Jxta_status status;
    unsigned char byte;
    int i;

    jbv = get_jxta_bytevector_object("ABCDEF");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Try whether the right byte is returned  */
    for (i = 0; i < 6; i++) {
        status = jxta_bytevector_get_byte_at(jbv, &byte, i);
        if (status != JXTA_SUCCESS || byte != (65 + i)) {
            JXTA_OBJECT_RELEASE(jbv);
            return FILEANDLINE;
        }
    }

    /* Try it with a wrong argument */
    status = jxta_bytevector_get_byte_at(jbv, &byte, 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Release the object */
    JXTA_OBJECT_RELEASE(jbv);

    return NULL;
}


/**
* Test the jxta_bytevector_get_bytes_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_get_bytes_at(void)
{
    Jxta_bytevector *jbv;
    Jxta_status status;
    unsigned char byte[100];

    jbv = get_jxta_bytevector_object("ABCDEF");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Try to copy the whole vector */
    status = jxta_bytevector_get_bytes_at(jbv, byte, 0, INT_MAX);
    byte[6] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(byte, "ABCDEF")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_get_bytes_at(jbv, byte, 3, INT_MAX);
    byte[3] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(byte, "DEF")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Try to copy a portion of the vector */
    status = jxta_bytevector_get_bytes_at(jbv, byte, 0, 3);
    byte[4] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(byte, "ABC")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_get_bytes_at(jbv, byte, 2, 2);
    byte[2] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(byte, "CD")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Release the object */
    JXTA_OBJECT_RELEASE(jbv);

    return NULL;
}

/**
* Test the jxta_bytevector_get_bytevector_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_get_bytevector_at(void)
{
    Jxta_bytevector *jbv;
    Jxta_bytevector *dest;
    Jxta_status status;
    JString *js;

    jbv = get_jxta_bytevector_object("ABCDEF");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Try to copy the whole vector */
    status = jxta_bytevector_get_bytevector_at(jbv, &dest, 0, INT_MAX);
    if (status != JXTA_SUCCESS || NULL == dest || TRUE != jxta_bytevector_equals(dest, jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        if (dest != NULL)
            JXTA_OBJECT_RELEASE(dest);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(dest);
    status = jxta_bytevector_get_bytevector_at(jbv, &dest, 3, INT_MAX);
    if (dest != NULL)
        js = jstring_new_3(dest);
    else
        js = NULL;
    if (status != JXTA_SUCCESS || NULL == dest || NULL == js || 0 != strcmp("DEF", jstring_get_string(js))) {
        if (js != NULL) {
            JXTA_OBJECT_RELEASE(js);
        }
        JXTA_OBJECT_RELEASE(jbv);
        if (dest != NULL)
            JXTA_OBJECT_RELEASE(dest);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);
    JXTA_OBJECT_RELEASE(dest);


    /* Try to copy a portion of the vector */
    status = jxta_bytevector_get_bytevector_at(jbv, &dest, 0, 3);
    if (dest != NULL)
        js = jstring_new_3(dest);
    else
        js = NULL;
    if (status != JXTA_SUCCESS || NULL == dest || NULL == js || 0 != strcmp("ABC", jstring_get_string(js))) {
        if (js != NULL) {
            JXTA_OBJECT_RELEASE(js);
        }
        JXTA_OBJECT_RELEASE(jbv);
        if (dest != NULL)
            JXTA_OBJECT_RELEASE(dest);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);
    JXTA_OBJECT_RELEASE(dest);

    status = jxta_bytevector_get_bytevector_at(jbv, &dest, 2, 2);
    if (dest != NULL)
        js = jstring_new_3(dest);
    else
        js = NULL;
    if (status != JXTA_SUCCESS || NULL == dest || NULL == js || 0 != strcmp("CD", jstring_get_string(js))) {
        if (js != NULL) {
            JXTA_OBJECT_RELEASE(js);
        }
        JXTA_OBJECT_RELEASE(jbv);
        if (dest != NULL)
            JXTA_OBJECT_RELEASE(dest);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);
    JXTA_OBJECT_RELEASE(dest);

    /* Release the object */
    JXTA_OBJECT_RELEASE(jbv);

    return NULL;
}


/**
* Test the jxta_bytevector_write function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_write(void)
{
    Jxta_bytevector *jbv;
    char buffer[100];
    Jxta_status status;
    read_write_test_buffer stream;

    jbv = get_jxta_bytevector_object("ABCDEF");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Try to copy the whole vector */
    stream.buffer = buffer;
    stream.position = 0;
    buffer[0] = '\0';
    status = jxta_bytevector_write(jbv, writeFunction, (void *) (&stream), 0, INT_MAX);
    buffer[6] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(buffer, "ABCDEF")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    buffer[0] = '\0';
    stream.position = 0;
    status = jxta_bytevector_write(jbv, writeFunction, (void *) (&stream), 3, INT_MAX);
    buffer[3] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(buffer, "DEF")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Try to copy a portion of the vector */
    buffer[0] = '\0';
    stream.position = 0;
    status = jxta_bytevector_write(jbv, writeFunction, (void *) (&stream), 0, 3);
    buffer[3] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(buffer, "ABC")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    buffer[0] = '\0';
    stream.position = 0;
    status = jxta_bytevector_write(jbv, writeFunction, (void *) (&stream), 2, 2);
    buffer[2] = '\0';
    if (status != JXTA_SUCCESS || 0 != strcmp(buffer, "CD")) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Release the object */
    JXTA_OBJECT_RELEASE(jbv);

    return NULL;
}

/**
* Test the jxta_bytevector_remove_byte_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_remove_byte_at(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("abcdef");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_remove_byte_at(jbv, 0);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_byte_at(jbv, 2);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_byte_at(jbv, 3);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_remove_byte_at(jbv, -1);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_byte_at(jbv, 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("bce", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}


/**
* Test the jxta_bytevector_remove_bytes_at function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_remove_bytes_at(void)
{
    Jxta_bytevector *jbv;
    JString *js;
    Jxta_status status;

    jbv = get_jxta_bytevector_object("abcdefghij");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

  /** Try it with a valid argument */
    status = jxta_bytevector_remove_bytes_at(jbv, 0, 2);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_bytes_at(jbv, 2, 3);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_bytes_at(jbv, 3, 2);
    if (status != JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

  /** Try it with an  invalid argument */
    status = jxta_bytevector_remove_bytes_at(jbv, -1, 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }
    status = jxta_bytevector_remove_bytes_at(jbv, 20, 20);
    if (status == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }


    /* Check that the content of the bytevector is as expected  */
    js = jstring_new_3(jbv);
    JXTA_OBJECT_RELEASE(jbv);
    if (js == NULL) {
        return FILEANDLINE;
    }
    if (0 != strcmp("cdh", jstring_get_string(js))) {
        JXTA_OBJECT_RELEASE(js);
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(js);

    return NULL;
}

/**
* Test the jxta_bytevector_size function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_size(void)
{
    Jxta_bytevector *jbv;

    jbv = get_jxta_bytevector_object("123456");
    if (NULL == jbv) {
        return FILEANDLINE;
    }

    /* Check that the size is correct  */
    if (6 != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    /* Change jbv and repeat test */
    jxta_bytevector_remove_bytes_at(jbv, 2, 2);
    if (4 != jxta_bytevector_size(jbv)) {
        JXTA_OBJECT_RELEASE(jbv);
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(jbv);

    return NULL;
}



/**
* Test the jxta_bytevector_equals function
* 
* @return NULL if the test run successfully, FALSE otherwise
*/
const char * test_bytevector_equals(void)
{
    Jxta_bytevector *jb1;
    Jxta_bytevector *jb2;

    jb1 = get_jxta_bytevector_object("abcdefg");
    if (NULL == jb1) {
        return FILEANDLINE;
    }
    jb2 = get_jxta_bytevector_object("abcdefg");
    if (NULL == jb2) {
        return FILEANDLINE;
    }

    /* Try if they are equal */
    if (TRUE != jxta_bytevector_equals(jb1, jb2)) {
        JXTA_OBJECT_RELEASE(jb1);
        JXTA_OBJECT_RELEASE(jb2);
        return FILEANDLINE;
    }

    /* Change jb2 and repeat test */
    jxta_bytevector_remove_bytes_at(jb2, 2, 2);
    if (FALSE != jxta_bytevector_equals(jb1, jb2)) {
        JXTA_OBJECT_RELEASE(jb1);
        JXTA_OBJECT_RELEASE(jb2);
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(jb1);
    JXTA_OBJECT_RELEASE(jb2);

    return NULL;
}


static struct _funcs testfunc[] = {
    {*test_bytevector_new_0, "jxta_bytevector_new_0"},
    {*test_bytevector_new_1, "jxta_bytevector_new_1"},
    {*test_bytevector_new_2, "jxta_bytevector_new_2"},
    {*test_bytevector_clear, "jxta_bytevector_clear"},
    {*test_bytevector_add_byte_at, "jxta_bytevector_add_byte_at"},
    {*test_bytevector_add_byte_first, "jxta_bytevector_add_byte_first"},
    {*test_bytevector_add_byte_last, "jxta_bytevector_add_byte_last"},
    {*test_bytevector_add_bytes_at, "jxta_bytevector_add_bytes_at"},
    {*test_bytevector_add_bytevector_at, "jxta_bytevector_add_bytevector_at"},
    {*test_bytevector_add_partial_bytevector_at, "jxta_bytevector_add_partial_bytevector_at"},
    /*{*test_bytevector_add_from_stream_at,         "jxta_bytevector_add_from_stream_at"         }, Discepancy between  function description and function working */
    {*test_bytevector_get_byte_at, "jxta_bytevector_get_byte_at"},
    {*test_bytevector_get_bytes_at, "jxta_bytevector_get_bytes_at"},
    {*test_bytevector_get_bytevector_at, "jxta_bytevector_get_bytevector_at"},
    {*test_bytevector_write, "jxta_bytevector_write"},
    {*test_bytevector_remove_byte_at, "jxta_bytevector_remove_byte_at"},
    {*test_bytevector_remove_bytes_at, "jxta_bytevector_remove_bytes_at"},
    {*test_bytevector_size, "jxta_bytevector_size"},
    {*test_bytevector_equals, "jxta_bytevector_equals"},
    {NULL, "null"}
};

/**
* Run the unit tests for the jxta_bytevector test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return NULL if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_bytevector_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(testfunc, tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(testfunc, argc, argv);
}
#endif
