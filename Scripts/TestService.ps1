
filter Logger {"$(Get-Date -Format G): $_" | Out-File -Append "TestService.log"}


# Get the UDP packet details from environment variables
$watchdogSecret = $env:WATCHDOG_SECRET
$watchdogPort = $env:WATCHDOG_PORT
if (-not $watchdogSecret -or -not $watchdogPort) {
    "WATCHDOG_SECRET or WATCHDOG_PORT environment variable is not set. Continuing without watchdog." | Logger
}
else {
    $port = [int]$watchdogPort
    "sending UDP ping packets to 127.0.0.1:'$watchdogPort'..."  | Logger
    
}

# Create a UDP client
$udpClient = New-Object System.Net.Sockets.UdpClient

# Get the event name from the environment variable SHUTDOWN_EVENT
$shutdownEventName = $env:SHUTDOWN_EVENT
if ($shutdownEventName) {
    "Monitoring the Win32 event named '$shutdownEventName'..."  | Logger
    # Open the Win32 event
    try {
        $eventHandle = [System.Threading.EventWaitHandle]::OpenExisting($shutdownEventName)
    } catch {
        "Failed to open the event '$shutdownEventName'. Please ensure it exists. Exiting." | Logger
        exit 1
    }
}
else {
    "SHUTDOWN_EVENT environment variable not set, continuing forever."  | Logger
}

$interval = 1
 
while ($true) {
    if (-not $shutdownEventName) {
        Start-Sleep -Seconds $interval
    }
    # else wait for the event to be signaled, timeout after 10 seconds
    elseif ($eventHandle.WaitOne($interval * 1000)) {
        "Event '$shutdownEventName' was signaled. Exiting the loop." | Logger
        break
    }

    # Test case: increase interval so it gradually becomes too long so the watchdog restarts the service
    $interval += 1

    # Write to the log file
    "working hard" | Logger

    if ($watchdogSecret -and $watchdogPort) 
    {
        # Send the UDP packet
        $data = [System.Text.Encoding]::UTF8.GetBytes($watchdogSecret)
        $udpClient.Send($data, $data.Length, "127.0.0.1", $port) | Out-Null
    }
}

# Dispose the UDP client
$udpClient.Close()

"Script execution completed." | Logger