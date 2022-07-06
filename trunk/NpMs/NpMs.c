/*
一个没有成功的测试代码.
测试的代码为msdn上的NamedPipe或者mailslot的示例代码.
此代码的测试环境为:win 8 and windows server 2012及以后的系统.

还是附加并过滤:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot"吧!在win 8以前支持,win 8上也支持,win 8以后就不说了.

made by correy
made at 2013.09.03
email:kouleguan at hotmail dot com
homepage:http://correy.webs.com
*/

#include <fltKernel.h>

PFLT_FILTER gFilterHandle;

#define TAG 'tset' //test


FLT_PREOP_CALLBACK_STATUS CreateNamedPipePreOPeration(__inout PFLT_CALLBACK_DATA Cbd, __in PCFLT_RELATED_OBJECTS FltObjects, __out PVOID * CompletionContext)
/*
拦截namedpipe and mailslot的操作.
可能只适宜于win 8 and windows server 2012及以后的系统.
win 8 以下需要附加并过滤:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot".
*/
{
    //没有走到这里过,如何能走到这里呢?难道要使用:FltCreateNamedPipeFile和FltCreateMailslotFile

    UNREFERENCED_PARAMETER(FltObjects);

    *CompletionContext = NULL;

    KdPrint(("i am in CreateNamedPipePreOPeration!\n"));

    if (0) {
        Cbd->IoStatus.Status = STATUS_ACCESS_DENIED;
        Cbd->IoStatus.Information = 0;
        return FLT_PREOP_COMPLETE;
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS CreatePostOperation(__inout PFLT_CALLBACK_DATA Data, 
                                               __in PCFLT_RELATED_OBJECTS FltObjects, 
                                               __in_opt PVOID CompletionContext, 
                                               __in FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(FltObjects);

    if (FlagOn(Data->Iopb->TargetFileObject->Flags, FO_NAMED_PIPE) || FlagOn(Data->Iopb->TargetFileObject->Flags, FO_MAILSLOT)) {//Data->Flags
        KdPrint(("i am in CreatePostOperation!\n"));//这也是识别NAMED_PIPE和MAILSLOT的一个办法。
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}


#pragma NPAGEDCODE
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,            0,  0,                              CreatePostOperation},
    { IRP_MJ_CREATE_NAMED_PIPE, 0,  CreateNamedPipePreOPeration,    0},
    { IRP_MJ_CREATE_MAILSLOT,   0,  CreateNamedPipePreOPeration,    0},//这个和IRP_MJ_CREATE_NAMED_PIPE的处理一样,用同一个函数.
    { IRP_MJ_OPERATION_END }
};


BOOLEAN PrintVolume(__in PCFLT_RELATED_OBJECTS FltObjects)
/*
功能：打印挂载的对象的信息。
这里始终没有打印到:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot".
倒是第一个是:L"\\Device\\Mup"和一些卷对象,win 8 以前也是这样的.
*/
{
    NTSTATUS status;
    PVOID Buffer;
    BOOLEAN r = FALSE;
    ULONG BufferSizeNeeded;
    UNICODE_STRING Volume;

    status = FltGetVolumeName(FltObjects->Volume, NULL, &BufferSizeNeeded);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        return FALSE;
    }

    Buffer = ExAllocatePoolWithTag(NonPagedPool, (SIZE_T)BufferSizeNeeded + 2, TAG);
    if (Buffer == NULL) {
        return FALSE;
    }
    RtlZeroMemory(Buffer, (size_t)BufferSizeNeeded + 2);

    Volume.Buffer = Buffer;
    Volume.Length = (USHORT)BufferSizeNeeded;
    Volume.MaximumLength = (USHORT)BufferSizeNeeded + 2;

    status = FltGetVolumeName(FltObjects->Volume, &Volume, &BufferSizeNeeded);//最后一个参数为NULL失败。
    if (!NT_SUCCESS(status)) {
        KdPrint(("FltGetVolumeName fail with error 0x%x!\n", status));
        ExFreePoolWithTag(Buffer, TAG);
        return FALSE;
    }

    KdPrint(("attached device:%wZ\n", &Volume));

    ExFreePoolWithTag(Buffer, TAG);
    return r;
}


NTSTATUS InstanceSetup(__in PCFLT_RELATED_OBJECTS FltObjects, 
                       __in FLT_INSTANCE_SETUP_FLAGS Flags,
                       __in DEVICE_TYPE VolumeDeviceType, 
                       __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(Flags);//与下面的if语句自相矛盾。

    PAGED_CODE();

    if (FILE_DEVICE_NAMED_PIPE == VolumeDeviceType || FILE_DEVICE_MAILSLOT == VolumeDeviceType) {
        KdPrint(("VolumeDeviceType: %d!\n", VolumeDeviceType));
    }

    if (FLT_FSTYPE_NPFS == VolumeFilesystemType|| FLT_FSTYPE_MSFS == VolumeFilesystemType) {
        KdPrint(("VolumeFilesystemType: %d!\n", VolumeFilesystemType));
    }

    PrintVolume(FltObjects);

    return STATUS_SUCCESS;//  Attach on manual attachment.
}


#pragma PAGEDCODE
NTSTATUS PtInstanceQueryTeardown(__in PCFLT_RELATED_OBJECTS FltObjects, __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    return STATUS_SUCCESS;
}


#pragma PAGEDCODE//#pragma alloc_text(PAGE, PtUnload)
NTSTATUS PtUnload(__in FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    FltUnregisterFilter(gFilterHandle);
    return STATUS_SUCCESS;
}


#pragma NPAGEDCODE
const FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),         //  Size

    /*
    注意这个编译的平台要选择WIN 8,这样才会起效果.
    #if FLT_MGR_WIN8
    #define FLT_REGISTRATION_VERSION   FLT_REGISTRATION_VERSION_0203  // Current version is 2.03
    #elif FLT_MGR_LONGHORN
    #define FLT_REGISTRATION_VERSION   FLT_REGISTRATION_VERSION_0202  // Current version is 2.02
    #else
    #define FLT_REGISTRATION_VERSION   FLT_REGISTRATION_VERSION_0200  // Current version is 2.00
    #endif
    */
    FLT_REGISTRATION_VERSION,           //  Version

#if (NTDDI_VERSION >= NTDDI_WIN8)
    FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS,   //  Flags
#else
    0,                                      //  Flags
#endif 

    0,                                  //  Context
    Callbacks,                          //  Operation callbacks
    PtUnload,                           //  MiniFilterUnload
    InstanceSetup,                      //  InstanceSetup
    PtInstanceQueryTeardown,            //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};


DRIVER_INITIALIZE DriverEntry;
#pragma alloc_text(INIT, DriverEntry)//#pragma INITCODE
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    KdBreakPoint();//DbgBreakPoint() 

    status = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);//Register with FltMgr to tell it our callback routines    
    if (NT_SUCCESS(status)) //FLT_ASSERT( NT_SUCCESS( status ) );
    {
        status = FltStartFiltering(gFilterHandle);
        if (!NT_SUCCESS(status)) {
            FltUnregisterFilter(gFilterHandle);
        }
    }

    return status;
}
