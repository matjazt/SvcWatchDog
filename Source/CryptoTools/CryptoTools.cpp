
#include <botan/auto_rng.h>
#include <botan/cipher_mode.h>
// #include <botan/hex.h>
#include <botan/base64.h>
// #include <botan/pbkdf2.h>
// #include <botan/sh _64.h>
#include <botan/pwdhash.h>
// #include <botan/symkey.h>
#include <memory>

#include <CryptoTools/CryptoTools.h>
#include <Logger/Logger.h>
#include <iostream>

using namespace std;

CryptoTools* CryptoTools::m_instance = nullptr;

CryptoTools::CryptoTools() {}

CryptoTools::~CryptoTools() {}

CryptoTools* CryptoTools::GetInstance() { return m_instance; }

void CryptoTools::SetInstance(CryptoTools* instance) { m_instance = instance; }

void CryptoTools::Configure(JsonConfig& cfg, const string& section, const string& defaultPassword)
{
    m_defaultPassword.clear();
    m_defaultKeyAndIv.clear();

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
                    m_defaultPassword.push_back(c);
                }
            }
        }
        catch (...)
        {
            LOGSTR(Error) << "unable to load default password from " << passwordFile;
        }

        // this one is out of the cash
        if (m_defaultPassword.length() < 12)
        {
            LOGSTR(Error) << "password file " << passwordFile << " is too short, at least 12 characters are required";
            m_defaultPassword.clear();
        }
    }

    if (m_defaultPassword.empty())
    {
        m_defaultPassword = defaultPassword;
    }

    // use empty salt
    const vector<uint8_t> salt = {};
    auto pbkdf = Botan::PasswordHashFamily::create_or_throw("PBKDF2(SHA-256)")->from_iterations(10000);
    m_defaultKeyAndIv.resize(48);
    pbkdf->hash(m_defaultKeyAndIv, m_defaultPassword, salt);
}

#define PREPARE_KEY_AND_IV()                                                                      \
    Botan::secure_vector<uint8_t> key(m_defaultKeyAndIv.begin(), m_defaultKeyAndIv.begin() + 32); \
    Botan::secure_vector<uint8_t> iv(m_defaultKeyAndIv.begin() + 32, m_defaultKeyAndIv.end());

// Returns the AES-256-CBC encrypted string of the given plain text using the specified password or the default password.
// OpenSSL equivalent: openssl enc -base64 -e -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword
// (you can ommit -pass and enter it interactively).
string CryptoTools::Aes256CbcEncrypt(const string& plainText)
{
    using namespace Botan;

    PREPARE_KEY_AND_IV();

    // secure_vector<uint8_t> key(key_iv.begin(), key_iv.begin() + 32);
    // secure_vector<uint8_t> iv(key_iv.begin() + 32, key_iv.end());

    // Create AES-256 CBC encryptor with PKCS7 padding
    auto enc = Cipher_Mode::create("AES-256/CBC/PKCS7", Cipher_Dir::Encryption);
    enc->set_key(key);
    enc->start(iv);

    // convert plain text to a buffer, then encrypt it in place
    secure_vector<uint8_t> buffer(plainText.begin(), plainText.end());
    enc->finish(buffer);

    // Return Base64-encoded ciphertext
    return base64_encode(buffer);
}

// Returns the plain text of the given AES-256-CBC encrypted string using the specified password or the default password.
// OpenSSL equivalent: openssl enc -base64 -d -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword
// (you can ommit -pass and enter it interactively).
string CryptoTools::Aes256CbcDecrypt(const string& base64CipherText)
{
    using namespace Botan;

    PREPARE_KEY_AND_IV();

    // Create AES-256 CBC decryptor with PKCS7 padding
    auto dec = Cipher_Mode::create("AES-256/CBC/PKCS7", Cipher_Dir::Decryption);
    dec->set_key(key);
    dec->start(iv);

    auto buffer = base64_decode(base64CipherText);

    // Decrypt in place
    dec->finish(buffer);

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