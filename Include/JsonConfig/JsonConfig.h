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
class JsonConfig
{
   public:
    JsonConfig() noexcept;
    ~JsonConfig();

    DELETE_COPY_AND_ASSIGNMENT(JsonConfig);

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

    template <typename T>
    T ParseSection(const std::string& section)
    {
        // configure the plugin
        json* sectionData = GetJson(section);
        if (!sectionData)
        {
            throw std::runtime_error("configuration section '" + section + "' not found");
        }

        try
        {
            return sectionData->template get<T>();
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Failed to parse configuration section '" + section + "': " + e.what());
        }
    }

   private:
    static JsonConfig* m_instance;

    json m_json;

    json* FindKey(const std::string& path, const std::string& key);
    template <typename T>
    T GetParameter(const std::string& path, const std::string& key, T defaultValue);
};

#define Cfg (*JsonConfig::GetInstance())

#endif
