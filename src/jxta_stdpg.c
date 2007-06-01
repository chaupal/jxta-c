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
 * $Id: jxta_stdpg.c,v 1.66 2006/09/29 01:28:45 slowhog Exp $
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

static const char *__log_cat = "STDPG";

#include <ctype.h>

#ifdef __APPLE__
#include <sys/types.h>
#endif

#ifdef WIN32
#include <winsock2.h>
    /* For platforms using the autoconf toolchain this will be defined in Makefile.am */
#define TARGET_ARCH_OS_PLATFORM "i386-Windows-Win32"
#else
#include "netinet/in.h"
#endif

#include "jpr/jpr_excep_proto.h"
#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_cm.h"
#include "jxta_object_priv.h"
#include "jxta_cm_private.h"
#include "jxta_defloader_private.h"
#include "jxta_stdpg_private.h"
#include "jxta_id.h"
#include "jxta_pa.h"
#include "jxta_pga.h"
#include "jxta_svc.h"
#include "jxta_hta.h"
#include "jxta_relaya.h"
#include "jxta_mia.h"
#include "jxta_routea.h"
#include "jxta_platformconfig.h"

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__        /* UNSUSED */
#endif
#endif

/*
 * Load a service properly; that is make sure it
 * initializes ok before returning it, otherwise, release it.
 */
Jxta_status stdpg_ld_mod(Jxta_PG * self, Jxta_id * class_id, const char *name, Jxta_MIA * impl_adv, Jxta_module ** module)
{
    Jxta_status res;

    res = jxta_defloader_instantiate(name, module);

    if (JXTA_SUCCESS != res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s failed to load (%d)\n", name, res);
        return res;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "%s loaded.\n", name);

    res = jxta_module_init(*module, self, class_id, (Jxta_advertisement *) impl_adv);

    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(*module);
        *module = NULL;

        if (res == JXTA_NOT_CONFIGURED) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "%s skipped as not configured \n", name);
            return JXTA_SUCCESS;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "%s failed to initialize (%d)\n", name, res);

        return res;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "%s loaded and initialized\n", name);

    return res;
}

#if APR_HAS_DSO
/**
 * Dynamically load a service properly; that is make sure it
 * initializes ok before returning it, otherwise, release it.
 */
static Jxta_module *ld_dso_mod(Jxta_stdpg * self, Jxta_id * class_id, const char *lib_name, const char *constructor_name,
                               Jxta_MIA * impl_adv)
{
    Jxta_status res;
    Jxta_module *m;
    apr_dso_handle_t *handle = NULL;
    apr_status_t status;
    Jxta_module *(*module_new_instance) (void);
    char error_buf[256];

    status = apr_dso_load(&handle, lib_name, self->pool);
    if (status != APR_SUCCESS) {
        apr_dso_error(handle, error_buf, 256);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to dynamically load library %s. Error %s\n", lib_name,
                        error_buf);
        return NULL;
    }

    status = apr_dso_sym((apr_dso_handle_sym_t *) &module_new_instance, handle, constructor_name);
    if (status != APR_SUCCESS) {
        apr_dso_error(handle, error_buf, 256);
        apr_dso_unload(handle);
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to dynamically get sym %s from library %s. Error %s\n",
                        constructor_name, lib_name, error_buf);
        return NULL;
    }

    if (module_new_instance != NULL) {
        m = (*module_new_instance) ();
    } else {
        apr_dso_unload(handle);
        return NULL;
    }

    if (m == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed to dynamically create init service using %s\n",
                        constructor_name);
        apr_dso_unload(handle);
        return NULL;
    } else {
      /** Keep it for calling dso_unload later one (when stop) **/
        JString *idstr;
        char *idstr_c;

        status = jxta_id_to_jstring(class_id, &idstr);
        idstr_c = apr_pstrdup(self->pool, jstring_get_string(idstr));

        apr_hash_set(self->services_handle, (const void *) idstr_c, strlen(idstr_c), (const void *) handle);
        JXTA_OBJECT_RELEASE(idstr);
    }

    res = jxta_module_init(m, (Jxta_PG *) self, class_id, (Jxta_advertisement *) impl_adv);

    if (JXTA_SUCCESS == res) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Module '%s' loaded and initialized\n", lib_name);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Module '%s' failed to initialize (%d)\n", lib_name, res);
        JXTA_OBJECT_RELEASE(m);
        m = NULL;
    }

    return m;
}
#endif

/*
 * Implementations for module methods
 */
static Jxta_status stdpg_start(Jxta_module * self, const char *args[])
{
    Jxta_status rv;
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Started.\n");

    rv = peergroup_start((Jxta_PG *) self);
    if (JXTA_SUCCESS != rv) {
        return rv;
    }

    /* Now, start the services */
    rv = jxta_stdpg_start_modules(self);
    return rv;
}

static void stdpg_stop(Jxta_module * self)
{
    Jxta_id *gid;
    char **keys_of_services = NULL;
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    /*
     * Unregister from the group registry if we're in-there.
     */
    gid = jxta_PA_get_GID(it->peer_adv);
    jxta_unregister_group_instance(gid, (Jxta_PG *) self);
    JXTA_OBJECT_RELEASE(gid);

    /** First stop, custom user services */
    keys_of_services = jxta_hashtable_keys_get(it->services);
    if (keys_of_services != NULL) {
        unsigned int i = 0;
        Jxta_module *module = NULL;
        Jxta_status res = JXTA_SUCCESS;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Custom services to stop\n");
        while (keys_of_services[i]) {
            res = jxta_hashtable_get(it->services, keys_of_services[i], strlen(keys_of_services[i]), JXTA_OBJECT_PPTR(&module));
            if (res == JXTA_SUCCESS) {
                jxta_module_stop(module);
                JXTA_OBJECT_RELEASE(module);
            }
            free(keys_of_services[i]);
            i++;
        }
        free(keys_of_services);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No custom services to stop\n");
    }

    if (it->membership != NULL) {
        jxta_module_stop((Jxta_module *) (it->membership));
    }
    if (it->pipe != NULL) {
        jxta_module_stop((Jxta_module *) (it->pipe));
    }
    if (it->discovery != NULL) {
        jxta_module_stop((Jxta_module *) (it->discovery));
    }
    if (it->resolver != NULL) {
        jxta_module_stop((Jxta_module *) (it->resolver));
    }
    if (it->srdi != NULL) {
        jxta_module_stop((Jxta_module *) (it->srdi));
    }
    if (it->rendezvous != NULL) {
        jxta_module_stop((Jxta_module *) (it->rendezvous));
    }

    /* We did not start peerinfo, why stop it?
       if (it->peerinfo != NULL) {
       jxta_module_stop((Jxta_module*) (it->peerinfo));
       }
     */

    peergroup_stop((Jxta_PG *) it);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Stopped.\n");
}

