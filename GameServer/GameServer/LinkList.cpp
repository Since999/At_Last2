#include "LinkList.h"

LinkList::LinkList()
{
	head = nullptr;
	tail = nullptr;
	count = 0;
}

LinkList::~LinkList()
{
	NodeDelete();
}

void LinkList::NodeDelete()
{
	if (head == nullptr)
	{
		return;
	}
	Node* cur = head;
	Node* del = nullptr;

	while (cur != nullptr)
	{
		del = cur;
		cur = cur->next;
		del->next = nullptr;
		delete del;
		del = nullptr;
	}

	head = nullptr;
	tail = nullptr;
	count = 0;
}

void LinkList::HeadInsert(AS_Node* node, float dis)
{
	Node* new_node = new Node;
	new_node->node = node;
	new_node->distance = dis;
	new_node->next = nullptr;

	new_node->next = head;
	head = new_node;
}

void LinkList::LastInsert(AS_Node* node, float dis)
{
	Node* new_node = new Node;
	new_node->node = node;
	new_node->distance = dis;
	new_node->next = nullptr;

	tail->next = new_node;
	tail = new_node;
}

void LinkList::DataInsert(AS_Node* node, float dis)
{
	if (head == nullptr)
		FirstHeadInsert(node, dis);
	else
	{
		if (head->distance > dis)
			HeadInsert(node, dis);
		else
			LastInsert(node, dis);
	}
	count++;
}

void LinkList::FirstHeadInsert(AS_Node* node, float dis)
{
	Node* new_node = new Node;
	new_node->node = node;
	new_node->distance = dis;
	new_node->next = nullptr;

	head = new_node;
	tail = new_node;
}