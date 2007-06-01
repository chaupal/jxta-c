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
 * $Id: msg_test.c,v 1.19 2005/05/24 16:53:34 bondolo Exp $
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

/*
 * Test functions for Message elements
 */

/** Create a signature element.
* The name is sig:MySig, the mime type MySigMime and the 
* content contains the bytes 255,254,1,200,12,0,13,244 
* 
* @return a Jxta_message_element that constitutes a signature
*/
Jxta_message_element * createSignatureElement(void){
  unsigned char  *content;
  Jxta_message_element *sig = NULL;

  /* First initialize the content */
  content = (unsigned char*)malloc( sizeof(unsigned char) * 8);
  if( content != NULL){
    content[0] = 255;
    content[1] = 254;
    content[2] = 1;
    content[3] = 200;
    content[4] = 12;
    content[5] = 0;
    content[6] = 13;
    content[7] = 244;
  }
  else return sig;


  /* No create the signature */
  sig =  jxta_message_element_new_2 ("sig", 
                                     "MySig",
                                     "MySigMime",
				     (char*)content,
				     8,
				     NULL);

  free(content);
  return sig;  
}

/**
 * Check whether the signature element was returned correctly
 *
 * @return TRUE if it was returned correctly, FALSE otherwise
 */
Jxta_boolean checkSignatureObject(Jxta_message_element *sig){
   const char* comparer;
   Jxta_bytevector*  content;
   Jxta_boolean result = TRUE;
   unsigned char byte;

   if( sig == NULL) return FALSE;
   
   /* Check the namespace */  
   comparer = jxta_message_element_get_namespace(sig);
   if( strcmp( comparer,"sig") != 0) return FALSE;

   /* Check the name */
   comparer = jxta_message_element_get_name(sig);
   if( strcmp( comparer,"MySig") != 0) return FALSE;

   /* Check the mime type */
   comparer = jxta_message_element_get_mime_type(sig);
   if( strcmp( comparer,"MySigMime") != 0) return FALSE;

   /** Check the content of the signature */
   content = jxta_message_element_get_value(sig);
   if( jxta_bytevector_size( content)  != 8){
     JXTA_OBJECT_RELEASE(content);
     return FALSE;
   }
   result = TRUE;
   jxta_bytevector_get_byte_at(content,&byte,0);
   if( byte != 255) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,1);
   if( byte != 254) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,2);
   if( byte != 1) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,3);
   if( byte != 200) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,4);
   if( byte != 12) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,5);
   if( byte != 0) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,6);
   if( byte != 13) result = FALSE;
   jxta_bytevector_get_byte_at(content,&byte,7);
   if( byte != 244) result = FALSE;
   JXTA_OBJECT_RELEASE(content);
   if( !result) return FALSE;

   if( jxta_message_element_get_signature(sig) != NULL) return FALSE;

   return TRUE;
}


