#pragma once

#include <ntifs.h>
#include <windef.h>
#include <fltkernel.h>
#include <ntddk.h>

#define TAG 'tset' //test


//////////////////////////////////////////////////////////////////////////////////////////////////


#define VALID_FAST_IO_DISPATCH_HANDLER(_FastIoDispatchPtr, _FieldName) \
    (((_FastIoDispatchPtr) != NULL) && \
(((_FastIoDispatchPtr)->SizeOfFastIoDispatch) >= (FIELD_OFFSET(FAST_IO_DISPATCH, _FieldName) + sizeof(void *))) && \
((_FastIoDispatchPtr)->_FieldName != NULL))


typedef struct _FILTER_DEVICE_EXTENSION {
    int type; //可以填写的值有NPFS,MSFS,CD0
    PDEVICE_OBJECT NextDeviceObject;
} FILTER_DEVICE_EXTENSION, * PFILTER_DEVICE_EXTENSION;


//2008版本的MSDN，非WDK。
//http://msdn.microsoft.com/en-us/library/windows/desktop/ms687420(v=vs.85).aspx
//上面的一些标注在低版本上的WDK出错。
NTSTATUS /* WINAPI */ ZwQueryInformationProcess(
    __in          HANDLE ProcessHandle,
    __in          PROCESSINFOCLASS ProcessInformationClass,
    __out         PVOID ProcessInformation,
    __in          ULONG ProcessInformationLength,
    __out_opt     PULONG ReturnLength);


//////////////////////////////////////////////////////////////////////////////////////////////////


#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILENAMEW__ (wcsrchr(_CRT_WIDE(__FILE__), L'\\') ? wcsrchr(_CRT_WIDE(__FILE__), L'\\') + 1 : _CRT_WIDE(__FILE__))

/*
既支持单字符也支持宽字符。
注意：
1.第三个参数是单字符，可以为空，但不要为NULL，更不能省略。
2.驱动在DPC上不要打印宽字符。
3.
*/

//这个支持3三个参数。
#define Print(ComponentId, Level, Format, ...) \
{DbgPrintEx(ComponentId, Level, "FILE:%s, LINE:%d, "##Format".\r\n", __FILENAME__, __LINE__, __VA_ARGS__);}

//这个最少4个参数。
#define PrintEx(ComponentId, Level, Format, ...) \
{KdPrintEx((ComponentId, Level, "FILE:%s, LINE:%d, "##Format".\r\n", __FILENAME__, __LINE__, __VA_ARGS__));}


//////////////////////////////////////////////////////////////////////////////////////////////////


//这些在WDK 7600.16385.1中没有定义,在WDK8.0中定义了.
//一下代码是解决办法之一.
#ifndef _USE_ATTRIBUTES_FOR_SAL
#define _Inout_
#define _In_
#define _In_opt_
#define _Out_opt_
#define _Inout_
#define _Flt_CompletionContext_Outptr_
#define _Out_
#define _Outptr_result_maybenull_
#define _Outptr_opt_ 
#define _Outptr_
#define _Must_inspect_result_
#define _Unreferenced_parameter_
#define _Interlocked_
#define _Requires_no_locks_held_
#define _IRQL_requires_same_ 

#define _IRQL_requires_(irql) 
#define _Out_writes_bytes_(ExpandComponentNameLength)
#define _When_(expr, annotes)   
#define _In_reads_bytes_(Length) //NOTHING   //XP FREE 注释掉这。
#define _In_reads_bytes_opt_(EaLength)
#define _Post_satisfies_(a)
#define _IRQL_requires_max_(a)
#define _At_(a, b)
#define _Success_(a)
#define _Analysis_assume_(expr) 
#define _Out_writes_bytes_to_(size,count)
#define _Outptr_result_bytebuffer_maybenull_(size)
#define _Pre_satisfies_(cond) 
#define _Guarded_by_(lock)
#define _Write_guarded_by_(lock)
#define _Requires_lock_held_(lock)
#define _Requires_exclusive_lock_held_(lock)
#define _Requires_shared_lock_held_(lock)
#define _Requires_lock_not_held_(lock)
#define _Acquires_lock_(lock)
#define _Acquires_exclusive_lock_(lock)
#define _Acquires_shared_lock_(lock)
#define _Releases_lock_(lock)
#define _Releases_exclusive_lock_(lock)
#define _Releases_shared_lock_(lock)
#define _Acquires_nonreentrant_lock_(lock)
#define _Releases_nonreentrant_lock_(lock)
#define FLT_ASSERT(_e) NT_ASSERT(_e)
#define _Dispatch_type_(x) 
#endif
//另一思路可参考：http://msdn.microsoft.com/zh-cn/library/windows/hardware/ff554695(v=vs.85).aspx


//////////////////////////////////////////////////////////////////////////////////////////////////
