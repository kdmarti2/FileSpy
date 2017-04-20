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
	//make the changes here
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
	Procmon* pnt = (Procmon*)removeSlist((snode**)&Pact, (snode*)Pact);
	if (!pnt)
	{
		KeReleaseGuardedMutex(&PMutex);
		return;
	}



	KeReleaseGuardedMutex(&PMutex);
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
BOOLEAN shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects,IOtrace* IO)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(IO);
	return TRUE;
}



