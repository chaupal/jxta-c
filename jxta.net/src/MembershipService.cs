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
    /// <summary>
    /// Summary of MembershipService.
    /// </summary>
    public interface MembershipService : Service
    {
        /// <summary>
        /// The current credentials for this peer.
        /// </summary>
        List<Credential> CurrentCredentials
        {
            get;
        }
    }

    internal class MembershipServiceImpl : JxtaObject, MembershipService
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
		private static extern void jxta_membership_service_get_currentcreds(IntPtr self, ref IntPtr creds);

        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_membership_service_resign(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern void jxta_service_get_MIA(IntPtr self, out IntPtr mia);
        
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


        public List<Credential> CurrentCredentials
		{
            get
            {
                JxtaVector jVec = new JxtaVector();
                jxta_membership_service_get_currentcreds(this.self, ref jVec.self);

                List<Credential> ret = new List<Credential>();

                foreach (IntPtr ptr in jVec)
                    ret.Add(new Credential(ptr));

                return ret;
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

		internal MembershipServiceImpl(IntPtr self) : base(self) {}
        internal MembershipServiceImpl() : base() { }
    }
}
