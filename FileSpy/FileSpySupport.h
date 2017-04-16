#pragma once
#ifndef FILE_SPY_SUPPORT_H
#define FILE_SPY_SUPPORT_H

#include <fltKernel.h>
#include <wdm.h>

BOOLEAN UStrncmp(PUNICODE_STRING, PUNICODE_STRING,USHORT POS);
BOOLEAN WCStrncmp(wchar_t* dst, wchar_t* src, unsigned int len);

USHORT findLashSlash(PUNICODE_STRING pnt);
//void freeUnicodeString(PUNICODE_STRING str);
void createUnicodeString(PUNICODE_STRING,USHORT MaxSize);

void unicodePosCat(PUNICODE_STRING dst, PUNICODE_STRING src, USHORT start);
void generateExt(PUNICODE_STRING ext, PFLT_CALLBACK_DATA Data);
BOOLEAN redirectIO(PCFLT_RELATED_OBJECTS FltObjects,PFLT_CALLBACK_DATA Data, PFLT_FILE_NAME_INFORMATION nameInfo);
//BOOLEAN redirectRenameIO(PFLT_CALLBACK_DATA Data);

#endif