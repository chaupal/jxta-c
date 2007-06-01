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
 * $Id: Log.cs,v 1.3 2006/02/13 10:06:10 lankes Exp $
 */

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using System.Threading;

namespace JxtaNET.Log
{
    /// <summary>
    /// Loglevels for the loggers.
    /// </summary>
    [Flags]
    public enum LogLevels
    {
        NONE = 0,
        FATAL = 0x01,
        ERROR = FATAL << 1,
        WARNING = ERROR << 1,
        INFO = WARNING << 1,
        DEBUG = INFO << 1,
        TRACE = DEBUG << 1,
        PARANOID = TRACE << 1,
        ALL = FATAL | ERROR | WARNING | INFO | DEBUG | TRACE | PARANOID
    };

    public static class GlobalLogger
    {
        /// <summary>
        /// Delegate for appending a log-message to the global logger. See <see cref="FileLogger.Append"/>.
        /// </summary>
        /// <param name="message"><see cref="LogMessage"/> to log.</param>
        public delegate void AppendLogDelegate(LogMessage message);

        /// <summary>
        /// Append a message to the global logger(s).
        /// </summary>
        public static event AppendLogDelegate AppendLogEvent;

        public static void AppendLog(LogMessage msg)
        {
            AppendLogEvent(msg);
        }

        #region native_callback-interop

        // Callback-method for nativ-interop.
        private static UInt32 log_callback(String cat, UInt32 level, String message, UInt32 thread)
        {
            try
            {
                // call the global logger
                LogMessage msg = new LogMessage(cat, LoggersLittleHelper.GetManagedLevel(level), message, DateTime.Now, (uint)thread);
                GlobalLogger.AppendLog(msg);
            }
            catch (JxtaException e)
            {
                return (UInt32)e.ErrorCode;
            }

            return Errors.JXTA_SUCCESS;
        }

        private delegate UInt32 native_log_callback(String cat, UInt32 level, String message, UInt32 thread);

        [DllImport("jxta.dll")]
        private static extern void jxta_managed_log_using(native_log_callback logCb);

        private static native_log_callback _delegate;
        
        public static void StartLogging()
        {
            if (_delegate != null)
                return;

            _delegate = new native_log_callback(log_callback);
            jxta_managed_log_using(_delegate);
        }

        public static void StopLogging()
        {
            if (_delegate == null)
                return;

            _delegate = null;
            jxta_managed_log_using(null);
        }

        static GlobalLogger()
        {
            _delegate = null;
        }
        #endregion
    }

    // Helper-class for nativ-interop; namely the LogLevel-constants.
    internal static class LoggersLittleHelper
    {
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_INVALID();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_MIN();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_FATAL();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_ERROR();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_WARNING();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_INFO();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_DEBUG();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_TRACE();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_PARANOID();
        [DllImport("jxta.dll")]
        private static extern UInt32 jxta_get_JXTA_LOG_LEVEL_MAX();


        private static System.Collections.Generic.Dictionary<LogLevels, UInt32> _nativeLevels;

        static LoggersLittleHelper()
        {
            _nativeLevels = new Dictionary<LogLevels, UInt32>();

            //nativeLevels[(int)LogLevel.INVALID] = jxta_get_JXTA_LOG_LEVEL_INVALID();
            //nativeLevels[(int)LogLevel.JXTA_LOG_LEVEL_MIN] = jxta_get_JXTA_LOG_LEVEL_MIN();
            _nativeLevels[LogLevels.FATAL] = jxta_get_JXTA_LOG_LEVEL_FATAL();
            _nativeLevels[LogLevels.ERROR] = jxta_get_JXTA_LOG_LEVEL_ERROR();
            _nativeLevels[LogLevels.WARNING] = jxta_get_JXTA_LOG_LEVEL_WARNING();
            _nativeLevels[LogLevels.INFO] = jxta_get_JXTA_LOG_LEVEL_INFO();
            _nativeLevels[LogLevels.DEBUG] = jxta_get_JXTA_LOG_LEVEL_DEBUG();
            _nativeLevels[LogLevels.TRACE] = jxta_get_JXTA_LOG_LEVEL_TRACE();
            _nativeLevels[LogLevels.PARANOID] = jxta_get_JXTA_LOG_LEVEL_PARANOID();
            //levels[(int)LogLevels.JXTA_LOG_LEVEL_MAX] = jxta_get_JXTA_LOG_LEVEL_MAX();
        }

        public static UInt32 GetNativeLevel(LogLevels level)
        {
            return _nativeLevels[level];
        }

        public static LogLevels GetManagedLevel(UInt32 level)
        {
            foreach (KeyValuePair<LogLevels, UInt32> l in _nativeLevels)
            {
                if (l.Value == level)
                    return l.Key;
            }

            return 0;
        }
    }

    public class LogMessage
    {
        /// <summary>
        /// This string can be used to categorize the log entry, also can be used by a selector to decide whether the message should be logged.
        /// </summary>
        public String Category;
        /// <summary>
        /// The level of the log message, can be used by a selector to decide whether the message should be logged.
        /// </summary>
        public LogLevels LogLevel;
        /// <summary>
        /// The message to log.
        /// </summary>
        public String Message;
        /// <summary>
        /// The Time of the LogMessage.
        /// </summary>
        public DateTime Time;
        /// <summary>
        /// Thread-ID of the calling thread.
        /// -1 if not set.
        /// </summary>
        public uint ThreadID = 0;
        /// <summary>
        /// FormatString for the ToString-method.
        /// c: Category;
        /// l: LogLevel;
        /// t: Time;
        /// i: ThreadID;
        /// m: Message;
        /// Escape with e.g. 'cit'
        /// </summary>
        public String FormatString = "[c]-l-[t(MM'/'dd HH:mm:ss:ffffff)][TID: i] - m";

