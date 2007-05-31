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
 * $Id: jxta_stdpg.c,v 1.22 2005/01/26 20:13:22 bondolo Exp $
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

#include <ctype.h>
#include "jpr/jpr_excep.h"
#include "jxta_errno.h"
#include "jxta_debug.h"
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

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED__  __attribute__((__unused__))
#else
#define UNUSED__  /* UNSUSED */
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
 */
static Jxta_module* ld_mod(Jxta_stdpg* self, Jxta_id* class_id,
                           const char* name, Throws) {
    Jxta_module* m;

    ThrowThrough();

    m = jxta_defloader_instantiate_e(name, MayThrow);
    Try {
        jxta_module_init_e(m, (Jxta_PG*)self, class_id, 0, MayThrow);
    } Catch {
        Jxta_status s = jpr_lasterror_get();
        JXTA_LOG("%s failed to initialize (%d)\n", name, s);
        JXTA_OBJECT_RELEASE(m);
        Throw(s);
    }

    JXTA_LOG("%s loaded and initialized\n", name);
    return m;
}

/*
 * Implementations for module methods
 */
static Jxta_PA* read_config(const char* fname) {

    Jxta_PA * ad;
    FILE *advfile;

    ad = jxta_PA_new();
    if (ad == NULL) {
        return NULL;
    }

    advfile = fopen (fname, "r");
    if (advfile == NULL) {
        JXTA_OBJECT_RELEASE(ad);
        return NULL;
    }
    jxta_PA_parse_file(ad,advfile);
    fclose(advfile);
    return ad;
}

static Jxta_status write_config(Jxta_PA* ad, const char* fname) {
    FILE *advfile;
    JString* advs;

    advfile = fopen (fname, "w");
    if (advfile == NULL) {
        return JXTA_IOERR;
    }

    jxta_PA_get_xml(ad, &advs);
    fwrite(jstring_get_string(advs), jstring_length(advs), 1, advfile);
    JXTA_OBJECT_RELEASE(advs);

    fclose(advfile);
    return JXTA_SUCCESS;
}

static Jxta_status stdpg_start(Jxta_module* self, char* args[]) {
    JXTA_LOG( "Started.\n" );

    return JXTA_SUCCESS;
}

static void stdpg_stop(Jxta_module* self) {
    Jxta_id* gid;
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /*
     * Unregister from the group registry if we're in-there.
     */
    gid = jxta_PA_get_GID(it->peer_adv);
    jxta_unregister_group_instance(gid, (Jxta_PG*) self);
    JXTA_OBJECT_RELEASE(gid);

    /*
     * Simplification: so far the C implementation does only
     * support two cases: either the group is the root group and
     * has an endpoint service, or it is a descendant group and
     * borrows the endpoint service from its home group.
     */
    if (it->home_group == NULL) {
        if (it->endpoint != NULL)
            jxta_module_stop((Jxta_module*) (it->endpoint));
    }
    if (it->resolver != NULL)
        jxta_module_stop((Jxta_module*) (it->resolver));
    if (it->rendezvous != NULL)
        jxta_module_stop((Jxta_module*) (it->rendezvous));
    if (it->discovery != NULL)
        jxta_module_stop((Jxta_module*) (it->discovery));
    if (it->pipe != NULL)
        jxta_module_stop((Jxta_module*) (it->pipe));
    if (it->membership != NULL)
        jxta_module_stop((Jxta_module*) (it->membership));
    if (it->peerinfo != NULL)
        jxta_module_stop((Jxta_module*) (it->peerinfo));

    JXTA_LOG("Stopped.\n");
}


/*
 * This routine inits the group but not its modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * We have a lot of alternatives, because as the base pg class, we also normaly
 * serve as the base class for the root group.
 */
