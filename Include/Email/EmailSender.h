#ifndef _EMAILSENDER_H_
#define _EMAILSENDER_H_

#include <JsonConfig/JsonConfig.h>

class EmailSender : public NoCopy
{
   public:
    EmailSender() noexcept;
    ~EmailSender();

    static EmailSender* GetInstance() noexcept;
    static void SetInstance(EmailSender* instance) noexcept;

    void Configure(JsonConfig& cfg, const string& section);

    int SendSimpleEmail(const string& subject, const string& utf8body, const vector<string>& toAddresses, const string& sourceAddress = "",
                        int timeout = 0);

   private:
    static EmailSender* m_instance;

    string m_smtpServerUrl;
    int m_sslFlag;  // see CURLOPT_USE_SSL option in libcurl
    string m_username;
    string m_password;
    string m_defaultSourceAddress;
    int m_timeout;  // in milliseconds
};

#endif