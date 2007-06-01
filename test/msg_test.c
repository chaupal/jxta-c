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
 * $Id: msg_test.c,v 1.29 2006/09/29 06:52:06 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>

#include <jxta.h>
#include <jxta_errno.h>

#include "unittest_jxta_func.h"

/****************************************************************
 **
 ** This test program tests the JXTA message code. 
 **
 ****************************************************************/

static unsigned char const signature_content[] = { 255, 254, 1, 200, 12, 0, 13, 244 };
static unsigned char const element_content[] = { 7, 65, 163, 85, 213, 174, 26, 98 };


static char const * test_jxta_message_read_write(int version);

/*
 * Test functions for Message elements
 */

/** Create a signature element.
* The name is sig:MySig, the mime type MySigMime
* 
* @return a Jxta_message_element that constitutes a signature
*/
Jxta_message_element *createSignatureElement(void)
{
    Jxta_message_element *sig = jxta_message_element_new_2("sig", "MySig", "MySigMime", signature_content, sizeof(*signature_content), NULL);

    return sig;
}

/**
 * Check whether the signature element was returned correctly
 *
 * @return NULL for success otherwise a message indicating failure.
 */
const char * checkSignatureObject(Jxta_message_element * sig)
{
    const char *comparer;
    Jxta_bytevector *content;
    const char * result = NULL;
    int each_sig_byte;

    if (sig == NULL)
        return FILEANDLINE "signature null";

    /* Check the namespace */
    comparer = jxta_message_element_get_namespace(sig);
    if (strcmp(comparer, "sig") != 0)
        return FILEANDLINE;

    /* Check the name */
    comparer = jxta_message_element_get_name(sig);
    if (strcmp(comparer, "MySig") != 0)
        return FILEANDLINE;

    /* Check the mime type */
    comparer = jxta_message_element_get_mime_type(sig);
    if (strcmp(comparer, "MySigMime") != 0)
        return FILEANDLINE;

   /** Check the content of the signature */
    content = jxta_message_element_get_value(sig);
    if (jxta_bytevector_size(content) != 8) {
        JXTA_OBJECT_RELEASE(content);
        return FILEANDLINE;
    }
    result = NULL;
    
    for( each_sig_byte = 0; each_sig_byte < sizeof(*signature_content); each_sig_byte++ ) {
        unsigned char byte;
    
        jxta_bytevector_get_byte_at(content, &byte, each_sig_byte );
        
        if( signature_content[each_sig_byte] != byte ) {
            return FILEANDLINE;
        }
    }
    
    JXTA_OBJECT_RELEASE(content);

    if (jxta_message_element_get_signature(sig) != NULL)
        return FILEANDLINE;

    return NULL;
}