void jxta_stdpg_set_configadv(Jxta_module * self, Jxta_PA * config_adv)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);
    if (NULL != it->config_adv) {
        JXTA_OBJECT_RELEASE(it->config_adv);
    }

    if (config_adv != NULL)
        JXTA_OBJECT_SHARE(config_adv);

    it->config_adv = config_adv;
}

/*
 * This routine inits the group but not its modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * We have a lot of alternatives, because as the base pg class, we also normally
 * serve as the base class for the root group.
 */
void jxta_stdpg_init_group(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * impl_adv)
{
    Jxta_status status;
    JString *mia_parm_jstring = NULL;
    Jxta_advertisement *parm_advs = NULL;
    Jxta_vector *parm_advs_vector = NULL;
    unsigned int i = 0;
    JString *peername_to_use = NULL;
    Jxta_id *pid_to_use = NULL;
    Jxta_id *gid_to_use = NULL;
    Jxta_id *msid_to_use = NULL;
    Jxta_svc *svc = NULL;
    Jxta_cm *cache_manager = NULL;
    Jxta_CacheConfigAdvertisement *cache_config = NULL;
    const char *home;
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);
    apr_thread_pool_t *thread_pool = NULL;

    /* The assigned ID is supposed to be the grp ID.
     * if we're the root group we may not have been given one, though.
     */
    if ((assigned_id == NULL) || jxta_id_equals(assigned_id, jxta_id_nullID)) {
        /*
         * We weren't given a gid.
         * assume this is a new grp being created and make a new gid.
         */
        jxta_id_peergroupid_new_1(&gid_to_use);
    } else {
        gid_to_use = JXTA_OBJECT_SHARE(assigned_id);
    }

    /*
     * If we do not have a home group, we serve as the root group.
     * Do what this implies.
     */
    if (group == NULL) {
        /*
         * We are the root group, and we have an initialized config adv.
         * Therefore that's were our PID and name are.
         */
        pid_to_use = jxta_PA_get_PID(it->config_adv);

        if ((NULL == pid_to_use) || jxta_id_equals(pid_to_use, jxta_id_nullID)) {
            jxta_id_peerid_new_1(&pid_to_use, gid_to_use);
            jxta_PA_set_PID(it->config_adv, pid_to_use);
        }

        peername_to_use = jxta_PA_get_Name(it->config_adv);
    } else {
        JXTA_OBJECT_SHARE(group);
        it->home_group = group;

        /*
         * By default get peerconfig from our home group. Subclasses
         * can change it. Getting the peerconfig from our home group
         * does not mean that we rely on it to get our pid and name. Our
         * home group ALWAYS has a pid and name to give us (and that's what we
         * use in the current JXTA design), it does not always
         * have a config to give us; it could be null.
         */

        /*
         * After all that seems not to be the best default behaviour.
         * it is very rare that the parent's config adv makes sense.
         * It does for netpg over platform in the SE implem, but that's it
         * and we do not have that set-up in C.
         */
        jxta_PG_get_configadv(group, &(it->config_adv));
        jxta_PG_get_PID(group, &(pid_to_use));
        jxta_PG_get_peername(group, &(peername_to_use));
    }
    /* create a cache manager */
    home = getenv("JXTA_HOME");
    jxta_PA_get_Svc_with_id(it->config_adv, jxta_cache_classid, &svc);
    if (NULL != svc) {
        cache_config = jxta_svc_get_CacheConfig(svc);
    }
    if (NULL == cache_config) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No cache configuration specified - Using default\n");
        cache_config = jxta_CacheConfigAdvertisement_new();
        status = jxta_CacheConfigAdvertisement_create_default(cache_config, FALSE);
        if (JXTA_SUCCESS != status) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, "Unable to create default cache Configuration %i\n", status);
        }
    }
    if (home != NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Using custom CM directory.\n");
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Using default CM directory.\n");
        home = ".cm";
    }
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Using CM directory : %s \n", home);
    thread_pool = jxta_PG_thread_pool_get((Jxta_PG *)it);
    if (jxta_cache_config_addr_is_single_db(cache_config)) {
        if (group != NULL) {
            peergroup_get_cache_manager(group, &cache_manager);
        }
        if (cache_manager) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Found a cache manager --- go shared\n");
            cache_manager = cm_shared_DB_new(cache_manager, gid_to_use);
        }
    }
    if (!cache_manager) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Didn't find a cache manager --  go new\n");
        cache_manager = cm_new(home, gid_to_use, cache_config, pid_to_use,  thread_pool);
    }
    JXTA_OBJECT_RELEASE(cache_config);
    JXTA_OBJECT_RELEASE(svc);

    peergroup_set_cache_manager((Jxta_PG *) it, cache_manager);
    JXTA_OBJECT_RELEASE(cache_manager);
    /*
     * It is a fatal error (equiv to bad address) to not be given an impl adv
     * so we have an impl adv where to fish for msid. As for name and desc, we
     * can provide bland defaults.
     */
    msid_to_use = jxta_MIA_get_MSID((Jxta_MIA *) impl_adv);

    if (it->name == NULL)
        it->name = jstring_new_2("Anonymous Group");
    if (it->desc == NULL)
        it->desc = jstring_new_2("Nondescript Group");

    /**
     * Getting user-services
     * Note 20051005 mjan: user-services will be started after 
     * hardcoded JXTA-C services. We should fix that and start services
     * according to their IDs as in JXTA-J2SE.
     */
    mia_parm_jstring = jxta_MIA_get_Parm((Jxta_MIA *) impl_adv);
    parm_advs = jxta_advertisement_new();
    jxta_advertisement_parse_charbuffer(parm_advs, jstring_get_string(mia_parm_jstring), jstring_length(mia_parm_jstring));
    JXTA_OBJECT_RELEASE(mia_parm_jstring);
    jxta_advertisement_get_advs(parm_advs, &parm_advs_vector);

    for (i = 0; i < jxta_vector_size(parm_advs_vector); i++) {
        Jxta_MIA *parm_mia;
        Jxta_id *parm_mia_msid;
        JString *parm_mia_msid_jstring;
        JString *mia_code;

        jxta_vector_get_object_at(parm_advs_vector, JXTA_OBJECT_PPTR(&parm_mia), i);
        if (parm_mia != NULL) {
            parm_mia_msid = jxta_MIA_get_MSID(parm_mia);

            if (parm_mia_msid != NULL) {
                jxta_id_to_jstring(parm_mia_msid, &parm_mia_msid_jstring);

                mia_code = jxta_MIA_get_Code((Jxta_MIA *) parm_mia);
                jxta_hashtable_put(it->services_name, jstring_get_string(parm_mia_msid_jstring),
                                   jstring_length(parm_mia_msid_jstring), (Jxta_object *) mia_code);
                JXTA_OBJECT_RELEASE(mia_code);
            }
        }

        if (parm_mia != NULL)
            JXTA_OBJECT_RELEASE(parm_mia);
        if (parm_mia_msid != NULL)
            JXTA_OBJECT_RELEASE(parm_mia_msid);
        if (parm_mia_msid_jstring != NULL)
            JXTA_OBJECT_RELEASE(parm_mia_msid_jstring);
    }
    if (parm_advs_vector != NULL)
        JXTA_OBJECT_RELEASE(parm_advs_vector);
    JXTA_OBJECT_RELEASE(parm_advs);

    /*
     * Build our peer adv which includes only a subset of what a peer adv
     * can hold. This will be the published version.
     */
    it->peer_adv = jxta_PA_new();

    /* Set our name and pid and gid in our peer adv. */
    jxta_PA_set_Name(it->peer_adv, peername_to_use);
    jxta_PA_set_PID(it->peer_adv, pid_to_use);
    jxta_PA_set_GID(it->peer_adv, gid_to_use);


    /* build our group adv
     */
    it->group_adv = jxta_PGA_new();
    jxta_PGA_set_GID(it->group_adv, gid_to_use);
    jxta_PGA_set_Name(it->group_adv, it->name);
    jxta_PGA_set_Desc(it->group_adv, it->desc);
    jxta_PGA_set_MSID(it->group_adv, msid_to_use);

    /* store the impl adv as-is. */
    JXTA_OBJECT_SHARE(impl_adv);
    it->impl_adv = impl_adv;

    /* Now we can get rid of all the intermediate variables */
    JXTA_OBJECT_RELEASE(peername_to_use);
    JXTA_OBJECT_RELEASE(pid_to_use);
    JXTA_OBJECT_RELEASE(gid_to_use);
    JXTA_OBJECT_RELEASE(msid_to_use);
}

