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
 * $Id: jxta_peerview.c,v 1.18 2005/09/01 03:27:11 slowhog Exp $
 */

/**
 * This is the implementation of JxtaRdvService struc.
 * This definition must be set before including jxta.h.
 * This is why reference to other JXTA types must be declared.
 **/

static const char *__log_cat = "PV";

#include "jxtaapr.h"

#include "jxta_peerview.h"

#include "jxta_errno.h"
#include "jxta_log.h"
#include "jxta_hashtable.h"
#include "jxta_listener.h"
#include "jxta_rdv_service.h"
#include "jxta_peer_private.h"
#include "jxta_svc.h"
#include "jxta_rdv_service_private.h"

const char JXTA_PEERVIEW_NAME[] = "PeerView";
const char JXTA_PEERVIEW_EDGE_ELEMENT_NAME[] = "PeerView.EdgePeer";
const char JXTA_PEERVIEW_RDVADV_ELEMENT_NAME[] = "PeerView.PeerAdv";
const char JXTA_PEERVIEW_RDVRESP_ELEMENT_NAME[] = "PeerView.PeerAdv.Response";
const char JXTA_PEERVIEW_FAILURE_ELEMENT_NAME[] = "PeerView.Failure";
const char JXTA_PEERVIEW_CACHED_ELEMENT_NAME[] = "PeerView.Cached";

const unsigned int HAPPY_PEERVIEW_SIZE = 4;

const apr_interval_time_t JXTA_PEERVIEW_PERIODIC_INTERVAL = ((apr_interval_time_t) 30) * 1000 * 1000;
const apr_interval_time_t JXTA_PEERVIEW_PVE_EXPIRIRATION = ((apr_interval_time_t) 20) * 60 * 1000 * 1000;

const Jxta_time_diff RDVA_REFRESH_INTERVAL = ((Jxta_time_diff) 30) * 1000 * 1000;
const Jxta_time_diff SEEDING_LOAD_INTERVAL = ((Jxta_time_diff) 20) * 60 * 1000 * 1000;

/**
 * Internal structure of a peer used by this service.
 **/
struct _jxta_peer_peerview_entry {
    Extends(_jxta_peer_entry);

    Jxta_time last_refresh;

    Jxta_RdvAdvertisement *rdva;
};

typedef struct _jxta_peer_peerview_entry _jxta_peer_peerview_entry;

struct _jxta_peerview_entry_methods {
    Extends(Jxta_Peer_entry_methods);

    /* additional methods */
};

typedef struct _jxta_peerview_entry_methods _jxta_peerview_entry_methods;


/**
*   Our fields.
**/
struct _jxta_peerview {
    Extends(Jxta_object);

    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;

    volatile Jxta_boolean running;
    apr_thread_cond_t *periodicCond;
    apr_thread_mutex_t *periodicMutex;
    volatile apr_thread_t *periodicThread;

    Jxta_PG *group;
    Jxta_PG *parentgroup;
    Jxta_PGID *gid;
    Jxta_PGID *parentgid;
    Jxta_PGID *pid;
    JString *groupUniqueID;

    JString *name;

    /* Configuration */
    Jxta_boolean useOnlySeeds;

    /* State */
    Jxta_endpoint_service *endpoint;
    Jxta_discovery_service *discovery;

    /** Pipe service of the *PARENT* peergroup **/
    Jxta_pipe_service *pipes;
    Jxta_pipe *advPipe;

    Jxta_listener *listener_peerview;

    Jxta_hashtable *localView;
    Jxta_vector *localViewOrder;
    _jxta_peer_peerview_entry *downPVE;
    _jxta_peer_peerview_entry *selfPVE;
    _jxta_peer_peerview_entry *upPVE;

    /* Seeding */
    Jxta_vector *seeding;
    Jxta_time last_seeding_update;
    Jxta_vector *seeds;
    Jxta_vector *activeSeeds;
};


typedef struct _jxta_peerview _jxta_peerview_mutable;

static _jxta_peerview_mutable *peerview_construct(_jxta_peerview_mutable * self);
static void peerview_destruct(_jxta_peerview_mutable * self);
static void peerview_delete(Jxta_object * addr);

static void peerview_listener(Jxta_object * obj, void *arg);
static Jxta_status process_peerview_message(_jxta_peerview_mutable * self, Jxta_RdvAdvertisement * rdva, Jxta_boolean edge,
                                            Jxta_boolean response, Jxta_boolean failure, Jxta_boolean cached);
static _jxta_peer_peerview_entry *peerview_entry_new(void);
static _jxta_peer_peerview_entry *peerview_entry_construct(_jxta_peer_peerview_entry * self,
                                                           const _jxta_peerview_entry_methods * methods);
static void peerview_entry_delete(Jxta_object * addr);
static void peerview_entry_destruct(_jxta_peer_peerview_entry * self);

static Jxta_pipe_adv *jxta_peerview_get_wirepipeadv(Jxta_peerview * pv, Jxta_PGID * destPeerGroupID);
static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *self);

static Jxta_status jxta_peerview_get_pve(_jxta_peerview_mutable * self, Jxta_id * pid, _jxta_peer_peerview_entry ** pve,
                                         Jxta_boolean addToLocalView);
static Jxta_status jxta_peerview_add_pve(_jxta_peerview_mutable * self, _jxta_peer_peerview_entry * pve);
static Jxta_status jxta_peerview_remove_pve(_jxta_peerview_mutable * self, Jxta_id * pid);

static Jxta_status jxta_peerview_send_pvm(_jxta_peerview_mutable * self, _jxta_peer_peerview_entry * pve,
                                          Jxta_endpoint_address * dest, Jxta_boolean response, Jxta_boolean failure);

static Jxta_RdvAdvertisement *jxta_peerview_build_rdva(Jxta_PG * group, JString * serviceName);

static const _jxta_peerview_entry_methods JXTA_PEERVIEW_ENTRY_METHODS = {
    {
     "Jxta_Peer_entry_methods",
     jxta_peer_lock,
     jxta_peer_unlock,
     jxta_peer_get_peerid,
     jxta_peer_set_peerid,
     jxta_peer_get_address,
     jxta_peer_set_address,
     jxta_peer_get_adv,
     jxta_peer_set_adv,
     jxta_peer_get_expires,
     jxta_peer_set_expires},
    "_jxta_peerview_entry_methods"
};

static _jxta_peer_peerview_entry *peerview_entry_new(void)
{
    _jxta_peer_peerview_entry *self = (_jxta_peer_peerview_entry *) calloc(1, sizeof(_jxta_peer_peerview_entry));

    if (NULL == self) {
        return NULL;
    }

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, peerview_entry_delete, 0);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Creating PVE [%p]\n", self);

    return peerview_entry_construct(self, &JXTA_PEERVIEW_ENTRY_METHODS);
}