/**
* Test the jxta_message_element_new_1 function
* 
* @return NULL for success otherwise a message indicating failure.
*/
const char * test_jxta_message_element_new_1(void)
{
    const char * result = NULL;
    Jxta_message_element *el = NULL, *sig = NULL;
    Jxta_bytevector *retContent;
    const char *comparer;
    unsigned char *comparer_2;

    /* First initialize the content */

    /* Create an element with no namespace, no mime type and no signature */
    el = jxta_message_element_new_1("NewElement", NULL, element_content, sizeof(*element_content), NULL);
    if (el == NULL) {
        result = FILEANDLINE "could not create message element";
        goto Common_Exit;
    }
    
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("", comparer) != 0) {
        result = FILEANDLINE "namespace didn't match";
        goto Common_Exit;
    }
    
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE "name didn't match";
        goto Common_Exit;
    }
    
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("application/octet-stream", comparer) != 0) {
        result = FILEANDLINE "mime didn't match";
        goto Common_Exit;
    }
    
    if (jxta_message_element_get_signature(el) != NULL) {
        result = "signature should be NULL";
        goto Common_Exit;
    }
    
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    
    if( sizeof(*element_content) != jxta_bytevector_size(retContent) ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    
    comparer_2 = calloc( 1, sizeof(*element_content) );
    jxta_bytevector_get_bytes_at( retContent, comparer_2, 0, sizeof(*element_content) );
    
    if( 0 != memcmp( element_content, comparer_2, sizeof(*element_content) ) ) {
        free(comparer_2);
        result = FILEANDLINE;
        goto Common_Exit;
    }

    free(comparer_2);
    
    JXTA_OBJECT_RELEASE(el);
    el = NULL;

    /* Create an element with a namespace, a mimetype and a signature */
    sig = createSignatureElement();
    if (sig == NULL) {
        result = FILEANDLINE "could not create signature message element";
        goto Common_Exit;
    }
    el = jxta_message_element_new_1("MySpace:NewElement", "MyMime", element_content, sizeof(*element_content), sig);
    if (el == NULL) {
        result = FILEANDLINE "could not create message element";
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(sig);
    sig = NULL;
    
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("MySpace", comparer) != 0) {
        result = FILEANDLINE "namespace didn't match";
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE "name didn't match";
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("MyMime", comparer) != 0) {
        result = FILEANDLINE "mime didn't match";
        goto Common_Exit;
    }
    if (!checkSignatureObject(jxta_message_element_get_signature(el))) {
        result = "signature didn't match";
        goto Common_Exit;
    }
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    
    if( sizeof(*element_content) != jxta_bytevector_size(retContent) ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    
    comparer_2 = calloc( 1, sizeof(*element_content) );
    jxta_bytevector_get_bytes_at( retContent, comparer_2, 0, sizeof(*element_content) );
    
    if( 0 != memcmp( element_content, comparer_2, sizeof(*element_content) ) ) {
        free(comparer_2);
        result = FILEANDLINE;
        goto Common_Exit;
    }

    free(comparer_2);

    JXTA_OBJECT_RELEASE(el);
    el = NULL;

  Common_Exit:
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);
    if (sig != NULL)
        JXTA_OBJECT_RELEASE(sig);
    if (retContent != NULL)
        JXTA_OBJECT_RELEASE(retContent);

    return result;
}


