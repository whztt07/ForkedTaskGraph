// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "windows.h"


/** Contains CPU utilization data. */
struct FCPUTime
{
	/** Initialization constructor. */
	FCPUTime( float InCPUTimePct, float InCPUTimePctRelative ) 
		: CPUTimePct( InCPUTimePct )
		, CPUTimePctRelative( InCPUTimePctRelative )
	{}

	FCPUTime& operator+=( const FCPUTime& Other )
	{
		CPUTimePct += Other.CPUTimePct;
		CPUTimePctRelative += Other.CPUTimePctRelative;
		return *this;
	}

	/** Percentage CPU utilization for the last interval. */
	float CPUTimePct;

	/** Percentage CPU utilization for the last interval relative to one core, 
	 ** so if CPUTimePct is 8.0% and you have 6 core this value will be 48.0%. */
	float CPUTimePctRelative;
};


/**
* Generic implementation for most platforms
**/
struct CORE_API FGenericPlatformTime
{

	/** Updates CPU utilization, called through a delegate from the Core ticker. */
	static bool UpdateCPUTime( float DeltaTime )
	{
		return false;
	}

	/**
	 * @return structure that contains CPU utilization data.
	 */
	static FCPUTime GetCPUTime()
	{
		return FCPUTime( 0.0f, 0.0f );
	}

	/**
	* @return seconds per cycle.
	*/
	static double GetSecondsPerCycle()
	{
		return SecondsPerCycle;
	}
	/** Converts cycles to milliseconds. */
	static float ToMilliseconds( const unsigned int Cycles )
	{
		return (float)double( SecondsPerCycle * 1000.0 * Cycles );
	}

	/** Converts cycles to seconds. */
	static float ToSeconds( const unsigned int Cycles )
	{
		return (float)double( SecondsPerCycle * Cycles );
	}
	/**
	 * @return seconds per cycle.
	 */
	static double GetSecondsPerCycle64();
	/** Converts cycles to milliseconds. */
	static double ToMilliseconds64(const unsigned long long Cycles)
	{
		return ToSeconds64(Cycles) * 1000.0;
	}

	/** Converts cycles to seconds. */
	static double ToSeconds64(const unsigned long long Cycles)
	{
		return GetSecondsPerCycle64() * double(Cycles);
	}

protected:

	static double SecondsPerCycle;
	static double SecondsPerCycle64;
};

struct CORE_API FWindowsPlatformTime
	: public FGenericPlatformTime
{
	static double InitTiming();

	static __forceinline double Seconds()
	{
		::LARGE_INTEGER Cycles;
		::QueryPerformanceCounter(&Cycles);

		// add big number to make bugs apparent where return value is being passed to float
		return Cycles.QuadPart * GetSecondsPerCycle() + 16777216.0;
	}

	static __forceinline unsigned int Cycles()
	{
		::LARGE_INTEGER Cycles;
		::QueryPerformanceCounter(&Cycles);
		return (int)Cycles.QuadPart;
	}

	static __forceinline unsigned long long Cycles64()
	{
		::LARGE_INTEGER Cycles;
		QueryPerformanceCounter(&Cycles);
		return Cycles.QuadPart;
	}


	static void SystemTime(int& Year, int& Month, int& DayOfWeek, int& Day, int& Hour, int& Min, int& Sec, int& MSec);
	static void UtcTime(int& Year, int& Month, int& DayOfWeek, int& Day, int& Hour, int& Min, int& Sec, int& MSec);

	static bool UpdateCPUTime(float DeltaTime);
	static FCPUTime GetCPUTime();

protected:

	/** Percentage CPU utilization for the last interval relative to one core. */
	static float CPUTimePctRelative;
};


typedef FWindowsPlatformTime FPlatformTime;
