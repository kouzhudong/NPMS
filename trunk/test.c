#include "pch.h"
#include "FastIo.h"


/*
一下是微软官方的回信:
Hi Kou,
Thanks for your feedback question about this topic.
In the Windows kernel, mailslot and named pipe operations are handled as file I/O.
Prior to Windows 8, the filter manager did not direct I/O for these ‘file types’ to the registered minifilters.
You would need to create a legacy style filter to capture any I/O for a mailslot or a named pipe.
Starting with Windows 8, however, your minifilter can register to filter I/O for the mailslot and named pipe file types also.
Thanks again,
Galen

本文的功能是附加并过滤下面两个对象的操作.
L"\\Device\\NamedPipe"
L"\\Device\\Mailslot"
没有fastio会蓝屏的,这个费了我一周的时间才解决,刚开始是确信没有fastio的,这是错误的.

named pipe测试代码:
http://msdn.microsoft.com/en-us/library/windows/desktop/aa365588(v=vs.85).aspx
http://msdn.microsoft.com/en-us/library/windows/desktop/aa365592(v=vs.85).aspx

Mailslots测试代码:
http://msdn.microsoft.com/en-us/library/windows/desktop/aa365160(v=vs.85).aspx
http://msdn.microsoft.com/en-us/library/windows/desktop/aa365802(v=vs.85).aspx
http://msdn.microsoft.com/en-us/library/windows/desktop/aa365785(v=vs.85).aspx

made by correy
made at 2013.09.05
email:kouleguan at hotmail dot com
*/


PDRIVER_OBJECT g_DriverObject;


#pragma warning(disable:28169) 
#pragma warning(disable:28175)


//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL GetProcessImageFileName(IN HANDLE PId, OUT UNICODE_STRING * file_name)
{
    NTSTATUS status = 0;
    PVOID us_ProcessImageFileName = 0;//UNICODE_STRING
    ULONG ProcessInformationLength = 0;
    ULONG ReturnLength = 0;
    PEPROCESS  EProcess = 0;
    HANDLE  Handle = 0;
    UNICODE_STRING * p = {0};
    UNICODE_STRING temp = {0};
    USHORT i = 0;

    /*
    必须转换一下，不然是无效的句柄。
    大概是句柄的类型转换为内核的。
    */
    status = PsLookupProcessByProcessId(PId, &EProcess);
    if (!NT_SUCCESS(status)) {
        KdPrint(("PsLookupProcessByProcessId fail with 0x%x in line %d\n", status, __LINE__));
        return FALSE;
    }
    ObDereferenceObject(EProcess); //微软建议加上。
    status = ObOpenObjectByPointer(EProcess,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   GENERIC_READ,
                                   *PsProcessType,
                                   KernelMode,
                                   &Handle);//注意要关闭句柄。  
    if (!NT_SUCCESS(status)) {
        KdPrint(("ObOpenObjectByPointer fail with 0x%x in line %d\n", status, __LINE__));
        return FALSE;
    }

    //获取需要的内存。
    status = ZwQueryInformationProcess(Handle,
                                       ProcessImageFileName,
                                       us_ProcessImageFileName,
                                       ProcessInformationLength,
                                       &ReturnLength);
    if (!NT_SUCCESS(status) && status != STATUS_INFO_LENGTH_MISMATCH) {
        KdPrint(("ZwQueryInformationProcess fail with 0x%x in line %d\n", status, __LINE__));
        ZwClose(Handle);
        return FALSE;
    }
    ProcessInformationLength = ReturnLength;
    us_ProcessImageFileName = ExAllocatePoolWithTag(NonPagedPool, ReturnLength, TAG);
    if (us_ProcessImageFileName == NULL) {
        KdPrint(("ExAllocatePoolWithTag fail with 0x%x\n", status));
        status = STATUS_INSUFFICIENT_RESOURCES;
        ZwClose(Handle);
        return FALSE;
    }
    RtlZeroMemory(us_ProcessImageFileName, ReturnLength);

    status = ZwQueryInformationProcess(Handle,
                                       ProcessImageFileName,
                                       us_ProcessImageFileName,
                                       ProcessInformationLength,
                                       &ReturnLength);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ZwQueryInformationProcess fail with 0x%x in line %d\n", status, __LINE__));
        ExFreePoolWithTag(us_ProcessImageFileName, TAG);
        ZwClose(Handle);
        return FALSE;
    }

    //KdPrint(("ProcessImageFileName:%wZ\n",us_ProcessImageFileName));//注意：中间有汉字是不会显示的。
    //形如：ProcessImageFileName:\Device\HarddiskVolume1\aa\Dbgvvvvvvvvvvvvvvvvvvvvvview.exe
    p = (UNICODE_STRING *)us_ProcessImageFileName;

    //从末尾开始搜索斜杠。
    for (i = p->Length / 2 - 1; ; i--) {
        if (p->Buffer[i] == L'\\') {
            break;
        }
    }

    i++;//跳过斜杠。

    //构造文件名结构，复制用的。
    temp.Length = p->Length - i * 2;
    temp.MaximumLength = p->MaximumLength - i * 2;
    temp.Buffer = &p->Buffer[i];

    //这个内存由调用者释放。
    file_name->Buffer = ExAllocatePoolWithTag(NonPagedPool, MAX_PATH, TAG);
    if (file_name->Buffer == NULL) {
        DbgPrint("发生错误的文件为:%s, 代码行为:%d\n", __FILE__, __LINE__);
        ExFreePoolWithTag(us_ProcessImageFileName, TAG);
        ZwClose(Handle);
        return FALSE;
    }
    RtlZeroMemory(file_name->Buffer, MAX_PATH);
    RtlInitEmptyUnicodeString(file_name, file_name->Buffer, MAX_PATH);

    RtlCopyUnicodeString(file_name, &temp);
    //KdPrint(("ProcessImageFileName:%wZ\n",file_name));

    ExFreePoolWithTag(us_ProcessImageFileName, TAG);
    ZwClose(Handle);
    return TRUE;
}


