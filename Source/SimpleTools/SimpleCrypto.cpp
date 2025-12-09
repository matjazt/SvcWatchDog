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

#include <SimpleTools/SimpleCrypto.h>
#include <PicoSHA2/picosha2.h>

std::vector<uint8_t> HmacSha256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& message)
{
    // HMAC-SHA256 constants
    constexpr size_t BLOCK_SIZE = 64;  // SHA-256 block size in bytes
    constexpr size_t HASH_SIZE = 32;   // SHA-256 hash size in bytes
    constexpr uint8_t IPAD = 0x36;     // Inner padding value
    constexpr uint8_t OPAD = 0x5C;     // Outer padding value

    // Step 1: Prepare the key
    std::vector<uint8_t> processedKey;

    if (key.size() > BLOCK_SIZE)
    {
        // If key is longer than block size, hash it first
        processedKey.resize(HASH_SIZE);
        picosha2::hash256(key, processedKey);
    }
    else
    {
        // Use key as-is if it's within block size
        processedKey = key;
    }

    // Pad the key to block size with zeros
    processedKey.resize(BLOCK_SIZE, 0);

    // Step 2: Create inner and outer padded keys
    std::vector<uint8_t> innerKey(BLOCK_SIZE);
    std::vector<uint8_t> outerKey(BLOCK_SIZE);

    for (size_t i = 0; i < BLOCK_SIZE; ++i)
    {
        innerKey[i] = processedKey[i] ^ IPAD;
        outerKey[i] = processedKey[i] ^ OPAD;
    }

    // Step 3: Compute inner hash: H((K ⊕ ipad) || message)
    std::vector<uint8_t> innerInput;
    innerInput.reserve(BLOCK_SIZE + message.size());
    innerInput.insert(innerInput.end(), innerKey.begin(), innerKey.end());
    innerInput.insert(innerInput.end(), message.begin(), message.end());

    std::vector<uint8_t> innerHash(HASH_SIZE);
    picosha2::hash256(innerInput, innerHash);

    // Step 4: Compute outer hash: H((K ⊕ opad) || innerHash)
    std::vector<uint8_t> outerInput;
    outerInput.reserve(BLOCK_SIZE + HASH_SIZE);
    outerInput.insert(outerInput.end(), outerKey.begin(), outerKey.end());
    outerInput.insert(outerInput.end(), innerHash.begin(), innerHash.end());

    std::vector<uint8_t> hmacResult(HASH_SIZE);
    picosha2::hash256(outerInput, hmacResult);

    return hmacResult;
}