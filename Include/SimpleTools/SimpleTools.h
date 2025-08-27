/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef _SIMPLETOOLS_H_
#define _SIMPLETOOLS_H_

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <mutex>
#include <condition_variable>

#include <string.h>

#define SAFE_STRING_COPY(t, s)                                                                         \
    do                                                                                                 \
    {                                                                                                  \
        static_assert(std::is_array_v<decltype(t)>, "Error: target must be an array, not a pointer!"); \
        strncpy((t), (s), sizeof(t) - 1);                                                              \
        (t)[sizeof(t) - 1] = 0;                                                                        \
    } while (0)

#define AUTO_TERMINATE(s)                                                                                 \
    do                                                                                                    \
    {                                                                                                     \
        static_assert(std::is_array_v<decltype(s)>, "Error: parameter must be an array, not a pointer!"); \
        s[sizeof(s) - 1] = 0;                                                                             \
    } while (0)

#define SAFE_DELETE(x)   \
    do                   \
    {                    \
        if (x)           \
        {                \
            delete x;    \
            x = nullptr; \
        }                \
    } while (0)

#define SAFE_DELETE_ARRAY(x) \
    do                       \
    {                        \
        if (x)               \
        {                    \
            delete[] x;      \
            x = nullptr;     \
        }                    \
    } while (0)

#define SAFE_FREE(x)     \
    do                   \
    {                    \
        if (x)           \
        {                \
            free(x);     \
            x = nullptr; \
        }                \
    } while (0)

#define SAFE_CLOSE_SOCKET(s)         \
    do                               \
    {                                \
        if ((s) != (INVALID_SOCKET)) \
        {                            \
            closesocket(s);          \
            s = INVALID_SOCKET;      \
        }                            \
    } while (0)

#define EMPTYIFNULL(s) ((s) ? (s) : "")
#define NULLOREMPTY(s) ((s) == nullptr || *(s) == 0)

std::string LoadTextFile(const std::filesystem::path& filePath);

void GetCurrentLocalTime(struct tm& localTime, int& milliseconds) noexcept;

uint64_t SteadyTime() noexcept;
#define SLEEP(MILLISECONDS) std::this_thread::sleep_for(std::chrono::milliseconds(MILLISECONDS))

std::filesystem::path GetExecutableFullPath();
std::string GetExecutableName();
std::string GetHostname();

