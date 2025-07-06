/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
// #include <botan/hex.h>
#include <botan/base64.h>
// #include <botan/pbkdf2.h>
// #include <botan/sh _64.h>
#include <botan/pwdhash.h>
// #include <botan/symkey.h>
#include <memory>
#include <iostream>

#include <CryptoTools/CryptoTools.h>
#include <Logger/Logger.h>

using namespace Botan;

CryptoTools* CryptoTools::m_instance = nullptr;

CryptoTools::CryptoTools() {}

CryptoTools::~CryptoTools() {}

CryptoTools* CryptoTools::GetInstance() { return m_instance; }

void CryptoTools::SetInstance(CryptoTools* instance) { m_instance = instance; }

void CryptoTools::Configure(JsonConfig& cfg, const string& section, const string& defaultPassword)
{
    string password;

    string passwordFile = section.empty() ? "" : cfg.GetString(section, "passwordFile", "");

    if (!passwordFile.empty())
    {
        try
        {
            string passwordData = LoadTextFile(passwordFile);
            // Take all regular ascii characters from the file. Ignore non-ascii ones to avoid issues, caused by invisible characters
            // (such as \n and \r) and various spaces (regular space, tab...).
            for (auto c : passwordData)
            {
                if (c > ' ')
                {
                    password.push_back(c);
                }
            }
        }
        catch (...)
        {
            LOGSTR(Error) << "unable to load default password from " << passwordFile;
        }

        // this one is out of the cash
        if (password.length() < 12)
        {
            LOGSTR(Error) << "password file " << passwordFile << " is too short, at least 12 characters are required";
            password.clear();
        }
    }

    if (password.empty())
    {
        password = defaultPassword;
    }

    // use empty salt
    const vector<uint8_t> salt = {};
    auto pbkdf = Botan::PasswordHashFamily::create_or_throw("PBKDF2(SHA-256)")->from_iterations(10000);
    Botan::secure_vector<uint8_t> key_iv(48);
    pbkdf->hash(key_iv, password, salt);

    Botan::secure_vector<uint8_t> key(key_iv.begin(), key_iv.begin() + 32);
    m_iv.assign(key_iv.begin() + 32, key_iv.end());

    m_encryptor = Botan::Cipher_Mode::create("AES-256/CBC/PKCS7", Cipher_Dir::Encryption);
    m_encryptor->set_key(key);

    m_decryptor = Botan::Cipher_Mode::create("AES-256/CBC/PKCS7", Cipher_Dir::Decryption);
    m_decryptor->set_key(key);
}

// Returns the AES-256-CBC encrypted string of the given plain text using the specified password or the default password.
// OpenSSL equivalent: openssl enc -base64 -e -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword
// (you can ommit -pass and enter it interactively).
string CryptoTools::Aes256CbcEncrypt(const string& plainText)
{
    m_encryptor->start(m_iv);

    // convert plain text to a buffer, then encrypt it in place
    secure_vector<uint8_t> buffer(plainText.begin(), plainText.end());
    m_encryptor->finish(buffer);

    // Return Base64-encoded ciphertext
    return base64_encode(buffer);
}

// Returns the plain text of the given AES-256-CBC encrypted string using the specified password or the default password.
// OpenSSL equivalent: openssl enc -base64 -d -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword
// (you can ommit -pass and enter it interactively).
string CryptoTools::Aes256CbcDecrypt(const string& base64CipherText)
{
    m_decryptor->start(m_iv);

    auto buffer = base64_decode(base64CipherText);

    // Decrypt in place
    m_decryptor->finish(buffer);

    // Return Base64-encoded ciphertext
    return string(buffer.begin(), buffer.end());
}

void CryptoTools::SelfTest()
{
    string plain = "Hahaha";
    auto encrypted = Aes256CbcEncrypt(plain);
    cout << "encrypted " << plain << " to " << encrypted << "\n";

    auto decrypted = Aes256CbcDecrypt(encrypted);
    cout << "decrypted " << encrypted << " to " << decrypted << "\n";
}

/*
Botan::secure_vector<uint8_t> DeriveKeyAndIv_Pbkdf2_Sha256(const string& password)
{
    using namespace Botan;

    // use empty salt
    const vector<uint8_t> salt = {};  // Empty salt like in your C# example

    auto pbkdf = Botan::PasswordHashFamily::create_or_throw("PBKDF2(SHA-256)")->from_iterations(10000);

    Botan::secure_vector<uint8_t> key_iv(48);
    pbkdf->hash(key_iv, password, salt);

    return key_iv;
}
*/