void jxta_stdpg_init_group(Jxta_module* self, Jxta_PG* group,
                           Jxta_id* assigned_id,
                           Jxta_advertisement* impl_adv) {
    JString* peername_to_use = NULL;
    Jxta_id* pid_to_use = NULL;
    Jxta_id* gid_to_use = NULL;
    Jxta_id* msid_to_use = NULL;

    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /* The assigned ID is supposed to be the grp ID.
     * if we're the root group we may not have been given one, though.
     */
    if (   (assigned_id == NULL)
            || jxta_id_equals(assigned_id, jxta_id_nullID)) {
        /*
         * We weren't given a gid.
         * assume this is a new grp being created and make a new gid.
         */
        jxta_id_peergroupid_new_1( &gid_to_use );
    } else {
        gid_to_use = assigned_id;
        JXTA_OBJECT_SHARE(gid_to_use);
    }

    /*
     * If we do not have a home group, we serve as the root group.
     * Do what this implies.
     */
    if (group == NULL) {
        it->config_adv = read_config("PlatformConfig");
        if (it->config_adv == NULL) {
            Jxta_id * pid = NULL;
	    Jxta_TCPTransportAdvertisement* tta = NULL;		/* append */
            Jxta_HTTPTransportAdvertisement* hta = NULL;
	    Jxta_RelayAdvertisement* rla = NULL;

	    Jxta_svc* tcpsvc;				/* append */
            Jxta_svc* htsvc;
            Jxta_svc* rdvsvc;
	    Jxta_svc* rlsvc;
	    
	    JString* tcp_proto;				/* append */
            JString* http_proto;
            JString* def_rtr;
            JString* def_rdv;
            JString* man_confmod;
            JString* def_name;
            Jxta_vector* rdvs;
            Jxta_vector* rtl;
            Jxta_vector* services;

	    tcp_proto = jstring_new_2("tcp");		/* append */

            http_proto = jstring_new_2("http");
            def_rtr = jstring_new_2("127.0.0.1:9700");
            def_rdv = jstring_new_2("http://127.0.0.1:9700");
            man_confmod = jstring_new_2("manual");
            def_name = jstring_new_2("JXTA-C Peer");

	    tta = jxta_TCPTransportAdvertisement_new();	/* append */
            hta = jxta_HTTPTransportAdvertisement_new();
	    rla = jxta_RelayAdvertisement_new();

	    tcpsvc = jxta_svc_new();
            htsvc = jxta_svc_new();
            rdvsvc = jxta_svc_new();
	    rlsvc = jxta_svc_new();

            rdvs = jxta_vector_new(1);
	    
	    /* append from here */
	    jxta_TCPTransportAdvertisement_set_Protocol(tta, tcp_proto);
	    jxta_TCPTransportAdvertisement_set_InterfaceAddress(tta, htonl(0x7f000001));
	    jxta_TCPTransportAdvertisement_set_Port(tta, 9701);
	    jxta_TCPTransportAdvertisement_set_ConfigMode(tta, jstring_new_2("manual"));
	    jxta_TCPTransportAdvertisement_set_ServerOff(tta, FALSE);
	    jxta_TCPTransportAdvertisement_set_ClientOff(tta, FALSE);
	    jxta_TCPTransportAdvertisement_set_MulticastAddr(tta, htonl(0xE0000155));
	    jxta_TCPTransportAdvertisement_set_MulticastPort(tta, 1234);
	    jxta_TCPTransportAdvertisement_set_MulticastSize(tta, 16384);
	    jxta_TCPTransportAdvertisement_set_MulticastOff(tta, TRUE);
            jxta_svc_set_TCPTransportAdvertisement(tcpsvc, tta);
            jxta_svc_set_MCID(tcpsvc, jxta_tcpproto_classid);

            jxta_HTTPTransportAdvertisement_set_Protocol(hta, http_proto);
            jxta_HTTPTransportAdvertisement_set_InterfaceAddress(hta,
                    htonl(0x7f000001));
            jxta_HTTPTransportAdvertisement_set_ConfigMode(hta, man_confmod);
            jxta_HTTPTransportAdvertisement_set_Port(hta, 9700);
            jxta_HTTPTransportAdvertisement_set_ServerOff(hta, TRUE);
            jxta_HTTPTransportAdvertisement_set_ProxyOff(hta, TRUE);

            rtl = jxta_HTTPTransportAdvertisement_get_Router(hta);
            jxta_vector_add_object_last(rtl, (Jxta_object*) def_rtr);
            jxta_svc_set_HTTPTransportAdvertisement(htsvc, hta);
            jxta_svc_set_MCID(htsvc, jxta_httpproto_classid);

	    jxta_RelayAdvertisement_set_IsClient(rla, jstring_new_2("true"));
	    jxta_RelayAdvertisement_set_IsServer(rla, jstring_new_2("false"));
	    rtl = jxta_RelayAdvertisement_get_HttpRelay(rla);
            jxta_vector_add_object_last(rtl, (Jxta_object*) def_rtr);
            jxta_svc_set_RelayAdvertisement(rlsvc, rla);
            jxta_svc_set_MCID(rlsvc, jxta_relayproto_classid);

            jxta_vector_add_object_last(rdvs, (Jxta_object*) def_rdv);
            jxta_svc_set_Addr(rdvsvc, rdvs);
            jxta_svc_set_MCID(rdvsvc, jxta_rendezvous_classid);

            it->config_adv = jxta_PA_new();
            services = jxta_PA_get_Svc(it->config_adv);
            jxta_vector_add_object_last(services, (Jxta_object*) rdvsvc);
	    jxta_vector_add_object_last(services, (Jxta_object*) tcpsvc);	/* append */
            jxta_vector_add_object_last(services, (Jxta_object*) htsvc);
            jxta_vector_add_object_last(services, (Jxta_object*) rlsvc);

            jxta_id_peerid_new_1( &pid, gid_to_use );
            jxta_PA_set_PID(it->config_adv, pid);
            jxta_PA_set_Name(it->config_adv, def_name);

            write_config(it->config_adv, "PlatformConfig");
            printf("A new configuration file has been output to "
                   "PlatformConfig.\n");
            printf("Please edit relevant elements before starting Jxta "
                   "again.\n");
            exit(0);
        }

        /*
         * We are the root group, and we have an initialized config adv.
         * Therefore that's were our PID and name are.
         */
        pid_to_use = jxta_PA_get_PID(it->config_adv);
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
        jxta_PG_get_configadv(group, &(it->config_adv));
        */
        jxta_PG_get_PID(group, &(pid_to_use));
        jxta_PG_get_peername(group, &(peername_to_use));
    }

    /*
     * It is a fatal error (equiv to bad address) to not be given an impl adv
     * so we have an impl adv where to fish for msid. As for name and desc, we
     * can provide bland defaults.
     */
    msid_to_use = jxta_MIA_get_MSID((Jxta_MIA*) impl_adv);
    if (it->name == NULL)
        it->name = jstring_new_2("Anonymous Group");
    if (it->desc == NULL)
        it->desc = jstring_new_2("Nondescript Group");

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

    /* create the hash table of services */
    it->services = jxta_hashtable_new(8);
}

