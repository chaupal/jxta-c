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
 * $Id: Shell.cs,v 1.8 2006/02/13 21:06:17 lankes Exp $
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
    /// Summary of Class1.
    /// </summary>
    class Shell
    {
        static PeerGroup netPeerGroup;

        public static void printHelp()
        {
            Console.WriteLine("# shell - Interactive enviroment for using JXTA.");
            Console.WriteLine("SYNOPSIS");
            Console.WriteLine("\tshell [-v] [-v] [-v] [-l file] [-f file]\n");
            Console.WriteLine("DESCRIPTION");
            Console.WriteLine("\tInteractive enviroment for using JXTA.\n");
            Console.WriteLine("OPTIONS\n");
            Console.WriteLine("\t-v\tProduce more verbose logging output by default.\n\t\tEach additional -v causes more output to be produced.");
            Console.WriteLine("\t-f file\tSpecify the file into which log output will go.");
            
            // not yet implemented
            //Console.WriteLine("\t-l file\tFile from which to read log settings.");
        }

        /// <summary>
        /// Main entry point of the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            string input = "";
            Assembly assembly = Assembly.GetExecutingAssembly();
            bool ende = false;
            bool man = false;

            LogSelector logSelector = new LogSelector();
            logSelector.InvertCategories = true;

            FileLogger fileLogger = new FileLogger();
            fileLogger.LogSelector = logSelector;

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
                            fileLogger.LogWriter = new StreamWriter(args[++i]);
                        break;
                    default:
                        Console.WriteLine("# Error - Bad option");
                        printHelp();
                        return;
                }
            }

            if (logDepth > 0)
                logSelector.AddLogLevel((LogLevels)((int)(Math.Pow(2, logDepth) - 1)));

            GlobalLogger.AppendLogEvent += fileLogger.Append;
            GlobalLogger.StartLogging();

            netPeerGroup = new PeerGroup();

            Console.WriteLine("=========================================================");
            Console.Write("======= Welcome to the JXTA.NET Shell Version ");
            FileVersionInfo info = FileVersionInfo.GetVersionInfo(assembly.Location);
            Console.Write(info.FileMajorPart.ToString());
            Console.Write(".");
            Console.Write(info.FileMinorPart.ToString());
            Console.WriteLine(" =======");
            Console.WriteLine("=========================================================");
            Console.WriteLine(" ");
            Console.WriteLine("The JXTA.NET Shell provides an interactive environment to the JXTA");
            Console.WriteLine("platform. The Shell provides basic commands to discover peers, ");
            Console.WriteLine("to create pipes between peers, and to send pipe messages.");
            /*Console.WriteLine("peergroups, to join and resign from peergroups, to create pipes");
            Console.WriteLine("between peers, and to send pipe messages. The Shell provides environment");
            Console.WriteLine("variables that permit binding symbolic names to Jxta platform objects.");
            Console.WriteLine("Environment variables allow Shell commands to exchange data between");
            Console.WriteLine("themselves. The shell command 'env' displays all defined environment");
            Console.WriteLine("variables in the current Shell session.");*/
            Console.WriteLine(" ");
            Console.WriteLine("A 'man' command is available to list the commands available.");
            Console.WriteLine("To exit the Shell, use the 'exit' or 'quit' command.");
            Console.WriteLine(" ");

            while (!ende)
            {
                man = false;

                Console.Write("JXTA.NET> ");
                input = Console.ReadLine();
                input = input.Trim();

                if (input.Length < 3)
                {
                    if (input.Length > 0)
                        Console.WriteLine("command not found");
                    continue;
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
                        Console.WriteLine("For which command do you want help?");

                        
                        // get all types of the current assembly
                        Type[] types = assembly.GetTypes();
                        for (int i = 0; i < types.Length; i++)
                        {
                            string cmd = types[i].ToString();

                            if (cmd.Contains("JxtaNETShell") == false)
                                continue;

                            cmd = cmd.Remove(0, 13);
                            cmd = cmd.ToLower();

                            // don't print types, which don't implement a command
                            if ((cmd.Contains("operation") == true) || (cmd.Contains("shellapp") == true) || (cmd.Contains("shell") == true))
                                continue;

                            Console.Write("\t");
                            Console.WriteLine(cmd);
                        }
                        continue;
                    }
                }

                char[] sep = { ' ' };
                string[] locargs = input.Split(sep);

                if ((locargs[0].Equals("exit") || locargs[0].Equals("quit")))
                {
                    ende = true;
                    break;
                }

                Type type = assembly.GetType("JxtaNETShell." + locargs[0], false, true);

                if (type != null)
                {
                    object[] param = { netPeerGroup };
                    ShellApp x = System.Activator.CreateInstance(type, param) as ShellApp;

                    if (man)
                        x.help();
                    else
                        x.run(locargs);
                }
                else
                    Console.WriteLine("command not found");

                //Console.WriteLine("");
            }
            
            Console.WriteLine("Exiting Jxta.NET Shell");

            GlobalLogger.StopLogging();
            GlobalLogger.AppendLogEvent -= fileLogger.Append; 
        }
    }
}
