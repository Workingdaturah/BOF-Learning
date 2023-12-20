#ifndef UNICODE
#define UNICODE
#endif
#pragma comment(lib, "Netapi32.lib")

#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <Windows.h>
#include <lm.h>
#include "base\helpers.h"

/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the debug build.
 */
#ifdef _DEBUG
#include "base\mock.h"
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#endif

extern "C" {
#include "beacon.h"
    // Define the Dynamic Function Resolution declaration for the GetLastError function
    DFR(KERNEL32, GetLastError);
    // Map GetLastError to KERNEL32$GetLastError 
    //IMPORT NetSessionEnum
    DECLSPEC_IMPORT NET_API_STATUS WINAPI Netapi32$NetSessionEnum(LMSTR, LMSTR, LMSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD, LPDWORD);
    DECLSPEC_IMPORT VOID WINAPI Kernel32$Sleep(DWORD);

    void go(char* args, int len)
    {
        LPSESSION_INFO_10 pBuf = NULL;
        LPSESSION_INFO_10 pTmpBuf;
        DWORD dwLevel = 10;
        DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
        DWORD dwEntriesRead = 0;
        DWORD dwTotalEntries = 0;
        DWORD dwResumeHandle = 0;
        DWORD i;
        DWORD dwTotalCount = 0;
        //LPTSTR pszServerName = NULL;
        LPTSTR pszClientName = NULL;
        LPTSTR pszUserName = NULL;
        NET_API_STATUS nStatus;

        //HardCode localhost for now
        wchar_t* pszServerName = L"localhost";

        nStatus = Netapi32$NetSessionEnum(pszServerName, NULL, NULL, dwLevel, (LPBYTE)&pBuf, dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);

        //Just a little print
        Kernel32$Sleep(3000);
        // Check if NetSessionEnum succeeded
        if (nStatus == NERR_Success) {
            pTmpBuf = pBuf; // Fixed assignment

            //1. Put everything in abuffer
            formatp buffer;

            //2. Allocate Memory to hold the Data
            BeaconFormatAlloc(&buffer, 1024);

            // Loop through the entries just because.
            for (i = 0; i < dwEntriesRead; i++) {
                // Print the retrieved data.
                /*
                BeaconPrintf(CALLBACK_OUTPUT, "\n\tClient: %s\n", pTmpBuf->sesi10_cname);
                BeaconPrintf(CALLBACK_OUTPUT, "\tUser:   %s\n", pTmpBuf->sesi10_username);
                BeaconPrintf(CALLBACK_OUTPUT, "\tActive: %d\n", pTmpBuf->sesi10_time);
                BeaconPrintf(CALLBACK_OUTPUT, "\tIdle:   %d\n", pTmpBuf->sesi10_idle_time);
                */
                BeaconFormatPrintf(&buffer, "Querying Sessions\n");
                BeaconFormatPrintf(&buffer, "================================");
                BeaconFormatPrintf(&buffer, "\n\tClient: %ls\n", pTmpBuf->sesi10_cname,i);
                BeaconFormatPrintf(&buffer, "\tUser:   %ls\n", pTmpBuf->sesi10_username, i);
                BeaconFormatPrintf(&buffer, "\tActive: %d\n", pTmpBuf->sesi10_time, i);
                BeaconFormatPrintf(&buffer, "\tIdle:   %d\n", pTmpBuf->sesi10_idle_time, i);
                BeaconFormatPrintf(&buffer, "================================");
                pTmpBuf++;
                dwTotalCount++;
            }
            BeaconPrintf(CALLBACK_OUTPUT, "%s\n", BeaconFormatToString(&buffer, NULL));
            BeaconFormatFree(&buffer);
            //NetApiBufferFree(pBuf); // Free the buffer when you're done because it needs it I think
        }
        else {
            //wprintf(L"NetSessionEnum failed with error code: %d\n", nStatus);
        }

        return;
    }
}

// Define a main function for the bebug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[]) {
    // Run BOF's entrypoint
    // To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
    bof::runMocked<>(go);
    return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

TEST(BofTest, Test1) {
    std::vector<bof::output::OutputEntry> got =
        bof::runMocked<>(go);
    std::vector<bof::output::OutputEntry> expected = {
        {CALLBACK_OUTPUT, "System Directory: C:\\Windows\\system32"}
    };
    // It is possible to compare the OutputEntry vectors, like directly
    // ASSERT_EQ(expected, got);
    // However, in this case, we want to compare the output, ignoring the case.
    ASSERT_EQ(expected.size(), got.size());
    ASSERT_STRCASEEQ(expected[0].output.c_str(), got[0].output.c_str());
}
#endif