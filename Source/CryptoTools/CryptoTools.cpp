
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
#include <iostream>

using namespace std;

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

// Returns the AES-256-CBC encrypted string of the given plain text using the specified password or the default password.
// OpenSSL equivalent: openssl enc -base64 -e -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword
// (you can ommit -pass and enter it interactively).
string Aes256CbcEncrypt(const string& plainText, const string& password)
{
    using namespace Botan;

    auto key_iv = DeriveKeyAndIv_Pbkdf2_Sha256(password);

    secure_vector<uint8_t> key(key_iv.begin(), key_iv.begin() + 32);
    secure_vector<uint8_t> iv(key_iv.begin() + 32, key_iv.end());

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
string Aes256CbcDecrypt(const string& base64CipherText, const string& password)
{
    using namespace Botan;

    auto key_iv = DeriveKeyAndIv_Pbkdf2_Sha256(password);

    secure_vector<uint8_t> key(key_iv.begin(), key_iv.begin() + 32);
    secure_vector<uint8_t> iv(key_iv.begin() + 32, key_iv.end());

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

void CryptoToolsTest()
{
    string plain = "Hahaha";
    string password = "SuperSecretPassword";
    auto encrypted = Aes256CbcEncrypt(plain, password);
    cout << "encrypted " << plain << " to " << encrypted << "\n";

    auto decrypted = Aes256CbcDecrypt(encrypted, password);
    cout << "decrypted " << encrypted << " to " << decrypted << "\n";
}
