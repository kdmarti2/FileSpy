#pragma once
#ifndef IO_HEURISTICS_H
#define IO_HEURISTICS_H

#include <fltKernel.h>
#include <ntifs.h>
#include <wdm.h>

#include "DecisionTreeGlobal.h"
#include "DecisionTree.h"


/**
	IOTHold needs to be above 20 to allow for changes
**/
#define IOThold 20

void initIOheuristics();
void IOhook(PCFLT_RELATED_OBJECTS FltObjects);
void hhook(PCFLT_RELATED_OBJECTS FltObjects);
void putProcess(Procmon* p);
int shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects, PUNICODE_STRING f);

#endif