/*
 * This routine just inits the modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * Exception-throwing variant.
 */
void jxta_stdpg_init_modules_e(Jxta_module* self, Throws) {
    Jxta_id* gid;
    Jxta_status res;
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /* Load/init all services */
    Try {
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

        it->rendezvous = (Jxta_rdv_service*)
                         ld_mod(it, jxta_rendezvous_classid,
                                "builtin:rdv_service", MayThrow);

        it->resolver = (Jxta_resolver_service*)
                       ld_mod(it, jxta_resolver_classid,
                              "builtin:resolver_service_ref", MayThrow);

        it->discovery = (Jxta_discovery_service*)
                        ld_mod(it, jxta_discovery_classid,
                               "builtin:discovery_service_ref", MayThrow);

        it->pipe = (Jxta_pipe_service*)
                   ld_mod(it, jxta_pipe_classid, "builtin:pipe_service", MayThrow);


        it->membership = (Jxta_membership_service*)
                         ld_mod(it, jxta_membership_classid,
                                "builtin:null_membership_service", MayThrow);

        /*
        it->peerinfo = (Jxta_peerinfo_service*)
         ld_mod(it, jxta_peerinfo_classid, "builtin:peerinfo", MayThrow);
        */

        /*
         * Now, register with the group registry.
         * If we're redundant, it is a bit late to find out, but not that
         * terrible, and it saves us from a synchronization in get_interface()
         * which we'd need if we were to go public with basic services not
         * hooked-up.
         */
        gid = jxta_PA_get_GID(it->peer_adv);
        res = jxta_register_group_instance(gid, (Jxta_PG*) self);
        JXTA_OBJECT_RELEASE(gid);

        if (res != JXTA_SUCCESS) {
        Throw(res);
        }

    } Catch {
        /*
         * For now, we assume that what was init'ed needs to be stopped,
         * but realy it should not be the case and the outside world
         * must not have to worry about stopping a group that was not
         * even successfully init'ed. Releasing it should be enough.
         *
         * So, call our own stop routine behind the scenes, just to be safe.
         */
        stdpg_stop(self);

        /*
         * The rest can be done all right by the dtor when/if released.
         */
        Throw(jpr_lasterror_get());
    }
}

