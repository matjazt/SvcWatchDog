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
#include <limits.h>
#endif

#include <SimpleTools/SimpleTools.h>
#include <fstream>
#include <filesystem>

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
    auto fileSize = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);

    // Reserve space and read the entire file into a string
    std::string content;
    content.resize(fileSize);
    file.read(&content[0], fileSize);

    return content;
}

void GetCurrentLocalTime(struct tm*& localTime, int& milliseconds)
{
    // Get the current time as a time_point
    auto now = std::chrono::system_clock::now();
    // Convert to time_t for formatting (seconds precision)
    auto now_time = std::chrono::system_clock::to_time_t(now);
    localTime = std::localtime(&now_time);
    // Extract milliseconds from the current time_point
    milliseconds = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000).count();
}

uint64_t SteadyTime()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return duration.count();
}

string GetExecutableName()
{
    char path[MAX_PATH];
#ifdef _WIN32
    GetModuleFileNameA(NULL, path, sizeof(path) - 1);
#else
    // TODO: not tested on linux
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len <= 0)
    {
        return "unknown";
    }
#endif
    return std::filesystem::path(path).stem().string();
}

string GetHostname()
{
    char hostname[256];
#ifdef _WIN32
    DWORD size = sizeof(hostname);
    bool ok = GetComputerNameA(hostname, &size);
#else
    // TODO: not tested on linux
    bool ok = gethostname(hostname, sizeof(hostname)) == 0;
#endif
    return ok ? hostname : "unknown";
}

template <typename T>
string NumberFormat(T num, const char* formatString)
{
    string retVal;
    retVal.resize(100);
    int l = snprintf(&retVal[0], retVal.size() - 1, formatString, num);
    if (l > 0)
    {
        retVal.resize(l);
    }
    return retVal;
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
    size_t start = str.find_first_not_of(leftTrimChars);
    if (start == string::npos) return "";  // All characters are trimmed
    size_t end = str.find_last_not_of(rightTrimChars);
    if (end == string::npos) return str.substr(start);  // No right trim
    return str.substr(start, end - start + 1);
}

string Trim(const string& str, const string& trimChars) { return TrimEx(str, trimChars, trimChars); }

string TrimLeft(const string& str, const string& trimChars) { return TrimEx(str, trimChars, ""); }

string TrimRight(const string& str, const string& trimChars) { return TrimEx(str, "", trimChars); }

// SyncEvent class implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

SyncEvent::SyncEvent(bool initialState, bool autoReset)
{
    m_signaled = initialState;
    m_autoReset = autoReset;
}

bool SyncEvent::SetEvent()
{
    lock_guard<mutex> lock(m_mtx);
    bool wasSignaled = m_signaled;
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
    lock_guard<mutex> lock(m_mtx);
    bool wasSignaled = m_signaled;
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
    bool success = m_cv.wait_for(lock, chrono::milliseconds(milliseconds), [this]() { return m_signaled; });
    if (success && m_autoReset)
    {
        m_signaled = false;  // Reset the event if auto-reset is enabled
    }
    return success;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////