static _jxta_peer_peerview_entry *peerview_entry_construct(_jxta_peer_peerview_entry * self,
                                                           const _jxta_peerview_entry_methods * methods)
{
    PEER_ENTRY_VTBL(self) = (const Jxta_Peer_entry_methods *) PTValid(methods, _jxta_peerview_entry_methods);
    self = (_jxta_peer_peerview_entry *) peer_entry_construct((_jxta_peer_entry *) self);

    if (NULL != self) {
        self->thisType = "_jxta_peer_peerview_entry";

        self->rdva = NULL;
    }

    return self;
}

static void peerview_entry_delete(Jxta_object * addr)
{
    _jxta_peer_peerview_entry *self = (_jxta_peer_peerview_entry *) addr;

    peerview_entry_destruct(self);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Deleting PVE [%p]\n", self);

    free(self);
}

static void peerview_entry_destruct(_jxta_peer_peerview_entry * self)
{
    if (NULL != self->rdva) {
        JXTA_OBJECT_RELEASE(self->rdva);
        self->rdva = NULL;
    }

    peer_entry_destruct((_jxta_peer_entry *) self);
}

static _jxta_peerview_mutable *peerview_construct(_jxta_peerview_mutable * self)
{
    apr_status_t res;

    self->thisType = "_jxta_peerview_mutable";

    apr_pool_create(&self->pool, NULL);

    res = apr_thread_mutex_create(&self->mutex, APR_THREAD_MUTEX_NESTED, self->pool);

    if (res != APR_SUCCESS) {
        return NULL;
    }
    res = apr_thread_cond_create(&self->periodicCond, self->pool);

    if (res != APR_SUCCESS) {
        return NULL;
    }

    res = apr_thread_mutex_create(&self->periodicMutex, APR_THREAD_MUTEX_DEFAULT, self->pool);

    if (APR_SUCCESS == res) {
        self->running = FALSE;
        self->periodicThread = NULL;

        self->group = NULL;
        self->parentgroup = NULL;
        self->gid = NULL;
        self->parentgid = NULL;
        self->pid = NULL;
        self->groupUniqueID = NULL;
        self->name = NULL;

        self->useOnlySeeds = FALSE;

        self->endpoint = NULL;
        self->discovery = NULL;
        self->pipes = NULL;
        self->advPipe = NULL;
        self->listener_peerview = NULL;

        self->localView = jxta_hashtable_new_0(20, TRUE);
        self->localViewOrder = jxta_vector_new(20);
        self->downPVE = NULL;
        self->selfPVE = NULL;
        self->upPVE = NULL;
        self->seeds = jxta_vector_new(1);
        self->last_seeding_update = 0;
        self->seeding = jxta_vector_new(1);
        self->activeSeeds = jxta_vector_new(1);
    } else {
        self = NULL;
    }

    return self;
}

static void peerview_destruct(_jxta_peerview_mutable * self)
{
    jxta_endpoint_service_remove_listener(self->endpoint, JXTA_PEERVIEW_NAME, jstring_get_string(self->groupUniqueID));

    if (NULL != self->listener_peerview) {
        jxta_listener_stop(self->listener_peerview);
        JXTA_OBJECT_RELEASE(self->listener_peerview);
    }

    if (NULL != self->group) {
        JXTA_OBJECT_RELEASE(self->group);
    }

    if (NULL != self->parentgroup) {
        JXTA_OBJECT_RELEASE(self->parentgroup);
    }

    if (NULL != self->groupUniqueID) {
        JXTA_OBJECT_RELEASE(self->groupUniqueID);
    }

    if (NULL != self->gid) {
        JXTA_OBJECT_RELEASE(self->gid);
    }

    if (NULL != self->parentgid) {
        JXTA_OBJECT_RELEASE(self->gid);
    }

    if (NULL != self->pid) {
        JXTA_OBJECT_RELEASE(self->pid);
    }

    if (NULL != self->name) {
        JXTA_OBJECT_RELEASE(self->name);
    }

    if (NULL != self->endpoint) {
        JXTA_OBJECT_RELEASE(self->endpoint);
    }

    if (NULL != self->discovery) {
        JXTA_OBJECT_RELEASE(self->discovery);
    }

    if (NULL != self->pipes) {
        JXTA_OBJECT_RELEASE(self->pipes);
    }

    if (NULL != self->advPipe) {
        JXTA_OBJECT_RELEASE(self->advPipe);
    }

    if (NULL != self->localView) {
        JXTA_OBJECT_RELEASE(self->localView);
    }

    if (NULL != self->localViewOrder) {
        JXTA_OBJECT_RELEASE(self->localViewOrder);
    }

    if (NULL != self->downPVE) {
        JXTA_OBJECT_RELEASE(self->downPVE);
    }

    if (NULL != self->selfPVE) {
        JXTA_OBJECT_RELEASE(self->selfPVE);
    }

    if (NULL != self->upPVE) {
        JXTA_OBJECT_RELEASE(self->upPVE);
    }

    /* Free the vectors of seeds */
    if (NULL != self->seeding) {
        JXTA_OBJECT_RELEASE(self->seeding);
    }

    if (NULL != self->seeds) {
        JXTA_OBJECT_RELEASE(self->seeds);
    }

    if (NULL != self->activeSeeds) {
        JXTA_OBJECT_RELEASE(self->activeSeeds);
    }

    /* Free the pool used to allocate the thread and mutex */
    apr_pool_destroy(self->pool);
}

static void peerview_delete(Jxta_object * obj)
{
    Jxta_status status;

    _jxta_peerview_mutable *self = (_jxta_peerview_mutable *) obj;

    if (NULL != self->periodicThread) {
        apr_thread_join(&status, self->periodicThread);
    }

    peerview_destruct(self);

    free(self);
}

