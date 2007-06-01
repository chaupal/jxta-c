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
 * $Id: Talk.cs,v 1.4 2006/02/13 21:06:17 lankes Exp $
 */
using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using JxtaNET;
using System.Threading;

namespace JxtaNETShell
{
    class Talk : ShellApp
    {

        /// <summary>
        /// Standart help-method; it displays the help-text.
        /// </summary>
        public override void help()
        {
            Console.WriteLine("talk - talk to another JXTA user");
            Console.WriteLine(" ");
            Console.WriteLine("SYNOPSIS");
            Console.WriteLine("    talk [-t <user>] talk to a remote user.");
            Console.WriteLine("         [-l <user>] login for the local user.");
            Console.WriteLine("         [-r <user> [-p]] register a new user.");
            Console.WriteLine("         [-h] print this information");
        }

        private enum operations { help, newuser, login, talk };

        /// <summary>
        /// Standart run-method. Call this for the 'whoami'-action.
        /// </summary>
        /// <param name="args">The commandline-parameters.</param>
        public override void run(string[] args)
        {
            operations operation = operations.help;

            String username = "";
            bool propagate = false;
           
            try
            {
                for (int i = 1; i < args.Length; i++)
                {
                    switch (args[i])
                    {
                        case "-l":
                            operation = operations.login;
                            username = args[++i].Trim();
                            break;
                        case "-r":
                            operation = operations.newuser;
                            username = args[++i].Trim();
                            if ((args.Length > i) && (args[++i] == "-p"))
                                propagate = true;
                            break;
                        case "-t":
                            operation = operations.talk;
                            username = args[++i].Trim();
                            break;
                        case "-h":
                            help();
                            return;
                        default:
                            Console.WriteLine("Error: invalid parameter");
                            break;
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
                case operations.login:
                    login_user(username);
                    break;
                case operations.newuser:
                    create_user(username, propagate);
                    break;
                case operations.talk:
                    connect_to_user(username);
                    break;
            }
        }

        private void create_user(String userName, bool propagate)
        {
            ID gid = this.netPeerGroup.getPeerGroupID();

            ID pipeid = ID.newPipeID(gid);

            PipeAdvertisement adv = new PipeAdvertisement();

            adv.setID(pipeid);
            adv.setType(propagate ? "JxtaPropagate" : "JxtaUnicast");
            adv.setName("JxtaTalkUserName." + userName);

            discovery.publish(adv, 2, DiscoveryService.DEFAULT_LIFETIME, DiscoveryService.DEFAULT_LIFETIME);

            Console.WriteLine("(wait 20 sec for RDV SRDI indexing).");
            Thread.Sleep(20 * 1000);
        }

        private PipeAdvertisement get_user_adv(String userName, Int64 timeout)
        {
            //  /* Some magic hackery to always have the IP2PGRP advertisement available */

            //  /* XXX 20050923 Should be a case insensitive comparison */
            //  if (0 == strcmp("ip2pgrp", userName)) {
            //      adv = jxta_pipe_adv_new();
            //      jxta_pipe_adv_set_Name(adv, "JxtaTalkUserName.IP2PGRP");
            //      /* FIXME 20050923 This is supposed to be specific for each group. This is the netgroup pipe.  */
            //      jxta_pipe_adv_set_Id(adv, "urn:jxta:uuid-59616261646162614E50472050325033D1D1D1D1D1D141D191D191D1D1D1D1D104");
            //      jxta_pipe_adv_set_Type(adv, "JxtaPropagate");

            //      return adv;
            //  }
            String talkUserName = "JxtaTalkUserName." + userName;

            /*
             * check first our local cache
             */
            JxtaVector<PipeAdvertisement> responses = discovery.getLocalAdvertisements(DiscoveryService.DISC_ADV, "Name", talkUserName);

            PipeAdvertisement adv;

            if (responses.Length == 0)
            {


                Listener listener = new Listener(null, 1, 1);

                try
                {
                    listener.start();

                    Int32 query_id = discovery.getRemoteAdvertisements(DiscoveryService.DISC_ADV, null, "Name", talkUserName, 1, listener);

                    DiscoveryResponse dr = listener.waitForEvent(timeout);

                    listener = discovery.cancelRemoteQuery(query_id);

                    listener.stop();

                    JxtaVector<JxtaString> res = dr.getResponses();

                    if (responses.Length == 0)
                    {
                        Console.WriteLine("No responses");
                        return null;
                    }

                    /**
                     ** Builds the advertisement.
                     **/
                    adv = new PipeAdvertisement();

                    adv.parse(res[0]);
                }
                catch (JxtaException e)
                {
                    Console.WriteLine(e.ErrorMessage);
                    Console.WriteLine("No responses");
                    return null;
                }
            }
            else
                adv = responses[0];

            return adv;
        }

        static void message_listener(Message msg)//IntPtr obj, IntPtr arg)
        {
            try
            {
                //                Message msg = new Message(obj);

                StreamReader reader;

                reader = new StreamReader(msg.getElement("GrpName").getValue());

                String message = msg.getElement("JxtaTalkSenderMessage").getValueAsString();

                reader = new StreamReader(msg.getElement("JxtaTalkSenderName").getValue());
                String senderName = reader.ReadLine();

                if (!message.Equals(""))
                {
                    Console.WriteLine("\n##############################################################");
                    Console.WriteLine("CHAT MESSAGE from " + senderName + ":");
                    Console.WriteLine(message);
                    Console.WriteLine("##############################################################\n");
                }
                else
                    Console.WriteLine("Received empty message");
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        private void login_user(String userName)
        {
            PipeAdvertisement adv = get_user_adv(userName, 30 * 1000 * 1000);

            if (adv != null)
            {
                Listener listener = new Listener(message_listener, 1, 200);

                Pipe pipe = pipeservice.timedAccept(adv, 1000);

                InputPipe ip = pipe.getInputPipe();

                ip.addListener(listener);

                listener.start();
            }
            else
                Console.WriteLine("Cannot retrieve advertisement for " + userName);
        }

        private void connect_to_user(String userName)
        {
            PipeAdvertisement adv = get_user_adv(userName, 1 * 5 * 1000 * 1000);

            if (adv != null)
            {
                Pipe pipe = pipeservice.timedConnect(adv, 30 * 1000 * 1000, null);

                OutputPipe op = pipe.getOutputPipe();

                Console.WriteLine("Welcome to JXTA-C Chat, " + userName);
                Console.WriteLine("Type a '.' at begining of line to quit.");

                String userMessage = "";

                for (; ; )
                {
                    userMessage = Console.ReadLine();
                    if (userMessage != "")
                    {
                        if (userMessage[0] == '.')
                        {
                            break;
                        }
                        send_message(op, userName, userMessage);
                    }
                }

            }
        }

        private void send_message(OutputPipe op, String userName, String userMessage)
        {
            Message msg = new Message();

            ByteStream stream = new ByteStream();
            StreamWriter writer = new StreamWriter(stream);

            writer.Write(this.netPeerGroup.getPeerName());
            writer.Close();

            msg.addElement(new MessageElement("JxtaTalkSenderName", "text/plain", stream));

            msg.addElement(new MessageElement("GrpName", "text/plain", new ByteStream("NetPeerGroup")));

            msg.addElement(new MessageElement("JxtaTalkSenderMessage", "text/plain", new ByteStream(userMessage)));

            op.send(msg);
        }



        private DiscoveryService discovery;
        private PipeService pipeservice;

        /// <summary>
        /// Standart constructor.
        /// </summary>
        /// <param name="grp">the PeerGroup</param>
        public Talk(PeerGroup grp)
            : base(grp)
        {
            this.discovery = grp.getDiscoveryService();
            this.pipeservice = grp.getPipeService();
        }
    }


}
