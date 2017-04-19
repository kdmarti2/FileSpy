#include "FileSpySupport.h"
UNICODE_STRING proclist[] = {
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\explorer.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\System32\\SearchProtocolHost.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\SearchIndexer.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\SearchFilterHost.exe")
};
//https://aonprog.wordpress.com/2009/02/16/process-name-and-id-in-file-filter-driver-callbacks/
BOOLEAN processWhiteList(PFLT_CALLBACK_DATA Data) {
	//GetProcessImageFileName
	PEPROCESS objCurProcess = IoThreadToProcess(Data->Thread);
	PUNICODE_STRING ProcName;
	SeLocateProcessImageName(objCurProcess,&ProcName);
	unsigned int i = 0;
	for (i = 0;i < WHITELISTSIZE;i++)
	{
		if(RtlEqualUnicodeString(ProcName, &proclist[i], TRUE))
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOLEAN UStrncmp(PUNICODE_STRING dst, PUNICODE_STRING src,USHORT POS)
{
	USHORT Len = 0;
	if (dst->Length-(POS*2) <= src->Length)
	{
		Len = dst->Length-(POS*2);
	}
	else{
		return FALSE;
	}
	int i = 0;
	for (i = 0;i < (Len/2);i++)
	{
		if (dst->Buffer[i+POS] != src->Buffer[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}
USHORT findLashSlash(PUNICODE_STRING pnt)
{
	USHORT i = 0;
	USHORT POS = 0;
	USHORT Len = pnt->Length / 2;
	for (i = 0;i < Len;i++)
	{
		if (pnt->Buffer[i] == L'\\')
		{
			POS = i;
		}
	}
	return POS;
}
void freeUnicodeString(PUNICODE_STRING str)
{
	ExFreePool(str->Buffer);
}
void createUnicodeString(PUNICODE_STRING dst,USHORT MaxSize)
{
	dst->Length = 0;
	dst->Buffer = (PWCH)ExAllocatePool(NonPagedPool, MaxSize);
	dst->MaximumLength = MaxSize;
}
void unicodePosCat(PUNICODE_STRING dst, PUNICODE_STRING src, USHORT start)
{
	USHORT i = 0;
	USHORT Len = src->Length / 2;
	for (i = 0;i < Len;i++)
	{
		dst->Buffer[i + start] = src->Buffer[i];
	}
	dst->Length += src->Length;
}

//https://aonprog.wordpress.com/2009/02/16/process-name-and-id-in-file-filter-driver-callbacks/
void generateExt(PUNICODE_STRING newExt, PFLT_CALLBACK_DATA Data) {
	UNICODE_STRING PID;

	PEPROCESS objCurProcess = IoThreadToProcess(Data->Thread);
	unsigned long long ProcId = (unsigned long long)PsGetProcessId(objCurProcess);
	PID.Buffer = (PWSTR)ExAllocatePool(NonPagedPool, 32);
	PID.Length = 0;
	PID.MaximumLength = 32;
	RtlIntegerToUnicodeString((ULONG)ProcId, 0, &PID);
	
	newExt->Buffer = (PWSTR)ExAllocatePool(NonPagedPool, PID.Length+4);
	newExt->Length = 0;
	newExt->MaximumLength = PID.Length + 4;
	newExt->Length = 4;
	newExt->Buffer[0] = L'.';
	newExt->Buffer[1] = L'$';
	unicodePosCat(newExt, &PID, 2);
	ExFreePool(PID.Buffer);
}
BOOLEAN redirectIO(PCFLT_RELATED_OBJECTS FltObjects,PFLT_CALLBACK_DATA Data, PFLT_FILE_NAME_INFORMATION nameInfo)
{
	/*http://www.progtown.com/topic259586-fltcreatefile-in-precreate-callback.html
	*/
	OBJECT_ATTRIBUTES objATT;
	HANDLE FHANDLE;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	PFILE_OBJECT realFileObject;
	InitializeObjectAttributes(&objATT, &nameInfo->Name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	status = FltCreateFileEx(
		FltObjects->Filter,
		FltObjects->Instance,
		&FHANDLE,
		&realFileObject,
		FILE_READ_DATA,
		&objATT,
		&ioStatusBlock,
		(PLARGE_INTEGER)NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		0,
		NULL,
		0,
		IO_IGNORE_SHARE_ACCESS_CHECK
	);
	if (!NT_SUCCESS(status))
	{
		return FALSE;
	}
	else {
		UNICODE_STRING newExt;
		UNICODE_STRING reTarget;
		generateExt(&newExt, Data);
		createUnicodeString(&reTarget, nameInfo->Name.Length + newExt.Length + 2);
		RtlCopyUnicodeString(&reTarget, &nameInfo->Name);
		unicodePosCat(&reTarget, &newExt, reTarget.Length / 2);		
		freeUnicodeString(&newExt);

		HANDLE THANDLE;
		IO_STATUS_BLOCK ioStatBlock2;
		OBJECT_ATTRIBUTES objATT3;
		InitializeObjectAttributes(&objATT3, &reTarget, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
		status = FltCreateFile(
			FltObjects->Filter,
			FltObjects->Instance,
			&THANDLE,
			FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
			&objATT3,
			&ioStatBlock2,
			(PLARGE_INTEGER)NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			FILE_OPEN, //test replace with overwrite
			0,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
		);
		if (NT_SUCCESS(status))
		{
			FltClose(THANDLE);
		} else {
			HANDLE OUTHANDLE;
			IO_STATUS_BLOCK ioStatBlock;
			OBJECT_ATTRIBUTES objATT2;
			PFILE_OBJECT shadowfile;
			InitializeObjectAttributes(&objATT2, &reTarget, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
			status = FltCreateFileEx(
				FltObjects->Filter,
				FltObjects->Instance,
				&OUTHANDLE,
				&shadowfile,
				FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
				&objATT2,
				&ioStatBlock,
				(PLARGE_INTEGER)NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OVERWRITE_IF, //test replace with overwrite
				0,
				NULL,
				0,
				IO_IGNORE_SHARE_ACCESS_CHECK
			);
			if (NT_SUCCESS(status))
			{
				unsigned char* buf = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, 1024, 'read');
				LARGE_INTEGER Pos;
				Pos.HighPart = 0;
				Pos.LowPart = 0;
				LARGE_INTEGER oldPos;
				oldPos.HighPart = 0;
				oldPos.LowPart = 0;
				ULONG readBytes = 0;
				do {
					NTSTATUS s = STATUS_SUCCESS;
					readBytes = 0;
					s = FltReadFile(
						FltObjects->Instance,
						realFileObject,
						&Pos,
						1024,
						buf,
						FLTFL_IO_OPERATION_NON_CACHED,
						&readBytes,
						0,
						0
					);
					Pos.QuadPart += readBytes;
					FltWriteFile(
						FltObjects->Instance,
						shadowfile,
						&oldPos,
						readBytes,
						buf,
						FLTFL_IO_OPERATION_NON_CACHED,
						0,
						0,
						0
					);
					oldPos.QuadPart += readBytes;
				} while (readBytes);
				FltFreePoolAlignedWithTag(FltObjects->Instance, buf, 'read');
				FltClose(OUTHANDLE);
			}
		}
		PEPROCESS objCurProcess = IoThreadToProcess(Data->Thread);
		unsigned long long PID = (unsigned long long)PsGetProcessId(objCurProcess);
		/**
			recording IO here
		**/
		recordIO(PID,&nameInfo->Name, &reTarget);
		/**
			recording IO here
		**/
		Data->IoStatus.Information = IO_REPARSE;
		Data->IoStatus.Status = STATUS_REPARSE;
		Data->Iopb->TargetFileObject->RelatedFileObject = NULL;
		ExFreePool(Data->Iopb->TargetFileObject->FileName.Buffer);
		Data->Iopb->TargetFileObject->FileName.Buffer = reTarget.Buffer;
		Data->Iopb->TargetFileObject->FileName.Length = reTarget.Length;
		Data->Iopb->TargetFileObject->FileName.MaximumLength = reTarget.MaximumLength;
		FltSetCallbackDataDirty(Data);
		FltClose(FHANDLE);
		return TRUE;
	}

}
BOOLEAN WCStrncmp(wchar_t* dst, wchar_t* src, unsigned int len)
{
	unsigned int i = 0;
	for (i = 0;i < len;i++)
	{
		if (dst[i] != src[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}
/*
BOOLEAN redirectRenameIO(PFLT_CALLBACK_DATA Data)
{
	/*UNICODE_STRING newExt;
	generateExt(&newExt, Data);
	PFILE_RENAME_INFORMATION fri = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
	
	wchar_t * newpath = ExAllocatePool(NonPagedPool, fri->FileNameLength + newExt.Length);
	RtlCopyMemory(newpath, fri->FileName, fri->FileNameLength);
	RtlCopyMemory(&newpath[fri->FileNameLength/2], newExt.Buffer, newExt.Length);

	PFILE_RENAME_INFORMATION newFri = ExAllocatePool(NonPagedPool, sizeof(FILE_RENAME_INFORMATION));

	RtlCopyMemory(newFri->FileName, newpath, fri->FileNameLength + newExt.Length);
	newFri->FileNameLength = fri->FileNameLength + newExt.Length;
	newFri->Flags = fri->Flags;
	newFri->ReplaceIfExists = fri->ReplaceIfExists;
	newFri->RootDirectory = fri->RootDirectory;

	Data->Iopb->Parameters.SetFileInformation.InfoBuffer = newFri;
	ExFreePool(fri);
	ExFreePool(newpath);

	Data->IoStatus.Information = IO_REPARSE;
	Data->IoStatus.Status = STATUS_REPARSE;
	FltSetCallbackDataDirty(Data);

	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, " new path %ws :::: ext %wZ\n", newpath,newExt));
	return TRUE;
}
*/