JXTA_DECLARE(Jxta_peerview *) jxta_peerview_new(void)
{
    _jxta_peerview_mutable *self = (_jxta_peerview_mutable *) calloc(1, sizeof(_jxta_peerview_mutable));

    /* Initialize the object */
    JXTA_OBJECT_INIT(self, peerview_delete, 0);

    return (Jxta_peerview *) peerview_construct(self);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_init(Jxta_peerview * pv, Jxta_PG * group, JString * name)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    Jxta_PA *conf_adv = NULL;
    Jxta_status res = JXTA_SUCCESS;

    self->group = JXTA_OBJECT_SHARE(group);
    jxta_PG_get_parentgroup(self->group, &self->parentgroup);
    jxta_PG_get_PID(group, &self->pid);
    jxta_PG_get_GID(group, &self->gid);
    jxta_id_get_uniqueportion(self->gid, &self->groupUniqueID);

    if (NULL != self->parentgroup) {
        jxta_PG_get_GID(self->parentgroup, &self->parentgid);
    }

    self->name = JXTA_OBJECT_SHARE(name);

    jxta_PG_get_endpoint_service(group, &(self->endpoint));

    /* 
     * Create our PVE
     */
    jxta_peerview_get_pve(self, self->pid, &self->selfPVE, FALSE);

    self->selfPVE->rdva = jxta_peerview_build_rdva(self->group, self->name);

    jxta_peer_set_expires((Jxta_peer *) self->selfPVE, JPR_ABSOLUTE_TIME_MAX);

    /*
     * Register Endpoint Listener for Rendezvous Peerview protocol.
     */
    self->listener_peerview = jxta_listener_new(peerview_listener, (void *) self, 10, 100);

    jxta_endpoint_service_add_listener(self->endpoint, JXTA_PEERVIEW_NAME, jstring_get_string(self->groupUniqueID),
                                       self->listener_peerview);

    jxta_listener_start(self->listener_peerview);

        /**
        * Initialize seeding
        **/
    jxta_PG_get_configadv(group, &conf_adv);

    if (conf_adv != NULL) {
        Jxta_svc *svc = NULL;
        Jxta_vector *svcs;
        unsigned int sz;
        unsigned int i;

        svcs = jxta_PA_get_Svc(conf_adv);
        JXTA_OBJECT_RELEASE(conf_adv);

        sz = jxta_vector_size(svcs);
        for (i = 0; i < sz; i++) {
            Jxta_id *mcid;
            Jxta_svc *tmpsvc = NULL;
            jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
            mcid = jxta_svc_get_MCID(tmpsvc);
            if (jxta_id_equals(mcid, jxta_rendezvous_classid)) {
                svc = tmpsvc;
                JXTA_OBJECT_RELEASE(mcid);
                break;
            }
            JXTA_OBJECT_RELEASE(mcid);
            JXTA_OBJECT_RELEASE(tmpsvc);
        }
        JXTA_OBJECT_RELEASE(svcs);

        if (svc != NULL) {
            Jxta_RdvConfigAdvertisement *rdvConfig = jxta_svc_get_RdvConfig(svc);

            if (NULL != rdvConfig) {
                Jxta_vector *seedEAs = jxta_RdvConfig_get_seeds(rdvConfig);
                unsigned int eachSeed;

                for (eachSeed = 0; eachSeed < jxta_vector_size(seedEAs); eachSeed++) {
                    Jxta_peer *peer = jxta_peer_new();
                    Jxta_endpoint_address *addr;

                    jxta_vector_get_object_at(seedEAs, (Jxta_object **) & addr, eachSeed);
                    jxta_peer_set_address(peer, addr);

                    jxta_vector_add_object_last(self->seeds, (Jxta_object *) peer);
                    JXTA_OBJECT_RELEASE(addr);
                    JXTA_OBJECT_RELEASE(peer);
                }

                JXTA_OBJECT_RELEASE(seedEAs);
                JXTA_OBJECT_RELEASE(rdvConfig);
            }

            JXTA_OBJECT_RELEASE(svc);
        }
    }

    /* If we are not running in the top group then register advertisement pipe in the parent group */
    if (NULL != self->parentgroup) {
        Jxta_pipe_adv *adv = jxta_peerview_get_wirepipeadv(pv, self->parentgid);

        jxta_PG_get_pipe_service(self->parentgroup, &self->pipes);

        if (NULL != self->pipes) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Waiting to bind peerview wire pipe\n");
            res = jxta_pipe_service_timed_connect(self->pipes, adv, 15 * JPR_INTERVAL_ONE_SECOND, NULL, &self->advPipe);

            if ((res != JXTA_SUCCESS) || (self->advPipe == NULL)) {
                jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Could not connect to peerview pipe\n");
            }
        }

        JXTA_OBJECT_RELEASE(adv);
    }

    return res;
}

/**
*   When started the peerview actively seeks other peers. rdv servers do this. rdv clients do not.
**/
JXTA_DECLARE(Jxta_status) jxta_peerview_start(Jxta_peerview * pv)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    Jxta_status res = JXTA_SUCCESS;

    /* Add our PVE */
    jxta_peerview_add_pve(self, self->selfPVE);

    /* Start listening on the pipe */
    if (NULL != self->advPipe) {
        Jxta_inputpipe *ip = NULL;

        res = jxta_pipe_get_inputpipe(self->advPipe, &ip);

        if (res != JXTA_SUCCESS) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_ERROR, FILEANDLINE "Cannot get inputpipe: %d\n", (int) res);
            return res;
        }

        res = jxta_inputpipe_add_listener(ip, self->listener_peerview);
    }

    /* start the periodic worker thread */
    apr_thread_mutex_lock(self->periodicMutex);

    apr_thread_create(&self->periodicThread, NULL, periodic_thread_main, self, self->pool);

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview started for group %s / %s \n",
                    jstring_get_string(self->groupUniqueID), jstring_get_string(self->name));

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_stop(Jxta_peerview * pv)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    if (NULL != self->pipes) {
        JXTA_OBJECT_RELEASE(self->pipes);
        self->pipes = NULL;
    }

    /* Announce that we are shutting down */
    (void) jxta_peerview_send_rdv_request((Jxta_peerview *) self, TRUE, TRUE);

    /* Remove our PVE */
    jxta_peerview_remove_pve(self, jxta_peer_get_peerid_priv((Jxta_peer *) self->selfPVE));

    apr_thread_mutex_lock(self->periodicMutex);
    self->running = FALSE;
    apr_thread_cond_signal(self->periodicCond);
    apr_thread_mutex_unlock(self->periodicMutex);

    return JXTA_SUCCESS;
}

/**
 * Construct the associated peergroup RDV wire pipe.
 **/
static Jxta_pipe_adv *jxta_peerview_get_wirepipeadv(Jxta_peerview * pv, Jxta_PGID * destPeerGroupID)
{
    Jxta_status status;
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    JString *buf;
    Jxta_id *pid;
    JString *pipeId;
    Jxta_pipe_adv *adv = NULL;

    buf = jstring_new_2(JXTA_PEERVIEW_NAME);
    jstring_append_1(buf, self->groupUniqueID);
    jstring_append_1(buf, self->name);

    /* create pipe id in the designated group */
    status = jxta_id_pipeid_new_2(&pid, destPeerGroupID, (unsigned char *) jstring_get_string(buf), jstring_length(buf));
    JXTA_OBJECT_RELEASE(buf);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Error creating RDV wire pipe adv\n");
        return NULL;
    }

    jxta_id_to_jstring(pid, &pipeId);
    JXTA_OBJECT_RELEASE(pid);

    adv = jxta_pipe_adv_new();

    jxta_pipe_adv_set_Id(adv, jstring_get_string(pipeId));
    JXTA_OBJECT_RELEASE(pipeId);
    jxta_pipe_adv_set_Type(adv, JXTA_PROPAGATE_PIPE);
    jxta_pipe_adv_set_Name(adv, JXTA_PEERVIEW_NAME);

    return adv;
}

