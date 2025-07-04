/*
 * MIT License
 *
 * Copyright (c) 2025 Matja� Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef __LOGGER_H
#define __LOGGER_H

#include <JsonConfig/JsonConfig.h>
#include <queue>
#include <sstream>

enum LogLevel
{
    // Anything and everything you might want to know about a running block of code.
    Verbose,

    // Internal system events that aren't necessarily observable from the outside.
    Debug,

    // The lifeblood of operational intelligence - things happen.
    Information,

    // Service is degraded or endangered.
    Warning,

    // Functionality is unavailable, invariants are broken or data is lost.
    Error,

    // If you have a pager, it goes off when one of these occurs.
    Fatal,

    // No logging at all.
    MaskAllLogs
};

class Logger : public NoCopy
{
   public:
    Logger();
    ~Logger();

    static Logger* GetInstance();
    static void SetInstance(Logger* instance);

    void Config(JsonConfig& cfg, const string& section);
    void Start();
    void Shutdown();
    void Mute(bool mute);
    void Log(LogLevel level, const string& message, const char* file = nullptr, const char* func = nullptr);
    void Msg(LogLevel level, const char* pszFmt, ...);

   private:
    static Logger* m_instance;

    LogLevel m_minConsoleLevel;
    LogLevel m_minFileLevel;
    filesystem::path m_filePath;
    bool m_logThreadId;
    int m_maxFileSize;
    int m_maxWriteDelay;
    size_t m_maxOldFiles;
    bool m_mute;
    unique_ptr<queue<string>> m_queue;
    thread m_thread;
    SyncEvent m_threadTrigger;
    bool m_running;

    mutex m_cs;

    string GetLocationPrefix(const char* file, const char* func);
    void Thread();
    void Flush();
};

#define Lg (*Logger::GetInstance())
#define LOGSTR(...) LoggerStream().GetEx(__FILE__, __FUNCTION__, __VA_ARGS__)  // optional log level
#define LOGMSG(LEVEL, MSG) Logger::GetInstance()->Log((LEVEL), (MSG), __FILE__, __FUNCTION__);
#define LOGASSERT(CONDITION)                                                                                               \
    do                                                                                                                     \
    {                                                                                                                      \
        if (!(CONDITION))                                                                                                  \
        {                                                                                                                  \
            Logger::GetInstance()->Log(Fatal, "assertion failure at line " + to_string(__LINE__), __FILE__, __FUNCTION__); \
        }                                                                                                                  \
    } while (0)

class LoggerStream : public NoCopy
{
   public:
    LoggerStream();
    virtual ~LoggerStream();
    std::ostringstream& Get(LogLevel level = Debug);
    std::ostringstream& GetEx(const char* file, const char* func, LogLevel level = Debug);

   protected:
    std::ostringstream m_buffer;

   private:
    LoggerStream(const LoggerStream&);
    const char* m_file;
    const char* m_func;
    LogLevel m_level;
};

#endif
