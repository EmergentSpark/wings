#pragma once

enum class InventoryType
{
	UNDEFINED,
	EQUIP,
	USE,
	SETUP,
	ETC,
	CASH,
	BANK,
	EQUIPPED = -1
};

static InventoryType getItemInventoryType(int itemId)
{
	InventoryType ret = InventoryType::UNDEFINED;

	int type = itemId / 1000000;
	if (type >= 1 && type <= 5) { ret = (InventoryType)type; }

	return ret;
}