static Jxta_RdvAdvertisement *jxta_peerview_build_rdva(Jxta_PG * group, JString * serviceName)
{
    Jxta_PG *parent;
    Jxta_PGID *pgid;
    Jxta_PID *peerid;
    JString *name;
    Jxta_vector *svcs = NULL;
    Jxta_RouteAdvertisement *route = NULL;
    Jxta_PA *padv;
    Jxta_RdvAdvertisement *rdva = NULL;
    Jxta_svc *svc = NULL;
    unsigned int sz;
    unsigned int i;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Building rdv advertisement\n");

    jxta_PG_get_parentgroup(group, &parent);

    if (NULL != parent) {
        jxta_PG_get_PA(parent, &padv);
        JXTA_OBJECT_RELEASE(parent);
    } else {
        jxta_PG_get_PA(group, &padv);
    }

    rdva = jxta_RdvAdvertisement_new();

    name = jxta_PA_get_Name(padv);
    jxta_RdvAdvertisement_set_Name(rdva, jstring_get_string(name));
    JXTA_OBJECT_RELEASE(name);

    jxta_RdvAdvertisement_set_Service(rdva, jstring_get_string(serviceName));

    pgid = jxta_PA_get_GID(padv);
    jxta_RdvAdvertisement_set_RdvGroupId(rdva, pgid);
    JXTA_OBJECT_RELEASE(pgid);

    peerid = jxta_PA_get_PID(padv);
    jxta_RdvAdvertisement_set_RdvPeerId(rdva, peerid);
    JXTA_OBJECT_RELEASE(peerid);

    svcs = jxta_PA_get_Svc(padv);
    sz = jxta_vector_size(svcs);
    for (i = 0; i < sz; i++) {
        Jxta_id *mcid;
        Jxta_svc *tmpsvc = NULL;
        jxta_vector_get_object_at(svcs, (Jxta_object **) & tmpsvc, i);
        mcid = jxta_svc_get_MCID(tmpsvc);

        if (jxta_id_equals(mcid, jxta_endpoint_classid)) {
            svc = tmpsvc;
            JXTA_OBJECT_RELEASE(mcid);
            break;
        }
        JXTA_OBJECT_RELEASE(mcid);
        JXTA_OBJECT_RELEASE(tmpsvc);
    }
    JXTA_OBJECT_RELEASE(padv);
    JXTA_OBJECT_RELEASE(svcs);

    if (svc == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Could not find the endpoint service params\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    route = jxta_svc_get_RouteAdvertisement(svc);
    JXTA_OBJECT_RELEASE(svc);
    if (route == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING,
                        FILEANDLINE "Could not find the route advertisement in service params\n");
        JXTA_OBJECT_RELEASE(rdva);
        return NULL;
    }

    jxta_RdvAdvertisement_set_Route(rdva, route);
    JXTA_OBJECT_RELEASE(route);

    return rdva;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_send_rdv_request(Jxta_peerview * pv, Jxta_boolean announce, Jxta_boolean failure)
{
    Jxta_status status = JXTA_SUCCESS;
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    Jxta_outputpipe *op = NULL;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    JString *peerdoc;
    Jxta_RdvAdvertisement *rdva;

    if (NULL == self->advPipe) {
        return status;
    }

    apr_thread_mutex_lock(self->mutex);
    rdva = JXTA_OBJECT_SHARE(self->selfPVE->rdva);
    apr_thread_mutex_unlock(self->mutex);

    status = jxta_RdvAdvertisement_get_xml(rdva, &peerdoc);
    JXTA_OBJECT_RELEASE(rdva);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to retrieve my RdvAdvertisement\n");
        return status;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME,
                                    announce ? JXTA_PEERVIEW_RDVRESP_ELEMENT_NAME : JXTA_PEERVIEW_RDVADV_ELEMENT_NAME, "text/xml",
                                    jstring_get_string(peerdoc), jstring_length(peerdoc), NULL);

    JXTA_OBJECT_RELEASE(peerdoc);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    if (!self->running) {
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_PEERVIEW_EDGE_ELEMENT_NAME, "text/plain", "true", strlen("true"),
                                        NULL);
        jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
    }

    if (failure) {
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_PEERVIEW_FAILURE_ELEMENT_NAME, "text/plain", "true",
                                        strlen("true"), NULL);
        jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
    }

    /*
     * Create the output wirepipe
     */
    status = jxta_pipe_get_outputpipe(self->advPipe, &op);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Cannot get peerview adv outputpipe\n");
        JXTA_OBJECT_RELEASE(msg);
        return status;
    }

    if (op == NULL) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Cannot get output pipe\n");
        JXTA_OBJECT_RELEASE(msg);
        return JXTA_FAILED;
    }

    status = jxta_outputpipe_send(op, msg);
    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Sending RDV wire pipe failed: %d\n", status);
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sent RDV wire pipe\n");
    }

    JXTA_OBJECT_RELEASE(op);
    JXTA_OBJECT_RELEASE(msg);

    return status;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_send_rdv_probe(Jxta_peerview * pv, Jxta_endpoint_address * dest)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    return jxta_peerview_send_pvm(self, self->selfPVE, dest, FALSE, FALSE);
}

