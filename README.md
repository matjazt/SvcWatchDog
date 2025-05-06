# SvcWatchDog
[SvcWatchDog](https://github.com/matjazt/SvcWatchDog) is a Windows utility that operates like a Win32 service, enabling the execution and monitoring of virtually any console application. By handling service-related functionality, it eliminates the need for developers to integrate their applications directly with the Win32 service system, while also enhancing reliability through robust monitoring mechanisms.
The program automatically detects when the application terminates and restarts it as needed. Additionally, it can be configured to listen for regular UDP "pings" from the application, confirming its operational state. If the application fails to send a ping within a defined timeframe, SvcWatchDog will restart it, helping to recover from unexpected hangs or crashes that do not result in termination.
To facilitate a controlled shutdown, SvcWatchDog creates a global Win32 event and shares its name with the application, allowing it to monitor the event and gracefully terminate when requested.

## How to test and learn
- Download the binary package

## How to use

- Download or build SvcWatchDog.exe
- Rename it to your desired service name (e.g., MyService.SvcWatchdog.exe)
- Create a configuration file named MyService.SvcWatchdog.json in the same directory as the executable. See the Configuration file section below.
- Install the service using the command: MyService.SvcWatchdog.exe -install
- Start the service using the command: net start MyService.SvcWatchdog or by using the Windows Services management console (services.msc).
- To stop the service, use the command: net stop MyService.SvcWatchdog or the Windows Services management console.
- To uninstall the service, use the command: MyService.SvcWatchdog.exe -uninstall.


## Configuration file

The configuration file should have the same name as the executable, but with a .json extension. It should be placed in the same directory as the executable. The file is in JSON format and contains sections and parameters, described below.
Please note that if you make a mistake in the JSON syntax, SvcWatchDog may not start or may behave unexpectedly. It is recommended to validate the file after each change, simply by running the SvcWatchDog executable without any parameters. If the file is not valid, SvcWatchDog will print an error message and exit.

### log section

- consoleLevel: Minimum log level to be displayed in the console. Possible values are from 0 to 5: verbose, debug, info, warning, error, fatal. Default is 0 (verbose).
- fileLevel: Minimum log level to be written to the file.
- filePath: Log file path, can be absolute or relative to the SvcWatchDog workDir (see below). Default is empty, which means that logs are note written to file.
- maxFileSize: Maximum size of the log file in bytes. Default 20 MB. Note that maxFileSize is a recommendation - files are rotated when they exceed this size, so the actual size may be a bit larger.
- maxOldFiles: Maximum number of old log files to keep. Default is 0, which means that no automatic deletion is performed.
- maxWriteDelay: Maximum delay in milliseconds for writing log messages to the file. Default is 500 ms.
- logThreadId: Set to true if you want to log the thread ID of the thread that generated the log message. Default is false.

### svcWatchDog section

- workDir: Path to the working directory, be it absolute or relative to the SvcWatchDog executable. Default is the directory where SvcWatchDog executable is located.
- args: List of arguments to use when starting the application, with the first argument being the path or at least the name of the application. Path can be absolute or relative to the working directory. 
- usePath: true if you wish to use the PATH environment variable to find the application. Default is false.
- autoStart: true if you wish to start the win32 service automatically when the system boots. Default is false.
- restartDelay: Delay in miliseconds before restarting the application after it has been stopped. Default is 5000 ms.
- shutdownTime: Timeout in milliseconds for the child application to shut down gracefully. Default is 10000 ms.
- watchdogTimeout: Timeout in miliseconds for the child application to send UDP ping. Default is -1, which means that the watchdog funcionality is disabled.

## 3rd party libraries and code

- SvcWatchDog is based on PJ Naughter's CNTService class, which is a wrapper around the Windows Service API. You can find more information about it here:
http://www.naughter.com/serv.html
Fragments of CNTService code were used with explicit permission by the author. Thanks to PJ Naughter ( http://www.naughter.com/ ) for his work and for allowing us to use it!
This disclaimer is also included in the file [LICENSE.CNTService](LICENSE.CNTService).

- SvcWatchDog uses the "nlohmann/json" library for JSON parsing. This library is included in the project and is licensed under the MIT License. You can find more information about it here:
https://github.com/nlohmann/json
https://json.nlohmann.me/
The disclaimer for this library is included in file [LICENSE.jsonhpp](LICENSE.jsonhpp).
Thanks to Niels Lohmann and contributors for this great library!