/*
 * This routine just inits the modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * Error-returning variant.
 */
Jxta_status jxta_stdpg_init_modules(Jxta_module * self)
{
    Jxta_status res;
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);
    char **keys_of_services_name = NULL;
    Jxta_id *gid;

    /* Load/init all services */

    /*
     * Simplification: so far the C implementation does only
     * support two cases: either the group is the root group and
     * has an endpoint service, or it is a descendant group and
     * borrows the endpoint service from its home group.
     * In addition, the creation of the endpoint by the root group
     * is care of the netpg. So, if we're the root group, we do
     * touch the endpoint. Otherwise, we get a ref from our parent.
     */
    if (it->home_group != NULL) {
        jxta_PG_get_endpoint_service(it->home_group, &(it->endpoint));
    }

    res = stdpg_ld_mod((Jxta_PG *) it, jxta_rendezvous_classid, "builtin:rdv_service", NULL, (Jxta_module **) & it->rendezvous);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res =
        stdpg_ld_mod((Jxta_PG *) it, jxta_resolver_classid, "builtin:resolver_service_ref", NULL,
                     (Jxta_module **) & it->resolver);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res = stdpg_ld_mod((Jxta_PG *) it, jxta_srdi_classid, "builtin:srdi_service_ref", NULL, (Jxta_module **) & it->srdi);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res =
        stdpg_ld_mod((Jxta_PG *) it, jxta_discovery_classid, "builtin:discovery_service_ref", NULL,
                     (Jxta_module **) & it->discovery);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res = stdpg_ld_mod((Jxta_PG *) it, jxta_pipe_classid, "builtin:pipe_service", NULL, (Jxta_module **) & it->pipe);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    res =
        stdpg_ld_mod((Jxta_PG *) it, jxta_membership_classid, "builtin:null_membership_service", NULL,
                     (Jxta_module **) & it->membership);
    if (JXTA_SUCCESS != res) {
        goto ERROR_EXIT;
    }

    /*
       res = stdpg_ld_mod( (Jxta_PG*) it, jxta_peerinfo_classid, "builtin:peerinfo", (Jxta_module**) &it->peerinfo);
       if( JXTA_SUCCESS != res ) {
       goto ERROR_EXIT;
       }
     */

    keys_of_services_name = jxta_hashtable_keys_get(it->services_name);

    if (keys_of_services_name != NULL) {
        unsigned int i = 0;
        Jxta_id *key_id = NULL;
        JString *key_jstring = NULL;
        Jxta_module *module = NULL;
        JString *path_jstring = NULL;
        char *path = NULL;
        char *res, *state;
        char *lib_name = NULL;
        char *constructor_name = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Custom services to load\n");
        while (keys_of_services_name[i]) {
            key_jstring = jstring_new_2(keys_of_services_name[i]);
            jxta_id_from_jstring(&key_id, key_jstring);
            if (key_jstring != NULL) {

                jxta_hashtable_get(it->services_name, keys_of_services_name[i],
                                   strlen(keys_of_services_name[i]), JXTA_OBJECT_PPTR(&path_jstring));
                path = strdup(jstring_get_string(path_jstring));
                JXTA_OBJECT_RELEASE(path_jstring);

                res = apr_strtok(path, "$", &state);
                lib_name = strdup(res);
                res = apr_strtok(NULL, "$", &state);
                constructor_name = strdup(res);
                free(path);

#if APR_HAS_DSO
                module = ld_dso_mod(it, key_id, lib_name, constructor_name, NULL);
#else
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                                "Failed to load: %s, installed APR does not support dynamic shared objects\n",
                                keys_of_services_name[i]);
#endif

                free(lib_name);
                free(constructor_name);

                if (module != NULL) {
                    jxta_hashtable_put(it->services, keys_of_services_name[i],
                                       strlen(keys_of_services_name[i]), (Jxta_object *) module);
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Adding custom service %s\n", keys_of_services_name[i]);
                    JXTA_OBJECT_RELEASE(module);
                } else {
                    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "Failed adding custom service %s\n",
                                    keys_of_services_name[i]);
                }

                JXTA_OBJECT_RELEASE(key_jstring);
                JXTA_OBJECT_RELEASE(key_id);
            }

            free(keys_of_services_name[i]);
            i++;
        }

        free(keys_of_services_name);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "No custom services to load\n");
    }

    /*
     * Now, register with the group registry.
     * If we're redundant, it is a bit late to find out, but not that
     * terrible, and it saves us from a synchronization in get_interface()
     * which we'd need if we were to go public with basic services not
     * hooked-up.
     */
    gid = jxta_PA_get_GID(it->peer_adv);
    res = jxta_register_group_instance(gid, (Jxta_PG *) self);
    JXTA_OBJECT_RELEASE(gid);

    if (res != JXTA_SUCCESS) {
        goto ERROR_EXIT;
    }

    return res;

  ERROR_EXIT:
    /*
     * For now, we assume that what was init'ed needs to be stopped,
     * but really it should not be the case and the outside world
     * must not have to worry about stopping a group that was not
     * even successfully init'ed. Releasing it should be enough.
     *
     * So, call our own stop routine behind the scenes, just to be safe.
     */
    stdpg_stop(self);

    return res;
}