static Jxta_status jxta_peerview_send_pvm(_jxta_peerview_mutable * self, _jxta_peer_peerview_entry * pve,
                                          Jxta_endpoint_address * dest, Jxta_boolean response, Jxta_boolean failure)
{
    Jxta_status status;
    Jxta_message *msg = NULL;
    Jxta_message_element *el = NULL;
    Jxta_RdvAdvertisement *rdva;
    JString *peerdoc;
    Jxta_endpoint_address *destAddr = NULL;
    char *addrStr = NULL;

    jxta_peer_lock((Jxta_peer *) pve);
    if (pve == self->selfPVE) {
        Jxta_time current = jpr_time_now();

        if ((NULL == pve->rdva) || (((Jxta_time_diff) (current - pve->last_refresh)) > RDVA_REFRESH_INTERVAL)) {
            pve->rdva = jxta_peerview_build_rdva(self->group, self->name);
            pve->last_refresh = current;
        }
    }

    if (NULL == pve->rdva) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "PVE has no rdv advertisement\n", addrStr);
        jxta_peer_unlock((Jxta_peer *) pve);
        return JXTA_FAILED;
    }

    rdva = JXTA_OBJECT_SHARE(pve->rdva);
    jxta_peer_unlock((Jxta_peer *) pve);

    status = jxta_RdvAdvertisement_get_xml(rdva, &peerdoc);
    JXTA_OBJECT_RELEASE(rdva);

    if (status != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed to build RdvAdvertisement\n");
        return status;
    }

    msg = jxta_message_new();

    el = jxta_message_element_new_2(JXTA_RDV_NS_NAME,
                                    response ? JXTA_PEERVIEW_RDVRESP_ELEMENT_NAME : JXTA_PEERVIEW_RDVADV_ELEMENT_NAME,
                                    "text/xml", jstring_get_string(peerdoc), jstring_length(peerdoc), NULL);
    JXTA_OBJECT_RELEASE(peerdoc);
    jxta_message_add_element(msg, el);
    JXTA_OBJECT_RELEASE(el);

    if (!self->running) {
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_PEERVIEW_EDGE_ELEMENT_NAME, "text/plain", "true",
                                        strlen("true"), NULL);
        jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
    }

    if (self->selfPVE != pve) {
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_PEERVIEW_CACHED_ELEMENT_NAME, "text/plain", "true", strlen("true"),
                                        NULL);
        jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
    }

    if (failure) {
        el = jxta_message_element_new_2(JXTA_RDV_NS_NAME, JXTA_PEERVIEW_FAILURE_ELEMENT_NAME, "text/plain", "true",
                                        strlen("true"), NULL);
        jxta_message_add_element(msg, el);
        JXTA_OBJECT_RELEASE(el);
    }

    JXTA_OBJECT_CHECK_VALID(msg);

    addrStr = jxta_endpoint_address_to_string(dest);
    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Sending PeerView message to : %s\n", addrStr);
    free(addrStr);

    if (0 != strcmp("jxta", jxta_endpoint_address_get_protocol_name(dest))) {
        char *serviceStr = malloc(strlen("EndpointService:") + jstring_length(self->groupUniqueID) + 1);
        char *serviceParamStr;

        if (NULL != serviceStr) {
            strcpy(serviceStr, "EndpointService:");
            strcat(serviceStr, jstring_get_string(self->groupUniqueID));

            serviceParamStr = malloc(strlen(JXTA_PEERVIEW_NAME) + jstring_length(self->groupUniqueID) + 2);

            if (NULL != serviceParamStr) {
                strcpy(serviceParamStr, JXTA_PEERVIEW_NAME);
                strcat(serviceParamStr, "/");
                strcat(serviceParamStr, jstring_get_string(self->groupUniqueID));

                destAddr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(dest),
                                                      jxta_endpoint_address_get_protocol_address(dest), serviceStr,
                                                      serviceParamStr);
                free(serviceParamStr);
            } else {
                status = JXTA_NOMEM;
            }
            free(serviceStr);
        } else {
            status = JXTA_NOMEM;
        }
    } else {
        destAddr = jxta_endpoint_address_new2(jxta_endpoint_address_get_protocol_name(dest),
                                              jxta_endpoint_address_get_protocol_address(dest),
                                              JXTA_PEERVIEW_NAME, jstring_get_string(self->groupUniqueID));
    }

    if (NULL != destAddr) {
        status = jxta_endpoint_service_send(self->group, self->endpoint, msg, destAddr);
        JXTA_OBJECT_RELEASE(destAddr);
    }

    JXTA_OBJECT_RELEASE(msg);

    return status;
}

/**
 ** This is the generic message handler of the JXTA Rendezvous Peerview Service
 ** protocol. Since this implementation is designed for an edge peer,
 ** it only has to support:
 **
 **   - Receive a peerview response when searching for a subgroup RDV
 **/
static void peerview_listener(Jxta_object * obj, void *arg)
{
    Jxta_status res = JXTA_SUCCESS;
    _jxta_peerview_mutable *self = PTValid(arg, _jxta_peerview_mutable);
    Jxta_message_element *el = NULL;
    Jxta_message *msg = (Jxta_message *) obj;
    Jxta_boolean response = FALSE;
    Jxta_boolean edge = FALSE;
    Jxta_boolean cached = FALSE;
    Jxta_boolean failure = FALSE;
    Jxta_bytevector *bytes;
    JString *string;
    Jxta_RdvAdvertisement *rdva;

    JXTA_OBJECT_CHECK_VALID(msg);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Rendezvous Service Peerview received a message [%p]\n", msg);

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_PEERVIEW_FAILURE_ELEMENT_NAME, &el);
    failure = (JXTA_SUCCESS == res) && (NULL != el);
    if (failure) {
        JXTA_OBJECT_RELEASE(el);
    }

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_PEERVIEW_EDGE_ELEMENT_NAME, &el);
    edge = (JXTA_SUCCESS == res) && (NULL != el);
    if (edge) {
        JXTA_OBJECT_RELEASE(el);
    }

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_PEERVIEW_CACHED_ELEMENT_NAME, &el);
    cached = (JXTA_SUCCESS == res) && (NULL != el);
    if (cached) {
        JXTA_OBJECT_RELEASE(el);
    }

    res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_PEERVIEW_RDVADV_ELEMENT_NAME, &el);

    if ((JXTA_SUCCESS != res) || (NULL == el)) {
        res = jxta_message_get_element_2(msg, JXTA_RDV_NS_NAME, JXTA_PEERVIEW_RDVRESP_ELEMENT_NAME, &el);

        if ((JXTA_SUCCESS != res) || (NULL == el)) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "RdvAdvertisement missing. Ignoring message [%p]\n", msg);
            goto Common_Exit;
        }

        response = TRUE;
    }

    bytes = jxta_message_element_get_value(el);
    JXTA_OBJECT_RELEASE(el);
    string = jstring_new_3(bytes);
    JXTA_OBJECT_RELEASE(bytes);
    rdva = jxta_RdvAdvertisement_new();

    res = jxta_RdvAdvertisement_parse_charbuffer(rdva, jstring_get_string(string), jstring_length(string));
    JXTA_OBJECT_RELEASE(string);
    if (JXTA_SUCCESS == res) {
        res = process_peerview_message(self, rdva, edge, response, failure, cached);
        JXTA_OBJECT_RELEASE(rdva);
    }

  Common_Exit:
    if (res != JXTA_SUCCESS) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed processing peerview message [%p] %d\n", msg, res);
    }
}

JXTA_DECLARE(Jxta_status) jxta_peerview_probe_cached_rdvadv(Jxta_peerview * pv, Jxta_RdvAdvertisement * rdva ) {
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    return process_peerview_message( self, rdva, FALSE, FALSE, FALSE, TRUE );
}

