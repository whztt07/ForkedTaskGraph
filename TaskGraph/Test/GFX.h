#pragma once
#include <vector>
#include <iostream>
#include "TaskGraphInterfaces.h"
struct GFX
{
	GFX()
	{
		Pos = 0;
	}

	std::vector<GFX*> Children; // 子GFX
	GFX* ParentGFX;				// 父GFX
	int Pos;					// 当前位置 
	void Tick();				// 发起任务
	void CallBackInGameThread(); // 指定在GameThread执行的任务
	FGraphEventRef GFXWaitForTickComplete;    // Tick完成的事件句柄
	std::string Name;				// for debug
};

