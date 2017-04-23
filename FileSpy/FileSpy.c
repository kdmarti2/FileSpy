#include "FileSpy.h"


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
FileSpyInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!FileSpyInstanceTeardownStart: Entered\n"));
}
VOID
FileSpyInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!FileSpyInstanceTeardownComplete: Entered\n"));
}
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(RegistryPath);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!DriverEntry: Entered\n"));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Entry\n"));
	status = FltRegisterFilter(DriverObject,
		&FilterRegistration,
		&gFilterHandle);

	FLT_ASSERT(NT_SUCCESS(status));
	InitComIface();
	if (NT_SUCCESS(status)) {
		status = FltStartFiltering(gFilterHandle);
		if (!NT_SUCCESS(status)) {

			FltUnregisterFilter(gFilterHandle);
			return status;
		}
		status = PsSetCreateProcessNotifyRoutineEx(&CreateProcessNotifyEx, FALSE);
		if (!NT_SUCCESS(status))
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "failed to register proctrace\n"));
			FltUnregisterFilter(gFilterHandle);
			return status;
		}
	}
	return status;
}

NTSTATUS
FileSpyUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!FileSpyUnload: Entered\n"));

	FltUnregisterFilter(gFilterHandle);
	PsSetCreateProcessNotifyRoutineEx(&CreateProcessNotifyEx, TRUE);
	return STATUS_SUCCESS;
}
FLT_PREOP_CALLBACK_STATUS
FileSpyPreWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PFLT_FILE_NAME_INFORMATION nameInfo;


	if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (processWhiteList(Data))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	//dont use FLT_FILE_NAME_NORMALIZED its slow as fuck for pre_create
	NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	if (!NT_SUCCESS(status))
	{
		//	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "potential file tunnel\n"));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (UStrncmp(&DirProtect, &nameInfo->ParentDir, 0))
	{
		FltParseFileNameInformation(nameInfo);
		PEPROCESS objCurProcess = IoThreadToProcess(Data->Thread);
		unsigned long long PID = (unsigned long long)PsGetProcessId(objCurProcess);
		if (PID != 4 && PID != 0)
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "wrote to %wZ\n", nameInfo->Name));
			updateIO(&nameInfo->Name, PID);
		}
		FltReleaseFileNameInformation(nameInfo);
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS
FileSpyPostWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!FileSpyPostOperation: Entered\n"));
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "PostOp\n"));
	return FLT_POSTOP_FINISHED_PROCESSING;;
}
/*


MIGHT BE REMOVED as it holds no need for it

*/
FileSpyPreSetInfo(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;

	if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (processWhiteList(Data))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	//need to test if the rename is going to be in the destination of just ioredirect it.
	if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileRenameInformation)
	{
		PFILE_RENAME_INFORMATION fri = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
		if (WCStrncmp(fri->FileName, wdirprotect, 37))
		{
			//This is when the file is going to be overwritten through the rename process of either copy/move for now it may be just denied its accesss
			//preform IO redirect
			//Just denied it for right now.... this needs to be an IO redirect but I do not have a solution for it
			Data->IoStatus.Status = STATUS_ACCESS_DENIED;
			Data->IoStatus.Information = 0;
			return FLT_PREOP_COMPLETE;
		}
	}
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	if (NT_SUCCESS(status))
	{
		status = FltParseFileNameInformation(nameInfo);
		if (NT_SUCCESS(status))
		{
			if (UStrncmp(&DirProtect, &nameInfo->ParentDir, 0))
			{
				//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "setInfo %wZ\n %ld\n", nameInfo->Name, Data->Iopb->Parameters.SetFileInformation.FileInformationClass));
				//https://www.osronline.com/showthread.cfm?link=226825
				if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation &&
					((PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile)
				{
					//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "File Delete %wZ %d\n", nameInfo->Name, Data->Iopb->Parameters.SetFileInformation.FileInformationClass));
					//just report that an attempt to delete happend
					//will delete its own shadow copy lol
				}
				else if (Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileRenameInformation)
				{
					PFILE_RENAME_INFORMATION fri = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
					//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Compared against %ws\n", wbin));
					if (WCStrncmp(wbin, fri->FileName, 20))
					{
					//	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Deleted %ws\n %ws\n", fri->FileName, fri->RootDirectory));
						//just report that an attempt to delete happend
						//will delete its own shadow copy lol
					}
					else {
					//	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Moved %ws\n %ws\n", fri->FileName, fri->RootDirectory));
						//touched
					}
				}
			}
		}
		FltReleaseFileNameInformation(nameInfo);
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS
FileSpyPostSetInfo(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSpy!FileSpyPostOperation: Entered\n"));
	//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "PostOp\n"));
	return FLT_POSTOP_FINISHED_PROCESSING;
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
		CHECK TO SEE IF I NEED TO DO an IO HIGHJACK HERE
	*/

		//HOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOK
	IOhook(FltObjects);
	hhook(FltObjects);

	/*
		CHECK TO SEE IF I NEED TO DO an IO HIGHJACK HERE
	*/
	/*
		Needs to be a function called isDirecotyr
	*/
	if (FlagOn(Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (FlagOn(Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)) {

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	if (processWhiteList(Data))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	//dont use FLT_FILE_NAME_NORMALIZED its slow as fuck for pre_create
	NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	if (!NT_SUCCESS(status))
	{
	//	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "potential file tunnel\n"));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	FltParseFileNameInformation(nameInfo);

	//	Need to denied access to shadow files pretty much we will denied them
	// This needs to be denied here
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
	if (UStrncmp(&DirProtect, &nameInfo->ParentDir,0))
	{
		//need to re
		if (redirectIO(FltObjects, Data, nameInfo))
		{
			FltReleaseFileNameInformation(nameInfo);
			return FLT_PREOP_COMPLETE;
		}
	}
	FltReleaseFileNameInformation(nameInfo);
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
