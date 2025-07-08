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

#include <SvcWatchDog/SvcWatchDog.h>
#include <JsonConfig/JsonConfig.h>
#include <Logger/Logger.h>

#include <curl/curl.h>

#include <process.h>
#include <direct.h>
#include <iostream>

SvcWatchDog* SvcWatchDog::m_instance = nullptr;

// NOTE: when you increment version, you have to do it here as well as in the resource file (but keep the fourth number at 0, as we're only
// using three of them - major.minor.patch)
#define SVCWATCHDOG_VERSION "1.0.0"

SvcWatchDog::SvcWatchDog() noexcept : m_section("svcWatchDog")
{
    // copy the address of the current object so we can access it from
    // the static member callback functions.
    // WARNING: This limits the application to only one SvcWatchDog object.
    m_instance = this;
    m_killTime = 0;
    m_isRunning = false;
    m_loopTriggerEvent = CreateEvent(NULL, FALSE, FALSE, nullptr);
    ZeroMemory(&m_argv, sizeof(m_argv));
    ZeroMemory(&m_serviceStatus, sizeof(m_serviceStatus));
    m_watchdogSocket = INVALID_SOCKET;
    m_watchdogPort = 0;
    m_serviceStatusHandle = nullptr;
    m_shutdownEvent = nullptr;

    // figure out paths and file names
    char tmp[1000] = "";
    GetModuleFileNameA(nullptr, tmp, sizeof(tmp) - 1);
    AUTO_TERMINATE(tmp);

    m_exeFile.assign(tmp);
    m_exeDir = m_exeFile.parent_path();
    m_serviceName = m_exeFile.stem().string();

    string workDir = Cfg.GetString(m_section, "workDir", "");
    m_workingDirectory = filesystem::absolute(m_exeDir / workDir);

    CdToWorkingDir();
}

void SvcWatchDog::Configure()
{
    LOGSTR(Information) << "libcurl version: " << curl_version();

    LOGSTR(Information) << "SvcWatchDog " << SVCWATCHDOG_VERSION << ", build timestamp: " << __DATE__ << " " << __TIME__;
    LOGSTR(Information) << "service name: " << m_serviceName;

    LOGSTR() << "exeFile=" << m_exeFile.string();
    LOGSTR() << "exeDir=" << m_exeDir.string();
    LOGSTR() << "workDir=" << m_workingDirectory.string();

    // set up the initial service status
    m_serviceStatusHandle = nullptr;
    m_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    m_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    m_serviceStatus.dwWin32ExitCode = 0;
    m_serviceStatus.dwServiceSpecificExitCode = 0;
    m_serviceStatus.dwCheckPoint = 0;
    m_serviceStatus.dwWaitHint = 0;

    const bool usePath = Cfg.GetBool(m_section, "usePath", false);
    LOGSTR() << "usePath=" << BOOL2STR(usePath);

    // read all child process arguments, starting with the actual executable path (or at least file name)
    auto argv = Cfg.GetStringVector(m_section, "args");

    int i = 0;
    for (const auto& arg : argv)
    {
        if (i >= SVCWATCHDOG_MAX_ARGV)
        {
            LOGSTR(Error) << "too many arguments, max is " << SVCWATCHDOG_MAX_ARGV;
            break;
        }

        LOGSTR(Information) << "arg #" << i << ": " << arg;

        if (i == 0)
        {
            if (usePath)
            {
                const char* achPath = getenv("PATH");
                LOGSTR() << "searching path " << achPath;
                auto pathDirectories = Split(achPath, ';');
                bool targetFound = false;
                for (const string& dir : pathDirectories)
                {
                    filesystem::path candidate = filesystem::path(dir) / arg;
                    if (filesystem::is_regular_file(candidate))
                    {
                        // found the file in the path, use it
                        argv[0] = candidate.string();
                        targetFound = true;
                        break;
                    }
                }
                if (!targetFound)
                {
                    LOGSTR(Error) << "target executable " << argv[0] << " not found in path";
                }
            }

            // while m_argv contains quoted parameters (starting with executable), we also need non-quoted executable (for logging purposes
            // and for actual execution)
            m_targetExecutable = argv[0];
            LOGSTR() << "using target executable " << m_targetExecutable;
        }

        m_argv[i] = _strdup(("\"" + argv[i] + "\"").c_str());
        i++;
    }
    m_argv[i] = nullptr;  // terminate the array of arguments
}

