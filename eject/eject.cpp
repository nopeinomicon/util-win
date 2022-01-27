// eject.cpp : This file contains the 'eject' application for util-win
//

#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS

#include "../common.h"
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <ntddscsi.h>
#include <shlwapi.h>

#define MAX_SENSE_LEN 18 //Sense data max length

// Structure to store SCSI pass-through data and sense data
typedef struct _SCSI_PASS_THROUGH_DIRECT_AND_SENSE_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    UCHAR SenseBuffer[MAX_SENSE_LEN];
}T_SPDT_SBUF;

/*
A command to detect and return the ejection status of an optical disc drive drive
(though it could feasably be used on any Windows device that has some kind of removable media)
 

Created referencing https://ws0.org/windows-how-to-get-the-tray-status-of-the-optical-drive/
*/
int GetEjectStatus(TCHAR* drvPath, bool verbose)
{
    /* 
    This first section will attempt to detect if the drive has media, the reason for this being
    that if the OS can detect media on the drive, it must be inserted. This is a faster method before
    resorting to the more complex but foolproof SCSI command check
    */

    if (verbose)
        _tprintf_s(_T("eject: Checking drive ejection status for toggle command..."));
    
    DWORD dwBytes;
    HANDLE hDevice = CreateFile(drvPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL); // Create a handle to access the drive
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        if (verbose)
            _tprintf(_T("Error: %x"), GetLastError());
        return -1;
    }

    int hasMedia = DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, &dwBytes, NULL);

    // Close device for now
    CloseHandle(hDevice);

    if (hasMedia == 1) // If the IO command returned a 1 then the drive has media and can be assumed to be closed
        return 0;

    /*
    This section runs if the drive is found not to have media. This section directly passes an SCSI command
    and reads the returned data to determine if the drive is open or closed.
    */

    T_SPDT_SBUF passthroughData;  //SCSI Pass Through Direct variable.
    byte DataBuf[8];  //Buffer for holding data from drive.

    hDevice = CreateFile(drvPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_READONLY,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("System Error: %x"), GetLastError());
        return -1;
    }

    passthroughData.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    passthroughData.sptd.PathId = 0;
    passthroughData.sptd.TargetId = 0;
    passthroughData.sptd.Lun = 0;
    passthroughData.sptd.CdbLength = 10;
    passthroughData.sptd.SenseInfoLength = MAX_SENSE_LEN;
    passthroughData.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    passthroughData.sptd.DataTransferLength = sizeof(DataBuf);
    passthroughData.sptd.TimeOutValue = 2;
    passthroughData.sptd.DataBuffer = (PVOID) & (DataBuf);
    passthroughData.sptd.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);

    passthroughData.sptd.Cdb[0] = 0x4a;
    passthroughData.sptd.Cdb[1] = 1;
    passthroughData.sptd.Cdb[2] = 0;
    passthroughData.sptd.Cdb[3] = 0;
    passthroughData.sptd.Cdb[4] = 0x10;
    passthroughData.sptd.Cdb[5] = 0;
    passthroughData.sptd.Cdb[6] = 0;
    passthroughData.sptd.Cdb[7] = 0;
    passthroughData.sptd.Cdb[8] = 8;
    passthroughData.sptd.Cdb[9] = 0;
    passthroughData.sptd.Cdb[10] = 0;
    passthroughData.sptd.Cdb[11] = 0;
    passthroughData.sptd.Cdb[12] = 0;
    passthroughData.sptd.Cdb[13] = 0;
    passthroughData.sptd.Cdb[14] = 0;
    passthroughData.sptd.Cdb[15] = 0;

    ZeroMemory(DataBuf, 8);
    ZeroMemory(passthroughData.SenseBuffer, MAX_SENSE_LEN);

    //Send the command to drive
    int status = DeviceIoControl((HANDLE)hDevice,
        IOCTL_SCSI_PASS_THROUGH_DIRECT,
        (PVOID)&passthroughData, (DWORD)sizeof(passthroughData),
        (PVOID)&passthroughData, (DWORD)sizeof(passthroughData),
        &dwBytes,
        NULL);

    // Close the device
    CloseHandle(hDevice);

    if (status)
    {
        if (DataBuf[5] == 0) // Tray closed
            return 0;
        else if (DataBuf[5] >= 1) // Tray open
            return 1;
    }
    return -1; // If no other return option was reached, we were unable to determine the status of the drive
}

int ReloadDrive(const TCHAR* drvPath)
{
    DWORD dwBytes;
    HANDLE hDevice = CreateFile(drvPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("System Error: %x"), GetLastError());
        return -1;
    }

    // Close the door:  
    DeviceIoControl(hDevice, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    CloseHandle(hDevice);

    return 0;
}

int EjectDrive(const TCHAR* drvPath)
{
    DWORD dwBytes;
    HANDLE hDevice = CreateFile(drvPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("System Error: %x"), GetLastError());
        return -1;
    }

    // Eject device
    DeviceIoControl(hDevice, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwBytes, NULL);

    // Wrap up
    CloseHandle(hDevice);
    return 0;
}

enum EjectionMode {Eject, Reload, Toggle};

/* 
Function to return the name of the executable file.
(to be used in help and about output)
*/
TCHAR* GetExecName()
{
    TCHAR* fileName = new TCHAR[MAX_PATH];
    GetModuleFileName(NULL, fileName, MAX_PATH);
    PathStripPath(fileName);
    return fileName;
}

