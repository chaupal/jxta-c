#include <stdio.h>

#include <jpr/jpr_excep.h>
#include <jxta.h>

#include "unittest_jxta_func.h"

/*
 * Declaring a function that throws
 */
void f(int someParam, Throws)
{
    /* Throw() is allowed here */
    if (someParam == 0)
        Throw(5);
}

/*
 * Invoking a function that throws
 */

int g(void)
{
    Try {
        f(0, MayThrow);
    } Catch {
        printf("Caught error number %d\n", jpr_lasterror_get());
    }
    return 0;
}

/*
 * Catching and throwing inline is ok too.
 */

void h(void)
{
    Try {
        Throw(3);
    } Catch {
        printf("Caught error %d\n", jpr_lasterror_get());
    }
}

/*
 * The following will refuse to compile because the function is
 * declared that it throws. The error will be that there is no
 * symbol __jpr_throwIsPermitted__ defined in the current context.
 */
#ifdef CHECK_ERROR_I
int i()
{
    Throw(1);
}
#endif
/*
 * The following will refuse to compile because it calls a function that
 * throws but does not catch. Whether the function declares that it throws
 * or not is not considered. This is to prevent accidental jumping
 * out of a function without cleanup.
 * The error will be that there is no symbol __jpr_invokeesMayThrow__
 * defined in the current context.
 */
#ifdef CHECK_ERROR_J
int j(void)
{
    f(0, MayThrow);
}
#endif

/*
 * The following will refuse to compile because the programmer forgot thst
 * he was calling a function that throws. The error will be that there
 * are not enough arguments to the call to f().
 */
#ifdef CHECK_ERROR_K
int k(void)
{
    f(0);
}
#endif

/*
 * The following will righfully refuse to compile because the
 * function catches and calls a function that throws, but
 * it also calls a function that throws from outside of any try block.
 */
#ifdef CHECK_ERROR_L
int l(void)
{
    Try {
        f(0, MayThrow);
    } Catch {
        printf("Caught error %d\n", jpr_lasterror_get());
    }
    f(1, MayThrow);
}
#endif

/*
 * It is OK to return from within a Try block.
 */
int m(int n)
{
    Try {
        f(n, MayThrow);
        /* It did not throw after all */
        return 0;
    }
    Catch {
    }
    return 1;
}

/*
 * All of the following should be OK too.
 */
int example(int n, Throws)
{
    Try {
        h();
        Try {
            f(n, MayThrow);
            /* did not throw after all */
            return 0;
        }
        Catch {
            printf("Caught error %d\n", jpr_lasterror_get());
            Throw(1);
        }
    }
    Catch {
        printf("Caught error 1, rethrowing.\n");
        Throw(10);
    }
}

int checkReturnFromTry(void)
{
    Try {
        example(1, MayThrow);   /* will not throw but return from nested Try blocks */
        Throw(100);
    } Catch {
        printf("My Own Try block was not damaged. (good)\n");
    }
    return 0;
}

/*
 * Just to make sure the redefinition of "return" does not harm.
 */
int checkPlainReturn(void)
{
    return 0;
}

const char* test_throw_internal()
{
    g();
    h();
    if (m(1) != 0 || m(0) != 1) {
        return FILEANDLINE "Test EXAMPLE failed\n";
    } 
    
    return NULL;
}
        
const char* test_returnvalue()
{
    Try {
        if (example(1, MayThrow) != 0) {
            return FILEANDLINE "Test EXAMPLE failed\n";
        } else {
            return NULL;
        }
    }
    Catch {
        return FILEANDLINE "Test EXAMPLE failed\n";
    }
}
        
const char* test_lasterror()
{
    Try {
        example(0, MayThrow);
        return FILEANDLINE "Test EXAMPLE failed\n";
    }
    Catch {
        if (jpr_lasterror_get() != 10) {
            return FILEANDLINE "Test EXAMPLE failed\n";
        } else {
            return NULL;
        }
    }
}
        
const char* test_throw_plainreturn()
{
    checkReturnFromTry();
    if (checkPlainReturn() != 0) {
        return FILEANDLINE "Test PLAINRETURN failed\n";
    } else {
        return NULL;
    }
}

static struct _funcs excep_test_funcs[] = {
    /* constructor tests */
    {*test_throw_internal, "test_throw_internal"},
    {*test_returnvalue, "test_returnvalue"},
    {*test_lasterror, "test_lasterror"},
    {*test_throw_plainreturn, "test_throw_plainreturn"},

    {NULL, "null"}
};

/**
* Run the unit tests for the lease_msg test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_excep_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(excep_test_funcs, tests_run, tests_passed, tests_failed);
}

#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(excep_test_funcs, argc, argv);
}
#endif
