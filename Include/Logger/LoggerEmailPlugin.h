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

class LoggerEmailPlugin : public ILoggerPlugin, NoCopy
{
   public:
    static void ConfigureAll(JsonConfig& cfg, Logger& logger, const string& parentSection = "log.email");

    LoggerEmailPlugin(JsonConfig& cfg, const string& section);
    ~LoggerEmailPlugin();

    virtual void Log(LogLevel level, const string& message);
    virtual LogLevel MinLogLevel();
    virtual void Flush(bool stillRunning, bool force);

   private:
    LogLevel m_minLogLevel;
    vector<string> m_recipients;
    string m_subject;
    string m_emailSection;
    int m_maxDelay;
    int m_maxLogs;
    int m_timeoutOnShutdown;

    EmailSender m_emailSender;
    unique_ptr<queue<string>> m_queue;
    uint64_t m_queueTimestamp;

    mutex m_cs;  // TODO: a ga rabim? Uporabljam?

    void SendEmail(unique_ptr<queue<string>> emailQueue, bool stillRunning);
};

#endif