// Default command line argument parsing
// Returns true if it found an arg it recognized, false if not
// Note: processing some arguments causes output to stdout or stderr to be generated.
bool SvcWatchDog::ParseStandardArgs(int argc, char* argv[])
{
    // See if we have any command line args we recognize
    if (argc <= 1) return false;

    if (_stricmp(argv[1], "-v") == 0)
    {
        // Spit out version info
        cout << "The " << m_serviceName << " service is " << (IsInstalled() ? " currently " : " not ") << "installed" << endl;
        return true;  // say we processed the argument
    }

    if (_stricmp(argv[1], "-i") == 0)
    {
        // Request to install.
        if (IsInstalled())
        {
            cerr << "The " << m_serviceName << " service is already installed." << endl;
        }
        else
        {
            // Try and install the copy that's running
            if (Install())
            {
                cout << m_serviceName << " service installed." << endl;
            }
            else
            {
                cerr << "The " << m_serviceName << " service failed to install, error " << GetLastError() << endl;
            }
        }
        return true;  // say we processed the argument
    }

    if (_stricmp(argv[1], "-u") == 0)
    {
        // Request to uninstall.
        if (!IsInstalled())
        {
            cerr << "The " << m_serviceName << " service is not installed." << endl;
        }
        else
        {
            // Try and remove the copy that's installed
            if (Uninstall())
            {
                cout << m_serviceName << " service uninstalled." << endl;
            }
            else
            {
                cerr << "Could not uninstall service " << m_serviceName << ", error " << GetLastError() << endl;
            }
        }

        return true;  // say we processed the argument
    }

    // Don't recognize the args
    return false;
}

void SvcWatchDog::CdToWorkingDir()
{
    // originate from workdir or from "exe's" directory, but not from winnt\system32
    if (_chdir(m_exeDir.string().c_str()))
    {
        LOGSTR(Error) << "failed to change directory to the folder where the SvcWatchDog binary is: " << m_exeDir.string();
        return;
    }
    if (_chdir(m_workingDirectory.string().c_str()))
    {
        LOGSTR(Error) << "failed to change directory to the working folder: " << m_workingDirectory.string();
        return;
    }
}

SvcWatchDog::~SvcWatchDog()
{
    LOGSTR() << "shutting down";
    for (int i = 0; i < SVCWATCHDOG_MAX_ARGV && m_argv[i]; i++)
    {
        free(m_argv[i]);
    }
    SAFE_CLOSE_HANDLE(m_shutdownEvent);
    SAFE_CLOSE_HANDLE(m_loopTriggerEvent);

    SAFE_CLOSE_SOCKET(m_watchdogSocket);
}

void SvcWatchDog::SetStatus(DWORD state)
{
    LOGSTR() << "serviceStatusHandle=" << (uint64_t)m_serviceStatusHandle << ", state=" << state;
    m_serviceStatus.dwCurrentState = state;
    ::SetServiceStatus(m_serviceStatusHandle, &m_serviceStatus);
}

void SvcWatchDog::StartUdpWatchDog()
{
    // Create UDP socket
    m_watchdogSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_watchdogSocket == INVALID_SOCKET)
    {
        LOGSTR(Error) << "failed to create UDP socket";
        return;
    }

    // Set socket to non-blocking mode
    u_long mode = 1;  // 1 = non-blocking
    ioctlsocket(m_watchdogSocket, FIONBIO, &mode);

    // Bind socket to a random UDP port on localhost
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(0);  // 0 = Let OS pick a random port

    if (::bind(m_watchdogSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        LOGSTR(Error) << "bind failed";
        SAFE_CLOSE_SOCKET(m_watchdogSocket);
        return;
    }

    // Get the assigned port number
    sockaddr_in assignedAddr;
    int addrLen = sizeof(assignedAddr);
    if (getsockname(m_watchdogSocket, (sockaddr*)&assignedAddr, &addrLen) == SOCKET_ERROR)
    {
        LOGSTR(Error) << "getsockname failed";
        SAFE_CLOSE_SOCKET(m_watchdogSocket);
        return;
    }

    m_watchdogPort = ntohs(assignedAddr.sin_port);
    LOGSTR(Information) << "listening on 127.0.0.1:" << m_watchdogPort << " (UDP)";
}

