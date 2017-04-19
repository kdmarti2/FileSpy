#pragma once
#ifndef D_TREE_GLOBAL_H
#define D_TREE_GLOBAL_H

#include <ntifs.h>
#include <wdm.h>

typedef struct IOtrace_t {
	struct IOtrace_t* next;
	PUNICODE_STRING realFile;// PUNICDOE_STRING
	PUNICODE_STRING shadowFile;// PUNICODE_STRING
} IOtrace;

typedef struct Childproc_t {
	struct Childproc_t* next;
	unsigned long long CPID;
	struct Procmon_t* cpnt;
} Childproc;

typedef struct Procmon_t {
	struct Procmon_t* next;
	unsigned long long PID;
	struct Procmon_t* parent;
	struct Childproc_t* child;
	struct IOtrace_t* IO;
	int ghost;
	unsigned long long trustpoints;
	unsigned long long delFiles;
	unsigned long long touchFiles;
	unsigned long long highEntropyFiles;
	unsigned long long lowSimilarityScore;
} Procmon;

#endif