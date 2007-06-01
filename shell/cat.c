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
 * $Id: cat.c,v 1.5 2005/08/24 01:21:19 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "cat.h"

#include "jxta_shell_getopt.h"

static Jxta_PG *group;
static JxtaShellEnvironment *environment;

JxtaShellApplication *jxta_cat_new(Jxta_PG * pg,
                                   Jxta_listener * standout,
                                   JxtaShellEnvironment * env, Jxta_object * parent, shell_application_terminate terminate)
{
    JxtaShellApplication *app = JxtaShellApplication_new(pg, standout, env, parent, terminate);
    if (app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object *) app,
                                      jxta_cat_print_help, jxta_cat_start, (shell_application_stdin) jxta_cat_process_input);
    group = pg;
    environment = env;
    return app;
}


void jxta_cat_process_input(Jxta_object * appl, JString * inputLine)
{
    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellApplication_terminate(app);
}


void jxta_cat_start(Jxta_object * appl, int argv, char **arg)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JxtaShellObject *object = NULL;
    JString *envr = jstring_new_0();
    if (argv < 1) {
        jxta_cat_print_help(appl);
    } else {
        JString *type = NULL;
        JString *xml = NULL;
        Jxta_PA *padv = NULL;
        Jxta_PGA *pgadv = NULL;

        jstring_append_2(envr, arg[0]);
        object = JxtaShellEnvironment_get(environment, envr);
        if (object != NULL) {
            type = JxtaShellObject_type(object);
            if (strncmp("PeerAdvertisement", jstring_get_string(type), strlen("PeerAdvertisement")) == 0 ||
                (strncmp("GroupAdvertisement", jstring_get_string(type), strlen("GroupAdvertisement")) == 0) ||
                (strncmp("Advertisement", jstring_get_string(type), strlen("Advertisement")) == 0)) {
                Jxta_advertisement *adv = (Jxta_advertisement *) JxtaShellObject_object(object);
                jxta_advertisement_get_xml(adv, &xml);
                JxtaShellApplication_print(app, xml);
                JXTA_OBJECT_RELEASE(xml);
                JXTA_OBJECT_RELEASE(adv);
            }
            JXTA_OBJECT_RELEASE(type);
            JXTA_OBJECT_RELEASE(object);
        }
    }
    JXTA_OBJECT_RELEASE(envr);
    JxtaShellApplication_terminate(app);
}

void jxta_cat_print_help(Jxta_object * appl)
{

    JxtaShellApplication *app = (JxtaShellApplication *) appl;
    JString *inputLine = jstring_new_2("     cat  - Concatanate and display a Shell object\n");
    jstring_append_2(inputLine, "SYNOPSIS\n\n");
    jstring_append_2(inputLine, "     cat [-p] <objectName>\n\n");
    jstring_append_2(inputLine, "DESCRIPTION\n\n");
    jstring_append_2(inputLine, "'cat' is the Shell command that displays on stdout the content\n");
    jstring_append_2(inputLine, "of objects stored in environment variables. 'cat' knows\n");
    jstring_append_2(inputLine, "how to display a limited (but growing) set of JXTA objects\n");
    jstring_append_2(inputLine, "Advertisement, Message and StructuredDocument)\n");
    jstring_append_2(inputLine, "If you are not sure, try to cat the object anyway: the\n");
    jstring_append_2(inputLine, "command will let you know if it can or not display that\n");
    jstring_append_2(inputLine, "object.\n\n");
    jstring_append_2(inputLine, "OPTIONS\n\n");
    jstring_append_2(inputLine, "    -p Pretty display\n\n");

    if (app != 0) {
        JxtaShellApplication_print(app, inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}

/* vim: set ts=4 sw=4 tw=130 et: */
