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
 * $Id: whoami.c,v 1.5 2005/11/15 18:41:31 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "jxta_membership_service.h"
#include "whoami.h"

#include "jxta_shell_getopt.h"

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_whoami_new(Jxta_PG * pg,
                                      Jxta_listener * standout,
                                      JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_whoami_print_help,
                                      jxta_whoami_start, (shell_application_stdin) jxta_whoami_process_input);
    group = pg;
    environment = env;
    return app;
}


void jxta_whoami_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void jxta_whoami_start(Jxta_object * appl, int argv, char **arg)
{

    Jxta_status res;
    JxtaShellGetopt *opt = JxtaShellGetopt_new(argv, arg, "gcph");
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellObject *object = NULL;
    JString *outputLine = jstring_new_0();
    Jxta_PA *myAdv = NULL;
    Jxta_PGA *myGroupAdv = NULL;
    Jxta_boolean printpretty = FALSE;
    Jxta_boolean printgroup = FALSE;
    Jxta_boolean printcreds = FALSE;
    JString *tmp = NULL;

    while (1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if (type == -1)
            break;
        switch (type) {
        case 'g':
            printgroup = TRUE;
            break;
        case 'c':
            printcreds = TRUE;
            break;
        case 'p':
            printpretty = TRUE;
            break;
        case 'h':
            jxta_whoami_print_help(appl);
            break;
        default:
            jxta_whoami_print_help(appl);
            error = 1;
            break;
        }
        if (error != 0)
            goto Common_Exit;
    }

    JxtaShellGetopt_delete(opt);

    if (!JXTA_OBJECT_CHECK_VALID(group)) {
        jstring_append_2(outputLine, "# ERROR - invalid group object\n");
        goto Common_Exit;
    }

    jxta_PG_get_PGA(group, &myGroupAdv);
    jxta_PG_get_PA(group, &myAdv);

    if (NULL == myAdv) {
        jstring_append_2(outputLine, "# ERROR - Invalid peer advertisement\n");
        goto Common_Exit;
    }

    if (NULL == myGroupAdv) {
        jstring_append_2(outputLine, "# ERROR - Invalid peer group advertisement\n");
        goto Common_Exit;
    }

    /* group adv part */
    if (printpretty && printgroup) {
        Jxta_id *gid = jxta_PGA_get_GID(myGroupAdv);
        Jxta_id *msid = jxta_PGA_get_MSID(myGroupAdv);
        JString *name = jxta_PGA_get_Name(myGroupAdv);
        JString *desc = jxta_PGA_get_Desc(myGroupAdv);

        jstring_append_2(outputLine, "Group : \n");

        jstring_append_2(outputLine, "  Name : ");
        jstring_append_1(outputLine, name);
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(name);
        name = NULL;

        jstring_append_2(outputLine, "  Desc : ");
        jstring_append_1(outputLine, desc);
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(desc);
        desc = NULL;

        jstring_append_2(outputLine, "  GID : ");
        if (JXTA_SUCCESS != jxta_id_to_jstring(gid, &tmp))
            jstring_append_2(outputLine, "<INVALID ID>");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
        }
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(gid);
        gid = NULL;

        jstring_append_2(outputLine, "  MSID : ");
        if (JXTA_SUCCESS != jxta_id_to_jstring(msid, &tmp))
            jstring_append_2(outputLine, "<INVALID ID>");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
        }
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(msid);
        msid = NULL;
    } else if (printgroup) {
        if (JXTA_SUCCESS != jxta_PGA_get_xml(myGroupAdv, &tmp))
            jstring_append_2(outputLine, "# Could not generate peer group advertisement XML");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
            jstring_append_2(outputLine, "\n\n");
        }
    }

    /* peer adv part */
    if (printpretty) {
        Jxta_id *pid = jxta_PA_get_PID(myAdv);
        Jxta_id *gid = jxta_PA_get_GID(myAdv);
        JString *name = jxta_PA_get_Name(myAdv);
        JString *tmp = NULL;

        jstring_append_2(outputLine, "Peer : \n");

        jstring_append_2(outputLine, "  Name : ");
        jstring_append_1(outputLine, name);
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(name);
        name = NULL;

        jstring_append_2(outputLine, "  PID : ");
        if (JXTA_SUCCESS != jxta_id_to_jstring(pid, &tmp))
            jstring_append_2(outputLine, "<INVALID ID>");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
        }
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(pid);
        pid = NULL;

        jstring_append_2(outputLine, "  GID : ");
        if (JXTA_SUCCESS != jxta_id_to_jstring(gid, &tmp))
            jstring_append_2(outputLine, "<INVALID ID>");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
        }
        jstring_append_2(outputLine, "\n");
        JXTA_OBJECT_RELEASE(gid);
        gid = NULL;
    } else {
        if (JXTA_SUCCESS != jxta_PA_get_xml(myAdv, &tmp))
            jstring_append_2(outputLine, "# Could not generate peer advertisement XML");
        else {
            jstring_append_1(outputLine, tmp);
            JXTA_OBJECT_RELEASE(tmp);
            tmp = NULL;
        }
    }

    /* credentials part */
    if (printcreds) {
        Jxta_membership_service *membership = NULL;
        Jxta_vector *creds = NULL;
        int eachCred;

        if (printpretty)
            jstring_append_2(outputLine, "Credentials:\n");

        jxta_PG_get_membership_service(group, &membership);

        if (NULL == membership) {
            jstring_append_2(outputLine, "# invalid membership service\n");
            goto Common_Exit;
        }

        res = jxta_membership_service_get_currentcreds(membership, &creds);

        if ((JXTA_SUCCESS != res) || (NULL == creds)) {
            jstring_append_2(outputLine, "# could not get credentials.\n");
            goto Common_Exit;
        }

        for (eachCred = 0; eachCred < jxta_vector_size(creds); eachCred++) {
            Jxta_credential *aCred = NULL;

            res = jxta_vector_get_object_at(creds, JXTA_OBJECT_PPTR(&aCred), eachCred);

            if ((JXTA_SUCCESS != res) || (NULL == aCred) || !JXTA_OBJECT_CHECK_VALID(aCred)) {
                jstring_append_2(outputLine, "# could not get credential\n");
                continue;
            }

            if (printpretty) {
                char sprintfbuf[256];
                Jxta_id *pid = NULL;
                Jxta_id *gid = NULL;
                JString *tmp = NULL;

                sprintf(sprintfbuf, "%d", eachCred);

                jstring_append_2(outputLine, "Credential #");
                jstring_append_2(outputLine, sprintfbuf);
                jstring_append_2(outputLine, ":\n");

                res = jxta_credential_get_peerid(aCred, &pid);
                jstring_append_2(outputLine, "  PID : ");
                if (JXTA_SUCCESS != jxta_id_to_jstring(pid, &tmp))
                    jstring_append_2(outputLine, "<INVALID ID>");
                else {
                    jstring_append_1(outputLine, tmp);
                    JXTA_OBJECT_RELEASE(tmp);
                    tmp = NULL;
                }
                jstring_append_2(outputLine, "\n");
                JXTA_OBJECT_RELEASE(pid);
                pid = NULL;

                res = jxta_credential_get_peergroupid(aCred, &gid);
                jstring_append_2(outputLine, "  GID : ");
                if (JXTA_SUCCESS != jxta_id_to_jstring(gid, &tmp))
                    jstring_append_2(outputLine, "<INVALID ID>");
                else {
                    jstring_append_1(outputLine, tmp);
                    JXTA_OBJECT_RELEASE(tmp);
                    tmp = NULL;
                }
                jstring_append_2(outputLine, "\n");
                JXTA_OBJECT_RELEASE(gid);
                gid = NULL;

            } else {
                JString *xmldoc = jstring_new_0();

                res = jxta_credential_get_xml_1(aCred, jstring_writefunc_appender, xmldoc);

                if (JXTA_SUCCESS != res) {
                    jstring_append_2(outputLine, "# could not get credential\n");
                } else {
                    jstring_append_1(outputLine, xmldoc);
                }

                JXTA_OBJECT_RELEASE(xmldoc);
                xmldoc = NULL;
            }

            JXTA_OBJECT_RELEASE(aCred);
            aCred = NULL;

        }
    }

  Common_Exit:
    if (jstring_length(outputLine) > 0)
        JxtaShellApplication_print(app, outputLine);
    JXTA_OBJECT_RELEASE(outputLine);

    JxtaShellApplication_terminate(app);
}

void jxta_whoami_print_help(Jxta_object * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("#\twhoami  - Display information about the local peer.\n");
    jstring_append_2(inputLine, "SYNOPSIS\n");
    jstring_append_2(inputLine, "\twhoami [-h] [-p] [-g] [-c]\n\n");
    jstring_append_2(inputLine, "DESCRIPTION\n");
    jstring_append_2(inputLine, "'whoami' displays information about the local peer and optionally the group ");
    jstring_append_2(inputLine, "the peer is a member of. The display can present either the raw XML advertisement or ");
    jstring_append_2(inputLine, "a \"pretty printed\" summary.\n\n");
    jstring_append_2(inputLine, "OPTIONS\n");
    jstring_append_2(inputLine, "\t-c\tinclude credential information.\n\n");
    jstring_append_2(inputLine, "\t-g\tinclude group information.\n\n");
    jstring_append_2(inputLine, "\t-h\tprint this help information.\n\n");
    jstring_append_2(inputLine, "\t-p\t\"pretty print\" the output.\n");

    if (app != NULL) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}