static Jxta_status process_peerview_message(_jxta_peerview_mutable * self, Jxta_RdvAdvertisement * rdva, Jxta_boolean edge,
                                            Jxta_boolean response, Jxta_boolean failure, Jxta_boolean cached)
{
    Jxta_status res = JXTA_SUCCESS;
    Jxta_RouteAdvertisement *route = jxta_RdvAdvertisement_get_Route(rdva);
    Jxta_id *dest = jxta_RdvAdvertisement_get_RdvPeerId(rdva);
    JString *pid;
    _jxta_peer_peerview_entry *pve = NULL;
    Jxta_boolean newbie = TRUE;

    jxta_id_to_jstring(dest, &pid);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Processing peerview msg from %s : %c %c %c %c \n", jstring_get_string(pid),
                    edge ? 'E' : 'e', response ? 'R' : 'r', failure ? 'F' : 'f', cached ? 'C' : 'c');


    if (failure) {
        /* FIXME 20050301 bondolo Handle failures here ! */
        jxta_peerview_remove_pve(self, dest);

        goto Common_Exit;
    }

    if (self->discovery == NULL) {
        jxta_PG_get_discovery_service(self->group, &(self->discovery));
    }

    if (!edge && !cached) {
        /*
           Publish the rdv advertisement
         */
        if (self->discovery != NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Publishing Rendezvous Advertisement [%p] for %s\n", rdva,
                            jstring_get_string(pid));
            discovery_service_publish(self->discovery, (Jxta_advertisement *) rdva, DISC_ADV,
                                      (Jxta_expiration_time) DEFAULT_EXPIRATION, (Jxta_expiration_time) DEFAULT_EXPIRATION);
        }
    }

    if (route != NULL) {
        /*
           Publish the route advertisement
         */
        if (self->discovery != NULL) {
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Publishing Route Advertisement [%p] for %s\n", route,
                            jstring_get_string(pid));
            discovery_service_publish(self->discovery, (Jxta_advertisement *) route, DISC_ADV,
                                      (Jxta_expiration_time) DEFAULT_EXPIRATION, (Jxta_expiration_time) DEFAULT_EXPIRATION);
        }

        JXTA_OBJECT_RELEASE(route);
    }

    res = jxta_peerview_get_pve(self, dest, &pve, (!edge && !cached));

    if ((NULL == pve) || (JXTA_SUCCESS != res)) {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, FILEANDLINE "Failed getting pve. %d \n", res);
        goto Common_Exit;
    }

    jxta_peer_lock((Jxta_peer *) pve);
    newbie = NULL == pve->rdva;

    if (NULL != pve->rdva) {
        JXTA_OBJECT_RELEASE(pve->rdva);
    }

    pve->rdva = JXTA_OBJECT_SHARE(rdva);
    pve->last_refresh = jpr_time_now();

    /* Set the upward bound to time at which we will consider this PVE "alive" */
    jxta_peer_set_expires((Jxta_peer *) pve, pve->last_refresh + 10 * 60 * 1000);
    jxta_peer_unlock((Jxta_peer *) pve);

    /*
     *   The (in)famous "Type 1" : We are being probed. Respond with our own PVE.
     */
    if (!response && !cached && self->running) {
        /* send our response */
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending my PVE to %s \n", jstring_get_string(pid));

        res = jxta_peerview_send_pvm(self, self->selfPVE, jxta_peer_get_address_priv((Jxta_peer *) pve), TRUE, FALSE);
    }

    /*
     *   The (in)famous "Type 2" : We received a referral we hadn't known. Probe it.
     */
    if (!self->useOnlySeeds && newbie) {
        if ((cached && !response) || (!cached && !response && !edge)) {
            /* send our response */
            jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Probing %s with my PVE\n", jstring_get_string(pid));

            res = jxta_peerview_send_pvm(self, self->selfPVE, jxta_peer_get_address_priv((Jxta_peer *) pve), FALSE, FALSE);
        }
    }

    /*
     *   The (in)famous "Type 3" : Someone is looking for rdvs. Try to help by sharing one of the Rdv Advs we know.
     */
    if (!cached && !response) {
        /* send our response */
        _jxta_peer_peerview_entry *aPVE = NULL;

        apr_thread_mutex_lock(self->mutex);

        if (jxta_vector_size(self->localViewOrder) > (self->running ? 1 : 0)) {
            unsigned int idx = rand() % jxta_vector_size(self->localViewOrder);

            jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & aPVE, idx);
        } else {
            aPVE = JXTA_OBJECT_SHARE(self->selfPVE);
        }

        apr_thread_mutex_unlock(self->mutex);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Sending a referal PVE to %s \n", jstring_get_string(pid));

        res = jxta_peerview_send_pvm(self, self->selfPVE, jxta_peer_get_address_priv((Jxta_peer *) pve), TRUE, FALSE);

        JXTA_OBJECT_RELEASE(aPVE);
    }

    JXTA_OBJECT_RELEASE(pve);
    res = JXTA_SUCCESS;

  Common_Exit:
    JXTA_OBJECT_RELEASE(pid);
    JXTA_OBJECT_RELEASE(dest);

    return res;
}

static Jxta_status jxta_peerview_get_pve(_jxta_peerview_mutable * self, Jxta_id * pid, _jxta_peer_peerview_entry ** pve,
                                         Jxta_boolean addToLocalView)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *pidString = NULL;

    apr_thread_mutex_lock(self->mutex);

    jxta_id_to_jstring(pid, &pidString);

    res = jxta_hashtable_get(self->localView, jstring_get_string(pidString), jstring_length(pidString) + 1, (Jxta_object **) pve);
    JXTA_OBJECT_RELEASE(pidString);

    if ((JXTA_SUCCESS != res) || (NULL == *pve)) {
        JString *uniq;
        Jxta_endpoint_address *ea;

        jxta_id_get_uniqueportion(pid, &uniq);

        ea = jxta_endpoint_address_new2("jxta", jstring_get_string(uniq), NULL, NULL);
        JXTA_OBJECT_RELEASE(uniq);

        *pve = peerview_entry_new();

        jxta_peer_set_peerid((Jxta_peer *) * pve, pid);
        jxta_peer_set_address((Jxta_peer *) * pve, ea);
        jxta_peer_set_expires((Jxta_peer *) * pve, JPR_ABSOLUTE_TIME_MAX);
        JXTA_OBJECT_RELEASE(ea);

        if (addToLocalView) {
            res = jxta_peerview_add_pve(self, *pve);
        } else {
            res = JXTA_SUCCESS;
        }
    }

    apr_thread_mutex_unlock(self->mutex);

    return res;
}