/**
* Test the jxta_message_element_new_2 function
* 
* @return NULL for success otherwise a message indicating failure.
*/
char const * test_jxta_message_element_new_2(void)
{
    Jxta_message_element *el = NULL, *sig = NULL;
    const char *comparer;
    Jxta_bytevector *retContent = NULL, *compContent = NULL;
    char const * result = NULL;

    /* First initialize the content */
    compContent = jxta_bytevector_new_3(element_content, sizeof(*element_content), FALSE);
    if (compContent == NULL)
        return FILEANDLINE;

    /* Create an element with no namespace, no mime type and no signature */
    el = jxta_message_element_new_2("", "NewElement", NULL, element_content, sizeof(*element_content), NULL);
    if (el == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("application/octet-stream", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (jxta_message_element_get_signature(el) != NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL || !jxta_bytevector_equals(compContent, retContent)) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(retContent);
    retContent = NULL;

    /* Create an element with a namespace, a mimetype and a signature */
    sig = createSignatureElement();
    if (sig == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    el = jxta_message_element_new_2("MySpace", "NewElement", "MyMime", element_content, sizeof(*element_content), sig);
    if (el == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("MySpace", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("MyMime", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (!checkSignatureObject(jxta_message_element_get_signature(el))) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL || !jxta_bytevector_equals(compContent, retContent)) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(retContent);
    retContent = NULL;

  Common_Exit:
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);
    if (sig != NULL)
        JXTA_OBJECT_RELEASE(sig);
    if (retContent != NULL)
        JXTA_OBJECT_RELEASE(retContent);
    if (compContent != NULL)
        JXTA_OBJECT_RELEASE(compContent);
    /* Do not need to free  content, as compContent takes care of that */

    return result;
}


/**
* Test the jxta_message_element_new_3 function
* 
* @return NULL for success otherwise a message indicating failure.
*/
char const * test_jxta_message_element_new_3(void)
{
    unsigned char *content = NULL;
    Jxta_message_element *el = NULL, *sig = NULL;
    const char *comparer;
    Jxta_bytevector *retContent = NULL, *compContent = NULL;
    char const * result = NULL;

    /* First initialize the content */
    compContent = jxta_bytevector_new_3(element_content, sizeof(*element_content), FALSE);
    
    if (compContent == NULL)
        return FILEANDLINE;

    /* Create an element with no namespace, no mime type and no signature */
    el = jxta_message_element_new_3("", "NewElement", NULL, compContent, NULL);
    if (el == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("application/octet-stream", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (jxta_message_element_get_signature(el) != NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL || !jxta_bytevector_equals(compContent, retContent)) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(el);
    el = NULL;

    /* Create an element with a namespace, a mimetype and a signature */
    sig = createSignatureElement();
    if (sig == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    el = jxta_message_element_new_3("MySpace", "NewElement", "MyMime", compContent, sig);
    if (el == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_namespace(el);
    if (comparer == NULL || strcmp("MySpace", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_name(el);
    if (comparer == NULL || strcmp("NewElement", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    comparer = jxta_message_element_get_mime_type(el);
    if (comparer == NULL || strcmp("MyMime", comparer) != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (!checkSignatureObject(jxta_message_element_get_signature(el))) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    retContent = jxta_message_element_get_value(el);
    if (retContent == NULL || !jxta_bytevector_equals(compContent, retContent)) {
        result = FILEANDLINE;
        goto Common_Exit;
    }


  Common_Exit:
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);
    if (sig != NULL)
        JXTA_OBJECT_RELEASE(sig);
    if (retContent != NULL)
        JXTA_OBJECT_RELEASE(retContent);
    if (compContent != NULL)
        JXTA_OBJECT_RELEASE(compContent);
    /* Do not need to free  content, as compContent takes care of that */

    return result;
}

/*
* Test functions to test jxta_message
*/

/**
* Write a test message
*/
const char* populate_test_msg(Jxta_message *message)
{
    Jxta_message_element *sig = NULL, *el = NULL;
    Jxta_endpoint_address *endpoint = NULL;
    char value[100], name[100];
    int i;
    const char *rv = NULL;

    sig = createSignatureElement();

    /* add elements for different namespaces */
    for (i = 0; i < 10; i++) {
        /* no namespace */
        value[0] = '\0';
        sprintf(value, "Empty namespace - Element %d", i);
        name[0] = '\0';
        sprintf(name, "EmptyElement_%d", i);
        el = jxta_message_element_new_1(name, NULL, value, strlen(value), sig);
        if (el == NULL) {
            rv = FILEANDLINE ": Failed to create an element\n";
            goto Common_Exit;
        }
        JXTA_OBJECT_SHARE(el);
        if (jxta_message_add_element(message, el) != JXTA_SUCCESS) {
            rv = FILEANDLINE ": Failed to add an element\n";
            goto Common_Exit;
        }
        JXTA_OBJECT_RELEASE(el);
        el = NULL;

        /* jxta namespace */
        value[0] = '\0';
        sprintf(value, "Jxta namespace - Element %d", i);
        name[0] = '\0';
        sprintf(name, "JxtaElement_%d", i);
        el = jxta_message_element_new_2("jxta", name, NULL, value, strlen(value), sig);
        if (el == NULL) {
            rv = FILEANDLINE ": Failed to create an element\n";
            goto Common_Exit;
        }
        if (jxta_message_add_element(message, el) != JXTA_SUCCESS) {
            rv = FILEANDLINE ": Failed to add an element\n";
            goto Common_Exit;
        }
        JXTA_OBJECT_RELEASE(el);
        el = NULL;

        /* MyOwn namespace */
        value[0] = '\0';
        sprintf(value, "MyOwn namespace - Element %d", i);
        name[0] = '\0';
        sprintf(name, "MyOwn:MyOwnElement_%d", i);
        el = jxta_message_element_new_1(name, NULL, value, strlen(value), sig);
        if (el == NULL) {
            rv = FILEANDLINE ": Failed to create an element\n";
            goto Common_Exit;
        }
        JXTA_OBJECT_SHARE(el);
        if (jxta_message_add_element(message, el) != JXTA_SUCCESS) {
            rv = FILEANDLINE ": Failed to add an element\n";
            goto Common_Exit;
        }
        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }

    /* Add a  source element */
    endpoint = jxta_endpoint_address_new_2("protNameSource", "protAddressSource", "serviceNameSource", "serviceParamsSource");
    if (endpoint == NULL) {
        rv = FILEANDLINE ": Failed to create endpoint address\n";
        goto Common_Exit;
    }

    if (jxta_message_set_source(message, endpoint) != JXTA_SUCCESS) {
        rv = FILEANDLINE ": Failed to set message source address\n";
        goto Common_Exit;
    }

    JXTA_OBJECT_RELEASE(endpoint);
    endpoint = NULL;

    /* Add a  destination element */
    endpoint = jxta_endpoint_address_new_2("protNameDest", "protAddressDest", "serviceNameDest", "serviceParamsDest");
    if (endpoint == NULL) {
        rv = FILEANDLINE ": Failed to create endpoint address\n";
        goto Common_Exit;
    }

    if (jxta_message_set_destination(message, endpoint) != JXTA_SUCCESS) {
        rv = FILEANDLINE ": Failed to set message destination address\n";
        goto Common_Exit;
    }
    
  Common_Exit:
    if (sig != NULL)
        JXTA_OBJECT_RELEASE(sig);
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);
    if (endpoint != NULL)
        JXTA_OBJECT_RELEASE(endpoint);
    return rv;
}

const char * create_test_msg(Jxta_message ** msg, apr_pool_t *p)
{
    apr_status_t rv;
    const char * result;

    rv = jxta_message_create(msg, p);
    if (rv != JXTA_SUCCESS) {
        *msg = NULL;
        return FILEANDLINE ": Failed to create message\n";
    }

    result = populate_test_msg(*msg);
    if (result) {
        jxta_message_destroy(*msg);
        *msg = NULL;
    }

    return NULL;
}

const char * getTestMessage(Jxta_message ** msg)
{
    const char * result;

    *msg = jxta_message_new();
    if (*msg == NULL) {
        return FILEANDLINE ": Failed to create message\n";
    }

    result = populate_test_msg(*msg);
    if (result) {
        jxta_message_destroy(*msg);
        *msg = NULL;
    }

    return NULL;
}


/**
* Test that the message is identical to the message created in 
* getTestMessage
*
* @param message the message to test
* @return TRUE if the messge is identical, FALSE otherwise
*/
char const * checkMessageCorrect(Jxta_message * message)
{
    Jxta_message_element *el = NULL;
    Jxta_endpoint_address *endpoint = NULL;
    char *buffer = NULL;
    int i;
    char const * result = NULL;
    Jxta_vector *elList = NULL;
    Jxta_bytevector *valueVector = NULL, *compVector = NULL;

    if (message == NULL)
        return FILEANDLINE;


    /* Check the number of elements with empty namespace */
    elList = jxta_message_get_elements_of_namespace(message, "");
    if (elList == NULL || jxta_vector_size(elList) != 10) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Check the content of the elements with empty namespace */
    for (i = 0; i < 10; i++) {
        buffer = (char *) calloc(100, sizeof(char));
        buffer[0] = '\0';
        sprintf(buffer, "EmptyElement_%d", i);
        if (JXTA_SUCCESS != jxta_message_get_element_1(message, buffer, &el) || el == NULL) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        buffer[0] = '\0';
        sprintf(buffer, "Empty namespace - Element %d", i);
        compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
        if (compVector == NULL ||
            strcmp(jxta_message_element_get_namespace(el), "") != 0 ||
            strcmp(jxta_message_element_get_mime_type(el), "application/octet-stream") != 0) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        valueVector = jxta_message_element_get_value(el);
        if (valueVector == NULL || !jxta_bytevector_equals(valueVector, compVector)) {
            result = FILEANDLINE;
            goto Common_Exit;
        }

        /* Signature saveing does not work correctly */
        /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
           result = FILEANDLINE;
           goto Common_Exit;
           } */

        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }
    JXTA_OBJECT_RELEASE(elList);
    elList = NULL;
    
    JXTA_OBJECT_RELEASE(compVector);
    compVector = NULL;

    /* Check the number of elements with jxta namespace */
    elList = jxta_message_get_elements_of_namespace(message, "jxta");
    if (elList == NULL || jxta_vector_size(elList) < 10) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Check the content of the elements with jxta namespace */
    for (i = 0; i < 10; i++) {
        buffer = (char *) calloc(100, sizeof(char));
        buffer[0] = '\0';
        sprintf(buffer, "jxta:JxtaElement_%d", i);
        if (JXTA_SUCCESS != jxta_message_get_element_1(message, buffer, &el) || el == NULL) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        buffer[0] = '\0';
        sprintf(buffer, "Jxta namespace - Element %d", i);
        compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
        if (compVector == NULL ||
            strcmp(jxta_message_element_get_namespace(el), "jxta") != 0 ||
            strcmp(jxta_message_element_get_mime_type(el), "application/octet-stream") != 0) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        valueVector = jxta_message_element_get_value(el);
        if (valueVector == NULL || !jxta_bytevector_equals(valueVector, compVector)) {
            result = FILEANDLINE;
            goto Common_Exit;
        }

        /* Signature saveing does not work correctly */
        /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
           result = FILEANDLINE;
           goto Common_Exit;
           } */

        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }
    JXTA_OBJECT_RELEASE(elList);
    elList = NULL;

    /* Check the number of  elements with MyOwn namespace */
    elList = jxta_message_get_elements_of_namespace(message, "MyOwn");
    if (elList == NULL || jxta_vector_size(elList) != 10) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    for (i = 0; i < 10; i++) {
        buffer = (char *) calloc(100, sizeof(char));
        buffer[0] = '\0';
        sprintf(buffer, "MyOwnElement_%d", i);
        if (JXTA_SUCCESS != jxta_message_get_element_2(message, "MyOwn", buffer, &el) || el == NULL) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        buffer[0] = '\0';
        sprintf(buffer, "MyOwn namespace - Element %d", i);
        compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
        if (compVector == NULL ||
            strcmp(jxta_message_element_get_namespace(el), "MyOwn") != 0 ||
            strcmp(jxta_message_element_get_mime_type(el), "application/octet-stream") != 0) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        valueVector = jxta_message_element_get_value(el);
        if (valueVector == NULL || !jxta_bytevector_equals(valueVector, compVector)) {
            result = FILEANDLINE;
            goto Common_Exit;
        }

        /* Signature saveing does not work correctly */
        /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
           result = FILEANDLINE;
           goto Common_Exit;
           } */

        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }
    JXTA_OBJECT_RELEASE(elList);
    elList = NULL;

    /* Check that the source element was retrieved correctly */
    endpoint = jxta_message_get_source(message);
    if (endpoint == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (strcmp(jxta_endpoint_address_get_protocol_name(endpoint), "protNameSource") != 0 ||
        strcmp(jxta_endpoint_address_get_protocol_address(endpoint), "protAddressSource") != 0 ||
        strcmp(jxta_endpoint_address_get_service_name(endpoint), "serviceNameSource") != 0 ||
        strcmp(jxta_endpoint_address_get_service_params(endpoint), "serviceParamsSource") != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(endpoint);
    endpoint = NULL;

    /* Check that the destination element was retrieved correctly */
    endpoint = jxta_message_get_destination(message);
    if (endpoint == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (strcmp(jxta_endpoint_address_get_protocol_name(endpoint), "protNameDest") != 0 ||
        strcmp(jxta_endpoint_address_get_protocol_address(endpoint), "protAddressDest") != 0 ||
        strcmp(jxta_endpoint_address_get_service_name(endpoint), "serviceNameDest") != 0 ||
        strcmp(jxta_endpoint_address_get_service_params(endpoint), "serviceParamsDest") != 0) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(endpoint);
    endpoint = NULL;

  Common_Exit:
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);
    if (endpoint != NULL)
        JXTA_OBJECT_RELEASE(endpoint);
    if (elList != NULL)
        JXTA_OBJECT_RELEASE(elList);
    if (valueVector != NULL)
        JXTA_OBJECT_RELEASE(valueVector);
    if (compVector != NULL)
        JXTA_OBJECT_RELEASE(compVector);

    return result;
}


/**
* Test the read/write functionality for jxta_message version 0
* 
* @return NULL for success otherwise a message indicating failure.
*/
char const * test_jxta_message_0_read_write(void){
    return test_jxta_message_read_write(0);
}

/**
* Test the read/write functionality for jxta_message verison 1
* 
* @return NULL for success otherwise a message indicating failure.
*/
char const * test_jxta_message_1_read_write(void){
    return test_jxta_message_read_write(1);
}

static Jxta_status JXTA_STDCALL msg_wireformat_size(void *arg, const char *buf, apr_size_t len)
{
    apr_uint64_t *size = (apr_uint64_t *) arg;  /* 8 bytes */
    *size += len;
    return JXTA_SUCCESS;
}

/**
* Test the read/write functionality for jxta_message
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
static char const * msg_read_write(Jxta_message * write_message, Jxta_message * read_message, int version)
{
    char const * result = NULL;
    JString *wire = NULL;
    char *stream = NULL;
    read_write_test_buffer stream_struct;
    apr_uint64_t msg_size;
    Jxta_status res;

    msg_size = APR_INT64_C(0);
    res = jxta_message_write_1(write_message, NULL, version, msg_wireformat_size, &msg_size);
    
    if( res != JXTA_SUCCESS ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    if( 0 == msg_size ) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Create the two char buffers */
    stream = (char *) calloc(msg_size, sizeof(char));
    if (stream == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Write the message to a stream in wire format */
    stream_struct.buffer = stream;
    stream_struct.position = 0;
    stream[0] = '\0';
    if (jxta_message_write_1(write_message, NULL, version, writeFunction, &stream_struct) != JXTA_SUCCESS) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (stream_struct.position != msg_size) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Read the message back in */
    stream_struct.position = 0;
    if (jxta_message_read(read_message, NULL, readFromStreamFunction, &stream_struct) != JXTA_SUCCESS) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    if (stream_struct.position != msg_size) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    /* Now test that the message elements where all read back in correctly */
    result = checkMessageCorrect(read_message);

  Common_Exit:
    if (wire != NULL)
        JXTA_OBJECT_RELEASE(wire);
    if (stream != NULL)
        free(stream);

    return result;
}

static char const * test_jxta_message_read_write(int version)
{
    Jxta_message *write_message;
    Jxta_message *read_message = jxta_message_new();
    char const * result = NULL;

    result = getTestMessage(&write_message);
    if (result) 
        return result;

  /** Check that checkMessageCorrect says the message is good */
    result = checkMessageCorrect(write_message);
    if (result) {
        goto FINAL_EXIT;
    }

    result = msg_read_write(write_message, read_message, version);

FINAL_EXIT:
    if (write_message != NULL)
        JXTA_OBJECT_RELEASE(write_message);
    if (read_message != NULL)
        JXTA_OBJECT_RELEASE(read_message);

    return result;
}


/**
* Test whether a message is cloned correctly
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
char const * test_jxta_message_clone(void)
{
    Jxta_message *message;
    Jxta_message *cloned = NULL;
    char const * result = NULL;

    result = getTestMessage(&message);
    if (result)
        return result;

  /** Check that checkMessageCorrect says the message is good */
    result = checkMessageCorrect(message);
    if (result) {
        goto Common_Exit;
    }

  /** Clone the Message */
    cloned = jxta_message_clone(message);
    if (cloned == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    result = checkMessageCorrect(cloned);

  Common_Exit:
    if (message != NULL)
        JXTA_OBJECT_RELEASE(message);
    if (cloned != NULL)
        JXTA_OBJECT_RELEASE(cloned);

    return result;
}


/**
* Test  the different remove functions
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
char const * test_jxta_message_remove(void)
{
    Jxta_message *message;
    char const * result = NULL;
    Jxta_message_element *el = NULL;
    char value[100], name[100];
    int i;

    result = getTestMessage(&message);
    if (result)
        return result;

    /* add an element and then remove it again */
    value[0] = '\0';
    sprintf(value, "Jxta namespace - Element");
    name[0] = '\0';
    sprintf(name, "JxtaElement");
    el = jxta_message_element_new_2("jxta", name, NULL, value, strlen(value), NULL);
    if (el == NULL) {
        result = FILEANDLINE;
        goto Common_Exit;
    }
    JXTA_OBJECT_SHARE(el);
    if (jxta_message_add_element(message, el) != JXTA_SUCCESS) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    if (JXTA_SUCCESS != jxta_message_remove_element(message, el)) {
        result = FILEANDLINE;
        goto Common_Exit;
    }

    result = checkMessageCorrect(message);
    if (result) {
        goto Common_Exit;
    }

    JXTA_OBJECT_RELEASE(el);
    el = NULL;

    /* Add some elements  */
    for (i = 0; i < 10; i++) {
        value[0] = '\0';
        sprintf(value, "Jxta namespace - Element %d", i);
        name[0] = '\0';
        sprintf(name, "JxtaRemoveElement_%d", i);
        el = jxta_message_element_new_2("jxta", name, NULL, value, strlen(value), NULL);
        if (el == NULL) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        JXTA_OBJECT_SHARE(el);
        if (jxta_message_add_element(message, el) != JXTA_SUCCESS) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
        JXTA_OBJECT_RELEASE(el);
        el = NULL;
    }

    /* Now remove them again */
    for (i = 0; i < 10; i += 2) {
        name[0] = '\0';
        sprintf(name, "jxta:JxtaRemoveElement_%d", i);
        if (JXTA_SUCCESS != jxta_message_remove_element_1(message, name)) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
    }
    for (i = 1; i < 10; i += 2) {
        name[0] = '\0';
        sprintf(name, "JxtaRemoveElement_%d", i);
        if (JXTA_SUCCESS != jxta_message_remove_element_2(message, "jxta", name)) {
            result = FILEANDLINE;
            goto Common_Exit;
        }
    }

    /* And check that the message is correct */
    result = checkMessageCorrect(message);
    if (result) {
        goto Common_Exit;
    }

  Common_Exit:
    if (message != NULL)
        JXTA_OBJECT_RELEASE(message);
    if (el != NULL)
        JXTA_OBJECT_RELEASE(el);

    return result;
}

char const * test_msg_create(void)
{
    apr_status_t rv;
    apr_pool_t *p;
    Jxta_message *msg;
    char const * result = NULL;

    rv = apr_pool_create(&p, NULL);
    if (APR_SUCCESS != rv) {
        return FILEANDLINE ": Failed to create apr_pool_t\n";
    }

    result = create_test_msg(&msg, p);
    if (result)
        return result;

    result = checkMessageCorrect(msg);
    apr_pool_destroy(p);
    return result;
}

char const * test_msg_with_qos(void)
{
    apr_status_t rv;
    apr_pool_t *p;
    Jxta_message *msg;
    Jxta_qos * qos;
    char const * result = NULL;
    Jxta_message *rmsg;
    Jxta_qos *rqos;
    apr_uint16_t val;

    rv = apr_pool_create(&p, NULL);
    if (APR_SUCCESS != rv) {
        return FILEANDLINE ": Failed to create apr_pool_t\n";
    }

    result = create_test_msg(&msg, p);
    if (result)
        goto FINAL_EXIT;

    rv = jxta_qos_create(&qos, p);
    if (JXTA_SUCCESS != rv) {
        result = FILEANDLINE ": Failed to create Jxta_qos\n";
        goto FINAL_EXIT;
    }

    jxta_qos_set_long(qos, "jxtaMsg:TTL", 5);
    jxta_qos_set(qos, "jxtaMsg:priority", "40000");
    jxta_message_attach_qos(msg, qos);

    result = checkMessageCorrect(msg);
    if (result)
        goto FINAL_EXIT;

    rv = jxta_message_create(&rmsg, p);
    if (JXTA_SUCCESS != rv) {
        result = FILEANDLINE ": Failed to create Jxta_message\n";
        goto FINAL_EXIT;
    }
    result = msg_read_write(msg, rmsg, 1);
    if (result)
        goto FINAL_EXIT;

    rv = jxta_message_get_ttl(rmsg, &val);
    if (JXTA_SUCCESS != rv || val != 4) {
        result = FILEANDLINE ": QoS was not read successfully\n";
        goto FINAL_EXIT;
    }

    rv = jxta_message_get_priority(rmsg, &val);
    if (JXTA_SUCCESS != rv || val != 40000) {
        result = FILEANDLINE ": QoS was not read successfully\n";
        goto FINAL_EXIT;
    }

FINAL_EXIT:
    apr_pool_destroy(p);
    return result;
}

char const * test_msg_qos_clone(void)
{
    apr_status_t rv;
    apr_pool_t *p;
    Jxta_message *msg;
    Jxta_qos * qos;
    char const * result = NULL;
    Jxta_message *cloned;
    Jxta_qos *rqos;
    apr_uint16_t val;

    rv = apr_pool_create(&p, NULL);
    if (APR_SUCCESS != rv) {
        return FILEANDLINE ": Failed to create apr_pool_t\n";
    }

    result = create_test_msg(&msg, p);
    if (result)
        goto FINAL_EXIT;

    rv = jxta_qos_create(&qos, p);
    if (JXTA_SUCCESS != rv) {
        result = FILEANDLINE ": Failed to create Jxta_qos\n";
        goto FINAL_EXIT;
    }

    jxta_qos_set_long(qos, "jxtaMsg:TTL", 5);
    jxta_qos_set(qos, "jxtaMsg:priority", "40000");
    jxta_message_attach_qos(msg, qos);

    result = checkMessageCorrect(msg);
    if (result)
        goto FINAL_EXIT;

    cloned = jxta_message_clone(msg);
    if (cloned == NULL) {
        result = FILEANDLINE;
        goto FINAL_EXIT;
    }

    result = checkMessageCorrect(cloned);
    if (result)
        goto FINAL_EXIT;

    rv = jxta_message_get_ttl(cloned, &val);
    if (JXTA_SUCCESS != rv || val != 5) {
        result = FILEANDLINE ": QoS was not read successfully\n";
        goto FINAL_EXIT;
    }

    rv = jxta_message_get_priority(cloned, &val);
    if (JXTA_SUCCESS != rv || val != 40000) {
        result = FILEANDLINE ": QoS was not read successfully\n";
        goto FINAL_EXIT;
    }

FINAL_EXIT:
    if (cloned != NULL)
        JXTA_OBJECT_RELEASE(cloned);
    apr_pool_destroy(p);
    return result;
}

static struct _funcs msg_test_funcs[] = {
    /* First run JXTA MESSAGE ELEMENT tests */
    {*test_jxta_message_element_new_1, "construction/retrieval for jxta_message_element_new_1"},
    {*test_jxta_message_element_new_2, "construction/retrieval for jxta_message_element_new_2"},
    {*test_jxta_message_element_new_3, "construction/retrieval for jxta_message_element_new_3"},

    /* jxta_message test functions */
    {*test_jxta_message_clone, "jxta_message_clone"},
    {*test_jxta_message_remove, "jxta_message_remove"},

    /* Serialization/Deserialization */
    {*test_jxta_message_0_read_write, "read/write test for jxta_message v1"},
    {*test_jxta_message_1_read_write, "read/write test for jxta_message v2"},

    /* Pool based msg test */
    {*test_msg_create, "jxta_message_create"},
    {*test_msg_with_qos, "read/write test with qos"},
    {*test_msg_qos_clone, "jxta_message_clone with qos"},

    {NULL, "null"}
};


/**
* Run the unit tests for the jxta_message test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_msg_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(msg_test_funcs, tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(msg_test_funcs, argc, argv);
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
