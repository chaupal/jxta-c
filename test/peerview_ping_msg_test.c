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

#include "jxta_peerview_ping_msg.h"

#include "unittest_jxta_func.h"

const char *test_jxta_peerview_ping_construction(void) {
    Jxta_peerview_ping_msg *msg = jxta_peerview_ping_msg_new();
    
    if( NULL == msg ) {
        return FILEANDLINE;
    }
    
    if( !JXTA_OBJECT_CHECK_VALID(msg) ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(msg);
       
    return NULL;
}

const char *test_jxta_peerview_ping_group_entry(void) {
    Jxta_pv_ping_msg_group_entry *entry = jxta_pv_ping_msg_entry_new(NULL, "group1", TRUE);

    jxta_peerview_ping_msg_entry_set_pv_id_only(entry, TRUE);

    JString *group_name = jxta_peerview_ping_msg_entry_group_name(entry);


    if (NULL == group_name || 0 != strcmp(jstring_get_string(group_name), "group1")) {
        return FILEANDLINE;
    }

    if (FALSE == jxta_peerview_ping_msg_entry_is_pv_id_only(entry)) {
        return FILEANDLINE;
    }

    return NULL;
}

const char *test_jxta_peerview_ping_serialization(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_peerview_ping_msg *msg;
    Jxta_advertisement *raw_msg;
    FILE *testfile;
    JString *dump1;
    JString *dump2;
    Jxta_vector* child_advs = NULL;
    Jxta_vector *entries=NULL;
    int i;
    Jxta_pv_ping_msg_group_entry *entry;
    char group_name[128];


    jxta_advertisement_register_global_handler("jxta:PeerviewPing", (JxtaAdvertisementNewFunc) jxta_peerview_ping_msg_new );

    msg = jxta_peerview_ping_msg_new();
    
    if( NULL == msg ) {
        return FILEANDLINE;
    }

    for (i=0; i<3; i++) {
        apr_snprintf(group_name, sizeof(group_name), "group%d", i + 1);
        entry = jxta_pv_ping_msg_entry_new(NULL, group_name, TRUE);
        if (JXTA_SUCCESS != jxta_peerview_ping_msg_add_group_entry(msg, entry)) {
            return FILEANDLINE;
        }
        JXTA_OBJECT_RELEASE(entry);
    }
    if (JXTA_SUCCESS != jxta_peerview_ping_msg_get_group_entries(msg, &entries)) {
        return FILEANDLINE;
    }

    for (i=0; i < jxta_vector_size(entries); i++) {
        JString *group_j;

        if (JXTA_SUCCESS != jxta_vector_get_object_at(entries, JXTA_OBJECT_PPTR(&entry), i)) {
            return FILEANDLINE;
        }
        group_j = jxta_peerview_ping_msg_entry_group_name(entry);
        apr_snprintf(group_name, sizeof(group_name), "group%d", i + 1);
        if (0 != strcmp(jstring_get_string(group_j), group_name)) {
            return FILEANDLINE;
        }
        JXTA_OBJECT_RELEASE(group_j);
        JXTA_OBJECT_RELEASE(entry);
    }

    testfile = fopen( "PeerviewPing.xml", "r");
    
    result = jxta_peerview_ping_msg_parse_file(msg, testfile);
    fclose(testfile);

    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    if( !JXTA_OBJECT_CHECK_VALID(msg) ) {
        return FILEANDLINE;
    }

    result = jxta_peerview_ping_msg_get_xml(msg, &dump1);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    
    JXTA_OBJECT_RELEASE(msg);
    msg = NULL;
    msg = jxta_peerview_ping_msg_new();
    result = jxta_peerview_ping_msg_parse_charbuffer( msg, jstring_get_string(dump1), jstring_length(dump1) );
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    if( !JXTA_OBJECT_CHECK_VALID(msg) ) {
        return FILEANDLINE;
    }

    result = jxta_advertisement_get_xml( (Jxta_advertisement *) msg, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(dump2);

    raw_msg = jxta_advertisement_new();
    result = jxta_advertisement_parse_charbuffer(raw_msg, jstring_get_string(dump1), jstring_length(dump1));

    if( !JXTA_OBJECT_CHECK_VALID(raw_msg) ) {
        return FILEANDLINE;
    }
    
    jxta_advertisement_get_advs( raw_msg, &child_advs );
    if( NULL == child_advs ) {
        return FILEANDLINE;
    }
    
    if( 1 != jxta_vector_size( child_advs ) ) {
        return FILEANDLINE;
    }
    
    result = jxta_vector_get_object_at( child_advs, JXTA_OBJECT_PPTR(&msg), 0 );
    JXTA_OBJECT_RELEASE(child_advs);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }

    result = jxta_advertisement_get_xml( (Jxta_advertisement *) msg, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }
    JXTA_OBJECT_RELEASE(dump2);

    JXTA_OBJECT_RELEASE(raw_msg);
    JXTA_OBJECT_RELEASE(msg);
    JXTA_OBJECT_RELEASE(dump1);

    return NULL;
}

static struct _funcs peerview_ping_test_funcs[] = {
    /* First run simple construction destruction test. */
    {*test_jxta_peerview_ping_construction, "Construction/destruction for Peerview Ping"},
    {*test_jxta_peerview_ping_group_entry, "Test Group entry functions"},
    /* Serialization/Deserialization */
    {*test_jxta_peerview_ping_serialization, "Read/write test for Peerview Ping"},

    {NULL, "null"}
};

/**
* Run the unit tests for the jxta_pa test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_peerview_ping_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(peerview_ping_test_funcs, tests_run, tests_passed, tests_failed);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(peerview_ping_test_funcs, argc, argv);
}
#endif
