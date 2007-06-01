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
 * $Id: unittest_jxta_func.h,v 1.2 2005/04/17 14:22:20 lankes Exp $
 */


/**
 * JString provides an encapsulation of c-style character 
 * buffers.
 *
 *
 * @warning JString is not thread-safe.
 */

#ifndef __UNITTEST_JXTA_FUNC_H__
#define __UNITTEST_JXTA_FUNC_H__

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif

/* Functions that can be used to run test functions */

/**
 * A structure that gives a char buffer to write or read from
 * and a position denoting the current read/write position
 */
struct _read_write_test_buffer{ 
  char * buffer;      /* The buffer to read or write */
  long position;      /* The current position */
};

typedef struct _read_write_test_buffer read_write_test_buffer; 


/** 
 * A structure that contains the test function to run
 * and status information
 */
struct _funcs {
  Jxta_boolean (*test)(void);
  const char * states;
};


/**
* Runs the test functions given in  testfunc. The number of tests run, passed and failed is
* updated as the tests are run
* 
* @param testfunc array of test functions to run, terminated by a NULL entry
* @param tests_run the variable to increment for each test run
* @param tests_passed the variable to increment for each test passed
* @param tests_failed the variable to increment for each test failed
*
* @return TRUE if all tests run successfully, FALSE otherwise
*/
Jxta_boolean run_testfunctions(const struct _funcs testfunc[],
			       int * tests_run,
			       int * tests_passed,
			       int * tests_failed);


/** 
 * A helper function that can be called from the main function if the test programs 
 * are run as standalone application. This is defined here so that the code does not 
 * need to be repeated in each test source file
 * 
 * @param testfunc array of test functions to run, terminated by a NULL entry
 * @param argc the number of command line arguments
 * @param argv the array of command line arguments
 */
int  main_test_function(const struct _funcs testfunc[],int argc, char ** argv);

/**
 *  Implementation of SendFunc, used for unit test, that writes the 
 *  data from buf into stream. If flag is equal to 1, the data are written to
 *  stream, which is assumed to be a character buffer. Otherwise, "flag set" is
 *  written to stream.
 * 
 *  @param stream the character buffer to which to write the data
 *  @param buf the buffer from which to copy the data
 *  @param len the amount of data to copy
 *  @param flag the flag that determines the behavior. See above for the effect of
 *         flag.
 * 
 *  @return always returns JXTA_SUCCESS
 */
Jxta_status JXTA_STDCALL
sendFunction(void *stream, const char *buf, size_t len, unsigned int flag);

/**
 * Implementation of  WriteFunc, used for unit test, that writes the 
 *  data from buf into stream,  which is assumed to be a character buffer.
 * 
 *  @param strean the character buffer to which to write the data
 *  @param buf the buffer from which to copy the data
 *  @param len the amount of data to copy
 *         
 *  @return always returns JXTA_SUCCESS
 */
Jxta_status JXTA_STDCALL
writeFunction(void *stream, const char *buf, size_t len);



/**
 * Implementation of  ReadFunc, used for unit test, that reads data from 
 * stream, which is assumed to be a character buffer. The data are then put
 * into buf.
 * 
 *  @param stream the character buffer from which to read the data
 *  @param buf the buffer  to which to copy the data
 *  @param len the amount of data to copy
 * 
 *  @return always returns JXTA_SUCCESS
 */
Jxta_status JXTA_STDCALL
readFromStreamFunction(void *stream, char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __UNITTEST_JXTA_FUNC_H__ */