/*
 * start the modules. This is not part of the public API, this is
 * for subclasses which may need a finer grain control on the init sequence.
 * specifically, the opportunity to init additional services before starting
 * ours, and then, their own.
 */
Jxta_status jxta_stdpg_start_modules(Jxta_module * self)
{
    const char *noargs[] = { NULL };
    char **keys_of_services = NULL;
    Jxta_stdpg *it = (Jxta_stdpg *) self;


    /*
     * Note, starting/stopping the endpoint is nolonger our business.
     * Either the endpoint is borrowed from our parent, or it is
     * managed by a subclass (for the root group).
     */
    jxta_module_start((Jxta_module *) (it->rendezvous), noargs);
    jxta_module_start((Jxta_module *) (it->srdi), noargs);
    jxta_module_start((Jxta_module *) (it->resolver), noargs);
    jxta_module_start((Jxta_module *) (it->discovery), noargs);
    jxta_module_start((Jxta_module *) (it->pipe), noargs);
    jxta_module_start((Jxta_module *) (it->membership), noargs);
    /*    jxta_module_start((Jxta_module*) (it->peerinfo), noargs); */

    keys_of_services = jxta_hashtable_keys_get(it->services);

    if (keys_of_services != NULL) {
        unsigned int i = 0;
        Jxta_module *module = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Custom services to start\n");
        while (keys_of_services[i]) {
            Jxta_status res =
                jxta_hashtable_get(it->services, keys_of_services[i], strlen(keys_of_services[i]), JXTA_OBJECT_PPTR(&module));
            if (res == JXTA_SUCCESS) {
                jxta_module_start(module, noargs);
                JXTA_OBJECT_RELEASE(module);
            }
            free(keys_of_services[i]);
            i++;
        }
        free(keys_of_services);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No custom services to start\n");
    }
    return JXTA_SUCCESS;
}

/*
 * The public init routine (does init and start of modules).
 */
static Jxta_status stdpg_init(Jxta_module * self, Jxta_PG * group, Jxta_id * assigned_id, Jxta_advertisement * implAdv)
{
    Jxta_status res = JXTA_SUCCESS;

    peergroup_init((Jxta_PG *) self, group);
    /* Do our general init */
    jxta_stdpg_init_group(self, group, assigned_id, implAdv);

    /* load/initing modules */
    res = jxta_stdpg_init_modules(self);

    if (JXTA_SUCCESS != res) {
        return res;
    }

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Inited.\n");

    return JXTA_SUCCESS;
}

/*
 * implementations for service methods
 */
static void stdpg_get_MIA(Jxta_service * self, Jxta_advertisement ** adv)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (it->impl_adv != NULL)
        JXTA_OBJECT_SHARE(it->impl_adv);
    *adv = it->impl_adv;
}

static void stdpg_get_interface(Jxta_service * self, Jxta_service ** service)
{
    PTValid(self, Jxta_stdpg);

    *service = JXTA_OBJECT_SHARE(self);
}

/*
 * implementations for peergroup methods
 */
static void stdpg_get_loader(Jxta_PG * self, Jxta_loader ** loader)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *loader = NULL;
}

