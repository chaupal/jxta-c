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
using System.Text;

namespace JxtaNET
{
    /// <summary>
    /// Provides an asynchronous mechanism for 
    /// discovering Advertisement (Peers, Groups, Pipes, Modules, etc.). 
	/// </summary>
    public abstract class DiscoveryService : JxtaObject, Service
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_PEER();

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_GROUP();

        [DllImport("jxta.dll")]
        private static extern Int32 jxta_get_DISC_ADV();

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_get_DEFAULT_LIFETIME();

        [DllImport("jxta.dll")]
        private static extern Int64 jxta_get_DEFAULT_EXPIRATION();
        #endregion 

        // some necessary constants, which are previously defined in JXTA-C
        public static readonly short DISC_PEER;
        public static readonly short DISC_GROUP;
        public static readonly short DISC_ADV;
        public static readonly long DEFAULT_LIFETIME;
        public static readonly long DEFAULT_EXPIRATION;

        public abstract Advertisement ImplAdvertisement
        {
            get;
        }

        /// <summary>
        /// Publish an Advertisement. The Advertisement will expire automatically on the local peer after DEFAULT_LIFETIME 
        /// and will expire on other peers after DEFAULT_EXPIRATION.
        /// </summary>
        /// <param name="adv">The Advertisement to publish.</param>
        public abstract void Publish(Advertisement adv);

        /// <summary>
        /// Publish an Advertisement. The Advertisement will expire automatically after the specified time. A peer that discovers 
        /// this advertisement will hold it for about expiration or lifetime milliseconds, whichever is smaller.
        /// </summary>
        /// <param name="adv">The Advertisement to publish.</param>
        /// <param name="lifetime">Duration in relative milliseconds that this advertisement will exist.</param>
        /// <param name="expiration">Duration in relative milliseconds that this advertisement will be cached by other peers.</param>
        public abstract void Publish(Advertisement adv, long lifetime, long expiration);

        /// <summary>
        /// Retrieve Stored Peer, Group, and General JXTA Advertisements
        /// </summary>
        /// <param name="type">DISC_PEER, DISC_GROUP, or DISC_ADV</param>
        /// <param name="attr">attribute name to narrow disocvery to</param>
        /// <param name="value">value of attribute to narrow disocvery to</param>
        /// <returns></returns>
        public abstract List<Advertisement> GetLocalAdvertisements(int type, string attr, string value);// where T : Advertisement;

        /// <summary>
        /// Discover advertisements from remote peers. The scope of advertisements returned 
        /// can be narrowed by specifying an attribute and value pair. You may also limit the number of responses any single peer may return. 
        /// </summary>
        /// <param name="peerid"><he ID of a peer which will receive the query or null in order to propagate the query/param>
        /// <param name="type">Discovery type PEER, GROUP, ADV</param>
        /// <param name="attr">indexed element name (see advertisement(s) for a list of indexed fields. 
        /// A null attribute indicates any advertisement of specified type</param>
        /// <param name="val">value of attribute to narrow discovery to valid values for this 
        /// parameter are null (don't care), Exact value, or use of wild card(s) 
        /// (e.g. if a Advertisement defines FooBar , a value of "*bar", "foo*", or "*ooB*", 
        /// will return the Advertisement</param>
        /// <param name="threshold">The upper limit of responses from each peer responding. 
        /// threshold of 0, and type of PEER has a special behaviour</param>
        /// <param name="listener">The listener which will be called when advertisement which match this query are 
        /// discovered or null if no callback is desired.</param>
        /// <returns>query ID for this discovery query.</returns>
        public abstract int GetRemoteAdvertisements(string peerid, int type, string attr, string val, int threshold, DiscoveryListener listener);

        /// <summary>
        /// Discover advertisements from remote peers. The scope of advertisements returned 
        /// can be narrowed by specifying an attribute and value pair. You may also limit the number of responses any single peer may return. 
        /// </summary>
        /// <param name="peerid"><he ID of a peer which will receive the query or null in order to propagate the query/param>
        /// <param name="type">Discovery type PEER, GROUP, ADV</param>
        /// <param name="attr">indexed element name (see advertisement(s) for a list of indexed fields. 
        /// A null attribute indicates any advertisement of specified type</param>
        /// <param name="val">value of attribute to narrow discovery to valid values for this 
        /// parameter are null (don't care), Exact value, or use of wild card(s) 
        /// (e.g. if a Advertisement defines FooBar , a value of "*bar", "foo*", or "*ooB*", 
        /// will return the Advertisement</param>
        /// <param name="threshold">The upper limit of responses from each peer responding. 
        /// threshold of 0, and type of PEER has a special behaviour</param>
        /// <returns>query ID for this discovery query.</returns>
        public abstract int GetRemoteAdvertisements(string peerid, int type, string attr, string val, int threshold);

