#pragma once

#include "NpcInfo.h"

class NpcInformationProvider
{
private:
	NpcInformationProvider() {}

public:
	static void registerNpc(int id, NpcInfo* info);
	static NpcInfo* getNpcInfo(int id);
};