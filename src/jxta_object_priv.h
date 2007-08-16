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
 * $Id$
 */


#ifndef __JXTA_OBJECTPRIV_H__
#define  __JXTA_OBJECTPRIV_H__

JXTA_DECLARE_DATA int _jxta_return;
#ifdef __cplusplus
extern "C" {
#if 0
};
#endif
#endif
/**
 * Flag for rendering any object immutable. Refcount and ojbect freeing
 * is disabled for immutable objects. USE WITH CARE!
 **/
#define JXTA_OBJECT_IMMUTABLE_FLAG (1U << 1)
/**
 * Flag for indicating whether object is constructed.
 **/
#define JXTA_OBJECT_CONSTRUCTED    (1U << 2)
/**
* Flag for indicating if share/release should be logged for this
* object.
**/
#define JXTA_OBJECT_SHARE_TRACK    (1U << 3)
/**
* Flag for indicating that the object is initialized if we are not
* using JXTA_OBJECT_CHECK_INITIALIZED_ENABLE.
**/
#define JXTA_OBJECT_INITED_BIT    (1U << (sizeof(unsigned int) * CHAR_BIT - 1))
/**
 * This is the configuration of the object. Each of the following
 * can independantely be set or not set.
 *
 * Those defines should probably be into a more generic configuration
 * file. To be fixed. lomax@jxta.org
**/

/**
 * This is used as a marker to see if an object has been initialized
 * or not. Any integer would work, but, in order to limit the risk
 * of a corruption of the data that might look fine, a "random" but yet
 * readable by a human, value is used.
 **/
#define JXTA_OBJECT_MAGIC_NUMBER   (0x600DF00DU)
/**
 * When defined, macros like SHARE and RELEASE will check if the
 * object is properly initialized (or not initialized), currently
 * logging an error message if that not the case (maybe a stronger
 * behavior on failure should happen).
 * Comment out the following line in order to disable that feature.
 */
#define JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
#define JXTA_OBJECT_CHECK_UNINITIALIZED_ENABLE
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
#define JXTA_OBJECT_CHECKINIT_MAYBE (unsigned int) JXTA_OBJECT_MAGIC_NUMBER,
#else
#define JXTA_OBJECT_CHECKINIT_MAYBE
#endif
/**
* When defined, adds a feature to jxta_object which tracks the creation
* point of the object.
*/
#undef JXTA_OBJECT_TRACKER
#ifdef JXTA_OBJECT_TRACKER
#define JXTA_OBJECT_NAME_MAYBE (const char*) NULL,
#else
#define JXTA_OBJECT_NAME_MAYBE
#endif
/**
 * The default behaviour is to NOT check if objects are valid.  
 * When defined, the macros JXTA_OBJECT_CHECK_VALID checks
 * if the object still has a positive reference count.
 */
#ifndef JXTA_OBJECT_CHECKING_ENABLE
#define JXTA_OBJECT_CHECKING_ENABLE 0
#endif

/**
 * This is the structure that is added to the begining of each object.
 **/
struct _jxta_object {
    unsigned int _bitset;
    JXTA_OBJECT_FREE_FUNC _free;
    void *_freeCookie;
#ifdef JXTA_OBJECT_TRACKER
    char *_name;
#endif
#ifdef JXTA_OBJECT_CHECK_INITIALIZED_ENABLE
    unsigned int _initialized;
#endif
    int _refCount;  /* signed int to better detect over-release */
};

typedef struct _jxta_object _jxta_object;

/*
 * Generate a static initializer for _jxta_objects that matches
 * the various options.
 *
 * This initializer makes the object correctly initialized if it
 * is a static object:
 * - the free function does nothing, the cookie is set to 0,
 * - the ref count is set to 1. (It is irrelevant since it probably
 *   will not be managed properly and the object cannot be freed anywyay.)
 * - The magic number is set if required.
 */

