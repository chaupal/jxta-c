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
 * $Id: Advertisement.cs,v 1.2 2006/08/04 10:33:18 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;

namespace JxtaNET
{
	/// <summary>
	/// Summary of Advertisement.
	/// </summary>
	public abstract class Advertisement : JxtaObject
    {
        #region import of jxta-c functions
        //[DllImport("jxta.dll")]
        //private static extern UInt32 jxta_advertisement_parse_charbuffer(IntPtr ad, String buf, int len);

        //[DllImport("jxta.dll")]
        //private static extern IntPtr jxta_advertisement_get_document_name(IntPtr ad);

        [DllImport("jxta.dll")]
        private static extern void jxta_advertisement_get_xml(IntPtr self, ref IntPtr jstring);

        //[DllImport("jxta.dll")]
        //private static extern IntPtr jxta_advertisement_get_id(IntPtr ad);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_advertisement_new();
        #endregion 

        /*public void Parse(string buf)
        {
            Errors.check(jxta_advertisement_parse_charbuffer(this.self, buf, buf.Length));
        }*/

        //public string GetDocumentName()
        //{
        //    return Marshal.PtrToStringAnsi(jxta_advertisement_get_document_name(self));
        //}

        /// <summary>
        /// Returns a unique ID suitable for indexing of this Advertisement.
        /// </summary>
        public abstract ID ID
        {
            get;
        }

        public abstract void ParseXML(string xml);

        /// <summary>
        /// Return a string representaion of this advertisement. The string will contain 
        /// the advertisement formated as a UTF-8 encoded XML Document.
        /// </summary>
        /// <returns>A String containing the advertisement.</returns>
		public override string ToString()
		{
            IntPtr ret = new IntPtr();
            jxta_advertisement_get_xml(this.self, ref ret);
            return new JxtaString(ret);
		}

        internal Advertisement(IntPtr self)
            : base(self)
        {
        }

        internal Advertisement()
            : base(jxta_advertisement_new())
        {
        }
	}
}
