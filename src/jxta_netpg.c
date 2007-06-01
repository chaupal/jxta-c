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
 * $Id: jxta_netpg.c,v 1.69 2006/02/17 18:24:08 slowhog Exp $
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

static const char *__log_cat = "NETPG";

#ifdef __APPLE__
#include <sys/types.h>
#endif

#include "jpr/jpr_excep.h"
#include "jxta_apr.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_defloader_private.h"
#include "jxta_netpg_private.h"
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_pga.h"
#include "jxta_svc.h"
#include "jxta_hta.h"
#include "jxta_mia.h"
#include "jxta_platformconfig.h"

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__    /* UNSUSED */
#endif
#endif

/*
 * All the methods:
 * We first cast this back to our local derived type
 * and verify that the object is indeed of that type.
 */

/*
 * Load a service properly; that is make sure it
 * initializes ok before returning it, otherwise, release it.
 * FIXME: we need to have stdpg do that for us.
 */
static Jxta_module *ld_mod(Jxta_netpg * self, Jxta_id * class_id, const char *name, Throws)
{
    Jxta_module *m;

    ThrowThrough();

    m = jxta_defloader_instantiate_e(name, MayThrow);
    Try {
        jxta_module_init_e(m, (Jxta_PG *) self, class_id, 0, MayThrow);
    } Catch {
        Jxta_status res = jpr_lasterror_get();
        JXTA_OBJECT_RELEASE(m);
        if (res == JXTA_NOT_CONFIGURED) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "%s skipped as not configured \n", name);
            return NULL;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "%s failed to initialize (%d)\n", name, res);

        Throw(res);
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "%s loaded and initialized\n", name);
    return m;
}

static Jxta_status netpg_start(Jxta_module * module, const char *args[])
{
    Jxta_netpg *self = (Jxta_netpg *) module;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count before start :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    PTValid(self, Jxta_netpg);

    /* Think of this as "Super.start()" */

    ((Jxta_module_methods *) & jxta_stdpg_methods)->start((Jxta_module *) self, args);

    /* we dont do anything */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count after start :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "started.\n");

    return JXTA_SUCCESS;
}

