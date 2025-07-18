#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>

typedef DWORD (WINAPI * MessagePtr)(UINT, UINT, DWORD, DWORD, DWORD);

#ifndef MMNOMIDI

typedef struct midiopenstrmid_tag
{
    DWORD dwStreamID;
    UINT uDeviceID;
} MIDIOPENSTRMID;

typedef struct midiopendesc_tag {
    HMIDI hMidi;
    DWORD_PTR dwCallback;
    DWORD_PTR dwInstance;
    DWORD_PTR dnDevNode;
    DWORD cIds;
    MIDIOPENSTRMID rgIds[1];
} MIDIOPENDESC;

typedef MIDIOPENDESC FAR *LPMIDIOPENDESC;

#endif // MMNOMIDI

#ifndef MODM_GETDEVCAPS
#define MODM_GETDEVCAPS     2
#endif

#ifndef MODM_OPEN
#define MODM_OPEN           3
#endif

#ifndef MODM_CLOSE
#define MODM_CLOSE          4
#endif

LONG testDriver()
{
    HDRVR hdrvr;
    DRVCONFIGINFO dci;
    LONG lRes;
    HMODULE lib;
    DWORD modRet;
    MIDIOUTCAPSA myCapsA;
    MessagePtr modMessagePtr;
    MIDIOPENDESC desc;
    LONG totalClientNum = 0;

    printf("Open opnmididrv.dll...\n");
    // Open the driver (no additional parameters needed this time).
    if((hdrvr = OpenDriver(L".\\opnmididrv.dll", 0, 0)) == 0)
    {
        printf("!!! Can't open the driver\n");
        return -1;
    }

    printf("Send DRV_QUERYCONFIGURE...\n");
    // Make sure driver has a configuration dialog box.
    if(SendDriverMessage(hdrvr, DRV_QUERYCONFIGURE, 0, 0) != 0)
    {
        // Set the DRVCONFIGINFO structure and send the message
        dci.dwDCISize = sizeof (dci);
        dci.lpszDCISectionName = (LPWSTR)0;
        dci.lpszDCIAliasName = (LPWSTR)0;
        printf("Send DRV_CONFIGURE...\n");
        lRes = SendDriverMessage(hdrvr, DRV_CONFIGURE, 0, (LPARAM)&dci);
        printf("<-- Got answer: %ld)\n", lRes);
    }
    else
    {
        printf("<-- No configure\n");
        lRes = DRVCNF_OK;
    }


    printf("Getting library pointer\n");
    if((lib = GetDriverModuleHandle(hdrvr)))
    {
        printf("Getting modMessage call\n");
        modMessagePtr = (MessagePtr)GetProcAddress(lib, "modMessage");
        if(!modMessagePtr)
        {
            CloseDriver(hdrvr, 0, 0);
            printf("!!! modMessage not found!\n");
            return -1;
        }

        printf("Getting capabilities...\n");
        modRet = modMessagePtr(0, MODM_GETDEVCAPS, (DWORD_PTR)NULL, (DWORD_PTR)&myCapsA, sizeof(myCapsA));
        if(modRet != MMSYSERR_NOERROR)
        {
            CloseDriver(hdrvr, 0, 0);
            printf("!!! modMessage returned an error!\n");
            return -1;
        }

        printf("<-- %s\n", myCapsA.szPname);

        printf("Trying to open driver\n");
        ZeroMemory(&desc, sizeof(desc));
        desc.dwInstance = (DWORD_PTR)hdrvr;
        modRet = modMessagePtr(0, MODM_OPEN, (DWORD_PTR)&totalClientNum, (DWORD_PTR)&desc, sizeof(desc));
        if(modRet != MMSYSERR_NOERROR)
        {
            CloseDriver(hdrvr, 0, 0);
            printf("!!! modMessage returned an error!\n");
            return -1;
        }

        printf("<-- Client Number: %ld\n", totalClientNum);

        printf("Trying to close\n");
        modRet = modMessagePtr(0, MODM_CLOSE, (DWORD_PTR)NULL, (DWORD_PTR)NULL, 0);
        if(modRet != MMSYSERR_NOERROR)
        {
            CloseDriver(hdrvr, 0, 0);
            printf("!!! modMessage returned an error!\n");
            return -1;
        }
    }
    else
    {
        CloseDriver(hdrvr, 0, 0);
        printf("!!! Error when getting module handler!\n");
        return -1;
    }

    printf("Close...\n");
    // Close the driver (no additional parameters needed this time).
    if(FAILED(CloseDriver(hdrvr, 0, 0)))
    {
        printf("!!! Error when closing\n");
        return -1;
    }

    printf("Return...\n");

    return 0;
}

int main()
{
    LONG d = testDriver();

    if(d == 0)
    {
        printf("TEST = OK\n");
        return 0;
    }
    else
    {
        printf("TEST = FAILED\n");
        return 1;
    }
}
