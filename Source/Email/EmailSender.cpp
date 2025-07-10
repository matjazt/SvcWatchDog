/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <curl/curl.h>
#include <Email/EmailSender.h>
#include <Logger/Logger.h>
#include <CryptoTools/CryptoTools.h>

#include <iostream>

EmailSender* EmailSender::m_instance = nullptr;

EmailSender::EmailSender() noexcept : m_sslFlag(CURLUSESSL_ALL), m_timeout(120000) {}

EmailSender::~EmailSender() {}

EmailSender* EmailSender::GetInstance() noexcept { return m_instance; }

void EmailSender::SetInstance(EmailSender* instance) noexcept { m_instance = instance; }

void EmailSender::Configure(JsonConfig& cfg, const string& section)
{
    LOGSTR() << "reading configuration from section: " << section;

    m_smtpServerUrl = cfg.GetString(section, "smtpServerUrl", "");
    LOGSTR() << "smtpServer=" << m_smtpServerUrl;

    m_defaultSourceAddress = cfg.GetString(section, "defaultSourceAddress", "");
    LOGSTR() << "defaultSourceAddress=" << m_defaultSourceAddress;

    if (m_smtpServerUrl.empty() || m_defaultSourceAddress.empty())
    {
        LOGSTR(Error) << "smtpServerUrl not configured in section: " << section;
        return;
    }

    m_sslFlag = cfg.GetNumber(section, "sslFlag", m_sslFlag);
    LOGSTR() << "sslFlag=" << m_sslFlag;

    m_username = cfg.GetString(section, "username", "");
    LOGSTR() << "username=" << m_username;

    m_password = Crypto.GetPossiblyEncryptedConfigurationString(Cfg, section, "password", "");
    LOGSTR() << "password=" << (m_password.empty() ? "<none>" : "<non-empty>");

    m_timeout = cfg.GetNumber(section, "timeout", m_timeout);
    LOGSTR() << "timeout=" << m_timeout;
}

/*
struct UploadContext
{
    string payload;
    size_t offset = 0;
};

size_t payload_source(char* ptr, size_t size, size_t nmemb, void* userp)
{
    UploadContext* ctx = (UploadContext*)userp;
    size_t room = size * nmemb;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
    {
        return 0;
    }

    size_t chunkSize = ctx->payload.length() - ctx->offset;
    if (room < chunkSize)
    {
        chunkSize = room;
    }

    memcpy(ptr, &ctx->payload[ctx->offset], chunkSize);
    ctx->offset += chunkSize;

    return chunkSize;
}
*/

int EmailSender::SendSimpleEmail(const string& subject, const string& utf8body, const vector<string>& toAddresses,
                                 const string& fromAddress, int timeout)
{
    string toString = JoinStrings(toAddresses, ",");
    LOGSTR(Information) << "sending email to " << toString;

    // TODO: verify that the addresses are valid email address, verify that subject is syntatically correct, etc.
    auto actualFromAddress = fromAddress.empty() ? m_defaultSourceAddress : fromAddress;

    auto curl = curl_easy_init();
    if (!curl)
    {
        LOGSTR(Error) << "failed to initialize curl";
        return CURLE_FAILED_INIT;
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)(timeout > 0 ? timeout : m_timeout));

    curl_easy_setopt(curl, CURLOPT_URL, m_smtpServerUrl.c_str());

    curl_easy_setopt(curl, CURLOPT_USE_SSL, m_sslFlag);

    if (!m_username.empty())
    {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_username.c_str());
    }

    if (!m_password.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password.c_str());
    }

    // Note that this option is not strictly required, omitting it results in
    // libcurl sending the MAIL FROM command with empty sender data. All
    // autoresponses should have an empty reverse-path, and should be directed
    // to the address in the reverse-path which triggered them. Otherwise,
    // they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
    // details.
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, actualFromAddress.c_str());

    // Add recipients
    curl_slist* recipients = nullptr;
    for (const auto& toAddress : toAddresses)
    {
        recipients = curl_slist_append(recipients, toAddress.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    // Add email headers (including Subject)
    auto headers = curl_slist_append(nullptr, ("Subject: " + subject).c_str());
    headers = curl_slist_append(headers, ("From: " + actualFromAddress).c_str());
    headers = curl_slist_append(headers, ("To: " + toString).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Create MIME message by default
    auto mime = curl_mime_init(curl);

    // Add email body
    auto part = curl_mime_addpart(mime);

    curl_mime_type(part, "text/plain; charset=UTF-8");
    curl_mime_type(part, "text/plain");

    curl_mime_data(part, utf8body.c_str(), CURL_ZERO_TERMINATED);

    // Add attachment - should we have any
    // part = curl_mime_addpart(mime);
    // curl_mime_encoder(part, "base64");
    // curl_mime_filedata(part, "s:\\temp\\bronson.gif");

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

    // Send the message
    const auto res = curl_easy_perform(curl);

    // Check for errors
    if (res == CURLE_OK)
    {
        LOGSTR(Information) << "email sent successfully to " << toString;
    }
    else
    {
        LOGSTR(Error) << "curl_easy_perform() failed with " << res << " while sending to " << toString << " (" << curl_easy_strerror(res)
                      << ")";
    }

    // Free the lists
    curl_slist_free_all(recipients);
    curl_slist_free_all(headers);
    curl_mime_free(mime);

    // curl does not send the QUIT command until you call cleanup, so you
    // should be able to reuse this connection for additional messages
    // (setting CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and
    // calling curl_easy_perform() again. It may not be a good idea to keep
    // the connection open for a long time though (more than a few minutes may
    // result in the server timing out the connection), and you do want to
    // clean up in the end.

    curl_easy_cleanup(curl);

    return res;
}
