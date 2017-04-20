#pragma once
#ifndef PROC_TRACE_H
#define PROC_TRACE_H


#define PROCWLISTSIZE 8
/**
huge amount of guidance from this cite
*/
//http://www.adlice.com/making-an-antivirus-engine-the-guidelines/
#include <fltKernel.h>
#include <wdm.h>
#include <Ntddk.h>
#include "ComIface.h"

BOOLEAN procNameWhiteList(PUNICODE_STRING Name);

VOID CreateProcessNotifyEx(
	_Inout_   PEPROCESS Process,
	_In_      HANDLE ProcessId,
	_In_opt_  PPS_CREATE_NOTIFY_INFO CreateInfo
);

#endif