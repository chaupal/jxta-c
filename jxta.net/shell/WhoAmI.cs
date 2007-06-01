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
 * $Id: WhoAmI.cs,v 1.2 2006/02/13 21:06:17 lankes Exp $
 */
using System;
using JxtaNET;

namespace JxtaNETShell
{
	/// <summary>
	/// Summary of WhoAmI.
	/// </summary>
	public class WhoAmI : ShellApp
	{

		/// <summary>
		/// Standart help-method; it displays the help-text.
		/// </summary>
		public override void help()
		{
			Console.WriteLine("\nwhoami  - Display information about the local peer.");
			Console.WriteLine("SYNOPSIS");
			Console.WriteLine("\twhoami [-h] [-p] [-g] [-c]\n");
			Console.WriteLine("DESCRIPTION");
			Console.WriteLine("'whoami' displays information about the local peer and optionally the group " + 
				"the peer is a member of. The display can present either the raw XML advertisement or " + 
				"a \"pretty printed\" summary.\n");
			Console.WriteLine("OPTIONS");
			Console.WriteLine("\t-c\tinclude credential information.");
			Console.WriteLine("\t-g\tinclude group information.");
			Console.WriteLine("\t-h\tprint this help information.");
			Console.WriteLine("\t-p\t\"pretty print\" the output.");
		}

		/// <summary>
		/// Standart run-method. Call this for the 'whoami'-action.
		/// </summary>
		/// <param name="args">The commandline-parameters.</param>
		public override void run(string[] args)
		{

			bool printpretty = false;
			bool printgroup = false;
			bool printcreds = false;

			for (int i = 1; i < args.Length; i++)
			{
				switch (args[i])
				{
					case "-g":
						printgroup = true;
						break;
					case "-c":
						printcreds = true;
						break;
					case "-p":
						printpretty = true;
						break;
					case "-h": 
						help();
                        return;
                    default:
                        Console.WriteLine("Error: invalid parameter");
                        return;
				}
			}

			PeerGroupAdvertisement myGroupAdv = this.netPeerGroup.getPeerGroupAdvertisement();
			if (myGroupAdv == null) 
			{
				Console.WriteLine("# ERROR - Invalid peer group advertisement\n");
				return;
			}
			PeerAdvertisement myAdv = this.netPeerGroup.getPeerAdvertisement();
			if (myAdv == null) 
			{
				Console.WriteLine("# ERROR - Invalid peer advertisement\n");
				return;
			}

			if (printpretty && printgroup) 
			{

				ID gid = myGroupAdv.getID();
				ID msid = myGroupAdv.getMSID();
				string name = myGroupAdv.getName();
				string desc = myGroupAdv.getDescription();

				Console.WriteLine("\nGroup : \n");
				Console.WriteLine("  Name : " + name);
				Console.WriteLine("  Desc : " + desc);
				Console.WriteLine("  GID  : " + gid);
				Console.WriteLine("  MSID : " + msid + "\n");
			} 
			else if (printgroup) 
			{
				Console.WriteLine(myGroupAdv.getXML());
			}

			/* peer adv part */
			if (printpretty) 
			{
				ID pid = myAdv.getID();
				ID gid = myAdv.getGID();
				string name = myAdv.getName();

				Console.WriteLine("\nPeer :");
				Console.WriteLine("  Name : " + name);
				Console.WriteLine("  PID : " + pid);
				Console.WriteLine("  GID : " + gid + "\n");
			} 
			else 
			{
				Console.WriteLine(myAdv.getXML());
			}

			/* credentials part */
			if (printcreds) 
			{
				if (printpretty)
					Console.WriteLine("Credentials:\n");

				MembershipService membership = this.netPeerGroup.getMembershipService();

				if (membership == null) 
				{
					Console.WriteLine("# invalid membership service\n");
					return;
				}

				JxtaVector<Credential> creds = membership.getCurrentCredentials();

				if (creds == null) 
				{
					Console.WriteLine("# could not get credentials.\n");
					return;
				}

				for (int i = 0; i < creds.Length; i++)
				{
                    if (creds[i] == null) 
					{
						Console.WriteLine("# could not get credential\n");
						continue;
					}

					if (printpretty) 
					{
						Console.WriteLine("Credential #" + i + ": \n");
                        Console.WriteLine("  PID : " + creds[i].getPeerID());
                        Console.WriteLine("  GID : " + creds[i].getPeerGroupID());

					} 
					else 
					{
                        Console.WriteLine("Cred:\n" + creds[i].getXML());
					}
				}  
			}	 
		}


		/// <summary>
		/// Standart constructor.
		/// </summary>
		/// <param name="grp">the PeerGroup</param>
		public WhoAmI(PeerGroup grp) : base(grp)	{}

	}
}
