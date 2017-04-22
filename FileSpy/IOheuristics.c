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
void IOhook(PCFLT_RELATED_OBJECTS FltObjects)
{
	UNREFERENCED_PARAMETER(FltObjects);
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
		if (shannonEntropy(FltObjects, IO))
		{
			proc->highEntropyFiles += 1;
		}
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
*/
BOOLEAN shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects,IOtrace* IO)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(IO);

	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "real File shannon Entroy %wZ\n", *IO->realFile));
	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "shdw File shannon Entroy %wZ\n", *IO->shadowFile));

	return FALSE;
}



