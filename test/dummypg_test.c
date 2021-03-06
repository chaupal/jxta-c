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
 * $Id$
 */

/*
 * This is a dummy implementation of peergroup to serve as a test case and
 * example. Implementing a service or a plain module is the same, just
 * shorter and initializing the tbl is simpler.
 *
 * Cooking receipe to implement a peergroup:
 * Copy this file.
 * Replace "dummy" by whatever you like.
 * Fill-in the body of the methods instead of the error returns and exception
 * throwing.
 * If you do not care for subclassers, you can merge
 * jxta_peegroup_dummy_private.h here and make everything static but
 * the "new" method.
 */

#include "jxta.h"

#include "dummypg_test_private.h"

#include "unittest_jxta_func.h"

/*
 * For the sake of example, we declare the variable "it" in every method
 * but it is not used since the body of the methods are not written.
 * We need to shut-up compilers about it.
 */
#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__    /* UNSUSED */
#endif
#endif

Jxta_dummypg *jxta_dummypg_new_instance(void);

/*
 * All the methods:
 * We first cast this back to our local derived type
 * and verify that the object is indeed of that type.
 */

/*
 * Implementations for module methods
 */
static Jxta_status dummy_init(Jxta_module * me, Jxta_PG * group, Jxta_id * assignedID, Jxta_advertisement * implAdv)
{
    Jxta_dummypg *UNUSED__ myself = PTValid(me, Jxta_dummypg);
   
    jxta_service_init((Jxta_service *) myself, NULL, NULL, NULL);

    peergroup_init( (Jxta_PG *) myself, group);

    return JXTA_SUCCESS;
}

static Jxta_status dummy_start(Jxta_module * self, const char *args[])
{
    Jxta_dummypg *UNUSED__ it = PTValid(self, Jxta_dummypg);

    return JXTA_SUCCESS;
}

static void dummy_stop(Jxta_module * self)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);
}

/*
 * implementations for peergroup methods
 */
static void dummy_get_loader(Jxta_PG * self, Jxta_loader ** loader)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *loader = NULL;
}

static void dummy_get_PGA(Jxta_PG * self, Jxta_PGA ** pga)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pga = NULL;
}

static void dummy_get_PA(Jxta_PG * self, Jxta_PA ** pa)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pa = NULL;
}

static Jxta_status dummy_lookup_service(Jxta_PG * self, Jxta_id * name, Jxta_service ** result)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static Jxta_boolean dummy_is_compatible(Jxta_PG * self, JString * compat)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return FALSE;
}

static Jxta_status dummy_loadfromimpl_module(Jxta_PG * self,
                                             Jxta_id * assignedID, Jxta_advertisement * impl, Jxta_module ** result)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static Jxta_status dummy_loadfromid_module(Jxta_PG * self,
                                           Jxta_id * assignedID, Jxta_MSID * specID, int where, Jxta_module ** result)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static void dummy_set_labels(Jxta_PG * self, JString * name, JString * description)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);
}

static Jxta_status dummy_newfromadv(Jxta_PG * self, Jxta_advertisement * pgAdv, Jxta_vector * resource_group, Jxta_PG ** result)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static Jxta_status dummy_newfromimpl(Jxta_PG * self, Jxta_PGID * gid,
                                     Jxta_advertisement * impl, JString * name,
                                     JString * description, Jxta_vector * resource_group, Jxta_PG ** result)
{

    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static Jxta_status dummy_newfromid(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_group, Jxta_PG ** result)
{

    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    return JXTA_NOTIMP;
}

static void dummy_get_rendezvous_service(Jxta_PG * self, Jxta_rdv_service ** rdv)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *rdv = NULL;
}

static void dummy_get_endpoint_service(Jxta_PG * self, Jxta_endpoint_service ** endp)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *endp = NULL;
}

static void dummy_get_resolver_service(Jxta_PG * self, Jxta_resolver_service ** resol)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *resol = NULL;
}

static void dummy_get_discovery_service(Jxta_PG * self, Jxta_discovery_service ** disco)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *disco = NULL;
}

static void dummy_get_peerinfo_service(Jxta_PG * self, Jxta_peerinfo_service ** peerinfo)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *peerinfo = NULL;
}

static void dummy_get_membership_service(Jxta_PG * self, Jxta_membership_service ** memb)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *memb = NULL;
}

static void dummy_get_pipe_service(Jxta_PG * self, Jxta_pipe_service ** pipe)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pipe = NULL;
}

static void dummy_get_GID(Jxta_PG * self, Jxta_PGID ** gid)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *gid = NULL;
}

static void dummy_get_PID(Jxta_PG * self, Jxta_PID ** pid)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pid = NULL;
}

static void dummy_get_groupname(Jxta_PG * self, JString ** nm)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *nm = NULL;
}

static void dummy_get_peername(Jxta_PG * self, JString ** nm)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *nm = NULL;
}

static void dummy_get_configadv(Jxta_PG * self, Jxta_PA ** pa)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pa = NULL;
}

static void dummy_get_genericpeergroupMIA(Jxta_PG * self, Jxta_MIA ** mia)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *mia = NULL;
}

static void dummy_get_resourcegroups(Jxta_PG * self, Jxta_vector ** rg)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *rg = NULL;
}

static void dummy_set_resourcegroups(Jxta_PG * self, Jxta_vector * rg)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);
}

static void dummy_get_parentgroup(Jxta_PG * self, Jxta_PG ** pg)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *pg = NULL;
}

static void dummy_get_compatstatement(Jxta_PG * self, JString ** compat)
{
    Jxta_dummypg *UNUSED__ it = (Jxta_dummypg *) self;
    PTValid(self, Jxta_dummypg);

    *compat = NULL;
}

