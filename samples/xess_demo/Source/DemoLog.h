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
#include "Log.h"

/// Log class for the Demo.
class DemoLog : public Log
{
public:
    DemoLog()
        : Log()
    {
#ifdef _DEBUG
        m_Level = LevelDebug;
#else
        m_Level = LevelInfo;
#endif
    }
    /// We override the message output handling.
    void HandleOutput(LogLevel Level, const std::string& Message) override
    {
        Log::HandleOutput(Level, Message);

        // save the message.
        m_Messages.push_back(LogMessage(Level, Message));
    }

    const std::vector<LogMessage>& GetMessages() const { return m_Messages; }

private:
    /// Log messages.
    std::vector<LogMessage> m_Messages;
};
