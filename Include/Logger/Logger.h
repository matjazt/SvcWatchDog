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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <JsonConfig/JsonConfig.h>
#include <vector>
#include <queue>
#include <sstream>
#include <thread>
#include <atomic>

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

/**
 * Logger plugin interface.
 *
 * Plugins must be registered before any logging threads start.
 * The Log() callback must be fast and non-blocking; use internal buffering/queueing if I/O is required.
 * Never call Logger methods from within plugin callbacks to avoid deadlock.
 */
class ILoggerPlugin
{
   public:
    virtual ~ILoggerPlugin() = default;

    // Called from any thread; must be fast and non-blocking.
    virtual void Log(LogLevel level, const std::string& message) = 0;

    // Returns the minimum log level this plugin wants to receive.
    virtual LogLevel MinLogLevel() = 0;

    // Called periodically and during shutdown; may block briefly.
    virtual void Flush(bool stillRunning, bool force) = 0;
};

/**
 * Thread-safe logging system with console, file, and plugin output.
 *
 * Typical usage:
 *   1. Configure() - set log levels and file paths
 *   2. RegisterPlugin() - add any plugins (before Start)
 *   3. Start() - begin logging thread
 *   4. [application runs, calls Log/LOGSTR/etc]
 *   5. Shutdown() - stop logging thread and flush
 */
class Logger
{
   public:
    Logger() noexcept;
    ~Logger();

    // prevent copying and assignment
    DELETE_COPY_AND_ASSIGNMENT(Logger);

    static Logger* GetInstance() noexcept;
    static void SetInstance(Logger* instance) noexcept;

    void SetFileNamePostfix(const std::string& postfix) noexcept;
    void Configure(JsonConfig& cfg, const std::string& section = "log");

    // Register plugins before Start() and before spawning additional threads.
    void RegisterPlugin(std::unique_ptr<ILoggerPlugin> plugin);
    LogLevel GetMinPluginLevel();

    void Start();     // Starts the background logging thread.
    void Shutdown();  // Stops the logging thread and flushes all output.
    void Mute(bool mute) noexcept;
    void Log(LogLevel level, const std::string& message, const char* file = nullptr, const char* func = nullptr);
    void Msg(LogLevel level, const char* pszFmt, ...);

    void Flush(
        bool force);  // flush both file queue and plugins - but avoid doing it manually, since the logger thread does it automatically

   private:
    static Logger* m_instance;

    LogLevel m_minConsoleLevel;
    LogLevel m_minFileLevel;
    std::filesystem::path m_filePath;
    std::string
        m_fileNamePostfix;  // used when we run multiple instances of the same app on the same machine, for example several MPI processes
    int m_maxFileSize;
    int m_maxWriteDelay;
    size_t m_maxOldFiles;
    bool m_logThreadId;

    std::vector<std::unique_ptr<ILoggerPlugin>> m_plugins;
    std::atomic_bool m_mute;
    std::unique_ptr<std::queue<std::string>> m_fileQueue;
    uint64_t m_emailTimestamp;
    std::thread m_thread;
    SyncEvent m_threadTrigger;
    std::atomic_bool m_running;

    std::mutex m_cs;

    void Thread();
    void FlushFileQueue();
    void LogErrorToConsole(const std::string& message);
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
            Logger::GetInstance()->Flush(false);                                                                                  \
        }                                                                                                                         \
    } while (0)

// logging macros which causes the logs to only *compile* in debug mode, so they have no impact whatsoever in release mode
#if (defined(_DEBUG) || defined(FORCE_LOG_DEBUG)) && !defined(PREVENT_LOG_DEBUG)
#define LOG_DEBUG(a) LOGSTR(Debug) << a;
#define LOG_DEBUG_ENABLED
#else
#define LOG_DEBUG(a) ;
#endif

#if (defined(_DEBUG) || defined(FORCE_LOG_VERBOSE)) && !defined(PREVENT_LOG_VERBOSE)
#define LOG_VERBOSE(a) LOGSTR(Verbose) << a;
#define LOG_VERBOSE_ENABLED
#else
#define LOG_VERBOSE(a) ;
#endif

class LoggerStream
{
   public:
    LoggerStream() noexcept;
    virtual ~LoggerStream();

    // prevent copying and assignment
    DELETE_COPY_AND_ASSIGNMENT(LoggerStream);

    std::ostringstream& Get(LogLevel level = Debug) noexcept;
    std::ostringstream& GetEx(const char* file, const char* func, LogLevel level = Debug) noexcept;

    // testing & debugging methods
    std::string GetBuffer() const;

   protected:
    std::ostringstream m_buffer;

   private:
    const char* m_file;
    const char* m_func;
    LogLevel m_level;
};

#endif
