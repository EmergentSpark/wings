#pragma once

#include <string>

class SkillInfo
{
private:
	std::string mName;

public:
	SkillInfo(const std::string& name) : mName(name) {}

	const std::string& getName() const { return mName; }

	virtual void attemptCast() = 0;
	virtual void drawIcon() = 0;
};