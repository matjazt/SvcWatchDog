#ifndef _CRYPTOTOOLS_H_
#define _CRYPTOTOOLS_H_

#include <JsonConfig/JsonConfig.h>

// NOTE: we're deliberately keeping the API independent of the actual crypto library being used. There
// should be no mention of Botan in the API. This requirement does however complicate the implementation
// a bit, for example default key and IV can't be stored as Botan's secure_vectors.

class CryptoTools : public NoCopy
{
   public:
    CryptoTools();
    ~CryptoTools();

    static CryptoTools* GetInstance();
    static void SetInstance(CryptoTools* instance);

    void Configure(JsonConfig& cfg, const string& section, const string& defaultPassword);

    string Aes256CbcEncrypt(const string& plainText);
    string Aes256CbcDecrypt(const string& base64CipherText);

    void SelfTest();

   private:
    string m_defaultPassword;
    vector<uint8_t> m_defaultKeyAndIv;

    static CryptoTools* m_instance;
};

#endif
