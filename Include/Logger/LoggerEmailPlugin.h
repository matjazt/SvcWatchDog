/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
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
