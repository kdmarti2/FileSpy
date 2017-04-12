#include "FileSpy.h"
#include "FileSpySupport.h"


static UNICODE_STRING DirProtect = RTL_CONSTANT_STRING(L"\\Users\\WDKRemoteUser\\Documents\\");

NTSTATUS
FileSpyInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}
NTSTATUS
FileSpyInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}

VOID
FileSpyInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyInstanceTeardownStart: Entered\n") );
}
VOID
FileSpyInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyInstanceTeardownComplete: Entered\n") );
}
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!DriverEntry: Entered\n") );
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Entry\n"));
    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {
        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}

NTSTATUS
FileSpyUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyUnload: Entered\n") );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}
FLT_PREOP_CALLBACK_STATUS
FileSpyPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
   // NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;

    UNREFERENCED_PARAMETER( CompletionContext );

	/*
		Ignores all directories
	*/
	if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	//if (FlagOn(Data->Iopb->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		//return FLT_PREOP_SUCCESS_NO_CALLBACK;
	//remove system files they suck
	//if (FlagOn(Data->Iopb->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_SYSTEM))
		//return FLT_PREOP_SUCCESS_NO_CALLBACK;
	/*
		for right now I am going to ignore all directories
	*/
	/*
		I need the tunneling name and volume name for redirection possibly
	*/
	FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	FltParseFileNameInformation(nameInfo);

	//	Need to denied access to shadow files pretty much we will denied them

	unsigned int Len = nameInfo->Extension.Length / 2;
	unsigned int i = 0;
	for (i = 0;i < Len;i++)
	{
		if (nameInfo->Extension.Buffer[i] == L'$')
		{
			UNICODE_STRING ext;
			generateExt(&ext, Data);
			if (UStrncmp(&ext, &nameInfo->Extension, 1))
			{
				FltReleaseFileNameInformation(nameInfo);
				ExFreePool(ext.Buffer);
				return FLT_PREOP_SUCCESS_WITH_CALLBACK;
			}
			ExFreePool(ext.Buffer);

			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			Data->IoStatus.Information = 0;
			FltReleaseFileNameInformation(nameInfo);
			return FLT_PREOP_COMPLETE;
		}
	}
	//make sure the request is in our protection directory
	if (UStrncmp(&DirProtect, &nameInfo->ParentDir,0))
	{
		ULONG uDisposition = Data->Iopb->Parameters.Create.Options >> 24 & 0xFF;
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "mask:%d Test %d\n", Data->Iopb->Parameters.Create.Options,nameInfo->Name));
		if (uDisposition == FILE_OPEN)
		{

			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "FILE_OPEN %wZ dir %wZ\n", nameInfo->Name, nameInfo->ParentDir));
			/*Write had been detected, this allows the program to potentially change file*/
			if (FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_GENERIC_WRITE)
				||
				FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, STANDARD_RIGHTS_WRITE))
			{
				//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "WRITE Access Requested\n"));
				if (redirectIO(FltObjects, Data, nameInfo))
				{
					FltReleaseFileNameInformation(nameInfo);
					return FLT_PREOP_COMPLETE;
				}
			}
		} else if (uDisposition == FILE_OPEN_IF) {
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "FILE_OPEN_IF %wZ dir %wZ\n", nameInfo->Name, nameInfo->Extension, nameInfo->ParentDir));
			if (FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, FILE_GENERIC_WRITE)
				||
				FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess, STANDARD_RIGHTS_WRITE))
			{
				//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "WRITE Access Requested\n"));
				if(redirectIO(FltObjects, Data, nameInfo))
				{
					FltReleaseFileNameInformation(nameInfo);
					return FLT_PREOP_COMPLETE;
				}
			}
		} else if (uDisposition == FILE_OVERWRITE_IF) {
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "FILE_OVERWRITE_IF %wZ dir %wZ\n", nameInfo->Name, nameInfo->Extension, nameInfo->ParentDir));
			if (redirectIO(FltObjects, Data, nameInfo))
			{
				FltReleaseFileNameInformation(nameInfo);
				return FLT_PREOP_COMPLETE;
			}
		} else if (uDisposition == FILE_OVERWRITE) {
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "FILE_OVERWRITE %wZ dir %wZ\n", nameInfo->Name, nameInfo->Extension, nameInfo->ParentDir));
			if (redirectIO(FltObjects, Data, nameInfo))
			{
				FltReleaseFileNameInformation(nameInfo);
				return FLT_PREOP_COMPLETE;
			}
		} else if (uDisposition == FILE_SUPERSEDE) {
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "FILE_SUPERSEDE %wZ dir %wZ\n", nameInfo->Name, nameInfo->Extension, nameInfo->ParentDir));
		}
	}
	FltReleaseFileNameInformation(nameInfo);
/*    if (FileSpyDoRequestOperationStatus( Data )) {
        status = FltRequestOperationStatusCallback( Data,
                                                    FileSpyOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {
        }
    }*/
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
VOID
FileSpyOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("FileSpy!FileSpyOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}
FLT_POSTOP_CALLBACK_STATUS
FileSpyPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyPostOperation: Entered\n") );
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "PostOp\n"));
    return FLT_POSTOP_FINISHED_PROCESSING;
}
FLT_PREOP_CALLBACK_STATUS
FileSpyPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileSpy!FileSpyPreOperationNoPostOperation: Entered\n") );

	//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "PreNoPostOp\n"));
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
BOOLEAN
FileSpyDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    return (BOOLEAN)
             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}
