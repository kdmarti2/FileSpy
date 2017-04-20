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
void inheritProcessNode(Procmon* parent, Procmon* child)
{
	if (!parent)
		return;

	if (!child)
		return;

	parent->delFiles += child->delFiles;
	parent->highEntropyFiles += child->highEntropyFiles;
	parent->lowSimilarityScore += child->lowSimilarityScore;
	parent->touchFiles += child->touchFiles;
	
	addLastSlist((snode**)&parent->IO, (snode*)child->IO);
	child->IO = 0;
}
/*
The litteral main trigger to this whole program
A very important function that triggers the heuristics and decision making of the minifilter
this will determine the very success and failure of this defense
Simple test this needs to be fixed to handle an actual process close
This CODE IS BUGGEDDDD
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
	//I have a child mark this parent as a ghost, move all of its states up to the most ancesteral non ghost
	if (c->child)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "del: I have a child %d\n", c->PID));
		c->ghost = 1;
		Procmon* p = c->parent;
		if (!p) 
		{
			c->ghost = 1;
		} else {
			//move all states to the parent
			Procmon* np = p;
			do {
				np = p;
				p = p->parent;
			} while (p);
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "del: %ld inherits ghost files from %ld\n", np->PID,c->PID));
			inheritProcessNode(np, c);
			//find the most last anscestor
		}
		addSortSlist((Idsnode**)&ProcessList, (Idsnode*)c); // add it back tp the list
	}
	else {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "del: I do not have a child %d\n", c->PID));
		//do I have a parent?
		Procmon* p = c->parent;
		Procmon* np = p;
		while (p)
		{
			np = p;
			p = p->next;
		}
		//np is the last most parent.
		if (np)
		{
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "oldest parent node %d\n", np->PID));
			//collapse merge and free the child linkage
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "del: %ld inherits files from %ld\n", np->PID, c->PID));
			inheritProcessNode(np, c);
			p = c->parent;
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "parent node %d\n", p->PID));
			Childproc* cpnt = (Childproc*)removeSortSlist((Idsnode**)&p->child, c->PID);
			if (cpnt)
			{
				ExFreePool(cpnt);
			}
			ExFreePool(c);
			//bugged
			while (p)
			{
				np = p->parent;
				if (!p->child && p->ghost)
				{
					if (!p->IO)
					{
						KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "ghost node removed %d\n", p->PID));
						removeSortSlist((Idsnode**)&ProcessList, p->PID);
						ExFreePool(p);
					}
					else {
						KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "ghost node removed has files %d\n", p->PID));
						removeSortSlist((Idsnode**)&ProcessList, p->PID);
						putProcess(p);
					}
					if (np) {
						Childproc* childpnt = (Childproc*)removeSortSlist((Idsnode**)&np->child, p->PID);
						if (childpnt)
						{
							ExFreePool(childpnt); //releasing the child pointer
						}
					}

				}
				p = np;
			}
		} else { //child has no parent
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "no parent node %d\n", c->PID));
			KeReleaseGuardedMutex(&DTMutex);
			putProcess(c);
			return;
		}
	}
	KeReleaseGuardedMutex(&DTMutex);
}