/*
 * This routine just init the modules.
 * This is not part of the public API, this is for subclasses which may
 * need a finer grain control on the init sequence: specifically, they may
 * need to init additional services before the lot of them are started.
 *
 * Error-returning variant.
 */
Jxta_status jxta_stdpg_init_modules(Jxta_module* self) {
    PTValid(self, Jxta_stdpg);
    Try {
        jxta_stdpg_init_modules_e(self, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }
    return JXTA_SUCCESS;
}

/*
 * start the modules. This is not part of the public API, this is
 * for subclasses which may need a finer grain control on the init sequence.
 * specifically, the opportunity to init additional services before starting
 * ours, and then, their own.
 */
void jxta_stdpg_start_modules(Jxta_module* self) {
    char* noargs[] = { NULL };

    Jxta_stdpg* it = (Jxta_stdpg*) self;


    /*
     * Note, starting/stopping the endpoint is nolonger our business.
     * Either the endpoint is borrowed from our parent, or it is
     * managed by a subclass (for the root group).
     */
    jxta_module_start((Jxta_module*) (it->rendezvous), noargs);
    jxta_module_start((Jxta_module*) (it->resolver), noargs);
    jxta_module_start((Jxta_module*) (it->discovery), noargs);
    jxta_module_start((Jxta_module*) (it->pipe), noargs);
    jxta_module_start((Jxta_module*) (it->membership), noargs);
    /*    jxta_module_start((Jxta_module*) (it->peerinfo), noargs); */

}

/*
 * The public init routine (does init and start of modules).
 */
static void stdpg_init_e(Jxta_module* self, Jxta_PG* group,
                         Jxta_id* assigned_id,
                         Jxta_advertisement* impl_adv,
                         Throws) {
    ThrowThrough();

    /* Do our general init */
    jxta_stdpg_init_group(self, group, assigned_id, impl_adv);

    /* load/initing modules */
    jxta_stdpg_init_modules_e(self, MayThrow);

    /* Now, start the services */
    jxta_stdpg_start_modules(self);
}

/*
 * The public init routine (does init and start of modules).
 */
static Jxta_status stdpg_init(Jxta_module* self, Jxta_PG* group,
                              Jxta_id* assigned_id,
                              Jxta_advertisement* implAdv) {
    Try {
        stdpg_init_e(self, group, assigned_id, implAdv, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }
    return JXTA_SUCCESS;
}

/*
 * implementations for service methods
 */
static void stdpg_get_MIA(Jxta_service* self,  Jxta_advertisement** adv) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    if (it->impl_adv != NULL)
        JXTA_OBJECT_SHARE(it->impl_adv);
    *adv = it->impl_adv;
}

static void stdpg_get_interface(Jxta_service* self, Jxta_service** service) {
    PTValid(self, Jxta_stdpg);

    JXTA_OBJECT_SHARE(self);
    *service = self;
}

/*
 * implementations for peergroup methods
 */
static void stdpg_get_loader(Jxta_PG* self, Jxta_loader** loader) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *loader = NULL;
}

static void stdpg_get_PGA(Jxta_PG* self, Jxta_PGA** pga) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    JXTA_OBJECT_SHARE(it->group_adv);
    *pga = it->group_adv;
}

static void stdpg_get_PA(Jxta_PG* self, Jxta_PA** pa) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    JXTA_OBJECT_SHARE(it->peer_adv);
    *pa = it->peer_adv;
}

