/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once
#include <mutex>

//#define _NO_LOG_ 1

#ifdef _NO_LOG_
#define LOG_DEBUG(Message) ((void)0)
#define LOG_INFO(Message) ((void)0)
#define LOG_WARN(Message) ((void)0)
#define LOG_ERROR(Message) ((void)0)

#define LOG_DEBUGF(Format, ...) ((void)0)
#define LOG_INFOF(Format, ...) ((void)0)
#define LOG_WARNF(Format, ...) ((void)0)
#define LOG_ERRORF(Format, ...) ((void)0)
#else
#define LOG_DEBUG(Message) Log::Write(Log::LevelDebug, Message)
#define LOG_INFO(Message) Log::Write(Log::LevelInfo, Message)
#define LOG_WARN(Message) Log::Write(Log::LevelWarning, Message)
#define LOG_ERROR(Message) Log::Write(Log::LevelError, Message)

#define LOG_DEBUGF(Format, ...) Log::WriteFormat(Log::LevelDebug, Format, ##__VA_ARGS__)
#define LOG_INFOF(Format, ...) Log::WriteFormat(Log::LevelInfo, Format, ##__VA_ARGS__)
#define LOG_WARNF(Format, ...) Log::WriteFormat(Log::LevelWarning, Format, ##__VA_ARGS__)
#define LOG_ERRORF(Format, ...) Log::WriteFormat(Log::LevelError, Format, ##__VA_ARGS__)
#endif

/// A simple log class.
class Log
{
public:
    enum LogLevel
    {
        LevelDebug = 0,
        LevelInfo,
        LevelWarning,
        LevelError
    };

    struct LogMessage
    {
        /// Constructor.
        LogMessage() = delete;

        /// Constructor.
        LogMessage(LogLevel Level, const char* Message)
            : m_Level(Level)
            , m_Message(Message) {

            };

        /// Constructor.
        LogMessage(LogLevel Level, const std::string& Message)
            : m_Level(Level)
            , m_Message(Message) {

            };

        /// Message level.
        LogLevel m_Level;
        /// Message content.
        std::string m_Message;
    };

    static void Write(LogLevel Level, const char* Message);
    static void WriteFormat(LogLevel Level, const char* Format, ...);

    /// Constructor.
    explicit Log();
    /// Destructor.
    ~Log();

    void SetLevel(LogLevel Level);
    void Flush();

protected:
    // Handle the message output. Can be overridden in subclass.
    virtual void HandleOutput(LogLevel Level, const std::string& Message);

    /// Log messages from other threads.
    std::queue<LogMessage> m_ThreadedMessages;
    /// Level of log.
    LogLevel m_Level;
    /// Mutex for threading.
    std::mutex m_Mutex;
};
