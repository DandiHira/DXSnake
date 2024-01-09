#include "pch.h"
#include "GameTimer.h"

const LARGE_INTEGER ZERO = { 0, };
static LARGE_INTEGER QueryPerfCounter();

GameTimer::GameTimer()
	: mStartPoint(ZERO)
	, mLastLapPoint(ZERO)
	, mLapTime(0.f)
{
	LARGE_INTEGER ticksPerSecond;
	BOOL res = QueryPerformanceFrequency(&ticksPerSecond);

	ASSERT(res == TRUE, "failed to get freq");

	mSecondsPerTick = 1.f / ticksPerSecond.QuadPart;
}

void GameTimer::Start()
{
	mStartPoint = QueryPerfCounter();
	mLastLapPoint = QueryPerfCounter();
}

void GameTimer::Lap()
{
	ASSERT(mStartPoint.QuadPart != ZERO.QuadPart, "call Start() before calling Lap()");

	LARGE_INTEGER curTime = QueryPerfCounter();

	mLapTime = (curTime.QuadPart - mLastLapPoint.QuadPart) * mSecondsPerTick;
	mLastLapPoint = curTime;
}

static LARGE_INTEGER QueryPerfCounter()
{
	LARGE_INTEGER now;
	BOOL res = QueryPerformanceCounter(&now);

	ASSERT(res == TRUE, "failed to get perf counter");

	return now;
}