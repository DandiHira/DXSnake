#pragma once

#pragma once

typedef enum class eDirection
{
	LEFT = 0,
	UP,
	RIGHT,
	DOWN,
	NUM_DIRECTION
} eDirection;

const int X_MOVEMENTS[static_cast<int>(eDirection::NUM_DIRECTION)] = { -1, 0, 1, 0 };
const int Y_MOVEMENTS[static_cast<int>(eDirection::NUM_DIRECTION)] = { 0, -1, 0, 1 };
const eDirection OPPOSITES[static_cast<int>(eDirection::NUM_DIRECTION)] = { eDirection::RIGHT, eDirection::DOWN, eDirection::LEFT, eDirection::UP };

typedef enum class eGameStatus
{
	ALIVE = 0,
	DEAD
} eGameStatus;

enum class eCellInfo
{
	EMPTY,
	SNAKE,
	FOOD
};

enum class eMoveResult
{
	JUST_MOVE,
	ATE_FOOD
};
