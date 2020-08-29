#include "NpcInformationProvider.h"

#include <unordered_map>
#include <memory>

std::unordered_map<int, std::unique_ptr<NpcInfo>> registeredNPCs;

void NpcInformationProvider::registerNpc(int id, NpcInfo* info) { registeredNPCs[id].reset(info); }

NpcInfo* NpcInformationProvider::getNpcInfo(int id) { return registeredNPCs[id].get(); }