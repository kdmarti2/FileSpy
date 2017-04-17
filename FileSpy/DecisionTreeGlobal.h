#pragma once
#ifndef D_TREE_GLOBAL_H
#define D_TREE_GLOBAL_H

//should we hash this for speed?
typedef struct extFileType_t {
	struct extFileType_t* next;
	char* ext;
} extFileType;

typedef struct IOtrace_t {
	struct IOtrace_t* next;
	unsigned word; //use bit wise. to record the action that had happen to it.  Time independed.
	char* filepath;
	char* shadowpath;
} IOtrace;

typedef struct Childproc_t {
	struct Childproc_t* next;
	unsigned long long CPID;
	struct Procmon_t* cpnt;
} Childproc;

typedef struct Procmon_t {
	struct Procmon_t* next;
	//struct Procmon_t* prev;
	unsigned long long PID;
	struct Procmon_t* parent;
	struct Childproc_t* child;
	struct IOtrace_t* IO;
	struct extFileType_t* outExtList;
	struct extFileType_t* inExtList;
	//sumary stats used for heuristics
	int ghost;
	unsigned long long trustpoints;
	unsigned long long delFiles;
	unsigned long long touchFiles;
	unsigned long long indiffExt;
	unsigned long long outdiffExt;
	unsigned long long highEntropyFiles;
	unsigned long long lowSimilarityScore;
} Procmon;

#endif