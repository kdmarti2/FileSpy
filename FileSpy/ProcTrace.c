#include "ProcTrace.h"
UNICODE_STRING wplist[] = {
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\explorer.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\System32\\SearchProtocolHost.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\SearchIndexer.exe"),
	RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume1\\Windows\\SearchFilterHost.exe")
};
BOOLEAN procNameWhiteList(PUNICODE_STRING Name) {
	unsigned int i = 0;
	for (i = 0;i < PROCWLISTSIZE;i++)
	{
		if (RtlEqualUnicodeString(&wplist[i], Name,TRUE))
		{
			return TRUE;
		}
	}
	return FALSE;
}
VOID CreateProcessNotifyEx(
	_Inout_   PEPROCESS Process,
	_In_      HANDLE ProcessId,
	_In_opt_  PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	//UNREFERENCED_PARAMETER(Process);
	//UNREFERENCED_PARAMETER(ProcessId);
	//UNREFERENCED_PARAMETER(CreateInfo);
	PUNICODE_STRING ProcName;
	SeLocateProcessImageName(Process, &ProcName);
	if (CreateInfo)
	{
		PUNICODE_STRING ParentProcName;
		PEPROCESS ParentHandle;
		PsLookupProcessByProcessId(CreateInfo->ParentProcessId, &ParentHandle);
		SeLocateProcessImageName(ParentHandle, &ParentProcName);
		//process created
		if (procNameWhiteList(ParentProcName)) {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "created (WHITELIST) ---> child:%wZ: %ld\n", ProcName, ProcessId));
			recordProcess(0, (unsigned long long)ProcessId);
		}
		else {
			KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "created parent: %wZ:%ld ---> child:%wZ: %ld\n", ParentProcName, CreateInfo->ParentProcessId, ProcName, ProcessId));
			recordProcess((unsigned long long)CreateInfo->ParentProcessId, (unsigned long long)ProcessId);
		}
	}
	else {
		KdPrintEx((DPFLTR_IHVDRIVER_ID, 0x08, "Exit %wZ: %ld\n", ProcName,ProcessId));
		deleteProcess((unsigned long long)ProcessId);//pretty much will trigger the whole minifilter
	}
}