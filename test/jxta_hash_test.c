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
 * $Id: jxta_hash_test.c,v 1.12 2005/04/17 14:22:18 lankes Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <apr_general.h>

#include "jxta_hashtable.h"
#include "jxta_errno.h"
#include "jxta_object.h"
#include "jxta.h"

typedef struct {
    JXTA_OBJECT_HANDLE;
    int n;
    int occur;
} Num_val;

Num_val* num_val_new(int n) {
    Num_val* this = (Num_val*) malloc(sizeof(Num_val));
    if (this == 0) {
	printf("Out of memory.\n");
	abort();
    }
    memset(this, 0, sizeof(*this));
    JXTA_OBJECT_INIT((Jxta_object*) this, free, 0);
    this->n = n;
    this->occur = 1;
    return this;
}

/* Get the undocumented debugging tool :-) */
extern Jxta_status JXTA_STDCALL
jxta_hashtable_stupid_lookup(Jxta_hashtable* this, const void* key,
			     size_t key_size, Jxta_object** found_value);


static Jxta_boolean test_num(void) {

    time_t start;
    time_t insertion;
    time_t deletion;
    time_t reinsertion;
    time_t lookup;
    time_t release;
    time_t getvals;

    double avg_hops;
    size_t usage;
    size_t occupancy;
    size_t max_occupancy;
    size_t total_size;
    size_t duplicates = 0;

    int i;
    Jxta_status res;
    Jxta_vector* all_vals;

    Jxta_hashtable* hash_num = jxta_hashtable_new(8);

    srand(1);
    start = time(NULL);
    for (i = 1000000; i--> 0; ) {
	int k = rand();
	Num_val* old_val = 0;
	Num_val* new_val = num_val_new(k);
	jxta_hashtable_replace(hash_num, &k, sizeof(k),
			       (Jxta_object*) new_val,
			       (Jxta_object**) &old_val);
    if (old_val != 0) {
	    if (old_val->n != k) {
		printf("Ooops replaced the wrong value\n");
		return FALSE;
	    }
		new_val->occur = old_val->occur + 1;
	    ++duplicates;
	    JXTA_OBJECT_RELEASE(old_val);
	}

	/* We do not keep a ref. The table does. */
	JXTA_OBJECT_RELEASE(new_val);
    }
    insertion = time(NULL) - start;

    /* remove every other entry */
    srand(1);
    start = time(NULL);
    for (i = 1000000; i--> 0; ) {
	int k = rand();
	if (i%2) {
	    Num_val* rmd;
	    res = jxta_hashtable_del(hash_num, &k, sizeof(k),
				     (Jxta_object**) &rmd);

	    if (res != JXTA_SUCCESS) {
		printf("Ooops %d removal failed: key = %d\n", i, k);
		printf("trying to lookup key\n");
		res = jxta_hashtable_get(hash_num, &k, sizeof(k),
					 (Jxta_object**) &rmd);
		if (res == JXTA_SUCCESS) {
		    JXTA_OBJECT_RELEASE(rmd);
		    printf("Key found: problem specific to removal\n");
		} else {
		    printf("Key not found\n");
		    res = jxta_hashtable_stupid_lookup(hash_num, &k, sizeof(k),
						       (Jxta_object**) &rmd);
		    if (res == JXTA_SUCCESS) {
			if (rmd->n == k) {
			    printf("The key's in the table. get screws up.\n");
			} else {
			    printf("The key's in the table but wrong val.\n");
			}
		    } else {
			printf("The key is not in the table. So lookup is "
			       "fine. Either del or put screws up.\n");
		    }
		}
		return FALSE;
	    }

	    if (rmd->n != k) {
		printf("Ooops removed the wrong object\n");
		return FALSE;
	    }

	    if (--(rmd->occur) != 0) {
		--duplicates;
		jxta_hashtable_put(hash_num, &k, sizeof(k),
				   (Jxta_object*) rmd);
	    }
	    /* Del in both cases. We don't keep a ref, the tbl does. */
	    JXTA_OBJECT_RELEASE(rmd);
	}
    }
    deletion = time(NULL) - start;

    /* insert half a million more (no srand()) */
    start = time(NULL);
    for (i = 500000; i--> 0; ) {
	int k = rand();
	Num_val* old_val = 0;
	Num_val* new_val = num_val_new(k);
	jxta_hashtable_replace(hash_num, &k, sizeof(k),
			       (Jxta_object*) new_val,
			       (Jxta_object**) &old_val);
	if (old_val != 0) {
	    if (old_val->n != k) {
		printf("Ooops replaced the wrong value\n");
		return FALSE;
	    }
		new_val->occur = old_val->occur + 1;
	    ++duplicates;
	    JXTA_OBJECT_RELEASE(old_val);
	}

	/* We do not keep a ref. The table does. */
	JXTA_OBJECT_RELEASE(new_val);
    }
    reinsertion = time(NULL) - start;

    /* now look them all up */
    srand(1);
    start = time(NULL);
    for (i = 1000000; i--> 0; ) {
	int k = rand();
	if (! i%2) { /* Those we did not remove :-) */
	    Num_val* found;
	    res = jxta_hashtable_get(hash_num, &k, sizeof(k),
						 (Jxta_object**) &found);
	    if (res != JXTA_SUCCESS) {
		printf("Ooops lookup failed (phase 1) : %d\n", k);
		    printf("trying stupid lookup\n");
		    res = jxta_hashtable_stupid_lookup(hash_num, &k, sizeof(k),
						       (Jxta_object**) &found);
		    if (res == JXTA_SUCCESS) {
			if (found->n == k) {
			    printf("The key's in the table. get screws up.\n");
			} else {
			    printf("The key's in the table but wrong val.\n");
			}
		    } else {
			printf("The key is not in the table. So lookup is "
			       "fine. Either del or put screws up.\n");
		    }
		return FALSE;
	    }

	    if (found->n != k) {
		printf("Ooops found the wrong object\n");
		return FALSE;
	    }

	    JXTA_OBJECT_RELEASE(found);
	}
    }
    for (i = 500000; i--> 0; ) {
	int k = rand();
	Num_val* found;
	res = jxta_hashtable_get(hash_num, &k, sizeof(k),
					     (Jxta_object**) &found);
	if (res != JXTA_SUCCESS) {
	    printf("Ooops lookup failed (phase 2) : %d\n", k);
		    printf("trying stupid lookup\n");
		    res = jxta_hashtable_stupid_lookup(hash_num, &k, sizeof(k),
						       (Jxta_object**) &found);
		    if (res == JXTA_SUCCESS) {
			if (found->n == k) {
			    printf("The key's in the table. get screws up.\n");
			} else {
			    printf("The key's in the table but wrong val.\n");
			}
		    } else {
			printf("The key is not in the table. So lookup is "
			       "fine. Either del or put screws up.\n");
		    }
	    return FALSE;
	}

	if (found->n != k) {
	    printf("Ooops found the wrong object\n");
	    return FALSE;
	}
	JXTA_OBJECT_RELEASE(found);
    }

    lookup = time(NULL) - start;

    start = time(NULL);
    all_vals = jxta_hashtable_values_get(hash_num);
    if (all_vals == NULL) {
	printf("There was not enough memory to allocate the values vector.\n");
    } else {
	printf("Found %ld values in hashtable\n", jxta_vector_size(all_vals));
	JXTA_OBJECT_RELEASE(all_vals);
    }
    getvals = time(NULL) - start;

    jxta_hashtable_stats(hash_num, &total_size, &usage,
			 &occupancy, &max_occupancy, &avg_hops);

    /* now delete the whole table and its 1.5 Million entries. */
    start = time(NULL);
    JXTA_OBJECT_RELEASE(hash_num);
    release = time(NULL) - start;


    printf("Statistics for numerical usage:\n");
    printf("===============================\n");
    printf("table size      : %d\n", total_size);
    printf("used entries    : %d (%d duplicates not shown)\n", usage, duplicates);
    printf("total occupancy : %d\n", occupancy);
    printf("max occupancy   : %d\n", max_occupancy);
    printf("avg rehash      : %f\n", avg_hops);

    printf("time (seconds) - 1000000 initial insertions: %ld\n", insertion);
    printf("time (seconds) - 500000  deletions         : %ld\n", deletion);
    printf("time (seconds) - 500000  addl insertions   : %ld\n", reinsertion);
    printf("time (seconds) - 1000000 lookups           : %ld\n", lookup);
    printf("time (seconds) - getting/releasing all vals: %ld\n", getvals);
    printf("time (seconds) - deletion                  : %ld\n", release);
    return TRUE;
}

/*
static int test_str(void) {
    return 0;
}

static int test_id(void) {
    return 0;
}
*/

/**
* Run the unit test for the jxta_hash_test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_jxta_hash_tests( int * tests_run,
				int * tests_passed,
				int * tests_failed){
  Jxta_boolean result;
  
  *tests_run += 1;
  result = test_num();
 
  if( result == TRUE){
    *tests_passed += 1;
  }
  else{
    *tests_failed += 1;
  }

  return result;
}



#ifdef STANDALONE
int main(int argc, char* argv[]) {
    Jxta_boolean result;
    int i = 0;

    jxta_initialize();

	result = test_num();
    if( result == FALSE)  
         i = -1;

    jxta_terminate();
    return i;
}
#endif

/* vim: set ts=4 sw=4 tw=130 et: */