        /// <summary>
        /// flushs stored Advertisement
        /// </summary>
        /// <param name="id">id of the advertisement to flush</param>
        /// <param name="type">DISC_PEER, DISC_GROUP, or DISC_ADV</param>
        public abstract void FlushAdvertisements(string id, int type);

        /// <summary>
        /// Register a Discovery listener. The Discovery listener will be called whenever 
        /// Advertisement responses are received from remote peers by the Discovery Service.
        /// </summary>
        /// <param name="dListener">The DiscoveryListener</param>
        public abstract void AddDiscoveryListener(DiscoveryListener dListener);

        /// <summary>
        /// Remove a Discovery listener which was previously registered with GetRemoteAdvertisements() 
        /// or addDiscoveryListener().
        /// </summary>
        /// <param name="dListener">The listener to be removed.</param>
        /// <returns>true if the listener was successfully removed, false otherwise</returns>
        public abstract bool RemoveDiscoveryListener(DiscoveryListener dListener);

        public abstract uint startApp(string[] args);

        public abstract void stopApp();

        public abstract void init(PeerGroup group, ID assignedID, Advertisement implAdv);

        internal DiscoveryService(IntPtr self) : base(self) {}
        internal DiscoveryService() : base() { }

        static DiscoveryService()
        {
            // intialization of the constants, which are previously defined in JXTA-C
            DISC_PEER = (short)jxta_get_DISC_PEER();
            DISC_GROUP = (short)jxta_get_DISC_GROUP();
            DISC_ADV = (short)jxta_get_DISC_ADV();
            DEFAULT_LIFETIME = jxta_get_DEFAULT_LIFETIME();
            DEFAULT_EXPIRATION = jxta_get_DEFAULT_EXPIRATION();
        }
    }

	internal class DiscoveryServiceImpl : DiscoveryService
    {
        #region import of jxta-c functions
        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_publish(IntPtr service, IntPtr adv, short type, Int64 lifetime, Int64 lifetimeForOthers);

        //[DllImport("jxta.dll")]
        //private static extern UInt32 discovery_service_cancel_remote_query(IntPtr service, Int32 query_id, ref IntPtr listener);

        //[DllImport("jxta.dll")]
        //private static extern Int32 discovery_service_remote_query(IntPtr self, String paddr, String query, Int32 responses, IntPtr ptr);

        //[DllImport("jxta.dll")]
        //private static extern UInt32 discovery_service_local_query(IntPtr self, String query, ref IntPtr res_vec);

        [DllImport("jxta.dll")]
        private static extern Int32 discovery_service_get_remote_advertisements(IntPtr self,
            String paddr, Int32 disc_peer, String attr, String value, Int32 responses, IntPtr ptr);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_get_local_advertisements(IntPtr _this,
            Int32 i, String attr, String value, ref IntPtr res_vec);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_flush_advertisements(IntPtr self, IntPtr ptr, Int32 i);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_add_discovery_listener(IntPtr self, IntPtr listener);

        [DllImport("jxta.dll")]
        private static extern UInt32 discovery_service_remove_discovery_listener(IntPtr self, IntPtr listener);

        [DllImport("jxta.dll")]
        private static extern void jxta_service_get_MIA(IntPtr self, out IntPtr mia);
        
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_start(IntPtr self, String[] args);

        [DllImport("jxta.dll")]
        private static extern void jxta_module_stop(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_module_init(IntPtr self, IntPtr group, IntPtr assigned_id, IntPtr impl_adv);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_vector_new(UInt32 initialSize);

        [DllImport("jxta.dll")]
        private static extern void jxta_advertisement_get_xml(IntPtr adv, ref IntPtr jxtaString);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_advertisement_get_document_name(IntPtr ad);
        #endregion

        public override void init(PeerGroup group, ID assignedID, Advertisement implAdv)
        {
            jxta_module_init(this.self, ((PeerGroupImpl)group).self, assignedID.self, implAdv.self);
        }

        public override uint startApp(string[] args)
        {
            return jxta_module_start(this.self, args);
        }

        public override void stopApp()
        {
            jxta_module_stop(this.self);
        }

        public override void Publish(Advertisement adv)
        {
            Publish(adv, DEFAULT_LIFETIME, DEFAULT_EXPIRATION);
        }

        public override void Publish(Advertisement adv, long lifetime, long expiration)
        {
            Errors.check(discovery_service_publish(this.self, adv.self, DISC_ADV, lifetime, expiration));
        }

        public void Publish(Advertisement adv, short type, long lifetime, long lifetimeForOthers)
        {
            Errors.check(discovery_service_publish(this.self, adv.self, type, lifetime, lifetimeForOthers));
        }

		public override List<Advertisement> GetLocalAdvertisements(int type, string attr, string value)
		{
            JxtaVector advs = new JxtaVector();
            Errors.check(discovery_service_get_local_advertisements(self, type, attr, value, ref advs.self));

            List<Advertisement> list = new List<Advertisement>();

            for (int i = 0; i < advs.Length; i++)
            {
                string advType = Marshal.PtrToStringAnsi(jxta_advertisement_get_document_name(advs[i]));

                advType = advType.Replace("jxta:", "");

                if (advType == "PA")
                    advType = "PeerAdvertisement";

                Type instanceType = 
                    System.Reflection.Assembly.GetExecutingAssembly().GetType("JxtaNET." + advType, false, true);

                Advertisement adv = (Advertisement)System.Activator.CreateInstance(instanceType,
                    new object[] { advs[i] });

                list.Add(adv);
            }

            return list;
		}

        private List<Listener<DiscoveryEventImpl>> list_listener = new List<Listener<DiscoveryEventImpl>>();

        public override int GetRemoteAdvertisements(string peerid, int type, string attr, string val, int threshold, DiscoveryListener dlistener)
		{
            if (dlistener == null)
                return GetRemoteAdvertisements(peerid, type, attr, val, threshold);

            Listener<DiscoveryEventImpl> listener = new Listener<DiscoveryEventImpl>(dlistener.DiscoveryEvent, 1, 200);
            
            int queryID = discovery_service_get_remote_advertisements(self, peerid, type, attr, val, threshold, listener.self);

            listener.Start();
            list_listener.Add(listener);

            return queryID;
        }

        public override int GetRemoteAdvertisements(string peerid, int type, string attr, string val, int threshold)
        {
            return discovery_service_get_remote_advertisements(self, peerid, type, attr, val, threshold, IntPtr.Zero);
        }

        /*public Int32 RemoteQuery(String peerid, String query, Int32 responses, Listener<DiscoveryResponse> listener)
        {
            IntPtr tmp;

            if (listener != null)
                tmp = listener.self;
            else
                tmp = IntPtr.Zero;

            return discovery_service_remote_query(self, peerid, query, responses, tmp);
        }*/

        /// <summary>
        /// Query the local cache with an XPath type query
        /// </summary>
        /// <param name="query">an XPath type like query</param>
        /// <returns></returns>
        /*public JxtaVector<Advertisement> LocalQuery(String query)
        {
            IntPtr ptr = new IntPtr();

            Errors.check(discovery_service_local_query(self, query, ref ptr));

            JxtaVector<T> ret = new JxtaVector<T>(ptr);

            return ret;
        }*/

        public override void FlushAdvertisements(string id, int type)
        {
            IntPtr ptr = Marshal.StringToHGlobalAnsi(id);
            discovery_service_flush_advertisements(self, ptr, type);
            Marshal.FreeHGlobal(ptr);
		}

        /// <summary>
        /// Stop process responses for remote queries issued earlier by calling either GetRemoteAdvertisements or RemoteQuery. 
        /// </summary>
        /// <param name="query_id">the query ID returned by previous call to issue remote query</param>
        /// <returns></returns>
        /*public Listener<DiscoveryResponse> CancelRemoteQuery(Int32 query_id)
        {
            IntPtr ret = new IntPtr();
            discovery_service_cancel_remote_query(this.self, query_id, ref ret);
            return new Listener<DiscoveryResponse>(ret);
        }*/

        public override void AddDiscoveryListener(DiscoveryListener dlistener)
        {
            Listener<DiscoveryEventImpl> listener = new Listener<DiscoveryEventImpl>(dlistener.DiscoveryEvent, 1, 200);

            uint status = discovery_service_add_discovery_listener(this.self, listener.self);
            if (status != Errors.JXTA_SUCCESS)
                throw new JxtaException(status);

            listener.Start();
            list_listener.Add(listener);
        }

        public override bool RemoveDiscoveryListener(DiscoveryListener dlistener)
        {
            for (int i = 0; i < list_listener.Count; i++)
            {
                if (list_listener[i].listener == (Listener<DiscoveryEventImpl>.ListenerFunction) dlistener.DiscoveryEvent)
                {
                    list_listener[i].Stop();
                    list_listener.RemoveAt(i);
                    discovery_service_add_discovery_listener(this.self, list_listener[i].self);
                    return true;
                }
            }

            return false;
        }

        public override Advertisement ImplAdvertisement
        {
            get
            {
                IntPtr adv = new IntPtr();
                jxta_service_get_MIA(this.self, out adv);
                return new ModuleAdvertisement(adv);
            }
        }

		internal DiscoveryServiceImpl(IntPtr self) : base(self) {}
        internal DiscoveryServiceImpl() : base() { }

        ~DiscoveryServiceImpl()
        {
            foreach(Listener<DiscoveryEventImpl> i in list_listener)
                i.Stop();
        }
	}
}
