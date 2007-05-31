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
 * $Id: jpr_excep_proto.h,v 1.1 2004/12/05 02:17:27 slowhog Exp $
 */

#ifndef JPR_EXCEP_PROTO_H
#define JPR_EXCEP_PROTO_H

/*
 * This header provides the minimum definitions needed to declare
 * a function that throws.
 * c code that defines such functions or uses them must include
 * jpr_excep.h
 * This separation limits the name space pollution produced by including
 * indirectly header files that declares functions that Throw from files
 * that do not call them and do not otherwise use exceptions.
 */

/*
 * This is the type of all variables used for exception control.
 * The value of these variables is never used, their sole purpose
 * is to use the compiler's prototype and declaration checking to
 * enforce proper use of the exception macros. Using a special type
 * reduces the chance of the system being fooled by programmer's mistakes.
 */

typedef struct _jpr_ExceptionControlT_ {
    int dummy;
} _jpr_ExceptionControlT_;

/*
 * _jpr_throwIsPermitted_ is not defined globaly. It is defined only
 * within a try block and within a function that declares that it throws.
 * Outside of these contexts any call to Throw() will fail to compile.
 * In addition, unlike C++ or java, it is not permitted
 * to throw "through" a function; there must be a Try block around any
 * call to a function that throws even if the call is from a function that
 * thows too. This is so to avoid inadevertantly jumping out of a function
 * without cleaning up. This is enforced by invocations of a function that
 * throws requiring _jpr_invokeesMayThrow_ to be defined. This symbol is
 * defined ONLY within a Try block.
 */

/*
 * Add Throws to the list of arguments of a function declaration and
 * definition so that the code inside it is allowed to throw. In addition
 * This will create an error if MayThrow is not added to the list
 * of arguments at the invocation point.
 */
#define Throws _jpr_ExceptionControlT_  _jpr_throwIsPermitted_

#endif /* JPR_EXCEP_PROTO_H */
