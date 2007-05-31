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
 * $Id: jxtaobject_test.c,v 1.18 2005/04/07 22:58:54 slowhog Exp $
 */

#include "jxta.h"


/****************************************************************
 **
 ** This test program tests the JXTA object sharing mechanism
 **
 ****************************************************************/

/**
 * Declaring the test object
 **/

typedef struct {
    JXTA_OBJECT_HANDLE;
    int field1;
    int field2;
} TestObject;


Jxta_boolean moduleA(void);
void moduleB(TestObject * obj);

void *allocatedObject;
void *allocatedCookie;
Jxta_boolean freePassed = FALSE;

#define VALUE1 0x12345678
#define VALUE2 0x87654321

/**
 * This is our customized free function. Just display
 * a message when the object is freed (and free the object, of course).
 * The cookie is only used here for testing purpose.
 **/

void testFree(void *obj)
{
    void *cookie = JXTA_OBJECT_GET_FREECOOKIE(obj);
    if (obj != allocatedObject) {
        printf("free: invalid object address\n");
        return;
    }
    if (cookie != allocatedCookie) {
        printf("free: invalid cookie address\n");
        return;
    }
    free(obj);
    freePassed = TRUE;
}

/**
 * This function emulates module A:
 * Creates an object, give it to module B.
 **/
Jxta_boolean moduleA()
{
    /* First create an instance of the object */
    TestObject *obj;
    obj = (TestObject *) malloc(sizeof(TestObject));
    allocatedObject = (void *) obj;
    allocatedCookie = (void *) "This is a test cookie";

    /* Initialize it */
    JXTA_OBJECT_INIT(obj, testFree, allocatedCookie);

    /* Set values in the object */
    obj->field1 = VALUE1;
    obj->field2 = VALUE2;
    /* Sharing it, and give it to B */
    JXTA_OBJECT_SHARE(obj);
    moduleB(obj);
    /* Release the object. Since moduleB is going to release the object
     * before coming back from the call, this release should trigger the
     * actual free. */
    JXTA_OBJECT_RELEASE(obj);
    if (freePassed) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Module B function. Just get the object and release it.
 */
void moduleB(TestObject * obj)
{

    /*
     * First test if the object is corrupted.
     */
    if ((obj->field1 != VALUE1) || (obj->field2 != VALUE2)) {
        /* Data has been corrupted */
        printf("Object data section has been corrupted.\n");
    }

    /* Release the object. Since we know that the object is shared
     * by module A, and since we know that module A has not yet released
     * it, this release should not actually free the object. */
    JXTA_OBJECT_RELEASE(obj);
}

Jxta_boolean checkHandle(TestObject * obj, int targetRefCount, void *freeCookie)
{

    Jxta_object *pt = (Jxta_object *) obj;
    if (pt->_refCount != targetRefCount) {
        printf("bad reference count (%d != %d) \n", pt->_refCount, targetRefCount);
        return FALSE;
    }
    if (pt->_free != (JXTA_OBJECT_FREE_FUNC) testFree) {
        printf("bad free function\n");
        return FALSE;
    }
    if (pt->_freeCookie != (void *) freeCookie) {
        printf("bad free cookie\n");
        return FALSE;
    }
    return TRUE;
}

Jxta_boolean jxtaobject_test(int *tests_run, int *tests_passed, int *tests_failed)
{

    TestObject *obj = (TestObject *) malloc(sizeof(TestObject));
    Jxta_object *pt = (Jxta_object *) obj;
    void *ptr;
    Jxta_boolean passed = TRUE;
    char *myCookie = (char *) "ThisIsASampleCookie";

    obj->field1 = VALUE1;
    obj->field2 = VALUE2;


    if (pt != JXTA_OBJECT(obj)) {
        printf("JXTA_OBJECT failed\n");
        return FALSE;
    }


  /**
   * First test jxtaobject individual functions.
   **/
    printf("JXTA_OBJECT_INIT: ");
    *tests_run += 1;
    JXTA_OBJECT_INIT(obj, testFree, (void *) myCookie);
    if (checkHandle(obj, 1, (void *) myCookie)) {
        printf("passed\n");
        *tests_passed += 1;
    } else {
        printf("failed\n");
        *tests_failed += 1;
        passed = FALSE;
    }


    printf("JXTA_OBJECT_SHARE: ");
    *tests_run += 1;
    JXTA_OBJECT_SHARE(obj);
    if (checkHandle(obj, 2, (void *) myCookie)) {
        *tests_passed += 1;
        printf("passed\n");
    } else {
        printf("failed\n");
        passed = FALSE;
        *tests_failed += 1;

    }


    printf("JXTA_OBJECT_RELEASE: ");
    *tests_run += 1;
    JXTA_OBJECT_RELEASE(obj);
    if (checkHandle(obj, 1, (void *) myCookie)) {
        *tests_passed += 1;
        printf("passed\n");
    } else {
        printf("failed\n");
        passed = FALSE;
        *tests_failed += 1;
    }


    printf("Data corruption: ");
    *tests_run += 1;
    if ((obj->field1 == VALUE1) && (obj->field2 == VALUE2)) {
        printf("passed\n");
        *tests_passed += 1;
    } else {
        printf("failed\n");
        passed = FALSE;
        *tests_failed += 1;
    }


    /* Individual test are completed. Now test a typical scenario */
    printf("Sample test program: ");
    *tests_run += 1;
    if (moduleA()) {
        printf("passed\n");
        *tests_passed += 1;
    } else {
        printf("failed\n");
        passed = FALSE;
        *tests_failed += 1;
    }
    return passed;
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    int run, failed, passed;
    Jxta_boolean result;

    run = failed = passed = 0;
    jxta_initialize();
    result = jxtaobject_test(&run, &passed, &failed);

    fprintf(stdout, "Tests run:    %d\n", run);
    fprintf(stdout, "Tests passed: %d\n", passed);
    fprintf(stdout, "Tests failed: %d\n", failed);
    if (result == FALSE) {
        fprintf(stdout, "Some tests failed\n");
    }

    jxta_terminate();
    return (int) failed;
}
#endif
