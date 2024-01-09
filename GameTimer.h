#pragma once

using Second = float;
extern const LARGE_INTEGER ZERO;

class GameTimer
{
public:
	GameTimer();
	void Start();
	void Lap();

	inline Second GetLapTime() const
	{
		ASSERT(mStartPoint.QuadPart != ZERO.QuadPart, "call Start() before GetLapTime()");
		ASSERT(mLastLapPoint.QuadPart != ZERO.QuadPart, "call Lap() before GetLapTime()");

		return mLapTime;
	}

	GameTimer(const GameTimer& other) = delete;
	GameTimer& operator=(GameTimer& rhs) = delete;

private:
	LARGE_INTEGER mStartPoint;
	LARGE_INTEGER mLastLapPoint;
	float mSecondsPerTick;
	Second mLapTime;
};