bool SvcWatchDog::ReceiveUdpPing()
{
    sockaddr_in clientAddr;
    char buffer[1024];
    int clientAddrSize = sizeof(clientAddr);
    int received = recvfrom(m_watchdogSocket, buffer, sizeof(buffer) - 2, 0, (sockaddr*)&clientAddr, &clientAddrSize);

    assert(received < (int)sizeof(buffer));
    if (received > 0 && received < (int)sizeof(buffer))
    {
        // Check if the received data matches the secret
        if (received == (int)m_watchdogSecret.length() && strncmp(buffer, m_watchdogSecret.c_str(), received) == 0)
        {
            // Successfully received a ping
            return true;
        }
        else
        {
            buffer[received] = 0;  // Null-terminate received data
            for (char* p = buffer; *p; p++)
            {
                if (!isprint(*p))
                {
                    *p = ' ';
                }
            }
            LOGSTR(Warning) << "received invalid ping data: " << buffer;
        }
    }
    else
    {
        if (received == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
            {
                LOGSTR(Error) << "recvfrom failed with error code: " << error;
            }
        }
    }

    return false;
}

void SvcWatchDog::Run()
{
    if (m_targetExecutable.empty() || m_workingDirectory.empty())
    {
        LOGSTR(Error) << "parameters missing, check configuration";
        while (m_isRunning)
        {
            WaitForSingleObject(m_loopTriggerEvent, 1000);
        }
        return;
    }

    CdToWorkingDir();

    const int watchdogTimeout = Cfg.GetNumber(m_section, "watchdogTimeout", -1);
    LOGSTR(Information) << "watchdogTimeout=" << watchdogTimeout;

    // if UDP watchdog is configured, start listening on a random port
    if (watchdogTimeout > 0)
    {
        // not much of a secret, but it should do
        m_watchdogSecret = to_string(rand()) + to_string(SteadyTime());
        StartUdpWatchDog();
        if (m_watchdogSocket != INVALID_SOCKET)
        {
#pragma warning(suppress : 6031)
            _putenv(("WATCHDOG_PORT=" + to_string(m_watchdogPort)).c_str());
#pragma warning(suppress : 6031)
            _putenv(("WATCHDOG_SECRET=" + m_watchdogSecret).c_str());
        }
    }

    auto tmp = filesystem::absolute(m_workingDirectory).string() + to_string(SteadyTime());
    string shutdownEventName = "Global\\SvcWatchDog.";
    for (const char ch : tmp)
    {
        if (isalnum(ch))
        {
            shutdownEventName += (char)tolower(ch);
        }
    }

    m_shutdownEvent = CreateEvent(NULL, TRUE, FALSE, shutdownEventName.c_str());
    if (m_shutdownEvent == nullptr)
    {
        LOGSTR(Error) << "CreateEvent failed for " << shutdownEventName << ", error code: " << GetLastError();
    }

#pragma warning(suppress : 6031)
    _putenv(("SHUTDOWN_EVENT=" + shutdownEventName).c_str());

    while (m_isRunning)
    {
        if (m_shutdownEvent)
        {
            // event might be signaled by the previous watchdog-initiated shutdown
            ResetEvent(m_shutdownEvent);
        }
        // also kill time might be set by the previous watchdog-initiated shutdown
        m_killTime = 0;

        LOGSTR(Information) << "starting " << m_targetExecutable;

        auto processHandle = _spawnv(_P_NOWAIT, &m_targetExecutable[0], m_argv);

        WaitForSingleObject(m_loopTriggerEvent, 250);

        // now wait until the child process terminates (on request or by itself)
        //
        DWORD exitCode = STILL_ACTIVE;
        BOOL exitCodeValid = FALSE;
        uint64_t now = SteadyTime();
        uint64_t nextPing = now + watchdogTimeout;

        while (processHandle >= 0 && exitCode == STILL_ACTIVE && (m_killTime == 0 || m_killTime > now))
        {
            WaitForSingleObject(m_loopTriggerEvent, 200);

            exitCode = 0;
#pragma warning(suppress : 6001)
            exitCodeValid = GetExitCodeProcess((HANDLE)processHandle, &exitCode);

            if (!exitCodeValid)
            {
                // TODO: should we ignore this error at least temporarily? Is it possible that the process is still running?
                LOGSTR(Warning) << "GetExitCodeProcess failed, error code = " << GetLastError();
            }

            now = SteadyTime();

            if (m_watchdogSocket != INVALID_SOCKET && m_killTime == 0)
            {
                while (ReceiveUdpPing())
                {
                    // the process is alive and well
                    LOGSTR(Verbose) << "received watchdog ping";
                    nextPing = now + watchdogTimeout;
                }

                if (now > nextPing)
                {
                    LOGSTR(Warning) << "child process stopped sending valid UDP ping packets, restarting it";
                    InitiateProcessShutdown();
                }
            }
        }

        if (processHandle >= 0)
        {
            if (exitCode == STILL_ACTIVE)
            {
                LOGSTR(Warning) << "forcibly terminating child process";
                exitCodeValid = FALSE;
            }

            // try to terminate the child process in any case - better safe than sorry
            TerminateProcess((HANDLE)processHandle, 0);
            Sleep(50);
            CloseHandle((HANDLE)processHandle);
        }

        LOGSTR(m_isRunning ? Warning : Information)
            << m_targetExecutable << " died, exit code " << (exitCodeValid ? to_string(exitCode) : "unknown");

        if (m_isRunning)
        {
            const int restartDelay = Cfg.GetNumber(m_section, "restartDelay", 5000);
            LOGSTR() << "waiting " << restartDelay << " ms before restarting";
            WaitForSingleObject(m_loopTriggerEvent, restartDelay);
        }
    }
}

