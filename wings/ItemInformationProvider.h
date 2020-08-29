#pragma once

#include "ItemInfo.h"
#include "Equip.h"

class ItemInformationProvider
{
private:
	ItemInformationProvider() {}

public:
	static void registerItem(int id, ItemInfo* info);
	static ItemInfo* getItemInfo(int id);

	static Equip* createEquipById(int id);

	static Equip* randomizeStats(Equip* equip);
};