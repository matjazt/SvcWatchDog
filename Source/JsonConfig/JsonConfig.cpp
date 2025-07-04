/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>

#include <JsonConfig/JsonConfig.h>

JsonConfig* JsonConfig::m_instance = nullptr;

JsonConfig::JsonConfig() {}

JsonConfig::~JsonConfig() {}

JsonConfig* JsonConfig::GetInstance() { return m_instance; }
void JsonConfig::SetInstance(JsonConfig* instance) { m_instance = instance; }

void JsonConfig::Load(const filesystem::path& filePath)
{
    string jsonText = LoadTextFile(filePath);
    try
    {
        m_json = json::parse(jsonText);
    }
    catch (...)
    {
        // show the file contents
        cerr << "JSON file:" << endl << jsonText << endl;
        throw;
    }
}

json* JsonConfig::GetJson() { return &m_json; }

string JsonConfig::GetString(const string& section, const string& key, const string& defaultValue)
{
    lock_guard<mutex> lock(m_cs);
    if (m_json.contains(section) && m_json[section].contains(key))
    {
        return m_json[section][key].get<string>();
    }
    else
    {
        return defaultValue;
    }
}

int JsonConfig::GetString(const string& section, const string& key, char* buffer, size_t bufferSize, const string& defaultValue)
{
    string v = GetString(section, key, defaultValue);
    strncpy(buffer, &v[0], bufferSize - 1);
    buffer[bufferSize - 1] = 0;
    return (int)strlen(buffer);
}

template <typename T>
T JsonConfig::GetNumber(const string& section, const string& key, T defaultValue)
{
    lock_guard<mutex> lock(m_cs);

    try
    {
        if (!m_json.contains(section) || !m_json[section].contains(key))
        {
            // key not present, so we should stop trying immediately
            return defaultValue;
        }

        // try to read it as a number
        return m_json[section][key].get<T>();
    }
    catch (...)
    {
    }

    // so the key is present, but it is not a number
    try
    {
        string s = m_json[section][key].get<string>();
        T value;
        char unparsed;
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        {
            // we probably have a hex number
            uint64_t hex;
            if (sscanf(s.c_str(), "%llx%c", &hex, &unparsed) == 1)
            {
                value = static_cast<T>(hex);
                // we're deliberately ignoring the overflow check here (if (static_cast<uint64_t>(value) == hex))
                return value;
            }
        }
        else
        {
            // attempt to parse the string into the target numeric type
            std::istringstream iss(s);
            iss >> value;

            // check for errors (e.g., invalid characters or partial parsing)
            if (!iss.fail() && iss.eof())
            {
                return value;
            }
        }
    }
    catch (...)
    {
    }

    return defaultValue;
}

bool JsonConfig::GetBool(const string& section, const string& key, bool defaultValue)
{
    lock_guard<mutex> lock(m_cs);
    if (m_json.contains(section) && m_json[section].contains(key))
    {
        return m_json[section][key].get<bool>();
    }
    else
    {
        return defaultValue;
    }
}

// Explicit instantiation for specific types
template int8_t JsonConfig::GetNumber(const string& section, const string& key, int8_t defaultValue);
template uint8_t JsonConfig::GetNumber(const string& section, const string& key, uint8_t defaultValue);

template int16_t JsonConfig::GetNumber(const string& section, const string& key, int16_t defaultValue);
template uint16_t JsonConfig::GetNumber(const string& section, const string& key, uint16_t defaultValue);

template int32_t JsonConfig::GetNumber(const string& section, const string& key, int32_t defaultValue);
template uint32_t JsonConfig::GetNumber(const string& section, const string& key, uint32_t defaultValue);

template int64_t JsonConfig::GetNumber(const string& section, const string& key, int64_t defaultValue);
template uint64_t JsonConfig::GetNumber(const string& section, const string& key, uint64_t defaultValue);

template double JsonConfig::GetNumber(const string& section, const string& key, double defaultValue);
template float JsonConfig::GetNumber(const string& section, const string& key, float defaultValue);
