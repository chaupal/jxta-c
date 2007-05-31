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
 * $Id: jxta_tutorial_args.c,v 1.3 2005/01/15 03:32:06 brent Exp $
 */

/*
 *  jxta_tutorial_args.c
 *  jxta tutorials
 *
 *  Created by Brent Gulanowski on 11/2/04.
 *  Copyright 2004 Bored Astronaut Software. All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>              /* for isalpha() */
#include <assert.h>             /* for assert() */

#include "jxta_tutorial_args.h"


struct _arg {
    char name;
    unsigned length;
    unsigned opCount;
    char **operands;
};


unsigned arg_parse_launch_args(int argc, const char *argv[], arg *** args)
{

    arg **argList;
    unsigned i, j;
    unsigned switchCount;
    unsigned *groupSizes;
    char ignore;

    /* skip over any arg strings that don't belong to a switch */
    for (i = 0; i < argc; i++) {
        if ('-' == argv[0][0]) {
            break;
        }
        /* move argv forward, skipping ignored arguments permanently; do not decrement argc */
        argv++;
    }
    /* now calculate the smaller number of arguments */
    argc -= i;

    if (argc < 1) {
        return 0;
    }
    assert(argv[0][0] == '-');

    /* each element of groupSizes is the number of args in a group, including the switch */
    groupSizes = (unsigned *) calloc(sizeof(unsigned), argc);

    /* first pass over the remaining set of strings: group them by switched argument */
    switchCount = 0;
    ignore = 0;
    for (i = 0; i < argc; i++) {
        /* may need to define isalpha() if not available on your platform */
        if ('-' == argv[i][0]) {
            if (isalpha(argv[i][1])) {
                ++switchCount;
                ignore = 0;
#ifdef TEST
                if (strlen(argv[i]) > 2) {
                    fprintf(stderr, "`-%c` is the switch\n", *(argv[i] + 1));
                    fprintf(stderr, "\t`%s` is an operand\n", argv[i] + 2);
                } else {
                    fprintf(stderr, "`%s` is the switch\n", argv[i]);
                }
#endif
            } else {
#ifdef TEST
                fprintf(stderr, "`%s` is not a valid switch\n", argv[i]);
#endif
                ignore = 1;
                continue;
            }
        } else {
#ifdef TEST
            fprintf(stderr, "\t`%s` is an operand\n", argv[i]);
#endif
        }
        if (0 == ignore) {
            ++groupSizes[switchCount - 1];
        }
    }

    /* make the arg list */
    argList = (arg **) malloc(sizeof(arg *) * switchCount);

    /*  initialize an arg object for each arg group */
    j = -1;
    for (i = 0; i < argc; i++) {
        /* start new group? always true when i=0 */
        if ('-' == argv[i][0] && isalpha(argv[i][1])) {
            j++;
            argList[j] = (arg *) calloc(sizeof(arg), 1);
            argList[j]->name = argv[i][1];
            if (strlen(argv[i]) > 2) {
                argList[j]->opCount = groupSizes[j];
                /* first operand is first argument, however... */
                argList[j]->operands = (char **) argv + i;
                /* must increment two characters */
                argList[j]->operands[0] += 2;
            } else {
                argList[j]->opCount = groupSizes[j] - 1;
                /* first operand starts at second argument in group; use its address */
                argList[j]->operands = (char **) &(argv[i + 1]);
            }
        }
    }

    *args = argList;

    return switchCount;
}

extern void arg_print_description(FILE * stream, arg * self)
{
    unsigned i;
    unsigned count = self->opCount;

    fprintf(stream, "Arg structure (%p); name: %c, %d operands:\n", self, self->name, count);
    for (i = 0; i < count; i++) {
        fprintf(stream, "\t%s\n", arg_operand_at_index(self, i));
    }
}

extern char arg_name(arg * self)
{
    return self->name;
}

extern unsigned arg_operand_count(arg * self)
{
    return self->opCount;
}

extern const char *arg_operand_at_index(arg * self, unsigned index)
{
    if (index >= self->opCount) {
        return NULL;
    }
    return *((self->operands) + index);
}


#ifdef TEST
void print_launch_args(int argc, const char *argv[]);
void test_parse_launch_args(void);


int main(int argc, const char *argv[])
{

    if (argc > 1) {
        arg **argList;
        unsigned count;
        unsigned i;

        print_launch_args(argc, argv);
        count = arg_parse_launch_args(argc, argv, &argList);
        for (i = 0; i < count; i++) {
            arg_print_description(stdout, argList[i]);
        }
    }
    test_parse_launch_args();

    return 0;
}

void print_launch_args(int argc, const char *argv[])
{
    int i;
    printf("Provided arguments to test:\n");
    for (i = 0; i < argc; i++) {
        printf("\t%s (%lu)\n", argv[i], strlen(argv[i]));
    }
}

void test_parse_launch_args(void)
{

/*    char *arg1 = "";*/
    char *arg2 = "a";
    char *arg3 = "aa";
    char *arg5 = "-";
    char *arg6 = "-a";
    char *arg7 = "--";
    char *arg8 = "-aa";
    char *arg9 = "'a a'";
/*    char *arg10 = "-a' '";*/

    arg **argList;
    unsigned count;
    unsigned i;

    const char *args1[] = {
        arg2, arg3, arg5
    };
    const char *args2[] = {
        arg6, arg7, arg8, arg9
    };

    print_launch_args(3, args1);
    count = arg_parse_launch_args(3, args1, &argList);
    for (i = 0; i < count; i++) {
        arg_print_description(stdout, argList[i]);
    }

    print_launch_args(4, args2);
    count = arg_parse_launch_args(4, args2, &argList);
    for (i = 0; i < count; i++) {
        arg_print_description(stdout, argList[i]);
    }
}
#endif
