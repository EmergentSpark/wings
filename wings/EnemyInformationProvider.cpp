#include "EnemyInformationProvider.h"

// TODO: use unordered_map<int, entry> for mob-specific droptables, i used this for simplicity
std::vector<std::unique_ptr<EnemyDropEntry>> enemyDropEntries;

void EnemyInformationProvider::addDropEntry(int itemId, int chance, int minimum, int maximum) { enemyDropEntries.push_back(std::unique_ptr<EnemyDropEntry>(new EnemyDropEntry(itemId, chance, minimum, maximum))); }

void EnemyInformationProvider::addDropEntry(int itemId, int chance) { addDropEntry(itemId, chance, 1, 1); }

std::vector<std::unique_ptr<EnemyDropEntry>>& EnemyInformationProvider::getDropEntries() { return enemyDropEntries; }