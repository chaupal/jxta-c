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
 * $Id: unit_test_runner.c,v 1.4 2005/04/07 22:58:57 slowhog Exp $
 */


#include <stdio.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

/** 
 * A structure that contains the test function to run
 * and status information
 */
struct _unit_tests {
  Jxta_boolean (*test)(int *, int *, int *);
  const char * states;
};


/**
* The prototype for the jstring_test runs. It is defined in
* jstring_test.c
*/
Jxta_boolean run_jstring_tests( int * tests_run,
				int * tests_passed,
				int * tests_failed);


/**
* The prototype for the jxta_bytevector_test runs. It is defined in
* jxta_bytevector_test.c
*/
Jxta_boolean run_jxta_bytevector_tests( int * tests_run,
				        int * tests_passed,
				        int * tests_failed);


/**
* The prototype for the jxta_vector_test runs. It is defined in
* jxta_vector_test.c
*/
Jxta_boolean run_jxta_vector_tests( int * tests_run,
				    int * tests_passed,
				    int * tests_failed);

/**
* The prototype for the jxta_xml_util_test runs. It is defined in
* jxta_xml_util_test.c
*/
Jxta_boolean run_jxta_xml_util_tests( int * tests_run,
				      int * tests_passed,
				      int * tests_failed);


/**
* The prototype for the jxtaobject_test runs. It is defined in
* jxtaobject_test.c
*/
Jxta_boolean 
jxtaobject_test( int * tests_run,
		 int * tests_passed,
		 int * tests_failed);


/**
* The prototype for the jxta_id_test runs. It is defined in
* jxta_id_test.c
*/
Jxta_boolean run_jxta_id_tests( int * tests_run,
				int * tests_passed,
				int * tests_failed);

/**
* The prototype for the jxta_hash_test runs. It is defined in
* jxta_hash_test.c
*/
Jxta_boolean run_jxta_hash_tests( int * tests_run,
				  int * tests_passed,
				  int * tests_failed);

/**
* The prototype for the jxta_objecthash_test runs. It is defined in
* jxta_hash_test.c
*/
Jxta_boolean run_jxta_objecthash_tests( int * tests_run,
				        int * tests_passed,
				        int * tests_failed);

/**
* The prototype for the excep_test runs. It is defined in
* excep_test.c
*/
Jxta_boolean run_excep_tests(int * tests_run,
			     int * tests_passed,
			     int * tests_failed);

/**
* The prototype for the dummypg_test runs. It is defined in
* dummypg_test.c
*/
Jxta_boolean run_dummypg_tests(int * tests_run,
			       int * tests_passed,
			       int * tests_failed);

/**
 * The prototype for the msg_test runs. It is defined in
* msg_test.c
*/
Jxta_boolean run_jxta_msg_tests( int * tests_run,
				 int * tests_passed,
				 int * tests_failed);


/**
 * The prototype for the dq_adv_test runs. It is defined in
* msg_test.c
*/
Jxta_boolean run_jxta_dq_tests( int * tests_run,
				int * tests_passed,
				int * tests_failed);


/** 
* The list of tests to run, terminated by NULL
*/
static struct _unit_tests testfunc[] = {
  {*run_jstring_tests,         "jstring_test"         },
  {*run_jxta_bytevector_tests, "jxta_bytevector_test" },
  {*run_jxta_vector_tests,     "jxta_vector_test"     },
  {*run_jxta_xml_util_tests,   "jxta_xml_util_test"   },
  {*jxtaobject_test,           "jxtaobject_test"      },
  {*run_jxta_id_tests,         "jxta_id_test"         },
  {*run_jxta_hash_tests,       "jxta_hash_test"       },
  {*run_jxta_objecthash_tests, "jxta_objecthash_test" },
  {*run_excep_tests,           "run_excep_tests"      },
  {*run_dummypg_tests,         "run_dummypg_tests"    },
  {*run_dummypg_tests,         "run_dummypg_tests"    },
  {*run_jxta_msg_tests,        "run_msg_tests"        },
  {*run_jxta_dq_tests,         "run_jxta_dq_tests"    },

  {NULL,                        "null"                }
};

int  
main(int argc, char ** argv) {
  int run,failed,passed;
  Jxta_boolean result = TRUE, tmp;
  int i = 0;
  
  run = failed = passed = 0;

    jxta_initialize();
 
    while (testfunc[i].test != NULL) {
      fprintf(stdout,"=========== %s ============\n",testfunc[i].states);
      tmp = testfunc[i].test(  &run, &passed, &failed );
      if( tmp == FALSE) {
         result = FALSE;
         fprintf(stdout,"Test: %s failed\n",testfunc[i].states);
      }
      i++;
    }

  fprintf(stdout,"Tests run:    %d\n",run);  
  fprintf(stdout,"Tests passed: %d\n",passed);  
  fprintf(stdout,"Tests failed: %d\n",failed);  
  if( result == FALSE)
    fprintf(stdout,"Some tests failed\n");

  if( result == TRUE)  i = 0;
  else                 i = -1;

  jxta_terminate();
  return i;
}
