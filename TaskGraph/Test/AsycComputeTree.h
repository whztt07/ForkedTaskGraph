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
void Merge();
void MergeParallel();
void MergeParallelWithY();
void CompareResultY();
void CompareResult();
void QSort();