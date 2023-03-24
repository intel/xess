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

#include "pch.h"
#include "Log.h"
#include <locale>

static Log* gInstance = nullptr;
static uint32_t gMainThreadId = 0;

Log::Log()
    : m_Level(LevelInfo)
{
    gInstance = this; // Initialize instance pointer.

    // It is required to call the Log constructor in main thread.
    gMainThreadId = Utility::GetThreadId();
}

Log::~Log()
{
}

void Log::SetLevel(LogLevel Level)
{
    m_Level = Level;
}

void Log::WriteFormat(LogLevel Level, const char* Format, ...)
{
    va_list args;
    va_start(args, Format);
    size_t size = std::vsnprintf(nullptr, 0, Format, args) + 1;
    std::string buffer(size, '\0');
    std::vsnprintf(&buffer[0], size, Format, args);

    Write(Level, &buffer[0]);
    va_end(args);
}

void Log::Write(LogLevel Level, const char* Message)
{
    assert(gInstance);
    std::lock_guard<std::mutex> lockGuard(gInstance->m_Mutex);

    if (gInstance->m_Level > Level)
        return;

    if (Utility::GetThreadId() == gMainThreadId)
    {
        // direct output if it's main thread.
        gInstance->HandleOutput(Level, Message);
    }
    else
    {
        // store messages for later process.
        gInstance->m_ThreadedMessages.push(LogMessage(Level, Message));
    }
}

void Log::HandleOutput(LogLevel Level, const std::string& Message)
{
    assert(Utility::GetThreadId() == gMainThreadId);

    static const char* PREFIX_NAMES[] = { "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]" };

    Utility::Printf("%s %s\n", PREFIX_NAMES[Level], Message.c_str());
}

void Log::Flush()
{
    assert(Utility::GetThreadId() == gMainThreadId);

    std::lock_guard<std::mutex> lockGuard(m_Mutex);

    while (!m_ThreadedMessages.empty())
    {
        const LogMessage& message = m_ThreadedMessages.front();

        // Output the message
        HandleOutput(message.m_Level, message.m_Message);

        m_ThreadedMessages.pop();
    }
}
