#include "IOheuristics.h"

Procmon* Pact; // contains all the process to test
IOtrace* IOact; //contains all the list of the trace IO need to apply
int _fltused;
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
	Procmon* proc = (Procmon*)removeSlist((snode**)&Pact, (snode*)Pact);
	KeReleaseGuardedMutex(&PMutex);

	if (!proc)
	{
		return;
	}

	IOtrace* IO = 0;
	for (IO = proc->IO;IO!=0;IO = IO->next)
	{
		int result = 0;
		result = shannonEntropy(FltObjects, IO->shadowFile);
		if (0 < result)
		{
			proc->delFiles += 1;
			continue;
		}
		if (result > 70000) //Checks for entropy to be above 7. In quick/informal testing this indicates encryption
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "shadowFile is encrypted\n"));
			if (IO->action) {
				proc->highEntropyFiles += 1;
			}
		}
	}

	if (!proc->touchFiles)
	{
		ExFreePool(proc);
		return;
	}


	double delRmScore = (double)proc->delFiles / proc->touchFiles * MAXDELETEPNTS;
	double entRmScore = (double)proc->highEntropyFiles / proc->touchFiles * MAXENTROPYPNTS;
	long score = MAXPOINTS - ((long)delRmScore + (long)entRmScore);
	
	if (score >= IOThold)
	{
		KeAcquireGuardedMutex(&IOMutex);
		ExFreePool(proc);
		addLastSlist((snode**)&IOact, (snode*)proc->IO);
		KeReleaseGuardedMutex(&IOMutex);
	}
	else {
		IOtrace* nIO = proc->IO;
		IO = proc->IO;
		proc->IO = 0;
		while (IO)
		{
			nIO = IO->next;

			ExFreePool(IO->realFile->Buffer);
			ExFreePool(IO->realFile);
			ExFreePool(IO->shadowFile->Buffer);
			ExFreePool(IO->shadowFile);
			ExFreePool(IO);

			IO = nIO;
		}
		ExFreePool(proc);
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
	double byte_count[256];
	ULONG i = 0;
	double count = 0.0;
	double entropy = 0.0;
	int length = 0;


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
	//RtlZeroMemory(byte_count, 256);
	for (i = 0; i < 256; i++)
	{
		byte_count[i] = 0.0;
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Print Shannon Entroy cleared byte_count[%ld]: %ld\n", i, byte_count[i])); //Test to see if buffer being cleared properly
	}
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
		//Read in byte & store # of each byte
		//http://stackoverflow.com/questions/25985005/calculating-entropy-in-c
		//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Print Shannon Entroy readBytes: %ld\n", readBytes)); //Test to see if reading from real file
		for (i = 0; i < readBytes; i++)
		{
			byte_count[(int)buf[i]]++;
			length++;
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Print Shannon Entroy (int)buf[i]: %ld\n", (int)buf[i])); //Check what's actually going in to byte_count
		}
		RtlZeroMemory(buf, 1024);
	} while (readBytes);
	
	//Calculate entropy, should match the given algorithm now (tested algorithm on another machine)
	for (i = 0; i < 256; i++)
	{
		if ((byte_count[i] != 0.0) && (length != 0))
		{
			count = (double) byte_count[i] / (double) length;
			entropy += -count * log2(count);
			//KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Print Shannon Entroy math: i = %ld; byte_count[i] = %ld; length = %ld; count %ld; entropy %ld\n", i, (int)byte_count[i], length, (int)count, (int)entropy));
		}
	}
	FltFreePoolAlignedWithTag(FltObjects->Instance, buf, 'shen');
	FltClose(FHANDLE);
	entropy = entropy * 10000; //Basically move decimal places over so they can be seen as ints
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Shannon Entroy result %ld\n", (int)entropy));
	return (int)entropy;
}

double log2(double value) {
	return ln(value) * M_LOG2E;
}


double ln(double value) {
	short reciprocal = 0;
	double x = 0;

	if (value > 2.0) {// Utilize ln(x) = -ln(1/x) for accuracy
		value = 1 / value;
		reciprocal = 1;
	}
	x = approx_log(value);

	if (reciprocal) { //Correct for the ln(1/x) = -ln(x)
		x = x * -1;
	}
	return x;
}

double power(double x, int exp) {
	int counter = exp;
	double result = x;
	if (exp == 0) { //x^0 = 1
		return 1;
	}
	while (counter > 1) { //Do exponentiation
		result = result*x;
		counter--;
	}
	return result;
}

double approx_log(double x) {

	double result = 0;
	int count = 1;
	while (count < 1000) { //The constant 1000 is a "precision" indicator, maybe change if a performance hit?
		result += power(-1, count + 1) * (power((x - 1), count) / count);
		count++;
	}
	return result;
}