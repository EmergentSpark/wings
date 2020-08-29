#pragma once

#include <glm/vec2.hpp>

struct KeyHash_GLMIVec2
{
	size_t operator()(const glm::ivec2& k) const
	{
		return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
	}
};

struct KeyEqual_GLMIVec2
{
	bool operator()(const glm::ivec2& a, const glm::ivec2& b) const
	{
		return a.x == b.x && a.y == b.y;
	}
};