static 
Jxta_status stdpg_add_relay_addr(Jxta_PG* self, Jxta_RdvAdvertisement* relay) {
  Jxta_RouteAdvertisement* route = NULL;
  Jxta_PA* padv;

  Jxta_stdpg* it = (Jxta_stdpg*) self;

  PTValid(self, Jxta_stdpg);

  route = jxta_PA_add_relay_address(it->peer_adv, relay);
  jxta_PG_get_PA(self, &padv);

  /*
   * Publish locally and remotly the new peer advertisement
   */
  discovery_service_publish(it->discovery, (Jxta_advertisement* ) padv, DISC_PEER,
			    (Jxta_expiration_time) PA_EXPIRATION_LIFE, (Jxta_expiration_time) PA_REMOTE_EXPIRATION);
  discovery_service_remote_publish(it->discovery, NULL, (Jxta_advertisement* ) padv, DISC_PEER,
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

static Jxta_status stdpg_remove_relay_addr(Jxta_PG* self, Jxta_id* relayid) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    return jxta_PA_remove_relay_address(it->peer_adv, relayid);
}

static Jxta_status stdpg_lookup_service(Jxta_PG* self, Jxta_id* name,
                                        Jxta_service** result) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    return JXTA_NOTIMP;
}

static void stdpg_lookup_service_e(Jxta_PG* self, Jxta_id* name,
                                   Jxta_service** svce, Throws) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    Throw(JXTA_NOTIMP);
}

static void stdpg_get_compatstatement(Jxta_PG* self,
                                      JString** compat) {
    JString* result;
    PTValid(self, Jxta_stdpg);

    /*
     * What this returns is the statement of modules that this
     * group can load, which is not necessarily the same than
     * the compat statement of the module that implements this group.
     * That's why we do not just return what's in this group's MIA.
     * Most often they should be the same, though.
     */
    result = jstring_new_2("<Efmt>"
                           TARGET_ARCH_OS_PLATFORM
                           "</Efmt>"
                           "<Bind>V2.0 C Ref Impl</Bind>");
    *compat = result;
}

static Jxta_boolean stdpg_is_compatible(Jxta_PG* self, JString* compat) {
    JString* mycomp;
    const char* mp;
    const char* op;

    Jxta_boolean result;
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

static void stdpg_loadfromimpl_module_e(Jxta_PG* self,
                                        Jxta_id* assigned_id,
                                        Jxta_advertisement* impl,
                                        Jxta_module** mod,
                                        Throws) {
    Jxta_MIA* impl_adv;
    JString* code;

    ThrowThrough();

    PTValid(self, Jxta_stdpg);

    impl_adv = (Jxta_MIA*) impl;

    /*
     * Looks a bit aerobatic but fine: granted that the impl adv
     * will not be released on us, there's no reason for the code
     * element to go away. So we can release it before we use it.
     */
    code = jxta_MIA_get_Code(impl_adv);
    JXTA_OBJECT_RELEASE(code);

    *mod = ld_mod((Jxta_stdpg*) self, assigned_id,
                  jstring_get_string(code), MayThrow);
}

static Jxta_status stdpg_loadfromimpl_module(Jxta_PG* self,
        Jxta_id* assigned_id,
        Jxta_advertisement* impl,
        Jxta_module** result) {
    PTValid(self, Jxta_stdpg);

    Try {
        stdpg_loadfromimpl_module_e(self, assigned_id, impl, result, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }

    return JXTA_SUCCESS;
}

static Jxta_status stdpg_loadfromid_module(Jxta_PG* self,
        Jxta_id* assigned_id,
        Jxta_MSID* spec_id,
        int where, Jxta_module** result) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    return JXTA_NOTIMP;
}

static void stdpg_loadfromid_module_e(Jxta_PG* self,
                                      Jxta_id* assigned_id,
                                      Jxta_MSID* spec_id, int where,
                                      Jxta_module** mod,
                                      Throws) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    Throw(JXTA_NOTIMP);
}

static void stdpg_set_labels(Jxta_PG* self, JString* name,
                             JString* description) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

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

