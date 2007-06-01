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
 * $Id: Credential.cs,v 1.1 2006/01/18 20:31:01 lankes Exp $
 */
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace JxtaNET
{
	/// <summary>
	/// Summary of Credential.
	/// </summary>
	public class Credential : JxtaObject
	{
		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_credential_get_peerid(IntPtr self, ref IntPtr pid);

		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_credential_get_peergroupid(IntPtr self, ref IntPtr gid);

		[DllImport("jxta.dll")]
        private static extern UInt32 jxta_credential_get_xml_1(IntPtr self, IntPtr appender, IntPtr xmldoc);

        [DllImport("jxta.dll")]
        private static extern IntPtr jxta_get_addr_writefunc_appender();

        /*[DllImport("kernel32")]
        private static extern int LoadLibrary(string lpLibFileName);

        [DllImport("kernel32")]
        private static extern bool FreeLibrary(int hLibModule);

        [DllImport("kernel32", CharSet = CharSet.Ansi)]
        private static extern IntPtr GetProcAddress(int hModule, string lpProcName);*/

        public string getXML()
		{
            JxtaString str = new JxtaString();

            Errors.check(jxta_credential_get_xml_1(self, writefunc_appender, str.self));

			return str;

		}

		public ID getPeerID()
		{
            IntPtr ret = new IntPtr();
			Errors.check(jxta_credential_get_peerid(self, ref ret));
            return new ID(ret);
		}

		public ID getPeerGroupID()
		{
            IntPtr ret = new IntPtr();
            Errors.check(jxta_credential_get_peergroupid(self, ref ret));
            return new ID(ret);
		}

		Credential(IntPtr self) : base(self) { }

        public Credential() : base() { }

        private static IntPtr writefunc_appender;

        static Credential()
        {
            // evaluate the address of jstring_writefunc_appender
            writefunc_appender = jxta_get_addr_writefunc_appender();

            /*
            //This version works only on a Windows system
            int hmod = LoadLibrary("jxta.dll");
            writefunc_appender = GetProcAddress(hmod, "_jstring_writefunc_appender@16");
            FreeLibrary(hmod);

            if (writefunc_appender == IntPtr.Zero)
            {
                Console.WriteLine("Error in Credential: Could not evaluate the address of jstring_writefunc_appender"); 
            }
            else 
            {
                Console.WriteLine("writefunc_appender = 0x" + writefunc_appender.ToString("X"));
            }*/
        }
	}
}
