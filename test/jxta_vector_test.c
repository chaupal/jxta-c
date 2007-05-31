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
 * $Id: jxta_vector_test.c,v 1.6 2003/12/18 19:37:50 wiarda Exp $
 */

#include "jxta.h"
#include "jxta_errno.h"
#include "unittest_jxta_func.h"


/****************************************************************
 **
 ** This test program tests the JXTA object sharing mechanism
 **
 ****************************************************************/

/**
 * Declaring the test object
 **/

typedef struct {
  JXTA_OBJECT_HANDLE;
  int field1;
  int field2;
} TestType;


/**
* Test the jxta_vector_new  function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_new(void) {
  Jxta_vector *vec  = jxta_vector_new(0);
  
  if( NULL == vec )
    return FALSE;
    
  if ( 0 != jxta_vector_size( vec ) ){
    JXTA_OBJECT_RELEASE( vec );
    return FALSE;
  }
  JXTA_OBJECT_RELEASE( vec );

  /* Supply a non-default initialization */
  vec = jxta_vector_new(5);
  if ( 0 != jxta_vector_size( vec ) ){
    JXTA_OBJECT_RELEASE( vec );
    return FALSE;
  }
  JXTA_OBJECT_RELEASE( vec );

  return TRUE;
}


/**
* Test the jxta_vector_add_object_at  and
* jxta_vector_get_object_at   function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_add_object_at(void) {
  Jxta_vector *vec  = jxta_vector_new(3);
  int i;
  TestType **pt;
  Jxta_object *holder;
  Jxta_status status;
  boolean result = TRUE;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * 5);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < 5; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field1 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[0],0);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[1],1);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[2],1);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[3],0);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[4],3);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }

  /* Try a couple of invalid input */
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[4],10);
  if( status == JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_add_object_at(vec,(Jxta_object*)pt[4],-1);
  if( status == JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }


  /* Check that the size is correct */
  if ( 5 != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }
  

  /* Check that the elements where added correctly */
  status = jxta_vector_get_object_at(vec, &holder,0);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[3]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);

  status = jxta_vector_get_object_at(vec, &holder,1);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[0]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);

  status = jxta_vector_get_object_at(vec, &holder,2);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[2]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
 
  status = jxta_vector_get_object_at(vec, &holder,3);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[4]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);

  status = jxta_vector_get_object_at(vec, &holder,4);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[1]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);


Common_Exit:
  for( i = 0; i < 5; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}


/**
* Test the jxta_vector_add_object_first  and
* jxta_vector_get_object_at   function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_add_object_first(void) {
  Jxta_vector *vec  = jxta_vector_new(3);
  int i,j;
  TestType **pt;
  Jxta_object *holder;
  Jxta_status status;
  boolean result = TRUE;
  size_t size = 10;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * size);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < size; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field1 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  for(i = 0; i < size; i++){
     status = jxta_vector_add_object_first(vec,(Jxta_object*)pt[i]);
     if( status != JXTA_SUCCESS){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* Check that the size is correct */
  if ( size != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }
  

  /* Check that the elements where added correctly */
  j = size -1;
  for( i = 0; i < size; i++){
     status = jxta_vector_get_object_at(vec, &holder,i);
     if( status != JXTA_SUCCESS ||
         holder != (Jxta_object*)pt[j]){
         if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
         result = FALSE;
         goto Common_Exit;
     }
     if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
     j--;
  }

Common_Exit:
  for( i = 0; i < size; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}


/**
* Test the jxta_vector_add_object_last  and
* jxta_vector_get_object_at   function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_add_object_last(void) {
  Jxta_vector *vec  = jxta_vector_new(3);
  int i;
  TestType **pt;
  Jxta_object *holder;
  Jxta_status status;
  boolean result = TRUE;
  size_t size = 100;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * size);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < size; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field1 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  for(i = 0; i < size; i++){
     status = jxta_vector_add_object_last(vec,(Jxta_object*)pt[i]);
     if( status != JXTA_SUCCESS){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* Check that the size is correct */
  if ( size != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }
  

  /* Check that the elements where added correctly */
  for( i = 0; i < size; i++){
     status = jxta_vector_get_object_at(vec, &holder,i);
     if( status != JXTA_SUCCESS ||
         holder != (Jxta_object*)pt[i]){
         if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
         result = FALSE;
         goto Common_Exit;
     }
     if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
  }

