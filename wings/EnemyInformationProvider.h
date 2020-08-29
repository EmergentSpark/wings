#pragma once

#include <vector>
#include <memory>

#include "EnemyDropEntry.h"

class EnemyInformationProvider
{
private:
	EnemyInformationProvider() {}

public:
	static void addDropEntry(int itemId, int chance, int minimum, int maximum);
	static void addDropEntry(int itemId, int chance);
	static std::vector<std::unique_ptr<EnemyDropEntry>>& getDropEntries();
};