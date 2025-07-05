/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <JsonConfig/JsonConfig.h>
#include <Logger/Logger.h>

#include <iostream>
#include <fstream>
#include <cstdarg>
#include <chrono>

Logger* Logger::m_instance = nullptr;

Logger::Logger()
    : m_minConsoleLevel(LogLevel::Verbose),
      m_minFileLevel(LogLevel::Verbose),
      m_maxFileSize(0),
      m_maxWriteDelay(0),
      m_maxOldFiles(0),
      m_minEmailLevel(LogLevel::MaskAllLogs),
      m_maxEmailDelay(0),
      m_maxEmailLogs(0),
      m_emailTimeoutOnShutdown(0),
      m_logThreadId(false),
      m_mute(false),
      m_emailTimestamp(0),
      m_threadTrigger(false, true),  // initialize the event with auto-reset, although it's not strictly necessary here
      m_running(false)
{
    m_fileQueue = std::make_unique<queue<string>>();
    m_emailQueue = std::make_unique<queue<string>>();
}

Logger::~Logger() { Shutdown(); }

Logger* Logger::GetInstance() { return m_instance; }

void Logger::SetInstance(Logger* instance) { m_instance = instance; }

void Logger::Configure(JsonConfig& cfg, const string& section)
{
    m_minConsoleLevel = (LogLevel)cfg.GetNumber(section, "minConsoleLevel", (int)LogLevel::Verbose);
    m_minFileLevel = (LogLevel)cfg.GetNumber(section, "minFileLevel", (int)LogLevel::Verbose);
    string tmp = cfg.GetString(section, "filePath", "");
    if (tmp.empty())
    {
        // if no file path is provided, disable file logging
        m_minFileLevel = MaskAllLogs;
    }
    else
    {
        // if the file path is provided, make sure it is absolute
        m_filePath = filesystem::absolute(tmp);
        filesystem::create_directories(m_filePath.parent_path());  // create the directory if it doesn't exist
    }
    m_maxFileSize = cfg.GetNumber(section, "maxFileSize", 20 * 1024 * 1024);
    m_maxWriteDelay = cfg.GetNumber(section, "maxWriteDelay", 500);
    m_maxOldFiles = cfg.GetNumber(section, "maxOldFiles", 0);

    m_minEmailLevel = (LogLevel)cfg.GetNumber(section, "minEmailLevel", (int)LogLevel::Verbose);
    m_emailSection = cfg.GetString(section, "emailSection", "");
    m_emailRecipients = (*cfg.GetJson())[section]["emailRecipients"].get<vector<string>>();
    m_emailSubject = cfg.GetString(section, "emailSubject", "");
    m_maxEmailDelay = cfg.GetNumber(section, "maxEmailDelay", 300);
    m_maxEmailLogs = cfg.GetNumber(section, "maxEmailLogs", 1000);
    m_emailTimeoutOnShutdown = cfg.GetNumber(section, "emailTimeoutOnShutdown", 3000);

    if (m_emailSection.empty() || m_emailRecipients.empty())
    {
        // make sure m_emailSection is empty, that's our signal that email logging is not configured
        m_emailSection = "";
        // email logging not fully configured, disable it
        m_minEmailLevel = MaskAllLogs;
    }
    else
    {
        if (m_emailSubject.empty())
        {
            // provide a portable default subject in the form of "logs from software @ host"
            m_emailSubject = GetExecutableName() + " @ " + GetHostname();
        }

        m_emailSender.Configure(cfg, m_emailSection);
    }

    m_logThreadId = cfg.GetBool(section, "logThreadId", false);
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

        LOGSTR() << "minEmailLevel=" << m_minEmailLevel << ", emailSection=" << m_emailSection
                 << ", emailRecipients=" << JoinStrings(m_emailRecipients, ", ") << ", emailSubject=" << m_emailSubject
                 << ", maxEmailDelay=" << m_maxEmailDelay << ", maxEmailLogs=" << m_maxEmailLogs
                 << ", emailTimeoutOnShutdown=" << m_emailTimeoutOnShutdown;
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

void Logger::Mute(bool mute) { m_mute = mute; }

void Logger::Log(LogLevel level, const string& message, const char* file, const char* func)
{
    if (m_mute || !m_running || (level < m_minConsoleLevel && level < m_minFileLevel && level < m_minEmailLevel))
    {
        return;
    }

    // if file and function are provided, use them to get the location prefix
    string locationPrefix = (file && func) ? GetLocationPrefix(file, func) : "";

    struct tm* localTime;
    int milliseconds;
    GetCurrentLocalTime(localTime, milliseconds);

    const char* levelName;
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
        uint32_t threadIdHash = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
        // convert it to a string
#pragma warning(suppress : 6031)
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
    size_t maxSize = message.length() + locationPrefix.length() + 60;  // 60 - timestamp, log level, threadId etc
    string fullMessage;
    fullMessage.resize(maxSize);  // 50 - timestamp, log level etc
    size_t actualLength =
        snprintf(&fullMessage[0], maxSize - 1, "%04d-%02d-%02d %02d:%02d:%02d.%03d [%s] %s%s%s\n", localTime->tm_year + 1900,
                 localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec, milliseconds,
                 levelName, threadIdPrefix, locationPrefix.c_str(), message.c_str());
    if (actualLength < 0)
    {
        // snprintf failed, so the buffer is obviously too small. It shouldn't happen, but let's handle it anyway.
        actualLength = maxSize - 1;
        fullMessage[actualLength - 1] = '\n';
    }
    fullMessage.resize(actualLength);

    // now obtain the lock to avoid messing up the output or crashing the queue when multiple threads are logging
    lock_guard<mutex> lock(m_cs);

    if (m_minConsoleLevel <= level)
    {
        cout << fullMessage;
    }

    // we deliberately ignore the logs from EmailSender, because we don't want them to start an email sending loop
    if (m_minEmailLevel <= level && (NULLOREMPTY(file) || !string(file).ends_with("EmailSender.cpp")))
    {
        if (m_emailQueue->empty())
        {
            m_emailTimestamp = SteadyTime();
        }
        m_emailQueue->push(fullMessage);
    }

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
    int maxSize = 5000;
    string message;
    message.resize(maxSize);

    int actualLength = _vsnprintf(&message[0], maxSize - 1, pszFmt, p);
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
        cerr << "Logger::Thread: exception while flushing file queue: " << e.what() << endl;
        // For the time being, we're just catching the exception and hope it was temporary.
    }
    catch (...)
    {
        // we can't afford to properly log exceptions here, because it might push us into a loop.
        cerr << "Logger::Thread: exception while flushing file queue" << endl;
        // For the time being, we're just catching the exception and hope it was temporary.
    }

    FlushEmailQueue(force);
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

    auto fileSize = outFile.tellp();
    outFile.close();

    // rotate file if needed
    if (m_maxFileSize > 0 && fileSize > m_maxFileSize)
    {
        // file grew too large, rename it
        struct tm* localTime;
        int dummy;
        GetCurrentLocalTime(localTime, dummy);

        char timestamp[32];
#pragma warning(suppress : 6031)
        snprintf(timestamp, sizeof(timestamp) - 1, "%04d%02d%02d%02d%02d%02d", localTime->tm_year + 1900, localTime->tm_mon + 1,
                 localTime->tm_mday, localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
        AUTO_TERMINATE(timestamp);

        auto extension = m_filePath.extension();
        auto baseName = m_filePath.stem();
        auto newFileName = m_filePath.parent_path() / (baseName.string() + "." + timestamp + extension.string());
        filesystem::rename(m_filePath, newFileName);

        // remove old files if needed
        if (m_maxOldFiles > 0)
        {
            vector<filesystem::path> oldFiles;

            // iterate through the directory and create a list of all old files
            auto folderIterator = filesystem::directory_iterator(m_filePath.parent_path());
            for (const auto& entry : folderIterator)
            {
                if (entry.is_regular_file() && entry.path().extension() == extension &&
                    entry.path().stem().string().find(baseName.string()) == 0)
                {
                    oldFiles.push_back(entry.path());
                }
            }

            if (oldFiles.size() > m_maxOldFiles)
            {
                // sort old files by name (name should contain timestamp, so we're basically sorting by time)
                sort(oldFiles.begin(), oldFiles.end());

                // remove the oldest files
                for (size_t i = 0; i < oldFiles.size() - m_maxOldFiles; i++)
                {
                    filesystem::remove(oldFiles[i]);
                }
            }
        }
    }
}

void Logger::FlushEmailQueue(bool force)
{
    m_cs.lock();

    if (m_emailQueue->empty() ||
        (!force && (int)m_emailQueue->size() < m_maxEmailLogs && (int)(SteadyTime() - m_emailTimestamp) < m_maxEmailDelay * 1000))
    {
        // nothing to flush yet, let's unlock and return
        m_cs.unlock();
        return;
    }

    auto queueCopy = std::move(m_emailQueue);
    m_emailQueue = std::make_unique<queue<string>>();  // create a new queue for future logs
    // we're done with m_emailQueue, it is now freshly initialized
    // let's unlock the logger and then take care of the email sending
    m_cs.unlock();

    // We need to start a new thread to send the email, because it might take a while and we don't want to block the logger thread (even
    // when shutting down - we need to shut down reasonably fast, regardless of the email delivery).
    // NOTE: it is dangerous to detach a thread that depends on the logger, because the logger might be destroyed before the thread
    // finishes. However, since SendSimpleEmail only needs the class data to prepare the email (which is done at the beginning), the chances
    // of a crash are low.
    // TODO: (not really urgent) make the email delivery independent of basically everything, so that it can be safely detached.
    auto smtpThread = thread(&Logger::SendEmail, this, std::move(queueCopy));
    if (m_running)
    {
        // fire & forget: assume that we'll be running long enough for the SMTP delivery to complete
        smtpThread.detach();
    }
    else
    {
        // we're shutting down, so we can't simply forget about the SMTP delivery because it would most likely get canceled if we simply
        // detach the thread. So, instead of detaching the thread, we try to join it in a reasonable time.
        // NOTE that we can't afford the usual long(ish) delivery timeout, but a shorter one. The SendMail method takes care of overriding
        // the default timeout when shutting down.
        smtpThread.join();
    }

    if (force)
    {
        // forced flush usually means that the software is shutting down. In such cases, a small delay might
        // prevent crash due to detaching thread too early (see the comment above).
        SLEEP(100);
    }
}

void Logger::SendEmail(unique_ptr<queue<string>> emailQueue)
{
    // prepare the email content
    std::ostringstream oss;
    while (!emailQueue->empty())
    {
        oss << emailQueue->front();
        emailQueue->pop();
    }

    m_emailSender.SendSimpleEmail(m_emailSubject, oss.str(), m_emailRecipients, "", m_running ? 0 : m_emailTimeoutOnShutdown);
}

string Logger::GetLocationPrefix(const char* file, const char* func)
{
    assert(!NULLOREMPTY(file));
    assert(!NULLOREMPTY(func));
    if (strstr(func, "::"))
    {
        return string(func) + ": ";
    }

    // the function name does not contain the class name, so we should log the file name to make it clear where the log came from
    const char* g = file + strlen(file) - 1;
    const char *f = g, *dot = g;
    while (f > file && *f != filesystem::path::preferred_separator)
    {
        if (dot == g && *f == '.')
        {
            dot = f;
        }
        f--;
    }
    if (*f == filesystem::path::preferred_separator)
    {
        f++;
    }

    string fileName(f, dot - f + 1);
    if (dot == g)
    {
        // dot not found in file name (strange, but possible I guess)
        return fileName + "." + func + ": ";
    }
    else
    {
        return fileName + func + ": ";
    }
}

LoggerStream::LoggerStream()
{
    m_level = Debug;
    m_file = nullptr;
    m_func = nullptr;
}

std::ostringstream& LoggerStream::Get(LogLevel level)
{
    m_level = level;
    return m_buffer;
}

std::ostringstream& LoggerStream::GetEx(const char* file, const char* func, LogLevel level)
{
    m_file = file;
    m_func = func;
    m_level = level;
    return m_buffer;
}

LoggerStream::~LoggerStream() { Logger::GetInstance()->Log(m_level, m_buffer.str(), m_file, m_func); }
