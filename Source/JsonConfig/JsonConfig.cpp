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

#include <inttypes.h>

#include <JsonConfig/JsonConfig.h>

JsonConfig* JsonConfig::m_instance = nullptr;

JsonConfig::JsonConfig() noexcept {}

JsonConfig::~JsonConfig() {}

JsonConfig* JsonConfig::GetInstance() noexcept { return m_instance; }
void JsonConfig::SetInstance(JsonConfig* instance) noexcept { m_instance = instance; }

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

json* JsonConfig::GetJson(const string& path) { return path.empty() ? &m_json : FindKey(path, ""); }

json* JsonConfig::FindKey(const string& path, const string& key)
{
    auto tokens = Split(path, '.');
    if (!key.empty())
    {
        tokens.push_back(key);
    }

    json* current = &m_json;
    for (const auto& token : tokens)
    {
        if (current->contains(token))
        {
            current = &(*current)[token];
        }
        else
        {
            return nullptr;
        }
    }

    return current;
}

template <typename T>
T JsonConfig::GetParameter(const string& path, const string& key, T defaultValue)
{
    try
    {
        const auto* parameter = FindKey(path, key);
        return parameter ? parameter->get<T>() : defaultValue;
    }
    catch (...)
    {
        return defaultValue;
    }
}

string JsonConfig::GetString(const string& path, const string& key, const string& defaultValue)
{
    return GetParameter(path, key, defaultValue);
}

int JsonConfig::GetString(const string& path, const string& key, char* buffer, size_t bufferSize, const string& defaultValue)
{
    string v = GetParameter(path, key, defaultValue);
    strncpy(buffer, &v[0], bufferSize - 1);
    buffer[bufferSize - 1] = 0;
    return (int)strlen(buffer);
}

template <typename T>
T JsonConfig::GetNumber(const string& path, const string& key, T defaultValue)
{
    const auto* parameter = FindKey(path, key);
    if (!parameter)
    {
        // key not present, so we should stop trying immediately
        return defaultValue;
    }

    try
    {
        // try to read it as a number
        return parameter->get<T>();
    }
    catch (...)
    {
    }

    // so the key is present, but it is not a number
    try
    {
        string s = parameter->get<string>();
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        {
            // we probably have a hex number
            uint64_t hex;
            char unparsed;
            if (sscanf(s.c_str(), "%" PRIX64 " %c", &hex, &unparsed) == 1)
            {
                // we're deliberately ignoring the overflow check here (if (static_cast<uint64_t>(value) == hex))
                return static_cast<T>(hex);
            }
        }
        else
        {
            // attempt to parse the string into the target numeric type
            std::istringstream iss(s);
            T value;
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

bool JsonConfig::GetBool(const string& path, const string& key, bool defaultValue) { return GetParameter(path, key, defaultValue); }

vector<string> JsonConfig::GetStringVector(const string& path, const string& key, vector<string> defaultValue)
{
    return GetParameter(path, key, defaultValue);
}

vector<string> JsonConfig::GetKeys(const string& path, bool includeObjects = true, bool includeArrays = true, bool includeOthers = true)
{
    vector<string> keys;
    auto* section = FindKey(path, "");
    if (section)
    {
        for (auto& item : section->items())
        {
            bool includeItem;
            if (item.value().is_object())
            {
                includeItem = includeObjects;
            }
            else if (item.value().is_array())
            {
                includeItem = includeArrays;
            }
            else
            {
                includeItem = includeOthers;
            }

            if (includeItem)
            {
                keys.push_back(item.key());
            }
        }
    }
    return keys;
}

// Explicit instantiation for specific types
template int8_t JsonConfig::GetNumber(const string& path, const string& key, int8_t defaultValue);
template uint8_t JsonConfig::GetNumber(const string& path, const string& key, uint8_t defaultValue);

template int16_t JsonConfig::GetNumber(const string& path, const string& key, int16_t defaultValue);
template uint16_t JsonConfig::GetNumber(const string& path, const string& key, uint16_t defaultValue);

template int32_t JsonConfig::GetNumber(const string& path, const string& key, int32_t defaultValue);
template uint32_t JsonConfig::GetNumber(const string& path, const string& key, uint32_t defaultValue);

template int64_t JsonConfig::GetNumber(const string& path, const string& key, int64_t defaultValue);
template uint64_t JsonConfig::GetNumber(const string& path, const string& key, uint64_t defaultValue);

template double JsonConfig::GetNumber(const string& path, const string& key, double defaultValue);
template float JsonConfig::GetNumber(const string& path, const string& key, float defaultValue);
