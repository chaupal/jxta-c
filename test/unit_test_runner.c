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
 * $Id$
 */


#include <stdio.h>

/** FIXME: apr_general.h need to be included somewhere in jpr.
 *  None of the code will compile or run in win32 without it.
 */
#include <apr_general.h>

#include <jxta.h>

#include "unittest_jxta_func.h"

/**
* The prototype for the jstring_test runs. It is defined in
* jstring_test.c
*/
Jxta_boolean run_jstring_tests(int *tests_run, int *tests_passed, int *tests_failed);


/**
* The prototype for the jxta_bytevector_test runs. It is defined in
* jxta_bytevector_test.c
*/
Jxta_boolean run_jxta_bytevector_tests(int *tests_run, int *tests_passed, int *tests_failed);


/**
* The prototype for the jxta_vector_test runs. It is defined in
* jxta_vector_test.c
*/
Jxta_boolean run_jxta_vector_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
* The prototype for the jxta_xml_util_test runs. It is defined in
* jxta_xml_util_test.c
*/
Jxta_boolean run_jxta_xml_util_tests(int *tests_run, int *tests_passed, int *tests_failed);


/**
* The prototype for the jxtaobject_test runs. It is defined in
* jxtaobject_test.c
*/
Jxta_boolean jxtaobject_test(int *tests_run, int *tests_passed, int *tests_failed);


/**
* The prototype for the jxta_id_test runs. It is defined in
* jxta_id_test.c
*/
Jxta_boolean run_jxta_id_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
* The prototype for the jxta_hash_test runs. It is defined in
* jxta_hash_test.c
*/
Jxta_boolean run_jxta_hash_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
* The prototype for the jxta_objecthash_test runs. It is defined in
* jxta_hash_test.c
*/
Jxta_boolean run_jxta_objecthash_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
* The prototype for the excep_test runs. It is defined in
* excep_test.c
*/
Jxta_boolean run_excep_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
* The prototype for the dummypg_test runs. It is defined in
* dummypg_test.c
*/
Jxta_boolean run_dummypg_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
 * The prototype for the msg_test runs. It is defined in
* msg_test.c
*/
Jxta_boolean run_jxta_msg_tests(int *tests_run, int *tests_passed, int *tests_failed);


/**
 * The prototype for the dq_adv_test runs. It is defined in
* msg_test.c
*/
Jxta_boolean run_jxta_dq_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
 * The prototype for the pga_adv_test runs. It is defined in
* pga_adv_test.c
*/
Jxta_boolean run_jxta_pga_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
 * The prototype for the pa_adv_test runs. It is defined in
* pa_adv_test.c
*/
Jxta_boolean run_jxta_pa_tests(int *tests_run, int *tests_passed, int *tests_failed);

/**
 * The prototype for the dq_adv_test runs. It is defined in
* pga_adv_test.c
*/
Jxta_boolean run_jxta_lease_msg_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_apa_adv_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_route_adv_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_rdv_diffusion_msg_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_peerview_ping_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_peerview_pong_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_peerview_address_request_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_peerview_address_assign_tests(int *tests_run, int *tests_passed, int *tests_failed);

Jxta_boolean run_jxta_endpoint_msg_tests(int *tests_run, int *tests_passed, int *tests_failed);

/** 
* The list of tests to run, terminated by NULL
*/
static struct _suite testfuncs[] = {
    {*run_jstring_tests, "JString Tests"},
    {*run_jxta_bytevector_tests, "Jxta_bytevector Tests"},
    {*run_jxta_vector_tests, "Jxta_vector Tests"},
    {*run_jxta_xml_util_tests, "Jxta_xml_util Tests"},
    {*jxtaobject_test, "Jxta_object Tests"},
    {*run_jxta_id_tests, "Jxta_ID Tests"},
    {*run_jxta_hash_tests, "jxta_hash Tests"},
    {*run_jxta_objecthash_tests, "Jxta_objecthash Tests"}, 
    {*run_excep_tests, "excep Tests"},
    {*run_dummypg_tests, "dummypg Tests"},
    {*run_jxta_pga_tests, "JXTA_PGA Tests"},
    {*run_jxta_pa_tests, "JXTA_PA Tests"},
    {*run_jxta_dq_tests, "Jxta_dq Tests"},
    {*run_jxta_msg_tests, "Jxta_Message Tests"},
    {*run_jxta_lease_msg_tests, "Jxta_lease_{resquest|response}_msg Tests"},
    {*run_jxta_route_adv_tests, "Jxta_apa Tests"},
    {*run_jxta_apa_adv_tests, "Jxta_routea Tests"},
    {*run_jxta_rdv_diffusion_msg_tests, "Jxta_rdv_diffusion Tests"},
    {*run_jxta_peerview_ping_tests, "Jxta_peerview_ping_msg Tests"},
    {*run_jxta_peerview_pong_tests, "Jxta_peerview_pong_msg Tests"},
    {*run_jxta_peerview_address_request_tests, "Jxta_peerview_address_request_msg Tests"},
    {*run_jxta_peerview_address_assign_tests, "Jxta_peerview_address_assign_msg Tests"},

    {NULL, "null"}
};

int main(int argc, char **argv)
{
    return main_test_suites(testfuncs, argc, argv);
}
