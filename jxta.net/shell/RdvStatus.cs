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
using JxtaNET;
using System.Runtime.InteropServices;
using System.IO;
using System.Collections.Generic;

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
		/// Standard help-method; it displays the help-text.
		/// </summary>
		public override void Help()
		{
			textWriter.WriteLine("rdvstatus - displays the connected state of rdvz.");
			textWriter.WriteLine("");
			textWriter.WriteLine("SYNOPSIS");
			textWriter.WriteLine("  rdvstatus");
			textWriter.WriteLine("");
			textWriter.WriteLine("DESCRIPTION");
			textWriter.WriteLine("  'rdvstatus' allows you see what rendezvous you are connected to.");
			textWriter.WriteLine("");
			textWriter.WriteLine("OPTIONS");
			textWriter.WriteLine("        [-h] print this information\n");
		}

		/// <summary>
		/// Standard run-method. Call this for the 'rdvstatus'-action.
		/// </summary>
		/// <param name="args">The commandline-parameters.</param>
		public override void Run(string[] args)
		{
            for(int i=1; i<args.Length; i++)
            {
                switch (args[i])
                {
                    case "-h":
                        Help();
                        return;
                    default:
                        textWriter.WriteLine("Error: invalid parameter" + args[i]);
                        break;
                }
            }

			RdvConfig config = rdvSvc.Configuration;

			textWriter.Write("Rendezvous service config : ");

			switch (config) 
			{
				case RdvConfig.adhoc:
					textWriter.WriteLine("ad-hoc");
					break;

				case RdvConfig.edge:
					textWriter.WriteLine("client");
					break;

				case RdvConfig.rendezvous:
					textWriter.WriteLine("rendezvous");
					break;

				default:
					textWriter.WriteLine("[unknown]");
					break;
			}

            long auto_interval = rdvSvc.AutoInterval;

            textWriter.Write("Auto-rdv check interval : ");

            if (auto_interval >= 0)
                textWriter.WriteLine(auto_interval + "ms");
            else
                textWriter.WriteLine("[disabled]");

            uint currentTime = (uint) (apr_time_now() / 1000);

            PeerView pv = rdvSvc.PeerView;

            Peer selfPVE = null;
            Peer down = null;
            Peer up = null;

            try
            {
                selfPVE = pv.SelfPeer;
                down = pv.DownPeer;
                up = pv.UpPeer;
            }
            catch (JxtaException) { }

            List<Peer> peers = null;

            /* Get the list of peers */
            try
            {
                peers = pv.LocalView;
            }
            catch (Exception)
            {
                textWriter.WriteLine("Failed getting peers");
                return;
            }

            textWriter.WriteLine("\nPeerview:");

            foreach (Peer peer in peers)
            {
                string name = "(unknown)";

                if (peer.Advertisement != null)
                    name = peer.Advertisement.Name;

                textWriter.Write(peer.ID + "/" + name + "\t");

                uint expires = rdvSvc.GetExpires(peer);

                if (expires >= currentTime)
                {
                    expires -= currentTime;

                    textWriter.WriteLine(expires + "ms");

                }
                else
                {
                    textWriter.WriteLine("(expired)");
                }

                if (down == peer)
                {
                    textWriter.Write("\t[DOWN]");
                }

                if (selfPVE == peer)
                {
                    textWriter.Write("\t[SELF]");
                }

                if (up == peer)
                {
                    textWriter.Write("\t[UP]");
                }
                textWriter.Write("\n");
            }


            textWriter.WriteLine("\nConnections:");

            peers = rdvSvc.ConnectedPeers;

            foreach (Peer peer in peers)
            {
                textWriter.WriteLine("Name: [" + peer.Advertisement.Name + "]");
                textWriter.WriteLine("PeerID: [" + peer.ID + "]\n");

                uint expires = rdvSvc.GetExpires(peer);

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


                textWriter.WriteLine("Lease expires in " + hours + " hour(s) "
                    + minutes + " minute(s) " + seconds + " second(s)");
            }

			textWriter.WriteLine("-----------------------------------------------------------------------------");
		}

        private RendezVousService rdvSvc;

        protected override void Initialize()
        {
            rdvSvc = netPeerGroup.RendezVousService;
        }
	}
}
