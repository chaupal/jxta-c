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
 * $Id: RdvStatus.cs,v 1.2 2006/02/13 21:06:16 lankes Exp $
 */
using System;
using JxtaNET;
using System.Runtime.InteropServices;

namespace JxtaNETShell
{
	/// <summary>
	/// RdvStatus is a ShellApp for the jxtaC#-Shell.
	/// 'rdvstatus' allows you see what rendezvous you are connected to.
	/// </summary>
	public class RdvStatus : ShellApp
	{
        [DllImport("libapr-1.dll")]
        public static extern ulong apr_time_now();

		/// <summary>
		/// Standart help-method; it displays the help-text.
		/// </summary>
		public override void help()
		{
			System.Console.WriteLine("rdvstatus - displays the connected state of rdvz.");
			Console.WriteLine("");
			Console.WriteLine("SYNOPSIS");
			Console.WriteLine("  rdvstatus");
			Console.WriteLine("");
			Console.WriteLine("DESCRIPTION");
			Console.WriteLine("  'rdvstatus' allows you see what rendezvous you are connected to.");
			Console.WriteLine("");
			Console.WriteLine("OPTIONS");
			Console.WriteLine("        [-h] print this information\n");
		}

		/// <summary>
		/// Standart run-method. Call this for the 'rdvstatus'-action.
		/// </summary>
		/// <param name="args">The commandline-parameters.</param>
		public override void run(string[] args)
		{
            for(int i=1; i<args.Length; i++)
            {
                switch (args[i])
                {
                    case "-h":
                        help();
                        return;
                    default:
                        Console.WriteLine("Error: invalid parameter");
                        return;
                }
            }

			JxtaNET.RendezVousService rdvSvc = netPeerGroup.getRendezVousService();

			RdvConfig config = rdvSvc.getConfiguration();

			Console.Write("Rendezvous service config : ");

			switch (config) 
			{
				case RdvConfig.adhoc:
					Console.WriteLine("ad-hoc");
					break;

				case RdvConfig.edge:
					Console.WriteLine("client");
					break;

				case RdvConfig.rendezvous:
					Console.WriteLine("rendezvous");
					break;

				default:
					Console.WriteLine("[unknown]");
					break;
			}

			long auto_interval = rdvSvc.getAutoInterval();

			Console.Write("Auto-rdv check interval : ");

			if (auto_interval >= 0) {
				Console.WriteLine(auto_interval + "ms");
			} else {
				Console.WriteLine("[disabled]");
			}

		
			uint currentTime = (uint)(apr_time_now()/1000);

			PeerView pv = rdvSvc.getPeerView();

            Peer selfPVE = null;
            Peer down = null;
            Peer up = null;

            try
            {
                selfPVE = pv.getSelfPeer();
                down = pv.getDownPeer();
                up = pv.getUpPeer();
            }
            catch (JxtaException) { }

			JxtaVector<Peer> peers = null;

			/* Get the list of peers */
			try
			{
				peers = pv.getLocalView();
			}
			catch (Exception)
			{
				Console.WriteLine("Failed getting peers");
				return;
			}

			Console.WriteLine("\nPeerview:");

			foreach (Peer peer in peers)
			{
				string name = "(unknown)";
				try 
				{
					name = peer.getAdvertisement().getName();
				}
				catch (Exception)
				{
				}

				Console.Write(peer.getID() + "/" + name + "\t");

				uint expires = rdvSvc.getExpires(peer);

				if (expires >= currentTime) {
					expires -= currentTime;

					Console.WriteLine(expires + "ms");

				} else {
					Console.WriteLine("(expired)");
				}

				if (down == peer) 
				{
					Console.Write("\t[DOWN]");
				}

				if (selfPVE == peer) 
				{
					Console.Write("\t[SELF]");
				}

				if (up == peer) 
				{
					Console.Write("\t[UP]");
				}
				Console.Write("\n");
			}
		

			Console.WriteLine("\nConnections:");

			peers = rdvSvc.getPeers();

			foreach (Peer peer in peers)
			{
					Console.WriteLine("Name: [" + peer.getAdvertisement().getName() + "]");
					Console.WriteLine("PeerID: [" + peer.getID() + "]\n");

					uint expires = rdvSvc.getExpires(peer);
					
					uint hours = 0;
					uint minutes = 0;
					uint seconds = 0;

					
					if (expires < currentTime) 
					{
						expires = 0;
					} 
					else 
					{
						expires -= currentTime;
					}

					seconds = expires / (1000);

					hours = seconds / (60 * 60);
					seconds -= hours * 60 * 60;

					minutes = seconds / 60;
					seconds -= minutes * 60;


					Console.WriteLine("Lease expires in " + hours + " hour(s) " 
						+ minutes + " minute(s) " + seconds + " second(s)");
			}

			Console.WriteLine("-----------------------------------------------------------------------------");
		}

		/// <summary>
		/// Standart constructor.
		/// </summary>
		/// <param name="grp">the PeerGroup</param>
		public RdvStatus(PeerGroup grp) : base(grp)	{}
	}
}
