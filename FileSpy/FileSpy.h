#pragma once
#ifndef FILE_SPY_MINIFILTER_H
#define FILE_SPY_MINIFILTER_H

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <Ntstrsafe.h>
#include <wdm.h>
#include <ntifs.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
FileSpyInstanceSetup(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
	_In_ DEVICE_TYPE VolumeDeviceType,
	_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
FileSpyInstanceTeardownStart(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
FileSpyInstanceTeardownComplete(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
FileSpyUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
FileSpyInstanceQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FileSpyPreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);
FLT_POSTOP_CALLBACK_STATUS
FileSpyPostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);
FLT_PREOP_CALLBACK_STATUS
FileSpyPreSetInfo(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);
FLT_POSTOP_CALLBACK_STATUS
FileSpyPostSetInfo(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);
VOID
FileSpyOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);
FLT_PREOP_CALLBACK_STATUS
FileSpyPreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
);

BOOLEAN
FileSpyDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

EXTERN_C_END


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FileSpyUnload)
#pragma alloc_text(PAGE, FileSpyInstanceQueryTeardown)
#pragma alloc_text(PAGE, FileSpyInstanceSetup)
#pragma alloc_text(PAGE, FileSpyInstanceTeardownStart)
#pragma alloc_text(PAGE, FileSpyInstanceTeardownComplete)
#endif

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE,
	0,
	FileSpyPreCreate,
	FileSpyPostCreate},
	{IRP_MJ_SET_INFORMATION,
	0,
	FileSpyPreSetInfo,
	FileSpyPostSetInfo},
	{ IRP_MJ_OPERATION_END }
};
CONST FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                          //  Operation callbacks

	FileSpyUnload,                           //  MiniFilterUnload

	FileSpyInstanceSetup,                    //  InstanceSetup
	FileSpyInstanceQueryTeardown,            //  InstanceQueryTeardown
	FileSpyInstanceTeardownStart,            //  InstanceTeardownStart
	FileSpyInstanceTeardownComplete,         //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};
#endif
