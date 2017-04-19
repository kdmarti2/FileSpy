#include "IOheuristics.h"

Procmon* Pact; // contains all the process to test
IOtrace* IOact; //contains all the list of the trace IO need to apply

KGUARDED_MUTEX IOMutex;
/*
	initialies callback functions to redirect into create
	registers simaphores
*/
void initIOheuristics()
{
	KeInitializeGuardedMutex(&IOMutex);
	Pact = 0;
	IOact = 0;
}
void IOhook(PCFLT_RELATED_OBJECTS FltObjects)
{
	UNREFERENCED_PARAMETER(FltObjects);
	//make changes to the system
	return;
}
void hhook(PCFLT_RELATED_OBJECTS FltObjects)
{
	UNREFERENCED_PARAMETER(FltObjects);
	//do one process at a time 
	return;
}
void putProcess(Procmon* p)
{
	if (!p)
		return;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Put proccess node for testing %d\n", p->PID));
	KeAcquireGuardedMutex(&IOMutex);
	//addLastSlist((snode**)&Pact, (snode*)p);
	KeReleaseGuardedMutex(&IOMutex);
}
BOOLEAN shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects,IOtrace* IO)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(IO);
	return TRUE;
}



