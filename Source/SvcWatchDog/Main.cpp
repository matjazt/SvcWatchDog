#include <SvcWatchDog/SvcWatchDog.h>
#include <JsonConfig/JsonConfig.h>
#include <Logger/Logger.h>
#include <conio.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

int main(int argc, char* argv[])
{
    DbgMemCheckPoint1();

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
        ::GetModuleFileName(nullptr, tmp, sizeof(tmp) - 1);
        AUTO_TERMINATE(tmp);

        auto exePath = filesystem::path(tmp);
        auto cfgPath = exePath.parent_path() / (exePath.stem().string() + ".json");

        JsonConfig cfg;
        JsonConfig::SetInstance(&cfg);
        try
        {
            cfg.load(cfgPath);
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
        Lg.Config(Cfg, "log");
        Lg.Start();

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
    }

    WSACleanup();

    DbgMemCheckPoint2();
    DbgMemCompareChkPoints();

    // _getch();

    return returnCode;
}
