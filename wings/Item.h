#pragma once

#include "InventoryType.h"

class Item
{
private:
	int mId;
	short mPosition;
	short mQuantity;

public:
	Item(int id) : mId(id), mPosition(0), mQuantity(1) {}
	Item(int id, short quantity) : mId(id), mPosition(0), mQuantity(quantity) {}
	Item(int id, short position, short quantity) : mId(id), mPosition(position), mQuantity(quantity) {}

	const int& getItemId() { return mId; }
	const short& getPosition() { return mPosition; }
	const short& getQuantity() { return mQuantity; }

	void setPosition(short position) { mPosition = position; }
	void setQuantity(short quantity) { mQuantity = quantity; }

	InventoryType getInventoryType() { return getItemInventoryType(mId); }
};