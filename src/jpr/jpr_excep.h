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
 * $Id: jpr_excep.h,v 1.4 2005/04/28 09:56:31 lankes Exp $
 */

#ifndef JPR_EXCEP_H
#define JPR_EXCEP_H

#include "jpr_setjmp.h"
#include "jpr_types.h"

#include "jpr_excep_proto.h"

#ifdef __cplusplus
extern "C" {
#if 0
}
#endif
#endif /* __cplusplus */
typedef struct Jpr_JmpFrame {
    int nested_in_func;
    struct Jpr_JmpFrame *prev_frame;
    sigjmp_buf tctx;
} Jpr_JmpFrame;

JPR_DECLARE(void) _jpr_threadPushCtx(Jpr_JmpFrame * frame, int nested_in_frame);
JPR_DECLARE(void) _jpr_threadPopCtx(void);
JPR_DECLARE(int) _jpr_threadPopAllFuncCtx(void);
JPR_DECLARE(Jpr_JmpFrame *) _jpr_threadRestoreCtx(int val);

JPR_DECLARE(Jpr_status) jpr_lasterror_get(void);

/*
 * These two macros must NEVER be collapsed into a single one because that
 * would prevent the number parameter from being expanded if it is itself a
 * macro (such as __LINE__).
 */
#define jxta_concat2(name,number) name##number
#define jxta_unique_with_number(n) jxta_concat2(_jxta_unique_,n)

#ifndef UNUSED__
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__        /* UNSUSED */
#endif
#endif


/*
 * There is a global variable _withinFuncTryBlk_ that's set to 0.
 * When we initialize one to 1 in the try block, we hide the global one, 
 * thus identifying the nested Try blocks in the function.
 * This permits to pop all the blocks of the function upon
 * return; Also, if __withinFuncTryBlk__ is 0 upon return, no attempt
 * is made at poping any try block, thus protecting the ones created by
 * invoking code.
 */
JPR_DECLARE_DATA int _jpr_withinFuncTryBlk_;

/*
 * Add MayThrow to the list of arguments when invoking a function that
 * was declared to throw. This will force a check that
 * _jpr_invokeesMayThrow_
 * is defined. If this is forgotten, the compiler will generate a function
 * prototype error.
 */
#define MayThrow _jpr_invokeesMayThrow_

/*
 * The following may be placed explicitly at the begining of a function
 * as a way for the programmer to state that it is OK for an invokee
 * to throw out of that function without it catching for cleanup.
 * In other words it relaxes the condition under which using "MayThrow" is
 * permitted. Unfortunately in order to check that the function duly
 * declares Throws, this macro has to reference _jpr_throwIsPermitted_.
 * since it also declares a variable, it means that there only one place
 * where it is always legal to put it: after the last variable declaration.
 */
#define ThrowThrough() _jpr_ExceptionControlT_ UNUSED__ _jpr_invokeesMayThrow_ = _jpr_throwIsPermitted_;

#define Try { \
    Jpr_JmpFrame curframe; \
    _jpr_threadPushCtx(&curframe, _jpr_withinFuncTryBlk_); \
    if (! sigsetjmp(curframe.tctx, 1)) { \
        _jpr_ExceptionControlT_ UNUSED__ _jpr_throwIsPermitted_; \
        _jpr_ExceptionControlT_ UNUSED__ _jpr_invokeesMayThrow_; \
        int UNUSED__ _jpr_withinFuncTryBlk_ = 1; \

#define Catch _jpr_threadPopCtx(); } else goto jxta_unique_with_number(__LINE__); \
    } \
    while (0) \
        jxta_unique_with_number(__LINE__):


#define Throw(n) siglongjmp( \
    _jpr_threadRestoreCtx(_jpr_throwIsPermitted_.dummy = n)->tctx, \
    1)

/*
 * return is redefined to check whether there are contexts to pop before
 * leaving.
 */
#define return while (   (   _jpr_withinFuncTryBlk_ \
                          && _jpr_threadPopAllFuncCtx()) \
		      || 1) return


#ifdef __cplusplus
}
#endif

#endif /* ! JPR_EXCEP_H */