static void netpg_stop(Jxta_module * self)
{
    Jxta_netpg *it = (Jxta_netpg *) self;
    PTValid(self, Jxta_netpg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count before stop :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    /* We stop http and router ourselves (as well as endpoint) */
    if (it->http != NULL) {
        jxta_module_stop((Jxta_module *) (it->http));
    }
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
    ((Jxta_module_methods *) & jxta_stdpg_methods)->stop(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count after stop :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

JXTA_DECLARE(Jxta_MIA *) jxta_get_netpeergroupMIA(void)
{
    JString *code;
    JString *desc;
    Jxta_MIA *mia = NULL;

    jxta_stdpg_methods.get_genericpeergroupMIA(NULL, &mia);

    code = jstring_new_2("builtin:netpg");
    desc = jstring_new_2("Net Peer Group native implementation");

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
static void netpg_init_e(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv, Throws)
{
    Jxta_boolean release_mia = FALSE;
    Jxta_status rv = JXTA_SUCCESS;
    Jxta_PA *config_adv = NULL;

    const char *noargs[] = { NULL };
    Jxta_netpg *it = (Jxta_netpg *) self;
    PTValid(self, Jxta_netpg);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count before init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    /* Must read the PlatformConfig file to start the proper net peergroup */
    config_adv = jxta_PlatformConfig_read("PlatformConfig");

    /* If we have no PlatformConfig file, create one */
    if (config_adv == NULL) {
        config_adv = jxta_PlatformConfig_create_default();
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "A new \"PlatformConfig\" file will be generated.\n");
    }

    /*
     * If we're used as the root group (normally always) and if
     * we aren't assigned an ID, then we are the default netpg.
     * Otherwise let things happen as they normaly do (stdpg will
     * pick a new ID) - as for the impl_adv and labels, if we're the
     * netpg, then we set it up ourselves and drop whatever we've been
     * given, else let the base class sort it out.
     */
    if (group == NULL && assigned_id == NULL) {
        JString *name;
        JString *desc;

        assigned_id = jxta_PA_get_GID(config_adv);

    /**
	 * We are simulating the old behaviour
	 * if jxta-Null is set read from the platformConfig
	 * well then set the default NetPeerGroupId 
	 */
        if (jxta_id_equals(assigned_id, jxta_id_nullID)) {
            JXTA_OBJECT_RELEASE(assigned_id);
            assigned_id = jxta_id_defaultNetPeerGroupID;
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
        }

        release_mia = TRUE;

        name = jstring_new_2("NetPeerGroup");
        desc = jstring_new_2("NetPeerGroup by default");

        jxta_stdpg_methods.set_labels((Jxta_PG *) it, name, desc);

        JXTA_OBJECT_RELEASE(name);
        JXTA_OBJECT_RELEASE(desc);
    }

    /*
     * use the superclass's split init routines, so that
     * we can init the transports before anything gets started.
     */
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count before super group init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    jxta_stdpg_set_configadv(self, config_adv);
    jxta_stdpg_init_group(self, group, assigned_id, impl_adv);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count after super group init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));

    if (release_mia) {
        /*
         * The MIA we built now belongs to the base class.
         */
        JXTA_OBJECT_RELEASE(impl_adv);
        release_mia = FALSE;
    }

    /*
     * To make it easier to some services, we need the transports
     * to be initialized first.
     */
    Try {
        /*
         * Borderline hack:
         * We load the endpoint, so that we can init it first.
         * stdpg cooperates with that by never setting the endpoint
         * when it is the root group.
         */
        ((Jxta_stdpg *) it)->endpoint = (Jxta_endpoint_service *)
            ld_mod(it, jxta_endpoint_classid, "builtin:endpoint_service", MayThrow);

        it->http = (Jxta_transport *)
            ld_mod(it, jxta_httpproto_classid, "builtin:transport_http", MayThrow);

        it->tcp = (Jxta_transport *)
            ld_mod(it, jxta_tcpproto_classid, "builtin:transport_tcp", MayThrow);

        it->router = (Jxta_transport *)
            ld_mod(it, jxta_routerproto_classid, "builtin:router_client", MayThrow);

        it->relay = (Jxta_transport *)
            ld_mod(it, jxta_relayproto_classid, "builtin:relay", MayThrow);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count before super modules init :%d.\n",
                        JXTA_OBJECT_GET_REFCOUNT(self));

        jxta_stdpg_init_modules_e(self, MayThrow);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count after super modules init :%d.\n",
                        JXTA_OBJECT_GET_REFCOUNT(self));

    }
    Catch {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Exception during module initialization :%d.\n",
                        jpr_lasterror_get());

        /*
         * For now, we assume that what was init'ed needs to be stopped,
         * but realy it should not be the case and the outside world
         * must not have to worry about stopping a group that was not
         * even successfully init'ed. Releasing it should be enough.
         *
         * So, call our own stop routine behind the scenes, just to be safe.
         */
        netpg_stop(self);

        /*
         * The rest can be done all right by the dtor when/if released.
         */
        Throw(jpr_lasterror_get());
    }

    /* Write out any updates to the PlatformConfig which were made during service init. */
    jxta_PlatformConfig_write(config_adv, "PlatformConfig");
    JXTA_OBJECT_RELEASE(config_adv);

    /* Now, start all services */

    rv = jxta_module_start((Jxta_module *) (((Jxta_stdpg *) it)->endpoint), noargs);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting endpoint: %d.\n", rv);
        netpg_stop(self);
        Throw(rv);
    }

    if (it->tcp) {
        rv = jxta_module_start((Jxta_module *) (it->tcp), noargs);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting tcp: %d.\n", rv);
            netpg_stop(self);
            Throw(rv);
        }
    }

    if (it->http) {
        rv = jxta_module_start((Jxta_module *) (it->http), noargs);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting http: %d.\n", rv);
            netpg_stop(self);
            Throw(rv);
        }
    }

    rv = jxta_module_start((Jxta_module *) (it->router), noargs);
    if (JXTA_SUCCESS != rv) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting router: %d.\n", rv);
        netpg_stop(self);
        Throw(rv);
    }

    if (it->relay) {
        rv = jxta_module_start((Jxta_module *) (it->relay), noargs);
        if (JXTA_SUCCESS != rv) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Failure starting relay: %d.\n", rv);
            netpg_stop(self);
            Throw(rv);
        }
    }

    /* Now, start all services */
    jxta_stdpg_start_modules(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, FILEANDLINE "NetPeerGroup Ref Count after init :%d.\n",
                    JXTA_OBJECT_GET_REFCOUNT(self));
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Initialized\n");
}

