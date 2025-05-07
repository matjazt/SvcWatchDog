/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 *
 * ==================================================================================
 *
 * Additional copyright notice: windows service integration is based on PJ Naughter's
 * CNTService class ( http://www.naughter.com/serv.html ) and contains fragments of
 * its code. It is used here with explicit permission by the author.
 *
 * ==================================================================================
 */

#include <windows.h>
#include <SimpleTools/SimpleTools.h>

#define SVCWATCHDOG_MAX_ARGV 25

#define SERVICE_CONTROL_USER 128

// Copyright notice: this class is based on PJ Naughter's CNTService class ( http://www.naughter.com/serv.html )
// and contains fragments of its code. It is used here with explicit permission by the author.

class SvcWatchDog : public NoCopy
{
   public:
    explicit SvcWatchDog();
    ~SvcWatchDog();

    void Configure();

    bool IsInstalled();
    bool Install();
    bool Uninstall();
    bool Start();
    void SetStatus(DWORD dwState);
    bool Initialize();
    void Run();
    bool OnInit();
    void OnStop();
    void OnInterrogate();
    void OnPause();
    void OnContinue();
    void OnShutdown();
    bool OnUserControl(DWORD dwOpcode);

    // static member functions
    static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI Handler(DWORD dwOpcode);

    bool ParseStandardArgs(int argc, char* argv[]);

    SERVICE_STATUS m_serviceStatus;

   private:
    void CdToWorkingDir();
    void StartUdpWatchDog();
    bool ReceiveUdpPing();
    void InitiateProcessShutdown();

    string m_section;
    string m_serviceName;
    filesystem::path m_exeFile;
    filesystem::path m_exeDir;
    filesystem::path m_workingDirectory;
    HANDLE m_shutdownEvent;

    // we need to pass an array of zero terminated strings to _spawnv method and also
    // keep the strings intact even after the call, because _spawnv doesn't copy them
    char* m_argv[SVCWATCHDOG_MAX_ARGV + 1];
    string m_targetExecutable;

    uint64_t m_killTime;  // uptime, pri katerem je treba ubiti proces

    SOCKET m_watchdogSocket;
    int m_watchdogPort;
    string m_watchdogSecret;

    SERVICE_STATUS_HANDLE m_serviceStatusHandle;

    bool m_isRunning;
    HANDLE m_loopTriggerEvent;

    // static data
    static SvcWatchDog* m_instance;
};
