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

    void Load(const std::filesystem::path& filePath);
    json* GetJson(const std::string& path = "");

    std::string GetString(const std::string& path, const std::string& key, const std::string& defaultValue = "");
    int GetString(const std::string& path, const std::string& key, char* buffer, size_t bufferSize, const std::string& defaultValue = "");

    template <typename T>
    T GetNumber(const std::string& path, const std::string& key, T defaultValue);
    bool GetBool(const std::string& path, const std::string& key, bool defaultValue = false);
    std::vector<std::string> GetStringVector(const std::string& path, const std::string& key, std::vector<std::string> defaultValue = {});
    std::vector<std::string> GetKeys(const std::string& path, bool includeObjects, bool includeArrays, bool includeOthers);

   private:
    static JsonConfig* m_instance;

    json m_json;

    json* FindKey(const std::string& path, const std::string& key);
    template <typename T>
    T GetParameter(const std::string& path, const std::string& key, T defaultValue);
};

#define Cfg (*JsonConfig::GetInstance())

#endif
