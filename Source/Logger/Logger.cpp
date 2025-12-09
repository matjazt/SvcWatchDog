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

#include <JsonConfig/JsonConfig.h>
#include <Logger/Logger.h>

#include <iostream>
#include <fstream>
#include <cstdarg>
#include <chrono>
#include <algorithm>
#include <ranges>
#include <cassert>

using namespace std;

Logger* Logger::m_instance = nullptr;

Logger::Logger() noexcept
    : m_minConsoleLevel(LogLevel::Verbose),
      m_minFileLevel(LogLevel::Verbose),
      m_maxFileSize(0),
      m_maxWriteDelay(0),
      m_maxOldFiles(0),
      m_logThreadId(false),
      m_mute(false),
      m_fileQueue(std::make_unique<queue<string>>()),
      m_emailTimestamp(0),
      m_threadTrigger(false, true),  // initialize the event with auto-reset, although it's not strictly necessary here
      m_running(false)
{
}

Logger::~Logger()
{
    Shutdown();
    if (m_instance == this)
    {
        m_instance = nullptr;
    }
}

Logger* Logger::GetInstance() noexcept { return m_instance; }

void Logger::SetInstance(Logger* instance) noexcept { m_instance = instance; }

void Logger::SetFileNamePostfix(const string& postfix) noexcept { m_fileNamePostfix = postfix; }

void Logger::Configure(JsonConfig& cfg, const string& section)
{
    m_minConsoleLevel = (LogLevel)cfg.GetNumber(section, "minConsoleLevel", TOINT(LogLevel::Verbose));
    m_minFileLevel = (LogLevel)cfg.GetNumber(section, "minFileLevel", TOINT(LogLevel::Verbose));
    const string tmp = cfg.GetString(section, "filePath", "");
    if (tmp.empty())
    {
        // if no file path is provided, disable file logging
        m_minFileLevel = MaskAllLogs;
    }
    else
    {
        // if the file path is provided, make sure it is absolute

        m_filePath = filesystem::absolute(tmp);
        if (!m_fileNamePostfix.empty())
        {
            // insert the postfix before the extension
            auto baseName = m_filePath.stem();
            auto extension = m_filePath.extension();
            m_filePath = m_filePath.parent_path() / (baseName.string() + "." + m_fileNamePostfix + extension.string());
        }
        filesystem::create_directories(m_filePath.parent_path());  // create the directory if it doesn't exist
    }
    m_maxFileSize = cfg.GetNumber(section, "maxFileSize", 20 * 1024 * 1024);
    m_maxWriteDelay = cfg.GetNumber(section, "maxWriteDelay", 500);
    m_maxOldFiles = cfg.GetNumber(section, "maxOldFiles", 0);

    m_logThreadId = cfg.GetBool(section, "logThreadId", false);
}

void Logger::RegisterPlugin(unique_ptr<ILoggerPlugin> plugin) { m_plugins.emplace_back(std::move(plugin)); }

LogLevel Logger::GetMinPluginLevel()
{
    auto it = ranges::min_element(m_plugins, {}, &ILoggerPlugin::MinLogLevel);
    return (it != m_plugins.end()) ? (*it)->MinLogLevel() : MaskAllLogs;
}

void Logger::Start()
{
    if (!m_running)
    {
        m_running = true;
        m_thread = thread(&Logger::Thread, this);

        LOGSTR() << "minConsoleLevel=" << m_minConsoleLevel << ", minFileLevel=" << m_minFileLevel << ", filePath=" << m_filePath.string()
                 << ", maxFileSize=" << m_maxFileSize << ", maxOldFiles=" << m_maxOldFiles << ", maxWriteDelay=" << m_maxWriteDelay
                 << ", logThreadId=" << BOOL2STR(m_logThreadId);
    }
}

void Logger::Shutdown()
{
    if (m_running)
    {
        LOGSTR() << "shutting down";
        m_running = false;
        m_threadTrigger.SetEvent();  // signal the thread to wake up and finish
        m_thread.join();
    }

    Flush(true);  // flush any remaining logs
}

void Logger::Mute(bool mute) noexcept { m_mute = mute; }

