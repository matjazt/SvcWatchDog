/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <JsonConfig/JsonConfig.h>
#include <vector>
#include <queue>
#include <sstream>
#include <thread>

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

class ILoggerPlugin
{
   public:
    virtual ~ILoggerPlugin() = default;
    virtual void Log(LogLevel level, const std::string& message) = 0;
    virtual LogLevel MinLogLevel() = 0;
    virtual void Flush(bool stillRunning, bool force) = 0;
};

class Logger : public NoCopy
{
   public:
    Logger() noexcept;
    ~Logger();

    static Logger* GetInstance() noexcept;
    static void SetInstance(Logger* instance) noexcept;

    void Configure(JsonConfig& cfg, const std::string& section = "log");
    void RegisterPlugin(std::unique_ptr<ILoggerPlugin> plugin);
    LogLevel GetMinPluginLevel();

    void Start();
    void Shutdown();
    void Mute(bool mute) noexcept;
    void Log(LogLevel level, const std::string& message, const char* file = nullptr, const char* func = nullptr);
    void Msg(LogLevel level, const char* pszFmt, ...);

   private:
    static Logger* m_instance;

    LogLevel m_minConsoleLevel;
    LogLevel m_minFileLevel;
    std::filesystem::path m_filePath;
    int m_maxFileSize;
    int m_maxWriteDelay;
    size_t m_maxOldFiles;
    // LogLevel m_minEmailLevel;
    // string m_emailSection;
    // vector<string> m_emailRecipients;
    // string m_emailSubject;
    // int m_maxEmailDelay;
    // int m_maxEmailLogs;
    // int m_emailTimeoutOnShutdown;
    bool m_logThreadId;

    std::vector<std::unique_ptr<ILoggerPlugin>> m_plugins;
    bool m_mute;
    std::unique_ptr<std::queue<std::string>> m_fileQueue;
    uint64_t m_emailTimestamp;
    std::thread m_thread;
    SyncEvent m_threadTrigger;
    bool m_running;

    std::mutex m_cs;

    void Thread();
    void Flush(bool force);
    void FlushFileQueue();
};

#define Lg (*Logger::GetInstance())
#define LOGSTR(...) LoggerStream().GetEx(__FILE__, FUNC_SIGNATURE __VA_OPT__(, __VA_ARGS__))  // optional log level;
// note that __VA_OPT__ was introduced in C++20, so this macro will only work with C++20 or later. For earlier versions, you can
// use compiler-specific hacks (like ##__VA_ARGS__ in GCC/Clang/MSVC)
#define LOGMSG(LEVEL, MSG) Logger::GetInstance()->Log((LEVEL), (MSG), __FILE__, FUNC_SIGNATURE);
#define LOGASSERT(CONDITION)                                                                                                      \
    do                                                                                                                            \
    {                                                                                                                             \
        if (!(CONDITION))                                                                                                         \
        {                                                                                                                         \
            Logger::GetInstance()->Log(Fatal, "assertion failure at line " + std::to_string(__LINE__), __FILE__, FUNC_SIGNATURE); \
        }                                                                                                                         \
    } while (0)

// logging macros which causes the logs to only *compile* in debug mode, so they have no impact whatsoever in release mode
#if defined(_DEBUG) || defined(FORCE_LOG_DEBUG)
#define LOG_DEBUG(a) LOGSTR(Debug) << a;
#else
#define LOG_DEBUG(a) ;
#endif

#if defined(_DEBUG) || defined(FORCE_LOG_VERBOSE)
#define LOG_VERBOSE(a) LOGSTR(Verbose) << a;
#else
#define LOG_VERBOSE(a) ;
#endif

class LoggerStream : public NoCopy
{
   public:
    LoggerStream() noexcept;
    virtual ~LoggerStream();
    std::ostringstream& Get(LogLevel level = Debug) noexcept;
    std::ostringstream& GetEx(const char* file, const char* func, LogLevel level = Debug) noexcept;

   protected:
    std::ostringstream m_buffer;

   private:
    LoggerStream(const LoggerStream&);
    const char* m_file;
    const char* m_func;
    LogLevel m_level;
};

#endif
