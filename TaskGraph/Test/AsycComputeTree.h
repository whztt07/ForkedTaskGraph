#pragma once
#include <vector>
#include "TaskGraphInterfaces.h"
class AsycComputeTree
{
public:
	AsycComputeTree();
	~AsycComputeTree();

};

void AllocResource();
void MergeRecursive();
void MergeParallelUE();
void MergeParallelWithY();
void CompareResultY();
void CompareResult();
void QSort();