void Logger::Log(LogLevel level, const string& message, const char* file, const char* func)
{
    if (m_mute || !m_running || (level < m_minConsoleLevel && level < m_minFileLevel && level < GetMinPluginLevel()))
    {
        return;
    }

    // if file and function are provided, use them to get the location prefix
    const string locationPrefix = (file && func) ? (GetLocationPrefix(file, func) + ": ") : "";

    struct tm localTime = {};
    int milliseconds = 0;
    GetCurrentLocalTime(localTime, milliseconds);

    const char* levelName = nullptr;
    switch (level)
    {
        case LogLevel::Verbose:
            levelName = "VRB";
            break;
        case LogLevel::Debug:
            levelName = "DBG";
            break;
        case LogLevel::Information:
            levelName = "INF";
            break;
        case LogLevel::Warning:
            levelName = "WRN";
            break;
        case LogLevel::Error:
            levelName = "ERR";
            break;
        case LogLevel::Fatal:
            levelName = "FAT";
            break;
        default:
            levelName = "UNK";
            break;
    }

    char threadIdPrefix[16] = "";
    if (m_logThreadId)
    {
        // get the thread id - we deliberately truncate the hash to 32 bits, because it should be good enough for our purposes.
        const uint32_t threadIdHash = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
        // convert it to a string
#ifdef WIN32
#pragma warning(suppress : 6031)
#endif
        snprintf(threadIdPrefix, sizeof(threadIdPrefix), "%08x: ", threadIdHash);
        AUTO_TERMINATE(threadIdPrefix);
    }
    else
    {
        // not logging thread id
        threadIdPrefix[0] = 0;
    }
    // this seems to be the fastest way to get a string representation of the thread id

    // put all data into a single log string, including trailing newline
    const size_t maxSize = message.length() + locationPrefix.length() + 60;  // 60 - timestamp, log level, threadId etc
    string fullMessage;
    fullMessage.resize(maxSize);  // 50 - timestamp, log level etc
    int actualLength = snprintf(&fullMessage[0], maxSize - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03d [%s] %s%s%s\n", localTime.tm_year + 1900,
                                localTime.tm_mon + 1, localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec,
                                milliseconds, levelName, threadIdPrefix, locationPrefix.c_str(), message.c_str());
    if (actualLength < 0)
    {
        // snprintf failed, so the buffer is obviously too small. It shouldn't happen, but let's handle it anyway.
        actualLength = TOINT(maxSize) - 1;
        fullMessage[actualLength - 1] = '\n';
    }
    fullMessage.resize(actualLength);

    // now obtain the lock to avoid messing up the output or crashing the queue when multiple threads are logging
    const lock_guard<mutex> lock(m_cs);

    // console output
    if (m_minConsoleLevel <= level)
    {
        cout << fullMessage;
    }

    // plugin output
    for (auto& plugin : m_plugins)
    {
        if (level >= plugin->MinLogLevel())
        {
            plugin->Log(level, fullMessage);
        }
    }

    // file output
    if (m_minFileLevel <= level)
    {
        m_fileQueue->push(std::move(fullMessage));
    }
}

void Logger::Msg(LogLevel level, const char* pszFmt, ...)
{
    if (!pszFmt || m_mute || !m_running || (level < m_minConsoleLevel && level < m_minFileLevel))
    {
        return;
    }

    va_list p;
    va_start(p, pszFmt);

    // prepare text, then use fixed string Log method
    const size_t maxSize = 5000;
    string message;
    message.resize(maxSize);

    const int actualLength = _vsnprintf(&message[0], maxSize - 1, pszFmt, p);
    va_end(p);
    message.resize(actualLength);

    Log(level, message);
}

void Logger::Thread()
{
    while (m_running)
    {
        if (m_threadTrigger.WaitForSingleEvent(m_maxWriteDelay))
        {
            if (m_running)
            {
                // strange - the wait succeeded although we're not shutting down
                // let's sleep for a while to prevent busy waiting (not really needed)
                SLEEP(m_maxWriteDelay);

                // NOTE: this should never happen, hence assert
                assert(0);
            }
        }

        Flush(false);
    }
}

