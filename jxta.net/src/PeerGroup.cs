/*
 * Copyright (c) 2005 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: PeerGroup.cs,v 1.3 2006/08/04 10:33:19 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
    /// <summary>
	/// Summary of PeerGroup.
	/// </summary>
    public interface PeerGroup : Service
    {
        /// <summary>
        /// Endpoint Service for this Peer Group. This service is present in every Peer Group.
        /// </summary>
        //EndpointService EndpointService
        //{
        //    get;
        //}

        /// <summary>
        /// Mebership service for this peer group
        /// </summary>
        MembershipService MembershipService
        {
            get;
        }

        /// <summary>
        /// Current group for its peer advertisement on this peer.
        /// </summary>
        PeerAdvertisement PeerAdvertisement
        {
            get;
        }

        /// <summary>
        /// Name of this peer group
        /// </summary>
        string PeerGroupName
        {
            get;
        }

        /// <summary>
        /// Discovery service of this peer group
        /// </summary>
        DiscoveryService DiscoveryService
        {
            get;
        }

        /// <summary>
        /// Name of this peer in this group.
        /// </summary>
        string PeerName
        {
            get;
        }

        /// <summary>
        ///  ID of this group.
        /// </summary>
        PeerGroupID PeerGroupID
        {
            get;
        }

        /// <summary>
        /// ID of this peer in this group.
        /// </summary>
        PeerID PeerID
        {
            get;
        }

        /// <summary>
        /// Group advertisement of this group
        /// </summary>
        PeerGroupAdvertisement PeerGroupAdvertisement
        {
            get;
        }

        /// <summary>
        /// Pipe service for this group
        /// </summary>
        PipeService PipeService
        {
            get;
        }

        /// <summary>
        /// Rendezvous service for this group.
        /// </summary>
        RendezVousService RendezVousService
        {
            get;
        }
    }

    public class PeerGroupImpl : JxtaObject, PeerGroup
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_PA(IntPtr self, ref IntPtr adv);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_membership_service(IntPtr self, ref IntPtr membership);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_groupname(IntPtr self, ref IntPtr gname);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_peername(IntPtr self, ref IntPtr pname);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_GID(IntPtr self, ref IntPtr gid);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_PID(IntPtr self, ref IntPtr pid);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_PGA(IntPtr self, ref IntPtr pga);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_pipe_service(IntPtr self, ref IntPtr pipe);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_discovery_service(IntPtr self, ref IntPtr disco);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_rendezvous_service(IntPtr self, ref IntPtr rdv);

        [DllImport("jxta.dll")]
        private static extern void jxta_service_get_MIA(IntPtr self, out IntPtr mia);

        [DllImport("jxta.dll")]
        private static extern void jxta_PG_get_endpoint_service(IntPtr self, out IntPtr endp);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_start(IntPtr self, String[] args);

        [DllImport("jxta.dll")]
        private static extern void jxta_module_stop(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_init(IntPtr self, IntPtr group, IntPtr assigned_id, IntPtr impl_adv);
        #endregion

        public void init(PeerGroup group, ID assignedID, Advertisement implAdv)
        {
            jxta_module_init(this.self, ((PeerGroupImpl)group).self, assignedID.self, implAdv.self);
        }

        public uint startApp(string[] args)
        {
            return jxta_module_start(this.self, args);
        }

        public void stopApp()
        {
            jxta_module_stop(this.self);
        }

        /*public EndpointService EndpointService
        {
            get
            {
                IntPtr endp = new IntPtr();
                jxta_PG_get_endpoint_service(this.self, out endp);
                return new EndpointServiceImpl(endp);
            }
        }*/

        public MembershipService MembershipService
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_membership_service(self, ref ret);
                return new MembershipServiceImpl(ret);
            }
        }

        public PeerAdvertisement PeerAdvertisement
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_PA(self, ref ret);
                return new PeerAdvertisement(ret);
            }
        }

        public string PeerGroupName
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_groupname(self, ref ret);
                return new JxtaString(ret);
            }
        }

        public DiscoveryService DiscoveryService
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_discovery_service(self, ref ret);
                return new DiscoveryServiceImpl(ret);
            }
        }

        public string PeerName
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_peername(self, ref ret);
                return new JxtaString(ret);
            }
        }

        public PeerGroupID PeerGroupID
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_GID(self, ref ret);
                return new PeerGroupIDImpl(ret);
            }
        }

        public PeerID PeerID
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_PID(self, ref ret);
                return new PeerIDImpl(ret);
            }
        }

        public PeerGroupAdvertisement PeerGroupAdvertisement
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_PGA(self, ref ret);
                return new PeerGroupAdvertisement(ret);
            }
        }

        public PipeService PipeService
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_pipe_service(self, ref ret);
                return new PipeServiceImpl(ret);
            }
        }

        public RendezVousService RendezVousService
        {
            get
            {
                IntPtr ret = new IntPtr();
                jxta_PG_get_rendezvous_service(self, ref ret);
                return new RendezVousServiceImpl(ret);
            }
        }

        public Advertisement ImplAdvertisement
        {
            get
            {
                IntPtr adv = new IntPtr();
                jxta_service_get_MIA(this.self, out adv);
                return new PeerGroupAdvertisement(adv);
            }
        }

        internal PeerGroupImpl(IntPtr self) : base(self) { }
        internal PeerGroupImpl() : base() { }

        ~PeerGroupImpl()
        {
            stopApp();
        }
    }
}
