#pragma once

#include "AS_Node.h"

struct Node
{
	AS_Node* node;
	float distance;
	Node* next;
};

class LinkList
{
private:
	Node* head;
	Node* tail;
	int count;

public:
	LinkList();
	~LinkList();
	void HeadInsert(AS_Node* node, float dis);
	void LastInsert(AS_Node* node, float dis);
	void DataInsert(AS_Node* node, float dis);
	void FirstHeadInsert(AS_Node* node, float dis);
	AS_Node* GetNode() { return head->node; };
	void NodeDelete();
};