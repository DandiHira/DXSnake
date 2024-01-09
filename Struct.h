#pragma once
#include "types.h"

typedef struct Coordinate
{
	int x;
	int y;

	Coordinate()
		: x(0)
		, y(0)
	{
	}

	Coordinate(int _x, int _y)
		: x(_x)
		, y(_y)
	{
	}

	bool operator==(const Coordinate& rhs)
	{
		return x == rhs.x && y == rhs.y;
	}

	Coordinate operator=(Coordinate rhs)
	{
		x = rhs.x;
		y = rhs.y;

		return *this;
	}
} Pos;