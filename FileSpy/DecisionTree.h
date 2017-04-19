#pragma once
#ifndef D_TREE_H
#define D_TREE_H

#include <fltKernel.h>
#include <wdm.h>

typedef struct Iddnode_t {
	struct Iddnode_t* next;
	struct Iddnode_t* prev;
	unsigned long long ID;
} Iddnode;

typedef struct dnode_t {
	struct dnode_t* next;
	struct dnode_t* prev;
} dnode;

typedef struct snode_t {
	struct snode_t* next;
} snode;

typedef struct Idsnode_t {
	struct Idsnode_t* next;
	unsigned long long ID;
} Idsnode;

//sorted double linked list
void addSortDlist(Iddnode** head, Iddnode* node);
Iddnode* removeSortDlist(Iddnode** head, unsigned long long ID);
Iddnode* findSortDlist(Iddnode** head, unsigned long long ID);

//non sorted double linked list
void addFirstDlist(dnode** head, dnode* node);
void addLastDlist(dnode** head, dnode* node);
dnode * removeDlist(dnode** head, dnode* node);

//sorted single linked list
void addSortSlist(Idsnode** head, Idsnode* node);
Idsnode* removeSortSlist(Idsnode** head, unsigned long long ID);
Idsnode* findSortSlist(Idsnode** head, unsigned long long ID);

//non sorted single linked list
void addFirstSlist(snode** head, snode* node);
void addLastSlist(snode** head, snode* node);
snode* removeSlist(snode** head, snode* node);

#endif


