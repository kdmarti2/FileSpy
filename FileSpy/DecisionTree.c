#include "DecisionTree.h"

//sorted double linked list
//needs to be tested.
void addSortDlist(Iddnode** head, Iddnode* node) {
	if (!head)
		return;

	if (!node)
		return;

	unsigned long long cid = node->ID;
	if (cid <= 0)
		return;

	if (!(*head))
	{
		node->next = NULL;
		node->prev = NULL;
		*head = node;
		return;
	}

	//if the head is less than the pid of node
	if (cid >= (*head)->ID)
	{
		node->next = *head;
		node->prev = (*head)->prev;
		(*head)->prev = node;
		*head = node;
		return;
	}
	//insert into the list
	Iddnode* ppnt = (*head);
	Iddnode* cpnt = (*head)->next;

	while (cpnt)
	{
		if (cid >= cpnt->ID)
		{
			break;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	ppnt->next = node;
	node->next = cpnt;
	node->prev = ppnt;
	if (cpnt)
		cpnt->prev = node;
} // needs te
  //needs to be tested
Iddnode* removeSortDlist(Iddnode** head, unsigned long long ID)
{
	if (!head)
		return 0;

	if (!(*head))
		return 0;

	if ((*head)->ID == ID) {
		Iddnode* node = *head;
		(*head) = node->next;
		if (*head)
			(*head)->prev = node->prev;

		return node;
	}

	Iddnode* cpnt = (Iddnode*)(*head)->next;
	Iddnode* ppnt = (Iddnode*)(*head);

	while (cpnt)
	{
		if (cpnt->ID == ID)
		{
			break;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	if (cpnt)
	{
		Iddnode* node = cpnt;
		ppnt->next = cpnt->next;
		if (cpnt->next) {
			cpnt->next->prev = ppnt;
		}
		return node;
	}
	return 0;
}
//needs to be tested
Iddnode* findSortDlist(Iddnode** head, unsigned long long ID)
{
	if (!head)
		return 0;

	if (ID <= 0)
		return 0;

	//make a better search algorithm
	Iddnode* cpnt = (*head);
	while (cpnt)
	{
		if (cpnt->ID == ID)
		{
			return cpnt;
		}
		cpnt = cpnt->next;
	}
	return cpnt;
}

//non sorted double linked list
//needs to be tested
void addFirstDlist(dnode** head, dnode* node)
{
	//make sure that something was submitted
	if (!head)
		return;
	//make sure a node was submitted
	if (!node)
		return;

	//insert the node to the beginning of the double linked list
	dnode* cpnt = *head;
	*head = node;
	(*head)->next = cpnt;

	// if cpnt is not null the prev of cpnt needs to be over written as 
	// the head needs to have the prev of cpn just incase the head prev is nut null.
	if (cpnt)
	{
		(*head)->prev = cpnt->prev;
		cpnt->prev = (*head);
	}

}
//needs to be tested
void addLastDlist(dnode** head, dnode* node)
{
	//make sure that something was submitted as the head node
	if (!head)
		return;

	//make sure a node was submitted
	if (!node)
		return;

	//if the head is null add it as the head
	if (!(*head))
	{
		*head = node;
		(*head)->next = NULL;
		(*head)->prev = NULL;
	}

	//find the last node
	dnode* cpnt = *head;
	while (cpnt->next)
	{
		cpnt = cpnt->next;
	}
	//add to the last of the linked list of a non sorted list
	cpnt->next = node;
	node->prev = cpnt;
	node->next = NULL;
}
//needs to be tested
dnode* removeDlist(dnode** head, dnode* node)
{
	if (!head)
		return 0;

	if (!node)
		return 0;

	if (!(*head))
		return 0;

	dnode* cpnt = *head;
	dnode* ppnt = (*head)->prev;

	while (cpnt)
	{
		//if the node address is the same as the node to submit then it is the same node
		if (cpnt == node)
		{
			break;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	if (!cpnt)
		return 0;

	if (ppnt)
	{
		dnode* n = cpnt;
		ppnt->next = cpnt->next;
		if (cpnt->next)
		{
			cpnt->next->prev = ppnt;
		}
		n->next = NULL;
		n->prev = NULL;
		return n;
	}
	else {
		dnode* n = cpnt;
		(*head) = cpnt->next;
		if (cpnt->next)
			(*head)->prev = 0;

		return n;
	}
}

//sorted single linked list
//needs to be tested
void addSortSlist(Idsnode** head, Idsnode* node)
{
	if (!head)
		return;

	if (!node)
		return;

	if (node->ID <= 0)
		return;

	if (!(*head))
	{
		*head = node;
		return;
	}
	Idsnode* ppnt = 0;
	Idsnode* cpnt = *head;

	while (cpnt)
	{
		if (node->ID >= cpnt->ID)
		{
			break;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	if (ppnt)
	{
		ppnt->next = node;
		node->next = cpnt;
	}
	else { //this can only happen if the node is greater than the head
		node->next = *head;
		*head = node;
	}
}
//needs to be tested
Idsnode* removeSortSlist(Idsnode** head, unsigned long long ID)
{
	if (!head)
		return 0;

	if (ID <= 0)
		return 0;

	if (!(*head))
		return 0;

	Idsnode* ppnt = NULL;
	Idsnode* cpnt = *head;
	while (cpnt) {

		if (cpnt->ID == ID)
		{
			Idsnode* npnt = cpnt;
			if (ppnt)
			{
				ppnt->next = cpnt->next;
			}
			else {
				*head = cpnt->next;
			}
			npnt->next = 0;
			return npnt;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	return 0;
}
//needs to be developed
Idsnode* findSortSlist(Idsnode** head, unsigned long long ID)
{
	if (!head)
		return 0;

	if (ID <= 0)
		return 0;

	Idsnode* cpnt = *head;
	while (cpnt)
	{
		if (cpnt->ID == ID)
		{
			return cpnt;
		}
		cpnt = cpnt->next;
	}
	return 0;
}

//non sortded single linked list
//needs to be developed
void addFirstSlist(snode** head, snode* node)
{
	if (!head)
		return;

	if (!node)
		return;

	snode* cpnt = *head;
	*head = node;
	node->next = cpnt;
}
//needs to be developed
void addLastSlist(snode** head, snode* node)
{
	if (!head)
		return;

	if (!node)
		return;

	if (!(*head))
	{
		*head = node;
		return;
	}
	snode* cpnt = *head;
	while (cpnt->next)
	{
		cpnt = cpnt->next;
	}
	cpnt->next = node;
	return;
}
//needs to be tested
snode* removeSlist(snode** head, snode* node)
{
	if (!head)
		return 0;

	if (!(*head))
		return 0;

	if (!node)
		return 0;

	snode* ppnt = NULL;
	snode* cpnt = *head;
	while (cpnt)
	{
		if (cpnt == node)
		{
			break;
		}
		ppnt = cpnt;
		cpnt = cpnt->next;
	}
	if (!cpnt)
		return 0;

	if (!ppnt)
	{
		*head = cpnt->next;
		return cpnt;
	}
	else {
		ppnt->next = cpnt->next;
		return cpnt;
	}
}