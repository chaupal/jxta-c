/*
 * Copyright (c) 2004 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: jxta_tutorial1.c,v 1.4 2005/04/04 22:56:59 bondolo Exp $
 */

/*
 *  JXTA-C Tutorial 1: Finding peers
 *
 *  Created by Brent Gulanowski on 11/1/04.
 *
 */


#include <unistd.h>             /* usleep() */
#include <assert.h>             /* assert() */
#include <string.h>

#include <jxta.h>
#include <jxta_types.h>
#include <jxta_peergroup.h>
#include <jxta_rdv_service.h>

#include "jxta_tutorial.h"
#include "jxta_tutorial_shared.h"
#include "jxta_tutorial_args.h"


/* macros and definitions */

#define BUFFER_SIZE 256


/* types */

typedef enum {
    NO_SEARCH_TYPE = -1,
    PEER_SEARCH_TYPE = DISC_PEER,
    GROUP_SEARCH_TYPE = DISC_GROUP,
    OTHER_SEARCH_TYPE = DISC_ADV
} discoverySearchType;


/* file prototypes */

void print_usage(const char *executable);


/* file variables */

Jxta_PG *npg;
Jxta_discovery_service *npgDiscoService;

/* used when initiating a discovery search */
discoverySearchType searchType;
char searchAttribute[BUFFER_SIZE];
char searchValue[BUFFER_SIZE];
int threshold;


Jxta_boolean process_args(int argc, const char *argv[])
{

    arg **args = NULL;
    unsigned i;
    Jxta_boolean wellFormedArguments = TRUE;
    char *executablePath;
    char *executable;

    searchType = NO_SEARCH_TYPE;
    searchAttribute[0] = '\0';
    searchValue[0] = '\0';
    executablePath = (char *) malloc(sizeof(char) * strlen(argv[0]));
    strcpy(executablePath, argv[0]);
    while (NULL != executablePath) {
        executable = strsep(&executablePath, "/");
    }

    /* organize the arguments; there must be at least one, specifying the search type */
    unsigned count = arg_parse_launch_args(argc, argv, &args);
    if (count < 1) {
        print_usage(executable);
        return FALSE;
    }

    /* interpret the arguments */
    for (i = 0; i < count; i++) {
        switch (arg_name(args[i])) {
        case 'p':
            searchType = PEER_SEARCH_TYPE;
            break;
        case 'g':
            searchType = GROUP_SEARCH_TYPE;
            break;
        case 'o':
            searchType = OTHER_SEARCH_TYPE;
            break;
        case 'a':
            /* if '-a' is used, '-v' must also be used, and each must have an operand */
            wellFormedArguments = FALSE;
            if (arg_operand_count(args[i]) > 0) {
                strncpy(searchAttribute, arg_operand_at_index(args[i], 0), BUFFER_SIZE);
                ++i;
                /* -v has to come after -a, not before, not by itself */
                if ('v' == arg_name(args[i]) && arg_operand_count(args[i]) > 0) {
                    strncpy(searchValue, arg_operand_at_index(args[i], 0), BUFFER_SIZE);
                    wellFormedArguments = TRUE;
                }
            }
            break;
        default:
            /* keep it simple */
            wellFormedArguments = FALSE;
            break;
        }
    }
    /* if the arguments were improper, print a message and tell the caller (main) to exit */
    if (NO_SEARCH_TYPE == searchType || FALSE == wellFormedArguments) {
        print_usage(executable);
        return FALSE;
    }

    threshold = 10;

    return TRUE;
}

/* instantiate the netPeerGroup and retrieve the discovery service */
Jxta_boolean init(void)
{
    Jxta_status status;

    jxta_initialize();

    status = jxta_PG_new_netpg(&npg);
    assert(JXTA_SUCCESS == status);

    jxta_PG_get_discovery_service(npg, &npgDiscoService);

    return TRUE;
}

/* release objects and shut down the library */
void cleanup()
{
    JXTA_OBJECT_RELEASE(npgDiscoService);
    JXTA_OBJECT_RELEASE(npg);
    jxta_terminate();
}

int run(void)
{

    Jxta_vector *ads = NULL;
    Jxta_status status;
    unsigned size;
    unsigned i;

    printf("\nChecking for connection...\n");
    if (FALSE == group_wait_for_network(npg)) {
        printf("\nCould not connect to jxta. Exiting.\n");
        return 0;
    }

    /* issue a remote search; results will be saved in local cache */
    discovery_service_get_remote_advertisements(npgDiscoService, NULL, searchType, searchAttribute, searchValue, threshold, NULL);
    /* wait a moment for results */
    printf("\nPlease wait one second...\n");
    sleep(1);

    /* now check the local cache -- found ads will be here */
    /* also, the next time the app is run, more ads will be here */
    status = discovery_service_get_local_advertisements(npgDiscoService, searchType, searchAttribute, searchValue, &ads);
    assert(JXTA_SUCCESS == status);

    if (NULL == ads) {
        printf("No ads were found\n\n");
        return 0;
    }

    /* print out all of the ads -- not pretty, but you get the idea */
    size = jxta_vector_size(ads);
    printf("Found %d ads for your search:\n\n", size);
    for (i = 0; i < size; i++) {

        JString *adString = NULL;
        Jxta_advertisement *ad = NULL;

        jxta_vector_get_object_at(ads, (Jxta_object **) & ad, i);
        jxta_advertisement_get_xml(ad, &adString);

        printf("\n%s", jstring_get_string(adString));
    }
    printf("\n\n");

    return 0;
}


void print_usage(const char *executable)
{
    printf("Usage: %s -p|g|o [-a attribute -v value]\n", executable);
    printf("-p\tsearch for peer advertisements\n");
    printf("-g\tsearch for group advertisements\n");
    printf("-o\tsearch for other advertisements (pipe, route, etc)\n");
}
