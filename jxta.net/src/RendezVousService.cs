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
 * $Id$
 */
using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace JxtaNET
{
	public enum RdvConfig
	{
		adhoc,
		edge,
		rendezvous
	};

    /// <summary>
	/// Summary of RendezVousService.
	/// </summary>
    public interface RendezVousService : Service
    {
        /// <summary>
        /// Vector of peers used by the Rendezvous Service.
        /// </summary>
        List<Peer> ConnectedPeers
        {
            get;
        }

        /// <summary>
        /// Test if the given peer is connected
        /// </summary>
        /// <param name="peer">peer to test upon</param>
        /// <returns></returns>
        bool IsConnected(Peer peer);

        /// <summary>
        /// Return the time in absolute milliseconds time at which the provided peer will expire if not renewed.
        /// </summary>
        /// <param name="peer">peer to test upon</param>
        /// <returns>time in absolute milliseconds</returns>
        uint GetExpires(Peer peer);

        /// <summary>
        /// The current rendezvous configuration of this peer within the current peer group.
        /// </summary>
        RdvConfig Configuration
        {
            get;
            set;
        }

         /// <summary>
        /// Interval at which this peer dynamically reconsiders its'
        /// rendezvous configuration within the current group.
        /// </summary>
        long AutoInterval
        {
            get;
            set;
        }

        /// <summary>
        /// Peer view of the current peer
        /// </summary>
        PeerView PeerView
        {
            get;
        }
    }

	internal class RendezVousServiceImpl : JxtaObject, RendezVousService
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_rdv_service_get_peers(IntPtr self, ref IntPtr peers);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_rdv_service_peer_is_connected(IntPtr self, IntPtr peers);

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_rdv_service_config(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_rdv_service_auto_interval(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_rdv_service_get_peerview(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_rdv_service_peer_get_expires(IntPtr self, IntPtr peer);

        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_get_config_adhoc();
        
        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_get_config_edge();
        
        //[DllImport("jxta.dll")] 
        //private static extern UInt32 jxta_get_config_rendezvous();

        [DllImport("jxta.dll")]
        private static extern void jxta_service_get_MIA(IntPtr self, out IntPtr mia);

        [DllImport("jxta.dll")]
        private static extern void jxta_rdv_service_set_auto_interval(IntPtr self, Int64 interval);

        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_rdv_service_set_config(IntPtr self, RdvConfig config);

        [DllImport("jxta.dll")] 
        private static extern UInt32 jxta_module_start(IntPtr self, String[] args);

        [DllImport("jxta.dll")] 
        private static extern void jxta_module_stop(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_init(IntPtr self, IntPtr group, IntPtr assigned_id, IntPtr impl_adv);
        #endregion

        public void init(PeerGroup group, ID assignedID, Advertisement implAdv)
        {
            jxta_module_init(this.self, ((PeerGroupImpl) group).self, assignedID.self, implAdv.self);
        }

        public uint startApp(string[] args)
        {
            return jxta_module_start(this.self, args);
        }

        public void stopApp()
        {
            jxta_module_stop(this.self);
        }

        public List<Peer> ConnectedPeers
		{
            get
            {
                JxtaVector jVec = new JxtaVector();
                Errors.check(jxta_rdv_service_get_peers(self, ref jVec.self));

                List<Peer> peers = new List<Peer>();

                foreach (IntPtr ptr in jVec)
                    peers.Add(new Peer(ptr));

                return peers;
            }
		}

		public bool IsConnected(Peer peer)
		{
            if (jxta_rdv_service_peer_is_connected(self, peer.self) == 0)
                return false;
            else
                return true;
		}

		public uint GetExpires(Peer peer)
		{
			return jxta_rdv_service_peer_get_expires(this.self, peer.self);
		}

		public RdvConfig Configuration
		{
            get
            {
                int conf = jxta_rdv_service_config(this.self);

                if (conf == jxta_get_config_adhoc())
                    return RdvConfig.adhoc;

                if (conf == jxta_get_config_edge())
                    return RdvConfig.edge;

                return RdvConfig.rendezvous;
            }
            set
            {
                jxta_rdv_service_set_config(this.self, value);

            }
		}

        public long AutoInterval
		{
            get
            {
			    return jxta_rdv_service_auto_interval(this.self);
            }
            set
            {
                jxta_rdv_service_set_auto_interval(this.self, value);
            }
		}

		public PeerView PeerView
		{
            get
            {
                return new PeerView(jxta_rdv_service_get_peerview(this.self));
            }
		}

        public Advertisement ImplAdvertisement
        {
            get
            {
                IntPtr adv = new IntPtr();
                jxta_service_get_MIA(this.self, out adv);
                return new ModuleAdvertisement(adv);
            }
        }

		internal RendezVousServiceImpl(IntPtr self) : base(self) {}
        //internal RendezVousServiceImpl() : base() { }
    }
}
