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

#ifndef _SIMPLECRYPTO_H_
#define _SIMPLECRYPTO_H_

#include <vector>
#include <cstdint>

/**
 * @brief Computes HMAC-SHA256 hash of a message using the specified key.
 *
 * This function implements the HMAC (Hash-based Message Authentication Code)
 * algorithm using SHA-256 as the underlying hash function. HMAC provides both
 * data integrity and authentication by combining a secret key with the message.
 *
 * The implementation follows RFC 2104 specification for HMAC construction:
 * HMAC(K, m) = H((K ⊕ opad) || H((K ⊕ ipad) || m))
 * where:
 * - K is the secret key
 * - m is the message
 * - H is the hash function (SHA-256)
 * - opad is the outer padding (0x5C repeated)
 * - ipad is the inner padding (0x36 repeated)
 * - || denotes concatenation
 * - ⊕ denotes XOR operation
 *
 * @param key The secret key used for authentication. Can be of any length.
 *            If longer than the hash block size (64 bytes for SHA-256),
 *            it will be hashed first.
 * @param message The message to be authenticated and hashed.
 * @return std::vector<uint8_t> The HMAC-SHA256 digest (32 bytes).
 *
 * @note This function uses the PicoSHA2 library for SHA-256 computation.
 * @note The resulting HMAC provides cryptographic authentication and can be
 *       used to verify both the integrity and authenticity of the message.
 */
std::vector<uint8_t> HmacSha256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& message);

#endif  // _SIMPLECRYPTO_H_