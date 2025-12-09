/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---------------------------------------------------------------------------
 *
 * Official repository: https://github.com/matjazt/SvcWatchDog
 */

#ifndef _LOGGEREMAILPLUGIN_H_
#define _LOGGEREMAILPLUGIN_H_

#include <Logger/Logger.h>
#include <Email/EmailSender.h>

class LoggerEmailPlugin : public ILoggerPlugin
{
   public:
    static void ConfigureAll(JsonConfig& cfg, Logger& logger, const std::string& parentSection = "log.email");

    LoggerEmailPlugin(JsonConfig& cfg, const std::string& section);
    ~LoggerEmailPlugin();

    // prevent copying and assignment
    DELETE_COPY_AND_ASSIGNMENT(LoggerEmailPlugin);

    virtual void Log(LogLevel level, const std::string& message);
    virtual LogLevel MinLogLevel();
    virtual void Flush(bool stillRunning, bool force);

   private:
    LogLevel m_minLogLevel;
    std::vector<std::string> m_recipients;
    std::string m_subject;
    std::string m_emailSection;
    int m_maxDelay;
    int m_maxLogs;
    int m_timeoutOnShutdown;

    EmailSender m_emailSender;
    std::unique_ptr<std::queue<std::string>> m_queue;
    std::uint64_t m_queueTimestamp;

    std::mutex m_cs;  // TODO: a ga rabim? Uporabljam?

    void SendEmail(std::unique_ptr<std::queue<std::string>> emailQueue, bool stillRunning);
};

#endif