static Jxta_status jxta_peerview_add_pve(_jxta_peerview_mutable * self, _jxta_peer_peerview_entry * pve)
{
    Jxta_status res = JXTA_SUCCESS;
    JString *pidString = NULL;

    apr_thread_mutex_lock(self->mutex);

    jxta_id_to_jstring(jxta_peer_get_peerid_priv((Jxta_peer *) pve), &pidString);

    res =
        jxta_hashtable_get(self->localView, jstring_get_string(pidString), jstring_length(pidString) + 1, (Jxta_object **) & pve);

    if ((JXTA_SUCCESS != res) || (NULL == pve)) {
        unsigned int eachPVE;
        Jxta_boolean added = FALSE;

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Adding new PVE [%p] for %s\n", pve, jstring_get_string(pidString));

        eachPVE = jxta_vector_size(self->localViewOrder) - 1;
        for (eachPVE = 0; !added && (eachPVE < jxta_vector_size(self->localViewOrder)); eachPVE++) {
            _jxta_peer_peerview_entry *comparePVE;
            JString *comparePVEpidString = NULL;

            jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & comparePVE, eachPVE);

            jxta_id_to_jstring(jxta_peer_get_peerid_priv((Jxta_peer *) comparePVE), &comparePVEpidString);

            if (strcmp(jstring_get_string(comparePVEpidString), jstring_get_string(pidString)) > 0) {
                jxta_vector_add_object_at(self->localViewOrder, (Jxta_object *) pve, eachPVE + 1);
                added = TRUE;
            }

            JXTA_OBJECT_RELEASE(comparePVEpidString);
            JXTA_OBJECT_RELEASE(comparePVE);
        }

        if (!added) {
            jxta_vector_add_object_at(self->localViewOrder, (Jxta_object *) pve, 0);
        }

        jxta_hashtable_put(self->localView, jstring_get_string(pidString), jstring_length(pidString) + 1, (Jxta_object *) pve);

        /* adjust up/down peers */
        if (NULL != self->downPVE) {
            JXTA_OBJECT_RELEASE(self->downPVE);
            self->downPVE = NULL;
        }

        if (NULL != self->upPVE) {
            JXTA_OBJECT_RELEASE(self->upPVE);
            self->upPVE = NULL;
        }

        for (eachPVE = 0; eachPVE < jxta_vector_size(self->localViewOrder); eachPVE++) {
            _jxta_peer_peerview_entry *comparePVE;

            jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & comparePVE, eachPVE);

            if (self->selfPVE == comparePVE) {
                if (eachPVE > 0) {
                    jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & self->downPVE, eachPVE - 1);
                }

                if ((eachPVE < (jxta_vector_size(self->localViewOrder) - 1))) {
                    jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & self->upPVE, eachPVE + 1);
                }

                JXTA_OBJECT_RELEASE(comparePVE);

                break;
            }

            JXTA_OBJECT_RELEASE(comparePVE);
        }

        res = JXTA_SUCCESS;
    } else {
        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_WARNING, "PVE [%p] already recorded for %s\n", pve,
                        jstring_get_string(pidString));
        res = JXTA_FAILED;
    }

    apr_thread_mutex_unlock(self->mutex);

    JXTA_OBJECT_RELEASE(pidString);

    return res;
}

static Jxta_status jxta_peerview_remove_pve(_jxta_peerview_mutable * self, Jxta_id * pid)
{
    Jxta_status res = JXTA_SUCCESS;
    unsigned int eachPVE;
    Jxta_boolean removed = FALSE;

    apr_thread_mutex_lock(self->mutex);

    for (eachPVE = 0; !removed && (eachPVE < jxta_vector_size(self->localViewOrder)); eachPVE++) {
        _jxta_peer_peerview_entry *comparePVE;

        res = jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & comparePVE, eachPVE);

        if (jxta_id_equals(pid, jxta_peer_get_peerid_priv((Jxta_peer *) comparePVE))) {
            res = jxta_vector_remove_object_at(self->localViewOrder, NULL, eachPVE);
            removed = TRUE;
        }

        JXTA_OBJECT_RELEASE(comparePVE);
    }

    if (removed) {
        JString *pidString = NULL;

        jxta_id_to_jstring(pid, &pidString);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_DEBUG, "Removed PVE for %s\n", jstring_get_string(pidString));

        res = jxta_hashtable_del(self->localView, jstring_get_string(pidString), jstring_length(pidString) + 1, NULL);

        JXTA_OBJECT_RELEASE(pidString);

        /* adjust up/down peers */
        if (NULL != self->downPVE) {
            JXTA_OBJECT_RELEASE(self->downPVE);
            self->downPVE = NULL;
        }
        if (NULL != self->upPVE) {
            JXTA_OBJECT_RELEASE(self->upPVE);
            self->upPVE = NULL;
        }

        for (eachPVE = 0; eachPVE < jxta_vector_size(self->localViewOrder); eachPVE++) {
            _jxta_peer_peerview_entry *comparePVE;

            res = jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & comparePVE, eachPVE);

            if (self->selfPVE == comparePVE) {
                if (eachPVE > 0) {
                    res = jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & self->downPVE, eachPVE - 1);
                }

                if ((eachPVE < (jxta_vector_size(self->localViewOrder) - 1))) {
                    res = jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & self->upPVE, eachPVE + 1);
                }

                JXTA_OBJECT_RELEASE(comparePVE);

                break;
            }

            JXTA_OBJECT_RELEASE(comparePVE);
        }
    }

    apr_thread_mutex_unlock(self->mutex);

    return res;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_localview(Jxta_peerview * pv, Jxta_vector ** view)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    unsigned int eachPVE;

    apr_thread_mutex_lock(self->mutex);

    jxta_vector_clone(self->localViewOrder, view, 0, INT_MAX);

    apr_thread_mutex_unlock(self->mutex);

    /* clean up expired PVEs */
    for (eachPVE = 0; eachPVE < jxta_vector_size(*view); eachPVE++) {
        _jxta_peer_peerview_entry *checkPVE;
        Jxta_time expiresAt;
        Jxta_time currentTime = (Jxta_time) jpr_time_now();

        jxta_vector_get_object_at(*view, (Jxta_object **) & checkPVE, eachPVE);

        expiresAt = jxta_peer_get_expires((Jxta_peer *) checkPVE);

        if (expiresAt < currentTime) {
            jxta_vector_remove_object_at(*view, NULL, eachPVE);
            jxta_peerview_remove_pve(self, jxta_peer_get_peerid_priv((Jxta_peer *) checkPVE));
        }

        JXTA_OBJECT_RELEASE(checkPVE);
    }

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_up_peer(Jxta_peerview * pv, Jxta_peer ** peer)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    apr_thread_mutex_lock(self->mutex);

    *peer = self->upPVE ? JXTA_OBJECT_SHARE(self->upPVE) : NULL;

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}


JXTA_DECLARE(JString *) jxta_peerview_get_name(Jxta_peerview * pv) {
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    return JXTA_OBJECT_SHARE(self->name);
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_self_peer(Jxta_peerview * pv, Jxta_peer ** peer)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    apr_thread_mutex_lock(self->mutex);

    *peer = self->selfPVE ? JXTA_OBJECT_SHARE(self->selfPVE) : NULL;

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_get_down_peer(Jxta_peerview * pv, Jxta_peer ** peer)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    apr_thread_mutex_lock(self->mutex);

    *peer = self->downPVE ? JXTA_OBJECT_SHARE(self->downPVE) : NULL;

    apr_thread_mutex_unlock(self->mutex);

    return JXTA_SUCCESS;
}

