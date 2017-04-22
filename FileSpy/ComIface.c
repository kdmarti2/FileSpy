#include "ComIface.h"

Procmon* ProcessList = 0;
IOtrace* ChangeList = 0;

KGUARDED_MUTEX DTMutex;
KGUARDED_MUTEX CLMutex;

//https://github.com/Microsoft/Windows-driver-samples/blob/master/general/obcallback/driver/tdriver.c
void InitComIface()
{
	KeInitializeGuardedMutex(&DTMutex);
	KeInitializeGuardedMutex(&CLMutex);
	initIOheuristics();
}
void destroyComIface()
{
	return;
}
Procmon* createProcessNode(unsigned long long PID, unsigned long long trustpoints)
{
	Procmon* n = ExAllocatePool(NonPagedPool, sizeof(Procmon));
	if (n)
	{
		RtlZeroMemory(n, sizeof(Procmon));
		n->PID = PID;
		n->trustpoints = trustpoints;
		return n;
	}
	return 0;
}
Childproc* createChildNode(unsigned long long PID, Procmon* n)
{
	Childproc* c = ExAllocatePool(NonPagedPool, sizeof(Childproc));
	if (c)
	{
		RtlZeroMemory(c, sizeof(Childproc));
		c->CPID = PID;
		c->cpnt = n;
		return c;
	}
	return 0;
}
//hook located in redirectIO
void recordIO(unsigned long long PID,PUNICODE_STRING realFile, PUNICODE_STRING shadowFile)
{
	//UNREFERENCED_PARAMETER(realFile);
	//UNREFERENCED_PARAMETER(shadowFile);
	//UNREFERENCED_PARAMETER(PID);
	if (!PID)
		return;

	if (!realFile)
		return;

	if (!shadowFile)
		return;

	KeAcquireGuardedMutex(&DTMutex);

	Procmon* p = (Procmon*)findSortSlist((Idsnode**)&ProcessList, PID);
	if (!p)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Process Node not found, %ld creating...\n", PID));
		p = createProcessNode(PID, MAXPOINTS);
		if (!p)
		{
			KeReleaseGuardedMutex(&DTMutex);
			return;
		}
		addSortSlist((Idsnode**)&ProcessList, (Idsnode*)p); //nodes not there you have to add it
	}

	IOtrace* nIO = ExAllocatePool(NonPagedPool, sizeof(IOtrace));
	nIO->realFile = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	RtlCopyMemory(nIO->realFile, realFile, sizeof(UNICODE_STRING));
	nIO->realFile->Buffer = (PWCH)ExAllocatePool(NonPagedPool, realFile->MaximumLength);
	RtlCopyMemory(nIO->realFile->Buffer, realFile->Buffer, realFile->Length);

	nIO->shadowFile = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	RtlCopyMemory(nIO->shadowFile, shadowFile, sizeof(UNICODE_STRING));
	nIO->shadowFile->Buffer = (PWCH)ExAllocatePool(NonPagedPool, shadowFile->MaximumLength);
	RtlCopyMemory(nIO->shadowFile->Buffer, shadowFile->Buffer, shadowFile->Length);

	//increment the touched files
	p->touchFiles += 1;

	addFirstSlist((snode**)&p->IO, (snode*)nIO);

	KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "added IOTrace %ld\n %wZ\n %wZ\n", PID, *(nIO->realFile), *(nIO->shadowFile)));
	KeReleaseGuardedMutex(&DTMutex);
}
void recordProcess(unsigned long long pPID, unsigned long long cPID)
{
	if (!cPID)
		return;

	KeAcquireGuardedMutex(&DTMutex);
	if (!pPID)
	{
		//no parent.
		Procmon* n = createProcessNode(cPID,MAXPOINTS);
		if (!n)
		{
			KeReleaseGuardedMutex(&DTMutex);
			return;
		}
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "created Process node %d\n", n->PID));
		addSortSlist((Idsnode**)&ProcessList, (Idsnode*)n);
		KeReleaseGuardedMutex(&DTMutex);
		return;
	}
	else {
		Procmon* pnode = (Procmon*)findSortSlist((Idsnode**)&ProcessList, pPID);
		Procmon* cnode = createProcessNode(cPID,MAXPOINTS);
		if (!cnode)
		{
			KeReleaseGuardedMutex(&DTMutex);
			return;
		}
		Childproc* childpnt = createChildNode(cPID, cnode);
		if (!childpnt)
		{
			addSortSlist((Idsnode**)&ProcessList, (Idsnode*)cnode);
			KeReleaseGuardedMutex(&DTMutex);
			return;
		}
		//Parentnode exists
		if (pnode)
		{
			cnode->parent = pnode;
			cnode->trustpoints = pnode->trustpoints;
			addSortSlist((Idsnode**)&pnode->child, (Idsnode*)childpnt);
			addSortSlist((Idsnode**)&ProcessList, (Idsnode*)cnode);
		}
		//parentnode does not exist
		else {
			pnode = (Procmon*)createProcessNode(pPID,MAXPOINTS);
			if (!pnode)
			{
				ExFreePool(childpnt);
			}
			else {
				addSortSlist((Idsnode**)&pnode->child, (Idsnode*)childpnt);
			}
			cnode->parent = pnode;
			addSortSlist((Idsnode**)&ProcessList, (Idsnode*)cnode);
		}
	}
	KeReleaseGuardedMutex(&DTMutex);
}
void inheritProcessNode(Procmon* child)
{
	if (!child)
		return;

	Procmon * p = child->parent;
	Procmon * np = p;
	while (p)
	{
		np = p;
		p = p->parent;
	}
	if (np)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Inherit PID:%ld ->> %ld\n", child->PID,np->PID));
		addLastSlist((snode**)&np->IO, (snode*)child->IO);
		np->touchFiles += child->touchFiles;
		child->IO = 0;
	}
}
/*
The litteral main trigger to this whole program
A very important function that triggers the heuristics and decision making of the minifilter
this will determine the very success and failure of this defense
Simple test this needs to be fixed to handle an actual process close
This CODE IS Fixed.
*/
void deleteProcess(unsigned long long cPID)
{
	if (!cPID)
		return;

	KeAcquireGuardedMutex(&DTMutex);
	Procmon* c = (Procmon*)removeSortSlist((Idsnode**)&ProcessList, cPID);
	if (!c)
	{
		KeReleaseGuardedMutex(&DTMutex);
		return;
	}
	if (c->parent && c->child)
	{
		inheritProcessNode(c);
		c->ghost = 1;
		addSortSlist((Idsnode**)&ProcessList, (Idsnode*)c);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "GHOST1 \nPID:%ld\n", c->PID));
	}
	else if (!c->parent && c->child) {
		c->ghost = 1;
		addSortSlist((Idsnode**)&ProcessList, (Idsnode*)c);
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "GHOST2 \nPID:%ld\n", c->PID));
	}
	else if (!c->parent && !c->child) {
		c->next = 0;
		if (c->IO)
		{
			putProcess(c);
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "added proc \nPID:%ld\n", c->PID));
		}
		else {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "NO IO Deleting\nPID:%ld\n", c->PID));
			ExFreePool(c);
		}
	}
	else if (c->parent && !c->child) {
		inheritProcessNode(c); // this moves the IO
		Procmon* p = c->parent;
		Childproc* childpnt = (Childproc*)removeSortSlist((Idsnode**)&p->child, c->PID);
		if (childpnt)
		{
			ExFreePool(childpnt);
		}
		ExFreePool(c);
		c = p;
		while (c)
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "LOOP TEST\n"));
			childpnt = 0;
			p = c->parent;
			if (p && c->ghost && !c->child) {
				removeSortSlist((Idsnode**)&ProcessList, c->PID);
				if (c->IO)
					inheritProcessNode(c);

				childpnt = (Childproc*)removeSortSlist((Idsnode**)&p->child, c->PID);
				if (childpnt)
				{
					ExFreePool(childpnt);
				}
				ExFreePool(c);
			}
			else if (!p && c->ghost && !c->child) {
				removeSortSlist((Idsnode**)&ProcessList, c->PID);
				if (c->IO)
				{
					KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "added proc2 \nPID:%ld\n", c->PID));
					putProcess(c);
				}
				else {
					ExFreePool(c);
				}
			}
			c = p;
		} 
	}
	KeReleaseGuardedMutex(&DTMutex);
}