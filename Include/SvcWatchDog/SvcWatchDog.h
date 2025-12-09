/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---------------------------------------------------------------------------
 *
 * Official repository: https://github.com/matjazt/SvcWatchDog
 *
 * ==================================================================================
 *
 * Additional copyright notice: windows service integration is based on PJ Naughter's
 * CNTService class ( http://www.naughter.com/serv.html ) and contains fragments of
 * its code. It is used here with explicit permission by the author.
 *
 * ==================================================================================
 */

#ifndef _SVCWATCHDOG_H_
#define _SVCWATCHDOG_H_

#include <windows.h>
#include <SimpleTools/SimpleTools.h>

using namespace std;

#define SVCWATCHDOG_MAX_ARGV 25

#define SERVICE_CONTROL_USER 128

// Copyright notice: this class is based on PJ Naughter's CNTService class ( http://www.naughter.com/serv.html )
// and contains fragments of its code. It is used here with explicit permission by the author.

class SvcWatchDog
{
   public:
    SvcWatchDog() noexcept;
    ~SvcWatchDog();

    // prevent copying and assignment
    DELETE_COPY_AND_ASSIGNMENT(SvcWatchDog);

    void Configure();

    bool IsInstalled() noexcept;
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

#endif