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

	std::vector<GFX*> Children; // ��GFX
	GFX* ParentGFX;				// ��GFX
	int Pos;					// ��ǰλ�� 
	void Tick();				// ��������
	void CallBackInGameThread(); // ָ����GameThreadִ�е�����
	FGraphEventRef GFXWaitForTickComplete;    // Tick��ɵ��¼����
	std::string Name;				// for debug
};

