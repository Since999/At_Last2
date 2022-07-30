#include "AstarAlgoritm.h"

AStarAI::AStarAI() 
{
	for (int i = 0; i < WORLD_HEIGHT; ++i)
		for (int j = 0; j < WORLD_WIDTH; ++j)
			as_node[i][j] = nullptr;

}

AStarAI::~AStarAI()
{
	/*
	for (int i = 0; i < WORLD_HEIGHT; ++i)
	{
		for (int j = 0; j < WORLD_WIDTH; ++j)
		{
			if (as_node[i][j] != NULL)
			{
				delete as_node[i][j];
				as_node[i][j] = NULL;
			}
		}
	}
	*/
}

void AStarAI::Delete()
{
	for (int i = start_y; i <= start_y + map_height; ++i)
	{
		for (int j = start_x; j <= start_x + map_width; ++j)
		{
			if (as_node[i][j] != nullptr)
			{
				delete as_node[i][j];
				as_node[i][j] = nullptr;
			}
		}
	}

	close_list.NodeDelete();
}

void AStarAI::Init()
{
	open_list.Init();

	for (int i = start_y; i <= start_y + map_height; ++i)
	{
		for (int j = start_x; j <= start_x + map_width; ++j)
		{
			if (as_node[i][j] == nullptr)
			{
				as_node[i][j] = new AS_Node;
				as_node[i][j]->Set_Start_Cost(0.0f);
				as_node[i][j]->Set_End_Cost(0.0f);
				as_node[i][j]->Set_X(j);
				as_node[i][j]->Set_Y(i);
				as_node[i][j]->Set_Map_Type(as_map.GetMapInfo(j, i));
				as_node[i][j]->Set_Parent(nullptr);
				memset(as_node[i][j]->near_node, NULL, (sizeof(AS_Node*) * 8));
			}
		}
	}

	Near_Node_Connect();
}

void AStarAI::New_Init()
{
	open_list.Init();

	for (int i = start_y; i <= start_y + map_height; ++i)
	{
		for (int j = start_x; j <= start_x + map_width; ++j)
		{
			as_node[i][j]->Set_Start_Cost(0.0f);
			as_node[i][j]->Set_End_Cost(0.0f);
			as_node[i][j]->Set_Parent(nullptr);
			memset(as_node[i][j]->near_node, NULL, (sizeof(AS_Node*) * 8));
		}
	}

	Near_Node_Connect();
}

void AStarAI::Near_Node_Connect()
{
	for (int i = start_y; i < start_y + map_height; ++i)
	{
		for (int j = start_x; j < start_x + map_width; ++j)
		{
			if (as_node[i][j] == nullptr) continue;

			if (i > start_y)
				as_node[i][j]->near_node[(int)Direction::UP] = as_node[i - 1][j];
			if (j > start_x)
				as_node[i][j]->near_node[(int)Direction::LEFT] = as_node[i][j - 1];
			if (i  < start_x + map_width-1)
				as_node[i][j]->near_node[(int)Direction::RIGHT] = as_node[i][j + 1];
			if (j < start_y + map_height-1)
				as_node[i][j]->near_node[(int)Direction::DOWN] = as_node[i + 1][j];
			if (i > start_y && j > start_x)
				as_node[i][j]->near_node[(int)Direction::UP_LEFT] = as_node[i - 1][j - 1];
			if (i > start_y && j < start_x + map_width-1)
				as_node[i][j]->near_node[(int)Direction::UP_RIGHT] = as_node[i - 1][j + 1];
			if (i < start_y + map_height && j > start_x)
				as_node[i][j]->near_node[(int)Direction::DOWN_LEFT] = as_node[i + 1][j - 1];
			if (i < start_y + map_height && j < start_x + map_width-1)
				as_node[i][j]->near_node[(int)Direction::DOWN_RIGHT] = as_node[i + 1][j + 1];
		}
	}
}

int AStarAI::Distance(int x1, int x2, int y1, int y2)
{
	int X = abs(x1 - x2);
	int Y = abs(y1 - y2);

	return X + Y;
}

