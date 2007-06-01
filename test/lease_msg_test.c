
#include <stdio.h>

#include <jxta.h>
#include <jxta_lease_request_msg.h>
#include <jxta_lease_response_msg.h>

#include "unittest_jxta_func.h"

const char *test_lease_request_construction(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_lease_request_msg *ad = jxta_lease_request_msg_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(ad);
       
    return NULL;
}

const char *test_lease_response_construction(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_lease_response_msg *ad = jxta_lease_response_msg_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(ad);
       
    return NULL;
}


const char *test_lease_request_serialization(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_lease_request_msg *ad;
    FILE *testfile;
    JString *dump1;
    JString *dump2;

    ad = jxta_lease_request_msg_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }

    testfile = fopen( "LeaseRequest.xml", "r");
    
    if( NULL == testfile ) {
        return FILEANDLINE;
    }
    
    result = jxta_lease_request_msg_parse_file(ad, testfile);
    fclose(testfile);

    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    result = jxta_lease_request_msg_get_xml(ad, &dump1);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    
    JXTA_OBJECT_RELEASE(ad);

    ad = jxta_lease_request_msg_new();
    result = jxta_lease_request_msg_parse_charbuffer( ad, jstring_get_string(dump1), jstring_length(dump1) );
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    result = jxta_lease_request_msg_get_xml(ad, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(ad);
    JXTA_OBJECT_RELEASE(dump1);
    JXTA_OBJECT_RELEASE(dump2);

    return NULL;
}

const char *test_lease_response_serialization(void) {
    Jxta_status result = JXTA_SUCCESS;
    Jxta_lease_response_msg *ad;
    FILE *testfile;
    JString *dump1;
    JString *dump2;

    ad = jxta_lease_response_msg_new();
    
    if( NULL == ad ) {
        return FILEANDLINE;
    }

    testfile = fopen( "LeaseResponse.xml", "r");
    
    if( NULL == testfile ) {
        return FILEANDLINE;
    }
    
    result = jxta_lease_response_msg_parse_file(ad, testfile);
    fclose(testfile);

    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    

    result = jxta_lease_response_msg_get_xml(ad, &dump1);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }    
    JXTA_OBJECT_RELEASE(ad);

    ad = jxta_lease_response_msg_new();
    result = jxta_lease_response_msg_parse_charbuffer( ad, jstring_get_string(dump1), jstring_length(dump1) );
    if( JXTA_SUCCESS != result ) {
        char * result = malloc( strlen(FILEANDLINE) + 10 + jstring_length(dump1) );
        strcpy( result, FILEANDLINE );
        strcat( result, "\n\n" );
        strcat( result, jstring_get_string(dump1) );
        strcat( result, "\n" );
        
        return result;
    }    

    result = jxta_lease_response_msg_get_xml(ad, &dump2);
    if( JXTA_SUCCESS != result ) {
        return FILEANDLINE;
    }
    
    if( 0 != jstring_equals( dump1, dump2 ) ) {
        return FILEANDLINE;
    }

    JXTA_OBJECT_RELEASE(ad);
    JXTA_OBJECT_RELEASE(dump1);
    JXTA_OBJECT_RELEASE(dump2);

    return NULL;
}

static struct _funcs lease_test_funcs[] = {
    /* constructor tests */
    {*test_lease_request_construction, "construction/destruction for lease request"},
    {*test_lease_response_construction, "construction/destruction for lease response"},

    /* Serialization/Deserialization */
    {*test_lease_request_serialization, "read/write test for lease request"},
    {*test_lease_response_serialization, "read/write test for lease response"},

    {NULL, "null"}
};

/**
* Run the unit tests for the lease_msg test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_lease_msg_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(lease_test_funcs, tests_run, tests_passed, tests_failed);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(lease_test_funcs, argc, argv);
}
#endif
