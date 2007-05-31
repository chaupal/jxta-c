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
 * $Id: jstring_test.c,v 1.20 2004/01/19 22:33:39 wiarda Exp $
 */


#include <stdio.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

#include "unittest_jxta_func.h"

/**
 * Unit test framework for jstring. 
 */


/**
 * Testing allocators is useful with a memory 
 * checking program.
 */
/**
* Test the jstring_new_0 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_new_0(void) {
  JString *js = jstring_new_0();
  char const *testString;
  
  if( NULL == js )
    return FALSE;
    
  if ( 0 != jstring_length( js ) ){
    return FALSE;
  }

  /* check that the returned string is empty */
  testString = jstring_get_string(js);
  if( strcmp(testString,"")  != 0){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  
  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);

  return TRUE;
}

/**
* Test the jstring_new_1 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_new_1(void) {
  JString *js = jstring_new_1( 42 );
  char const *testString;
  
  if( NULL == js ) {
    return FALSE;
  }
    
  if ( 0 != jstring_length( js ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* check that the returned string is empty */
  testString = jstring_get_string(js);
  if( strcmp(testString,"")  != 0) {
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }  

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);

  return TRUE;
}

/**
* Test the jstring_new_2 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_new_2(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  
  if( NULL == js ){
    return FALSE;
  }
    
  if ( strlen(source) != jstring_length( js ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  
  if ( 0 != strcmp(source, jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);
  
  return TRUE;
}

/**
* Test the jstring_new_3 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_new_3(void) {
  Jxta_bytevector * vector = jxta_bytevector_new_1(6);
  Jxta_status status;
  int i;
  JString *js = NULL;

  if( NULL == vector) {
    return FALSE;
  }

  /* Initialize the byte vector with ABCDEF */
  for( i = 65; i < 71; i++) {
    status = jxta_bytevector_add_byte_last( vector,(unsigned char)i);
    if( JXTA_SUCCESS != status){
      JXTA_OBJECT_RELEASE(vector);
      return FALSE;
    }
  }
  
  /* Create a JString object from the byte vector */
  js = jstring_new_3(vector);
  JXTA_OBJECT_RELEASE(vector);
  if(  NULL == js){
    return FALSE;
  }

  /* Test whether length and content of jstring is correct */
  if ( 6 != jstring_length( js ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  
  if ( 0 != strcmp("ABCDEF", jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);  

  return TRUE;
}

/**
* Test the jstring_clone function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean
test_jstring_clone(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  JString *clone = NULL;
  
  if( NULL == js ){
    return FALSE;
  }
  
  /* Clone the string */  
  clone = jstring_clone(js);
  JXTA_OBJECT_RELEASE(js);
  if( NULL == clone) {
    return FALSE;
  }

  /* Test that the length and contents of the cloned string is correct */
  if ( strlen(source) != jstring_length(clone) ){
    JXTA_OBJECT_RELEASE(clone);
    return FALSE;
  }
  
  if ( 0 != strcmp(source, jstring_get_string(clone) ) ){
    JXTA_OBJECT_RELEASE(clone);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(clone);
  
  return TRUE;
}

/**
* Test the jstring_trim function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean
test_jstring_trim(void) {
  static const char * source = "\v \t\nTest String\twith tab\t\f\r\v\v";
  JString *js = jstring_new_2(source);
  
  if( NULL == js ){
    return FALSE;
  }

  /* Trim the JString object and test that the result is as expected */
  jstring_trim( js );
  if( 0 != strcmp("Test String\twith tab", jstring_get_string( js) ) ){
      JXTA_OBJECT_RELEASE(js);
      return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);
  
  return TRUE;
}

/**
* Test the jstring_length function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean
test_jstring_length(void) {
  static const char * source = "\v \t\nTest String\twith tab\t\f\r\v\v";
  static const char *append = " Append value";
  JString *js = jstring_new_2(source);
  
  if( NULL == js ){
    return FALSE;
  }

  /* Check that the length is as expected */
  if( strlen(source) != jstring_length( js) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Trim and check that we get the expected length and text */
  jstring_trim( js );
  if( strlen( "Test String\twith tab") != jstring_length( js) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  if( 0 != strcmp("Test String\twith tab", jstring_get_string( js) ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Append data and check that we get the expected length and text */
  jstring_append_0(js,append, strlen(append) );
  if( strlen("Test String\twith tab Append value") != jstring_length( js) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  if( 0 != strcmp("Test String\twith tab Append value", jstring_get_string(js))){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);
  
  return TRUE;
}

/**
* Test the jstring_get_string function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean
test_jstring_get_string(void) {
  static const char * source = "\v \t\nGetStringTest\t\f\r\v\v";
  JString *js = jstring_new_2(source);
  
  if( NULL == js ){
    return FALSE;
  }

  /* Check that the content is as expected */
  if( 0 != strcmp(source,jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Trim and check that the content is as expected */
  jstring_trim( js );
  if( 0 != strcmp("GetStringTest", jstring_get_string( js) )) {
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);
  
  return TRUE;
}

/**
* Test the jstring_reset function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean
test_jstring_reset(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  char *buffer[1];
  Jxta_status status;

  if( NULL == js){
    return FALSE;
  }

  /* Check correct reset with passing of string */
  buffer[0] = NULL;
  status = jstring_reset(js,buffer);
  if( status != JXTA_SUCCESS){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  if( 0 != strcmp(buffer[0],source) ){
    JXTA_OBJECT_RELEASE(js);
    if( buffer[0] != NULL) free( buffer[0]);
    return FALSE;
  }
  if( buffer[0] != NULL) free( buffer[0]);
  buffer[0] = NULL;
  if ( 0 != strcmp("", jstring_get_string(js) ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Check correct reset without passing of string */
  status = jstring_reset(js,NULL);
  if( status != JXTA_SUCCESS){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
  if ( 0 != strcmp("", jstring_get_string(js) ) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  /* Release the allocated string */
  JXTA_OBJECT_RELEASE(js);

  return TRUE;
}

/**
* Test the jstring_print function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_print(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  char buffer[100];
  
  if( NULL == js ){
    return FALSE;
  }
 
  /* Check jstring_print using sprintf as the PrintFunc object */
  buffer[0] = '\0';
  jstring_print(js,sprintf,buffer);
  JXTA_OBJECT_RELEASE(js);
  if( 0 != strcmp(  "yahoo!\n",buffer) ){
    return FALSE;
  } 
  
  return TRUE;
}


/**
* Test the jstring_send function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_send(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  char buffer[100];
  read_write_test_buffer stream;
  
  if( NULL == js ){
    return FALSE;
  }

  /* Send data with a flag of 1 which should write the data to the buffer */ 
  stream.buffer = buffer;
  stream.position = 0;
  buffer[0] = '\0';
  jstring_send(js,sendFunction,(void*)(&stream),1);
  if( 0 != strcmp(  "yahoo!",buffer) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  } 

  /* Send data with a flag of 0, which should write "flag set" to the buffer */
  buffer[0] = '\0';
  stream.position = 0;
  jstring_send(js,sendFunction,(void*)(&stream),0);
  JXTA_OBJECT_RELEASE(js);
  if( 0 != strcmp(  "flag set",buffer) ){
    return FALSE;
  } 
  
  return TRUE;
}


/**
* Test the jstring_write function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_write(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  read_write_test_buffer stream;
  char buffer[100];
  
  if( NULL == js ){
    return FALSE;
  }
 
  /* Check the function using  writeFunction as the WriteFunc object */
  stream.buffer = buffer;
  stream.position = 0;
  buffer[0] = '\0';
  jstring_send(js,writeFunction,(void*)(&stream),1);
  JXTA_OBJECT_RELEASE(js);
  if( 0 != strcmp(  "yahoo!",buffer) ){
    return FALSE;
  } 
  
  return TRUE;
}

/**
* Test the jstring_append_0 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_append_0(void) {
  static const char * source = "yahoo!xxx ";
  JString *js = jstring_new_2(source);
  size_t size = 6;
  
  if( NULL == js){
    return FALSE;
  }
 
  jstring_append_0(js,source,size);
  if( 0 != strcmp( "yahoo!xxx yahoo!", jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  } 

  JXTA_OBJECT_RELEASE(js);
  return TRUE;
}



/**
* Test the jstring_append_1 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_append_1(void) {
  static const char * source = "yahoo!";
  JString *js1 = jstring_new_2(source);
  JString *js2 = jstring_new_2(source);
  
  if( NULL == js1 || js2 == NULL ){
    if( js1 != NULL) JXTA_OBJECT_RELEASE(js1);
    if( js2 != NULL) JXTA_OBJECT_RELEASE(js2);
    return FALSE;
  }
 
  jstring_append_1(js1,js2);
  JXTA_OBJECT_RELEASE(js2);
  if( 0 != strcmp( "yahoo!yahoo!", jstring_get_string(js1)) ){
    JXTA_OBJECT_RELEASE(js1);
    return FALSE;
  } 
  
  JXTA_OBJECT_RELEASE(js1);
  return TRUE;
}

/**
* Test the jstring_append_2 function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_append_2(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  
  if( NULL == js){
    return FALSE;
  }
 
  jstring_append_2(js,source);
  if( 0 != strcmp( "yahoo!yahoo!", jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  } 

  JXTA_OBJECT_RELEASE(js);
  return TRUE;
}

/**
* Test the jstring_concat function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_concat(void) {
  static const char * source = "yahoo!";
  JString *js = jstring_new_2(source);
  
  if( NULL == js){
    return FALSE;
  }
 
  jstring_concat(js,3,source,source,source);
  if( 0 != strcmp( "yahoo!yahoo!yahoo!yahoo!", 
                   jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  } 

  JXTA_OBJECT_RELEASE(js);
  return TRUE;
}

/**
* Test the jstring_writefunc_appender function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jstring_writefunc_appender(void) {
  static const char * source = "yahoo!xxx ";
  JString *js = jstring_new_2(source);
  size_t size = 6;
  Jxta_status status;
  int result;
  
  if( NULL == js){
    return FALSE;
  }
 
  result = jstring_writefunc_appender((void*)js, NULL, size, &status);
  if( status == JXTA_SUCCESS || result != -1){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }

  result = jstring_writefunc_appender((void*)js, source, size, &status);
  if( status != JXTA_SUCCESS || result != size ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  }
 

  if( 0 != strcmp( "yahoo!xxx yahoo!", jstring_get_string(js)) ){
    JXTA_OBJECT_RELEASE(js);
    return FALSE;
  } 

  JXTA_OBJECT_RELEASE(js);
  return TRUE;
}



static struct _funcs testfunc[] = {
  {*test_jstring_new_0,              "jstring_new_0"              },
  {*test_jstring_new_1,              "jstring_new_1"              },
  {*test_jstring_new_2,              "jstring_new_2"              },
  {*test_jstring_new_3,              "jstring_new_3"              },
  {*test_jstring_clone,              "jstring_clone"              },
  {*test_jstring_trim,               "jstring_trim"               },
  {*test_jstring_length,             "jstring_length"             },
  {*test_jstring_get_string,         "jstring_get_string"         },
  {*test_jstring_reset,              "jstring_reset"              },
  {*test_jstring_print,              "jstring_print"              },
  {*test_jstring_send,               "jstring_send"               }, 
  {*test_jstring_write,              "jstring_write"              },
  {*test_jstring_append_0,           "jstring_append_0"           },
  {*test_jstring_append_1,           "jstring_append_1"           },
  {*test_jstring_append_2,           "jstring_append_2"           },
  {*test_jstring_concat,             "jstring_concat"             }, 
  {*test_jstring_writefunc_appender, "jstring_writefunc_appender" },
  {NULL,                             "null"                       }
};


/**
* Run the unit tests for the jstring test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jstring_tests( int * tests_run,
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

