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
using System.IO;
using System.Collections.Generic;
using System.Text;
using JxtaNET;
using System.Threading;

namespace JxtaNETShell
{
    /// <summary>
    /// Class of the shell command "talk".
    /// </summary>
    public class Talk : ShellApp
    {

        /// <summary>
        /// Standard help-method; it displays the help-text.
        /// </summary>
        public override void Help()
        {
            textWriter.WriteLine("talk - talk to another JXTA user");
            textWriter.WriteLine(" ");
            textWriter.WriteLine("SYNOPSIS");
            textWriter.WriteLine("    talk [-u <me> <you>] talk to a remote user.");
            textWriter.WriteLine("         [-login <user>] login for the local user.");
            textWriter.WriteLine("         [-logout <user>] logout the local user.");
            textWriter.WriteLine("         [-register <user> [-p]] register a new user.");
            textWriter.WriteLine("         [-h] print this information");
        }

        private enum operations { help, newuser, login, logout, talk };

        /// <summary>
        /// Standard run-method. Call this for the 'talk'-action.
        /// </summary>
        /// <param name="args">The commandline-parameters.</param>
        public override void Run(string[] args)
        {
            operations operation = operations.help;

            string username = "";
            string myName = "";
            bool propagate = false;
           
            try
            {
                for (int i = 1; i < args.Length; i++)
                {
                    switch (args[i])
                    {
                        case "-lo":
                        case "-logout":
                            operation = operations.logout;
                            username = args[++i].Trim();
                            break;
                        case "-l":
                        case "-login":
                            operation = operations.login;
                            username = args[++i].Trim();
                            break;
                        case "-r":
                        case "-register":
                            operation = operations.newuser;
                            username = args[++i].Trim();
                            if ((args.Length > i+1) && (args[++i] == "-p"))
                                propagate = true;
                            break;
                        case "-u":
                            operation = operations.talk;
                            myName = args[++i].Trim();
                            username = args[++i].Trim();
                            break;
                        case "-h":
                            Help();
                            return;
                        default:
                            textWriter.WriteLine("Error: invalid parameter");
                            break;
                    }
                }
            }
            catch
            {
                textWriter.WriteLine("Error: invalid parameter");
                return;
            }

            switch (operation)
            {
                case operations.logout:
                    LogoutUser(username);
                    break;
                case operations.login:
                    LoginUser(username, new TalkMessageListener(textWriter));
                    break;
                case operations.newuser:
                    CreateUser(username, propagate);
                    textWriter.WriteLine("(wait 20 sec for RDV SRDI indexing).");
                    for (int i = 1; i < 20; i++)
                    {
                        Thread.Sleep(1000);
                        textWriter.Write(".");
                    }
                    textWriter.WriteLine();
                    break;
                case operations.talk:
                    TalkToUser(myName, username);
                    break;
                default:
                    textWriter.WriteLine("Error: invalid parameter");
                    break;
            }
        }

       public void CreateUser(string userName, bool propagate)
        {
            PeerGroupID gid = this.netPeerGroup.PeerGroupID;

            PipeID pipeid = IDFactory.NewPipeID(gid);

            PipeAdvertisement adv = new PipeAdvertisement();

            adv.SetPipeID(pipeid);
            adv.Type = (propagate ? "JxtaPropagate" : "JxtaUnicast");
            adv.Name = "JxtaTalkUserName." + userName;
            adv.Desc = "created by JXTA .NET";

            discovery.Publish(adv);
        }

        public PipeAdvertisement GetUserAdv(string userName, long timeout)
        {
            string talkUserName = "JxtaTalkUserName." + userName;

            /*
             * check first our local cache
             */
            List<Advertisement> responses = discovery.GetLocalAdvertisements(DiscoveryService.DISC_ADV, "Name", talkUserName);

            PipeAdvertisement adv;

            if (responses.Count == 0)
            {
                try
                {
                    int query_id = discovery.GetRemoteAdvertisements(null, DiscoveryService.DISC_ADV, "Name", talkUserName, 1);

                    for (int i = 0; i <= 20; i++)
                    {
                        Thread.Sleep(500);
                        textWriter.Write(".");

/*                    DiscoveryResponse dr = null;

                    dr = listener.WaitForEvent(timeout);

                    listener = discovery.CancelRemoteQuery(query_id);

                    listener.Stop();

                    responses = discovery.GetLocalAdvertisements<PipeAdvertisement>(DiscoveryService.DISC_ADV, "Name", talkUserName);
*/
                        responses = discovery.GetLocalAdvertisements(DiscoveryService.DISC_ADV, "Name", talkUserName);
                        if (responses.Count != 0)
                            break;
                        
                    }
//                    textWriter.WriteLine("");
                    
                    if (responses.Count == 0)
                    {
                        textWriter.WriteLine("No responses");
                        return null;
                    }
                }
                catch (JxtaException e)
                {
                    textWriter.WriteLine(e.ErrorMessage);
                    textWriter.WriteLine("No responses");
                    return null;
                }
            }

            adv = (PipeAdvertisement)responses[0];

            return adv;
        }

