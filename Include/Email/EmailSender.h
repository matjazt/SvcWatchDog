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

    void Configure(JsonConfig& cfg, const std::string& section);

    int SendSimpleEmail(const std::string& subject, const std::string& utf8body, const std::vector<std::string>& toAddresses,
                        const std::string& sourceAddress = "", int timeout = 0);

   private:
    static EmailSender* m_instance;

    std::string m_smtpServerUrl;
    int m_sslFlag;  // see CURLOPT_USE_SSL option in libcurl
    std::string m_username;
    std::string m_password;
    std::string m_defaultSourceAddress;
    int m_timeout;  // in milliseconds
};

#endif