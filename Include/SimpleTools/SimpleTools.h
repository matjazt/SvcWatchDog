/*
 * MIT License
 *
 * Copyright (c) 2025 Matjaž Terpin (mt.dev@gmx.com)
 *
 * Permission is hereby granted, free of charge, ... (standard MIT license).
 */

#ifndef _SIMPLETOOLS_H_
#define _SIMPLETOOLS_H_

#include <string>
#include <filesystem>
#include <mutex>

using namespace std;

#define SAFE_STRING_COPY(t, s)                                                                              \
    do                                                                                                      \
    {                                                                                                       \
        static_assert(std::is_array<decltype(t)>::value, "Error: target must be an array, not a pointer!"); \
        strncpy((t), (s), sizeof(t) - 1);                                                                   \
        (t)[sizeof(t) - 1] = 0;                                                                             \
    } while (0)

#define AUTO_TERMINATE(s)                                                                                      \
    do                                                                                                         \
    {                                                                                                          \
        static_assert(std::is_array<decltype(s)>::value, "Error: parameter must be an array, not a pointer!"); \
        s[sizeof(s) - 1] = 0;                                                                                  \
    } while (0)

#define SAFE_DELETE(x)   \
    do                   \
    {                    \
        if (x)           \
        {                \
            delete x;    \
            x = nullptr; \
        }                \
    } while (0)

#define SAFE_DELETE_ARRAY(x) \
    do                       \
    {                        \
        if (x)               \
        {                    \
            delete[] x;      \
            x = nullptr;     \
        }                    \
    } while (0)

#define SAFE_FREE(x)     \
    do                   \
    {                    \
        if (x)           \
        {                \
            free(x);     \
            x = nullptr; \
        }                \
    } while (0)

#define SAFE_CLOSE_SOCKET(s)         \
    do                               \
    {                                \
        if ((s) != (INVALID_SOCKET)) \
        {                            \
            closesocket(s);          \
            s = INVALID_SOCKET;      \
        }                            \
    } while (0)

#define EMPTYIFNULL(s) ((s) ? (s) : "")
#define NULLOREMPTY(s) ((s) == nullptr || *(s) == 0)

string LoadTextFile(const filesystem::path& filePath);

void GetCurrentLocalTime(struct tm*& localTime, int& milliseconds) noexcept;

uint64_t SteadyTime() noexcept;
#define SLEEP(MILLISECONDS) std::this_thread::sleep_for(std::chrono::milliseconds(MILLISECONDS))

string GetExecutableName();
string GetHostname();

class NoCopy
{
   public:
    NoCopy() noexcept {}

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

template <typename T>
string NumberFormat(T num, const char* formatString);
#define FLOAT2(f) NumberFormat((f), "%3.2lf")
#define FLOATFULL(f) NumberFormat((f), "%3.17lf")
#define BOOL2STR(b) ((b) ? "true" : "false")

vector<string> Split(const string& str, char delimiter);
string JoinStrings(const vector<string>& words, const string& delimiter);
string TrimEx(const string& str, const string& leftTrimChars, const string& rightTrimChars);
string Trim(const string& str, const string& trimChars = " \t\n\r\f\v");
string TrimLeft(const string& str, const string& trimChars = " \t\n\r\f\v");
string TrimRight(const string& str, const string& trimChars = " \t\n\r\f\v");

#ifdef WIN32

#define SAFE_CLOSE_HANDLE(h) \
    do                       \
    {                        \
        if (h)               \
        {                    \
            CloseHandle(h);  \
            h = nullptr;     \
        }                    \
    } while (0)

#define O_SYNC 0
#define strcasecmp stricmp

#endif

#ifdef LINUX

#define UNIX

// #include <unistd.h>
// #include <pthread.h>
// #include <errno.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close

#define _stat stat

#define __stdcall
#define Sleep(ms) usleep((unsigned long)(ms) * 1000ul)

#define O_BINARY 0

#define stricmp strcasecmp

#define _MAX_PATH 256
#define MAX_PATH 256

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define _chdir(d) chdir(d)

int GetModuleFileName(void* pDummy, char* achBuf, int iBufLen);

// #include <assert.h>

#define _ftime ftime
#define _timeb struct timeb

#endif

/**
 * @class SyncEvent
 * @brief A thread synchronization primitive that mimics the behavior of Win32 event objects.
 * It allows threads to wait for or signal events.
 *
 * SyncEvent provides a mechanism for threads to wait until an event is signaled.
 * It supports both manual-reset and auto-reset modes:
 * - In manual-reset mode, the event remains signaled until explicitly reset.
 * - In auto-reset mode, the event automatically resets after releasing a single waiting thread.
 *
 * Typical usage:
 * - One or more threads call WaitForSingleEvent() to wait for the event to be signaled.
 * - Another thread calls SetEvent() to signal the event, releasing waiting threads.
 * - ResetEvent() can be used to reset the event to the non-signaled state (manual-reset mode).
 *
 * @note This class is thread-safe.
 */
class SyncEvent : public NoCopy
{
   public:
    /**
     * @brief Constructs a SyncEvent.
     * @param initialState Initial signaled state of the event.
     * @param autoReset If true, event is auto-reset; otherwise, manual-reset.
     */
    SyncEvent(bool initialState, bool autoReset);

    /**
     * @brief Sets the event to the signaled state.
     *        Wakes one or all waiting threads depending on auto-reset mode.
     * @return true if the event was not signaled before the call, false if it was already signaled.
     */
    bool SetEvent();

    /**
     * @brief Resets the event to the non-signaled state.
     * @return true if the event was signaled before the call, false if it was already non-signaled.
     */
    bool ResetEvent();

    /**
     * @brief Waits indefinitely for the event to become signaled.
     */
    void WaitForSingleEvent();

    /**
     * @brief Waits for the event to become signaled or until timeout.
     * @param milliseconds Timeout in milliseconds.
     * @return true if the event was signaled, false if timeout occurred.
     */
    bool WaitForSingleEvent(int milliseconds);

   private:
    bool m_autoReset;
    mutex m_mtx;
    condition_variable m_cv;
    bool m_signaled;
};

#endif