// Test if the service is currently installed
bool SvcWatchDog::IsInstalled() noexcept
{
    bool result = false;

    // Open the Service Control Manager
    SC_HANDLE scmHandle = ::OpenSCManager(nullptr,                 // local machine
                                          nullptr,                 // ServicesActive database
                                          SC_MANAGER_ALL_ACCESS);  // full access
    if (scmHandle)
    {
        // Try to open the service
        SC_HANDLE serviceHandle = ::OpenService(scmHandle, m_serviceName.c_str(), SERVICE_QUERY_CONFIG);
        if (serviceHandle)
        {
            result = true;
            ::CloseServiceHandle(serviceHandle);
        }

        ::CloseServiceHandle(scmHandle);
    }

    return result;
}

bool SvcWatchDog::Install()
{
    // Open the Service Control Manager
    SC_HANDLE scmHandle = ::OpenSCManager(nullptr,                 // local machine
                                          nullptr,                 // ServicesActive database
                                          SC_MANAGER_ALL_ACCESS);  // full access
    if (!scmHandle) return false;

    string loadOrderGroup = Cfg.GetString(m_section, "loadOrderGroup", "");
    LOGSTR(Information) << "loadOrderGroup=" << loadOrderGroup;

    const bool autoStart = Cfg.GetBool(m_section, "autoStart", false);
    LOGSTR(Information) << "autoStart=" << BOOL2STR(autoStart);

    // Create the service
    SC_HANDLE serviceHandle =
        ::CreateService(scmHandle, m_serviceName.c_str(), m_serviceName.c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
                        autoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,  // start condition
                        SERVICE_ERROR_NORMAL, m_exeFile.string().c_str(), loadOrderGroup.empty() ? nullptr : loadOrderGroup.c_str(),
                        nullptr, nullptr, nullptr, nullptr);

    if (!serviceHandle)
    {
        ::CloseServiceHandle(scmHandle);
        LOGSTR(Error) << "failed to create service " << m_serviceName.c_str();
        return false;
    }

    // tidy up
    ::CloseServiceHandle(serviceHandle);
    ::CloseServiceHandle(scmHandle);

    LOGSTR(Information) << "service " << m_serviceName.c_str() << " installed";
    return true;
}

bool SvcWatchDog::Uninstall()
{
    // Open the Service Control Manager
    SC_HANDLE scmHandle = ::OpenSCManager(nullptr,                 // local machine
                                          nullptr,                 // ServicesActive database
                                          SC_MANAGER_ALL_ACCESS);  // full access
    if (!scmHandle) return false;

    bool result = false;
    SC_HANDLE serviceHandle = ::OpenService(scmHandle, m_serviceName.c_str(), DELETE);
    if (serviceHandle)
    {
        if (::DeleteService(serviceHandle))
        {
            LOGSTR(Information) << "service " << m_serviceName << " removed";
            result = true;
        }
        else
        {
            LOGSTR(Error) << "service " << m_serviceName << " NOT removed";
        }
        ::CloseServiceHandle(serviceHandle);
    }

    ::CloseServiceHandle(scmHandle);
    return result;
}

bool SvcWatchDog::Start()
{
    char serviceName[100];
    SAFE_STRING_COPY(serviceName, m_serviceName.c_str());
    const SERVICE_TABLE_ENTRY st[] = {{serviceName, ServiceMain}, {nullptr, nullptr}};

    LOGSTR(Verbose) << "calling StartServiceCtrlDispatcher()";
    const bool b = ::StartServiceCtrlDispatcher(st) != 0;
    LOGSTR(Verbose) << "StartServiceCtrlDispatcher() result: " << BOOL2STR(b);
    return b;
}

