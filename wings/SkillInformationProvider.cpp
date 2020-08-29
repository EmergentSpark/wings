#include "SkillInformationProvider.h"

#include <unordered_map>
#include <memory>

std::unordered_map<int, std::unique_ptr<SkillInfo>> loadedSkills;

void SkillInformationProvider::registerSkill(int id, SkillInfo* skill) { loadedSkills[id].reset(skill); }

SkillInfo* SkillInformationProvider::getSkillInfo(int id)
{
	auto info = loadedSkills.find(id);
	if (info != loadedSkills.end()) { return info->second.get(); }
	else { return 0; }
}