/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef _JSONCONFIG_H_
#define _JSONCONFIG_H_

#include <SimpleTools/SimpleTools.h>
// #undef snprintf
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief JsonConfig is a lightweight wrapper around the nlohmann::json library, designed
 *        to simplify the use of JSON files as configuration sources.
 *
 * It provides intuitive methods for retrieving values with optional defaults, ensuring that your code works even when certain
 * parameters are omitted from the JSON file. This approach reduces the need to explicitly define every single setting in your configuration
 * object and the JSON file, letting you fallback to sensible defaults where needed. It's particularly handy when all you require is the
 * flexibility to adjust some "constants" later without overengineering the config structure. JsonConfig is built with resilience in mind:
 * its getter functions never throw exceptions. Instead, they quietly return the provided default value if a key is missing or any parsing
 * error occurs. This makes it ideal for relaxed, fault-tolerant configuration scenarios. If your application demands stricter validation
 * and error reporting, JsonConfig may not be the right tool.
 */
class JsonConfig : public NoCopy
{
   public:
    JsonConfig() noexcept;
    ~JsonConfig();

    static JsonConfig* GetInstance() noexcept;
    static void SetInstance(JsonConfig* instance) noexcept;

    void Load(const filesystem::path& filePath);
    json* GetJson(const string& path = "");

    string GetString(const string& path, const string& key, const string& defaultValue = "");
    int GetString(const string& path, const string& key, char* buffer, size_t bufferSize, const string& defaultValue = "");

    template <typename T>
    T GetNumber(const string& path, const string& key, T defaultValue);
    bool GetBool(const string& path, const string& key, bool defaultValue = false);
    vector<string> GetStringVector(const string& path, const string& key, vector<string> defaultValue = {});
    vector<string> GetKeys(const string& path, bool includeObjects, bool includeArrays, bool includeOthers);

   private:
    static JsonConfig* m_instance;

    json m_json;

    json* FindKey(const string& path, const string& key);
    template <typename T>
    T GetParameter(const string& path, const string& key, T defaultValue);
};

#define Cfg (*JsonConfig::GetInstance())

#endif