static void stdpg_get_PGA(Jxta_PG * self, Jxta_PGA ** pga)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *pga = JXTA_OBJECT_SHARE(it->group_adv);
}

static void stdpg_get_PA(Jxta_PG * self, Jxta_PA ** pa)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *pa = JXTA_OBJECT_SHARE(it->peer_adv);
}

static
Jxta_status stdpg_add_relay_addr(Jxta_PG * self, Jxta_RdvAdvertisement * relay)
{
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_PA *padv;

    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    route = jxta_PA_add_relay_address(it->peer_adv, relay);
    jxta_PG_get_PA(self, &padv);

    /*
     * Publish locally and remotely the new peer advertisement
     */
    discovery_service_publish(it->discovery, (Jxta_advertisement *) padv, DISC_PEER,
                              (Jxta_expiration_time) PA_EXPIRATION_LIFE, LOCAL_ONLY_EXPIRATION);
    discovery_service_remote_publish(it->discovery, NULL, (Jxta_advertisement *) padv, DISC_PEER,
                                     (Jxta_expiration_time) PA_REMOTE_EXPIRATION);

    /*
     * update our local route
     *
     */
    jxta_endpoint_service_set_local_route(it->endpoint, route);
    JXTA_OBJECT_RELEASE(route);
    JXTA_OBJECT_RELEASE(padv);

    return JXTA_SUCCESS;
}

static Jxta_status stdpg_remove_relay_addr(Jxta_PG * self, Jxta_id * relayid)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    return jxta_PA_remove_relay_address(it->peer_adv, relayid);
}

static Jxta_status stdpg_lookup_service(Jxta_PG * self, Jxta_id * name, Jxta_service ** result)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *name_jstring = NULL;
    Jxta_service *service = NULL;
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (name == NULL)
        return JXTA_FAILED;

    jxta_id_to_jstring(name, &name_jstring);
    res = jxta_hashtable_get(it->services, jstring_get_string(name_jstring),
                             strlen(jstring_get_string(name_jstring)), JXTA_OBJECT_PPTR(&service));

    if (res == JXTA_SUCCESS)
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Successfully retrieved custom service id %s\n",
                        jstring_get_string(name_jstring));
    else
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Failed to retrieve custom service id %s\n",
                        jstring_get_string(name_jstring));

    JXTA_OBJECT_RELEASE(name_jstring);
    *result = service;

    return res;
}

static void stdpg_get_compatstatement(Jxta_PG * self, JString ** compat)
{
    JString *result;
    PTValid(self, Jxta_stdpg);

    /*
     * What this returns is the statement of modules that this
     * group can load, which is not necessarily the same than
     * the compat statement of the module that implements this group.
     * That's why we do not just return what's in this group's MIA.
     * Most often they should be the same, though.
     */
    result = jstring_new_2("<Efmt>" TARGET_ARCH_OS_PLATFORM "</Efmt>" "<Bind>V2.0 C Ref Impl</Bind>");
    *compat = result;
}

static Jxta_boolean stdpg_is_compatible(Jxta_PG * self, JString * compat)
{
    JString *mycomp;
    const char *mp;
    const char *op;

    Jxta_boolean result = TRUE;
    PTValid(self, Jxta_stdpg);

    stdpg_get_compatstatement(self, &mycomp);
    mp = jstring_get_string(mycomp);
    op = jstring_get_string(compat);

    while (*mp != '\0' && *op != '\0') {
        if (isspace(*mp)) {
            ++mp;
            continue;
        }
        if (isspace(*op)) {
            ++op;
            continue;
        }
        if (*mp != *op) {
            result = FALSE;
            break;
        }
        ++op;
        ++mp;
    }

    if (*mp != *op)
        result = FALSE;

    JXTA_OBJECT_RELEASE(mycomp);

    return result;
}

static Jxta_status stdpg_loadfromimpl_module(Jxta_PG * self,
                                             Jxta_id * assigned_id, Jxta_advertisement * impl, Jxta_module ** module)
{
    Jxta_status res;
    Jxta_MIA *impl_adv = (Jxta_MIA *) impl;
    JString *code;

    PTValid(self, Jxta_stdpg);

    code = jxta_MIA_get_Code(impl_adv);

    res = stdpg_ld_mod(self, assigned_id, jstring_get_string(code), impl_adv, module);

    JXTA_OBJECT_RELEASE(code);

    return res;
}

static Jxta_status stdpg_loadfromid_module(Jxta_PG * self,
                                           Jxta_id * assigned_id, Jxta_MSID * spec_id, int where, Jxta_module ** result)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    return JXTA_NOTIMP;
}

static void stdpg_set_labels(Jxta_PG * self, JString * name, JString * description)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    /* These can be set only once. */
    if (it->name == NULL) {
        JXTA_OBJECT_SHARE(name);
        it->name = name;
    }

    if (it->desc == NULL) {
        JXTA_OBJECT_SHARE(description);
        it->desc = description;
    }
}

static void stdpg_get_genericpeergroupMIA(Jxta_PG * self, Jxta_MIA ** mia)
{
    JString *comp;
    JString *code;
    JString *puri;
    JString *prov;
    JString *desc;
    Jxta_MIA *new_mia = NULL;

    /**
     * Note 20051005 mjan: get_netpeergroup_MIA call this function
     * and since there is no peer group available we cannot do that
     * It doesn't hurt since we do nothing with self anyway! 
    PTValid(self, Jxta_stdpg);
     */

    /*
     * The statement that goes in the MIA describes the requirement for
     * being able to load this module. This is not necessarily the same
     * that what kind of modules this group can load. That's what we do
     * not reuse the statement generated by get_compatstatement, even if
     * happens to be the same (as should most often be the case).
     */
    comp = jstring_new_2("<Efmt>" TARGET_ARCH_OS_PLATFORM "</Efmt>" "<Bind>V2.0 C Ref Impl</Bind>");
    code = jstring_new_2("builtin:stdpg");
    puri = jstring_new_2("self");
    prov = jstring_new_2("sun.com");
    desc = jstring_new_2("Generic Peer Group. A native implementation.");

    new_mia = jxta_MIA_new();

    jxta_MIA_set_Comp(new_mia, comp);
    jxta_MIA_set_Code(new_mia, code);
    jxta_MIA_set_PURI(new_mia, puri);
    jxta_MIA_set_Prov(new_mia, prov);
    jxta_MIA_set_Desc(new_mia, desc);

    JXTA_OBJECT_RELEASE(comp);
    JXTA_OBJECT_RELEASE(code);
    JXTA_OBJECT_RELEASE(puri);
    JXTA_OBJECT_RELEASE(prov);
    JXTA_OBJECT_RELEASE(desc);

    jxta_MIA_set_MSID(new_mia, jxta_genericpeergroup_specid);
    *mia = new_mia;
}