extern void jxta_object_freemenot(Jxta_object * obj);

/*
 * The static initializer macro.
 * use as follows :
 *
 * static Jxta_object_derivate my_obj = {
 *   JXTA_OBJECT_STATIC_INIT,
 *   "other fields initializers"
 * };
 */

#define JXTA_OBJECT_STATIC_INIT { \
        (unsigned int) JXTA_OBJECT_IMMUTABLE_FLAG | JXTA_OBJECT_INITED_BIT, \
        (JXTA_OBJECT_FREE_FUNC) jxta_object_freemenot, \
        (void*) NULL, \
        JXTA_OBJECT_NAME_MAYBE \
        JXTA_OBJECT_CHECKINIT_MAYBE \
        (int) 1 \
     }

/**
 * Helper functions
 **/

/**
 * Returns a properly cast pointer to the object handle part
 * of the object.
 *
 * @param x the address of the object
 * @return a pointer to the _jxta_object.
 **/
Jxta_object *JXTA_OBJECT(void *x);

/**
 * Returns a properly cast pointer to the object free function
 * associated with the provided object.
 * 
 * @param x the address of the object
 * @return a pointer to the free function.
**/
JXTA_OBJECT_FREE_FUNC JXTA_OBJECT_GET_FREE_FUNC(Jxta_object * x);

/**
 * Returns a properly cast pointer to the object free function
 * cookie associated with the provided object.
 * 
 * @param x the address of the object.
 * @return a pointer to the free function cookie.
**/
void *JXTA_OBJECT_GET_FREECOOKIE(Jxta_object * x);


/**
 * Helper macros
 **/
#define JXTA_OBJECT(x)                    ((Jxta_object*)(void*)(x))

#define JXTA_OBJECT_GET_FREE_FUNC(x)      (JXTA_OBJECT(x)->_free)

#define JXTA_OBJECT_GET_FREECOOKIE(x)     (JXTA_OBJECT(x)->_freeCookie)

#if JXTA_OBJECT_CHECKING_ENABLE
JXTA_DECLARE(Jxta_boolean) _jxta_object_check_valid(Jxta_object * obj, const char *file, int line);
#define JXTA_OBJECT_CHECK_VALID(obj) _jxta_object_check_valid( JXTA_OBJECT(obj), __FILE__, __LINE__ )
#else
#define JXTA_OBJECT_CHECK_VALID(obj) (_jxta_return = 1)
#endif

JXTA_DECLARE(void *) _jxta_object_init(Jxta_object *, unsigned int flags, JXTA_OBJECT_FREE_FUNC, void *, const char *, int);
JXTA_DECLARE(void *) _jxta_object_share(Jxta_object *, const char *, int);
JXTA_DECLARE(int) _jxta_object_release(Jxta_object *, const char *, int);
JXTA_DECLARE(int) _jxta_object_get_refcount(Jxta_object * obj);

#define JXTA_OBJECT_INIT(obj,free,cookie) _jxta_object_init (JXTA_OBJECT(obj), 0, free, cookie, __FILE__, __LINE__)

#define JXTA_OBJECT_INIT_FLAGS(obj,flags,free,cookie) _jxta_object_init (JXTA_OBJECT(obj), flags, free, cookie, __FILE__, __LINE__)

#define JXTA_OBJECT_SHARE(obj)  _jxta_object_share (JXTA_OBJECT(obj), __FILE__, __LINE__)

#define JXTA_OBJECT_RELEASE(obj) _jxta_object_release (JXTA_OBJECT(obj), __FILE__, __LINE__)

#define JXTA_OBJECT_GET_REFCOUNT(obj) _jxta_object_get_refcount (JXTA_OBJECT(obj))

#ifdef __cplusplus
#if 0
{
#endif
}
#endif


#endif /* __JXTA_OBJECTPRIV_H__ */

/* vi: set ts=4 sw=4 tw=130 et: */
