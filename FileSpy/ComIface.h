#pragma once
#ifndef COM_I_FACE_H
#define COM_I_FACE_H

#include "DecisionTreeGlobal.h"
#include "DecisionTree.h"
#include "IOheuristics.h"
#define MAXPOINTS 100
void InitComIface();
//void DestroyComIface();

//creating nodes
Procmon* createProcessNode(unsigned long long PID, unsigned long long trustpoints);
Childproc* createChildNode(unsigned long long PID, Procmon* n);

//IO interactions need to make an initToSetUpSemaphores
void recordProcess(unsigned long long pPID, unsigned long long cPID);
void deleteProcess(unsigned long long cPID);
void recordIO(unsigned long long PID,PUNICODE_STRING realFile, PUNICODE_STRING shadowFile);

#endif