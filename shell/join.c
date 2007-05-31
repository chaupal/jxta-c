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
 * $Id: join.c,v 1.4 2004/12/05 02:16:39 slowhog Exp $
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <jpr/jpr_thread.h>

#include "jxta.h"
#include "jxta_shell_application.h"
#include "jxta_peergroup.h"
#include "jxta_membership_service.h"
#include "join.h"
#include <jxta_rdv.h>

#include "jxta_shell_getopt.h"

static Jxta_PG * group;
static JxtaShellEnvironment * environment;

JxtaShellApplication * jxta_join_new(Jxta_PG * pg,
                                     Jxta_listener* standout,
                                     JxtaShellEnvironment *env,
                                     Jxta_object * parent,
                                     shell_application_terminate terminate) {
    JxtaShellApplication *app  =
        JxtaShellApplication_new(pg,standout,env,parent,terminate);
    if( app == 0)
        return 0;
    JxtaShellApplication_setFunctions(app,
                                      (Jxta_object*)app,
                                      jxta_join_print_help,
                                      jxta_join_start,
                                      (shell_application_stdin)jxta_join_process_input);
    group = pg;
    environment = env;
    return app;
}


void  jxta_join_process_input(Jxta_object * appl,
                              JString * inputLine) {
    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JxtaShellApplication_terminate(app);
}