static void stdpg_get_genericpeergroupMIA(Jxta_PG* self, Jxta_MIA** mia) {
    JString* comp;
    JString* code;
    JString* puri;
    JString* prov;
    JString* desc;
    Jxta_MIA* new_mia = NULL;

    PTValid(self, Jxta_stdpg);

    /*
     * The statement that goes in the MIA describes the requirement for
     * being able to load this module. This is not necessarily the same
     * that what kind of modules this group can load. That's what we do
     * not reuse the statement generated by get_compatstatement, even if
     * happens to be the same (as should most often be the case).
     */
    comp = jstring_new_2("<Efmt>"
                         TARGET_ARCH_OS_PLATFORM
                         "</Efmt>"
                         "<Bind>V2.0 C Ref Impl</Bind>");
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

static void stdpg_newfromimpl_e(Jxta_PG* self, Jxta_PGID* gid,
                                Jxta_advertisement* impl, JString* name,
                                JString* description,
                                Jxta_vector* resource_groups,
                                Jxta_PG** pg,
                                Throws) {
    Jxta_PGA* pga;
    Jxta_PG* g;
    Jxta_MIA* impl_adv;
    JString* code;

    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    if (   (gid != NULL)
            && (jxta_lookup_group_instance(gid, pg) == JXTA_SUCCESS)) {
        return;
    }

    impl_adv = (Jxta_MIA*) impl;

    code = jxta_MIA_get_Code(impl_adv);

    Try {
        g = (Jxta_PG*) jxta_defloader_instantiate_e(jstring_get_string(code),
                MayThrow);
        jxta_PG_set_labels(g, name, description);
        jxta_PG_set_resourcegroups(g, resource_groups);
        jxta_module_init_e((Jxta_module*) g, self, gid,
                           (Jxta_advertisement*) impl_adv, MayThrow);

    } Catch {
        Jxta_status s = jpr_lasterror_get();
        JXTA_LOG("%s failed to load/init (%d)\n", jstring_get_string(code), s);
        JXTA_OBJECT_RELEASE(code);
        JXTA_OBJECT_RELEASE(g);
        Throw(s);
    }

    /*
     * FIXME: jice@jxta.org 20020404 - this is probably an abuse but I just
     * want the group published somewhere for testing purposes.
     * (hence the short life time).
     */
    jxta_PG_get_PGA(g, &pga);
    discovery_service_publish(it->discovery, (Jxta_advertisement*) pga,
                              DISC_GROUP, 120000, 10000);
    JXTA_OBJECT_RELEASE(pga);

    jxta_service_get_interface((Jxta_service*) g, (Jxta_service**) pg);
    JXTA_OBJECT_RELEASE(g);

    JXTA_OBJECT_RELEASE(code);
}

static Jxta_status stdpg_newfromimpl(Jxta_PG* self, Jxta_PGID* gid,
                                     Jxta_advertisement* impl, JString* name,
                                     JString* description,
                                     Jxta_vector* resource_groups,
                                     Jxta_PG** result) {

    PTValid(self, Jxta_stdpg);

    Try {
        stdpg_newfromimpl_e(self, gid, impl, name, description,
                            resource_groups, result, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }

    return JXTA_SUCCESS;
}

static void stdpg_newfromadv_e(Jxta_PG* self,
                               Jxta_advertisement* pgAdv,
                               Jxta_vector* resource_groups,
                               Jxta_PG** pg,
                               Throws) {
    Jxta_PGA* pga;
    Jxta_id* msid;
    Jxta_id* gid;
    Jxta_MIA* mia = NULL;
    JString* desc;
    JString* name;

    PTValid(self, Jxta_stdpg);

    pga = (Jxta_PGA*) pgAdv;

    gid = jxta_PGA_get_GID(pga);
    if (jxta_lookup_group_instance(gid, pg) == JXTA_SUCCESS) {
        JXTA_OBJECT_RELEASE(gid);
        return;
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
        JString* code;
        JString* desc;
        stdpg_get_genericpeergroupMIA(self, &mia);

        code = jstring_new_2("builtin:netpg");
        desc = jstring_new_2("Net Peer Group native implementation");

        jxta_MIA_set_Code(mia, code);
        jxta_MIA_set_Desc(mia, desc);

        JXTA_OBJECT_RELEASE(code);
        JXTA_OBJECT_RELEASE(desc);
        jxta_MIA_set_MSID(mia, msid);
    } else if (jxta_id_equals(msid, jxta_genericpeergroup_specid)) {
        stdpg_get_genericpeergroupMIA(self, &mia);
    }
    JXTA_OBJECT_RELEASE(msid);

    if (mia == NULL) {
        JXTA_OBJECT_RELEASE(gid);
        Throw(JXTA_ITEM_NOTFOUND);
    }

    name = jxta_PGA_get_Name(pga);
    desc = jxta_PGA_get_Desc(pga);

    Try {
        stdpg_newfromimpl_e(self, gid,
                            (Jxta_advertisement*) mia,
                            name, desc, resource_groups, pg, MayThrow);
    } Catch {

        JXTA_OBJECT_RELEASE(gid);
        JXTA_OBJECT_RELEASE(name);
        JXTA_OBJECT_RELEASE(desc);
        JXTA_OBJECT_RELEASE(mia);
        Throw(jpr_lasterror_get());
    }

    JXTA_OBJECT_RELEASE(gid);
    JXTA_OBJECT_RELEASE(name);
    JXTA_OBJECT_RELEASE(desc);
    JXTA_OBJECT_RELEASE(mia);
}

static Jxta_status stdpg_newfromadv(Jxta_PG* self, Jxta_advertisement* pgAdv,
                                    Jxta_vector* resource_groups,
                                    Jxta_PG** result) {
    PTValid(self, Jxta_stdpg);

    Try {
        stdpg_newfromadv_e(self, pgAdv, resource_groups, result, MayThrow);
    } Catch {
        return jpr_lasterror_get();
    }

    return JXTA_SUCCESS;
}

static void stdpg_newfromid_e(Jxta_PG* self, Jxta_PGID* gid,
                              Jxta_vector* resource_group,
                              Jxta_PG** pg,
                              Throws) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    Throw(JXTA_NOTIMP);
}

static Jxta_status stdpg_newfromid(Jxta_PG* self, Jxta_PGID* gid,
                                   Jxta_vector* resource_group,
                                   Jxta_PG** result) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    return JXTA_NOTIMP;
}

