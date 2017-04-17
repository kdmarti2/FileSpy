#include "ComIface.h"

Procmon* ProcessList = 0;
KGUARDED_MUTEX DTMutex;

//https://github.com/Microsoft/Windows-driver-samples/blob/master/general/obcallback/driver/tdriver.c
void InitComIface()
{
	KeInitializeGuardedMutex(&DTMutex);
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
/*
Simple test this needs to be fixed to handle an actual process close
*/
void deleteProcess(unsigned long long cPID)
{
	if (!cPID)
		return;

	return;
	/*
	KeAcquireGuardedMutex(&DTMutex);
	Procmon* p = (Procmon*)removeSortSlist((Idsnode**)&ProcessList, cPID);
	if (p)
	{
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x8, "Deleted Process node %d\n", p->PID));
		ExFreePool(p);
	}
	KeReleaseGuardedMutex(&DTMutex);*/
}