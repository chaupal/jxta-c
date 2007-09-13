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
using JxtaNET.Log;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reflection;
using System.Diagnostics;
using System.IO;

namespace JxtaNETShell
{
    /// <summary>
    /// Main class of the JxtaNETShell. Include the entry point of JxtaNETShell.
    /// </summary>
    public class Shell
    {
        /// <summary>
        /// Represents a JXTA peer group
        /// </summary>
        private PeerGroup netPeerGroup;

        /// <summary>
        /// textWriter is a represent, which write the output messages into a output stream or string.
        /// </summary>
        private TextWriter textWriter;

        /// <summary>
        /// textReader is a represent, which reads the output messages from a output stream or string.
        /// </summary>
        private TextReader textReader;

        /// <summary>
        /// Defines an Assembly, which is a reusable, versionable, 
        /// and self-describing building block of a common language runtime application.
        /// </summary>
        private static Assembly assembly = Assembly.GetExecutingAssembly();

        /// <summary>
        /// Main entry point of the JxtaNETShell.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Shell shell = new Shell(PeerGroupFactory.NewNetPeerGroup());
            
            LogSelector logSelector = new LogSelector();
            logSelector.InvertCategories = true;

            StandardLogger logger = new StandardLogger();
            logger.LogSelector = logSelector;

            int logDepth = 2;
            for (int i = 0; i < args.Length; i++)
            {
                switch(args[i])
                {
                    case "-v":
                        logDepth++;
                        break;
                    case "-f":
                        if (args.Length > i)
                            logger.LogWriter = new StreamWriter(args[++i]);
                        break;
                    default:
                        Console.WriteLine("# Error - Bad option");
                        shell.PrintHelp();
                        return;
                }
            }

            if (logDepth > 0)
                logSelector.AddLogLevel((LogLevels)((int)(Math.Pow(2, logDepth) - 1)));

            GlobalLogger.AppendLogEvent += logger.AppendLogMessage;
            GlobalLogger.StartLogging();

            shell.PrintWelcome();
            shell.ControlLoop();
            
            Console.WriteLine("Exiting Jxta.NET Shell");

            GlobalLogger.StopLogging();
            GlobalLogger.AppendLogEvent -= logger.AppendLogMessage;
        }

        /// <summary>
        /// Create a shell with a given peer group, input and output stream.
        /// </summary>
        /// <param name="netPG"></param>
        /// <param name="writer"></param>
        /// <param name="reader"></param>
        public Shell(PeerGroup netPG, TextWriter writer, TextReader reader)
        {
            netPeerGroup = netPG;
            textWriter = writer;
            textReader = reader;
        }

        /// <summary>
        /// Create a shell with a given peer group.
        /// </summary>
        /// <param name="netPG"></param>
        public Shell(PeerGroup netPG)
        {
            netPeerGroup = netPG;
            textWriter = Console.Out;
            textReader = Console.In;
        }

        /// <summary>
        /// Prints a short introduction
        /// </summary>
        public void PrintHelp()
        {
            textWriter.WriteLine("# shell - Interactive enviroment for using JXTA.");
            textWriter.WriteLine("SYNOPSIS");
            textWriter.WriteLine("\tshell [-v] [-v] [-v] [-l file] [-f file]\n");
            textWriter.WriteLine("DESCRIPTION");
            textWriter.WriteLine("\tInteractive enviroment for using JXTA.\n");
            textWriter.WriteLine("OPTIONS\n");
            textWriter.WriteLine("\t-v\tProduce more verbose logging output by default.\n\t\tEach additional -v causes more output to be produced.");
            textWriter.WriteLine("\t-f file\tSpecify the file into which log output will go.");

            // not yet implemented
            //textWriter.WriteLine("\t-l file\tFile from which to read log settings.");
        }

