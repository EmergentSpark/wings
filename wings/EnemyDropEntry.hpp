#pragma once

class EnemyDropEntry
{
private:
	int mItemId;
	int mChance;
	int mMinimum;
	int mMaximum;

public:
	EnemyDropEntry(int itemId, int chance, int minimum, int maximum) : mItemId(itemId), mChance(chance), mMinimum(minimum), mMaximum(maximum) {}

	const int& getItemId() const { return mItemId; }
	const int& getChance() const { return mChance; }
	const int& getMinimum() const { return mMinimum; }
	const int& getMaximum() const { return mMaximum; }
};