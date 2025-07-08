/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#include <SvcWatchDog/SvcWatchDog.h>
#include <JsonConfig/JsonConfig.h>
#include <Logger/Logger.h>
#include <CryptoTools/CryptoTools.h>
#include <conio.h>
#include <iostream>
#include <Logger/LoggerEmailPlugin.h>

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

int main(int argc, char* argv[])
{
    // NOTE: this seems to be far better option than _CrtDumpMemoryLeaks(), because it checks the memory leaks later in the
    // shutdown process, thus not showing false leaks for Botan library, for example.
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    int returnCode;
    {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            cerr << "WSAStartup failed" << endl;
            return -1;
        }

        // figure out the configuration file path - it should be exactly the same as
        // the executable name, but with a .json extension
        char tmp[1000] = "";
        GetModuleFileNameA(nullptr, tmp, sizeof(tmp) - 1);
        AUTO_TERMINATE(tmp);

        auto exePath = filesystem::path(tmp);
        auto cfgPath = exePath.parent_path() / (exePath.stem().string() + ".json");

        JsonConfig cfg;
        JsonConfig::SetInstance(&cfg);
        try
        {
            cfg.Load(cfgPath);
        }
        catch (const std::exception& e)
        {
            cerr << "Unable to use configuration file " << cfgPath.string() << "." << endl << e.what() << endl;
            return -2;
        }

        // Create the service object first, because we need it to change directory to the working folder, so relative log paths work
        SvcWatchDog cService;

        Logger logger;
        Logger::SetInstance(&logger);
        Lg.Configure(Cfg);
        Lg.Start();

        CryptoTools cryptoTools;
        CryptoTools::SetInstance(&cryptoTools);

        // initialize CryptoTools with a default password, in case password file is not configured in the
        // configuration file (cryptoTools->passwordFile)
        cryptoTools.Configure(Cfg, "cryptoTools", "A7k2TDrZkf3kMCGMmBhA");

        // configure all email plugins
        LoggerEmailPlugin::ConfigureAll(Cfg, Lg);

        // now we can configure the service, because the logger is ready
        cService.Configure();

        // Parse for standard arguments (install, uninstall, version etc.)
        if (!cService.ParseStandardArgs(argc, argv))
        {
            // Didn't find any standard args so start the service
            // Uncomment the DebugBreak line below to enter the debugger
            // when the service is started.
            // DebugBreak();
            cService.Start();
        }

        // When we get here, the service has been stopped
        returnCode = cService.m_serviceStatus.dwWin32ExitCode;
        LOGSTR() << "exiting with result code " << returnCode;

        // cryptoTools.SelfTest();
        // cryptoTools.SelfTest();
        // _getch();
    }

    WSACleanup();

    // _getch();

    // memory leak detection test
    // malloc(666);

    return returnCode;
}
