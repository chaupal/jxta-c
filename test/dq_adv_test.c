#include <stdio.h>
#include "jxta_errno.h"
#include "jxta_discovery_service.h"
#include "jxta_dq.h"

#include "unittest_jxta_func.h"

/**
* Create a discovery advertisment using jxta_discovery_query_new
* 
* @return the newly created Jxta_DiscoveryQuery if successful, NULL
*         otherwise
*/
Jxta_DiscoveryQuery *createDiscoveryQuery()
{
    Jxta_DiscoveryQuery *query = jxta_discovery_query_new();
    JString *peerAdv = jstring_new_2("The Peer Advertisement content");
    JString *attr = jstring_new_2("The Attribute");
    JString *value = jstring_new_2("The Value");

    if (query == NULL && peerAdv == NULL) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }

    /* Set the type of the query */
    if (JXTA_SUCCESS != jxta_discovery_query_set_type(query, DISC_PEER)) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }

    /* Set  the threshold */
    if (JXTA_SUCCESS != jxta_discovery_query_set_threshold(query, 20)) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }


    /* Set the peer advertisment */
    JXTA_OBJECT_SHARE(peerAdv);
    if (JXTA_SUCCESS != jxta_discovery_query_set_peeradv(query, peerAdv)) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }

    /* Set the attribute value */
    JXTA_OBJECT_SHARE(attr);
    if (JXTA_SUCCESS != jxta_discovery_query_set_attr(query, attr)) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }

    /* Set the value */
    JXTA_OBJECT_SHARE(value);
    if (JXTA_SUCCESS != jxta_discovery_query_set_value(query, value)) {
        if (query != NULL)
            JXTA_OBJECT_RELEASE(query);
        query = NULL;
        goto Common_Exit;
    }
  Common_Exit:
    if (peerAdv != NULL)
        JXTA_OBJECT_RELEASE(peerAdv);
    if (attr != NULL)
        JXTA_OBJECT_RELEASE(attr);
    if (value != NULL)
        JXTA_OBJECT_RELEASE(value);

    return query;
}


/**
* Create a discovery advertisment using jxta_discovery_query_new_1
* 
* @return the newly created Jxta_DiscoveryQuery if successful, NULL
*         otherwise
*/
Jxta_DiscoveryQuery *createDiscoveryQuery_1()
{
    Jxta_DiscoveryQuery *query = NULL;
    JString *peerAdv = jstring_new_2("The Peer Advertisement content");

    if (peerAdv == NULL)
        return query;

    JXTA_OBJECT_SHARE(peerAdv);
    query = jxta_discovery_query_new_1(DISC_PEER, "The Attribute", "The Value", 20, peerAdv);
    JXTA_OBJECT_RELEASE(peerAdv);
    return query;
}

/**
* Check that the discovery message contains the correct data
* 
* @param query the query to check
* @return TRUE if the message is correct, FALSE otherwise
*/
Jxta_boolean checkDiscoveryQuery(Jxta_DiscoveryQuery * query)
{
    JString *comp = NULL;
    Jxta_boolean result = TRUE;

    if (query == NULL) {
        result = FALSE;
        goto Common_Exit;
    }

    /* Check the type of the query */
    if (jxta_discovery_query_get_type(query) != DISC_PEER) {
        result = FALSE;
        goto Common_Exit;
    }

    /* Check  the threshold */
    if (jxta_discovery_query_get_threshold(query) != 20) {
        result = FALSE;
        goto Common_Exit;
    }

    /* Check the peer advertisment */
    if (JXTA_SUCCESS != jxta_discovery_query_get_peeradv(query, &comp)) {
        result = FALSE;
        goto Common_Exit;
    }
    if (comp == NULL || strcmp("The Peer Advertisement content", jstring_get_string(comp)) != 0) {
        result = FALSE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(comp);
    comp = NULL;

    /* Check the attribute */
    if (JXTA_SUCCESS != jxta_discovery_query_get_attr(query, &comp)) {
        result = FALSE;
        goto Common_Exit;
    }
    if (comp == NULL || strcmp("The Attribute", jstring_get_string(comp)) != 0) {
        result = FALSE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(comp);
    comp = NULL;

    /* Check the value */
    if (JXTA_SUCCESS != jxta_discovery_query_get_value(query, &comp)) {
        result = FALSE;
        goto Common_Exit;
    }
    if (comp == NULL || strcmp("The Value", jstring_get_string(comp)) != 0) {
        result = FALSE;
        goto Common_Exit;
    }
    JXTA_OBJECT_RELEASE(comp);
    comp = NULL;


  Common_Exit:
    if (comp != NULL)
        JXTA_OBJECT_RELEASE(comp);
    return result;
}

/**
 * Test the empty constructor for jxta_discovery_query
 */
Jxta_boolean test_jxta_discovery_query_new(void)
{
    Jxta_DiscoveryQuery *query = createDiscoveryQuery();
    Jxta_boolean result = TRUE;

    if (query == NULL)
        return FALSE;

    if (!checkDiscoveryQuery(query))
        result = FALSE;

    jxta_discovery_query_free(query);

    return result;
}

/**
 * Test the _1 constructor for jxta_discovery_query
 */
Jxta_boolean test_jxta_discovery_query_new_1(void)
{
    Jxta_DiscoveryQuery *query = createDiscoveryQuery_1();
    Jxta_boolean result = TRUE;

    if (query == NULL)
        return FALSE;

    if (!checkDiscoveryQuery(query))
        result = FALSE;

    jxta_discovery_query_free(query);

    return result;
}

/**
 * Test the read/write functionality of the  Jxta_DiscoveryQuery
 * object
 */
Jxta_boolean test_jxta_discovery_read_write(void)
{
    Jxta_DiscoveryQuery *query = createDiscoveryQuery_1();
    Jxta_boolean result = TRUE;
    JString *doc = NULL;

    if (query == NULL)
        return FALSE;

    /* Write the query to a string */
    if (JXTA_SUCCESS != jxta_discovery_query_get_xml(query, &doc)) {
        result = FALSE;
        goto Common_Exit;
    }

    /* Read the message back in */
    jxta_discovery_query_free(query);
    query = jxta_discovery_query_new();
    if (query == NULL) {
        result = FALSE;
        goto Common_Exit;
    }
    if (JXTA_SUCCESS != jxta_discovery_query_parse_charbuffer(query, jstring_get_string(doc), jstring_length(doc))) {
        result = FALSE;
        goto Common_Exit;
    }

  /** Check that the message is correct */
    if (!checkDiscoveryQuery(query)) {
        result = FALSE;
        goto Common_Exit;
    }
  Common_Exit:
    if (doc != NULL)
        JXTA_OBJECT_RELEASE(doc);
    if (query != NULL)
        jxta_discovery_query_free(query);
    return result;
}

static struct _funcs testfunc[] = {
    {*test_jxta_discovery_query_new, "test_jxta_discovery_query_new"},
    {*test_jxta_discovery_query_new_1, "test_jxta_discovery_query_new_1"},
    {*test_jxta_discovery_read_write, "test_jxta_discovery_read_write"},

    {NULL, "null"}
};


/**
* Run the unit tests for the jxta_dq test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_dq_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(testfunc, tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(testfunc, argc, argv);
}
#endif