void Logger::Flush(bool force)
{
    try
    {
        FlushFileQueue();
    }
    catch (const std::exception& e)
    {
        // we can't afford to properly log exceptions here, because it might push us into a loop.
        cerr << "Logger::Thread: exception while flushing file queue: " << e.what() << "\n";
        // For the time being, we're just catching the exception and hope it was temporary.
    }
    catch (...)
    {
        // we can't afford to properly log exceptions here, because it might push us into a loop.
        cerr << "Logger::Thread: exception while flushing file queue\n";
        // For the time being, we're just catching the exception and hope it was temporary.
    }

    // flush plugins
    for (auto& plugin : m_plugins)
    {
        plugin->Flush(m_running, force);
    }
}

void Logger::FlushFileQueue()
{
    m_cs.lock();

    if (m_fileQueue->empty())
    {
        // nothing to flush, let's unlock and return
        m_cs.unlock();
        return;
    }

    auto queueCopy = std::move(m_fileQueue);
    m_fileQueue = std::make_unique<queue<string>>();  // create a new queue for future logs
    // we're done with m_fileQueue, it is now freshly initialized
    // let's unlock the logger and write the data to the file
    m_cs.unlock();

    // open the file in append mode
    std::ofstream outFile(m_filePath, std::ios::app);

    if (outFile.is_open())
    {
        // outFile << buffer;
        while (!queueCopy->empty())
        {
            outFile << queueCopy->front();  // write to the file
            queueCopy->pop();
        }
    }
    else
    {
        LOGSTR() << "unable to write to file " << m_filePath.string();
        // it's worth trying to create the folder again, although it should already exist
        filesystem::create_directories(m_filePath.parent_path());
    }

    const auto fileSize = outFile.tellp();
    outFile.close();

    // rotate file if needed
    if (m_maxFileSize > 0 && fileSize > m_maxFileSize)
    {
        // file grew too large, rename it
        struct tm localTime = {};
        int dummy = 0;
        GetCurrentLocalTime(localTime, dummy);

        char timestamp[32];
#ifdef WIN32
#pragma warning(suppress : 6031)
#endif
        snprintf(timestamp, sizeof(timestamp) - 1, "%04d%02d%02d%02d%02d%02d", localTime.tm_year + 1900, localTime.tm_mon + 1,
                 localTime.tm_mday, localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
        AUTO_TERMINATE(timestamp);

        auto extension = m_filePath.extension();
        auto baseName = m_filePath.stem();
        auto newFileName = m_filePath.parent_path() / (baseName.string() + "." + timestamp + extension.string());
        filesystem::rename(m_filePath, newFileName);

        // remove old files if needed
        if (m_maxOldFiles > 0)
        {
            std::vector<filesystem::path> oldFiles;

            // iterate through the directory and create a list of all old files
            auto folderIterator = filesystem::directory_iterator(m_filePath.parent_path());
            for (const auto& entry : folderIterator)
            {
                if (entry.is_regular_file() && entry.path().extension() == extension &&
                    entry.path().stem().string().starts_with(baseName.string()))
                {
                    oldFiles.push_back(entry.path());
                }
            }

            if (oldFiles.size() > m_maxOldFiles)
            {
                // sort old files by name (name should contain timestamp, so we're basically sorting by time)
                std::ranges::sort(oldFiles);

                // remove the oldest files
                for (size_t i = 0; i < oldFiles.size() - m_maxOldFiles; i++)
                {
                    filesystem::remove(oldFiles[i]);
                }
            }
        }
    }
}

LoggerStream::LoggerStream() noexcept : m_file(nullptr), m_func(nullptr), m_level(LogLevel::Debug) {}

std::ostringstream& LoggerStream::Get(LogLevel level) noexcept
{
    m_level = level;
    return m_buffer;
}

std::ostringstream& LoggerStream::GetEx(const char* file, const char* func, LogLevel level) noexcept
{
    m_file = file;
    m_func = func;
    m_level = level;
    return m_buffer;
}

std::string LoggerStream::GetBuffer() const { return m_buffer.str(); }

LoggerStream::~LoggerStream()
{
    // Now we need to actually log the collected text. While doing it, we prefer to ignore errors, because after all,
    // it's "just" logging... and we don't have a suitable way of reporting the errors, either.
    try
    {
        const auto logger = Logger::GetInstance();
        if (logger)
        {
            logger->Log(m_level, m_buffer.str(), m_file, m_func);
        }
    }
    catch (...)
    {
    }
}
