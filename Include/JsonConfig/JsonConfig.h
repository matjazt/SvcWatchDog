/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef JSONCONFIG_H
#define JSONCONFIG_H

#include <SimpleTools/SimpleTools.h>
// #undef snprintf
#include <nlohmann/json.hpp>
#include <mutex>

using json = nlohmann::json;

class JsonConfig : public NoCopy
{
   public:
    JsonConfig();
    ~JsonConfig();

    static JsonConfig* GetInstance();
    static void SetInstance(JsonConfig* instance);

    void Load(const filesystem::path& filePath);
    json* GetJson();

    string GetString(const string& section, const string& key, const string& defaultValue = "");
    int GetString(const string& section, const string& key, char* buffer, size_t bufferSize, const string& defaultValue = "");

    template <typename T>
    T GetNumber(const string& section, const string& key, T defaultValue);
    bool GetBool(const string& section, const string& key, bool defaultValue);

   private:
    static JsonConfig* m_instance;

    mutex m_cs;
    json m_json;
};

#define Cfg (*JsonConfig::GetInstance())

#endif