JXTA_DECLARE(unsigned int) jxta_peerview_get_localview_size(Jxta_peerview * pv)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);
    unsigned int size;

    apr_thread_mutex_lock(self->mutex);

    size = jxta_vector_size(self->localViewOrder);

    apr_thread_mutex_unlock(self->mutex);

    return size;
}

JXTA_DECLARE(Jxta_boolean) jxta_peerview_get_happy_size(Jxta_peerview * pv)
{
    _jxta_peerview_mutable *self = PTValid(pv, _jxta_peerview_mutable);

    return HAPPY_PEERVIEW_SIZE;
}

/**
*   Handles administrative tasks while running.
**/
static void *APR_THREAD_FUNC periodic_thread_main(apr_thread_t * thread, void *arg)
{
    _jxta_peerview_mutable *self = PTValid(arg, _jxta_peerview_mutable);
    Jxta_status res;
    Jxta_vector *probe_seeds = NULL;

    apr_thread_mutex_lock(self->periodicMutex);

    self->running = TRUE;

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview worker thread started.\n");

    while (self->running) {
        unsigned int eachPVE;
        _jxta_peer_peerview_entry *current_up;
        _jxta_peer_peerview_entry *current_down;

        res = apr_thread_cond_timedwait(self->periodicCond, self->periodicMutex, JXTA_PEERVIEW_PERIODIC_INTERVAL);

        if (res != APR_TIMEUP) {
            continue;
        }

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peerview worker thread RUN.\n");

        apr_thread_mutex_lock(self->mutex);

        /* Remove Expired PVEs */
        for (eachPVE = 0; eachPVE < jxta_vector_size(self->localViewOrder); eachPVE++) {
            _jxta_peer_peerview_entry *checkPVE;
            Jxta_time expiresAt;
            Jxta_time currentTime = (Jxta_time) jpr_time_now();

            res = jxta_vector_get_object_at(self->localViewOrder, (Jxta_object **) & checkPVE, eachPVE);

            if ((JXTA_SUCCESS != res) || (NULL == checkPVE)) {
                continue;
            }

            expiresAt = jxta_peer_get_expires((Jxta_peer *) checkPVE);

            if (expiresAt < currentTime) {
                jxta_peerview_remove_pve(self, jxta_peer_get_peerid_priv((Jxta_peer *) checkPVE));
            }

            JXTA_OBJECT_RELEASE(checkPVE);
        }

        /* Update the up/down peers */
        current_down = self->downPVE ? JXTA_OBJECT_SHARE(self->downPVE) : NULL;
        current_up = self->upPVE ? JXTA_OBJECT_SHARE(self->upPVE) : NULL;

        apr_thread_mutex_unlock(self->mutex);

        /* Refresh the up and down peers. 1/3 of the time ask for a referral if we don't have a happy view */
        if (current_down) {
            res =
                jxta_peerview_send_pvm(self, self->selfPVE, jxta_peer_get_address_priv((Jxta_peer *) current_down),
                                       (0 == (rand() % 3))
                                       && (jxta_vector_size(self->localViewOrder) >= HAPPY_PEERVIEW_SIZE), FALSE);

            if (JXTA_SUCCESS != res) {
                jxta_peerview_remove_pve(self, jxta_peer_get_peerid_priv((Jxta_peer *) current_down));
            }

            JXTA_OBJECT_RELEASE(current_down);
        }

        if (current_up) {
            res =
                jxta_peerview_send_pvm(self, self->selfPVE, jxta_peer_get_address_priv((Jxta_peer *) current_up),
                                       (0 == (rand() % 3))
                                       && (jxta_vector_size(self->localViewOrder) >= HAPPY_PEERVIEW_SIZE), FALSE);

            if (JXTA_SUCCESS != res) {
                jxta_peerview_remove_pve(self, jxta_peer_get_peerid_priv((Jxta_peer *) current_up));
            }

            JXTA_OBJECT_RELEASE(current_up);
        }

        /* 
         *  Do Kick
         */
        /* FIXME 20050417 bondolo Multicast announce ourself */

        /* get some seeds if necessary */
        if ((jxta_vector_size(self->localViewOrder) < HAPPY_PEERVIEW_SIZE)) {
            unsigned int probed = 0;
            if ((NULL == probe_seeds) || (0 == jxta_vector_size(probe_seeds))) {
                if (NULL != probe_seeds) {
                    JXTA_OBJECT_RELEASE(probe_seeds);
                    probe_seeds = NULL;
                }

                probe_seeds = jxta_peerview_get_seeds(self);
            }

            while ((probed < 3) && (0 != jxta_vector_size(probe_seeds))) {
                /* send a connect to the first peer in the list */
                Jxta_peer *peer;
                Jxta_endpoint_address *addr;

                jxta_vector_remove_object_at(probe_seeds, (Jxta_object **) & peer, 0);

                PTValid(peer, _jxta_peer_entry);

                addr = jxta_peer_get_address_priv(peer);

                JXTA_OBJECT_CHECK_VALID(addr);

                jxta_peerview_send_rdv_probe(self, addr);

                JXTA_OBJECT_RELEASE(peer);

                probed++;
            }
        } else {
            if (NULL != probe_seeds) {
                JXTA_OBJECT_RELEASE(probe_seeds);
                probe_seeds = NULL;
            }
        }

        /* Notify the advertising wire */
        res = jxta_peerview_send_rdv_request((Jxta_peerview *) self, TRUE, FALSE);

        jxta_log_append(__log_cat, JXTA_LOG_LEVEL_TRACE, "Peerview worker thread RUN DONE.\n");
    }

    apr_thread_mutex_unlock(self->periodicMutex);

    jxta_log_append(__log_cat, JXTA_LOG_LEVEL_INFO, "Peerview worker thread exiting.\n");

    apr_thread_exit(thread, JXTA_SUCCESS);

    /* NOTREACHED */
    return NULL;
}

JXTA_DECLARE(Jxta_vector *) jxta_peerview_get_seeds(Jxta_peerview * peerview)
{
    _jxta_peerview_mutable *self = PTValid(peerview, _jxta_peerview_mutable);
    Jxta_vector *result = NULL;

    /** FIXME 20050206 Repopulate active seeds properly. For now just copy seeds to active seeds.**/
    JXTA_OBJECT_RELEASE(self->activeSeeds);
    jxta_vector_clone(self->seeds, &self->activeSeeds, 0, INT_MAX);

    jxta_vector_clone(self->activeSeeds, &result, 0, INT_MAX);

    jxta_vector_shuffle( result );

    return result;
}

JXTA_DECLARE(Jxta_status) jxta_peerview_add_seed(Jxta_peerview * peerview, Jxta_peer * peer)
{
    _jxta_peerview_mutable *self = PTValid(peerview, _jxta_peerview_mutable);
    Jxta_status result = JXTA_SUCCESS;

    jxta_vector_add_object_last(self->seeds, (Jxta_object *) peer);

    return result;
}

/* vim: set ts=4 sw=4 et tw=130: */