/*
 * We do not need to subtype the base methods table, we're not adding to it.
 * However, typedef it to be nice to subclassers that expect the naming
 * to follow the regular scheme. Hopefully we should be able to get away
 * with just that and not be forced to always subclass the vtbl.
 *
 * Note: The following could be exported so that subclassers get a static
 * init block for their own methods table, but then, the methods themselves
 * must be exported too, so that the linker can resolve the items. Could
 * cause name collisions. And a static init block is not very usefull
 * if the subclasser wants to override some of the methods. The only
 * alternative is to use thread_once in the subclasse's "new" method and make
 * a copy of the methods table below at runtime to get default values. This
 * table is exported.
 */

Jxta_dummypg_methods const jxta_dummypg_methods = {
    {
     {
      "Jxta_module_methods",
      dummy_init,
      dummy_start,
      dummy_stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl,
     service_on_option_set},
    "Jxta_PG_methods",
    dummy_get_loader,
    dummy_get_PGA,
    dummy_get_PA,
    dummy_lookup_service,
    dummy_is_compatible,
    dummy_loadfromimpl_module,
    dummy_loadfromid_module,
    dummy_set_labels,
    dummy_newfromadv,
    dummy_newfromimpl,
    dummy_newfromid,
    dummy_get_rendezvous_service,
    dummy_get_endpoint_service,
    dummy_get_resolver_service,
    dummy_get_discovery_service,
    dummy_get_peerinfo_service,
    dummy_get_membership_service,
    dummy_get_pipe_service,
    dummy_get_GID,
    dummy_get_PID,
    dummy_get_groupname,
    dummy_get_peername,
    dummy_get_configadv,
    dummy_get_genericpeergroupMIA,
    dummy_set_resourcegroups,
    dummy_get_resourcegroups,
    dummy_get_parentgroup,
    dummy_get_compatstatement
};

/*
 * Make sure we have a new that subclassers can call.
 */
Jxta_dummypg *dummypg_construct(Jxta_dummypg * myself, Jxta_dummypg_methods const * methods)
{
    PTValid(methods, Jxta_PG_methods);
    
    jxta_PG_construct((Jxta_PG *) myself, (Jxta_PG_methods *) methods);
    
    myself->thisType = "Jxta_dummypg";
    
    return myself;
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_dummypg_destruct(Jxta_dummypg * self)
{
    jxta_PG_destruct((Jxta_PG *) self);
}

/*
 * So, how does this "implementation" gets hooked up ?
 * Idealy, some impl adv points at some content that has this piece
 * of code in it. This piece of code gets loaded and its newInstance methods
 * remains associated with whatever was used to designate it in the impl adv.
 * This means finding this "designated entry point" in the code when loading it
 * something that shared libs have some support for.
 *
 * Since for now we're not playing with code loading, everything is built-in,
 * so we go and hook it into a table at link time. This is done by looking
 * for all the jxta_*_new_instance symbols that we can find. The name
 * it is associated with is simply whatever the "*" is. Or even, at phase 0
 * we just write the table by hand :-)
 * So, an impl adv that designates a builtin class "netpg" will name the
 * package "builtin:" and the code "netpg".
 * When asked to instantiate a "builtin:netpg" the loader will lookup "netpg"
 * in its built-in table and call the new_instance method that's there.
 */

static void dummypg_delete(Jxta_object *obj)
{
    printf("FREE %p\n", obj);

    jxta_dummypg_destruct((Jxta_dummypg *) obj);
    free(obj);
}

/*
 * New is not public, but it is exported so that we can hook it to some
 * table.
 */
Jxta_dummypg *jxta_dummypg_new_instance(void)
{
    Jxta_dummypg *myself = (Jxta_dummypg *) calloc(1, sizeof(Jxta_dummypg));
    JXTA_OBJECT_INIT(myself, dummypg_delete, 0);
    
    dummypg_construct(myself, &jxta_dummypg_methods);
    
    return myself;
}

/**
* Run the unit tests for dummypg
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
* 
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
const char * dummy_pg_test(void)
{
    Jxta_PG *test_grp = (Jxta_PG *) jxta_dummypg_new_instance();
    Jxta_advertisement *mia = NULL;
    jxta_service_get_MIA((Jxta_service *) test_grp, &mia);

    if (mia != NULL) {
        return FILEANDLINE " Uhoh, Jxta_dummypg::jxta_service_MIA_get returned, but with the wrong result\n";
    }

    if (jxta_module_start((Jxta_module *) test_grp, NULL) != JXTA_SUCCESS) {
        return FILEANDLINE " Uhoh, Jxta_dummypg::jxta_module_stop returned, but with the wrong result\n";
    }

    JXTA_OBJECT_RELEASE(test_grp);

    return NULL;
}

static struct _funcs dummypg_test_funcs[] = {
    /*  */
    {*dummy_pg_test, "Construction and starting for dummy peer group"},

    {NULL, "null"}
};


/**
* Run the unit tests for the jxta_message test routines
*
* @param tests_run the variable in which to accumulate the number of tests run
* @param tests_passed the variable in which to accumulate the number of tests passed
* @param tests_failed the variable in which to accumulate the number of tests failed
*
* @return TRUE if all tests were run successfully, FALSE otherwise
*/
Jxta_boolean run_dummypg_tests(int *tests_run, int *tests_passed, int *tests_failed)
{
    return run_testfunctions(dummypg_test_funcs, tests_run, tests_passed, tests_failed);
}


#ifdef STANDALONE
int main(int argc, char **argv)
{
    return main_test_function(dummypg_test_funcs, argc, argv);
}
#endif

/* vim: set ts=4 sw=4 et tw=130: */
