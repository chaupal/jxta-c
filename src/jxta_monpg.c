/*
 * Copyright (c) 2008 Sun Microsystems, Inc.  All rights reserved.
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
 * This is an implementation of the netPeerGroup spec that does not need
 * the impl adv to provide the list of services. It knows them all by heart.
 * Since it cannot be customized, it cannot be used for anything else but
 * a netPeerGroup-like peergroup. That is, only groups that use
 * NetPeerGroupSpecID as their group subclass spec may be implemented with
 * this class.
 * However, this implementation can be derived-from to implement other specs
 * by overloading some methods. (such as the init method)...can wait.
 */

static const char *__log_cat = "MONPG";

#ifdef __APPLE__
#include <sys/types.h>
#endif

#include "jpr/jpr_excep_proto.h"
#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_defloader_private.h"
#include "jxta_monpg_private.h"
#include "jxta_netpg_private.h"
#include "jxta_netpg.h"
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_pga.h"
#include "jxta_svc.h"
#include "jxta_hta.h"
#include "jxta_mia.h"
#include "jxta_monitorconfig.h"
#include "jxta_id_uuid_priv.h"

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__        /* UNSUSED */
#endif
#endif

static void monpg_stop(Jxta_module * self)
{
    Jxta_monpg *it = (Jxta_monpg *) self;
    Jxta_monpg* myself = PTValid(self, Jxta_monpg);

    if (!it->started) return;

    it->started = FALSE;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, FILEANDLINE "MonPeerGroup Ref Count before stop :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(myself));

    if (it->tcp != NULL) {
        jxta_module_stop((Jxta_module *) (it->tcp));
    }
    if (it->router != NULL) {
        jxta_module_stop((Jxta_module *) (it->router));
    }
    if (it->relay != NULL) {
        jxta_module_stop((Jxta_module *) (it->relay));
    }
    if (((Jxta_stdpg *) it)->endpoint != NULL) {
        jxta_module_stop((Jxta_module *) (((Jxta_stdpg *) it)->endpoint));
    }
    /* Think of this as "Super.stop()" */
    ((Jxta_module_methods *) & jxta_stdpg_methods)->stop((Jxta_module*)myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count after stop :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

static Jxta_status monpg_start(Jxta_module * me, const char *args[])
{
    Jxta_status rv;
    const char *noargs[] = { NULL };
    Jxta_monpg* myself = PTValid(me, Jxta_monpg);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count before start :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(me));

    rv = peergroup_start((Jxta_PG *) myself);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    /* Now, start all services */
    rv = jxta_module_start((Jxta_module *) (((Jxta_stdpg *) myself)->endpoint), noargs);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting endpoint: %d.\n", rv);
        monpg_stop(me);
        return rv;
    }

    if (myself->tcp) {
        rv = jxta_module_start((Jxta_module *) (myself->tcp), noargs);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting tcp: %d.\n", rv);
            monpg_stop(me);
            return rv;
        }
    }

    rv = jxta_module_start((Jxta_module *) (myself->router), noargs);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting router: %d.\n", rv);
        monpg_stop(me);
        return rv;
    }

    if (myself->relay) {
        rv = jxta_module_start((Jxta_module *) (myself->relay), noargs);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting relay: %d.\n", rv);
            monpg_stop(me);
            return rv;
        }
    }

    /* Now, start all services */
    rv = jxta_stdpg_start_modules(me);
    if (JXTA_SUCCESS != rv) {
        monpg_stop(me);
        return rv;
    }

    /* we dont do anything */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count after start :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(me));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "started.\n");

    myself->started = TRUE;

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_MIA *) jxta_get_monpeergroupMIA(void)
{
    JString *code;
    JString *desc;
    Jxta_MIA *mia = NULL;

    jxta_stdpg_methods.get_genericpeergroupMIA(NULL, &mia);

    code = jstring_new_2("builtin:monpg");
    desc = jstring_new_2("Monitor Group native implementation");

    jxta_MIA_set_Code(mia, code);
    jxta_MIA_set_Desc(mia, desc);

    JXTA_OBJECT_RELEASE(code);
    JXTA_OBJECT_RELEASE(desc);

    jxta_MIA_set_MSID(mia, jxta_ref_netpeergroup_specid);
    return mia;
}

/*
 * This is the net peer group and since jxta-C does not have "platform"
 * underneath it, it is the one to initiate the peer id and config.
 * However, since I might in the future rename this class and
 * derive other kind of groups from it, I will not assume it serves as the
 * root group in all cases.
 */
