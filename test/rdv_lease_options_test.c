/* 
 * Copyright (c) 2001 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id$
 */

#include <stdio.h>

#include "jxta.h"
#include "jxta_rdv_lease_options.h"

#include "unittest_jxta_func.h"

const char *test_rdv_lease_options_construction(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_rdv_lease_options *ad = jxta_rdv_lease_options_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }
    
    if( !JXTA_OBJECT_CHECK_VALID(ad) ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(ad);
       
    return NULL;
}

const char *test_rdv_lease_options_serialization(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_rdv_lease_options *ad;
    Jxta_advertisement *raw_ad;
    FILE *testfile;
    JString *dump1;
    JString *dump2;
    Jxta_vector* child_advs = NULL;

    ad = jxta_rdv_lease_options_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }

    testfile = fopen( "RdvLeaseOptions.xml", "r");
    
    result = jxta_rdv_lease_options_parse_file(ad, testfile);
    fclose(testfile);

    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    if( !JXTA_OBJECT_CHECK_VALID(ad) ) {
        return FILEANDLINE;
    }

    result = jxta_rdv_lease_options_get_xml(ad, &dump1);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    
    JXTA_OBJECT_RELEASE(ad);
    ad = NULL;

    ad = jxta_rdv_lease_options_new();
    result = jxta_rdv_lease_options_parse_charbuffer( ad, jstring_get_string(dump1), jstring_length(dump1) );
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    if( !JXTA_OBJECT_CHECK_VALID(ad) ) {
        return FILEANDLINE;
    }

    result = jxta_advertisement_get_xml((Jxta_advertisement*) ad, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(dump2);

    raw_ad = jxta_advertisement_new();
    result = jxta_advertisement_parse_charbuffer(raw_ad, jstring_get_string(dump1), jstring_length(dump1));

    if( !JXTA_OBJECT_CHECK_VALID(raw_ad) ) {
        return FILEANDLINE;
    }
    
    jxta_advertisement_get_advs( raw_ad, &child_advs );
    if( NULL == child_advs ) {
        return FILEANDLINE;
    }
    
    if( 1 != jxta_vector_size( child_advs ) ) {
        return FILEANDLINE;
    }
    
    result = jxta_vector_get_object_at( child_advs, JXTA_OBJECT_PPTR(&ad), 0 );
    JXTA_OBJECT_RELEASE(child_advs);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }

    result = jxta_advertisement_get_xml((Jxta_advertisement*) ad, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(dump2);

    JXTA_OBJECT_RELEASE(raw_ad);
    JXTA_OBJECT_RELEASE(ad);
    JXTA_OBJECT_RELEASE(dump1);

    return NULL;
}

static struct _funcs rdv_lease_options_test_funcs[] = {
    /* First run simple construction destruction test. */
    {*test_rdv_lease_options_construction, "construction/destruction for RdvLeaseOptions"},

    /* Serialization/Deserialization */
    {*test_rdv_lease_options_serialization, "read/write test for RdvLeaseOptions"},

    {NULL, "null"}
};

/**
* Run the unit tests for the rdv_lease_options test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_rdv_lease_options_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(rdv_lease_options_test_funcs, tests_run, tests_passed, tests_failed);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(rdv_lease_options_test_funcs, argc, argv);
}
#endif
