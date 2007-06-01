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
 * $Id: jpr_excep.c,v 1.8 2006/02/15 01:09:53 slowhog Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include <apr_thread_proc.h>

#include "jpr_excep.h"
#include "jpr_priv.h"

/* we do not want to start chasing our tail. */
#undef return

/*
 * The thread private data key for the jmp frame ptr.
 */
static apr_threadkey_t *jmp_key_p = NULL;

/*
 * Thread private data key for the status.
 */
static apr_threadkey_t *status_key_p = NULL;

/*
 * There is a global variable _withinFuncTryBlk_ that's set to 0.
 * When we initialize one to 1 in the try block, we hide the global one, 
 * thus identifying the nested Try blocks in the function.
 * This permits to pop all the blocks of the function upon
 * return; Also, if __withinFuncTryBlk__ is 0 upon return, no attempt
 * is made at poping any try block, thus protecting the ones created by
 * invoking code.
 */
int _jpr_withinFuncTryBlk_ = 0;

/*
 * Destructor for jmp Frames and status keys.
 * make sure to do nothing, jmp frames are alloced on the stack and status is
 * not a pointer.
 */
static void destructor(void *ptd)
{
}

/*
 * The default exception handler.
 */
static void default_handler(int n)
{
    fprintf(stderr, "Unhandled exception %d. Exiting now.\n", n);
    abort();
}

apr_status_t jpr_excep_initialize(void)
{
    apr_status_t rv;

    rv = apr_threadkey_private_create(&jmp_key_p, destructor, _jpr_global_pool);
    if (APR_SUCCESS == rv) {
        rv = apr_threadkey_private_create(&status_key_p, destructor, _jpr_global_pool);
    }

    if (APR_SUCCESS != rv) {
        status_key_p = jmp_key_p = NULL;
    }

    return rv;
}

void jpr_excep_terminate(void)
{
    if (jmp_key_p) {
        apr_threadkey_private_delete(jmp_key_p);
        jmp_key_p = NULL;
    }

    if (status_key_p) {
        apr_threadkey_private_delete(status_key_p);
        status_key_p = NULL;
    }
}

/*
 * Saves the context of the current thread in a frame on the stack
 * and links this context with the previous one.
 * nestedInFunc tells if this is the outmost context of a function. This
 * flag also goes in the context.
 * This routine behaves like setjmp: the natural return returns 0, the
 * jumped one returns non-zero.
 */
JPR_DECLARE(void) _jpr_threadPushCtx(Jpr_JmpFrame * frame, int nested_in_func)
{
    apr_threadkey_private_get((void **) &(frame->prev_frame), jmp_key_p);
    frame->nested_in_func = nested_in_func;
    apr_threadkey_private_set(frame, jmp_key_p);
}

/*
 * Restores the innermost saved context.
 * This causes threadSaveCtx to return a second time with the result "1", and
 * the last error (not errno but similar) set to val.
 * If there is no saved context, a default exception handler is called.
 * The new saved context is the next innerMost one if any.
 * The Frame that must be restored is returned so that the caller can
 * siglongjmp into it.
 */
JPR_DECLARE(Jpr_JmpFrame *) _jpr_threadRestoreCtx(int val)
{
    Jpr_JmpFrame *curframe_p;
    apr_threadkey_private_get((void **) &curframe_p, jmp_key_p);
    if (curframe_p == 0)
        default_handler(val);
    apr_threadkey_private_set(curframe_p->prev_frame, jmp_key_p);
    apr_threadkey_private_set((void *) val, status_key_p);
    return curframe_p;
}

/*
 * Pops the inner most saved context out of the chain of saved constexts
 * without restoring it. The new saved context is the next innerMost one if
 * any.
 */
JPR_DECLARE(void) _jpr_threadPopCtx(void)
{
    Jpr_JmpFrame *curframe_p;
    apr_threadkey_private_get((void **) &curframe_p, jmp_key_p);
    if (curframe_p == 0)
        return;
    apr_threadkey_private_set(curframe_p->prev_frame, jmp_key_p);
}

/*
 * Pops all the saved contexts for the current function without restoring
 * any. The new saved context is the inner most saved context in the chain
 * that was not saved by the same function than the current one.
 * Always return 1. This is too keep the "return" macro syntacticaly
 * transparent.
 */
JPR_DECLARE(int) _jpr_threadPopAllFuncCtx(void)
{
    Jpr_JmpFrame *curframe_p;
    apr_threadkey_private_get((void **) &curframe_p, jmp_key_p);
    while (curframe_p->nested_in_func)
        curframe_p = curframe_p->prev_frame;
    apr_threadkey_private_set(curframe_p->prev_frame, jmp_key_p);
    return 1;
}

/*
 * jpr_lasterror_get()/set() manipulate an apr_status_t that's the number of
 * the last error that was thrown. That value can be obtained any time but is
 * guaranteed current within the catch block handling the error. It is similar
 * to errno but simpler because we do not try to pretend it is a plain
 * variable, and portable since errno may or may not exist and is actually
 * hidden by APR. 
 */
JPR_DECLARE(Jpr_status) jpr_lasterror_get(void)
{
    Jpr_status status;
    apr_threadkey_private_get((void **) &status, status_key_p);
    return status;
}

/* vim: set ts=4 sw=4 tw=130 et: */
