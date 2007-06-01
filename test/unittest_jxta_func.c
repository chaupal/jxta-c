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
 * $Id: unittest_jxta_func.c,v 1.7 2005/04/17 14:22:20 lankes Exp $
 */

#include <stdio.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

#include "unittest_jxta_func.h"

Jxta_boolean 
run_testfunctions(const struct _funcs testfunc[],
		  int * tests_run,
		  int * tests_passed,
		  int * tests_failed){
  Jxta_boolean passed = TRUE;
  int i = 0;

  while (testfunc[i].test != NULL) {
     *tests_run += 1;
      if (testfunc[i].test()){
	fprintf(stdout,"Passed test_%s state test\n",testfunc[i].states);
	*tests_passed += 1;
      }
      else
	{
	  fprintf(stdout,"Failed test_%s state test\n",testfunc[i].states);
	  passed = FALSE;
	  *tests_failed += 1;
	}
      i++;
    }

  return passed;
}

/** 
 * A helper function that can be called from the main function if the test programs 
 * are run as standalone application.
 */
int  
main_test_function(const struct _funcs testfunc[],int argc, char ** argv){
  int run,failed,passed;
  Jxta_boolean result;
  
  run = failed = passed = 0;

    jxta_initialize();
 
  result = run_testfunctions(testfunc, &run, &passed, &failed);
  fprintf(stdout,"Tests run:    %d\n",run);  
  fprintf(stdout,"Tests passed: %d\n",passed);  
  fprintf(stdout,"Tests failed: %d\n",failed);  
  if( result == FALSE)
    fprintf(stdout,"Some tests failed\n");

  if( result == TRUE) run = 0;
  else                run = -1;

  jxta_terminate();
  return run;
}


/**
 * Implemenation of SendFunc
 */
Jxta_status JXTA_STDCALL
sendFunction(void *stream, const char *buf, size_t len, unsigned int flag){
  read_write_test_buffer * buffer = (read_write_test_buffer *)stream;
  int i;
  
  if(flag == 1) {
    for( i = 0; i < len; i++){
      buffer->buffer[i + buffer->position] = buf[i];
    }
    buffer->buffer[len + buffer->position] = '\0';
    buffer->position += len;
  }
  else  {
    strcat(buffer->buffer,"flag set");
    buffer->position += strlen( buffer->buffer);  
  }
  return JXTA_SUCCESS;
}


/**
 * Implementation of WriteFunc
 */
Jxta_status JXTA_STDCALL
writeFunction(void *stream, const char *buf, size_t len){
  read_write_test_buffer * target = (read_write_test_buffer *)stream;
  int i;
  
  for( i = 0; i < len; i++){
    target->buffer[i + target->position] = buf[i];
  }
  target->buffer[len + target->position] = '\0';
  target->position += len;
  return JXTA_SUCCESS;
}


/**
 * Implemenation of ReadFunc
 */
Jxta_status JXTA_STDCALL
readFromStreamFunction(void *stream, char *buf, size_t len){
  read_write_test_buffer * buffer = (read_write_test_buffer *)stream;
  int i;

  for( i = 0; i < len; i++){
    buf[i] = buffer->buffer[i + buffer->position];
  }
  buffer->position += len;
  return JXTA_SUCCESS;
}


