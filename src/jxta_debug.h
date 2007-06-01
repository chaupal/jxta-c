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
 * $Id: jxta_debug.h,v 1.5.4.2 2005/05/20 22:33:59 slowhog Exp $
 */


#ifndef __JXTA_DEBUG_H__
#define __JXTA_DEBUG_H__

#include <stddef.h>
#include <jxta_log.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNUSED__
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__                /* UNSUSED */
#endif
#endif
    /**
     * Change the following line to <tt>#define JXTA_LOG_ENABLE</tt>
     *
     * This will enable debug log for all the platform.
     **/
#ifndef JXTA_LOG_ENABLE
#define JXTA_LOG_ENABLE 1
#endif

    /**
    *   Record the specified message to the JXTA debug log.
    *
    *   @param fmt  The format string for the log event.
    *   @return number of characters written to the log.
    **/
    int JXTA_LOG(const char *fmt, ...);

    /**
    *   Record the specified message to the JXTA debug log, but without
    *   the file or line information
    *
    *   @param fmt  The format string for the log event.
    *   @return number of characters written to the log.
    **/
    int JXTA_LOG_NOLINE(const char *fmt, ...);

#if JXTA_LOG_ENABLE

    /**
    *   Record the specified message to the JXTA debug log, but without
    *   the file or line information
    *
    *   @param fmt  The format string for the log event.
    *   @return number of characters written to the log.
    **/
    int jxta_log(const char *fmt, ...);
    
extern const char * __debug_log_cat;

    /*
     * __VA_ARGS__  is c99.
     */
#if defined(C99) || defined(__GNUC__)

#define JXTA_LOG(...) jxta_log_append( __debug_log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE __VA_ARGS__)
#define JXTA_LOG_NOLINE(...) jxta_log_append( __debug_log_cat, JXTA_LOG_LEVEL_DEBUG, __VA_ARGS__)

#else

#define JXTA_LOG jxta_log
#define JXTA_LOG_NOLINE jxta_log

#endif

#else                           /* JXTA_LOG_ENABLE */
#ifdef C99
#define JXTA_LOG(...) (0)
#define JXTA_LOG_NOLINE(...) (0)
#else

    static int UNUSED__ _jxta_nop(const char *UNUSED__ u, ...) {
        return 0;
    }
#define JXTA_LOG _jxta_nop
#define JXTA_LOG_NOLINE _jxta_nop
#endif                          /* C99 */
#endif                          /* JXTA_LOG_ENABLED */
    /**
     * Display the content of a buffer (wireformat) onto the console.
     * Non printable values are printed into their decimal value.
     * @param buf the buffer to display.
     * @param size the length of the buffer.
     **/ void jxta_buffer_display(const char *buf, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __JXTA_DEBUG_H__  */