        /// <summary>
        /// Reads the next commad form the standard input stream and interpret this command.
        /// </summary>
        public void ControlLoop()
        {
            string      input = "";
            bool        done = false;

            while (!done)
            {
                textWriter.Write("JXTA.NET> ");
                input = Console.ReadLine();

                done = ProcessCommand(ref input);

                //textWriter.WriteLine("");
            }
        }

        /// <summary>
        /// Prints a short welcome message
        /// </summary>
        public void PrintWelcome()
        {
            textWriter.WriteLine("=========================================================");
            textWriter.Write("======= Welcome to the JXTA.NET Shell Version ");
            FileVersionInfo info = FileVersionInfo.GetVersionInfo(assembly.Location);
            textWriter.Write(info.FileMajorPart.ToString());
            textWriter.Write(".");
            textWriter.Write(info.FileMinorPart.ToString());
            textWriter.WriteLine(" =======");
            textWriter.WriteLine("=========================================================");
            textWriter.WriteLine(" ");
            textWriter.WriteLine("The JXTA.NET Shell provides an interactive environment to the JXTA");
            textWriter.WriteLine("platform. The Shell provides basic commands to discover peers, ");
            textWriter.WriteLine("to create pipes between peers, and to send pipe messages.");
            /*textWriter.WriteLine("peergroups, to join and resign from peergroups, to create pipes");
            textWriter.WriteLine("between peers, and to send pipe messages. The Shell provides environment");
            textWriter.WriteLine("variables that permit binding symbolic names to Jxta platform objects.");
            textWriter.WriteLine("Environment variables allow Shell commands to exchange data between");
            textWriter.WriteLine("themselves. The shell command 'env' displays all defined environment");
            textWriter.WriteLine("variables in the current Shell session.");*/
            textWriter.WriteLine(" ");
            textWriter.WriteLine("A 'man' command is available to list the commands available.");
            textWriter.WriteLine("To exit the Shell, use the 'exit' or 'quit' command.");
            textWriter.WriteLine(" ");
        }

        /// <summary>
        /// Interprets the command which is given by an input string
        /// </summary>
        /// <param name="input">Command as string</param>
        /// <returns></returns>
        public bool ProcessCommand(ref string input)
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            bool man = false;

            input = input.Trim();

            if (input.Length < 3)
            {
                if (input.Length > 0)
                    textWriter.WriteLine("command not found");
                return false;
            }

            if (input.Substring(0, 3).Equals("man"))
            {
                man = true;
                if (input.Length > 4)
                {
                    input = input.Substring(4, input.Length - 4);
                }
                else
                {
                    textWriter.WriteLine("For which command do you want help?");

                    // get all types of the current assembly
                    Type[] types = assembly.GetTypes();
                    for (int i = 0; i < types.Length; i++)
                    {
                        string cmd = types[i].ToString();

                        cmd = cmd.ToLower();

                        if (cmd.StartsWith("jxtanetshell"))
                        {
                            cmd = cmd.Remove(0, 13);

                            // don't print types, which don't implement a command
                            if ((cmd.Contains("+") != true) && 
                                (cmd.Contains("operation") != true) && 
                                (cmd.Contains("shellapp") != true) && 
                                (cmd.Contains("shell") != true))
                            {
                                textWriter.WriteLine("\t" + cmd);
                            }
                        }
                    }
                    return false;
                }
            }

            char[] sep = { ' ' };
            string[] locargs = input.Split(sep);

            if ((locargs[0].Equals("exit") || locargs[0].Equals("quit")))
            {
                return true;
            }

            Type type = assembly.GetType("JxtaNETShell." + locargs[0], false, true);

            if (type != null)
            {
                try
                {
                    ShellApp x = ShellAppFactory.BuildShellApp(type, netPeerGroup, textWriter, textReader);

                    if (man)
                        x.Help();
                    else
                        x.Run(locargs);
                }
                catch (Exception)
                {
                    textWriter.WriteLine("command not found");
                }
            }
            else
                textWriter.WriteLine("command not found");

            return false;
        }
    }
}
