#include "FileSpySupport.h"

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
					if (!NT_SUCCESS(s))
						break;
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
					KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Read %ld readoffset: %ld writeoffset: %ld\n", readBytes,Pos.LowPart,oldPos.LowPart));
				} while (readBytes);
				FltFreePoolAlignedWithTag(FltObjects->Instance, buf, 'read');
				FltClose(OUTHANDLE);
			}
		}
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