static Jxta_status monpg_init(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status res;
    Jxta_boolean release_mia = FALSE;
    Jxta_monpg *it = PTValid(self, Jxta_monpg);
    Jxta_PA *config_adv = NULL;
    Jxta_id *pg_id = NULL;
    apr_uuid_t uuid;

    /* Must read the MonitorConfig file to start the proper net peergroup */
    config_adv = jxta_MonitorConfig_read("MonitorConfig");
    it->started = FALSE;

    /* If we have no MonitorConfig file, create one */
    if (config_adv == NULL) {
        config_adv = jxta_MonitorConfig_create_default();
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "A new \"MonitorConfig\" file will be generated.\n");
    }

    if (!jxta_PA_get_SN(config_adv, &uuid)) {
        apr_uuid_t * uuid_ptr;
        uuid_ptr = jxta_id_uuid_new();
        memmove(&uuid, uuid_ptr, sizeof(apr_uuid_t));
        free(uuid_ptr);
    } else {
        Jpr_absolute_time ttime;
        int cmp;
        ttime = apr_time_now();
        cmp = jxta_id_uuid_time_compare(&uuid, ttime);
        if (UUID_EQUALS == cmp) {
            /* time was moved back on this system */
            jxta_id_uuid_increment_seq_number(&uuid);
        }
    }
    jxta_PA_set_SN(config_adv, &uuid);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonitorPeerGroup Ref Count before init : %d [%pp].\n",
                    JXTA_OBJECT_GET_REFCOUNT(self), self);

    res = peergroup_init((Jxta_PG *) self, NULL);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    /*
     * If we're used as the root group (normally always) and if
     * we aren't assigned an ID, then we are the default monpg.
     * Otherwise let things happen as they normaly do (stdpg will
     * pick a new ID) - as for the impl_adv and labels, if we're the
     * monpg, then we set it up ourselves and drop whatever we've been
     * given, else let the base class sort it out.
     */
    if (group == NULL && assigned_id == NULL) {
        JString *name;
        JString *desc;

        pg_id = jxta_PA_get_GID(config_adv);
        assigned_id = pg_id;

        /*
         * We are simulating the old behaviour
         * if jxta-Null is set read from the platformConfig
         * well then set the default NetPeerGroupId 
         */
        if (jxta_id_equals(assigned_id, jxta_id_nullID)) {
            /* no share needed as jxta_id_defaultMonPeerGroupID is static object */
            assigned_id = jxta_id_defaultMonPeerGroupID;
            jxta_PA_set_GID(config_adv, assigned_id);
        }

        /*
         * Whatever mia was given to us, we have not counted a
         * ref on it, so it does not need to be released
         * However, the one we make here comes with a count for us,
         * so we'll have to release it when we're done.
         */
        if (impl_adv == NULL) {
            impl_adv = (Jxta_advertisement *) jxta_get_netpeergroupMIA();
        } else {
            JXTA_OBJECT_SHARE(impl_adv);
        }

        release_mia = TRUE;

        name = jstring_new_2("MonPeerGroup");
        desc = jstring_new_2("MonPeerGroup by default");

        jxta_stdpg_methods.set_labels((Jxta_PG *) it, name, desc);

        JXTA_OBJECT_RELEASE(name);
        JXTA_OBJECT_RELEASE(desc);
    }

    /*
     * use the superclass's split init routines, so that
     * we can init the transports before anything gets started.
     */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count before super group init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    jxta_stdpg_set_configadv(self, config_adv);
    res = jxta_stdpg_init_group_1(self, group, assigned_id, impl_adv);

    /* can only release pg_id after we done with assigned_id */
    JXTA_OBJECT_RELEASE(pg_id);


    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count after super group init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    if (release_mia) {
        /*
         * The MIA we built now belongs to the base class.
         */
        JXTA_OBJECT_RELEASE(impl_adv);
        release_mia = FALSE;
    }

    /* Check the result of the stdpg init after releasing any held memory */
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    /*
     * To make it easier to some services, we need the transports
     * to be initialized first.
     */

    /*
     * Borderline hack:
     * We load the endpoint, so that we can init it first.
     * stdpg cooperates with that by never setting the endpoint
     * when it is the root group.
     */
    res =
        stdpg_ld_mod((Jxta_PG *) it, jxta_endpoint_classid, "builtin:endpoint_service", NULL,
                     (Jxta_module **) & ((Jxta_stdpg *) it)->endpoint);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res = stdpg_ld_mod((Jxta_PG *) it, jxta_tcpproto_classid, "builtin:transport_tcp", NULL, (Jxta_module **) & it->tcp);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }
    
    res = stdpg_ld_mod((Jxta_PG *) it, jxta_routerproto_classid, "builtin:router_client", NULL, (Jxta_module **) & it->router);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res = stdpg_ld_mod((Jxta_PG *) it, jxta_relayproto_classid, "builtin:relay", NULL, (Jxta_module **) & it->relay);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count before super modules init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    res = jxta_stdpg_init_modules(self, FALSE);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count after super modules init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    /* Write out any updates to the MonitorConfig which were made during service init. */
    jxta_MonitorConfig_write(config_adv, "MonitorConfig");
    JXTA_OBJECT_RELEASE(config_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "MonPeerGroup Ref Count after init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");

    return res;

  ERROR_EXIT:
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure during module initialization :%d.\n", res);

    /*
     * For now, we assume that what was init'ed needs to be stopped,
     * but realy it should not be the case and the outside world
     * must not have to worry about stopping a group that was not
     * even successfully init'ed. Releasing it should be enough.
     *
     * So, call our own stop routine behind the scenes, just to be safe.
     */
    monpg_stop(self);

    /*
     * The rest can be done all right by the dtor when/if released.
     */
    return res;
}

