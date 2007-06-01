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
 * $Id: jxta_object.h,v 1.15 2005/11/26 07:52:44 mmx2005 Exp $
 */


#ifndef __JXTA_OBJECT_H__
#define  __JXTA_OBJECT_H__

#include <limits.h>
#include <stddef.h>

#include "jxta_types.h"

#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif

/**
 **     OBJECT MANAGEMENT
 **
 ** This include file defines a set of macros that are to be used when
 ** a component shares an object with another.
 **
 ** The mechanism is simple: each object is prefixed with a handle (an instance
 ** of _jxta_object). This handle contains a reference count, which is set to 1
 ** when the object is initialized, incremented by one everytime the object is
 ** shared, decrement by one everytime the object is released. When the reference
 ** counts drops to zero, the object is effectively freed.
 **
 ** Usage:
 **
 ** All JXTA objects must be defined with JXTA_OBJECT_HANDLE as their first member.
 **
 ** For instance, the following class is a valid JXTA object:
 ** <pre>
 **     typedef struct {
 **         JXTA_OBJECT_HANDLE;
 **         int field1;
 **         int field2;
 **     } TestObject;
 **
 ** </pre>
 ** Failure to comply to that rule may cause random problems, and would be considered
 ** as a bug is the software implementing a JXTA object.
 **
 ** Memory allocation of the objects is let to the application. Note that
 ** <pre> sizeof (object-type)</pre> will return the real size of the object, including
 ** the handle.
 **
 ** Once the object is allocated, it must be initialized before being used. This
 ** is done by:
 ** <pre>
 **
 ** JXTA_OBJECT_INIT (&obj, free_dunction, free_dunction_cookie);
 **
 ** </pre>
 ** A pointer to the free function (of type JXTA_OBJECT_FREE_FUNC) must be provided
 ** in order to actually free the data. This free function must be able to free what
 ** as originally allocated the memory for the storage of the object. A cookie (opaque)
 ** can optionally be passed, so the free function can retrieve it). That can be useful
 ** if the free function needs more than the pointer to the object in order to free it.
 **
 ** Once the object has been initialized, it will have to be released after being used.
 ** Release is done by:
 ** <pre>
 **
 ** JXTA_OBJECT_RELEASE (&obj);
 **
 ** </pre>
 ** This macro will decrease the reference count and invoke the free function if the reference
 ** counts drops to zero.
 **
 ** An initialized object can be shared (given to a different compoment):
 ** <pre>
 **
 ** JXTA_OBJECT_SHARE (&obj);
 **
 ** </pre>
 ** This will increment the reference count of the object. A shared object must be
 ** released.
 **
 ** The number of release of an object must be equal to the number of share + 1 (init does
 ** initialize the reference count to one.
 **
 ** If <pre>JXTA_OBJECT_CHECK_INITIALIZE</pre> is define, INIT, SHARE and RELEASE do
 ** check if the object is initialized (or not) as it is supposed to be. In that purpose
 ** a magic number 0xDEADBEEF is set into the handle when initialized.
 **
 ** Note that when an optional feature is not enabled, it does not take any memory,
 ** nor CPU (it's all done by macros which are empty when not enabled).
 **
 ** Sample code:
 **
 ** Module A.c:
 ** 
 ** <pre>
 **
 ** #include "jxta_object.h"
 **
 **
 ** // This declaration should really be in a common include file between
 ** // module A and B.
 ** typedef struct {
 ** JXTA_OBJECT_HANDLE;
 ** int a;
 ** int b;
 ** } my_type;
 **       )
 **
 ** void
 ** myFree (void* obj) {
 **   printf ("FREE %x\n",(unsigned long) obj);
 **   free (obj);
 ** }
 **
 ** main () {
 **
 **   my_type* obj;
 **   obj = malloc (sizeof (my_type));
 **
 **   JXTA_OBJECT_INIT(v, myFree, 0);
 **
 **   // Give object to module B
 **   JXTA_OBJECT_SHARE(&v);
 **   b-function (&v);
 **
 **   // This module does not need the object anymore.
 **   JXTA_OBJECT_RELEASE(&v);
 ** }
 ** </pre>
 ** On B.c:
 ** <pre>
 ** void
 ** b-function (my_type* obj) {
 **
 **     // Do whatever
 **     // When the object is no longer in use by B, do:
 **     JXTA_OBJECT_RELEASE (obj);
 ** }
 **/

/**
 * This is the beginning of the public API of the object
 **/
typedef struct _jxta_object Jxta_object;

extern Jxta_status jxta_object_initialize(void);
extern void jxta_object_terminate(void);

/**
 * Prototype of the free function provided by the application. 
 * This function is invoked by JXTA_OBJECT_RELEASE when the object
 * is to be freed (not used by anyone anymore).
 * JXTA_OBJECT_GET_FREE_COOKIE() can be used in order to recover the
 * cookie that has been passed during the initialization of the object,
 * such as the pool where the object belongs, for instance.
 *
 * @param data is a pointer to the object.
 **/

typedef void (*JXTA_OBJECT_FREE_FUNC) (Jxta_object * data);