static Jxta_status stdpg_newfromimpl(Jxta_PG * self, Jxta_PGID * gid,
                                     Jxta_advertisement * impl, JString * name,
                                     JString * description, Jxta_vector * resource_groups, Jxta_PG ** pg)
{
    Jxta_status res;
    const char *noargs[] = { NULL };
    Jxta_PGA *pga;
    Jxta_PG *g = NULL;
    Jxta_MIA *impl_adv;
    JString *code;
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if ((gid != NULL) && (jxta_lookup_group_instance(gid, pg) == JXTA_SUCCESS)) {
        return JXTA_SUCCESS;
    }

    impl_adv = (Jxta_MIA *) impl;

    code = jxta_MIA_get_Code(impl_adv);

    res = jxta_defloader_instantiate(jstring_get_string(code), (Jxta_module **) & g);
    JXTA_OBJECT_RELEASE(code);
    if (JXTA_SUCCESS != res) {
        return res;
    }

    jxta_PG_set_labels(g, name, description);
    jxta_PG_set_resourcegroups(g, resource_groups);
    res = jxta_module_init((Jxta_module *) g, self, gid, (Jxta_advertisement *) impl_adv);
    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(g);
        return res;
    }
    res = jxta_module_start((Jxta_module *) g, noargs);
    if (JXTA_SUCCESS != res) {
        JXTA_OBJECT_RELEASE(g);
        return res;
    }

    /**
     * Publishing the group advertisement in the parent group with defaults lifetime and expiration as in JXTA-J2SE
     */
    jxta_PG_get_PGA(g, &pga);
    discovery_service_publish(it->discovery, (Jxta_advertisement *) pga, DISC_GROUP, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
    JXTA_OBJECT_RELEASE(pga);

    jxta_service_get_interface((Jxta_service *) g, (Jxta_service **) pg);
    JXTA_OBJECT_RELEASE(g);
    PTValid(self, Jxta_stdpg);

    return JXTA_SUCCESS;
}

static Jxta_status stdpg_newfromadv(Jxta_PG * self, Jxta_advertisement * pgAdv, Jxta_vector * resource_groups, Jxta_PG ** pg)
{
    Jxta_status res;
    Jxta_PGA *pga;
    Jxta_id *msid;
    Jxta_id *gid;
    Jxta_MIA *mia = NULL;
    JString *desc;
    JString *name;

    PTValid(self, Jxta_stdpg);

    pga = (Jxta_PGA *) pgAdv;

    gid = jxta_PGA_get_GID(pga);
    if (jxta_lookup_group_instance(gid, pg) == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(gid);
        return JXTA_SUCCESS;
    }

    msid = jxta_PGA_get_MSID(pga);

    /*
     * We implement ONLY two group msids: 
     * - jxta_ref_netpeergroup_specid.
     * - jxta_genericpeergroup_specid.
     *
     * That's it.
     * The following is a hack so that we do not depend on them being
     * published.
     */
    if (jxta_id_equals(msid, jxta_ref_netpeergroup_specid)) {
        JString *code;
        JString *miadesc;
        stdpg_get_genericpeergroupMIA(self, &mia);

        code = jstring_new_2("builtin:netpg");
        miadesc = jstring_new_2("Net Peer Group native implementation");

        jxta_MIA_set_Code(mia, code);
        jxta_MIA_set_Desc(mia, miadesc);

        JXTA_OBJECT_RELEASE(code);
        JXTA_OBJECT_RELEASE(miadesc);
        jxta_MIA_set_MSID(mia, msid);
    } else if (jxta_id_equals(msid, jxta_genericpeergroup_specid)) {
        stdpg_get_genericpeergroupMIA(self, &mia);
    }
    JXTA_OBJECT_RELEASE(msid);

    if (mia == NULL) {
        JXTA_OBJECT_RELEASE(gid);
        return JXTA_ITEM_NOTFOUND;
    }

    name = jxta_PGA_get_Name(pga);
    desc = jxta_PGA_get_Desc(pga);

    res = stdpg_newfromimpl(self, gid, (Jxta_advertisement *) mia, name, desc, resource_groups, pg);

    JXTA_OBJECT_RELEASE(gid);
    JXTA_OBJECT_RELEASE(name);
    JXTA_OBJECT_RELEASE(desc);
    JXTA_OBJECT_RELEASE(mia);

    return res;
}

static Jxta_status stdpg_newfromid(Jxta_PG * self, Jxta_PGID * gid, Jxta_vector * resource_group, Jxta_PG ** result)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    return JXTA_NOTIMP;
}

static void stdpg_get_rendezvous_service(Jxta_PG * self, Jxta_rdv_service ** rdv)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *rdv = JXTA_OBJECT_SHARE(it->rendezvous);
}

static void stdpg_get_endpoint_service(Jxta_PG * self, Jxta_endpoint_service ** endp)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *endp = JXTA_OBJECT_SHARE(it->endpoint);
}

static void stdpg_get_resolver_service(Jxta_PG * self, Jxta_resolver_service ** resol)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *resol = JXTA_OBJECT_SHARE(it->resolver);
}