static void stdpg_get_rendezvous_service(Jxta_PG* self, Jxta_rdv_service** rdv) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *rdv = JXTA_OBJECT_SHARE(it->rendezvous);
}

static void stdpg_get_endpoint_service(Jxta_PG* self,
                                       Jxta_endpoint_service** endp) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *endp = JXTA_OBJECT_SHARE(it->endpoint);
}

static void stdpg_get_resolver_service(Jxta_PG* self,
                                       Jxta_resolver_service** resol) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *resol = JXTA_OBJECT_SHARE(it->resolver);
}

static void stdpg_get_discovery_service(Jxta_PG* self,
                                        Jxta_discovery_service** disco) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);
    
    if (it->discovery != NULL)
      *disco = JXTA_OBJECT_SHARE(it->discovery);
}

static void stdpg_get_peerinfo_service(Jxta_PG* self,
                                       Jxta_peerinfo_service** peerinfo) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /* JXTA_OBJECT_SHARE(it->peerinfo); *peerinfo =  it->peerinfo; not yet */

    *peerinfo = NULL;
}

static void stdpg_get_membership_service(Jxta_PG* self,
        Jxta_membership_service** memb) {
    Jxta_stdpg* UNUSED__ it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *memb = JXTA_OBJECT_SHARE(it->membership);
}

static void stdpg_get_pipe_service(Jxta_PG* self, Jxta_pipe_service** pipe) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *pipe = JXTA_OBJECT_SHARE(it->pipe);
}

static void stdpg_get_GID(Jxta_PG* self, Jxta_PGID** gid) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *gid = jxta_PA_get_GID(it->peer_adv);
}

static void stdpg_get_PID(Jxta_PG* self, Jxta_PID** pid) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *pid = jxta_PA_get_PID(it->peer_adv);
}

static void stdpg_get_groupname(Jxta_PG* self, JString** nm) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *nm = JXTA_OBJECT_SHARE(it->name);
}

static void stdpg_get_peername(Jxta_PG* self, JString** nm) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    *nm = jxta_PA_get_Name(it->peer_adv);
}

static void stdpg_get_configadv(Jxta_PG* self, Jxta_PA** adv) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    if (it->config_adv != NULL) {
        JXTA_OBJECT_SHARE(it->config_adv);
    }
    *adv = it->config_adv;
}

static void stdpg_get_resourcegroups(Jxta_PG* self,
                                     Jxta_vector** resource_groups) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /*
     * If there's nothing in-there, and if we have a home_group, add it.
     * It is unclear yet whether the home group should be added always, or
     * only if there's no resource group set. What is supposed to happen
     * if a group is later added to the vector ? Should the home_group
     * go away as soon as there's something in the resource_group list ?
     * For now, it stays.
     */
    if (jxta_vector_size(it->resource_groups) == 0 && it->home_group != NULL) {
        jxta_vector_add_object_last(it->resource_groups,
                                    (Jxta_object*) it->home_group);
    }

    *resource_groups = JXTA_OBJECT_SHARE(it->resource_groups);
}