/**
 * Initialize an object.
 * All objects must be initialized before being used and shared.
 * All initialized objects are considered as being shared once. In other
 * word, JXTA_OBJECT_RELEASE must be invoked in order to actually free an
 * initialized object.
 *
 * For static initialization of JXTA objects, you can instead use the macro
 * JXTA_OBJECT_STATIC_INIT as follows:
 *
 * static Jxta_object_derivate my_obj = {
 *   JXTA_OBJECT_STATIC_INIT,
 *   "other fields initializers"
 * };
 *
 * @param obj pointer to the object to initialize.
 * @param pf_free pointer to the free function of the object.
 * @param cookie is void* that is not interpreted by the object management,
 *        but can be used within the free function. That can be a pointer to 
 *        a pool, or whatever, or even be null.
 * @return the created object. (same value as x)
 * @note this macros does not need to be thread safe, because it would be
 * a bug in the calling code if a same object is being used before being
 * initialized.
 **/
JXTA_DECLARE(void *) JXTA_OBJECT_INIT(void *obj, JXTA_OBJECT_FREE_FUNC pf_free, void *cookie);

/**
 * Initialize an object.
 * All objects must be initialized before being used and shared.
 * All initialized objects are considered as being shared once. In other
 * words, JXTA_OBJECT_RELEASE must be invoked in order to actually free an
 * initialized object.
 *
 * For static initialization of JXTA objects, you can instead use the macro
 * JXTA_OBJECT_STATIC_INIT as follows:
 *
 * static Jxta_object_derivate my_obj = {
 *   JXTA_OBJECT_STATIC_INIT,
 *   "other fields initializers"
 * };
 *
 * @param obj pointer to the object to initialize.
 * @param flags Flags that will be set for the object.
 * @param pf_free pointer to the free function of the object.
 * @param cookie is void* that is not interpreted by the object management,
 *        but can be used within the free function. That can be a pointer to 
 *        a pool, or whatever, or even be null.
 * @return the created object. (same value as x)
 * @note this macros does not need to be thread safe, because it would be
 * a bug in the calling code if a same object is being used before being
 * initialized.
 **/
JXTA_DECLARE(void *) JXTA_OBJECT_INIT_FLAGS(void *obj, unsigned int flags, JXTA_OBJECT_FREE_FUNC pf_free, void *cookie);

/**
 * Share an object. 
 * When an object is shared, it must be released by JXTA_OBJECT_RELEASE
 *
 * @param obj pointer to the object to share.
 * @return the shared object. Same value as obj if OBJ is a valid object
 *  otherwise NULL)
 **/
JXTA_DECLARE(void *) JXTA_OBJECT_SHARE(Jxta_object * obj);

/**
 * Release an object
 *
 * When an object has been initialized, and/or, shared, it must be released.
 *
 * @param obj pointer to the object to release.
 * @return The current refcount. A signed number to allow for easier
 * detection of underflow.
 **/
JXTA_DECLARE(int) JXTA_OBJECT_RELEASE(void *obj);

/**
 * Check an object
 * Perform checks on an object to determine if it is a valid object.
 * 
 * @param obj object to check
 * @return  TRUE if the object is valid, otherwise FALSE.
 **/
JXTA_DECLARE(Jxta_boolean) JXTA_OBJECT_CHECK_VALID(Jxta_object * obj);

/**
 * Get the current refcount for an object. A signed number to allow for easier
 * detection of underflow.
 * 
 * @param obj object to check
 * @return  The reference count
 **/
JXTA_DECLARE(int) JXTA_OBJECT_GET_REFCOUNT(Jxta_object * obj);

/*
 * Jxta_object standard functions. Modules dealing with objects should
 * use these prototypes.
 */

/**
 * Compares two objects of the same type for equality and returns
 * TRUE if they are equal or false if they are not. Definition of
 * equality for any particular object type is up to the function doing
 * the evaluation, it may not be the same as binary equivalent.
 *
 * @param obj1 first object to compare.
 * @param obj2 second object to compare.
 * @return TRUE if equivalent or FALSE if not.
 **/
typedef Jxta_boolean(JXTA_STDCALL * Jxta_object_equals_func) (Jxta_object const *obj1, Jxta_object const *obj2);

/**
 * Compares two objects of the same type and returns an int which
 * describes their natural ordering relationship. Not all object types 
 * have an ordering relationship.
 *
 * -1 for obj1 < obj2
 *  0 for obj1 == obj2
 *  1 for obj1 > obj2
 * 
 * @param obj1 first object to compare.
 * @param obj2 second object to compare.
 * @return -1 if obj1 < obj2, 0 for obj1 == obj2, 1 for obj1 > obj2
 **/
typedef int (JXTA_STDCALL * Jxta_object_compare_func) (Jxta_object const *obj1, Jxta_object const *obj2);

/**
 * Calculates a hash value for the provided object. The value should be
 * reasonably non-colliding over the range of unsigned int and have the
 * property that if obj1 and obj2 are equivalent via an equals function
 * then they should have the same hash value.
 * 
 * @param obj1 the object who's hash value is desired.
 * @return a hash value.
 **/
typedef unsigned int (JXTA_STDCALL * Jxta_object_hash_func) (Jxta_object * obj);

/**
 *   Private implementation of Jxta_object is found in this file. Applications
 *   should not need to use depend upon the declarations found in this file.
 **/
#include "jxta_object_priv.h"

#define JXTA_OBJECT_HANDLE const Jxta_object _jxta_obj
#define JXTA_OBJECT_PPTR(x) (Jxta_object**)(void*)(x)

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif /* __JXTA_OBJECT_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
