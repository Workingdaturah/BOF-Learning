#include <stdio.h>
#include <Windows.h>
#include <Iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")
#include <lm.h>

#pragma comment(lib, "Netapi32.lib")

#include "base\helpers.h"

/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the the debug build.
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
    #define GetLastError KERNEL32$GetLastError 

    //IMPORTS
    DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegOpenKeyExA(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
    DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegQueryValueExA(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegCloseKey(HKEY);
    DECLSPEC_IMPORT BOOL WINAPI KERNEL32$GetComputerNameA(LPSTR, LPDWORD);
    DECLSPEC_IMPORT VOID WINAPI KERNEL32$GetNativeSystemInfo(LPSYSTEM_INFO);

    //GPA
    DECLSPEC_IMPORT FARPROC WINAPI KERNEL32$GetProcAddress(HMODULE, LPCSTR);
    DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$GetModuleHandleW(LPCWSTR);

    //Network API
    DECLSPEC_IMPORT ULONG WINAPI IPHLPAPI$GetAdaptersInfo(PIP_ADAPTER_INFO, PULONG);

    //Domain Grab
    DECLSPEC_IMPORT NET_API_STATUS WINAPI NETAPI32$NetWkstaGetInfo(LPWSTR, DWORD, LPBYTE);

    void go(char* args, int len) 
    {
        DWORD i;
        //Get Hostname
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);

        if (KERNEL32$GetComputerNameA(computerName, &size)) {
            //BeaconPrintf(CALLBACK_OUTPUT,"Host Name: %s\n", computerName);
        }
        else {
            BeaconPrintf(CALLBACK_OUTPUT,"Failed to retrieve the computer name. Error code: %d\n", KERNEL32$GetLastError());
        }
        //Get OS Version using Registry Keys
        HKEY hKey;
        LONG regOpenResult;
        DWORD keyType, dataSize;
        char value[1024];
        char registerdOwner[1024];

        // Query the "OS Name" registry key
        regOpenResult = ADVAPI32$RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);

        dataSize = sizeof(value);

        regOpenResult = ADVAPI32$RegQueryValueExA(hKey, "ProductName", NULL, &keyType, (LPBYTE)value, &dataSize);

        if (keyType == REG_SZ) {
            //BeaconPrintf(CALLBACK_OUTPUT, "OS Name: %s\n", value);
        }
        else {
            BeaconPrintf(CALLBACK_OUTPUT, "Registry Value is not a string.\n");
        }

        ADVAPI32$RegCloseKey(hKey);

        // Query the Registered Owner registry key
        regOpenResult = ADVAPI32$RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);

        dataSize = sizeof(registerdOwner);

        regOpenResult = ADVAPI32$RegQueryValueExA(hKey, "RegisteredOwner", NULL, &keyType, (LPBYTE)registerdOwner, &dataSize);

        if (keyType == REG_SZ) {
            //BeaconPrintf(CALLBACK_OUTPUT, "Registered Owner: %s\n", value);
        }
        else {
            BeaconPrintf(CALLBACK_OUTPUT, "Registry Value is not a string.\n");
        }

        ADVAPI32$RegCloseKey(hKey);

        //Get OS Version
        typedef NTSTATUS(WINAPI* RTLGETVERSION)(PRTL_OSVERSIONINFOW);
        RTL_OSVERSIONINFOW osvi;
        osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

        HMODULE hMod = KERNEL32$GetModuleHandleW(L"ntdll.dll");
        RTLGETVERSION pRtlGetVersion = (RTLGETVERSION)KERNEL32$GetProcAddress(hMod, "RtlGetVersion");
        pRtlGetVersion(&osvi);
       //BeaconPrintf(CALLBACK_OUTPUT, "Operating System Version: %d.%d.%d\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

        //Processors
        SYSTEM_INFO systemInfo;
        KERNEL32$GetNativeSystemInfo(&systemInfo);

        //Grab Domain Name
        LPWKSTA_INFO_100 pWorkstationInfo;

        NETAPI32$NetWkstaGetInfo(NULL, 100, (BYTE*)&pWorkstationInfo);

        //BeaconPrintf(CALLBACK_OUTPUT, "Domain Name: %ls\n", pWorkstationInfo ? pWorkstationInfo->wki100_langroup : L"Unknown");

        //NetApiBufferFree(pWorkstationInfo);

         //1. Put everything in abuffer
        formatp buffer;

        //2. Allocate Memory to hold the Data
        BeaconFormatAlloc(&buffer, 1024);

        BeaconFormatPrintf(&buffer, "Querying OS Information\n");
        BeaconFormatPrintf(&buffer, "================================\n");
        BeaconFormatPrintf(&buffer, "Host Name: %s\t\n", computerName, i);
        BeaconFormatPrintf(&buffer, "OS Name: %s\t\n",value, i);
        BeaconFormatPrintf(&buffer, "Registered Owner: %s\t\n", value, i);
        BeaconFormatPrintf(&buffer, "Operating System Version: %d.%d.%d\t\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, systemInfo.dwNumberOfProcessors, i);
        BeaconFormatPrintf(&buffer, "================================");
        BeaconPrintf(CALLBACK_OUTPUT, "%s\n", BeaconFormatToString(&buffer, NULL));
        BeaconFormatFree(&buffer);

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