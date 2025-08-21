/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <climits>
#endif

#include <SimpleTools/SimpleTools.h>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <algorithm>
#include <utility>

using namespace std;

string LoadTextFile(const filesystem::path& filePath)
{
    // Check if the file exists and is a regular file
    if (filePath.empty() || !filesystem::exists(filePath) || !filesystem::is_regular_file(filePath))
    {
        throw std::runtime_error("File does not exist or is not a valid file: " + filePath.string());
    }

    // Open the file using an ifstream
    std::ifstream file(filePath, std::ios::in);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open the file: " + filePath.string());
    }

    // Move the file pointer to the end to get the file size
    file.seekg(0, std::ios::end);
    const auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Reserve space and read the entire file into a string
    std::string content;
    content.resize((size_t)fileSize);
    file.read(&content[0], fileSize);

    return content;
}

void GetCurrentLocalTime(struct tm& localTime, int& milliseconds) noexcept
{
    // Get the current time as a time_point
    const auto now = std::chrono::system_clock::now();
    // Convert to time_t for formatting (seconds precision)
    const auto now_time = std::chrono::system_clock::to_time_t(now);

#if defined(_MSC_VER)
    localtime_s(&localTime, &now_time);
#else
    localtime_r(&now_time, &localTime);
#endif

    // localTime = std::localtime(&now_time);
    // Extract milliseconds from the current time_point
    milliseconds = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000).count();
}

uint64_t SteadyTime() noexcept
{
    const auto now = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return duration.count();
}

filesystem::path GetExecutableFullPath()
{
    char path[MAX_PATH];
#ifdef _WIN32
    GetModuleFileNameA(NULL, path, sizeof(path) - 1);
#else
    const ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len > 0 && len < (ssize_t)(sizeof(path) - 1))
    {
        path[len] = 0;  // Null-terminate the string, because readlink does not do it
    }
    else
    {
        SAFE_STRING_COPY(path, "unknown");
    }
#endif
    AUTO_TERMINATE(path);
    return filesystem::path(path);
}

string GetExecutableName()
{
    auto fullPath = GetExecutableFullPath();
    return fullPath.stem().string();
}

string GetHostname()
{
    char hostname[256] = "";
#ifdef _WIN32
    DWORD size = sizeof(hostname);
    const bool ok = GetComputerNameA(hostname, &size);
#else
    // TODO: not tested on linux
    const bool ok = gethostname(hostname, sizeof(hostname)) == 0;
#endif
    return ok ? hostname : "unknown";
}

// helper function to split a string by delimiter
vector<string> Split(const string& str, char delimiter)
{
    vector<std::string> tokens;
    size_t start = 0, end = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(str.substr(start));  // Add last token
    return tokens;
}

string JoinStrings(const vector<string>& words, const string& delimiter)
{
    string result;
    for (const string& word : words)
    {
        if (!result.empty())
        {
            result += delimiter;  // Add delimiter before the next word
        }
        result += word;
    }
    return result;
}

string TrimEx(const string& str, const string& leftTrimChars, const string& rightTrimChars)
{
    const size_t start = str.find_first_not_of(leftTrimChars);
    if (start == string::npos)
    {
        return "";  // All characters are trimmed
    }
    const size_t end = str.find_last_not_of(rightTrimChars);
    if (end == string::npos)
    {
        return str.substr(start);  // No right trim
    }
    return str.substr(start, end - start + 1);
}

string Trim(const string& str, const string& trimChars) { return TrimEx(str, trimChars, trimChars); }

string TrimLeft(const string& str, const string& trimChars) { return TrimEx(str, trimChars, ""); }

string TrimRight(const string& str, const string& trimChars) { return TrimEx(str, "", trimChars); }