        /// <summary>
        /// Generate String with specified format string.
        /// </summary>
        /// <param name="formatString">The format string to be used for this operation. <see cref="FormatString"/></param>
        /// <returns></returns>
        public String ToString(string formatString)
        {
            string ret = "";
            int end = 0;

            for (int i = 0; i < formatString.Length; i++)
            {
                switch (formatString[i])
                {
                    case '\'': end = formatString.IndexOf('\'', i + 1);
                        ret += formatString.Substring(i + 1, end - i - 1);
                        i = end;
                        break;
                    case 'c': ret += Category;
                        break;
                    case 'l': ret += LogLevel;
                        break;
                    case 'i': ret += ThreadID;
                        break;
                    case 'm': ret += Message;
                        break;
                    case 't': if (i + 3 < formatString.Length)
                            if (formatString[i + 1] == '(')
                            {
                                end = formatString.IndexOf(')', i + 1);
                                ret += Time.ToString(formatString.Substring(i + 2, end - i - 2));
                                i = end;
                            }
                            else
                                ret += Time.ToString();
                        break;
                    default: ret += formatString[i];
                        break;
                }
            }

            return ret.TrimEnd();
        }

        public override String ToString()
        {
            return ToString(this.FormatString);
        }

/*        public LogMessage(String category, LogLevels logLevel, String message)
        {
            Category = category;
            LogLevel = logLevel;
            Message = message;
        }


        public LogMessage(String category, LogLevels logLevel, String message, DateTime time)
        {
            Category = category;
            LogLevel = logLevel;
            Message = message;
            Time = time;
        }
*/
        public LogMessage(String category, LogLevels logLevel, String message, DateTime time, uint threadID)
        {
            Category = category;
            LogLevel = logLevel;
            Message = message;
            Time = time;
            ThreadID = threadID;
        }
    }

    /// <summary>
    /// Helper structure to determine whether a log should be recorded by the logger.
    /// </summary>
    public class LogSelector
    {

        private Collection<string> _categories = new Collection<string>();
        private LogLevels _logLevel = new LogLevels();
        private bool _invertCategories;

        /// <summary>
        /// switch between logging categories which are added vs. categories which are not added
        /// </summary>
        public bool InvertCategories
        {
            get { return _invertCategories; }
            set { _invertCategories = value; }
        }

        /// <summary>
        /// add categories to be logged
        /// </summary>
        /// <param name="cat">categories to be logged</param>
        public void AddCategory(params string[] cat)
        {
            foreach (string s in cat)
                _categories.Add(s);
        }

        /// <summary>
        /// remove categoriers from logging
        /// </summary>
        /// <param name="cat">categories to be removed</param>
        public void RemoveCategory(params string[] cat)
        {
            foreach (string s in cat)
                _categories.Remove(s);
        }

        /// <summary>
        /// add LogLevels
        /// </summary>
        /// <param name="levels">LogLevels to be added</param>
        public void AddLogLevel(params LogLevels[] levels)
        {
            foreach (LogLevels l in levels)
                _logLevel = _logLevel | l;
        }

        /// <summary>
        /// remove LogLevels
        /// </summary>
        /// <param name="levels">LogLevels to be removed</param>
        public void RemoveLogLevel(params LogLevels[] levels)
        {
            foreach (LogLevels l in levels)
                _logLevel = _logLevel ^ (_logLevel & l);
        }

        /// <summary>
        /// Use this method to determine if a LogMessage should be logged.
        /// </summary>
        /// <param name="logMessage">the <see cref="LogMessage"/> which is to be logged </param>
        /// <returns>true, if the category and the loglevel of the logMessage are covered by this LogSelector; false otherwise</returns>
        public bool IsSelected(LogMessage logMessage)
        {
            bool levelCheck = (_logLevel & logMessage.LogLevel) > 0;
            bool categoryCheck = _invertCategories ^ _categories.Contains(logMessage.Category);

            return (categoryCheck && levelCheck);
        }
    }

    /// <summary>
    /// a logger to log to files or the console (assuming you specify System.Console.Out for the property LogWriter)
    /// </summary>
    public class FileLogger
    {
        private TextWriter _logWriter;

        /// <summary>
        /// TextWriter where this logger writes to
        /// </summary>
        /// 
        public TextWriter LogWriter
        {
            get { return _logWriter; }
            set { _logWriter = value; }
        }

        private LogSelector _logSelector;

        /// <summary>
        /// <see cref="LogSelector"/> used for the LogMessage-filtering
        /// </summary>
        public LogSelector LogSelector
        {
            get { return _logSelector; }
            set { _logSelector = value; }
        }

        /// <summary>
        /// the log-method; add this method to the GlobalLogger.AppendLogEvent
        /// </summary>
        /// <param name="logMessage"><see cref="LogMessage"/> to append. </param>
        public void Append(LogMessage logMessage)
        {
            if ((_logSelector == null) || _logSelector.IsSelected(logMessage))
                _logWriter.Write("[" + logMessage.Category + "]-" + logMessage.LogLevel + "-[" + logMessage.Time.ToString("MM'/'dd HH:mm:ss:ffffff") + "][TID: " + logMessage.ThreadID + "] - " + logMessage.Message);
        }

        public FileLogger()
        {
            this._logWriter = Console.Out;
        }
    }
}