// eject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <Shldisp.h>
#include <strsafe.h>

int ejectCD(const TCHAR* drvStr)
{
    TCHAR drvPath[8] = _T("\\\\.\\");
    _tcsncat(drvPath, drvStr, 3);
    DWORD dwBytes;
    HANDLE hCdRom = CreateFile(drvPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCdRom == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Error: %x"), GetLastError());
        return 1;
    }

    // Open the door:  
    DeviceIoControl(hCdRom, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    Sleep(1000);

    // Close the door:  
    DeviceIoControl(hCdRom, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    CloseHandle(hCdRom);

    return 0;
}

int ejectDrive(const TCHAR* drvStr)
{
    TCHAR drvPath[8] = _T("\\\\.\\");
    _tcsncat(drvPath, drvStr, 3);
    DWORD dwBytes;
    HANDLE hCdRom = CreateFile(drvPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCdRom == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Error: %x"), GetLastError());
        return 1;
    }

    // Open the door:  
    DeviceIoControl(hCdRom, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    Sleep(1000);

    // Close the door:  
    DeviceIoControl(hCdRom, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    CloseHandle(hCdRom);

    return 0;
}

int _tmain()
{
    ejectCD(_T("G:"));
    return 0;
}