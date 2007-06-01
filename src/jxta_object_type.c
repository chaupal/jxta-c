/*
 * Copyright (c) 2002 Sun Microsystems, Inc.  All rights
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
 * $Id: jxta_object_type.c,v 1.11 2006/03/29 21:49:16 slowhog Exp $
 */

static const char *__log_cat = "OBJECT";

#include <string.h>
#include <stdlib.h>

#include "jxta_log.h"
#include "jxta_object_type.h"

JXTA_DECLARE(void *) jxta_assert_same_type(const void *object, const char *object_type, const char *want_type, const char *file,
                                           int line)
{
    if (object == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL, FILEANDLINE "Using NULL Object [%pp] @ [%s:%d] \n", object, file, line);
        abort();
    }

    if (object_type == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL,
                        FILEANDLINE "Using Object [%pp] of unknown (uninitialized?) type @ [%s:%d] \n", object, file, line);
        abort();
    }

    if (want_type == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL,
                        FILEANDLINE "Using Object [%pp] of type <%.30s>, but tested against NULL type @ [%s:%d] \n", object,
                        object_type, file, line);
        abort();
    }

    if (strcmp(object_type, want_type)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_FATAL,
                        FILEANDLINE "Using Object [%pp] of type <%.30s> where <%.30s> was expected @ [%s:%d]\n", object,
                        object_type, want_type, file, line);
        abort();
    }

    return (void *) object;
}