stack<AS_Node*> AStarAI::AstartSearch(int s_x, int s_y, int e_x, int e_y)
{
	Init();
	//New_Init();

	AS_Node* node = nullptr;

	if (as_node[s_y][s_x] == nullptr)
	{
		cout << "�̰� ����־�!! \n";
		stack<AS_Node*> a;
		return a;
	}

	// ù ��� ����
	//as_lock.lock();
	as_node[s_y][s_x]->Set_Map_Type(AS_MAP_TYPE::START);
	as_node[s_y][s_x]->Set_Start_Cost(0.0f);

	int dis = Distance(s_x, e_x, s_y, e_y);

	as_node[s_y][s_x]->Set_End_Cost((float)dis);
	as_node[s_y][s_x]->Set_Total_Distance((float)dis, 0.0f);
	as_node[s_y][s_x]->Set_Open_List(true);
	as_node[s_y][s_x]->Set_Parent(nullptr);
	as_node[s_y][s_x]->Set_Close_List(false);
	open_list.Insert(as_node[s_y][s_x], (float)dis);
	//as_lock.unlock();

	// ���� �������� ��� ã�� ����
	while (open_list.Empty() == false)
	{
		// ������ ��� ã��
		node = open_list.Delete();

		node->Set_Open_List(false);
		node->Set_Close_List(true);

		// ���� ��Ͽ� ������ ��� �ֱ�
		close_list.DataInsert(node, node->Get_End_Cost());

		// ��ǥ ������ �������� ��� ��� ����
		if (node->Get_X() == e_x && node->Get_Y() == e_y)
		{
			stack<AS_Node*> s;
			AS_Node* cur = node;

			while (cur != nullptr)
			{
				s.push(cur);
				cur = cur->Get_Parent();
			}
			return s;
		}

		// ��ǥ�� �������� ���ߴٸ� �ֺ� 8�� Ž��, �����¿�, �밢�� 4����
		for (int i = 0; i < 8; ++i)
		{
			if (node->near_node[i] == nullptr)	// ����� ��尡 ����ִٸ�?
				continue;

			if (node->near_node[i]->Get_Map_Type() == AS_MAP_TYPE::WALL)	// ����� ��尡 ���࿡ ���̶��? ���� �����Ƿ� �н�
				continue;

			if (node->near_node[i]->Get_Close_List())	// ���� ����̶�� �̹� Ȯ�������Ƿ� �н�
				continue;

			if (node->near_node[i]->Get_Open_List() == false)	// �� ���� ������ ���¸���Ʈ�� ���� ��
			{
				// ������ ��带 ���� ����Ʈ�� �ְ�, �θ�� ����
				node->near_node[i]->Set_Open_List(true);
				node->near_node[i]->Set_Parent(node);

				// ������ ������ ����, �� ��ü��� ���ϱ�
				int parent_x = node->Get_X();
				int parent_y = node->Get_Y();
				int child_x = node->near_node[i]->Get_X();
				int child_y = node->near_node[i]->Get_Y();

				float adjdis = (float)sqrt((child_x - parent_x)*(child_x - parent_x) + (child_y - parent_y)*(child_y - parent_y));
				float enddis = (float)Distance(child_x, e_x, child_y, e_y);

				// ������ ����� ���� ����� ���������� �ش� ����� �Ÿ�, �׸��� ���� ���� ���� ����� �Ÿ��� ���Ѵ�.
				node->near_node[i]->Set_Start_Cost(node->Get_Start_Cost() + adjdis);
				node->near_node[i]->Set_End_Cost(enddis);
				node->near_node[i]->Set_Total_Distance(node->near_node[i]->Get_Start_Cost() + enddis);

				open_list.Insert(node->near_node[i], node->near_node[i]->Get_Total_Distance());
			}
			else																		// ������ �ְ� ���¸���Ʈ���� �ִ� ��
			{
				// ���ο� �� ���� �̹� �ִ� ����� ��
				// node�� ���� node���� ������������ �Ÿ��� ���� ��
				float adjdis = (float)sqrt((node->Get_X() - node->near_node[i]->Get_X())* (node->Get_X() - node->near_node[i]->Get_X()) + ((node->Get_Y() - node->near_node[i]->Get_Y())* (node->Get_Y() - node->near_node[i]->Get_Y())));
				float new_start_cost = adjdis + node->Get_Start_Cost();

				// ���ο� �� ����� �� ����ϸ� �ش� ��尡 �켱������ �Ǿ� ��
				// �ƴϸ� ������ ��
				// �׷��� ��� ����̵� �߰��ǰų� ��ġ�� ����� ������ ��� ��� ���� �ʿ�
				if (new_start_cost < node->near_node[i]->Get_Start_Cost())
				{
					node->near_node[i]->Set_Parent(node);	// ���� ����� �θ� node�� ����

					node->near_node[i]->Set_Start_Cost(new_start_cost);
					node->near_node[i]->Set_Total_Distance(new_start_cost + node->near_node[i]->Get_End_Cost());

					open_list.Update(node->near_node[i], node->near_node[i]->Get_Total_Distance());	// ����
				}
			}
		}
	}

	// ���Ⱑ ���� ��θ� ã�� ���� ��
	AS_Node* cur = close_list.GetNode();
	close_list.NodeDelete();

	stack<AS_Node*> closed_stack;

	while (cur != nullptr)
	{
		closed_stack.push(cur);
		cur = cur->Get_Parent();
	}

	return closed_stack;
}