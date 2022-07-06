/*
һ��û�гɹ��Ĳ��Դ���.
���ԵĴ���Ϊmsdn�ϵ�NamedPipe����mailslot��ʾ������.
�˴���Ĳ��Ի���Ϊ:win 8 and windows server 2012���Ժ��ϵͳ.

���Ǹ��Ӳ�����:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot"��!��win 8��ǰ֧��,win 8��Ҳ֧��,win 8�Ժ�Ͳ�˵��.

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
����namedpipe and mailslot�Ĳ���.
����ֻ������win 8 and windows server 2012���Ժ��ϵͳ.
win 8 ������Ҫ���Ӳ�����:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot".
*/
{
    //û���ߵ������,������ߵ�������?�ѵ�Ҫʹ��:FltCreateNamedPipeFile��FltCreateMailslotFile

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
        KdPrint(("i am in CreatePostOperation!\n"));//��Ҳ��ʶ��NAMED_PIPE��MAILSLOT��һ���취��
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}


#pragma NPAGEDCODE
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,            0,  0,                              CreatePostOperation},
    { IRP_MJ_CREATE_NAMED_PIPE, 0,  CreateNamedPipePreOPeration,    0},
    { IRP_MJ_CREATE_MAILSLOT,   0,  CreateNamedPipePreOPeration,    0},//�����IRP_MJ_CREATE_NAMED_PIPE�Ĵ���һ��,��ͬһ������.
    { IRP_MJ_OPERATION_END }
};


BOOLEAN PrintVolume(__in PCFLT_RELATED_OBJECTS FltObjects)
/*
���ܣ���ӡ���صĶ������Ϣ��
����ʼ��û�д�ӡ��:L"\\Device\\NamedPipe" and L"\\Device\\Mailslot".
���ǵ�һ����:L"\\Device\\Mup"��һЩ�����,win 8 ��ǰҲ��������.
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

    status = FltGetVolumeName(FltObjects->Volume, &Volume, &BufferSizeNeeded);//���һ������ΪNULLʧ�ܡ�
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
    UNREFERENCED_PARAMETER(Flags);//�������if�������ì�ܡ�

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
    ע����������ƽ̨Ҫѡ��WIN 8,�����Ż���Ч��.
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
