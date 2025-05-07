/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <SimpleTools/SimpleTools.h>
#include <fstream>

string loadTextFile(const filesystem::path& filePath)
{
    // Check if the file exists and is a regular file
    if (!filesystem::exists(filePath) || !filesystem::is_regular_file(filePath))
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

#ifdef WIN32

_CrtMemState g_memChkPoint1;
_CrtMemState g_memChkPoint2;
_CrtMemState g_memDiff;

#endif

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
vector<string> split(const string& str, char delimiter)
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
