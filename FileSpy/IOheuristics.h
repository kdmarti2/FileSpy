#pragma once
#ifndef IO_HEURISTICS_H
#define IO_HEURISTICS_H

#include <fltKernel.h>
#include <ntifs.h>
#include <wdm.h>

#include "DecisionTreeGlobal.h"
#include "DecisionTree.h"


void initIOheuristics();
void IOhook(PCFLT_RELATED_OBJECTS FltObjects);
void hhook(PCFLT_RELATED_OBJECTS FltObjects);
void putProcess(Procmon* p);
BOOLEAN shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects, IOtrace* IO);


#endif