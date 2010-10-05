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
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "search.h"

#include "jxta_shell_getopt.h"

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_search_new(Jxta_PG * pg,
                                      Jxta_listener * standout,
                                      JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_search_print_help,
                                      jxta_search_start, (shell_application_stdin) jxta_search_process_input);
    group = pg;
    environment = env;
    return app;
}


void jxta_search_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void jxta_search_start(Jxta_object * appl, int argv, char **arg)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, (const char **) arg, "rflhp:n:a:v:q:i:");
    JString *pid = jstring_new_0();
    JString *attr = jstring_new_0();
    JString *value = jstring_new_0();
    JString *query = jstring_new_0();
    JString *infile = jstring_new_0();
    Jxta_boolean rf = FALSE;
    Jxta_boolean pf = FALSE;
    Jxta_boolean qf = FALSE;
    Jxta_boolean ff = FALSE;
    Jxta_boolean inf = FALSE;

    Jxta_discovery_service *discovery;
    long qid = -1;
    long responses = 10;
    int error = 0;
    Jxta_vector *res_vec = NULL;

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);

        if (type == -1)
            break;
        switch (type) {
        case 'r':
            rf = TRUE;
            break;
        case 'f':
            ff = TRUE;
            break;
        case 'p':
            pf = TRUE;
            jstring_append_1(pid, JxtaShellGetopt_OptionArgument(opt));
            break;
        case 'q':
            qf = TRUE;
            jstring_append_1(query, JxtaShellGetopt_OptionArgument(opt));
            break;
        case 'i':
            qf = TRUE;
            inf = TRUE;
            jstring_append_1(infile, JxtaShellGetopt_OptionArgument(opt));
            break;
        case 'n':
            responses = strtol(jstring_get_string(JxtaShellGetopt_OptionArgument(opt)), NULL, 0);
            break;
        case 'a':
            jstring_append_1(attr, JxtaShellGetopt_OptionArgument(opt));
            break;
        case 'v':
            jstring_append_1(value, JxtaShellGetopt_OptionArgument(opt));
            break;
        case 'h':
            jxta_search_print_help(appl);
            break;
        default:
            jxta_search_print_help(appl);
            error = 1;
            break;
        }
        if (error != 0)
            break;
    }
    JxtaShellGetopt_delete(opt);
    jxta_PG_get_discovery_service(group, &discovery);
    if (ff) {
		JString *tmp_str = NULL;
        discovery_service_flush_advertisements(discovery, NULL, DISC_ADV);
        tmp_str = jstring_new_2("Advertisement");
        JxtaShellEnvironment_delete_type(environment, tmp_str);
        JxtaShellApplication_terminate(app);
        JXTA_OBJECT_RELEASE(tmp_str);
        JXTA_OBJECT_RELEASE(pid);
        JXTA_OBJECT_RELEASE(attr);
        JXTA_OBJECT_RELEASE(value);
        JXTA_OBJECT_RELEASE(query);
        JXTA_OBJECT_RELEASE(infile);
        JXTA_OBJECT_RELEASE(discovery);
        return;
    }
    if (rf) {
        char *paddr = NULL;
        if (pf) {
            paddr = (char *) jstring_get_string(pid);
        }
        if (!qf) {
            qid = discovery_service_get_remote_advertisements(discovery,
                                                              (Jxta_id *) paddr,
                                                              DISC_ADV,
                                                              (char *) jstring_get_string(attr),
                                                              (char *) jstring_get_string(value), responses, NULL);
        } else {
            if (inf) {
                search_read_query_file(infile, query);
            }

            qid = discovery_service_remote_query(discovery,
                                                 (Jxta_id *) paddr, (char *) jstring_get_string(query), responses, NULL);
        }
        printf("query ID: %ld\n", qid);

    } else {
        if (!qf) {
            discovery_service_get_local_advertisements(discovery,
                                                       DISC_ADV,
                                                       (char *) jstring_get_string(attr),
                                                       (char *) jstring_get_string(value), &res_vec);
        } else {
            if (inf) {
                search_read_query_file(infile, query);
            }
            discovery_service_local_query(discovery, (char *) jstring_get_string(query), &res_vec);
        }

        if (res_vec != NULL) {
            unsigned int i;
            Jxta_PA *padv;
            char buf[64];
            JString *name = NULL;
            JxtaShellObject *sh_obj;
            printf("restored %d advertisement(s) \n", jxta_vector_size(res_vec));
            for (i = 0; i < jxta_vector_size(res_vec); i++) {
                JString *env_type;
                sprintf(buf, "adv%d", i);
                name = jstring_new_2(buf);
                jxta_vector_get_object_at(res_vec, JXTA_OBJECT_PPTR(&padv), i);
                env_type = jstring_new_2("Advertisement");
                sh_obj = JxtaShellObject_new(name, (Jxta_object *) padv, env_type);
                JxtaShellEnvironment_add_0(environment, sh_obj);
                JXTA_OBJECT_RELEASE(sh_obj);
                JXTA_OBJECT_RELEASE(env_type);
                JXTA_OBJECT_RELEASE(name);
                JXTA_OBJECT_RELEASE(padv);
            }
            JXTA_OBJECT_RELEASE(res_vec);
        } else {
            printf("No advertisements retrieved\n");
        }
    }
    JXTA_OBJECT_RELEASE(pid);
    JXTA_OBJECT_RELEASE(attr);
    JXTA_OBJECT_RELEASE(value);
    JXTA_OBJECT_RELEASE(query);
    JXTA_OBJECT_RELEASE(infile);
    JXTA_OBJECT_RELEASE(discovery);
    JxtaShellApplication_terminate(app);
}


void jxta_search_print_help(Jxta_object * appl)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("search - discover advertisements \n\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "    search [-p peerid name attribute]\n");
    jstring_append_2(inputLine, "           [-n n] limit the number of responses to n from a single peer\n");
    jstring_append_2(inputLine, "           [-r] discovers jxta advertisements using remote propagation\n");
    jstring_append_2(inputLine, "           [-a] specify Attribute name to limit discovery to\n");
    jstring_append_2(inputLine, "           [-v] specify Attribute value to limit discovery to. wild card is allowed\n");
    jstring_append_2(inputLine, "           [-q] specify an XPath query\n");
    jstring_append_2(inputLine, "           [-i] specify a file containing an XPath query\n");
    jstring_append_2(inputLine, "           [-f] flush advertisements\n");
    if (app != 0) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

void search_read_query_file(JString * filename, JString * jquery)
{
    FILE *input;
    char expression[5000];
    int len;
    printf("Opening %s for XPath Query \n", jstring_get_string(filename));
    input = fopen(jstring_get_string(filename), "r");
    if (input == NULL) {
        printf("Cannot open %s for reading\n", jstring_get_string(filename));
        jstring_reset(jquery, NULL);
        return;
    }
    while (fgets(expression, 4500, input) != NULL) {
        len = strlen(expression);
        len--;
        while ((len >= 0) &&
               ((expression[len] == '\n') || (expression[len] == '\t') || (expression[len] == '\r') || (expression[len] == ' ')))
            len--;
        expression[len + 1] = 0;
        if (len >= 0) {
            jstring_reset(jquery, NULL);
            jstring_append_2(jquery, expression);
            break;
        }
    }
    fclose(input);
}
