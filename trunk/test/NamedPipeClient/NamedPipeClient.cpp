﻿// NamedPipeClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// https://docs.microsoft.com/zh-cn/windows/win32/ipc/named-pipe-client?redirectedfrom=MSDN

/*
A named pipe client uses the CreateFile function to open a handle to a named pipe.
If the pipe exists but all of its instances are busy, CreateFile returns INVALID_HANDLE_VALUE and the GetLastError function returns ERROR_PIPE_BUSY.
When this happens, the named pipe client uses the WaitNamedPipe function to wait for an instance of the named pipe to become available.

The CreateFile function fails if the access specified is incompatible with the access specified (duplex, outbound, or inbound) when the server created the pipe.
For a duplex pipe, the client can specify read, write, or read/write access;
for an outbound pipe (write-only server), the client must specify read-only access;
and for an inbound pipe (read-only server), the client must specify write-only access.

The handle returned by CreateFile defaults to byte-read mode, blocking-wait mode, overlapped mode disabled, and write-through mode disabled.
The pipe client can use CreateFile to enable overlapped mode by specifying FILE_FLAG_OVERLAPPED or to enable write-through mode by specifying FILE_FLAG_WRITE_THROUGH.
The client can use the SetNamedPipeHandleState function to enable nonblocking mode by specifying PIPE_NOWAIT or to enable message-read mode by specifying PIPE_READMODE_MESSAGE.

The following example shows a pipe client that opens a named pipe, sets the pipe handle to message-read mode,
uses the WriteFile function to send a request to the server, and uses the ReadFile function to read the server's reply.
This pipe client can be used with any of the message-type servers listed at the bottom of this topic.
With a byte-type server, however, this pipe client fails when it calls SetNamedPipeHandleState to change to message-read mode.
Because the client is reading from the pipe in message-read mode, it is possible for the ReadFile operation to return zero after reading a partial message.
This happens when the message is larger than the read buffer.
In this situation, GetLastError returns ERROR_MORE_DATA, and the client can read the remainder of the message using additional calls to ReadFile.

This pipe client can be used with any of the pipe servers listed under See Also.
*/

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFSIZE 512

int _tmain(int argc, TCHAR * argv[])
{
    HANDLE hPipe;
    LPCTSTR lpvMessage = TEXT("Default message from client.");
    TCHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;
    LPCTSTR lpszPipename = TEXT("\\\\.\\pipe\\mynamedpipe");

    DebugBreak();

    if (argc > 1)
        lpvMessage = argv[1];

    // Try to open a named pipe; wait for it, if necessary. 

    while (1) {
        hPipe = CreateFile(
            lpszPipename,   // pipe name 
            GENERIC_READ | GENERIC_WRITE,// read and write access 
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 
        if (hPipe != INVALID_HANDLE_VALUE)
            break;// Break if the pipe handle is valid. 

        if (GetLastError() != ERROR_PIPE_BUSY) {
            _tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            return -1;// Exit if an error other than ERROR_PIPE_BUSY occurs. 
        }

        if (!WaitNamedPipe(lpszPipename, 20000)) {// All pipe instances are busy, so wait for 20 seconds. 
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    // The pipe connected; change to message-read mode. 
    dwMode = PIPE_READMODE_MESSAGE;
    fSuccess = SetNamedPipeHandleState(
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess) {
        _tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    // Send a message to the pipe server. 
    cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
    _tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);
    fSuccess = WriteFile(
        hPipe,                  // pipe handle 
        lpvMessage,             // message 
        cbToWrite,              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 
    if (!fSuccess) {
        _tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    printf("\nMessage sent to server, receiving reply as follows:\n");

    do {
        // Read from the pipe. 
        fSuccess = ReadFile(
            hPipe,    // pipe handle 
            chBuf,    // buffer to receive reply 
            BUFSIZE * sizeof(TCHAR),  // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 
        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        _tprintf(TEXT("\"%s\"\n"), chBuf);
    } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    if (!fSuccess) {
        _tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    printf("\n<End of message, press ENTER to terminate connection and exit>");
    (void)_getch();

    CloseHandle(hPipe);

    return 0;
}
