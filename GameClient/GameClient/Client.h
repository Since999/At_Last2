#pragma once

#include "common.h"

class Client
{
public:
	int _id;
	char Name[MAX_NAME_SIZE];
	PlayerType _type;
	PlayerState _state;
	int hp, shp;
	int maxhp, maxshp;
	float x, z;
	float infection;
	float mx, mz;		// 마우스 클릭 위치

public:
	Client();
	~Client();


};