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

#ifndef _CRYPTOTOOLS_H_
#define _CRYPTOTOOLS_H_

#include <botan/cipher_mode.h>

#include <JsonConfig/JsonConfig.h>

class CryptoTools
{
   public:
    CryptoTools() noexcept;
    ~CryptoTools();

    // prevent copying and assignment
    DELETE_COPY_AND_ASSIGNMENT(CryptoTools);

    static CryptoTools* GetInstance() noexcept;
    static void SetInstance(CryptoTools* instance) noexcept;

    void Configure(JsonConfig& cfg, const std::string& section, const std::string& defaultPassword);

    std::string Aes256CbcEncrypt(const std::string& plainText);
    std::string Aes256CbcDecrypt(const std::string& base64CipherText);

    std::string GetPossiblyEncryptedConfigurationString(JsonConfig& cfg, const std::string& section, const std::string& key,
                                                        const std::string& defaultValue = "");

    // void SelfTest();

   private:
    std::unique_ptr<Botan::Cipher_Mode> m_encryptor;
    std::unique_ptr<Botan::Cipher_Mode> m_decryptor;

    Botan::secure_vector<uint8_t> m_iv;

    static CryptoTools* m_instance;
};

#define Crypto (*CryptoTools::GetInstance())

#endif
