# SvcWatchDog

[**SvcWatchDog**](https://github.com/matjazt/SvcWatchDog) is a Windows utility that operates like a Win32 service, enabling the execution and monitoring of virtually any non-interactive application. By handling service-related functionality, it eliminates the need for developers to integrate their applications directly with the Win32 service system, while also enhancing reliability through robust monitoring mechanisms. 
The utility consists of a single executable file and requires a single JSON configuration file.
It is programmed in C++ and is designed to be as independent as possible, while remaining easy to integrate into your software.

## How it works

**SvcWatchDog** integrates seamlessly with the Windows services system, functioning as a dedicated Windows service. It manages the lifecycle of a configured application, ensuring that it starts and stops in sync with the Windows service. Additionally, it continuously monitors the application's state, **automatically restarting** it if an unexpected termination occurs.
To verify the application's operational status, **SvcWatchDog** can listen for **UDP pings** sent by the application at regular intervals. If a ping is not received within the defined timeout, the watchdog assumes the application has become unresponsive and restarts it — helping to recover from hangs or silent failures that do not result in termination.  

Unlike a full-fledged service manager, **SvcWatchDog** is designed to run only a **single application per instance**. However, **multiple instances** can operate simultaneously, each configured separately with distinct names and configuration files. Each instance runs as an independent Windows service, making it possible to manage multiple applications using **SvcWatchDog**.
Both configuration and service files can be bundled with your software, allowing **SvcWatchDog** to operate discreetly in the background.

Check out the [**NotificationServer**](https://github.com/matjazt/NotificationServer) and
[**SvcWatchDogClientDemo**](https://github.com/matjazt/SvcWatchDogClientDemo) demo projects for a hands-on example of integrating
**SvcWatchDog** with a **C#** or **Python** application. Additionally, take a look at the included 
[**TestService.ps1**](https://github.com/matjazt/SvcWatchDog/blob/main/Scripts/TestService.ps1) file —
a straightforward **PowerShell** script showcasing the same integration.

## Features

- Windows service integration  
- Auto-restart applications  
- Support for graceful shutdown  
- Run virtually any application as a Windows service  
- UDP watchdog (health checks)
- Detailed logging
- Fully configurable log delivery by e-mail (SMTP)
- JSON configuration file  

## Download

You can download the precompiled utility (x64) from the **Releases** section of the GitHub repository: <https://github.com/matjazt/SvcWatchDog/releases>

## How to use

Preparation steps:
- Build **SvcWatchDog**.exe from source or download binary version (see the **Download** section). 
- Rename it to your desired service name (e.g., *MyService.SvcWatchDog.exe*)  
- Create a configuration file named *MyService.SvcWatchDog.json* in the same directory as the executable. See the **Configuration file** section below.  

Installation steps (**Admin credentials required**):
- Install the service using the command: `MyService.SvcWatchDog.exe -install`. Once the service is installed, the **SvcWatchDog** executable and JSON configuration files must remain in the same folder where they were located during installation. Moving them elsewhere may disrupt functionality.
- Start the service using the command: `net start MyService.SvcWatchDog` or by using the Windows Services management console (services.msc).  
- To stop the service, use the command: `net stop MyService.SvcWatchDog` or the Windows Services management console.  
- To uninstall the service, use the command: `MyService.SvcWatchDog.exe -uninstall`.  

## Configuration file

The configuration file should have the same name as the executable, but with a **.json** extension. It should be placed in the same directory as the executable. The file is in JSON format and contains sections and parameters, described below.  
Please note that if you make a mistake in the JSON syntax, **SvcWatchDog** may not start or may behave unexpectedly. It is recommended to validate the file after each change, simply by running the **SvcWatchDog** executable without any parameters. If the file is not valid, **SvcWatchDog** will print an error message and exit.  

### Example configuration file:  

Please note that parameters prefixed with DISABLED or OPTIONAL are ignored by SvcWatchDog. To enable them, simply
remove the prefix and accompanying space.

```json
{
  "log": {
    "minConsoleLevel": 99,
    "minFileLevel": 0,
    "filePath": "log\\SvcWatchDog.log",
    "maxFileSize": 10000000,
    "maxOldFiles": 3,
    "maxWriteDelay": 500,
    "logThreadId": false,
    "DISABLED email": {
      "someRecipients": {
        "minLogLevel": 2,
        "recipients": [ "janet@example.com", "brad@example.com" ],
        "OPTIONAL subject": "dammit Janet",
        "emailSection": "smtp.email-gmx",
        "maxDelay": 10,
        "maxLogs": 2000,
        "emailTimeoutOnShutdown": 2000
      },
      "anotherRecipient": {
        "minLogLevel": 2,
        "recipients": [ "riff.raff@example.com" ],
        "subject": "time warp news",
        "emailSection": "smtp.email-gmail",
        "maxDelay": 10,
        "maxLogs": 2000,
        "emailTimeoutOnShutdown": 2000
      }
    }
  },
  "svcWatchDog": {
    "workDir": "..",
    "args": [
      "powershell.exe",
      "-ExecutionPolicy",
      "Unrestricted",
      "-File",
      "TestService\\TestService.ps1"
    ],
    "usePath": true,
    "autoStart": true,
    "restartDelay": 2000,
    "shutdownTime": 3000,
    "watchdogTimeout": 7000
  },
  "smtp": {
    "email-gmail": {
      "smtpServerUrl": "smtps://smtp.gmail.com:465",
      "sslFlag": 3,
      "defaultSourceAddress": "riff.raff at gmail.com",
      "username": "riff.raff at gmail.com",
      "password": "riff raff's application passkey",
      "timeout": 10000
    },
    "email-gmx": {
      "smtpServerUrl": "smtps://mail.gmx.com:465",
      "sslFlag": 3,
      "defaultSourceAddress": "riff raff at gmx.com",
      "username": "riff raff at gmx.com",
      "password": "riff raff's actual account password",
      "timeout": 10000
    }
  },
  "OPTIONAL cryptoTools": {
    "passwordFile": "C:\\Windows\\CryptoToolsPassword.txt"
  }
}
```

### log section parameters:

- **minConsoleLevel**: Minimum log level to be displayed in the console. Possible values are from 0 to 5: verbose, debug, info, warning, error, fatal. Default is 0 (verbose).  
- **minFileLevel**: Minimum log level to be written to the file.  
- **filePath**: Log file path, can be absolute or relative to the **workDir** (see below). Default is empty, which means that logs are not written to file.  
- **maxFileSize**: Maximum size of the log file in bytes. Default 20 MB. Note that **maxFileSize** is a recommendation - files are rotated when they exceed this size, so the actual size may be a bit larger.  
- **maxOldFiles**: Maximum number of old log files to keep. Default is 0, which means that no automatic deletion is performed.  
- **maxWriteDelay**: Maximum delay in milliseconds for writing log messages to the file. Default is 500 ms.  
- **logThreadId**: Set to true if you want to log the thread ID of the thread that generated the log message. Default is false.  

### log.email sections:

To enable email delivery of **SvcWatchDog** logs, you must configure at least one **LoggerEmailPlugin** instance. You can define multiple instances as needed, with each one
configured independently from the rest.

In a typical setup, you likely want to be alerted only when the monitored service crashes or becomes unresponsive. For that purpose,
it's recommended to set minLogLevel to 3 (Warning). This ensures that only warnings and errors are sent via email — so as 
long as the service is operating normally, you won’t receive any messages.

If you’d also like to be notified on service startup or shutdown, consider setting minLogLevel to 1 (Debug). This provides a
broader log capture, excluding only the most frequent entries such as UDP ping notifications. It’s a suitable option for
high-availability or 24/7 monitored environments.

Keep in mind that sending emails during Windows shutdown can be unreliable, as it depends on the state of the networking stack at the time SvcWatchDog
is terminated. Therefore, messages generated during shutdown may not always be delivered.

**LoggerEmailPlugin** section parameters:

- **minLogLevel**: Minimum log level to be sent by email.
- **recipients**: A list of recipient email addresses.
- **subject**: Optional email subject line. If omitted, a default will be generated in the format **service-name@machine-name**, which is typically sufficient for most use cases.
- **emailSection**: Defines the path to the relevant SMTP configuration. Use dot notation to traverse nested JSON levels (e.g., smtp.email-gmail). A single SMTP section can be shared across multiple **LoggerEmailPlugin** configurations
- **maxDelay**: Specifies the maximum buffering time, in seconds, before logs are sent via email.
Some level of buffering is always applied to avoid generating a separate email for each log entry, which could lead to
excessive email volume. Unless near-instant alerts are needed, a value around 300 seconds (5 minutes) is recommended and
used by default.
- **maxLogs**: - Defines the maximum number of log entries to buffer before triggering an email dispatch. An email is sent as
soon as either this limit is reached or the **maxDelay** threshold is exceeded—whichever comes first. Default value is 1000.
- **emailTimeoutOnShutdown**: Specifies the SMTP timeout (in seconds) to be used during application shutdown. Since the shutdown
process is time-sensitive, this value should be shorter than the standard timeout to avoid delays.



### **SvcWatchDog** section parameters:

- **workDir**: Path to the working directory, be it absolute or relative to the **SvcWatchDog** executable. Default is the directory where **SvcWatchDog** executable is located.  
- **args**: List of arguments to use when starting the application, with the first argument being the path or at least the name of the application. Path can be absolute or relative to the working directory.  
- **usePath**: true if you wish to use the PATH environment variable to find the application. Default is false.  
- **autoStart**: true if you wish to start the win32 service automatically when the system boots. Default is false.  
- **restartDelay**: Delay in milliseconds before restarting the application after it has been stopped. Default is 5000 ms.  
- **shutdownTime**: Timeout in milliseconds for the child application to shut down gracefully. Default is 10000 ms.  
- **watchdogTimeout**: The timeout (in milliseconds) for the child application to send a UDP ping packets. The default value is -1, which disables the watchdog functionality. If you keep it this way, your application does not need to send pings.  
If you do enable it, it is **recommended to use a relatively large timeout value**. Otherwise, occasional system overloads, which are common in virtualized environments, may cause your application to be restarted due to delayed pings.  
The default configuration file includes a short watchdogTimeout just to make testing quicker.  
Additionally, the watchdogTimeout should be set to **at least twice the interval** at which your application sends pings.

### **SMTP** sections:

You can define any number of **SMTP** sections, and their names are entirely up to you — just avoid using duplicate names.
Keep in mind that simply defining an SMTP section doesn't guarantee it will be used. In the case of **SvcWatchDog**,
SMTP sections are only utilized if they're explicitly referenced by logger email plugins.
Section parameters:

 - **smtpServerUrl**: SMTP server address, specified in the format expected by the **Curl** library. Refer to the provided examples and the Curl documentation for further guidance.
 - **sslFlag**: See [**Curl** documentation](https://curl.se/libcurl/c/CURLOPT_USE_SSL.html). Note that the values span from 0 to 3, 0 being **CURLUSESSL_NONE** and 3 being **CURLUSESSL_ALL**.
 - **defaultSourceAddress**: The default "From" email address used when sending messages.
 - **username**: Optional SMTP username.
 - **password**: Optional SMTP password. For security reasons, it’s recommended to provide this value in encrypted form — refer to the encryption notes below for details.
 - **timeout**: SMTP delivery timeout in milliseconds.

### **cryptoTools** section parameters:

- **passwordFile**: Specifies the file that contains the master password used to decrypt SMTP credentials. If this parameter is omitted, a default hardcoded password will be applied. For more information, refer to the encryption notes below.

## Application considerations

You can run virtually any non-interactive application as a Windows service using **SvcWatchDog**. The application does not need to be aware of **SvcWatchDog**, but without explicit integration, features like **graceful shutdown** and **UDP ping monitoring** will not be available.
The following sections explain how to implement graceful shutdown and UDP ping monitoring in your application. You are encouraged to explore **TestService.ps1**, a short PowerShell demo script that demonstrates full integration with **SvcWatchDog**.

### Graceful shutdown
To enable graceful shutdown, your application must monitor the global Win32 event created by **SvcWatchDog**. The event name is provided to the application via the `SHUTDOWN_EVENT` environment variable.
When **SvcWatchDog** receives a shutdown command, it signals this event. Your application should detect this signal and terminate cleanly, ensuring proper shutdown procedures are followed.

### UDP watchdog
To enable **UDP ping monitoring**, your application must send periodic UDP packets to the **SvcWatchDog** process. **SvcWatchDog** listens for these packets, and if none are received within the specified timeout (configured via **watchdogTimeout**), it automatically restarts the application.
SvcWatchDog provides two **environment variables** to facilitate this:
- `WATCHDOG_PORT`: The UDP port to which the UDP packet should be sent. The destination address must be 127.0.0.1 (**do not** use *localhost* or *::1* etc.).
- `WATCHDOG_SECRET`: A short string that should be sent in the UDP packets to ensure they originate from the intended application.

## Configuration encryption
To enable SMTP support, password-based authentication must be implemented (though some SMTP servers may allow operation without it).
The password should be configured like any other parameter — in the JSON file. While plain text passwords are supported, their use
is discouraged. If **SvcWatchDog** detects a password stored in plain text, it will automatically encrypt it and log
the encrypted version, recommending that you replace the original with the secure version in your configuration file.

If you follow this recommendation, be sure to remove the plain text password from the log files as needed.

In this way, **SvcWatchDog** eliminates the need for a separate encryption tool by proactively guiding you through the process.

The actual encryption method used is **AES-256/CBC/PKCS7**, compatible with the following openssl command:

`openssl enc -base64 -e -aes-256-cbc -pbkdf2 -nosalt -pass pass:SuperSecretPassword`

While the choice of encryption algorithm is important (assuming it’s not severely outdated), the location of the master password
used to encrypt and decrypt the SMTP password plays an even more critical role in overall security.
**SvcWatchDog** offers two options for managing this master password:
- Store it in a dedicated text file (using all ASCII characters in the file), or
- Use the default, hardcoded master password.

Since **SvcWatchDog** is open source, the hardcoded password is publicly accessible — even to those without programming
experience — offering virtually no real protection. For this reason, it's strongly recommended that you configure a **password 
file** (see the `cryptoTools.passwordFile` parameter above).
Doing so helps protect your SMTP credentials in case your JSON configuration file becomes visible to unauthorized
parties — for instance, during a screen-sharing session. It also reduces the sensitivity of backing up the config file,
provided the master password file isn’t included in the same backup.

That said, keep in mind that this encryption approach offers little protection if someone gains direct access to your machine.
While integrating **Windows Credential Manager** could enhance security in this area, support for it is not currently
planned for **SvcWatchDog** in the near future.


## 3rd party libraries and code  

- Windows service integration is based on PJ Naughter's **CNTService** class, which is a wrapper around the Windows Service API. You can find more information about it here:  
<http://www.naughter.com/serv.html>  
Fragments of CNTService code were used with explicit permission by the author. Thank you!  
This disclaimer is also included in the file [LICENSE-CNTService](LICENSE-CNTService).  

- nlohmann/json library is being used for JSON parsing. You can find more information about it here:  
<https://github.com/nlohmann/json>  
<https://json.nlohmann.me/>  
Thanks to Niels Lohmann and contributors!  
The disclaimer for this library is included in file [LICENSE-jsonhpp](LICENSE-jsonhpp).

- libcurl (<https://curl.se/libcurl/>) is being used for SMTP email delivery. Thanks to Daniel Stenberg and contributors!
The disclaimer for this library is included in file [LICENSE-libcurl](LICENSE-libcurl).

- Botan library (<https://botan.randombit.net/>) is being used for encryption and decryption purposes. Thanks to 

## Competition
There are several alternatives to **SvcWatchDog** available, including:
- https://github.com/luisperezphd/RunAsService
- https://nssm.cc
- https://github.com/kflu/serman
- https://github.com/winsw/winsw
- https://sysprogs.com/legacy/tools/srvman/

Each of these projects is a bit different, so you may want to explore them to find the one that best fits your needs.

## Contact

If you have any questions about the utility, I encourage you to [open an issue on GitHub](https://github.com/matjazt/SvcWatchDog/issues). Please provide a detailed description of your request, problem, or inquiry, and be sure to include the SvcWatchDog **version** you are using for better context.
In case you would like to contact me directly, you can do so at: mt.dev[at]gmx.com .