static void stdpg_set_resourcegroups(Jxta_PG* self,
                                     Jxta_vector* resource_groups) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    /*
     * We never set resource_groups to NULL. Can be empty, though, until
     * it is first queried.
     */
    if (resource_groups == NULL) {
        resource_groups =  jxta_vector_new(1);
    } else {
        JXTA_OBJECT_SHARE(resource_groups);
    }

    JXTA_OBJECT_RELEASE(it->resource_groups);
    it->resource_groups = resource_groups;
}

static void stdpg_get_parentgroup(Jxta_PG* self,
                                  Jxta_PG** parent_group) {
    Jxta_stdpg* it = (Jxta_stdpg*) self;
    PTValid(self, Jxta_stdpg);

    if (it->home_group != NULL) {
        JXTA_OBJECT_SHARE(it->home_group);
    }
    *parent_group = it->home_group;
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

Jxta_stdpg_methods jxta_stdpg_methods = {
                                            {
                                                {
                                                    "Jxta_module_methods",
                                                    stdpg_init,
                                                    stdpg_init_e,
                                                    stdpg_start,
                                                    stdpg_stop
                                                },
                                                "Jxta_service_methods",
                                                stdpg_get_MIA,
                                                stdpg_get_interface
                                            },
                                            "Jxta_PG_methods",
                                            stdpg_get_loader,
                                            stdpg_get_PGA,
                                            stdpg_get_PA,
                                            stdpg_lookup_service,
                                            stdpg_lookup_service_e,
                                            stdpg_is_compatible,
                                            stdpg_loadfromimpl_module,
                                            stdpg_loadfromimpl_module_e,
                                            stdpg_loadfromid_module,
                                            stdpg_loadfromid_module_e,
                                            stdpg_set_labels,
                                            stdpg_newfromadv,
                                            stdpg_newfromadv_e,
                                            stdpg_newfromimpl,
                                            stdpg_newfromimpl_e,
                                            stdpg_newfromid,
                                            stdpg_newfromid_e,
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
					    stdpg_remove_relay_addr
                                        };

/*
 * Make sure we have a ctor that subclassers can call.
 */
void jxta_stdpg_construct(Jxta_stdpg* self,
                          Jxta_stdpg_methods* methods) {
    PTValid(methods, Jxta_PG_methods);
    jxta_PG_construct((Jxta_PG*) self, (Jxta_PG_methods*) methods);

    self->thisType = "Jxta_stdpg";
    self->resource_groups = jxta_vector_new(1);

    self->services = NULL;
    self->endpoint = NULL;
    self->pipe = NULL;
    self->resolver = NULL;
    self->rendezvous = NULL;
    self->discovery = NULL;
    self->membership = NULL;
    self->peerinfo = NULL;
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
void jxta_stdpg_destruct(Jxta_stdpg* self) {
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
    if (self->services != NULL)
        JXTA_OBJECT_RELEASE(self->services);
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

    JXTA_OBJECT_RELEASE(self->resource_groups);

    jxta_PG_destruct((Jxta_PG*) self);

    JXTA_LOG("Destruction finished\n");
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
 * So, an impl adv that designates a builtin class "stdpg" will name the
 * package "builtin:" and the code "stdpg".
 * When asked to instantiate a "builtin:stdpg" the loader will lookup "stdpg"
 * in its built-in table and call the new_instance method that's there.
 */


/*
 * New is not public, but it is exported so that we can hook it to some
 * table.
 */

static void myFree (Jxta_object* obj) {
    jxta_stdpg_destruct((Jxta_stdpg*) obj);
    free((void*) obj);
}

Jxta_stdpg* jxta_stdpg_new_instance(void) {
    Jxta_stdpg* self = (Jxta_stdpg*) malloc(sizeof(Jxta_stdpg));
    JXTA_OBJECT_INIT(self, myFree, 0);
    jxta_stdpg_construct(self, &jxta_stdpg_methods);
    return self;
}

