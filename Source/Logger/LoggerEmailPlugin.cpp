/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <JsonConfig/JsonConfig.h>
#include <Logger/LoggerEmailPlugin.h>

#include <iostream>
#include <chrono>

using namespace std;

void LoggerEmailPlugin::ConfigureAll(JsonConfig& cfg, Logger& logger, const string& parentSection)
{
    // find all sections of parentSection and configure a LoggerEmailPlugin for each of them.
    auto sections = cfg.GetKeys(parentSection, true, false, false);
    for (auto& section : sections)
    {
        logger.RegisterPlugin(make_unique<LoggerEmailPlugin>(cfg, parentSection + "." + section));
    }
}

LoggerEmailPlugin::LoggerEmailPlugin(JsonConfig& cfg, const string& section)
    : m_queue(std::make_unique<queue<string>>()), m_queueTimestamp(0)
{
    m_minLogLevel = (LogLevel)cfg.GetNumber(section, "minLogLevel", (int)LogLevel::Verbose);
    m_recipients = cfg.GetStringVector(section, "recipients");
    m_subject = cfg.GetString(section, "subject", "");
    m_emailSection = cfg.GetString(section, "emailSection", "");
    m_maxDelay = cfg.GetNumber(section, "maxDelay", 300);
    m_maxLogs = cfg.GetNumber(section, "maxLogs", 1000);
    m_timeoutOnShutdown = cfg.GetNumber(section, "timeoutOnShutdown", 3000);

    if (m_emailSection.empty() || m_recipients.empty() || m_minLogLevel >= MaskAllLogs)
    {
        // make sure m_emailSection is empty, that's our signal that email logging is not configured
        m_emailSection = "";
        // email logging not fully configured, disable it
        m_minLogLevel = MaskAllLogs;

        LOGSTR() << "section=" << section << ": disabled or not fully configured";
    }
    else
    {
        if (m_subject.empty())
        {
            // provide a portable default subject in the form of "logs from software @ host"
            m_subject = GetExecutableName() + " @ " + GetHostname();
        }

        m_emailSender.Configure(cfg, m_emailSection);

        LOGSTR() << "section=" << section << ": minLogLevel=" << m_minLogLevel << ", emailSection=" << m_emailSection
                 << ", recipients=" << JoinStrings(m_recipients, ", ") << ", subject=" << m_subject << ", maxDelay=" << m_maxDelay
                 << ", maxLogs=" << m_maxLogs << ", timeoutOnShutdown=" << m_timeoutOnShutdown;
    }
}

LoggerEmailPlugin::~LoggerEmailPlugin() = default;

LogLevel LoggerEmailPlugin::MinLogLevel() { return m_minLogLevel; }

void LoggerEmailPlugin::Log(LogLevel level, const string& message)
{
    if (level < m_minLogLevel)
    {
        return;
    }

    // we deliberately ignore the logs from EmailSender, because we don't want them to start an email sending loop
    if (m_minLogLevel <= level && message.find("EmailSender") == string::npos)
    {
        if (m_queue->empty())
        {
            m_queueTimestamp = SteadyTime();
        }
        m_queue->push(message);
    }
}

void LoggerEmailPlugin::Flush(bool stillRunning, bool force)
{
    m_cs.lock();

    if (m_queue->empty() || (!force && (int)m_queue->size() < m_maxLogs && (int)(SteadyTime() - m_queueTimestamp) < m_maxDelay * 1000))
    {
        // nothing to flush yet, let's unlock and return
        m_cs.unlock();
        return;
    }

    auto queueCopy = std::move(m_queue);
    m_queue = std::make_unique<queue<string>>();  // create a new queue for future logs
    // we're done with m_emailQueue, it is now freshly initialized
    // let's unlock the logger and then take care of the email sending
    m_cs.unlock();

    // We need to start a new thread to send the email, because it might take a while and we don't want to block the logger thread (even
    // when shutting down - we need to shut down reasonably fast, regardless of the email delivery).
    // NOTE: it is dangerous to detach a thread that depends on the logger, because the logger might be destroyed before the thread
    // finishes. However, since SendSimpleEmail only needs the class data to prepare the email (which is done at the beginning), the chances
    // of a crash are low.
    // TODO: (not really urgent) make the email delivery independent of basically everything, so that it can be safely detached.
    auto smtpThread = thread(&LoggerEmailPlugin::SendEmail, this, std::move(queueCopy), stillRunning);
    if (stillRunning)
    {
        // fire & forget: assume that we'll be running long enough for the SMTP delivery to complete
        smtpThread.detach();
        if (force)
        {
            // forced flush might mean that the software is shutting down. In such cases, a small delay might
            // prevent crash due to detaching thread too early (see the comment above).
            SLEEP(100);
        }
    }
    else
    {
        // we're shutting down, so we can't simply forget about the SMTP delivery because it would most likely get canceled if we simply
        // detach the thread. So, instead of detaching the thread, we try to join it in a reasonable time.
        // NOTE that we can't afford the usual long(ish) delivery timeout, but a shorter one. The SendMail method takes care of overriding
        // the default timeout when shutting down.
        smtpThread.join();
    }
}

void LoggerEmailPlugin::SendEmail(unique_ptr<queue<string>> emailQueue, bool stillRunning)
{
    // prepare the email content
    std::ostringstream oss;
    while (!emailQueue->empty())
    {
        oss << emailQueue->front();
        emailQueue->pop();
    }

    m_emailSender.SendSimpleEmail(m_subject, oss.str(), m_recipients, "", stillRunning ? 0 : m_timeoutOnShutdown);
}
