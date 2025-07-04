#ifndef _CRYPTOTOOLS_H_
#define _CRYPTOTOOLS_H_

#include <string>

std::string Aes256CbcEncrypt(const std::string& plainText, const std::string& password = "default");
std::string Aes256CbcDecrypt(const std::string& base64CipherText, const std::string& password = "default");

void CryptoToolsTest();

#endif