static Jxta_status netpg_init(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * implAdv)
{
    Try {
        netpg_init_e(self, group, assigned_id, implAdv, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }

    return JXTA_SUCCESS;
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

Jxta_netpg_methods jxta_netpg_methods;
void netpg_init_methods(void)
{
    memcpy(&jxta_netpg_methods, &jxta_stdpg_methods, sizeof(jxta_stdpg_methods));
    ((Jxta_module_methods *) & jxta_netpg_methods)->init = netpg_init;
    ((Jxta_module_methods *) & jxta_netpg_methods)->init_e = netpg_init_e;
    ((Jxta_module_methods *) & jxta_netpg_methods)->start = netpg_start;
    ((Jxta_module_methods *) & jxta_netpg_methods)->stop = netpg_stop;
}

/*
 * Make sure we have a ctor that subclassers can call.
 */
void jxta_netpg_construct(Jxta_netpg * self, Jxta_netpg_methods * methods)
{
    /*
     * Our methods table type is just a typedefed, it is identical to
     * Jxta_PG_methods and has the same rt type checking signature. So that's
     * what we must check for.
     */
    PTValid(methods, Jxta_PG_methods);
    jxta_stdpg_construct((Jxta_stdpg *) self, (Jxta_stdpg_methods *) methods);

    self->thisType = "Jxta_netpg";
    self->tcp = NULL;
    self->http = NULL;
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
void jxta_netpg_destruct(Jxta_netpg * self)
{
    PTValid(self, Jxta_netpg);

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
     * We'll revisit that later; it's not that stopping the netpg is all that
     * critical at this stage. 
     */

    /* We are handling http and router directly for now. */
    if (self->relay != NULL) {
        JXTA_OBJECT_RELEASE(self->relay);
    }
    if (self->tcp != NULL) {
        JXTA_OBJECT_RELEASE(self->tcp);
    }
    if (self->http != NULL) {
        JXTA_OBJECT_RELEASE(self->http);
    }
    if (self->router != NULL) {
        JXTA_OBJECT_RELEASE(self->router);
    }

    jxta_stdpg_destruct((Jxta_stdpg *) self);

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
 * So, an impl adv that designates a builtin class "netpg" will name the
 * package "builtin:" and the code "netpg".
 * When asked to instantiate a "builtin:netpg" the loader will lookup "netpg"
 * in its built-in table and call the new_instance method that's there.
 */


/*
 * New is not public, but it is exported so that we can hook it to some
 * table.
 */

static void myFree(Jxta_object * obj)
{
    jxta_netpg_destruct((Jxta_netpg *) obj);
    free((void *) obj);
}

Jxta_netpg *jxta_netpg_new_instance(void)
{
    Jxta_netpg *self = (Jxta_netpg *) calloc(1, sizeof(Jxta_netpg));

    JXTA_OBJECT_INIT(self, myFree, 0);

    /*
     * Initialize the methods table if needed.
     */
    jxta_netpg_construct(self, &jxta_netpg_methods);
    return self;
}

/* vim: set ts=4 sw=4 et tw=130: */
