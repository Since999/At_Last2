#pragma once

#include "protocol.h"
#include "Exception.h"
#include "Expover.h"
#include "Socket.h"

#include <random>
#include <stack>
#include "Timer.h"

const int START_WORLD_WIDTH = 60;
const int START_WORLD_HEIGHT = 60;

const int ROAD_WORLD_WIDTH = 180;
const int ROAD_WORLD_HEIGHT = 24;

const int BASE_WORLD_WIDTH = 120;
const int BASE_WORLD_HEIGHT = 120;

const int END_WORLD_WIDTH = 240;
const int END_WORLD_HEIGHT = 120;

const float CAMERA_WIDTH = 30.0;
const float CAMERA_HEIGHT = 18.0f;

enum class MoveResult : char
{
	MOVE,
	ARRIVE,
	COLLIED,
	FAIL
};