void NamedPipe(IN PIO_STACK_LOCATION irpStack, IN PIRP Irp)
/*
这里的内容要通过栈回溯来获取？
如：inputBuffer和outputBuffer。
*/
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    PVOID outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    UNICODE_STRING file_name;

    GetProcessImageFileName(PsGetCurrentProcessId(), &file_name);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE: //估计这里全是打开的操作.

        if (FlagOn(irpStack->FileObject->Flags, FO_NAMED_PIPE)) {
            KdPrint(("FO_NAMED_PIPE!\n"));//这里没有拦截到.
        }

        if (Irp->IoStatus.Information == FILE_CREATED)//应该在完成后检测.
        {
            KdPrint(("named pipe FILE_CREATED!\n"));//这里没有拦截到.
        }

        /*
        检测到的名字有:操作是在桌面刷新一下.
        "\srvsvc"  这两个在对象管理器中没有找到.
        "\lsarpc"
        "\wkssvc"
        "\qqpcmgr\3_1_80" 这个估计是QQ电脑管家的.
        还有就是在我们的客户端测试程序拦截的,我们的东西"\mynamedpipe".
        */
        KdPrint(("named pipe name:%wZ\n", &(irpStack->FileObject->FileName)));

        break;
    case IRP_MJ_CREATE_NAMED_PIPE:
        KdPrint(("IRP_MJ_CREATE_NAMED_PIPE!\n"));//这里拦截到了,估计这里是创建的.用测试代码,也是创建操作在这里过.
        KdPrint(("create named pipe name:%wZ\n", &(irpStack->FileObject->FileName)));
        break;
    case IRP_MJ_READ:
        KdPrint(("IRP_MJ_READ:ProcessImageFileName:%wZ, pipe name:%wZ, Length:%d\n",
                 &file_name,
                 &currentIrpStack->FileObject->FileName,
                 outputBufferLength));//大小不是inputBufferLength
        break;
    case IRP_MJ_WRITE:
        KdPrint(("IRP_MJ_WRITE:ProcessImageFileName:%wZ, pipe name:%wZ, Length:%d\n",
                 &file_name,
                 &currentIrpStack->FileObject->FileName,
                 outputBufferLength));
        break;
    default:
        //KdPrint(("NAMED PIPE MajorFunction == %d!\n",irpStack->MajorFunction));//FltGetIrpName
        KdPrint(("NAMED PIPE %s!\n", FltGetIrpName(irpStack->MajorFunction)));
        break;
    }

    RtlFreeUnicodeString(&file_name);
}


void Mailslot(IN PIO_STACK_LOCATION irpStack, IN PIRP Irp)
{
    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE: //估计这里全是打开的操作.

        if (FlagOn(irpStack->FileObject->Flags, FO_MAILSLOT)) {
            KdPrint(("FO_MAILSLOT!\n"));//这里没有拦截到.
        }

        if (Irp->IoStatus.Information == FILE_CREATED)//应该在完成后检测.
        {
            KdPrint(("mail slot FILE_CREATED!\n"));//这里没有拦截到.
        }

        KdPrint(("mail slot name:%wZ\n", &(irpStack->FileObject->FileName)));

        break;
    case IRP_MJ_CREATE_MAILSLOT:

        //打开局域网上的文件,发现了"\NET\GETDC042".

        KdPrint(("IRP_MJ_CREATE_MAILSLOT!\n"));//这里拦截到了,估计这里是创建的.
        KdPrint(("create mail slot name:%wZ\n", &(irpStack->FileObject->FileName)));
        break;
    case IRP_MJ_READ:
        KdPrint(("mail slot read!\n"));
        break;
    case IRP_MJ_WRITE:
        KdPrint(("mail slot write!\n"));
        break;
    case IRP_MJ_SET_INFORMATION:
        KdPrint(("mail slot set information!\n"));
        break;
    default:
        KdPrint(("mail slot MajorFunction == %d!\n", irpStack->MajorFunction));
        break;
    }
}