/*
 * Note: The following could be in a .h so that subclassers get a static
 * init block for their own methods table, but then, the methods themselves
 * must be exported too, so that the linker can resolve the items. Could
 * cause name collisions. And a static init block is not very usefull
 * if the subclasser wants to override some of the methods. The only
 * alternative is to use thread_once in the subclasse's "new" method and make
 * a copy of the methods table below at runtime to get default values. This
 * table is exported.
 */

Jxta_monpg_methods jxta_monpg_methods;

void monpg_init_methods(void)
{
    memcpy(&jxta_monpg_methods, &jxta_stdpg_methods, sizeof(jxta_stdpg_methods));
    ((Jxta_module_methods *) & jxta_monpg_methods)->init = monpg_init;
    ((Jxta_module_methods *) & jxta_monpg_methods)->start = monpg_start;
    ((Jxta_module_methods *) & jxta_monpg_methods)->stop = monpg_stop;
}

/*
 * Make sure we have a ctor that subclassers can call.
 */
void jxta_monpg_construct(Jxta_monpg * self, Jxta_monpg_methods const *methods)
{
    /*
     * Our methods table type is just a typedefed, it is identical to
     * Jxta_PG_methods and has the same rt type checking signature. So that's
     * what we must check for.
     */
    Jxta_PG_methods* pgmethods = PTValid(methods, Jxta_PG_methods);
    jxta_stdpg_construct((Jxta_stdpg *) self, (Jxta_stdpg_methods const *) pgmethods);

    self->thisType = "Jxta_monpg";
    self->tcp = NULL;
    self->router = NULL;
    self->relay = NULL;
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes
 * un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_monpg_destruct(Jxta_monpg * self)
{
    Jxta_monpg* myself = PTValid(self, Jxta_monpg);

    /*
     * FIXME: jice@jxta.org - 20020222
     * Services and their group reference each other left and right.
     * So if we just unreference the group, it's destructor will not get
     * called because it is referenced by the services, and the other way
     * around.
     * In theory the services and group stop methods should probably undo
     * what init has done, that is getting rid of all the cross-references.
     * May be services should just have weak (uncounted) references to the
     * group and other services since the group holds a master reference to
     * them all at all times. (that's one case where ref-counting is a bit
     * superfluous).
     * We'll revisit that later; it's not that stopping the monpg is all that
     * critical at this stage. 
     */

    /* We are handling http and router directly for now. */
    if (myself->relay != NULL) {
        JXTA_OBJECT_RELEASE(myself->relay);
    }
    if (myself->tcp != NULL) {
        JXTA_OBJECT_RELEASE(myself->tcp);
    }
    if (myself->router != NULL) {
        JXTA_OBJECT_RELEASE(myself->router);
    }

    jxta_stdpg_destruct((Jxta_stdpg *) myself);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
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
 * So, an impl adv that designates a builtin class "monpg" will name the
 * package "builtin:" and the code "monpg".
 * When asked to instantiate a "builtin:monpg" the loader will lookup "monpg"
 * in its built-in table and call the new_instance method that's there.
 */


/*
 * New is not public, but it is exported so that we can hook it to some
 * table.
 */

static void myFree(Jxta_object * obj)
{
    jxta_monpg_destruct((Jxta_monpg *) obj);
    free((void *) obj);
}

Jxta_monpg *jxta_monpg_new_instance(void)
{
    Jxta_monpg *self = (Jxta_monpg *) calloc(1, sizeof(Jxta_monpg));

    JXTA_OBJECT_INIT(self, myFree, 0);

    jxta_monpg_construct(self, &jxta_monpg_methods);

    return self;
}

/* vim: set ts=4 sw=4 et tw=130: */
