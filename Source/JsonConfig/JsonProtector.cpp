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

#include <JsonConfig/JsonProtector.h>
#include <SimpleTools/SimpleCrypto.h>
#include <SimpleTools/SimpleTools.h>
#include <sstream>
#include <iomanip>

namespace
{

/**
 * @brief Helper function to get a nested JSON section using dot-separated path.
 * @param data The JSON data to search in
 * @param path Dot-separated path (e.g., "simulatorCore.stateVariables")
 * @return Reference to the found JSON section
 * @throws std::runtime_error if the path is not found
 */
const json& GetNestedSection(const json& data, const std::string& path)
{
    const json* current = &data;
    std::stringstream ss(path);
    std::string part;

    while (std::getline(ss, part, '.'))
    {
        if (!current->is_object() || !current->contains(part))
        {
            throw std::runtime_error("Section path '" + path + "' not found in configuration");
        }
        current = &(*current)[part];
    }

    return *current;
}

/**
 * @brief Compute HMAC-SHA256 hash of JSON data serialized with sorted keys.
 * @param data JSON data to hash
 * @param password Password for HMAC computation
 * @return Hexadecimal string representation of the hash
 */
std::string ComputeJsonHash(const json& data, const std::string& password)
{
    // Serialize JSON with sorted keys and compact format (no spaces)
    auto serialized = data.dump(-1, ' ', false, json::error_handler_t::strict);

    // Convert string to bytes for HMAC computation
    const std::vector<uint8_t> message(serialized.begin(), serialized.end());
    const std::vector<uint8_t> key(password.begin(), password.end());

    // Compute HMAC-SHA256
    const auto hash = HmacSha256(key, message);

    return BytesToHexString(hash);
}

}  // anonymous namespace

void ProtectJson(json* data, const std::string& password)
{
    if (data == nullptr)
    {
        throw std::invalid_argument("JSON data pointer cannot be null");
    }

    if (!data->contains("protectedSections"))
    {
        throw std::invalid_argument("JSON data must contain 'protectedSections' array");
    }

    json& protectedSections = (*data)["protectedSections"];

    if (!protectedSections.is_array())
    {
        throw std::invalid_argument("'protectedSections' must be an array");
    }

    // Process each protected section
    for (auto& section : protectedSections)
    {
        if (!section.is_object() || !section.contains("sectionName"))
        {
            throw std::invalid_argument("Each protected section must be an object with 'sectionName' field");
        }

        const std::string sectionName = section["sectionName"];

        try
        {
            // Get the section data using the dot-separated path
            const json& sectionData = GetNestedSection(*data, sectionName);

            // Compute HMAC-SHA256 hash of the serialized section
            const auto hash = ComputeJsonHash(sectionData, password);

            // Store the hash in the protected section entry
            section["hash"] = hash;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Failed to process protected section '" + sectionName + "': " + e.what());
        }
    }

    // Compute hash of the entire protectedSections array
    const auto protectedSectionsHash = ComputeJsonHash(protectedSections, password);
    (*data)["protectedSectionsHash"] = protectedSectionsHash;
}

void VerifyJsonProtection(const json* data, const std::string& password)
{
    if (data == nullptr)
    {
        throw std::invalid_argument("JSON data pointer cannot be null");
    }

    if (!data->contains("protectedSections") || !data->contains("protectedSectionsHash"))
    {
        throw std::invalid_argument("JSON data must contain 'protectedSections' array and 'protectedSectionsHash' field");
    }

    const json& protectedSections = (*data)["protectedSections"];

    if (!protectedSections.is_array())
    {
        throw std::invalid_argument("'protectedSections' must be an array");
    }

    // First, verify the hash of the protectedSections array itself
    // If this is compromised, there's no point checking individual sections
    const std::string storedProtectedSectionsHash = (*data)["protectedSectionsHash"];
    const auto computedProtectedSectionsHash = ComputeJsonHash(protectedSections, password);

    if (storedProtectedSectionsHash != computedProtectedSectionsHash)
    {
        throw std::runtime_error("protectedSectionsHash verification failed - protected sections array has been tampered with");
    }

    // Verify each protected section
    for (const auto& section : protectedSections)
    {
        if (!section.is_object() || !section.contains("sectionName") || !section.contains("hash"))
        {
            throw std::invalid_argument("Each protected section must be an object with 'sectionName' and 'hash' fields");
        }

        const std::string sectionName = section["sectionName"];
        const std::string storedHash = section["hash"];

        try
        {
            // Get the section data and compute its hash
            const auto& sectionData = GetNestedSection(*data, sectionName);
            const auto computedHash = ComputeJsonHash(sectionData, password);

            // Compare hashes
            if (storedHash != computedHash)
            {
                throw std::runtime_error("Hash verification failed for protected section '" + sectionName + "'");
            }
        }
        catch (const std::runtime_error& e)
        {
            // Re-throw runtime errors with section context
            throw std::runtime_error("Failed to verify protected section '" + sectionName + "': " + e.what());
        }
    }
}