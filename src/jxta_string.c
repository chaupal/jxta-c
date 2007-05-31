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
 * $Id: jxta_string.c,v 1.3 2002/02/17 18:30:53 lomax Exp $
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "jxta_string.h"

/* An unencumbered version of Glib's g_strconcat.

   Takes: at least one char* followed but a NULL arg terminator.
   Returns: a new char*.
 */
char* jxta_strconcat (const char* s1, ...) {
	va_list argv;
	int n;
	char* s;
	char* news;
	int offset;

	va_start(argv, s1);
	for (n = strlen(s1); (s = va_arg(argv, char*)); n += strlen(s)) {
	}
	va_end(argv);

	va_start(argv, s1);
	news = (char*) malloc(n +1);
	strcpy(news, s1);
	for (offset = strlen(news);
	     (s = va_arg(argv, char*));
	     offset += strlen(s))
		strcpy(news + offset, s);
	va_end(argv);

	return news;
}

#ifdef STANDALONE

#include <stdio.h>

int main (void) {
	char* s;

	s = jxta_strconcat("a", NULL);
	printf("%s\n", s);
	free(s);

	s = jxta_strconcat("b", "c", NULL);
	printf("%s\n", s);
	free(s);

	s = jxta_strconcat("a", "b", "c", NULL, "d");
	printf("%s\n", s);
	free(s);

	return 0;
}

#endif
