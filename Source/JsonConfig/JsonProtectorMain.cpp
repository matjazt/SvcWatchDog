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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <JsonConfig/JsonProtector.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

void PrintUsage(const char* programName)
{
    std::cout << "JSON Protector - Cryptographic protection tool for JSON configuration files\n\n";
    std::cout << "Usage: " << programName << " <source_file> <target_file> <password>\n\n";
    std::cout << "Parameters:\n";
    std::cout << "  source_file  Path to the input JSON file to be protected\n";
    std::cout << "  target_file  Path to the output JSON file with computed hashes\n";
    std::cout << "  password     Secret password used for HMAC-SHA256 hash computation\n\n";
    std::cout << "Description:\n";
    std::cout << "  This tool reads a JSON configuration file containing a 'protectedSections'\n";
    std::cout << "  array and computes HMAC-SHA256 hashes for each specified section. The\n";
    std::cout << "  protected data is written to the target file with user-friendly indented\n";
    std::cout << "  formatting. The parameter and section order from the input file is preserved.\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << programName << " config.json protected_config.json mySecretKey123\n\n";
}

int main(int argc, char* argv[])
{
    // Check command line arguments
    if (argc != 4)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string sourceFile = argv[1];
    std::string targetFile = argv[2];
    std::string password = argv[3];

    try
    {
        // Validate source file exists
        if (!std::filesystem::exists(sourceFile))
        {
            std::cerr << "Error: Source file '" << sourceFile << "' does not exist.\n";
            return 2;
        }

        // Load source JSON file preserving order
        std::ifstream inputFile(sourceFile);
        if (!inputFile.is_open())
        {
            std::cerr << "Error: Cannot open source file '" << sourceFile << "' for reading.\n";
            return 3;
        }

        ordered_json orderedConfigData;
        json configData;
        try
        {
            // Parse with preserved order for output
            inputFile >> orderedConfigData;
            inputFile.close();

            // Convert to regular json for protection computation (which requires sorted keys for consistent hashing)
            configData = orderedConfigData;
        }
        catch (const json::exception& e)
        {
            std::cerr << "Error: Failed to parse JSON from source file '" << sourceFile << "':\n";
            std::cerr << "  " << e.what() << "\n";
            return 4;
        }

        std::cout << "Successfully loaded JSON configuration from '" << sourceFile << "'\n";

        // Apply protection (this works on sorted json for consistent hashing)
        try
        {
            ProtectJson(&configData, password);
            std::cout << "Successfully computed protection hashes\n";
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Failed to protect JSON configuration:\n";
            std::cerr << "  " << e.what() << "\n";
            return 5;
        }

        // Update the ordered JSON with the computed hashes while preserving order
        try
        {
            // Copy the computed hashes to the ordered json
            if (configData.contains("protectedSections"))
            {
                orderedConfigData["protectedSections"] = configData["protectedSections"];
            }
            if (configData.contains("protectedSectionsHash"))
            {
                orderedConfigData["protectedSectionsHash"] = configData["protectedSectionsHash"];
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Failed to update ordered JSON with protection hashes:\n";
            std::cerr << "  " << e.what() << "\n";
            return 5;
        }

        // Create target directory if it doesn't exist
        std::filesystem::path targetPath(targetFile);
        if (targetPath.has_parent_path())
        {
            try
            {
                std::filesystem::create_directories(targetPath.parent_path());
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "Error: Cannot create target directory:\n";
                std::cerr << "  " << e.what() << "\n";
                return 6;
            }
        }

        // Write protected JSON to target file with indented formatting
        std::ofstream outputFile(targetFile);
        if (!outputFile.is_open())
        {
            std::cerr << "Error: Cannot open target file '" << targetFile << "' for writing.\n";
            return 7;
        }

        try
        {
            // Use indented formatting (1 tab) for user-friendly output with preserved order
            outputFile << orderedConfigData.dump(1, '\t') << std::endl;
            outputFile.close();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Failed to write protected JSON to target file:\n";
            std::cerr << "  " << e.what() << "\n";
            return 8;
        }

        std::cout << "Successfully wrote protected JSON configuration to '" << targetFile << "'\n";
        std::cout << "Protection completed successfully!\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 9;
    }
    catch (...)
    {
        std::cerr << "Unexpected unknown error occurred.\n";
        return 10;
    }
}