void jxta_join_start(Jxta_object * appl,
                     int argc,
                     char **argv) {

    Jxta_status res;
    JxtaShellGetopt * opt = JxtaShellGetopt_new(argc,argv,"hd:s:");
    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * pgadvVAR = NULL;
    JString * pgVAR = NULL;
    JString * outputLine = jstring_new_0();
    Jxta_boolean doingJoin = FALSE;

    while(1) {
        int type = JxtaShellGetopt_getNext(opt);
        int error = 0;

        if( type == -1)
            break;
        switch(type) {
        case 'd':
            doingJoin = TRUE;
            pgadvVAR = JxtaShellGetopt_getOptionArgument(opt);
            break;
        case 's':
            doingJoin = TRUE;
            pgVAR = JxtaShellGetopt_getOptionArgument(opt);
            break;
        case 'h':
            jxta_join_print_help(appl);
            break;
        default:
            jxta_join_print_help(appl);
            error = 1;
            break;
        }
        if( error != 0)
            goto Common_Exit;
    }

    JxtaShellGetopt_delete(opt);

    if( !JXTA_OBJECT_CHECK_VALID( group ) ) {
        jstring_append_2 ( outputLine, "# ERROR - invalid group object\n" );
        goto Common_Exit;
    }

    if( doingJoin && (NULL == pgadvVAR) ) {
        jstring_append_2 ( outputLine, "# ERROR - group adv not specified\n" );
        goto Common_Exit;
    }

    if( doingJoin ) {
        JxtaShellObject * object = JxtaShellEnvironment_get ( environment, pgadvVAR );
        Jxta_PG * newPG = NULL;
        Jxta_PGA * pga = NULL;
        Jxta_vector * resources = NULL;
        JString * type = NULL;
        JxtaShellObject *var = NULL;

        if (object == NULL) {
            jstring_append_2 ( outputLine, "# ERROR - group advertisement not found.\n" );
            goto Common_Exit;
        }

        type = JxtaShellObject_type (object);

        if ( 0 != strcmp( jstring_get_string(type), "GroupAdvertisement" ) ) {
            jstring_append_2 ( outputLine, "# ERROR - variable is not a group advertisement.\n" );
            JXTA_OBJECT_RELEASE(type);
            type = NULL;
            goto Common_Exit;
        }
        JXTA_OBJECT_RELEASE(type);
        type = NULL;

        pga = (Jxta_PGA*) JxtaShellObject_object (object);

        /* Reuse the resource groups of our parent. */
        jxta_PG_get_resourcegroups( group, &resources );

        /* add the new group's parent */
        jxta_vector_add_object_last( resources, (Jxta_object*) group );

        res = jxta_PG_newfromadv ( group, (Jxta_advertisement*) pga, resources, &newPG );

        if( res != JXTA_SUCCESS ) {
            jstring_append_2 ( outputLine, "# ERROR - New from adv failed\n" );
            goto Common_Exit;
        }

        if( NULL == pgVAR )
            pgVAR = JxtaShellEnvironment_createName();

        type = jstring_new_2( "Group" );
        var = JxtaShellObject_new( pgVAR, (Jxta_object*) newPG, type);
        JxtaShellEnvironment_add_0(environment,var);
        jstring_append_2(outputLine, "Joined group is available as: " );
        jstring_append_1(outputLine, pgVAR );
        jstring_append_2(outputLine, "\n" );
        JXTA_OBJECT_RELEASE(pgVAR);
        pgVAR = NULL;
        JXTA_OBJECT_RELEASE(type);
        type = NULL;
        JXTA_OBJECT_RELEASE(var); /* release local */

        res = JxtaShellEnvironment_set_current_group ( environment, newPG );

        if ( JXTA_SUCCESS == res ) {
        jstring_append_2(outputLine, "Joined group successfully " );
        }
    } else {
        JString *       grouptype = jstring_new_2( "Group" );
        Jxta_vector*    groups = JxtaShellEnvironment_get_of_type( environment, grouptype );
        int             eachItem;

        jstring_append_2(outputLine, "Groups :\n" );

        for( eachItem = 0; eachItem < jxta_vector_size(groups); eachItem++ ) {
            JxtaShellObject * object = NULL;

            jxta_vector_get_object_at( groups, (Jxta_object**) &object, eachItem );

            if( (NULL != object) ) {
                JString * itsName = JxtaShellObject_name(object);
                Jxta_PG * itsObject = (Jxta_PG*) JxtaShellObject_object(object);
                char      displayBuffer[256];

                jstring_append_2(outputLine, "  " );
                jstring_append_1(outputLine, itsName );
                sprintf( displayBuffer, " [%p] ", itsObject );
                jstring_append_2(outputLine, displayBuffer );
                jstring_append_2(outputLine, "\n" );

                JXTA_OBJECT_RELEASE(itsName);
                itsName = NULL;
                JXTA_OBJECT_RELEASE(itsObject);
                itsObject = NULL;
            }
            JXTA_OBJECT_RELEASE(object);
            object = NULL;
        }

        JXTA_OBJECT_RELEASE(groups);
        groups = NULL;
        JXTA_OBJECT_RELEASE(grouptype);
        grouptype = NULL;
    }


Common_Exit:
    if( jstring_length( outputLine ) > 0 )
        JxtaShellApplication_print(app, outputLine );

    JxtaShellApplication_terminate(app);
}

void jxta_join_print_help(Jxta_object *appl) {

    JxtaShellApplication * app = (JxtaShellApplication*)appl;
    JString * inputLine = jstring_new_2("#\tjoin - Join a peer group or display the list of joined groups.\n");
    jstring_append_2(inputLine,"SYNOPSIS\n");
    jstring_append_2(inputLine,"\tjoin [-h] [-d pgadv] [-s envvar]\n\n");
    jstring_append_2(inputLine,"DESCRIPTION\n");
    jstring_append_2(inputLine,"\t'join' allows you to join a peer group or displays the list of joined " );
    jstring_append_2(inputLine,"peer groups.\n\n" );
    jstring_append_2(inputLine,"OPTIONS\n");
    jstring_append_2(inputLine,"\t-d\tspecify the group to join. \n");
    jstring_append_2(inputLine,"\t-h\tthis help information. \n");
    jstring_append_2(inputLine,"\t-s\tshell variable in which to place the group. \n\n");

    if( app != NULL ) {
        JXTA_OBJECT_SHARE(inputLine);
        JxtaShellApplication_print(app,inputLine);
    }
    JXTA_OBJECT_RELEASE(inputLine);
}
