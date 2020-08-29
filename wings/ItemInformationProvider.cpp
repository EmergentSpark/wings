#include "ItemInformationProvider.h"

#include <unordered_map>
#include <memory>
#include <algorithm>

#include "Randomizer.h"

std::unordered_map<int, std::unique_ptr<ItemInfo>> registeredItems;

int getRandStat(int defaultValue, int maxRange)
{
	if (defaultValue == 0) { return 0; }
	int lMaxRange = (int)std::min(std::ceil(defaultValue * 0.1), (double)maxRange);
	return (int)((defaultValue - lMaxRange) + std::floor(Randomizer::getRandomDouble() * (lMaxRange * 2 + 1)));
}

float getRandStatf(float defaultValue, float maxRange)
{
	if (defaultValue == 0) { return 0; }
	float lMaxRange = std::min(std::ceilf(defaultValue * 0.1f), maxRange);
	return ((defaultValue - lMaxRange) + std::floorf(Randomizer::getRandomFloat() * (lMaxRange * 2 + 1)));
}

void ItemInformationProvider::registerItem(int id, ItemInfo* info) { registeredItems[id].reset(info); }
ItemInfo* ItemInformationProvider::getItemInfo(int id) { return registeredItems.count(id) > 0 ? registeredItems[id].get() : 0; }

Equip* ItemInformationProvider::createEquipById(int id)
{
	Equip* equip = new Equip(id);

	if (id == 1492001)
	{
		equip->setAttackDamage(1);
	}
	else if (id == 1492002)
	{
		equip->setAttackDamage(2);
	}
	else if (id == 1492003)
	{
		equip->setAttackDamage(3);
	}
	else if (id == 1492004)
	{
		equip->setAttackDamage(2);
		equip->setMoveSpeed(1.0f);
	}
	else if (id == 1492005)
	{
		equip->setAttackDamage(3);
		equip->setMoveSpeed(2.0f);
	}
	else if (id == 1492006)
	{
		equip->setAttackDamage(4);
		equip->setMoveSpeed(3.0f);
	}

	return equip;
}

Equip* ItemInformationProvider::randomizeStats(Equip* equip)
{
	equip->setHP(getRandStat(equip->getHP(), 10));
	equip->setMP(getRandStat(equip->getMP(), 10));
	equip->setAttackDamage(getRandStat(equip->getAttackDamage(), 5));
	equip->setMoveSpeed(getRandStatf(equip->getMoveSpeed(), 5));

	return equip;
}