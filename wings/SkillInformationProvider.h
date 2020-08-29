#pragma once

#include "SkillInfo.h"

class SkillInformationProvider
{
private:
	SkillInformationProvider() {}

public:
	static void registerSkill(int id, SkillInfo* skill);
	static SkillInfo* getSkillInfo(int id);
};