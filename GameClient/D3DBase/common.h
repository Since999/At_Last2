#pragma once

#include "protocol.h"
#include "Exception.h"
#include "Expover.h"
#include "Socket.h"

enum class ClientAnimationState : char
{
	IDLE,
	WALK,
	ATTACKED,
	WALK_AND_SHOOT,
	WALK_AND_RELOAD,
	DEAD
};

enum class ZombieAnimationState : char
{
	IDLE,
	SPAWN,
	WALK,
	ATTACK,
	ATTACKED,
	DEAD
};