        private class TalkMessageListener : PipeMsgListener
        {
            TextWriter textWriter;

            public void PipeMsgEvent(PipeMsgEvent pipeMsgEvent)
            {
                try
                {
                    Message msg = pipeMsgEvent.Message;
                    string grpName = msg.GetMessageElement("GrpName").ToString();
                    string message = msg.GetMessageElement("JxtaTalkSenderMessage").ToString();
                    string senderName = msg.GetMessageElement("JxtaTalkSenderName").ToString();

                    if (!message.Equals(""))
                    {
                        textWriter.WriteLine("\n##############################################################");
                        textWriter.WriteLine("CHAT MESSAGE from " + senderName + ":");
                        textWriter.WriteLine(message);
                        textWriter.WriteLine("##############################################################\n");
                    }
                    else
                        textWriter.WriteLine("Received empty message");
                }
                catch (Exception e)
                {
                    textWriter.WriteLine(e.Message);
                }
            }

            public TalkMessageListener(TextWriter textWriter)
            {
                this.textWriter = textWriter;
            }
        }

        /// <summary>
        /// Contains the listeners of all logged in users
        /// </summary>
        private static Dictionary<string, InputPipe> _userDictionary = new Dictionary<string,InputPipe>();
        
        public void LoginUser(string userName, PipeMsgListener talkMessageListener)
        {
            PipeAdvertisement adv = GetUserAdv(userName, 30 * 1000 * 1000);

            if (_userDictionary.ContainsKey(userName) == true)
            {
                textWriter.WriteLine("User " + userName + " is already logged in!!!");
                return;
            }

            if (adv != null)
            {
                InputPipe inputPipe = pipeservice.CreateInputPipe(adv, talkMessageListener);
                
                // Releasing of these objects are prevented by this container.
                _userDictionary.Add(userName, inputPipe);
            }
            else
                textWriter.WriteLine("Cannot retrieve advertisement for " + userName);
        }

        public void LogoutUser(string userName)
        {
            if (_userDictionary.Remove(userName) == true)
                return;

            textWriter.WriteLine("User " + userName + " isn't logged in!!!");
        }

        private Dictionary<string, OutputPipe> _outputPipe = new Dictionary<string, OutputPipe>();

        public Dictionary<string, OutputPipe> OutputPipe
        {
            get
            {
                return _outputPipe;
            }
        }

        public OutputPipe ConnectToUser(string userName)
        {
            PipeAdvertisement adv = GetUserAdv(userName, 1 * 5 * 1000 * 1000);

            if (adv != null)
            {
                OutputPipe op = pipeservice.CreateOutputPipe(adv, 30 * 1000);

                if (op == null)
                {
                    textWriter.WriteLine("peer not found.");
                    return null;
                }

                _outputPipe.Add(userName, op);

                return op;
            }
            return null;
        }

        private void TalkToUser(string myName, string userName)
        {
            OutputPipe op = ConnectToUser(userName);

            if (op != null)
            {
                textWriter.WriteLine("Welcome to JXTA-C Chat, " + userName);
                textWriter.WriteLine("Type a '.' at begining of line to quit.");

                String userMessage = "";

                while (true)
                {
                    userMessage = textReader.ReadLine();
                    if (userMessage != "")
                    {
                        if (userMessage[0] == '.')
                            break;
                        SendMessage(op, myName, userMessage);
                    }
                }
            }
        }

        public void SendMessage(OutputPipe op, string myName, string userMessage)
        {
            Message msg = new Message();

            msg.AddMessageElement(new MessageElement("JxtaTalkSenderName", "text/plain", myName));
            msg.AddMessageElement(new MessageElement("GrpName", "text/plain", "NetPeerGroup"));
            msg.AddMessageElement(new MessageElement("JxtaTalkSenderMessage", "text/plain", userMessage));

            op.Send(msg);
        }

        private DiscoveryService discovery;
        private PipeService pipeservice;

        protected override void Initialize()
        {
            this.discovery = netPeerGroup.DiscoveryService;
            this.pipeservice = netPeerGroup.PipeService;
        }
    }
}