class NoCopy
{
   public:
    NoCopy() noexcept {}

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

template <typename T>
std::string NumberFormat(T num, const char* formatString)
{
    std::string retVal;
    retVal.resize(100);
    int l = snprintf(&retVal[0], retVal.size() - 1, formatString, num);
    if (l > 0)
    {
        retVal.resize(l);
    }
    return retVal;
}
#define FLOAT2(f) NumberFormat((f), "%3.2lf")
#define FLOAT3(f) NumberFormat((f), "%3.3lf")
#define FLOAT4(f) NumberFormat((f), "%3.4lf")
#define FLOAT5(f) NumberFormat((f), "%3.5lf")
#define FLOAT6(f) NumberFormat((f), "%3.6lf")
#define FLOAT7(f) NumberFormat((f), "%3.7lf")
#define FLOAT8(f) NumberFormat((f), "%3.8lf")
#define FLOAT9(f) NumberFormat((f), "%3.9lf")
#define FLOATFULL(f) NumberFormat((f), "%3.17lf")
#define BOOL2STR(b) ((b) ? "true" : "false")

std::vector<std::string> Split(const std::string& str, char delimiter);
std::string JoinStrings(const std::vector<std::string>& words, const std::string& delimiter);
std::string TrimEx(const std::string& str, const std::string& leftTrimChars, const std::string& rightTrimChars);
std::string Trim(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");
std::string TrimLeft(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");
std::string TrimRight(const std::string& str, const std::string& trimChars = " \t\n\r\f\v");
std::string GetFileStem(const char* file);
std::string GetLocationPrefix(const char* file, const char* funcSignature);

#ifdef WIN32

#define SAFE_CLOSE_HANDLE(h) \
    do                       \
    {                        \
        if (h)               \
        {                    \
            CloseHandle(h);  \
            h = nullptr;     \
        }                    \
    } while (0)

#define O_SYNC 0
#define strcasecmp stricmp

#define FUNC_SIGNATURE __FUNCSIG__

#endif

#ifdef LINUX

#define UNIX

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

#define _stat stat

#define __stdcall
#define Sleep(ms) usleep((unsigned long)(ms) * 1000ul)

#define O_BINARY 0

#define stricmp strcasecmp

#define _MAX_PATH 256
#define MAX_PATH 256

#define _chdir(d) chdir(d)
#define _getcwd(buf, size) getcwd(buf, size)
#define _vsnprintf vsnprintf

#define _ftime ftime
#define _timeb struct timeb

#define FUNC_SIGNATURE __PRETTY_FUNCTION__

#endif

/**
 * @class SyncEvent
 * @brief A thread synchronization primitive that mimics the behavior of Win32 event objects.
 * It allows threads to wait for or signal events.
 *
 * SyncEvent provides a mechanism for threads to wait until an event is signaled.
 * It supports both manual-reset and auto-reset modes:
 * - In manual-reset mode, the event remains signaled until explicitly reset.
 * - In auto-reset mode, the event automatically resets after releasing a single waiting thread.
 *
 * Typical usage:
 * - One or more threads call WaitForSingleEvent() to wait for the event to be signaled.
 * - Another thread calls SetEvent() to signal the event, releasing waiting threads.
 * - ResetEvent() can be used to reset the event to the non-signaled state (manual-reset mode).
 *
 * @note This class is thread-safe.
 */
class SyncEvent : public NoCopy
{
   public:
    /**
     * @brief Constructs a SyncEvent.
     * @param initialState Initial signaled state of the event.
     * @param autoReset If true, event is auto-reset; otherwise, manual-reset.
     */
    SyncEvent(bool initialState, bool autoReset);

    /**
     * @brief Sets the event to the signaled state.
     *        Wakes one or all waiting threads depending on auto-reset mode.
     * @return true if the event was not signaled before the call, false if it was already signaled.
     */
    bool SetEvent();

    /**
     * @brief Resets the event to the non-signaled state.
     * @return true if the event was signaled before the call, false if it was already non-signaled.
     */
    bool ResetEvent();

    /**
     * @brief Waits indefinitely for the event to become signaled.
     */
    void WaitForSingleEvent();

    /**
     * @brief Waits for the event to become signaled or until timeout.
     * @param milliseconds Timeout in milliseconds.
     * @return true if the event was signaled, false if timeout occurred.
     */
    bool WaitForSingleEvent(int milliseconds);

   private:
    bool m_autoReset;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    bool m_signaled;
};

/**
 * @class Stopwatch
 * @brief A high-precision timing utility class for measuring both wall clock and CPU time.
 *
 * The Stopwatch class provides functionality to measure elapsed time using two different timing mechanisms:
 * - Wall clock time (real-world time) using std::chrono::steady_clock
 * - CPU time (processor time consumed by the current process) using std::clock
 *
 * The stopwatch automatically starts timing upon construction and can be controlled with Start() and Stop() methods.
 * Time measurements can be retrieved even while the stopwatch is running, providing real-time elapsed time information.
 *
 * @note Wall clock time represents actual elapsed time, while CPU time represents the amount of
 *       processor time consumed by the program. These values may differ significantly in
 *       multi-threaded applications or when the process is suspended.
 */
class Stopwatch : public NoCopy
{
   public:
    /**
     * @brief Default constructor that initializes the stopwatch and immediately starts timing.
     */
    Stopwatch(std::string name = "");

    /**
     * @brief Starts or restarts the timing measurement.
     *
     * Records the current wall clock time using std::chrono::steady_clock and the current
     * CPU time using std::clock() as the starting reference points. Sets the stopwatch
     * to running state, allowing elapsed time calculations.
     *
     * @note If the stopwatch is already running, this method will restart the timing
     *       from the current moment, discarding any previous timing data.
     */
    void Start();

    /**
     * @brief Stops the timing measurement and records the end times.
     *
     * Captures the current wall clock time and CPU time as end points for timing calculations.
     * Sets the stopwatch to stopped state. After stopping, elapsed time methods will return
     * the time duration between the last Start() and this Stop() call.
     *
     * @note If ElapsedWallMilliseconds() or ElapsedCpuMilliseconds() are called after stopping,
     *       they will return the elapsed time from the last Start() to Stop() interval.
     */
    void Stop();

    /**
     * @brief Returns the elapsed wall clock time in milliseconds.
     *
     * Calculates the elapsed real-world time since the last Start() call. If the stopwatch
     * is currently running, returns the time elapsed up to the current moment. If stopped,
     * returns the time elapsed between the last Start() and Stop() calls.
     *
     * @return double The elapsed wall clock time in milliseconds with high precision.
     */
    double ElapsedWallMilliseconds() const;

    /**
     * @brief Returns the elapsed CPU time in milliseconds.
     *
     * Calculates the amount of processor time consumed by the current process since the
     * last Start() call. If the stopwatch is currently running, returns the CPU time
     * consumed up to the current moment. If stopped, returns the CPU time consumed
     * between the last Start() and Stop() calls.
     *
     * @return double The elapsed CPU time in milliseconds.
     *
     * @note CPU time represents actual processor time used by the program and may be
     *       significantly different from wall clock time, especially in multi-threaded
     *       applications or when the process is waiting for I/O operations.
     */
    double ElapsedCpuMilliseconds() const;

    /**
     * @brief Returns a human-readable summary of the stopwatch.
     */
    std::string SummaryText() const;

   private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_startWall, m_endWall;
    std::clock_t m_startCpu = 0, m_endCpu = 0;
    bool m_running = false;
};

/**
 * @class CallGraphMonitor
 * @brief Provides performance monitoring capabilities for function call hierarchies.
 *
 * This class tracks function entry/exit times and maintains statistics about call patterns
 * and execution durations. The class follows a singleton pattern and is designed
 * to be thread-safe for basic operations.
 *
 * Key features:
 * - Tracks nested function calls as a call stack
 * - Maintains execution time statistics per unique call path
 * - Provides summary reports sorted by total execution time
 * - Integrates with CallGraphMonitorAgent for automatic scope-based monitoring
 *
 * Usage:
 *   1. use the CALL_GRAPH_MONITOR_INITIALIZE macro to initialize and calibrate the singleton
 *   2. Use CALL_GRAPH_MONITOR_AGENT macro at the beginning of functions to automatically track calls
 *   3. Call SummaryText() to get performance statistics or use the CALL_GRAPH_MONITOR_LOG_STATS macro
 *
 * @note The associated macros are controlled by the CALL_GRAPH_MONITOR_ENABLED preprocessor definition.
 *       When this symbol is not defined (which is the default), the macros expand to empty statements,
 *       ensuring zero runtime overhead.
 *
 * @note Although CallGraphMonitor measures its own overhead and compensates for it in the reported timings,
 *       there may still be cases where extremely fast functions are not measured accurately. Besides that,
 *       compiling your project with CallGraphMonitor enabled can alter the optimizations the compiler can do
 *       and may lead to different inlining decisions or other performance-related changes.
 */
class CallGraphMonitor : public NoCopy
{
   public:
    /**
     * @brief Default constructor that initializes the call graph monitor.
     */
    CallGraphMonitor() noexcept;

    /**
     * @brief Gets the current singleton instance of the call graph monitor.
     * @return Pointer to the current instance, or nullptr if not set.
     */
    static CallGraphMonitor* GetInstance() noexcept;

    /**
     * @brief Sets the singleton instance of the call graph monitor.
     * @param instance Pointer to the CallGraphMonitor instance to use as singleton.
     */
    static void SetInstance(CallGraphMonitor* instance) noexcept;

    /**
     * @brief Calibrates the monitoring system by measuring overhead and adjusting timings.
     */
    void Calibrate();

    /**
     * @brief Records the start of a function call and pushes it onto the call stack.
     * @param functionName Name of the function being entered.
     */
    void StartFunction(const std::string& functionName);

    /**
     * @brief Records the end of the current function call and updates statistics.
     */
    void StopFunction();

    /**
     * @brief Clears all collected statistics and resets the monitor to initial state.
     */
    void Reset();

    /**
     * @brief Generates a formatted summary report of all collected call statistics.
     * @return String containing performance statistics sorted by total execution time.
     */
    std::string SummaryText();

   private:
    static CallGraphMonitor* m_instance;

    std::string GetCallStackAsString() const;  // last method on top

    std::mutex m_mtx;
    uint64_t m_totalCallCount = 0;
    double m_overheadPerCall = 0;

    struct CallInfo
    {
        std::string functionName;
        std::chrono::steady_clock::time_point startTime;
        uint64_t totalCallCount;
        // std::chrono::steady_clock::time_point endTime;
    };

    std::vector<CallInfo> m_callStack;

    struct CallStackStats
    {
        uint64_t callCount = 0;
        uint64_t totalDuration = 0;
    };

    std::map<std::string, CallStackStats> m_callStackStats;

    struct CallStackSummaryStats
    {
        std::string callStack;
        uint64_t callCount = 0;
        uint64_t totalDuration = 0;
        uint64_t averageDuration = 0;
    };
};

class CallGraphMonitorAgent : public NoCopy
{
   public:
    /**
     * @brief Constructor that automatically starts monitoring for the current function scope.
     * @param file Source file name where the agent is created.
     * @param func Function name being monitored.
     */
    CallGraphMonitorAgent(const char* file, const char* func);

    /**
     * @brief Destructor that automatically stops monitoring when leaving function scope.
     */
    ~CallGraphMonitorAgent();
};

#ifdef CALL_GRAPH_MONITOR_ENABLED
/**
 * @brief Initializes a CallGraphMonitor instance and sets it as the singleton.
 *
 * Creates a local CallGraphMonitor object and configures it as the global instance
 * for call graph monitoring. Should be called once at application startup.
 */
#define CALL_GRAPH_MONITOR_INITIALIZE()               \
    CallGraphMonitor callGraphMonitor;                \
    CallGraphMonitor::SetInstance(&callGraphMonitor); \
    callGraphMonitor.Calibrate();

/**
 * @brief Logs the current call graph statistics to the application log.
 *
 * Outputs a formatted summary of all collected performance statistics to the
 * Information log level using the LOGSTR macro.
 */
#define CALL_GRAPH_MONITOR_LOG_STATS() LOGSTR(Information) << "CallGraphMonitor statistics:\n" << callGraphMonitor.SummaryText()

/**
 * @brief Creates a scope-based monitoring agent for the current function.
 *
 * Instantiates a CallGraphMonitorAgent that automatically tracks function entry/exit
 * times for the current scope. The agent is destroyed when leaving the scope.
 */
#define CALL_GRAPH_MONITOR_AGENT() CallGraphMonitorAgent __callGraphAgent(__FILE__, FUNC_SIGNATURE);
#else
/**
 * @brief Empty macros to be used when monitoring is disabled.
 *
 * When CALL_GRAPH_MONITOR_ENABLED is not defined, these macros expand to nothing,
 * ensuring absolutely no processing overhead.
 */
#define CALL_GRAPH_MONITOR_INITIALIZE()
#define CALL_GRAPH_MONITOR_LOG_STATS()
#define CALL_GRAPH_MONITOR_AGENT()
#endif

#endif
