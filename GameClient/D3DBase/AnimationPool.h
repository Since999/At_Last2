#pragma once
#include "stdafx.h"
#include <map>
class Animation;

class CAnimationPool
{
	std::map<std::string, Animation>animations;
public:
	CAnimationPool();
	Animation* GetAnimation(const std::string&);
};