string GetLocationPrefix(const char* file, const char* func)
{
    assert(!NULLOREMPTY(file));
    assert(!NULLOREMPTY(func));
    if (strstr(func, "::"))
    {
        return func;
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

    const string fileName(f, dot - f + 1);
    if (dot == g)
    {
        // dot not found in file name (strange, but possible I guess)
        return fileName + "." + func;
    }
    else
    {
        return fileName + func;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyncEvent
////////////////////////////////////////////////////////////////////////////////////////////////////////

SyncEvent::SyncEvent(bool initialState, bool autoReset) : m_autoReset(autoReset), m_signaled(initialState) {}

bool SyncEvent::SetEvent()
{
    const lock_guard<mutex> lock(m_mtx);
    const bool wasSignaled = m_signaled;
    m_signaled = true;
    if (m_autoReset)
    {
        // Notify one waiting thread if auto-reset is enabled
        m_cv.notify_one();
    }
    else
    {
        // Notify all waiting threads if auto-reset is disabled
        m_cv.notify_all();
    }
    return wasSignaled == false;
}

bool SyncEvent::ResetEvent()
{
    const lock_guard<mutex> lock(m_mtx);
    const bool wasSignaled = m_signaled;
    m_signaled = false;
    return wasSignaled;
}

void SyncEvent::WaitForSingleEvent()
{
    unique_lock<mutex> lock(m_mtx);
    m_cv.wait(lock, [this]() { return m_signaled; });
    if (m_autoReset)
    {
        m_signaled = false;  // Reset the event if auto-reset is enabled
    }
}

bool SyncEvent::WaitForSingleEvent(int milliseconds)
{
    unique_lock<mutex> lock(m_mtx);
    const bool success = m_cv.wait_for(lock, chrono::milliseconds(milliseconds), [this]() { return m_signaled; });
    if (success && m_autoReset)
    {
        m_signaled = false;  // Reset the event if auto-reset is enabled
    }
    return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stopwatch
////////////////////////////////////////////////////////////////////////////////////////////////////////

Stopwatch::Stopwatch(std::string name) : m_name(std::move(name)), m_startWall(), m_endWall() { Start(); }

void Stopwatch::Start()
{
    m_startWall = std::chrono::steady_clock::now();
    m_startCpu = std::clock();
    m_running = true;
}

void Stopwatch::Stop()
{
    m_endWall = std::chrono::steady_clock::now();
    m_endCpu = std::clock();
    m_running = false;
}

double Stopwatch::ElapsedWallMilliseconds() const
{
    auto end = m_running ? std::chrono::steady_clock::now() : m_endWall;
    std::chrono::duration<double, std::milli> const duration = end - m_startWall;
    return duration.count();
}

double Stopwatch::ElapsedCpuMilliseconds() const
{
    std::clock_t const end = m_running ? std::clock() : m_endCpu;
    return double(end - m_startCpu) * 1000.0 / CLOCKS_PER_SEC;
}

std::string Stopwatch::SummaryText() const
{
    return (m_name.empty() ? "Stopwatch" : m_name) + ": duration " + std::to_string(int(ElapsedWallMilliseconds())) + " ms, CPU time " +
           std::to_string(int(ElapsedCpuMilliseconds())) + " ms";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// CallGraphMonitor && CallGraphMonitorAgent
////////////////////////////////////////////////////////////////////////////////////////////////////////

CallGraphMonitor* CallGraphMonitor::m_instance = nullptr;

CallGraphMonitor::CallGraphMonitor() noexcept = default;

void CallGraphMonitor::Calibrate()
{
    // measure the overhead, introduced by each measurement
    auto calibrationStartTime = std::chrono::steady_clock::now();

    bool calibrationDone = false;
    while (!calibrationDone)
    {
        CALL_GRAPH_MONITOR_AGENT();

        auto now = std::chrono::steady_clock::now();
        calibrationDone = std::chrono::duration_cast<std::chrono::milliseconds>(now - calibrationStartTime).count() >= 500;
    }

    double overheadMeasurementTime =
        double(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - calibrationStartTime).count());

    m_overheadPerCall = overheadMeasurementTime / m_totalCallCount;

    // We need to somewhat adjust the measurement to account for the more populated m_callStackStats, expected in real scenarios.
    // There is no way to predict the exact behaviour in advance, so we use a calibration factor.
#ifdef CALL_GRAPH_MONITOR_CALIBRATION_FACTOR
    m_overheadPerCall *= CALL_GRAPH_MONITOR_CALIBRATION_FACTOR;
#elif WIN32
    m_overheadPerCall *= 2.5;
#else
    m_overheadPerCall *= 2.1;
#endif

    // clear the stats, we only needed them for calibration
    m_callStackStats.clear();
}

CallGraphMonitor* CallGraphMonitor::GetInstance() noexcept { return m_instance; }

void CallGraphMonitor::SetInstance(CallGraphMonitor* instance) noexcept { m_instance = instance; }

void CallGraphMonitor::StartFunction(const std::string& functionName)
{
    const lock_guard<mutex> lock(m_mtx);

    m_callStack.push_back({functionName, std::chrono::steady_clock::now(), m_totalCallCount});
    m_totalCallCount++;
}

void CallGraphMonitor::StopFunction()
{
    const lock_guard<mutex> lock(m_mtx);

    auto now = std::chrono::steady_clock::now();
    if (m_callStack.empty())
    {
        throw std::runtime_error("No function is currently being monitored.");
    }

    auto callStackString = GetCallStackAsString();
    auto& callInfo = m_callStack.back();
    uint64_t overhead = (uint64_t)((m_totalCallCount - callInfo.totalCallCount) * m_overheadPerCall);
    uint64_t duration = std::chrono::duration_cast<std::chrono::microseconds>(now - callInfo.startTime).count();
    // Ensure duration is not negative; it might be due to non-exact timings
    if (duration > overhead)
    {
        duration -= overhead;
    }
    else
    {
        duration = 0;
    }
    m_callStack.pop_back();

    // now update statistics
    auto& stats = m_callStackStats[callStackString];
    stats.callCount++;
    stats.totalDuration += duration;
}

void CallGraphMonitor::Reset()
{
    const lock_guard<mutex> lock(m_mtx);

    m_callStack.clear();
    m_callStackStats.clear();
}

std::string CallGraphMonitor::SummaryText()
{
    const lock_guard<mutex> lock(m_mtx);

    // first copy the stats from the dictionary to an array, then sort it by time and finally compose the summary text
    std::vector<CallStackSummaryStats> statsArray;
    statsArray.reserve(m_callStackStats.size());
    for (const auto& [callStack, stats] : m_callStackStats)
    {
        statsArray.push_back({callStack, stats.callCount, stats.totalDuration, stats.totalDuration / stats.callCount});
    }

    std::ranges::sort(statsArray,
                      [](const CallStackSummaryStats& a, const CallStackSummaryStats& b) { return a.totalDuration > b.totalDuration; });

    std::string summary;
    const char* separator = "";
    for (const auto& stats : statsArray)
    {
        summary += separator;
        separator = "\n";
        summary += std::to_string(stats.totalDuration) + " us : ";
        summary += stats.callStack;
        summary += " (" + std::to_string(stats.callCount) + " calls, ";
        summary += std::to_string(stats.averageDuration) + " us average)";
    }
    return summary;
}

std::string CallGraphMonitor::GetCallStackAsString() const
{
    std::string callStack;
    const char* separator = "";
    for (const auto& frame : m_callStack)
    {
        callStack += separator + frame.functionName;
        separator = " -> ";
    }
    return callStack;
}

CallGraphMonitorAgent::CallGraphMonitorAgent(const char* file, const char* func)
{
    auto instance = CallGraphMonitor::GetInstance();
    if (instance)
    {
        instance->StartFunction(GetLocationPrefix(file, func));
    }
}

CallGraphMonitorAgent::~CallGraphMonitorAgent()
{
    auto instance = CallGraphMonitor::GetInstance();
    if (instance)
    {
        instance->StopFunction();
    }
}
