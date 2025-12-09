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

#ifndef _JSONPROTECTOR_H_
#define _JSONPROTECTOR_H_

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/**
 * @brief Protects JSON configuration data by computing and storing HMAC-SHA256 hashes.
 *
 * This function processes a JSON configuration object that contains a "protectedSections"
 * array. For each section specified in the array, it computes an HMAC-SHA256 hash of
 * the section's content and stores it in the corresponding "hash" field.
 *
 * The protection mechanism works as follows:
 * 1. Iterates through each entry in the "protectedSections" array
 * 2. For each entry, retrieves the section data using the "sectionName" (dot-separated path)
 * 3. Serializes the section data using nlohmann::json with sorted keys
 * 4. Computes HMAC-SHA256 of the serialized data using the provided password
 * 5. Stores the hash in the corresponding entry's "hash" field
 * 6. Finally, computes a hash of the entire "protectedSections" array and stores it
 *    in "protectedSectionsHash"
 *
 * Expected JSON structure:
 * ```json
 * {
 *   "protectedSections": [
 *     {
 *       "sectionName": "log",
 *       "hash": "computed_hash_here"
 *     },
 *     {
 *       "sectionName": "simulatorCore.stateVariables",
 *       "hash": "computed_hash_here"
 *     }
 *   ],
 *   "protectedSectionsHash": "computed_hash_of_protected_sections_array",
 *   "log": { ... },
 *   "simulatorCore": {
 *     "stateVariables": [ ... ]
 *   }
 * }
 * ```
 *
 * @param data Pointer to the JSON data to be protected. Modified in-place.
 * @param password The secret password used for HMAC computation.
 *
 * @throws std::invalid_argument If data is null or protectedSections is missing/invalid
 * @throws std::runtime_error If section paths are not found or HMAC computation fails
 */
void ProtectJson(json* data, const std::string& password);

/**
 * @brief Verifies the integrity of protected JSON configuration data.
 *
 * This function validates that all protected sections in the JSON configuration
 * have correct HMAC-SHA256 hashes, ensuring that the data has not been tampered with.
 *
 * The verification process:
 * 1. Checks that "protectedSections" and "protectedSectionsHash" exist
 * 2. For each entry in "protectedSections", retrieves the section data and
 *    computes its HMAC-SHA256 hash using the provided password
 * 3. Compares the computed hash with the stored hash in the "hash" field
 * 4. Computes the hash of the entire "protectedSections" array and compares
 *    it with "protectedSectionsHash"
 * 5. Returns true only if all hashes match correctly
 *
 * @param data Pointer to the JSON data to be verified.
 * @param password The secret password used for HMAC computation.
 *
 * @throws std::invalid_argument If data is null or required fields are missing/invalid
 * @throws std::runtime_error If verification fails or section paths are not found
 */
void VerifyJsonProtection(const json* data, const std::string& password);
#endif  // _JSONPROTECTOR_H_