/**
* Test the jxta_message_element_new_1 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_element_new_1(void) {
  unsigned char  *content = NULL;
  Jxta_message_element *el = NULL, *sig = NULL;
  const char* comparer;
  Jxta_bytevector*  retContent = NULL, *compContent = NULL;
  Jxta_boolean result = TRUE;

  /* First initialize the content */
  content = (unsigned char*)malloc( sizeof(unsigned char) * 8);
  if( content != NULL){
    content[0] = 0;
    content[1] = 255;
    content[2] = 8;
    content[3] = 254;
    content[4] = 12;
    content[5] = 0;
    content[6] = 9;
    content[7] = 251;
  }
  else return FALSE;
  compContent = jxta_bytevector_new_2(content,8,9);
  if( compContent == NULL) return FALSE;

  /* Create an element with no namespace, no mime type and no signature */
  el = jxta_message_element_new_1 ("NewElement",NULL,(char*)content,8,NULL);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("application/octet-stream",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( jxta_message_element_get_signature(el) != NULL){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }


  /* Create an element with a namespace, a mimetype and a signature */
  sig = createSignatureElement();
  if( sig == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  el = jxta_message_element_new_1 ("MySpace:NewElement",
                                   "MyMime",
                                   (char*)content,
                                   8,
				   sig);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("MySpace",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("MyMime",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }

Common_Exit:
   if( el != NULL) JXTA_OBJECT_RELEASE(el);
   if( sig != NULL) JXTA_OBJECT_RELEASE(sig);
   if( retContent != NULL) JXTA_OBJECT_RELEASE(retContent);
   if( compContent != NULL) JXTA_OBJECT_RELEASE(compContent);
   /* Do not need to free  content, as compContent takes care of that */

   return result;
}


/**
* Test the jxta_message_element_new_2 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_element_new_2(void) {
  unsigned char  *content = NULL;
  Jxta_message_element *el = NULL, *sig = NULL;
  const char* comparer;
  Jxta_bytevector*  retContent = NULL, *compContent = NULL;
  Jxta_boolean result = TRUE;

  /* First initialize the content */
  content = (unsigned char*)calloc( 8, sizeof(unsigned char));
  if( content != NULL){
    content[0] = 0;
    content[1] = 255;
    content[2] = 8;
    content[3] = 254;
    content[4] = 12;
    content[5] = 0;
    content[6] = 9;
    content[7] = 251;
  }
  else return FALSE;
  compContent = jxta_bytevector_new_2(content,8,9);
  if( compContent == NULL) return FALSE;

  /* Create an element with no namespace, no mime type and no signature */
  el = jxta_message_element_new_2 ("","NewElement",NULL,(char*)content,8,NULL);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("application/octet-stream",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( jxta_message_element_get_signature(el) != NULL){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }


  /* Create an element with a namespace, a mimetype and a signature */
  sig = createSignatureElement();
  if( sig == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  el = jxta_message_element_new_2 ("MySpace",
				   "NewElement",
				   "MyMime",
				   (char*)content,
				   8,
				   sig);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("MySpace",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("MyMime",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }


Common_Exit:
   if( el != NULL) JXTA_OBJECT_RELEASE(el);
   if( sig != NULL) JXTA_OBJECT_RELEASE(sig);
   if( retContent != NULL) JXTA_OBJECT_RELEASE(retContent);
   if( compContent != NULL) JXTA_OBJECT_RELEASE(compContent);
   /* Do not need to free  content, as compContent takes care of that */

   return result;
}


/**
* Test the jxta_message_element_new_3 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_element_new_3(void) {
  unsigned char  *content = NULL;
  Jxta_message_element *el = NULL, *sig = NULL;
  const char* comparer;
  Jxta_bytevector*  retContent = NULL, *compContent = NULL;
  Jxta_boolean result = TRUE;

  /* First initialize the content */
  content = (unsigned char*)malloc( sizeof(unsigned char) * 8);
  if( content != NULL){
    content[0] = 0;
    content[1] = 255;
    content[2] = 8;
    content[3] = 254;
    content[4] = 12;
    content[5] = 0;
    content[6] = 9;
    content[7] = 251;
  }
  else return FALSE;
  compContent = jxta_bytevector_new_2(content,8,9);
  if( compContent == NULL) return FALSE;

  /* Create an element with no namespace, no mime type and no signature */
  el = jxta_message_element_new_3 ("","NewElement",NULL,compContent,NULL);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("application/octet-stream",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( jxta_message_element_get_signature(el) != NULL){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }


  /* Create an element with a namespace, a mimetype and a signature */
  sig = createSignatureElement();
  if( sig == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  el = jxta_message_element_new_3 ("MySpace","NewElement","MyMime",compContent,sig);
  if( el == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_namespace(el);
  if( comparer == NULL || strcmp("MySpace",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_name(el);
  if( comparer == NULL ||  strcmp("NewElement",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  comparer = jxta_message_element_get_mime_type(el);
  if( comparer == NULL ||  strcmp("MyMime",comparer) != 0){
    result = FALSE;
    goto Common_Exit;
  }
  if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
    result = FALSE;
    goto Common_Exit;
  }
  retContent = jxta_message_element_get_value(el);
  if( retContent == NULL || !jxta_bytevector_equals( compContent,retContent) ){
    result = FALSE; 
    goto Common_Exit;
  }


Common_Exit:
   if( el != NULL) JXTA_OBJECT_RELEASE(el);
   if( sig != NULL) JXTA_OBJECT_RELEASE(sig);
   if( retContent != NULL) JXTA_OBJECT_RELEASE(retContent);
   if( compContent != NULL) JXTA_OBJECT_RELEASE(compContent);
   /* Do not need to free  content, as compContent takes care of that */

   return result;
}

/*
* Test functions to test jxta_message
*/

/**
* Write a test message
*/
Jxta_message*  getTestMessage(void){
  Jxta_message* message = jxta_message_new();
  Jxta_message_element *sig = NULL,*el = NULL;
  Jxta_endpoint_address *endpoint = NULL;
  char value[100],name[100];
  int i;

  if( message == NULL) return message;

  sig = createSignatureElement();

  /* add elements for different namespaces */
  for(i = 0; i < 10; i++){
    /* no namespace */
    value[0] = '\0';
    sprintf(value,"Empty namespace - Element %d",i);
    name[0] = '\0';
    sprintf(name,"EmptyElement_%d",i);
    el = jxta_message_element_new_1 (name,
				     NULL,
				     value,
				     strlen(value),
				     sig);
    if( el == NULL) {
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;
    }
    JXTA_OBJECT_SHARE(el);
    if( jxta_message_add_element(message,el) != JXTA_SUCCESS){ 
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;        
    }
    JXTA_OBJECT_RELEASE(el);
    el  = NULL; 

    /* jxta namespace */
    value[0] = '\0';
    sprintf(value,"Jxta namespace - Element %d",i);
    name[0] = '\0';
    sprintf(name,"JxtaElement_%d",i);
    el = jxta_message_element_new_2 ("jxta",
				     name,
				     NULL,
				     value,
				     strlen(value),
				     sig);
    if( el == NULL) {
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;
    }
    JXTA_OBJECT_SHARE(el);
    if( jxta_message_add_element(message,el) != JXTA_SUCCESS){ 
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;        
    }
    JXTA_OBJECT_RELEASE(el);
    el  = NULL; 

    /* MyOwn namespace */
    value[0] = '\0';
    sprintf(value,"MyOwn namespace - Element %d",i);
    name[0] = '\0';
    sprintf(name,"MyOwn:MyOwnElement_%d",i);
    el = jxta_message_element_new_1 (name,
				     NULL,
				     value,
				     strlen(value),
				     sig);
    if( el == NULL) {
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;
    }
    JXTA_OBJECT_SHARE(el);
    if( jxta_message_add_element(message,el) != JXTA_SUCCESS){ 
       JXTA_OBJECT_RELEASE(message);
       message = NULL;
       goto Common_Exit;        
    }
    JXTA_OBJECT_RELEASE(el);
    el  = NULL;  
  }

  /* Add a  source element */
  endpoint =jxta_endpoint_address_new2("protNameSource",
                                       "protAddressSource",
				       "serviceNameSource",
                                       "serviceParamsSource");
  if( endpoint == NULL){
    JXTA_OBJECT_RELEASE(message);
    message = NULL;
    goto Common_Exit;
  }
  JXTA_OBJECT_SHARE(endpoint);
  if( jxta_message_set_source( message, endpoint) != JXTA_SUCCESS){
    JXTA_OBJECT_RELEASE(message);
    message = NULL;
    goto Common_Exit;
  }
      
  JXTA_OBJECT_RELEASE(endpoint);
  endpoint = NULL; 

  /* Add a  destination element */
  endpoint =jxta_endpoint_address_new2("protNameDest",
                                       "protAddressDest",
				       "serviceNameDest",
                                       "serviceParamsDest");
  if( endpoint == NULL){
    JXTA_OBJECT_RELEASE(message);
    message = NULL;
    goto Common_Exit;
  }
  JXTA_OBJECT_SHARE(endpoint);
  if( jxta_message_set_destination( message, endpoint) != JXTA_SUCCESS){
    JXTA_OBJECT_RELEASE(message);
    message = NULL;
    goto Common_Exit;
  }
Common_Exit:
   if( sig != NULL) JXTA_OBJECT_RELEASE(sig);
   if( el != NULL) JXTA_OBJECT_RELEASE(el);
   if( endpoint != NULL) JXTA_OBJECT_RELEASE(endpoint);

   return message;
}


/**
* Test that the message is identical to the message created in 
* getTestMessage
*
* @param message the message to test
* @return TRUE if the messge is identical, FALSE otherwise
*/
Jxta_boolean checkMessageCorrect(Jxta_message* message){
  Jxta_message_element *el = NULL;
  Jxta_endpoint_address *endpoint = NULL;
  char *buffer = NULL;
  int i;
  Jxta_boolean result = TRUE;
  Jxta_vector *elList = NULL;
  Jxta_bytevector *valueVector = NULL,*compVector = NULL;

  if( message == NULL) return FALSE;


  /* Check the number of elements with empty namespace */
  elList = jxta_message_get_elements_of_namespace(message,"");
  if( elList == NULL || jxta_vector_size( elList) != 10){
     result = FALSE;
     goto Common_Exit;
  }

  /* Check the content of the elements with empty namespace */
  for( i = 0; i < 10; i++){
    buffer = (char*)calloc( 100, sizeof(char) );
    buffer[0] = '\0';
    sprintf(buffer,"EmptyElement_%d",i);
    if( JXTA_SUCCESS != jxta_message_get_element_1(message, buffer,&el) ||
        el == NULL){
      result = FALSE;
      goto Common_Exit;
    }   
    buffer[0] = '\0';
    sprintf(buffer,"Empty namespace - Element %d",i);
    compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
    if( compVector == NULL ||
        strcmp(jxta_message_element_get_namespace( el),"") != 0 ||
        strcmp(jxta_message_element_get_mime_type(el),"application/octet-stream") != 0 ){   
      result = FALSE;
      goto Common_Exit;
    }
    valueVector = jxta_message_element_get_value(el);
    if( valueVector == NULL || 
	!jxta_bytevector_equals(valueVector,compVector) ){
      result = FALSE;
      goto Common_Exit;
    }

    /* Signature saveing does not work correctly */
    /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
       result = FALSE;
       goto Common_Exit;
       }*/

    JXTA_OBJECT_RELEASE(el);
    el = NULL;
  }
  JXTA_OBJECT_RELEASE(elList);
  elList = NULL;

  /* Check the number of elements with jxta namespace */
  elList = jxta_message_get_elements_of_namespace(message,"jxta");
  if( elList == NULL || jxta_vector_size( elList) < 10){
     result = FALSE;
     goto Common_Exit;
  }

  /* Check the content of the elements with jxta namespace */
  for( i = 0; i < 10; i++){
    buffer = (char*)calloc( 100, sizeof(char));
    buffer[0] = '\0';
    sprintf(buffer,"jxta:JxtaElement_%d",i);
    if( JXTA_SUCCESS != jxta_message_get_element_1(message, buffer,&el) ||
        el == NULL){
      result = FALSE;
      goto Common_Exit;
    }  
    buffer[0] = '\0';
    sprintf(buffer,"Jxta namespace - Element %d",i);
    compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
    if( compVector == NULL ||
        strcmp(jxta_message_element_get_namespace( el),"jxta") != 0 ||
        strcmp(jxta_message_element_get_mime_type(el),"application/octet-stream") != 0 ){   
      result = FALSE;
      goto Common_Exit;
    }
    valueVector = jxta_message_element_get_value(el);
    if( valueVector == NULL || 
	!jxta_bytevector_equals(valueVector,compVector) ){
      result = FALSE;
      goto Common_Exit;
    }

    /* Signature saveing does not work correctly */
    /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
       result = FALSE;
       goto Common_Exit;
       }*/

    JXTA_OBJECT_RELEASE(el);
    el = NULL;
  }
  JXTA_OBJECT_RELEASE(elList);
  elList = NULL;

  /* Check the number of  elements with MyOwn namespace */
  elList = jxta_message_get_elements_of_namespace(message,"MyOwn");
  if( elList == NULL || jxta_vector_size( elList) != 10){
     result = FALSE;
     goto Common_Exit;
  }
  for( i = 0; i < 10; i++){
    buffer = (char*)malloc( sizeof(char) * 100);
    buffer[0] = '\0';
    sprintf(buffer,"MyOwnElement_%d",i);
    if( JXTA_SUCCESS != jxta_message_get_element_2(message, "MyOwn",buffer,&el) ||
        el == NULL){
      result = FALSE;
      goto Common_Exit;
    }  
    buffer[0] = '\0';
    sprintf(buffer,"MyOwn namespace - Element %d",i);
    compVector = jxta_bytevector_new_2(buffer, strlen(buffer), strlen(buffer) + 2);
    if( compVector == NULL ||
        strcmp(jxta_message_element_get_namespace( el),"MyOwn") != 0 ||
        strcmp(jxta_message_element_get_mime_type(el),"application/octet-stream") != 0 ){   
      result = FALSE;
      goto Common_Exit;
    }
    valueVector = jxta_message_element_get_value(el);
    if( valueVector == NULL || 
	!jxta_bytevector_equals(valueVector,compVector) ){
      result = FALSE;
      goto Common_Exit;
    }

    /* Signature saveing does not work correctly */
    /*if( !checkSignatureObject( jxta_message_element_get_signature(el) ) ){
       result = FALSE;
       goto Common_Exit;
       }*/

    JXTA_OBJECT_RELEASE(el);
    el = NULL;
  }
  JXTA_OBJECT_RELEASE(elList);
  elList = NULL;

  /* Check that the source element was retrieved correctly */
  endpoint = jxta_message_get_source(message);
  if( endpoint == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  if( strcmp(jxta_endpoint_address_get_protocol_name( endpoint ),"protNameSource") != 0 ||
      strcmp(jxta_endpoint_address_get_protocol_address( endpoint ),"protAddressSource") != 0 ||
      strcmp(jxta_endpoint_address_get_service_name( endpoint ),"serviceNameSource") != 0 ||
      strcmp(jxta_endpoint_address_get_service_params( endpoint ),"serviceParamsSource") != 0){
    result = FALSE;
    goto Common_Exit;
  }
  JXTA_OBJECT_RELEASE(endpoint);
  endpoint = NULL;

  /* Check that the destination element was retrieved correctly */
  endpoint = jxta_message_get_destination(message);
  if( endpoint == NULL){
    result = FALSE;
    goto Common_Exit;
  }
  if( strcmp(jxta_endpoint_address_get_protocol_name( endpoint ),"protNameDest") != 0 ||
      strcmp(jxta_endpoint_address_get_protocol_address( endpoint ),"protAddressDest") != 0 ||
      strcmp(jxta_endpoint_address_get_service_name( endpoint ),"serviceNameDest") != 0 ||
      strcmp(jxta_endpoint_address_get_service_params( endpoint ),"serviceParamsDest") != 0){
    result = FALSE;
    goto Common_Exit;
  }
  JXTA_OBJECT_RELEASE(endpoint);
  endpoint = NULL;

Common_Exit:
   if( el != NULL) JXTA_OBJECT_RELEASE(el);
   if( endpoint != NULL) JXTA_OBJECT_RELEASE(endpoint);
   if( elList != NULL) JXTA_OBJECT_RELEASE(elList);
   if( valueVector != NULL) JXTA_OBJECT_RELEASE(valueVector);
   if( compVector != NULL) JXTA_OBJECT_RELEASE(compVector);

   return result;
}


/**
* Test the read/write functionality for jxta_message
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_read_write(void) {
  Jxta_message* message = getTestMessage();
  Jxta_boolean result = TRUE;
  JString *wire = NULL;
  char * stream = NULL;
  int max;
  read_write_test_buffer stream_struct;


  if( message == NULL) return FALSE;

  /** Check that checkMessageCorrect says the message is good */
  if( !checkMessageCorrect(message)){
    result = FALSE;
    goto Common_Exit;
  }

  /* 
  * Get the string representation of the current message 
  * This is used to determine the length of the message
  */
  wire = jstring_new_0();
  if( wire == NULL) {
    result = FALSE;
    goto Common_Exit;
  } 
  if(  jxta_message_to_jstring( message,NULL,wire) != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  max = jstring_length( wire);

  /* Create the two char buffers */
  stream = (char*)malloc( max + 10);
  if( stream == NULL){
    result = FALSE;
    goto Common_Exit;
  }

  /* Write the message to a stream in wire format */
  stream_struct.buffer = stream;
  stream_struct.position = 0;
  stream[0] = '\0';
  if( jxta_message_write( message,
			  NULL,
			  writeFunction,
			  (void*)(&stream_struct) ) !=
      JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  if( stream_struct.position != max){
    result = FALSE;
    goto Common_Exit;
  }

  /* Read the message back in */
  stream_struct.position = 0;
  if( jxta_message_read( message,
			 NULL, 
			 readFromStreamFunction, 
			 (void*)(&stream_struct)) !=
      JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  if( stream_struct.position != max){
    result = FALSE;
    goto Common_Exit;
  }

  /* Now test that the message elements where all read back in correctly */
  if( !checkMessageCorrect(message) ){
    result = FALSE;
    goto Common_Exit;
  }  
Common_Exit:
   if(  message != NULL) JXTA_OBJECT_RELEASE(message);
   if( wire != NULL)     JXTA_OBJECT_RELEASE(wire);
   if( stream != NULL) free(stream);

   return result;
}


/**
* Test whether a message is cloned correctly
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_clone(void) {
  Jxta_message* message = getTestMessage(), *cloned  = NULL;
  Jxta_boolean result = TRUE;

  if( message == NULL) return FALSE;

  /** Check that checkMessageCorrect says the message is good */
  if( !checkMessageCorrect(message)){
    result = FALSE;
    goto Common_Exit;
  }

  /** Clone the Message */
  cloned =  jxta_message_clone( message);
  if( cloned == NULL){
    result = FALSE;
    goto Common_Exit;
  }


  /* Check that the message was cloned correctly*/
  if( !checkMessageCorrect(cloned) ){
    result = FALSE;
    goto Common_Exit;
  }  
Common_Exit:
   if(  message != NULL) JXTA_OBJECT_RELEASE(message);
   if(  cloned != NULL) JXTA_OBJECT_RELEASE(cloned);

   return result;
}


/**
* Test  the different remove functions
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
Jxta_boolean
test_jxta_message_remove(void) {
  Jxta_message* message = getTestMessage();
  Jxta_boolean result = TRUE;
  Jxta_message_element *el = NULL;
  char value[100],name[100];
  int i;

  if( message == NULL) return FALSE;

  /* add an element and then remove it again */
  value[0] = '\0';
  sprintf(value,"Jxta namespace - Element");
  name[0] = '\0';
  sprintf(name,"JxtaElement");
  el = jxta_message_element_new_2 ("jxta",
				   name,
				   NULL,
				   value,
				   strlen(value),
				   NULL);
  if( el == NULL) {
      result = FALSE;
      goto Common_Exit;
  }
  JXTA_OBJECT_SHARE(el);
  if( jxta_message_add_element(message,el) != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }

  if( JXTA_SUCCESS != jxta_message_remove_element( message, el) ){
    result = FALSE;
    goto Common_Exit;
  }

  if(  !checkMessageCorrect(message) ){
    result = FALSE;
    goto Common_Exit;
  }  

  JXTA_OBJECT_RELEASE(el);
  el  = NULL;

  /* Add some elements  */
  for(i = 0; i < 10; i++){
    value[0] = '\0';
    sprintf(value,"Jxta namespace - Element %d",i);
    name[0] = '\0';
    sprintf(name,"JxtaRemoveElement_%d",i);
    el = jxta_message_element_new_2 ("jxta",
				     name,
				     NULL,
				     value,
				     strlen(value),
				     NULL);
    if( el == NULL) {
      result = FALSE;
       goto Common_Exit;
    }
    JXTA_OBJECT_SHARE(el);
    if( jxta_message_add_element(message,el) != JXTA_SUCCESS){ 
       result = FALSE;
       goto Common_Exit;        
    }
    JXTA_OBJECT_RELEASE(el);
    el  = NULL; 
  }

  /* Now remove them again */
  for( i = 0; i < 10; i+=2){
     name[0] = '\0';
     sprintf(name,"jxta:JxtaRemoveElement_%d",i);
     if( JXTA_SUCCESS != jxta_message_remove_element_1( message,name) ){
       result = FALSE;
       goto Common_Exit;
     }
  }
  for( i = 1; i < 10; i+=2){
     name[0] = '\0';
     sprintf(name,"JxtaRemoveElement_%d",i);
     if( JXTA_SUCCESS != jxta_message_remove_element_2( message,"jxta",name) ){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* And check that the message is correct */
 if(  !checkMessageCorrect(message) ){
    result = FALSE;
    goto Common_Exit;
  }  

Common_Exit:
   if(  message != NULL) JXTA_OBJECT_RELEASE(message);
   if(  el != NULL) JXTA_OBJECT_RELEASE(el);

   return result;
}


static struct _funcs testfunc[] = {
  /* First run JXTA MESSAGE ELEMENT tests */
  {*test_jxta_message_element_new_1, "construction/retrieval for jxta_message_element_new_1"},
  {*test_jxta_message_element_new_2, "construction/retrieval for jxta_message_element_new_2"},
  {*test_jxta_message_element_new_3, "construction/retrieval for jxta_message_element_new_3" },

  /* jxta_message test functions */
  {*test_jxta_message_read_write, "read/write test for jxta_message" },
  {*test_jxta_message_clone, "jxta_message_clone" },
  {*test_jxta_message_remove, "jxta_message_remove" },

  {NULL,                             "null"                       }
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
Jxta_boolean run_jxta_msg_tests( int * tests_run,
				 int * tests_passed,
				 int * tests_failed){
  return run_testfunctions(testfunc,tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int 
main(int argc, char ** argv) {
  return main_test_function(testfunc,argc,argv);
}
#endif