Common_Exit:
  for( i = 0; i < size; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}


/**
* Test the jxta_vector_add_object_last  and
* jxta_vector_get_object_at   function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_remove_object_at(void) {
  Jxta_vector *vec  = jxta_vector_new(0);
  int i;
  TestType **pt;
  Jxta_object *holder;
  Jxta_status status;
  boolean result = TRUE;
  size_t size = 5;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * size);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < size; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field2 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  for(i = 0; i < size; i++){
     status = jxta_vector_add_object_last(vec,(Jxta_object*)pt[i]);
     if( status != JXTA_SUCCESS){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* Remove a couple of objects */
  status = jxta_vector_remove_object_at(vec, NULL ,0);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }
  status = jxta_vector_remove_object_at(vec, &holder,2);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[3] ){
         if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
         result = FALSE;
         goto Common_Exit;
  }
  status = jxta_vector_remove_object_at(vec, NULL ,2);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }

  /* Check that the size is correct */
  if ( 2 != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }
  
  /* Check that the correct elements remain */
  status = jxta_vector_get_object_at(vec, &holder,0);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[1]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);

  status = jxta_vector_get_object_at(vec, &holder,1);
  if( status != JXTA_SUCCESS ||
      holder != (Jxta_object*)pt[2]){
    if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
    result = FALSE;
    goto Common_Exit;
  }
  if( holder != NULL) JXTA_OBJECT_RELEASE(holder);

Common_Exit:
  for( i = 0; i < size; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}


/**
* Test the jxta_vector_clone   function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_clone(void) {
  Jxta_vector *vec  = jxta_vector_new(3);
  Jxta_vector *cloned;
  Jxta_object *holder;
  int i;
  TestType **pt;
  Jxta_status status;
  boolean result = TRUE;
  size_t size = 100;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * size);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < size; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field1 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  for(i = 0; i < size; i++){
     status = jxta_vector_add_object_last(vec,(Jxta_object*)pt[i]);
     if( status != JXTA_SUCCESS){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* Check that the size is correct */
  if ( size != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }

  /* Clone vector from 0 to 19 and check that cloning worked */
  status = jxta_vector_clone(vec, &cloned, 0, 20);
  if( status != JXTA_SUCCESS){
    if( cloned != NULL) JXTA_OBJECT_RELEASE(cloned);
    result = FALSE;
    goto Common_Exit;
  } 
  for( i = 0; i < 20; i++){
     status = jxta_vector_get_object_at(cloned, &holder,i);
     if( status != JXTA_SUCCESS ||
         holder != (Jxta_object*)pt[i]){
         if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
         result = FALSE;
         goto Common_Exit;
     }
     if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
  }

  /* Clone vector from 20 to 39 and check that cloning worked */
  status = jxta_vector_clone(vec, &cloned, 20, 20);
  if( status != JXTA_SUCCESS){
    if( cloned != NULL) JXTA_OBJECT_RELEASE(cloned);
    result = FALSE;
    goto Common_Exit;
  }  
  for( i = 20; i < 20; i++){
     status = jxta_vector_get_object_at(cloned, &holder,i);
     if( status != JXTA_SUCCESS ||
         holder != (Jxta_object*)pt[i]){
         if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
         result = FALSE;
         goto Common_Exit;
     }
     if( holder != NULL) JXTA_OBJECT_RELEASE(holder);
  }

Common_Exit:
  for( i = 0; i < size; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}

