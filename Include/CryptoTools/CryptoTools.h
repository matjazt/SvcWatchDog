/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
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
