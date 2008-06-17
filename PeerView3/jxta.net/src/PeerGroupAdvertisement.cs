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
 * $Id: PeerGroupAdvertisement.cs,v 1.2 2006/08/04 10:33:20 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of PeerGroupAdvertisment.
	/// </summary>
	public class PeerGroupAdvertisement : Advertisement
    {
        #region import jxta-c functions
        [DllImport("jxta.dll")]
		private static extern IntPtr jxta_PGA_get_Name(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern void jxta_PGA_set_Name(IntPtr ad, IntPtr name);

		[DllImport("jxta.dll")]
		private static extern IntPtr jxta_PGA_get_MSID(IntPtr self);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_PGA_get_Desc(IntPtr ad);

        [DllImport("jxta.dll")]
        private static extern void jxta_PGA_set_Desc(IntPtr ad, IntPtr desc);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_advertisement_get_id(IntPtr ad);

        [DllImport("jxta.dll")]
        private static extern void jxta_PGA_parse_charbuffer(IntPtr ad, String buf, Int32 len);
        #endregion

        public ModuleSpecID GetModuleSpecID()
		{
            IntPtr ret = jxta_PGA_get_MSID(this.self);

            return new ModuleSpecIDImpl(ret);
		}

        public override void ParseXML(string xml)
        {
            jxta_PGA_parse_charbuffer(this.self, xml, xml.Length);
        }

        public override ID ID
        {
            get
            {
                IntPtr ret = jxta_advertisement_get_id(self);
                return new PeerGroupIDImpl(ret);
            }
        }

        /// <summary>
        /// Name of the peergroup
        /// </summary>
        public String Name
        {
            get
            {
                return new JxtaString(jxta_PGA_get_Name(this.self));
            }
            set
            {
                jxta_PGA_set_Name(this.self, new JxtaString(value).self);
            }
        }

		public String Description
		{
            get
            {
                return new JxtaString(jxta_PGA_get_Desc(this.self));
            }
            set
            {
                jxta_PGA_set_Desc(this.self, new JxtaString(value).self);
            }
		}

        internal PeerGroupAdvertisement(IntPtr self) : base(self) { }
        internal PeerGroupAdvertisement() : base() { }
    }
}
