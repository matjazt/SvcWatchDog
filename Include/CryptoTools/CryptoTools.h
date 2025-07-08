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

class CryptoTools : public NoCopy
{
   public:
    CryptoTools() noexcept;
    ~CryptoTools();

    static CryptoTools* GetInstance() noexcept;
    static void SetInstance(CryptoTools* instance) noexcept;

    void Configure(JsonConfig& cfg, const string& section, const string& defaultPassword);

    string Aes256CbcEncrypt(const string& plainText);
    string Aes256CbcDecrypt(const string& base64CipherText);

    string GetPossiblyEncryptedConfigurationString(JsonConfig& cfg, const string& section, const string& key,
                                                   const string& defaultValue = "");

    // void SelfTest();

   private:
    unique_ptr<Botan::Cipher_Mode> m_encryptor;
    unique_ptr<Botan::Cipher_Mode> m_decryptor;

    Botan::secure_vector<uint8_t> m_iv;

    static CryptoTools* m_instance;
};

#define Crypto (*CryptoTools::GetInstance())

#endif
