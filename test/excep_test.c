#include <stdio.h>

#include "jpr/jpr_excep.h"
#include <jxta.h>

/*
 * Declaring a function that throws
 */
void f(int someParam, Throws) {
  /* Throw() is allowed here */
  if (someParam == 0) Throw(5);
}

/*
 * Invoking a function that throws
 */

int g(void) {
  Try {
    f(0, MayThrow);
  } Catch {
    printf("Caught error number %ld\n", jpr_lasterror_get());
  }
  return 0;
}

/*
 * Catching and throwing inline is ok too.
 */

void h(void) {
  Try {
    Throw(3);
  } Catch {
    printf("Caught error %ld\n", jpr_lasterror_get());
  }
}

/*
 * The following will refuse to compile because the function is
 * declared that it throws. The error will be that there is no
 * symbol __jpr_throwIsPermitted__ defined in the current context.
 */
#ifdef CHECK_ERROR_I
int i() {
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
int j(void) {
  f(0, MayThrow);
}
#endif

/*
 * The following will refuse to compile because the programmer forgot thst
 * he was calling a function that throws. The error will be that there
 * are not enough arguments to the call to f().
 */
#ifdef CHECK_ERROR_K
int k(void) {
  f(0);
}
#endif

/*
 * The following will righfully refuse to compile because the
 * function catches and calls a function that throws, but
 * it also calls a function that throws from outside of any try block.
 */
#ifdef CHECK_ERROR_L
int l(void) {
  Try {
    f(0, MayThrow);
  } Catch {
    printf("Caught error %ld\n", jpr_lasterror_get());
  }
  f(1, MayThrow);
}
#endif

/*
 * It is OK to return from within a Try block.
 */
int m(int n) {
  Try {
    f(n, MayThrow);
    /* It did not throw after all */
    return 0;
  } Catch {
  }
  return 1;
}

/*
 * All of the following should be OK too.
 */
int example(int n, Throws) {
  Try {
    h();
    Try {
      f(n, MayThrow);
      /* did not throw after all */
      return 0;
    } Catch {
      printf("Caught error %ld\n", jpr_lasterror_get());
      Throw(1);
    }
  } Catch {
    printf("Caught error 1, rethrowing.\n");
    Throw(10);
  }
}

int checkReturnFromTry(void) {
  Try {
    example(1, MayThrow); /* will not throw but return from nested Try blocks */
    Throw(100);
  } Catch {
    printf("My Own Try block was not damaged. (good)\n");
  }
  return 0;
}

/*
 * Just to make sure the redefinition of "return" does not harm.
 */
int checkPlainReturn(void) {
  return 0;
}

/**
* Run the unit tests for excep_test
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
* 
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_excep_tests(int * tests_run,
			     int * tests_passed,
			     int * tests_failed){
   Jxta_boolean result = TRUE;

   *tests_run += 1;
   g();
   h();
   if (m(1) != 0 || m(0) != 1){
     *tests_failed += 1;
     printf("Test M failed\n");
     result = FALSE;
  }
  else{
     *tests_passed += 1;
  }

  *tests_run += 1;
  Try {
    if (example(1, MayThrow) != 0){
      *tests_failed += 1;
      result = FALSE;
      printf("Test EXAMPLE failed\n");
    }
    else{
       *tests_passed += 1;
    }
  } Catch {
     *tests_failed += 1;
     result = FALSE;
     printf("Test EXAMPLE failed\n");
  }


  *tests_run += 1;
  Try {
    example(0, MayThrow);
    printf("Test EXAMPLE failed\n");
    *tests_failed += 1;
    result = FALSE;
  } Catch {
     if (jpr_lasterror_get() != 10) {
         printf("Test EXAMPLE failed\n");
	 *tests_failed += 1;
	 result = FALSE;
     }
     else{
          *tests_passed += 1;
     }
  }

  *tests_run += 1;
  checkReturnFromTry();
  if (checkPlainReturn() != 0) {
      printf("Test PLAINRETURN failed\n");
      *tests_failed += 1;
      result = FALSE;
  }
  else{
    printf("Everything looks shipshape.\n");
    *tests_passed += 1;
  }

  return result;
}

#ifdef STANDALONE
int main(int argc, char*argv[]) {
  int run,passed,failed;
  int i;
  Jxta_boolean result;

#ifdef WIN32 
    apr_app_initialize(&argc, &argv, NULL);
#else
    apr_initialize();
#endif

    run = failed = passed = 0;
    result = run_excep_tests(&run, &passed, & failed);
    fprintf(stdout,"Tests run:    %d\n",run);  
    fprintf(stdout,"Tests passed: %d\n",passed);  
    fprintf(stdout,"Tests failed: %d\n",failed);
    if( result == FALSE){
       fprintf(stdout,"Some tests failed\n");
    }

    if( result == TRUE) run = 0;
    else                run = -1;
    return run;
}
#endif