static void stdpg_get_discovery_service(Jxta_PG * self, Jxta_discovery_service ** disco)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (it->discovery != NULL) {
        *disco = JXTA_OBJECT_SHARE(it->discovery);
    }
}
static void stdpg_get_srdi_service(Jxta_PG * self, Jxta_srdi_service ** srdio)
{
    Jxta_stdpg *it = (Jxta_stdpg *) self;
    PTValid(self, Jxta_stdpg);

    if (it->srdi != NULL)
        *srdio = JXTA_OBJECT_SHARE(it->srdi);
}

static void stdpg_get_peerinfo_service(Jxta_PG * self, Jxta_peerinfo_service ** peerinfo)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    /* JXTA_OBJECT_SHARE(it->peerinfo); *peerinfo =  it->peerinfo; not yet */

    *peerinfo = NULL;
}

static void stdpg_get_membership_service(Jxta_PG * self, Jxta_membership_service ** memb)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);
    if (it->membership) {
        JXTA_OBJECT_SHARE(it->membership);
    }
    *memb = it->membership;
}

static void stdpg_get_pipe_service(Jxta_PG * self, Jxta_pipe_service ** pipe)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *pipe = JXTA_OBJECT_SHARE(it->pipe);
}

static void stdpg_get_GID(Jxta_PG * self, Jxta_PGID ** gid)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *gid = jxta_PA_get_GID(it->peer_adv);
}

static void stdpg_get_PID(Jxta_PG * self, Jxta_PID ** pid)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *pid = jxta_PA_get_PID(it->peer_adv);
}

static void stdpg_get_cache_manager(Jxta_PG * self, Jxta_cm ** cm)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (it->cm) {
        JXTA_OBJECT_SHARE(it->cm);
    }
    *cm = it->cm;
}

static void stdpg_set_cache_manager(Jxta_PG * self, Jxta_cm * cm)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);
    if (cm) {
        JXTA_OBJECT_SHARE(cm);
    }
    it->cm = cm;
}

static void stdpg_get_groupname(Jxta_PG * self, JString ** nm)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *nm = JXTA_OBJECT_SHARE(it->name);
}

static void stdpg_get_peername(Jxta_PG * self, JString ** nm)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    *nm = jxta_PA_get_Name(it->peer_adv);
}

static void stdpg_get_configadv(Jxta_PG * self, Jxta_PA ** adv)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (it->config_adv != NULL) {
        JXTA_OBJECT_SHARE(it->config_adv);
    }
    *adv = it->config_adv;
}

static void stdpg_get_resourcegroups(Jxta_PG * self, Jxta_vector ** resource_groups)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    /*
     * If there's nothing in-there, and if we have a home_group, add it.
     * It is unclear yet whether the home group should be added always, or
     * only if there's no resource group set. What is supposed to happen
     * if a group is later added to the vector ? Should the home_group
     * go away as soon as there's something in the resource_group list ?
     * For now, it stays.
     */
    if (jxta_vector_size(it->resource_groups) == 0 && it->home_group != NULL) {
        jxta_vector_add_object_last(it->resource_groups, (Jxta_object *) it->home_group);
    }

    *resource_groups = JXTA_OBJECT_SHARE(it->resource_groups);
}

static void stdpg_set_resourcegroups(Jxta_PG * self, Jxta_vector * resource_groups)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    /*
     * We never set resource_groups to NULL. Can be empty, though, until
     * it is first queried.
     */
    if (resource_groups == NULL) {
        resource_groups = jxta_vector_new(1);
    } else {
        JXTA_OBJECT_SHARE(resource_groups);
    }

    JXTA_OBJECT_RELEASE(it->resource_groups);
    it->resource_groups = resource_groups;
}

static void stdpg_get_parentgroup(Jxta_PG * self, Jxta_PG ** parent_group)
{
    Jxta_stdpg *it = PTValid(self, Jxta_stdpg);

    if (it->home_group != NULL) {
        JXTA_OBJECT_SHARE(it->home_group);
    }
    *parent_group = it->home_group;
}

/*
 * Note: The following could be in a .h so that subclassers get a static
 * init block for their own methods table, but then, the methods themselves
 * must be exported too, so that the linker can resolve the items. Could
 * cause name collisions. And a static init block is not very useful
 * if the subclasser wants to override some of the methods. The only
 * alternative is to use thread_once in the subclasse's "new" method and make
 * a copy of the methods table below at runtime to get default values. This
 * table is exported.
 */

const Jxta_stdpg_methods jxta_stdpg_methods = {
    {
     {
      "Jxta_module_methods",
      stdpg_init,
      stdpg_start,
      stdpg_stop},
     "Jxta_service_methods",
     jxta_service_get_MIA_impl,
     jxta_service_get_interface_impl,
     service_on_option_set},
    "Jxta_PG_methods",
    stdpg_get_loader,
    stdpg_get_PGA,
    stdpg_get_PA,
    stdpg_lookup_service,
    stdpg_is_compatible,
    stdpg_loadfromimpl_module,
    stdpg_loadfromid_module,
    stdpg_set_labels,
    stdpg_newfromadv,
    stdpg_newfromimpl,
    stdpg_newfromid,
    stdpg_get_rendezvous_service,
    stdpg_get_endpoint_service,
    stdpg_get_resolver_service,
    stdpg_get_discovery_service,
    stdpg_get_peerinfo_service,
    stdpg_get_membership_service,
    stdpg_get_pipe_service,
    stdpg_get_GID,
    stdpg_get_PID,
    stdpg_get_groupname,
    stdpg_get_peername,
    stdpg_get_configadv,
    stdpg_get_genericpeergroupMIA,
    stdpg_set_resourcegroups,
    stdpg_get_resourcegroups,
    stdpg_get_parentgroup,
    stdpg_get_compatstatement,
    stdpg_add_relay_addr,
    stdpg_remove_relay_addr,
    stdpg_get_srdi_service,
    stdpg_set_cache_manager,
    stdpg_get_cache_manager
};

