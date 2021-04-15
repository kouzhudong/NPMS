// MailSlotServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// https://docs.microsoft.com/zh-cn/windows/win32/ipc/reading-from-a-mailslot


/*
The process that creates a mailslot can read messages from it by using the mailslot handle in a call to the ReadFile function. 
The following example calls the GetMailslotInfo function to determine whether there are messages in the mailslot. 
If messages are waiting, each is displayed along with the number of messages remaining to be read.

A mailslot exists until the CloseHandle function is called for all open server handles or until all server processes that own a mailslot handle exit. 
In both cases, any unread messages are deleted from the mailslot, all client handles to the mailslot are closed, and the mailslot itself is deleted from memory.
*/

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

HANDLE hSlot;
LPCTSTR SlotName = TEXT("\\\\.\\mailslot\\sample_mailslot");

BOOL ReadSlot()
{
    DWORD cbMessage, cMessage, cbRead;
    BOOL fResult;
    LPTSTR lpszBuffer;
    TCHAR achID[80];
    DWORD cAllMessages;
    HANDLE hEvent;
    OVERLAPPED ov;

    cbMessage = cMessage = cbRead = 0;

    hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("ExampleSlot"));
    if (NULL == hEvent)
        return FALSE;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    ov.hEvent = hEvent;

    fResult = GetMailslotInfo(hSlot, // mailslot handle 
                              (LPDWORD)NULL,               // no maximum message size 
                              &cbMessage,                   // size of next message 
                              &cMessage,                    // number of messages 
                              (LPDWORD)NULL);              // no read time-out 

    if (!fResult) {
        printf("GetMailslotInfo failed with %d.\n", GetLastError());
        return FALSE;
    }

    if (cbMessage == MAILSLOT_NO_MESSAGE) {
        printf("Waiting for a message...\n");
        return TRUE;
    }

    cAllMessages = cMessage;

    while (cMessage != 0)  // retrieve all messages
    {
        // Create a message-number string. 

        StringCchPrintf((LPTSTR)achID,
                        80,
                        TEXT("\nMessage #%d of %d\n"),
                        cAllMessages - cMessage + 1,
                        cAllMessages);

        // Allocate memory for the message. 

        lpszBuffer = (LPTSTR)GlobalAlloc(GPTR,
                                         lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage);
        if (NULL == lpszBuffer)
            return FALSE;
        lpszBuffer[0] = '\0';

        fResult = ReadFile(hSlot,
                           lpszBuffer,
                           cbMessage,
                           &cbRead,
                           &ov);

        if (!fResult) {
            printf("ReadFile failed with %d.\n", GetLastError());
            GlobalFree((HGLOBAL)lpszBuffer);
            return FALSE;
        }

        // Concatenate the message and the message-number string. 

        StringCbCat(lpszBuffer,
                    lstrlen((LPTSTR)achID) * sizeof(TCHAR) + cbMessage,
                    (LPTSTR)achID);

        // Display the message. 

        _tprintf(TEXT("Contents of the mailslot: %s\n"), lpszBuffer);

        GlobalFree((HGLOBAL)lpszBuffer);

        fResult = GetMailslotInfo(hSlot,  // mailslot handle 
                                  (LPDWORD)NULL,               // no maximum message size 
                                  &cbMessage,                   // size of next message 
                                  &cMessage,                    // number of messages 
                                  (LPDWORD)NULL);              // no read time-out 

        if (!fResult) {
            printf("GetMailslotInfo failed (%d)\n", GetLastError());
            return FALSE;
        }
    }
    CloseHandle(hEvent);
    return TRUE;
}

BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{
    hSlot = CreateMailslot(lpszSlotName,
                           0,                             // no maximum message size 
                           MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
                           (LPSECURITY_ATTRIBUTES)NULL); // default security

    if (hSlot == INVALID_HANDLE_VALUE) {
        printf("CreateMailslot failed with %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

int main()
{
    MakeSlot(SlotName);

    while (TRUE) {
        ReadSlot();
        Sleep(3000);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//https://docs.microsoft.com/zh-cn/windows/win32/ipc/creating-a-mailslot?redirectedfrom=MSDN

/*
Mailslots are supported by three specialized functions: CreateMailslot, GetMailslotInfo, and SetMailslotInfo. 
These functions are used by the mailslot server.

The following code sample uses the CreateMailslot function to retrieve the handle to a mailslot named "sample_mailslot". 
The code sample in Writing to a Mailslot shows how client application can write to this mailslot.

To create a mailslot that can be inherited by child processes, 
an application should change the SECURITY_ATTRIBUTES structure passed as the last parameter of CreateMailslot. 
To do this, the application sets the bInheritHandle member of this structure to TRUE (the default setting is FALSE).
*/

#if 0

#include <windows.h>
#include <stdio.h>

HANDLE hSlot;
LPCTSTR SlotName = TEXT("\\\\.\\mailslot\\sample_mailslot");

BOOL WINAPI MakeSlot(LPCTSTR lpszSlotName)
{
    hSlot = CreateMailslot(lpszSlotName,
                           0,                             // no maximum message size 
                           MAILSLOT_WAIT_FOREVER,         // no time-out for operations 
                           (LPSECURITY_ATTRIBUTES)NULL); // default security

    if (hSlot == INVALID_HANDLE_VALUE) {
        printf("CreateMailslot failed with %d\n", GetLastError());
        return FALSE;
    } else printf("Mailslot created successfully.\n");
    return TRUE;
}

void main()
{
    MakeSlot(SlotName);
}

#endif // 0


//////////////////////////////////////////////////////////////////////////////////////////////////
