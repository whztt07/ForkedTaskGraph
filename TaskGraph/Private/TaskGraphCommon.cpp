#include "TaskGraphCommon.h"

unsigned int GGameThreadId = 0;
unsigned int GRenderThreadId = 0;
unsigned int GSlateLoadingThreadId = 0;
unsigned int GAudioThreadId = 0;
bool	GIsGameThreadIdInitialized = false;