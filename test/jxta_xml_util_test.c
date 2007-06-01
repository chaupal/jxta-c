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
 * $Id: jxta_xml_util_test.c,v 1.10 2005/09/23 20:07:15 slowhog Exp $
 */

#include <stdio.h>

/* Ok, here is the deal: apr is not dealing with 
 * arpa/ and netinet/ very well.  At some point 
 * it probably will.  In the meantime, this stuff
 * will have to be manually defined the way we want it.
 * It will likely be a full days work to sort it out
 * among linux, solaris, win32, bsd etc.
 */
#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#endif
#include <apr_want.h>
#include <apr_network_io.h>

#include <jxta.h>
#include "jxta_xml_util.h"
#include "unittest_jxta_func.h"

/** Exercise the function for extracting ip numbers
 *  and ports from a character buffer.
 */
Jxta_boolean extract_ip_and_port_test(void)
{

    Jxta_boolean passed = TRUE;
    int length;
    Jxta_in_addr ip = 0;
    Jxta_port port = 0;
    int addr;
    const char *all_delims = "    \t \n";
    const char *ip_port = "\n\n127.0.0.1:9700\t\n";

    length = strlen(all_delims);
    extract_ip_and_port(all_delims, length, &ip, &port);

    if (ip != 0 || port != 0) {
        printf("Failed extract_ip_and_port()\n");
        passed = FALSE;
    } else {
        printf("Passed extract_ip_and_port()\n");
    }

    length = strlen(ip_port);
    extract_ip_and_port(ip_port, length, &ip, &port);
    addr = apr_inet_addr("127.0.0.1");
    if (addr == ip && port == 9700) {
        printf("Passed extract_ip_and_port()\n");
    } else {
        printf("Failed extract_ip_and_port()\n");
        passed = FALSE;
    }

    return passed;
}


Jxta_boolean extract_ip_test(void)
{

    Jxta_boolean passed = TRUE;
    int length;
    Jxta_in_addr ip = 0;
    int addr;
    const char *all_delims = "    \t \n";
    const char *ip_port = "\n\n127.0.0.1\t\n";

    length = strlen(all_delims);
    extract_ip(all_delims, length, &ip);

    if (ip != 0) {
        printf("Failed extract_ip_test()\n");
        passed = FALSE;
    } else {
        printf("Passed extract_ip_test()\n");
    }

    length = strlen(ip_port);
    extract_ip(ip_port, length, &ip);
    addr = apr_inet_addr("127.0.0.1");
    if (addr == ip) {
        printf("Passed extract_ip_test()\n");
    } else {
        printf("Failed extract_ip_test()\n");
        passed = FALSE;
    }

    return passed;
}


Jxta_boolean extract_port_test(void)
{

    Jxta_boolean passed = TRUE;
    int length;
    Jxta_port port = 0;
    const char *all_delims = "    \t \n";
    const char *ip_port = "\n\n9700\t\n";

    length = strlen(all_delims);
    extract_port(all_delims, length, &port);

    if (port != 0) {
        printf("Failed extract_port_test() all delims\n");
        passed = FALSE;
    } else {
        printf("Passed extract_port_test() all delims\n");
    }

    length = strlen(ip_port);
    extract_port(ip_port, length, &port);

    if (port == 9700) {
        printf("Passed extract_port_test() port = 9700\n");
    } else {
        printf("Failed extract_port_test() port = 9700\n");
        passed = FALSE;
    }

    return passed;
}

Jxta_boolean extract_char_data_test(void)
{

    Jxta_boolean passed = TRUE;
    const char *all_delims = "    \t \n";
    const char *string = "  \n\n foo.bar.com \t\n  ";
    char tok[128] = { 0 };

    extract_char_data(all_delims, strlen(all_delims), tok);
#ifdef DEBUG
    printf("tok: %s, length: %d\n", tok, strlen(tok));
#endif
    /* This test may need to be used in other places 
     * where tok is tested against NULL.
     */
    if (*tok == '\0') {
        printf("Passed extract_char_data_test() with empty string\n");
    } else {
        printf("Failed extract_char_data_test() with empty string\n");
        passed = FALSE;
    }

    extract_char_data(string, strlen(string), tok);
#ifdef DEBUG
    printf("tok: %s\n", tok);
#endif
    if (!strncmp("foo.bar.com", tok, strlen(string))) {
        printf("Passed extract_char_data_test() with valid string\n");
    } else {
        printf("Failed extract_char_data_test() with valid string\n");
        passed = FALSE;
    }

    return passed;
}

Jxta_boolean encode_xml_test(void)
{
    Jxta_status res = JXTA_SUCCESS;
    const char *source = "<xml>&</xml>";
    const char *expect = "&lt;xml&gt;&amp;&lt;/xml&gt;";
    JString *input;
    JString *output = NULL;

    input = jstring_new_2(source);

    res = jxta_xml_util_encode_jstring(input, &output);

    printf("input='%s'\noutput='%s'\nexpect='%s'\nres=%d\n", source, jstring_get_string(output), expect, res);
    return (0 == strcmp(expect, jstring_get_string(output)));
}

Jxta_boolean decode_xml_test(void)
{
    Jxta_status res = JXTA_SUCCESS;
    const char *source = "&lt;xml&gt;&amp;&lt;/xml&gt;";
    const char *expect = "<xml>&</xml>";

    JString *input;
    JString *output = NULL;

    input = jstring_new_2(source);

    res = jxta_xml_util_decode_jstring(input, &output);

    printf("input='%s'\noutput='%s'\nexpect='%s'\nres=%d\n", source, jstring_get_string(output), expect, res);

    return (0 == strcmp(expect, jstring_get_string(output)));
}

static struct _funcs testfunc[] = {
    {*extract_ip_and_port_test, "extract_ip_and_port_test"},
    {*extract_ip_test, "extract_ip_test"},
    {*extract_port_test, "extract_port_test"},
    {*extract_char_data_test, "extract_char_data_test"},
    {*encode_xml_test, "encode_xml_test"},
    {*decode_xml_test, "decode_xml_test"},
    {NULL, "null"}
};



/**
* Run the unit tests for the jxta_xml_util test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_xml_util_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(testfunc, tests_run, tests_passed, tests_failed);
}


/* If the test code ever needs to be compiled into a 
 * test library all of the main methods will need to 
 * be wrapped so that the linker doesn't choke.
 */
#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(testfunc, argc, argv);
}
#endif