/**
* Test the jxta_vector_clear  function
* 
* @return TRUE if the test run successfully, FALSE otherwise
*/
boolean 
test_jxta_vector_clear(void) {
  Jxta_vector *vec  = jxta_vector_new(3);
  int i;
  TestType **pt;
  Jxta_object *holder;
  Jxta_status status;
  boolean result = TRUE;
  size_t size = 100;

  if( NULL == vec )
    return FALSE;

  /* Initialize the test objects */
  pt = (TestType**) malloc (sizeof (TestType*) * size);
  if( pt == NULL){
    JXTA_OBJECT_RELEASE(vec);
    return FALSE;
  }
  for( i = 0; i < size; i++){
     pt[i] = (TestType*) malloc (sizeof (TestType));
     if( pt[i] != NULL){
        JXTA_OBJECT_INIT ((Jxta_object*)pt[i], free, 0);
        pt[i]->field1 = i;
        pt[i]->field1 = i + 1;
     }
  }
  
  /* Add the objects to the vector */
  for(i = 0; i < size; i++){
     status = jxta_vector_add_object_last(vec,(Jxta_object*)pt[i]);
     if( status != JXTA_SUCCESS){
       result = FALSE;
       goto Common_Exit;
     }
  }

  /* Check that the size is correct */
  if ( size != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }

  /* Clear the vector */
  status = jxta_vector_clear( vec);
  if( status != JXTA_SUCCESS){
    result = FALSE;
    goto Common_Exit;
  }

  /* Check that the size is correct */
  if ( 0 != jxta_vector_size( vec ) ){
    result = FALSE;
    goto Common_Exit;
  }
  
Common_Exit:
  for( i = 0; i < size; i++){
    if( pt[i] == NULL)
      JXTA_OBJECT_RELEASE(pt[i]);
  }
  JXTA_OBJECT_RELEASE( vec );

  return result;
}


static struct _funcs testfunc[] = {
  {*test_jxta_vector_new,              "jxta_vector_new"              },
  {*test_jxta_vector_add_object_at,    "jxta_vector_add_object_at"    },
  {*test_jxta_vector_add_object_first, "jxta_vector_add_object_first" },
  {*test_jxta_vector_add_object_last,  "jxta_vector_add_object_last"  },
  {*test_jxta_vector_clone,            "jxta_vector_clone"            },
  {*test_jxta_vector_remove_object_at, "jxta_vector_remove_object_at" },
  {*test_jxta_vector_clear,            "jxta_vector_clear"            },
  {NULL,                               "null"                         }
};


/**
* Run the unit tests for the jxta_vector test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_vector_tests( int * tests_run,
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





/*boolean 
jxta_vector_test(void) {

  int i;
  Jxta_vector* vector = jxta_vector_new (5);
  Jxta_vector* copy;
  Jxta_status err;

  for (i = 0; i < 10; ++i) {
    TestType* pt = (TestType*) malloc (sizeof (TestType));
    JXTA_OBJECT_INIT (pt, free, 0);
    printf ("Adding [%d] %p\n", i, pt);
    jxta_vector_add_object_last (vector, (Jxta_object*) pt);
  }
  printf("size= %d\n", jxta_vector_size(vector));

  for (i = 0; i < 10; ++i) {
    TestType* pt;
    jxta_vector_get_object_at (vector, (Jxta_object**) &pt, i);
    printf ("Get    [%d] %p\n", i, pt);
  }

  jxta_vector_remove_object_at (vector, NULL, 0);
  printf("size= %d\n", jxta_vector_size(vector));

  for (i = 0; i < jxta_vector_size (vector); ++i) {
    TestType* pt;
    jxta_vector_get_object_at (vector, (Jxta_object**) &pt, i);
    printf ("Get    [%d] %p\n", i, pt);
  }

  err = jxta_vector_clone (vector, &copy, 0, jxta_vector_size(vector));
  if (err != JXTA_SUCCESS) {
    printf ("Clone failed\n");
    return FALSE;
  }

  for (i = 0; i < jxta_vector_size (copy); ++i) {
    TestType* pt;
    jxta_vector_get_object_at (copy, (Jxta_object**) &pt, i);
    printf ("Copy   [%d] %p\n", i, pt);
  }

  return TRUE;

  }*/



/*#ifdef STANDALONE
int
main (int argc, char **argv) {
#ifdef WIN32 
    apr_app_initialize(&argc, &argv, NULL);
#else
    apr_initialize(); 
#endif

  if(jxta_vector_test()) {
    return TRUE;
  } else {
    return FALSE;
  }
}
#endif */
