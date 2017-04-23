#include "IOheuristics.h"

Procmon* Pact; // contains all the process to test
IOtrace* IOact; //contains all the list of the trace IO need to apply

KGUARDED_MUTEX IOMutex;
KGUARDED_MUTEX PMutex;
/*
	initialies callback functions to redirect into create
	registers simaphores
*/
void initIOheuristics()
{
	KeInitializeGuardedMutex(&IOMutex);
	KeInitializeGuardedMutex(&PMutex);
	Pact = 0;
	IOact = 0;
}
void overWriteFile(PCFLT_RELATED_OBJECTS FltObjects,PUNICODE_STRING rFile, PUNICODE_STRING sFile)
{
	OBJECT_ATTRIBUTES objATT;
	HANDLE SHANDLE;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	PFILE_OBJECT sFileObject;

	InitializeObjectAttributes(&objATT, sFile, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	status = FltCreateFileEx(
		FltObjects->Filter,
		FltObjects->Instance,
		&SHANDLE,
		&sFileObject,
		FILE_READ_DATA | DELETE,
		&objATT,
		&ioStatusBlock,
		(PLARGE_INTEGER)NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_DELETE_ON_CLOSE,
		NULL,
		0,
		IO_IGNORE_SHARE_ACCESS_CHECK
	);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "failed shadow File %wZ\n", sFile));
		return;
	}
	HANDLE RHANDLE;
	IO_STATUS_BLOCK ioStatBlock;
	OBJECT_ATTRIBUTES objATT2;
	PFILE_OBJECT rFileObject;
	InitializeObjectAttributes(&objATT2, rFile, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	status = FltCreateFileEx(
		FltObjects->Filter,
		FltObjects->Instance,
		&RHANDLE,
		&rFileObject,
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
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "failed real File %wZ\n", sFile));
		FltClose(SHANDLE);
		return;
	}
	unsigned char* buf = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, 1024, 'IOup');
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
			sFileObject,
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
			rFileObject,
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
	FltFreePoolAlignedWithTag(FltObjects->Instance, buf, 'IOup');
	FltClose(SHANDLE);
	FltClose(RHANDLE);
}
void deleteFile(PCFLT_RELATED_OBJECTS FltObjects, PUNICODE_STRING sFile)
{
	OBJECT_ATTRIBUTES objATT;
	HANDLE SHANDLE;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	PFILE_OBJECT sFileObject;

	InitializeObjectAttributes(&objATT, sFile, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	status = FltCreateFileEx(
		FltObjects->Filter,
		FltObjects->Instance,
		&SHANDLE,
		&sFileObject,
		FILE_READ_DATA | DELETE,
		&objATT,
		&ioStatusBlock,
		(PLARGE_INTEGER)NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_DELETE_ON_CLOSE,
		NULL,
		0,
		IO_IGNORE_SHARE_ACCESS_CHECK
	);

	if (NT_SUCCESS(status))
		FltClose(SHANDLE);
}
void IOhook(PCFLT_RELATED_OBJECTS FltObjects)
{
	KeAcquireGuardedMutex(&IOMutex);
	if (!IOact)
	{
		KeReleaseGuardedMutex(&IOMutex);
		return;
	}
	IOtrace* IO = (IOtrace*)removeSlist((snode**)&IOact,(snode*)IOact);
	while (IO)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "change real File %wZ\n", *IO->realFile));
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "change shdw File %wZ\n", *IO->shadowFile));

		if (IO->action)
			overWriteFile(FltObjects, IO->realFile, IO->shadowFile);
		else
			deleteFile(FltObjects, IO->shadowFile);

		ExFreePool(IO->realFile->Buffer);
		ExFreePool(IO->shadowFile->Buffer);
		ExFreePool(IO->realFile);
		ExFreePool(IO->shadowFile);
		ExFreePool(IO);

		IO = (IOtrace*)removeSlist((snode**)&IOact, (snode*)IOact);
	}

	KeReleaseGuardedMutex(&IOMutex);
	return;
}
void hhook(PCFLT_RELATED_OBJECTS FltObjects)
{
	UNREFERENCED_PARAMETER(FltObjects);
	//do one process at a time 

	KeAcquireGuardedMutex(&PMutex);
	if (!Pact)
	{
		KeReleaseGuardedMutex(&PMutex);
		return;
	}
	//do IO heuristics
	Procmon* proc = (Procmon*)removeSlist((snode**)&Pact, (snode*)Pact);
	KeReleaseGuardedMutex(&PMutex);

	if (!proc)
	{
		return;
	}

	IOtrace* IO = proc->IO;
	while (IO)
	{
		shannonEntropy(FltObjects, IO->realFile);
		shannonEntropy(FltObjects, IO->shadowFile);
		IO = IO->next;
	}

	/*This part will allow the change to go through on teh next IRP_MJ_CREATE*/
	if (proc->trustpoints >= IOThold)
	{
		KeAcquireGuardedMutex(&IOMutex);
		addLastSlist((snode**)&IOact, (snode*)proc->IO);
		proc->IO = 0;
		ExFreePool(proc);
		KeReleaseGuardedMutex(&IOMutex);
	}
	return;
}
void putProcess(Procmon* p)
{
	if (!p)
		return;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Put proccess node for testing %d\n", p->PID));
	KeAcquireGuardedMutex(&PMutex);
	addLastSlist((snode**)&Pact, (snode*)p);
	KeReleaseGuardedMutex(&PMutex);
}

/*
This is the shannon entropy function seth you can play in here
//http://stackoverflow.com/questions/25985005/calculating-entropy-in-c
*/
//estimation of a log function
int log2f(int num)
{
	return num;
}
int shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects,PUNICODE_STRING f)
{
	int avg_entropy = 0;
	int rep = 0;
	OBJECT_ATTRIBUTES objATT;
	HANDLE FHANDLE;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status;
	PFILE_OBJECT fFileObject;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "shannon Entroy %wZ\n", *f));
	InitializeObjectAttributes(&objATT, f, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	status = FltCreateFileEx(
		FltObjects->Filter,
		FltObjects->Instance,
		&FHANDLE,
		&fFileObject,
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
		return -1;
	// do math

	unsigned char* buf = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, 1024, 'shen');
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
			fFileObject,
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
			fFileObject,
			&oldPos,
			readBytes,
			buf,
			FLTFL_IO_OPERATION_NON_CACHED,
			0,
			0,
			0
		);
		oldPos.QuadPart += readBytes;

		//Entropy
		/*
		http://stackoverflow.com/questions/25985005/calculating-entropy-in-c
		ULONG i = 0;
		int count =0.0;
		int entropy = 0.0;
		for(i = 0;i < readBytes;i++)
		{
			if (buf[i])
			{
				count = buf[i] / readBytes;
				entropy += -count * log2f(count);
			}
		}
		avg_entropy += entropy;
		rep++; */
		RtlZeroMemory(buf, 1024);
	} while (readBytes);
	FltFreePoolAlignedWithTag(FltObjects->Instance, buf, 'shen');
	FltClose(FHANDLE);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "shannon Entroy result %ld\n", avg_entropy / rep));
	return 0;
}