// static member function (callback)
void SvcWatchDog::ServiceMain(DWORD, LPTSTR*)
{
    // Get a pointer to the C++ object
    SvcWatchDog* instance = m_instance;

    LOGSTR(Verbose) << "entering";
    // Register the control request handler

    SetLastError(NO_ERROR);

    instance->m_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    instance->m_serviceStatusHandle = RegisterServiceCtrlHandler(instance->m_serviceName.c_str(), Handler);
    if (instance->m_serviceStatusHandle == nullptr)
    {
        LOGSTR(Error) << "RegisterServiceCtrlHandler failed";
        return;
    }

    // Start the initialisation
    if (instance->Initialize())
    {
        // Do the real work.
        // When the Run function returns, the service has stopped.
        instance->m_isRunning = true;
        instance->m_serviceStatus.dwWin32ExitCode = 0;
        instance->m_serviceStatus.dwCheckPoint = 0;
        instance->m_serviceStatus.dwWaitHint = 0;
        instance->Run();
    }

    // Tell the service manager we are stopped
    instance->SetStatus(SERVICE_STOPPED);

    LOGSTR(Verbose) << "done";
}

bool SvcWatchDog::Initialize()
{
    LOGSTR() << "entering";

    // Start the initialization
    SetStatus(SERVICE_START_PENDING);

    // Perform the actual initialization
    const bool result = OnInit();

    // Set final state
    m_serviceStatus.dwWin32ExitCode = GetLastError();
    m_serviceStatus.dwCheckPoint = 0;
    m_serviceStatus.dwWaitHint = 0;
    if (!result)
    {
        LOGSTR(Error) << "failed";
        SetStatus(SERVICE_STOPPED);
        return false;
    }

    SetStatus(SERVICE_RUNNING);

    LOGSTR() << "done OK";
    return true;
}

// static member function (callback) to handle commands from the service control manager
void SvcWatchDog::Handler(DWORD dwOpcode)
{
    // Get a pointer to the object
    SvcWatchDog* instance = m_instance;

    switch (dwOpcode)
    {
        case SERVICE_CONTROL_STOP:  // 1
            instance->SetStatus(SERVICE_STOP_PENDING);
            instance->OnStop();
            instance->m_isRunning = false;
            break;

        case SERVICE_CONTROL_PAUSE:  // 2
            instance->OnPause();
            break;

        case SERVICE_CONTROL_CONTINUE:  // 3
            instance->OnContinue();
            break;

        case SERVICE_CONTROL_INTERROGATE:  // 4
            instance->OnInterrogate();
            break;

        case SERVICE_CONTROL_SHUTDOWN:  // 5

            instance->SetStatus(SERVICE_STOP_PENDING);
            instance->OnShutdown();
            instance->m_isRunning = false;
            break;

        default:
            if (dwOpcode < SERVICE_CONTROL_USER || !instance->OnUserControl(dwOpcode))
            {
                LOGSTR(Error) << "unknown user control code " << dwOpcode;
            }
            break;
    }

    ::SetServiceStatus(instance->m_serviceStatusHandle, &instance->m_serviceStatus);
}

void SvcWatchDog::InitiateProcessShutdown()
{
    const uint64_t shutdownTime = Cfg.GetNumber(m_section, "shutdownTime", 10000);
    LOGSTR(Information) << "signalling the process and setting timeout to now + " << shutdownTime << " ms";

    // signal the child process, so it can shut down gracefully
    if (m_shutdownEvent)
    {
        SetEvent(m_shutdownEvent);
    }

    m_killTime = SteadyTime() + shutdownTime;
}

void SvcWatchDog::OnStop()
{
    LOGSTR() << "stopping service";
    m_isRunning = false;

    CdToWorkingDir();

    InitiateProcessShutdown();
    SetEvent(m_loopTriggerEvent);
}

void SvcWatchDog::OnPause() { LOGSTR(Verbose) << "doing nothing"; }

void SvcWatchDog::OnContinue() { LOGSTR(Verbose) << "doing nothing"; }

void SvcWatchDog::OnInterrogate() { LOGSTR(Verbose) << "doing nothing"; }

bool SvcWatchDog::OnInit()
{
    LOGSTR() << "doing nothing";
    return true;
}

void SvcWatchDog::OnShutdown()
{
    LOGSTR() << "shutting down";
    OnStop();
}

// Process user control requests
bool SvcWatchDog::OnUserControl(DWORD dwOpcode)
{
    LOGSTR() << "dwOpcode=" << dwOpcode;

    CdToWorkingDir();

    switch (dwOpcode)
    {
        case SERVICE_CONTROL_USER:
            // Save the current status in the registry
            return true;

        default:
            break;
    }
    return false;  // say not handled
}