/*
Prints the short version of the command help.
(to be used when user enters command incorrectly, without specifiying help argument)
*/
void PrintShortHelp()
{
    TCHAR* execName = GetExecName();
    _tprintf_s(_T("Usage: %s [OPTIONS...] [DRIVE LETTER]\n"), execName);
    _tprintf_s(_T("Try \'%s -h\' for more information.\n"), execName);
}

void PrintLongHelp() 
{
    TCHAR* execName = GetExecName();
    _tprintf_s(_T("Usage: %s [OPTIONS...] [DRIVE LETTER]\n"), execName);
    _tprintf_s(_T("The drive letter should be formatted like \'C:\', \'D:\', etc.\n\n"));
    _tprintf_s(_T("Eject removable media.\n\n"));
    _tprintf_s(_T("Options:\n"));
    _tprintf_s(_T("-h, --help\t\t Displays this help information\n"));
    _tprintf_s(_T("-t, --trayclose\t\t Closes the tray of the specified drive (unsupported on non-optical drives)\n"));
    _tprintf_s(_T("-T, --traytoggle\t Toggles the tray of the specified drive (unsupported on non-optical drives)\n"));
    _tprintf_s(_T("-v, --verbose\t\t Enable verbose command output\n"));
    _tprintf_s(_T("-V, --version\t\t Displays version information about this program\n"));
}

void PrintVersionInfo()
{
    TCHAR* execName = GetExecName();
    _tprintf_s(_T("%s from util-win %i.%i.%i %s\n"), execName, UTIL_WIN_VERSION, UTIL_WIN_RELEASE, UTIL_WIN_REVISION, UTIL_WIN_BUILDTYPE);
}


int _tmain(int argc, TCHAR* argv[])
{
    // Initialize variables to store command parameters as specified by the user
    EjectionMode mode = Eject;
    bool verbose = false;
    TCHAR drvPath[8] = _T("\\\\.\\");
    bool longHelp = false;
    bool versionInfo = false;
    bool invalidParam = false;

    if (argc == 1) // Check if no arguments are given
    {
        PrintShortHelp();
        return -1;
    }
    
    // Itterate through arguments, parse, and check for errors
    for (int i=1; i < argc; i++)
    {
        if (StrCmp(argv[i], _T("--verbose")) == 0) // Check for verbose option
            verbose = true;
        else if (StrCmp(argv[i], _T("--traytoggle")) == 0) // Check for toggle option
            mode = Toggle;
        else if (StrCmp(argv[i], _T("--trayclose")) == 0) // Check for reload option
            mode = Reload;
        else if (StrCmp(argv[i], _T("--help")) == 0) // Check for help option
            longHelp = true;
        else if (StrCmp(argv[i], _T("--version")) == 0) // Check for version option
            versionInfo = true;
        else if (StrCmp(argv[i], _T("-v")) == 0) // Check for short verbose option
            verbose = true;
        else if (StrCmp(argv[i], _T("-T")) == 0) // Check for short toggle option
            mode = Toggle;
        else if (StrCmp(argv[i], _T("-t")) == 0) // Check for short reload option
            mode = Reload;
        else if (StrCmp(argv[i], _T("-h")) == 0) // Check for short help option
            longHelp = true;
        else if (StrCmp(argv[i], _T("-V")) == 0) // Check for short version option
            versionInfo = true;
        else if (lstrlen(argv[i]) == 2 || StrStr(argv[i], _T(":")) == &argv[argc - 1][1]) // Check if argument is a drive letter 
                                                                                                        // with proper formatting
        {
            // Mutate original drive letter into proper path form for IO commands, then store
            _tcsncat(drvPath, argv[i], 3);
        }
        else // If no other options worked then the parameter is invalid
        {
            _tprintf_s(_T("eject: invalid argument -- \'%s\'\n"), argv[i]);
            printf_s("eject: If you meant this to be your drive letter, note that it must be formatted like \'C:\', \'D:\', etc.\n\n");
            invalidParam = true;
        }
    }

    // If the help option is specified, just show help, don't execute
    if (longHelp)
    {
        PrintLongHelp();
        return 0;
    }

    // If the version option is specified, just show version info, don't execute
    if (versionInfo)
    {
        PrintVersionInfo();
        return 0;
    }

    // If there is no drive specified, show error
    if (StrCmp(drvPath, _T("\\\\.\\")) == 0)
        _tprintf_s(_T("eject: No drive specified!\n\n"));

    // If there was an invalid parameter or no drive specified, show help text and abort
    if (invalidParam || StrCmp(drvPath, _T("\\\\.\\")) == 0)
    {
        if (longHelp)
            PrintLongHelp();
        else
            PrintShortHelp();
        return -1;
    }

    int result = -1; // Return of main ejection function

    // Check eject mode and run needed functions
    switch (mode)
    {
    case Eject:
        result = EjectDrive(drvPath);
        break;
    case Reload:
        result = ReloadDrive(drvPath);
        break;
    case Toggle:
        switch (GetEjectStatus(drvPath, verbose))
        {
        case -1:
            _tprintf_s(_T("There was an error in getting the drive status!"));
            break;
        case 0:
            result = EjectDrive(drvPath);
            break;
        case 1:
            result = ReloadDrive(drvPath);
            break;
        }
        break;
    default:
        break;
    }
    return result;
}