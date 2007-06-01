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
 * $Id: Peers.cs,v 1.2 2006/02/13 21:06:16 lankes Exp $
 */
using System;
using JxtaNET;

namespace JxtaNETShell
{
    /// <summary>
    /// Peers is a ShellApp for the jxtaC#-Shell.
    /// 'peers' allows you discover and display peers.
    /// </summary>
    public class Peers : ShellApp
    {
        /// <summary>
        /// Standart help-method; it displays the help-text.
        /// </summary>
        public override void help()
        {
            Console.WriteLine("peers - discover peers \n");
            Console.WriteLine("SYNOPSIS");
            Console.WriteLine("  peers " +/*[-p peerid name attribute]");
			Console.WriteLine("        */
                                         "[-n n] limit the number of responses to n from a single peer");
            Console.WriteLine("        [-r] discovers peer groups using remote propagation");
            Console.WriteLine("        [-a] specify Attribute name to limit discovery to");
            Console.WriteLine("        [-v] specify Attribute value to limit discovery to. wild cards allowed");
            Console.WriteLine("        [-f] flush peer advertisements");
            Console.WriteLine("        [-h] print this information");
        }

        /// <summary>
        /// Standart run-method. Call this for the 'rdvstatus'-action.
        /// </summary>
        /// <param name="args">The commandline-parameters.</param>
        public override void run(string[] args)
        {
            DiscoveryService dis = netPeerGroup.getDiscoveryService();

            bool remote = false;
            bool flush = false;

            string attr = "";
            string val = "";
            //			string peer = "";

            int responses = 10;

            try
            {
                for (int i = 1; i < args.Length; i++)
                {
                    switch (args[i])
                    {
                        case "-r":
                            remote = true;
                            break;
                        case "-f":
                            flush = true;
                            break;
                        case "-h":
                            help();
                            return;
                        case "-a":
                            attr = args[++i].Trim();
                            break;
                        case "-v":
                            val = args[++i].Trim();
                            break;
                        case "-n":
                            responses = Int32.Parse(args[++i].Trim());
                            break;
                        /*case "p": 
                         * peer = args[++i].Trim(); 
                         * break; */
                        default:
                            Console.WriteLine("Error: invalid parameter");
                            return;
                    }
                }
            }
            catch
            {
                Console.WriteLine("Error: invalid parameter");
                return;
            }

            if (flush)
            {
                dis.flushAdvertisements(null, DiscoveryService.DISC_PEER);
                return;
            }

            if (remote)
            {
                dis.getRemoteAdvertisements(DiscoveryService.DISC_PEER, null, attr, val, responses, null);

                System.Console.WriteLine("peer discovery message send");
                return;
            }

            JxtaVector<PeerAdvertisement> vec = dis.getLocalAdvertisements(DiscoveryService.DISC_PEER, attr, val);

            for (int i = 0; i < vec.Length; i++)
                System.Console.WriteLine("peer" + i + ": " + (vec[i]).getName());

            if (vec.Length == 0) 
                Console.WriteLine("No peer advertisements retrieved");
        }


        /// <summary>
        /// Standart constructor.
        /// </summary>
        /// <param name="grp">the PeerGroup</param>
        public Peers(PeerGroup grp) : base(grp) { }


    }
}
