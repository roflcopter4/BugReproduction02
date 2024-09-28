#pragma comment(lib, "Onecore.lib")
#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

static void memRegionInfoTest(void);

int main(void)
{
    memRegionInfoTest();
}

static void dumpErrorMessage(DWORD errVal)
{
    wchar_t buf[512];
    DWORD res = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errVal, LANG_SYSTEM_DEFAULT, buf, 512, NULL
    );
    wprintf(L"Last error: %08lX: %ls", errVal, res == 0 ? L"Unknown\n" : buf);
}

/****************************************************************************************/

#ifdef _WIN64
# define PFMT "%012zX"
#else
# define PFMT "%08zX"
#endif

struct Fixed_WIN32_MEMORY_REGION_INFORMATION {
    PVOID AllocationBase;
    ULONG AllocationProtect;

    union {
        ULONG Flags;
        struct {
            ULONG Private        : 1;
            ULONG MappedDataFile : 1;
            ULONG MappedImage    : 1;
            ULONG MappedPageFile : 1;
            ULONG MappedPhysical : 1;
            ULONG DirectMapped   : 1;
            ULONG Reserved       : 26;
        };
    };

    SIZE_T RegionSize;
    SIZE_T CommitSize;

#ifndef _WIN64
    UINT32 unusedPadding[2];
#endif
};
typedef struct Fixed_WIN32_MEMORY_REGION_INFORMATION Fixed_WIN32_MEMORY_REGION_INFORMATION;

static DWORD doMemRegionInfoTest(_In_ LPVOID mem, _Out_writes_bytes_(dataSize) LPVOID data, _In_ SIZE_T dataSize)
{
    memset(data, 0xCC, dataSize);
    SetLastError(0);
    SIZE_T nWritten = 0;
    BOOL   bRes     = QueryVirtualMemoryInformation(GetCurrentProcess(), mem, MemoryRegionInfo, data, dataSize, &nWritten);
    DWORD  dwRes    = GetLastError();
    wprintf(L"QueryVirtualMemoryInformation for " PFMT L" returned %d and wrote %zu bytes.\n",
            (UINT_PTR)mem, bRes, nWritten);
    if (bRes) {
        WIN32_MEMORY_REGION_INFORMATION *realData = data;
        wprintf(PFMT L"\n%08X\n%08X\n%zu\n%zu\n", (UINT_PTR)realData->AllocationBase,
                realData->AllocationProtect, realData->Flags, realData->RegionSize, realData->CommitSize);
    }
    dumpErrorMessage(dwRes);
    return dwRes;
}

static void memRegionInfoTest(void)
{
    LPVOID mem = VirtualAlloc(NULL, 32768, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) {
        DWORD dwErr = GetLastError();
        fputws(L"Error allocating memory.\n", stdout);
        dumpErrorMessage(dwErr);
        exit((int)dwErr);
    }

    {
        fputws(L"<<<Official Windows structure>>>\n", stdout);
        WIN32_MEMORY_REGION_INFORMATION data;
        doMemRegionInfoTest(mem, &data, sizeof data);
    }
    {
        fputws(L"\n<<<Structure with extra padding>>>\n", stdout);
        Fixed_WIN32_MEMORY_REGION_INFORMATION data;
        DWORD dwRes = doMemRegionInfoTest(mem, &data, sizeof data);
#ifndef _WIN64
        if (dwRes == 0) {
            for (unsigned i = 0; i < ARRAYSIZE(data.unusedPadding); ++i)
                wprintf(L"%08zX ", data.unusedPadding[i]);
            fputwc(L'\n', stdout);
        }
#endif
    }

    VirtualFree(mem, 0, MEM_RELEASE);
}

