#pragma once
#ifndef IO_HEURISTICS_H
#define IO_HEURISTICS_H

#include <fltKernel.h>
#include <ntifs.h>
#include <wdm.h>

#include "DecisionTreeGlobal.h"
#include "DecisionTree.h"


/**
	IOTHold needs to be above 30 to allow for changes
**/
#define IOThold 30
#define MAXDELETEPNTS 50
#define MAXENTROPYPNTS 50
#define MAXPOINTS 100

//Constant for converting log(x) to log2(x)
#define M_LOG2E 1.44269504088896340736 //log2(e)

void initIOheuristics();
void IOhook(PCFLT_RELATED_OBJECTS FltObjects);
void hhook(PCFLT_RELATED_OBJECTS FltObjects);
void putProcess(Procmon* p);
int shannonEntropy(PCFLT_RELATED_OBJECTS FltObjects, PUNICODE_STRING f);

double approx_log(double x);
double power(double x, int exp);
double ln(double value);
double log2(double value);

#endif