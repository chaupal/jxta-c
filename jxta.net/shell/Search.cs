/*
 * Copyright (c) 2006 Sun Microsystems, Inc.  All rights reserved.
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
 * $Id: Search.cs,v 1.3 2006/02/13 22:37:38 lankes Exp $
 */
using System;
using System.Collections.Generic;
using System.Text;

using JxtaNET;

namespace JxtaNETShell
{
    class Search : ShellApp
    {
        //static Int32 timeout = 30 * 1000 * 1000;
        private enum operations { help, remote, local, flush };
        private operations operation = operations.local;
        private DiscoveryService discovery;


        /// <summary>
        /// Standart help-method; it displays the help-text.
        /// </summary>
        public override void help()
        {
            Console.WriteLine("search - discover advertisements");
            Console.WriteLine(" ");
            Console.WriteLine("SYNOPSIS");
            Console.WriteLine("    search [-p <peerid>] Searchs for advertisements at a given peer location.");
            Console.WriteLine("           [-n <num>] limit the number of responses to n from a single peer");
            Console.WriteLine("           [-r] discovers jxta advertisements using remote propagation");
            Console.WriteLine("           [-a <name>] specify Attribute name to limit discovery to");
            Console.WriteLine("           [-v <attribute>] specify Attribute value to limit discovery to. Wildcard is allowed");
            //Console.WriteLine("           [-q] specify an XPath query");
            //Console.WriteLine("           [-i] specify a file containing an XPath query");
            Console.WriteLine("           [-f] flush advertisements");
            Console.WriteLine("           [-h] print this information\n");
        }

        /// <summary>
        /// Standart run-method. Call this for the 'Search'-action.
        /// </summary>
        /// <param name="args">The commandline-parameters.</param>
        public override void run(string[] args)
        {
            String peerID = null;
            String attr = "Name";
            String val = @"*";
            Int32 num = 10;

            try 
            {
                if (args.Length == 1)
                {
                    operation = operations.local;
                }
                else
                {
                    for (int i = 1; i < args.Length; i++)
                    {
                        switch (args[i])
                        {
                            case "-p":
                                peerID = args[++i].Trim();
                                break;
                            case "-r":
                                operation = operations.remote;
                                break;
                            case "-f":
                                operation = operations.flush;
                                break;
                            case "-a":
                                operation = operations.local;
                                attr = args[++i].Trim();
                                break;
                            case "-n":
                                operation = operations.local;
                                num = Convert.ToInt32(args[++i].Trim());
                                break;
                            case "-v":
                                operation = operations.local;
                                val = args[++i].Trim();
                                break;
                            case "-h":
                                operation = operations.help;
                                break;
                            default:
                                Console.WriteLine("Error: invalid parameter");
                                return;
                        }
                    }
                }
            }
            catch
            {
                Console.WriteLine("Error: invalid parameter");
                return;
            }

            switch (operation)
            {
                case operations.remote:
                    PeerSearch(peerID, attr, val, num);
                    break;
                case operations.local:
                    PeerSearch(peerID, attr, val, num);
                    break;
                case operations.flush:
                    Console.WriteLine("Flushing...");
                    discovery.flushAdvertisements(null, DiscoveryService.DISC_ADV);
                    break;
                case operations.help:
                    help();
                    return;
            }

        }

        
        public void PeerSearch(String peerID, String attr, String val, Int32 num)
        {
            Int32 qid = -1;
            //discovery = this.netPeerGroup.getDiscoveryService();
            JxtaVector<Advertisement> JXTAVec = new JxtaVector<Advertisement>();
            if (operation == operations.remote)
            {
                Console.WriteLine("Searching for remote peers...");
                qid = discovery.getRemoteAdvertisements(DiscoveryService.DISC_PEER, null, attr, val, num, null);
                Console.WriteLine("query ID: " + qid);
            }
            else
            {
                Console.WriteLine("Searching for local peers...");
                JXTAVec = discovery.getLocalAdvertisements(DiscoveryService.DISC_PEER, attr, val);
                Console.WriteLine("Found " + JXTAVec.Length + " local Peers.");
                //for (int i = 0; i < JXTAVec.Length; i++)
                //{
                //    Console.WriteLine(i + ".: " + JXTAVec[i].ToString());
                //}
            }
        }


        public Search(PeerGroup grp) : base(grp)
        {
            discovery = this.netPeerGroup.getDiscoveryService();
        }
    }
}