/*
 * Make sure we have a ctor that subclassers can call.
 */
void jxta_stdpg_construct(Jxta_stdpg * self, Jxta_stdpg_methods const *methods)
{
    PTValid(methods, Jxta_PG_methods);
    jxta_PG_construct((Jxta_PG *) self, (Jxta_PG_methods *) methods);

    self->thisType = "Jxta_stdpg";
    self->resource_groups = jxta_vector_new(1);

    self->services = jxta_hashtable_new(1);
    self->services_name = jxta_hashtable_new(1);
    apr_pool_create(&self->pool, NULL);
    self->services_handle = apr_hash_make(self->pool);
    self->endpoint = NULL;
    self->pipe = NULL;
    self->resolver = NULL;
    self->rendezvous = NULL;
    self->discovery = NULL;
    self->membership = NULL;
    self->peerinfo = NULL;
    self->srdi = NULL;
    self->home_group = NULL;
    self->impl_adv = NULL;
    self->peer_adv = NULL;
    self->group_adv = NULL;
    self->name = NULL;
    self->desc = NULL;
    self->config_adv = NULL;
}

/*
 * The destructor is never called explicitly; it is called
 * by the free function installed by the allocator when the object becomes
 * un-referenced.
 * Therefore, we not do need the destructor to be virtual.
 * The free function plays that role.
 */
void jxta_stdpg_destruct(Jxta_stdpg * self)
{
    char **keys_of_services = NULL;
    PTValid(self, Jxta_stdpg);

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
     * We'll revisit that later; it's not that stopping the stdpg is all that
     * critical at this stage. 
     */

    if (self->endpoint != NULL)
        JXTA_OBJECT_RELEASE(self->endpoint);
    if (self->resolver != NULL)
        JXTA_OBJECT_RELEASE(self->resolver);
    if (self->discovery != NULL)
        JXTA_OBJECT_RELEASE(self->discovery);
    if (self->pipe != NULL)
        JXTA_OBJECT_RELEASE(self->pipe);
    if (self->membership != NULL)
        JXTA_OBJECT_RELEASE(self->membership);
    if (self->rendezvous != NULL)
        JXTA_OBJECT_RELEASE(self->rendezvous);
    if (self->peerinfo != NULL)
        JXTA_OBJECT_RELEASE(self->peerinfo);
    if (self->impl_adv != NULL)
        JXTA_OBJECT_RELEASE(self->impl_adv);
    if (self->config_adv != NULL)
        JXTA_OBJECT_RELEASE(self->config_adv);
    if (self->peer_adv != NULL)
        JXTA_OBJECT_RELEASE(self->peer_adv);
    if (self->group_adv != NULL)
        JXTA_OBJECT_RELEASE(self->group_adv);
    if (self->name != NULL)
        JXTA_OBJECT_RELEASE(self->name);
    if (self->desc != NULL)
        JXTA_OBJECT_RELEASE(self->desc);
    if (self->home_group != NULL)
        JXTA_OBJECT_RELEASE(self->home_group);
    if (self->srdi != NULL)
        JXTA_OBJECT_RELEASE(self->srdi);

    keys_of_services = jxta_hashtable_keys_get(self->services);

    if (keys_of_services != NULL) {

#if APR_HAS_DSO

        int i = 0;
        JString *key_jstring = NULL;
        Jxta_module *module = NULL;
        apr_dso_handle_t *handle = NULL;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Custom services to destroy\n");
        while (keys_of_services[i]) {
            Jxta_status res = JXTA_SUCCESS;

            key_jstring = jstring_new_2(keys_of_services[i]);
            if (key_jstring != NULL) {
                res =
                    jxta_hashtable_del(self->services, jstring_get_string(key_jstring), jstring_length(key_jstring),
                                       JXTA_OBJECT_PPTR(&module));
                if (res == JXTA_SUCCESS) {
                    JXTA_OBJECT_RELEASE(module);
                }

                handle =
                    (apr_dso_handle_t *) apr_hash_get(self->services_handle, (const void *) jstring_get_string(key_jstring),
                                                      jstring_length(key_jstring));
                if (handle != NULL)
                    apr_dso_unload(handle);

                JXTA_OBJECT_RELEASE(key_jstring);
            }
            free(keys_of_services[i]);
            i++;
        }
#endif
        free(keys_of_services);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "No custom services to destroy\n");
    }

    if (self->services != NULL) {
        JXTA_OBJECT_RELEASE(self->services);
    }

    if (self->services_name != NULL) {
        JXTA_OBJECT_RELEASE(self->services_name);
    }

    if (self->cm != NULL) {
        cm_stop(self->cm);
        JXTA_OBJECT_RELEASE(self->cm);
    }

    if (self->pool != 0)
        apr_pool_destroy(self->pool);

    JXTA_OBJECT_RELEASE(self->resource_groups);

    jxta_PG_destruct((Jxta_PG *) self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Destruction finished\n");
}

/*
 * So, how does this "implementation" gets hooked up ?
 * Ideally, some impl adv points at some content that has this piece
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
 * So, an impl adv that designates a builtin class "stdpg" will name the
 * package "builtin:" and the code "stdpg".
 * When asked to instantiate a "builtin:stdpg" the loader will lookup "stdpg"
 * in its built-in table and call the new_instance method that's there.
 */


/*
 * New is not public, but it is exported so that we can hook it to some
 * table.
 */

static void myFree(Jxta_object * obj)
{
    jxta_stdpg_destruct((Jxta_stdpg *) obj);
    free((void *) obj);
}

Jxta_stdpg *jxta_stdpg_new_instance(void)
{
    Jxta_stdpg *self;

    self = (Jxta_stdpg *) calloc(1, sizeof(Jxta_stdpg));
    JXTA_OBJECT_INIT(self, myFree, 0);
    jxta_stdpg_construct(self, &jxta_stdpg_methods);
    return self;
}

/* vim: set ts=4 sw=4 tw=130 et: */
