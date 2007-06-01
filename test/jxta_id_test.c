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
 * $Id: jxta_id_test.c,v 1.9 2006/06/30 20:38:34 bondolo Exp $
 */


/*
 *  unit test and example code for jxta ids.  The jxtaid implementation 
 *  in this code is based on the "new" id format.  There needs to be 
 *  an implementation of the current reference format the way the java 
 *  code does it.
 */

#include <stdio.h>
#include <assert.h>
#include "jxta.h"
#include "jxta_types.h"
#include "unittest_jxta_func.h"

static const char * jxta_id_test_wellknown(void)
{
    Jxta_status res;
    JString *string1 = NULL;
    JString *string2 = NULL;
    Jxta_id *id1 = NULL;
    Jxta_id *id2 = NULL;

    res = jxta_id_get_uniqueportion(jxta_id_nullID, &string1);

    printf("unique : %s\n", jstring_get_string(string1));

    res = jxta_id_to_jstring(jxta_id_nullID, &string1);

    printf("id : %s\n", jstring_get_string(string1));

    res = jxta_id_from_jstring(&id1, string1);

    if (!jxta_id_equals(jxta_id_nullID, id1))
        return FILEANDLINE;

    res = jxta_id_get_uniqueportion(jxta_id_worldNetPeerGroupID, &string1);

    printf("unique : %s\n", jstring_get_string(string1));

    res = jxta_id_to_jstring(jxta_id_worldNetPeerGroupID, &string1);

    printf("id : %s\n", jstring_get_string(string1));

    res = jxta_id_from_jstring(&id1, string1);

    if (!jxta_id_equals(jxta_id_worldNetPeerGroupID, id1))
        return FILEANDLINE;

    res = jxta_id_get_uniqueportion(jxta_id_defaultNetPeerGroupID, &string1);

    printf("unique : %s\n", jstring_get_string(string1));

    res = jxta_id_to_jstring(jxta_id_defaultNetPeerGroupID, &string2);

    printf("id : %s\n", jstring_get_string(string2));

    res = jxta_id_from_jstring(&id2, string2);

    if (!jxta_id_equals(jxta_id_defaultNetPeerGroupID, id2))
        return FILEANDLINE;

    if (jxta_id_equals(id1, id2))
        return FILEANDLINE;

    return NULL;
}

static const char * jxta_id_test_peergroupid(void)
{
    JString *pgid = NULL;

    Jxta_id *peergroupid = NULL;
    jxta_id_peergroupid_new_1(&peergroupid);

    printf("format : %s\n", jxta_id_get_idformat(peergroupid));

    jxta_id_get_uniqueportion(peergroupid, &pgid);
    printf("unique : %s\n", jstring_get_string(pgid));
    JXTA_OBJECT_RELEASE(pgid);

    jxta_id_to_jstring(peergroupid, &pgid);
    printf("pg id  : %s\n", jstring_get_string(pgid));
    JXTA_OBJECT_RELEASE(pgid);

    JXTA_OBJECT_RELEASE(peergroupid);
    peergroupid = NULL;

    return NULL;
}


static const char * jxta_id_test_peerid(void)
{

    Jxta_id *peergroupid = NULL;
    Jxta_id *peerid = NULL;
    Jxta_id *peerid2;
    JString *pid = NULL;

    jxta_id_peergroupid_new_1(&peergroupid);
    jxta_id_peerid_new_1(&peerid, peergroupid);

    printf("format : %s\n", jxta_id_get_idformat(peerid));

    jxta_id_get_uniqueportion(peerid, &pid);
    printf("unique : %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(peerid, &pid);
    printf("peer id: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(peerid, &pid);
    peerid2 = NULL;
    jxta_id_from_jstring(&peerid2, pid);
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(peerid2, &pid);
    printf("peer id 2: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    printf("peer id == peer id 2 : %s\n", jxta_id_equals(peerid, peerid2) ? "true" : "false");

    jxta_id_peerid_new_1(&peerid2, jxta_id_worldNetPeerGroupID);

    jxta_id_to_jstring(peerid2, &pid);
    printf("peer id 2: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    printf("peer id == peer id 2 : %s\n", jxta_id_equals(peerid, peerid2) ? "true" : "false");

    JXTA_OBJECT_RELEASE(peergroupid);
    peergroupid = NULL;

    JXTA_OBJECT_RELEASE(peerid);
    peerid = NULL;

    JXTA_OBJECT_RELEASE(peerid2);
    peerid2 = NULL;

    /* This needs to return a TRUE to pass. */
    return NULL;
}

static const char * jxta_id_test_pipeid(void)
{

    Jxta_id *peergroupid = NULL;
    Jxta_id *pipeid = NULL;
    Jxta_id *pipeid2;
    JString *pid = NULL;

    jxta_id_peergroupid_new_1(&peergroupid);
    jxta_id_pipeid_new_1(&pipeid, peergroupid);

    printf("format : %s\n", jxta_id_get_idformat(pipeid));

    jxta_id_get_uniqueportion(pipeid, &pid);
    printf("unique : %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(pipeid, &pid);
    printf("pipe id: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(pipeid, &pid);
    pipeid2 = NULL;
    jxta_id_from_jstring(&pipeid2, pid);
    JXTA_OBJECT_RELEASE(pid);

    jxta_id_to_jstring(pipeid2, &pid);
    printf("pipe id 2: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    printf("pipe id == pipe id 2 : %s\n", jxta_id_equals(pipeid, pipeid2) ? "true" : "false");

    jxta_id_pipeid_new_1(&pipeid2, jxta_id_worldNetPeerGroupID);

    jxta_id_to_jstring(pipeid2, &pid);
    printf("pipe id 2: %s\n", jstring_get_string(pid));
    JXTA_OBJECT_RELEASE(pid);

    printf("pipe id == pipe id 2 : %s\n", jxta_id_equals(pipeid, pipeid2) ? "true" : "false");

    JXTA_OBJECT_RELEASE(peergroupid);
    peergroupid = NULL;

    JXTA_OBJECT_RELEASE(pipeid);
    pipeid = NULL;

    JXTA_OBJECT_RELEASE(pipeid2);
    pipeid2 = NULL;

    /* This needs to return a TRUE to pass. */
    return NULL;
}


static struct _funcs testfunc[] = {
    {*jxta_id_test_wellknown, "jxta_id_test_wellknown"},
    {*jxta_id_test_peergroupid, "jxta_id_test_peergroupid"},
    {*jxta_id_test_peerid, "jxta_id_test_peerid"},
    {*jxta_id_test_pipeid, "jxta_id_test_pipeid"},
    {NULL, "null"}
};


/**
* Run the unit tests for the jxta_id test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return NULL if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_id_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(testfunc, tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(testfunc, argc, argv);
}
#endif