NTSTATUS PassThrough(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
#pragma alloc_text(PAGE, PassThrough)
NTSTATUS PassThrough(IN PDEVICE_OBJECT pDO, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFILTER_DEVICE_EXTENSION pDevExt = (PFILTER_DEVICE_EXTENSION)pDO->DeviceExtension;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (pDevExt->type == 'NPFS') {
        NamedPipe(irpStack, Irp);
    } else if (pDevExt->type == 'MSFS') {
        Mailslot(irpStack, Irp);
    } else if (pDevExt->type == 'XPUM') {
        //Mailslot (irpStack,Irp);
    } else {
        //CD0,这里可以有控制设备的处理.
    }

    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(pDevExt->NextDeviceObject, Irp);//AttachedDevice
    if (!NT_SUCCESS(Status)) {
        //这里失败是很正常的。
    }

    return Status;
}


VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
#pragma alloc_text(PAGE, DriverUnload)
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT preDeviceObject, CurrentDeviceObject;
    PFILTER_DEVICE_EXTENSION pDevExt;

    PAGED_CODE();

    preDeviceObject = DriverObject->DeviceObject;
    while (preDeviceObject != NULL) {
        pDevExt = (PFILTER_DEVICE_EXTENSION)preDeviceObject->DeviceExtension;
        if (pDevExt->NextDeviceObject) { //确保本驱动创建的设备都包含PFILTER_DEVICE_EXTENSION,没有挂载的把此值设置为0.
            IoDetachDevice(pDevExt->NextDeviceObject);//反附加.AttachedDevice
        }

        CurrentDeviceObject = preDeviceObject;
        preDeviceObject = CurrentDeviceObject->NextDevice;
        IoDeleteDevice(CurrentDeviceObject);
    }
}


NTSTATUS AttachedDevice(PCWSTR Device, PCWSTR FilterDeviceName, int Flag)
/*
功能:创建一个(过滤)设备并挂载到指定的设备上.
参数说明:
pdevice 被挂载的设备.
filter_device_name 创建的过滤设备.
flag 创建的过滤设备的标志,在设备对象的扩展结构里面.

失败的处理还不完善.需要改进.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT FilterDeviceObject;
    UNICODE_STRING ObjectName;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT P_Target_DEVICE_OBJECT = 0;
    PFILTER_DEVICE_EXTENSION pDevExt = 0;
    PDEVICE_OBJECT  AttachedDevice = 0;
    PDEVICE_OBJECT      fileSysDevice;

    RtlInitUnicodeString(&ObjectName, Device);
    Status = IoGetDeviceObjectPointer(&ObjectName, FILE_ALL_ACCESS, &FileObject, &P_Target_DEVICE_OBJECT);
    if (Status != STATUS_SUCCESS) {
        KdPrint(("IoGetDeviceObjectPointer fail!\n"));
        KdBreakPoint();
    }

    ObDereferenceObject(FileObject);

    RtlInitUnicodeString(&DeviceName, FilterDeviceName);// NT_DEVICE_NAME  FILTER_DEVICE_NAME
    Status = IoCreateDevice(g_DriverObject,
                            sizeof(FILTER_DEVICE_EXTENSION),
                            &DeviceName,
                            P_Target_DEVICE_OBJECT->DeviceType,
                            0,
                            FALSE,
                            &FilterDeviceObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ClearFlag(FilterDeviceObject->Flags, DO_DEVICE_INITIALIZING); //filterDeviceObject->Flags &= ~0x00000080;

    AttachedDevice = IoAttachDeviceToDeviceStack(FilterDeviceObject, P_Target_DEVICE_OBJECT);//返回附加前的顶层设备.
    if (AttachedDevice == NULL) {
        KdPrint(("IoAttachDeviceToDeviceStack fail!\n"));
        KdBreakPoint();
    }

    pDevExt = (PFILTER_DEVICE_EXTENSION)FilterDeviceObject->DeviceExtension;
    pDevExt->NextDeviceObject = AttachedDevice;
    pDevExt->type = Flag;

    return Status;
}


DRIVER_INITIALIZE DriverEntry;
//#pragma INITCODE
#pragma alloc_text(INIT, DriverEntry)
//_Function_class_(DRIVER_INITIALIZE)
//_IRQL_requires_same_
//_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    int i;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(RegistryPath);

    KdBreakPoint();

    g_DriverObject = DriverObject;

    DriverObject->DriverUnload = DriverUnload;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = PassThrough;
    }

    DriverObject->FastIoDispatch = &g_FastIoDispatch;

    Status = AttachedDevice(L"\\Device\\NamedPipe", L"\\Device\\NPFS", 'NPFS');
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = AttachedDevice(L"\\Device\\Mailslot", L"\\Device\\MSFS", 'MSFS');
    if (!NT_SUCCESS(Status)) {
        //添加删除和反附加设备的代码.
        KdBreakPoint();
    }

    //Status = AttachedDevice(L"\\Device\\MUP", L"\\Device\\NUPX", 'XPUM');//MUPX
    if (!NT_SUCCESS(Status)) {
        //添加删除和反附加设备的代码.
        KdBreakPoint();
    }

    //\Device\LanmanagerRedirector LMRD

    return Status;
}
