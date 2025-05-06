#ifndef _SIMPLETOOLS_H_
#define _SIMPLETOOLS_H_

#include <string>
#include <filesystem>

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
        if (x) delete x; \
        x = nullptr;     \
    } while (0)

#define SAFE_FREE(x)    \
    do                  \
    {                   \
        if (x) free(x); \
        x = nullptr;    \
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

string loadTextFile(const filesystem::path& filePath);

void GetCurrentLocalTime(struct tm*& localTime, int& milliseconds);

uint64_t SteadyTime();

class NoCopy
{
   public:
    NoCopy() {}

    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

template <typename T>
string NumberFormat(T num, const char* formatString);
#define FLOAT2(f) NumberFormat((f), "%3.2lf")
#define FLOATFULL(f) NumberFormat((f), "%3.17lf")
#define BOOL2STR(b) ((b) ? "true" : "false")

vector<string> split(const string& str, char delimiter);

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

// memory debug tools
extern _CrtMemState g_memChkPoint1;
extern _CrtMemState g_memChkPoint2;
extern _CrtMemState g_memDiff;
#define DbgMemCheckPoint1() _CrtMemCheckpoint(&g_memChkPoint1)
#define DbgMemCheckPoint2() _CrtMemCheckpoint(&g_memChkPoint2)
#define DbgMemCompareChkPoints()                                         \
    do                                                                   \
    {                                                                    \
        _CrtMemDifference(&g_memDiff, &g_memChkPoint1, &g_memChkPoint2); \
        _CrtMemDumpStatistics(&g_memDiff);                               \
        _CrtDumpMemoryLeaks();                                           \
    } while (0)

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

// memory debug
#define DbgMemCheckPoint1()
#define DbgMemCheckPoint2()
#define DbgMemCompareChkPoints()

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define _chdir(d) chdir(d)

int GetModuleFileName(void* pDummy, char* achBuf, int iBufLen);

// #include <assert.h>

#define _ftime ftime
#define _timeb struct timeb

#endif

#endif